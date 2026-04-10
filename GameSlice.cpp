// ============================================================
//  GameSlice.cpp
//  メッシュ切断・肉片（Gib）物理・切断ピース描画
//
//  【責務】
//  - 盾投げによるリアルタイムメッシュ切断
//  - 切断ピースの物理演算（重力・回転・床衝突）
//  - Bullet Physicsベースの肉片（Gib）生成・更新・描画
//  - 切断用レンダリングリソースの初期化
//
//  【設計意図】
//  ゴア表現はゲームの「手触り」に直結する重要な要素。
//  MeshSlicerクラスが数学的な切断処理を担当し、
//  このファイルはGPUバッファ作成と物理演算を担当する。
//  切断 → パーツ生成 → 物理シミュレーション → 描画 の
//  一連のパイプラインがこの1ファイルに集約されている。
//
//  【技術的なポイント】
//  - 切断面の法線は盾の飛行方向から動的に計算
//  - アニメーション適用済み頂点を取得してから切断
//  - 着地した肉片は物理ワールドから除去（CPU負荷軽減）
// ============================================================

#include "Game.h"
#include "MeshSlicer.h"
#include <algorithm>

using namespace DirectX;

// ============================================================
//  定数定義
// ============================================================
namespace SliceConstants
{
    // 切断ピースの寿命と物理パラメータ
    constexpr float PIECE_LIFETIME = 5.0f;          // 切断ピースの生存時間（秒）
    constexpr float PIECE_FADE_THRESHOLD = 1.0f;    // フェード開始する残り寿命
    constexpr float GRAVITY = -9.8f;                // 重力加速度 (m/s?)
    constexpr float FLOOR_FRICTION = 0.9f;          // 床との摩擦（速度に乗算）
    constexpr float ANGULAR_DAMPING = 0.95f;        // 回転の減衰率
    constexpr float MAX_ANGULAR_VEL = 5.0f;         // 最大回転速度（rad/s）

    // 肉片（Gib）のパラメータ
    constexpr int   GIB_MAX_COUNT = 100;            // 同時存在の上限
    constexpr float GIB_RESTITUTION = 0.3f;         // 反発係数（少し弾む）
    constexpr float GIB_FRICTION = 0.8f;            // 摩擦係数（地面で止まる）
    constexpr float GIB_LAND_HEIGHT = 0.5f;         // 着地判定の高さ閾値
    constexpr float GIB_LAND_SPEED = 3.0f;          // 着地判定の速度閾値
    constexpr float GIB_FADE_THRESHOLD = 1.0f;      // フェード開始する残り寿命

    // 敵モデルのスケール（描画時と一致させること）
    constexpr float MODEL_SCALE = 0.01f;

    // 切断面の傾き係数（盾のXZ方向が切断面に与える影響）
    constexpr float SLICE_TILT_FACTOR = 0.3f;

    // 切断位置のクランプ範囲（体の20%?80%で切る）
    constexpr float SLICE_MIN_RATIO = 0.2f;
    constexpr float SLICE_MAX_RATIO = 0.8f;

    // AABB計算用のセンチネル値
    constexpr float FLOAT_SENTINEL = 99999.0f;

    // ボーン空間に圧縮されたメッシュの判定閾値
    constexpr float COMPRESSED_MESH_THRESHOLD = 0.1f;
}

// ============================================================
//  InitSliceRendering - 切断描画用のエフェクトとレイアウト作成
//
//  【呼び出し】Game::CreateRenderResources() 内で1回
//  【作成するもの】
//  - テクスチャあり用 BasicEffect + InputLayout
//  - テクスチャなし用 BasicEffect + InputLayout（断面用）
//  - 両面描画用のラスタライザステート
// ============================================================
void Game::InitSliceRendering()
{
    // テクスチャあり Effect（切断面以外の元のメッシュ部分）
    m_sliceEffectTex = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_sliceEffectTex->SetLightingEnabled(true);
    m_sliceEffectTex->SetLightEnabled(0, true);
    m_sliceEffectTex->SetLightDirection(0, XMVectorSet(0, -1, 0.3f, 0));
    m_sliceEffectTex->SetLightDiffuseColor(0, XMVectorSet(1, 0.9f, 0.8f, 1));
    m_sliceEffectTex->SetAmbientLightColor(XMVectorSet(0.4f, 0.4f, 0.4f, 1));
    m_sliceEffectTex->SetTextureEnabled(true);

    // テクスチャなし Effect（断面の蓋ポリゴン用）
    m_sliceEffectNoTex = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_sliceEffectNoTex->SetLightingEnabled(true);
    m_sliceEffectNoTex->SetLightEnabled(0, true);
    m_sliceEffectNoTex->SetLightDirection(0, XMVectorSet(0, -1, 0.3f, 0));
    m_sliceEffectNoTex->SetLightDiffuseColor(0, XMVectorSet(1, 0.9f, 0.8f, 1));
    m_sliceEffectNoTex->SetAmbientLightColor(XMVectorSet(0.4f, 0.4f, 0.4f, 1));
    m_sliceEffectNoTex->SetTextureEnabled(false);

    // InputLayout（テクスチャあり）
    void const* shaderByteCode;
    size_t byteCodeLength;
    m_sliceEffectTex->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    m_d3dDevice->CreateInputLayout(
        VertexPositionNormalTexture::InputElements,
        VertexPositionNormalTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_sliceInputLayoutTex.ReleaseAndGetAddressOf());

    // InputLayout（テクスチャなし）
    m_sliceEffectNoTex->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    m_d3dDevice->CreateInputLayout(
        VertexPositionNormalTexture::InputElements,
        VertexPositionNormalTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_sliceInputLayoutNoTex.ReleaseAndGetAddressOf());

    // 両面描画ラスタライザ（切断面は裏側も見えるため）
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    m_d3dDevice->CreateRasterizerState(&rsDesc, m_sliceNoCullRS.ReleaseAndGetAddressOf());
}

// ============================================================
//  TestMeshSlice - 切断アルゴリズムの動作テスト
//
//  【用途】デバッグ専用。2x2x2の立方体をY=0で水平切断し、
//  切断結果をOutputDebugStringAで出力する。
//  ゲームプレイ中は呼ばれない。
// ============================================================
void Game::TestMeshSlice()
{
    // テスト用の箱メッシュ（6面×4頂点）
    std::vector<SliceVertex> vertices = {
        {{ -1, -1,  1 }, { 0, 0, 1 }, { 0, 1 }}, {{  1, -1,  1 }, { 0, 0, 1 }, { 1, 1 }},
        {{  1,  1,  1 }, { 0, 0, 1 }, { 1, 0 }}, {{ -1,  1,  1 }, { 0, 0, 1 }, { 0, 0 }},
        {{  1, -1, -1 }, { 0, 0,-1 }, { 0, 1 }}, {{ -1, -1, -1 }, { 0, 0,-1 }, { 1, 1 }},
        {{ -1,  1, -1 }, { 0, 0,-1 }, { 1, 0 }}, {{  1,  1, -1 }, { 0, 0,-1 }, { 0, 0 }},
        {{ -1,  1,  1 }, { 0, 1, 0 }, { 0, 1 }}, {{  1,  1,  1 }, { 0, 1, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 0, 1, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, { 0, 1, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, { 0,-1, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 0,-1, 0 }, { 1, 1 }},
        {{  1, -1,  1 }, { 0,-1, 0 }, { 1, 0 }}, {{ -1, -1,  1 }, { 0,-1, 0 }, { 0, 0 }},
        {{  1, -1,  1 }, { 1, 0, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 1, 0, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 1, 0, 0 }, { 1, 0 }}, {{  1,  1,  1 }, { 1, 0, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, {-1, 0, 0 }, { 0, 1 }}, {{ -1, -1,  1 }, {-1, 0, 0 }, { 1, 1 }},
        {{ -1,  1,  1 }, {-1, 0, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, {-1, 0, 0 }, { 0, 0 }},
    };
    std::vector<uint32_t> indices = {
         0, 1, 2,  0, 2, 3,   4, 5, 6,  4, 6, 7,   8, 9,10,  8,10,11,
        12,13,14, 12,14,15,  16,17,18, 16,18,19,  20,21,22, 20,22,23,
    };

    XMFLOAT3 planePoint = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 planeNormal = { 0.0f, 1.0f, 0.0f };

    OutputDebugStringA("[TEST] Slicing cube at Y=0...\n");
    SliceResult result = MeshSlicer::Slice(vertices, indices, planePoint, planeNormal);

    if (result.success)
    {
        char buf[512];
        sprintf_s(buf,
            "[TEST] Slice SUCCESS! Upper: %zu verts/%zu tris, Lower: %zu verts/%zu tris\n",
            result.upperVertices.size(), result.upperIndices.size() / 3,
            result.lowerVertices.size(), result.lowerIndices.size() / 3);
        OutputDebugStringA(buf);
    }
    else
    {
        OutputDebugStringA("[TEST] Slice FAILED - no split occurred\n");
    }
}

// ============================================================
//  CreateSlicedPiece - 切断結果からGPUバッファを作成
//
//  【引数】vertices/indices: MeshSlicerの出力
//         position: ワールド上の初期位置
//         velocity: 初速度（吹っ飛び方向）
//  【戻り値】SlicedPiece構造体（active=falseなら作成失敗）
// ============================================================
Game::SlicedPiece Game::CreateSlicedPiece(
    const std::vector<SliceVertex>& vertices,
    const std::vector<uint32_t>& indices,
    XMFLOAT3 position,
    XMFLOAT3 velocity)
{
    SlicedPiece piece;
    piece.active = false;

    if (vertices.empty() || indices.empty()) return piece;

    // SliceVertex → DirectXTKのVertexPositionNormalTexture に変換
    std::vector<VertexPositionNormalTexture> dxVertices;
    dxVertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        VertexPositionNormalTexture dxv;
        dxv.position = v.position;
        dxv.normal = v.normal;
        dxv.textureCoordinate = v.uv;
        dxVertices.push_back(dxv);
    }

    // 頂点バッファ作成
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = (UINT)(dxVertices.size() * sizeof(VertexPositionNormalTexture));
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = dxVertices.data();

    HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbData,
        piece.vertexBuffer.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return piece;

    // インデックスバッファ作成
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = (UINT)(indices.size() * sizeof(uint32_t));
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibData,
        piece.indexBuffer.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return piece;

    piece.indexCount = (UINT)indices.size();
    piece.position = position;
    piece.velocity = velocity;
    piece.rotation = { 0, 0, 0 };

    // ランダムな回転速度（切断時のインパクト感を出す）
    piece.angularVel = {
        ((float)rand() / RAND_MAX - 0.5f) * SliceConstants::MAX_ANGULAR_VEL,
        ((float)rand() / RAND_MAX - 0.5f) * SliceConstants::MAX_ANGULAR_VEL,
        ((float)rand() / RAND_MAX - 0.5f) * SliceConstants::MAX_ANGULAR_VEL
    };

    piece.lifetime = SliceConstants::PIECE_LIFETIME;
    piece.active = true;

    return piece;
}

// ============================================================
//  ExecuteSliceTest - デバッグ用切断テスト実行
//
//  【呼び出し】キー入力で発動（デバッグ用）
//  プレイヤーの前方3mにキューブを生成して切断する
// ============================================================
void Game::ExecuteSliceTest()
{
    XMFLOAT3 playerPos = m_player->GetPosition();
    XMFLOAT3 playerRot = m_player->GetRotation();
    float fx = sinf(playerRot.y);
    float fz = cosf(playerRot.y);

    constexpr float TEST_DISTANCE = 3.0f;
    constexpr float TEST_BODY_CENTER_OFFSET = 0.9f;
    float cubeX = playerPos.x + fx * TEST_DISTANCE;
    float cubeY = playerPos.y - TEST_BODY_CENTER_OFFSET;
    float cubeZ = playerPos.z + fz * TEST_DISTANCE;

    // テスト用キューブの頂点データ（TestMeshSliceと同一）
    std::vector<SliceVertex> vertices = {
        {{ -1, -1,  1 }, { 0, 0, 1 }, { 0, 1 }}, {{  1, -1,  1 }, { 0, 0, 1 }, { 1, 1 }},
        {{  1,  1,  1 }, { 0, 0, 1 }, { 1, 0 }}, {{ -1,  1,  1 }, { 0, 0, 1 }, { 0, 0 }},
        {{  1, -1, -1 }, { 0, 0,-1 }, { 0, 1 }}, {{ -1, -1, -1 }, { 0, 0,-1 }, { 1, 1 }},
        {{ -1,  1, -1 }, { 0, 0,-1 }, { 1, 0 }}, {{  1,  1, -1 }, { 0, 0,-1 }, { 0, 0 }},
        {{ -1,  1,  1 }, { 0, 1, 0 }, { 0, 1 }}, {{  1,  1,  1 }, { 0, 1, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 0, 1, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, { 0, 1, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, { 0,-1, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 0,-1, 0 }, { 1, 1 }},
        {{  1, -1,  1 }, { 0,-1, 0 }, { 1, 0 }}, {{ -1, -1,  1 }, { 0,-1, 0 }, { 0, 0 }},
        {{  1, -1,  1 }, { 1, 0, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 1, 0, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 1, 0, 0 }, { 1, 0 }}, {{  1,  1,  1 }, { 1, 0, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, {-1, 0, 0 }, { 0, 1 }}, {{ -1, -1,  1 }, {-1, 0, 0 }, { 1, 1 }},
        {{ -1,  1,  1 }, {-1, 0, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, {-1, 0, 0 }, { 0, 0 }},
    };
    std::vector<uint32_t> indices = {
         0, 1, 2,  0, 2, 3,   4, 5, 6,  4, 6, 7,   8, 9,10,  8,10,11,
        12,13,14, 12,14,15,  16,17,18, 16,18,19,  20,21,22, 20,22,23,
    };

    XMFLOAT3 planePoint = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 planeNormal = { 0.0f, 1.0f, 0.0f };

    SliceResult result = MeshSlicer::Slice(vertices, indices, planePoint, planeNormal);

    if (result.success)
    {
        // 上半分: 上に吹っ飛ぶ
        SlicedPiece upper = CreateSlicedPiece(
            result.upperVertices, result.upperIndices,
            { cubeX, cubeY + 0.5f, cubeZ },
            { 0.0f, 4.0f, 0.0f });
        if (upper.active) m_slicedPieces.push_back(std::move(upper));

        // 下半分: 少し横に落ちる
        SlicedPiece lower = CreateSlicedPiece(
            result.lowerVertices, result.lowerIndices,
            { cubeX, cubeY - 0.5f, cubeZ },
            { 1.0f, -1.0f, 0.0f });
        if (lower.active) m_slicedPieces.push_back(std::move(lower));

        OutputDebugStringA("[SLICE] Cube sliced! 2 pieces created.\n");
    }
}

// ============================================================
//  SliceEnemyWithShield - 盾投げで敵をリアルタイム切断
//
//  【処理フロー】
//  1. 敵タイプに応じた3Dモデルを選択
//  2. 全メッシュの頂点を収集（ボーン適用 or 生頂点）
//  3. 盾の軌道から切断面を計算
//  4. MeshSlicer::Slice() で切断実行
//  5. 上下2パーツのGPUバッファを作成
//  6. 血エフェクトを発生
//
//  【なぜGetAnimatedVerticesを使う？】
//  FBXモデルの頂点はボーン空間に圧縮されており、
//  生の頂点データでは正しい切断位置が得られない。
//  現在のアニメーションポーズを適用した頂点を使うことで、
//  プレイヤーが見ている姿勢通りに切断される。
// ============================================================
void Game::SliceEnemyWithShield(Enemy& enemy, XMFLOAT3 shieldDir)
{
    // --- 敵タイプに対応するモデルを選択 ---
    Model* enemyModel = nullptr;
    switch (enemy.type)
    {
    case EnemyType::NORMAL: enemyModel = m_enemyModel.get(); break;
    case EnemyType::RUNNER: enemyModel = m_runnerModel.get(); break;
    case EnemyType::TANK:   enemyModel = m_tankModel.get(); break;
    default: return;  // BOSS/MIDBOSSは切断しない（演出上の理由）
    }
    if (!enemyModel || enemyModel->GetMeshes().empty()) return;

    // --- モデルの全メッシュから頂点を収集 ---
    std::vector<SliceVertex> allVertices;
    std::vector<uint32_t> allIndices;

    for (int mi = 0; mi < (int)enemyModel->GetMeshes().size(); mi++)
    {
        const auto& mesh = enemyModel->GetMeshes()[mi];
        uint32_t baseVertex = (uint32_t)allVertices.size();

        // 生頂点のY範囲をチェック（ボーン圧縮されているか判定）
        float rawMinY = SliceConstants::FLOAT_SENTINEL;
        float rawMaxY = -SliceConstants::FLOAT_SENTINEL;
        for (const auto& mv : mesh.vertices)
        {
            float y = mv.position.y * SliceConstants::MODEL_SCALE;
            if (y < rawMinY) rawMinY = y;
            if (y > rawMaxY) rawMaxY = y;
        }
        float rawHeight = rawMaxY - rawMinY;

        // Y範囲が十分なら生の頂点を使用、
        // 小さすぎる（ボーン空間に圧縮）ならバインドポーズを使用
        bool needBoneTransform = (rawHeight < SliceConstants::COMPRESSED_MESH_THRESHOLD);

        if (needBoneTransform)
        {
            std::vector<ModelVertex> bindVerts = enemyModel->GetBindPoseVertices(mi);
            for (const auto& mv : bindVerts)
            {
                SliceVertex sv;
                sv.position.x = mv.position.x * SliceConstants::MODEL_SCALE;
                sv.position.y = mv.position.y * SliceConstants::MODEL_SCALE;
                sv.position.z = mv.position.z * SliceConstants::MODEL_SCALE;
                sv.normal = mv.normal;
                sv.uv = mv.texCoord;
                allVertices.push_back(sv);
            }
        }
        else
        {
            std::vector<ModelVertex> animVerts =
                enemyModel->GetAnimatedVertices(mi, enemy.currentAnimation, enemy.animationTime);
            for (const auto& mv : animVerts)
            {
                SliceVertex sv;
                sv.position.x = mv.position.x * SliceConstants::MODEL_SCALE;
                sv.position.y = mv.position.y * SliceConstants::MODEL_SCALE;
                sv.position.z = mv.position.z * SliceConstants::MODEL_SCALE;
                sv.normal = mv.normal;
                sv.uv = mv.texCoord;
                allVertices.push_back(sv);
            }
        }

        for (const auto& idx : mesh.indices)
            allIndices.push_back(baseVertex + idx);
    }

    if (allVertices.empty()) return;

    // --- 切断面の計算 ---
    EnemyTypeConfig config = GetEnemyConfig(enemy.type);

    // 盾→敵のレイキャストで正確な衝突点を取得
    float hitRelativeY = config.bodyHeight * 0.4f;  // デフォルト（腰あたり）

    if (m_dynamicsWorld)
    {
        btVector3 rayFrom(m_thrownShieldPos.x, m_thrownShieldPos.y, m_thrownShieldPos.z);
        btVector3 rayTo(enemy.position.x,
            enemy.position.y + config.bodyHeight * 0.5f,
            enemy.position.z);

        // 対象の敵のカプセルだけに当てるカスタムコールバック
        struct EnemyRayCallback : public btCollisionWorld::ClosestRayResultCallback
        {
            int targetID;
            EnemyRayCallback(const btVector3& from, const btVector3& to, int id)
                : ClosestRayResultCallback(from, to), targetID(id) {
            }

            btScalar addSingleResult(btCollisionWorld::LocalRayResult& result,
                bool normalInWorldSpace) override
            {
                void* ptr = result.m_collisionObject->getUserPointer();
                if (!ptr) return 1.0f;
                int id = (int)(intptr_t)ptr - 1;
                if (id != targetID) return 1.0f;
                return ClosestRayResultCallback::addSingleResult(result, normalInWorldSpace);
            }
        };

        EnemyRayCallback callback(rayFrom, rayTo, enemy.id);
        m_dynamicsWorld->rayTest(rayFrom, rayTo, callback);

        if (callback.hasHit())
            hitRelativeY = callback.m_hitPointWorld.getY() - enemy.position.y;
    }

    // --- モデルのY範囲を取得してスライス高さを決定 ---
    float modelMinY = SliceConstants::FLOAT_SENTINEL;
    float modelMaxY = -SliceConstants::FLOAT_SENTINEL;
    for (const auto& v : allVertices)
    {
        if (v.position.y < modelMinY) modelMinY = v.position.y;
        if (v.position.y > modelMaxY) modelMaxY = v.position.y;
    }
    float modelHeight = modelMaxY - modelMinY;

    // hitRelativeYをモデルのY範囲にマッピング
    float hitRatio = hitRelativeY / config.bodyHeight;
    hitRatio = max(SliceConstants::SLICE_MIN_RATIO, min(SliceConstants::SLICE_MAX_RATIO, hitRatio));
    float sliceHeight = modelMinY + modelHeight * hitRatio;

    // --- 切断面の法線（盾の飛行方向で少し傾ける） ---
    XMFLOAT3 sliceNormal;
    sliceNormal.x = -shieldDir.z * SliceConstants::SLICE_TILT_FACTOR;
    sliceNormal.y = 1.0f;  // 基本は水平切断
    sliceNormal.z = shieldDir.x * SliceConstants::SLICE_TILT_FACTOR;

    // 法線を正規化
    float nLen = sqrtf(sliceNormal.x * sliceNormal.x +
        sliceNormal.y * sliceNormal.y +
        sliceNormal.z * sliceNormal.z);
    sliceNormal.x /= nLen;
    sliceNormal.y /= nLen;
    sliceNormal.z /= nLen;

    XMFLOAT3 planePoint = { 0.0f, sliceHeight, 0.0f };

    // --- 切断実行 ---
    SliceResult result = MeshSlicer::Slice(allVertices, allIndices, planePoint, sliceNormal);
    if (!result.success) return;

    // --- 2パーツを生成 ---
    float centerY = enemy.position.y + config.bodyHeight * 0.5f;
    ID3D11ShaderResourceView* tex = enemyModel->GetTexture();

    // 上パーツ（法線方向 + 盾方向に吹っ飛ぶ）
    XMFLOAT3 upperVel = {
        sliceNormal.x * 3.0f + shieldDir.x * 2.0f,
        3.0f + sliceNormal.y * 2.0f,
        sliceNormal.z * 3.0f + shieldDir.z * 2.0f
    };
    SlicedPiece upper = CreateSlicedPiece(
        result.upperVertices, result.upperIndices,
        { enemy.position.x, centerY, enemy.position.z }, upperVel);
    if (upper.active)
    {
        upper.texture = tex;
        m_slicedPieces.push_back(std::move(upper));
    }

    // 下パーツ（法線の反対方向にゆっくり倒れる）
    XMFLOAT3 lowerVel = {
        -sliceNormal.x * 2.0f,
        -1.0f,
        -sliceNormal.z * 2.0f
    };
    SlicedPiece lower = CreateSlicedPiece(
        result.lowerVertices, result.lowerIndices,
        { enemy.position.x, centerY, enemy.position.z }, lowerVel);
    if (lower.active)
    {
        lower.texture = tex;
        m_slicedPieces.push_back(std::move(lower));
    }

    // --- 元の敵を非表示にして血エフェクト ---
    enemy.isExploded = true;

    XMFLOAT3 hitPos = { enemy.position.x, centerY, enemy.position.z };
    m_gpuParticles->EmitSplash(hitPos, shieldDir, 60, 8.0f);
    m_gpuParticles->EmitMist(hitPos, 200, 4.0f);
    m_gpuParticles->Emit(hitPos, 100, 6.0f);
    m_bloodSystem->OnEnemyKilled(enemy.position, m_player->GetPosition());
}

// ============================================================
//  UpdateSlicedPieces - 切断ピースの物理更新
//
//  【毎フレーム】重力・位置・回転を更新し、床で止める
//  【寿命管理】lifetime切れのピースを配列から削除
// ============================================================
void Game::UpdateSlicedPieces(float deltaTime)
{
    for (auto& piece : m_slicedPieces)
    {
        if (!piece.active) continue;

        // 重力
        piece.velocity.y += SliceConstants::GRAVITY * deltaTime;

        // 位置更新
        piece.position.x += piece.velocity.x * deltaTime;
        piece.position.y += piece.velocity.y * deltaTime;
        piece.position.z += piece.velocity.z * deltaTime;

        // 回転更新
        piece.rotation.x += piece.angularVel.x * deltaTime;
        piece.rotation.y += piece.angularVel.y * deltaTime;
        piece.rotation.z += piece.angularVel.z * deltaTime;

        // 床で止まる（GetMeshFloorHeightで実際の地面高さを取得）
        float floorY = GetMeshFloorHeight(piece.position.x, piece.position.z, piece.position.y);
        if (piece.position.y < floorY)
        {
            piece.position.y = floorY;
            piece.velocity.y = 0.0f;
            piece.velocity.x *= SliceConstants::FLOOR_FRICTION;
            piece.velocity.z *= SliceConstants::FLOOR_FRICTION;
            piece.angularVel.x *= SliceConstants::ANGULAR_DAMPING;
            piece.angularVel.y *= SliceConstants::ANGULAR_DAMPING;
            piece.angularVel.z *= SliceConstants::ANGULAR_DAMPING;
        }

        // 寿命
        piece.lifetime -= deltaTime;
        if (piece.lifetime <= 0.0f)
            piece.active = false;
    }

    // 死んだピースを配列から削除（erase-remove イディオム）
    m_slicedPieces.erase(
        std::remove_if(m_slicedPieces.begin(), m_slicedPieces.end(),
            [](const SlicedPiece& p) { return !p.active; }),
        m_slicedPieces.end());
}

// ============================================================
//  DrawSlicedPieces - 切断ピースの描画
//
//  【描画方式】テクスチャありとなしでEffectを切り替え
//  【フェード】残り寿命1秒以下でアルファが下がる
// ============================================================
void Game::DrawSlicedPieces(XMMATRIX view, XMMATRIX proj)
{
    if (m_slicedPieces.empty()) return;
    if (!m_sliceEffectTex) return;

    // 両面描画ON（切断面は裏側も見える）
    m_d3dContext->RSSetState(m_sliceNoCullRS.Get());

    UINT stride = sizeof(VertexPositionNormalTexture);
    UINT offset = 0;

    for (const auto& piece : m_slicedPieces)
    {
        if (!piece.active) continue;

        XMMATRIX world =
            XMMatrixRotationX(piece.rotation.x) *
            XMMatrixRotationY(piece.rotation.y) *
            XMMatrixRotationZ(piece.rotation.z) *
            XMMatrixTranslation(piece.position.x, piece.position.y, piece.position.z);

        // 寿命が1秒以下でフェードアウト
        float alpha = (piece.lifetime < SliceConstants::PIECE_FADE_THRESHOLD)
            ? piece.lifetime : 1.0f;

        if (piece.texture)
        {
            m_sliceEffectTex->SetTexture(piece.texture);
            m_sliceEffectTex->SetDiffuseColor(XMVectorSet(1, 1, 1, alpha));
            m_sliceEffectTex->SetWorld(world);
            m_sliceEffectTex->SetView(view);
            m_sliceEffectTex->SetProjection(proj);
            m_sliceEffectTex->Apply(m_d3dContext.Get());
            m_d3dContext->IASetInputLayout(m_sliceInputLayoutTex.Get());
        }
        else
        {
            m_sliceEffectNoTex->SetDiffuseColor(XMVectorSet(0.8f, 0.2f, 0.2f, alpha));
            m_sliceEffectNoTex->SetWorld(world);
            m_sliceEffectNoTex->SetView(view);
            m_sliceEffectNoTex->SetProjection(proj);
            m_sliceEffectNoTex->Apply(m_d3dContext.Get());
            m_d3dContext->IASetInputLayout(m_sliceInputLayoutNoTex.Get());
        }

        ID3D11Buffer* vb = piece.vertexBuffer.Get();
        m_d3dContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        m_d3dContext->IASetIndexBuffer(piece.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_d3dContext->DrawIndexed(piece.indexCount, 0, 0);
    }

    m_d3dContext->RSSetState(nullptr);
}

// ============================================================
//  SpawnGibs - Bullet Physicsで肉片を生成
//
//  【仕組み】
//  ダイナミック剛体（mass>0）のBoxを複数生成し、
//  ランダムな方向にインパルスを与える。
//  Bulletが重力・衝突・回転を自動計算してくれる。
//
//  【サイズ分布】
//  最初の3個 = 大パーツ（胴体片）
//  次の4個   = 中パーツ（手足）
//  残り      = 小パーツ（肉片）
// ============================================================
void Game::SpawnGibs(XMFLOAT3 position, int count, float power)
{
    // 上限を超えたら古いのを削除（メモリ管理）
    while (m_gibs.size() > SliceConstants::GIB_MAX_COUNT)
    {
        auto& old = m_gibs.front();
        if (old.body)
        {
            m_dynamicsWorld->removeRigidBody(old.body);
            delete old.body->getMotionState();
            delete old.body;
            delete old.shape;
        }
        m_gibs.erase(m_gibs.begin());
    }

    // 肉片の色パターン（血の赤?暗い肉色）
    constexpr XMFLOAT4 GIB_COLORS[] = {
        { 0.85f, 0.10f, 0.05f, 1.0f },  // 暗い赤
        { 0.95f, 0.15f, 0.05f, 1.0f },  // 血の赤
        { 0.75f, 0.05f, 0.05f, 1.0f },  // 深い赤
        { 0.90f, 0.25f, 0.10f, 1.0f },  // 肉色
        { 0.70f, 0.08f, 0.08f, 1.0f },  // ほぼ黒赤
        { 1.00f, 0.20f, 0.10f, 1.0f },  // 明るい赤
    };
    constexpr int GIB_COLOR_COUNT = sizeof(GIB_COLORS) / sizeof(GIB_COLORS[0]);

    // サイズ区分の定数
    constexpr int LARGE_PARTS = 3;
    constexpr int MEDIUM_PARTS = 7;

    for (int i = 0; i < count; i++)
    {
        // サイズ（大→中→小の順で生成）
        float size;
        if (i < LARGE_PARTS)
            size = 0.25f + (float)rand() / RAND_MAX * 0.2f;
        else if (i < MEDIUM_PARTS)
            size = 0.14f + (float)rand() / RAND_MAX * 0.12f;
        else
            size = 0.07f + (float)rand() / RAND_MAX * 0.08f;

        btCollisionShape* shape = new btBoxShape(btVector3(size, size, size));

        // 初期位置（敵の位置からランダムオフセット）
        float offsetX = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
        float offsetY = 0.5f + (float)rand() / RAND_MAX * 1.0f;
        float offsetZ = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(
            position.x + offsetX,
            position.y + offsetY,
            position.z + offsetZ));

        // ランダム回転
        btQuaternion rotation(
            (float)rand() / RAND_MAX * 3.14f,
            (float)rand() / RAND_MAX * 3.14f,
            (float)rand() / RAND_MAX * 3.14f);
        transform.setRotation(rotation);

        // ダイナミック剛体（mass > 0 → 重力で落ちる）
        btScalar mass = 0.5f + (float)rand() / RAND_MAX * 1.5f;
        btVector3 localInertia(0, 0, 0);
        shape->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState* motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
        rbInfo.m_restitution = SliceConstants::GIB_RESTITUTION;
        rbInfo.m_friction = SliceConstants::GIB_FRICTION;

        btRigidBody* body = new btRigidBody(rbInfo);

        // 爆発的な力を加える（放射状 + 上向き）
        float angleXZ = ((float)rand() / RAND_MAX) * 6.28f;
        float upForce = power * (0.6f + (float)rand() / RAND_MAX * 0.8f);
        btVector3 impulse(
            cosf(angleXZ) * power * (0.5f + (float)rand() / RAND_MAX),
            upForce,
            sinf(angleXZ) * power * (0.5f + (float)rand() / RAND_MAX));
        body->applyCentralImpulse(impulse);

        // ランダム回転力（グルグル回る演出）
        btVector3 torque(
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f,
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f,
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f);
        body->applyTorqueImpulse(torque);

        m_dynamicsWorld->addRigidBody(body);

        // Gib構造体に保存
        Gib gib;
        gib.body = body;
        gib.shape = shape;
        gib.lifetime = 3.0f + (float)rand() / RAND_MAX * 2.0f;
        gib.size = size;
        gib.color = GIB_COLORS[i % GIB_COLOR_COUNT];
        gib.hasLanded = false;
        m_gibs.push_back(gib);
    }
}

// ============================================================
//  UpdateGibs - 肉片の更新（寿命管理 + 着地最適化）
//
//  【着地最適化】速度が閾値以下になったら物理ワールドから除去。
//  最終位置を保存して静的に描画する（CPU負荷軽減）。
//  100個の肉片が全て物理演算中だとFPSに影響するため。
// ============================================================
void Game::UpdateGibs(float deltaTime)
{
    for (auto it = m_gibs.begin(); it != m_gibs.end();)
    {
        it->lifetime -= deltaTime;

        if (it->lifetime <= 0.0f)
        {
            // 寿命切れ → 物理オブジェクトを解放
            if (it->body)
            {
                m_dynamicsWorld->removeRigidBody(it->body);
                delete it->body->getMotionState();
                delete it->body;
                delete it->shape;
            }
            it = m_gibs.erase(it);
        }
        else
        {
            // 着地判定：高さと速度が閾値以下なら物理を停止
            if (!it->hasLanded && it->body)
            {
                btTransform t;
                it->body->getMotionState()->getWorldTransform(t);
                float y = t.getOrigin().getY();
                float speed = it->body->getLinearVelocity().length();

                if (y < SliceConstants::GIB_LAND_HEIGHT &&
                    speed < SliceConstants::GIB_LAND_SPEED)
                {
                    it->hasLanded = true;

                    // 最終位置を保存
                    it->finalPos = { t.getOrigin().getX(), t.getOrigin().getY(), t.getOrigin().getZ() };
                    btQuaternion rot = t.getRotation();
                    it->finalRot = { rot.x(), rot.y(), rot.z(), rot.w() };

                    // 物理ワールドから除去（CPU負荷軽減）
                    m_dynamicsWorld->removeRigidBody(it->body);
                    delete it->body->getMotionState();
                    delete it->body;
                    delete it->shape;
                    it->body = nullptr;
                    it->shape = nullptr;
                }
            }
            ++it;
        }
    }
}

// ============================================================
//  DrawGibs - 肉片キューブの描画
//
//  【描画方式】GeometricPrimitive::CreateBox を使い回す
//  【位置取得】飛行中=Bulletから、着地済み=保存した位置から
//  【フェード】残り1秒でアルファが下がる
// ============================================================
void Game::DrawGibs(XMMATRIX view, XMMATRIX proj)
{
    if (m_gibs.empty()) return;

    for (const auto& gib : m_gibs)
    {
        XMMATRIX scale = XMMatrixScaling(gib.size, gib.size, gib.size);
        XMMATRIX rotation;
        XMMATRIX translation;

        if (gib.body != nullptr)
        {
            // まだ飛んでる → Bulletから位置・回転を取得
            btTransform transform;
            gib.body->getMotionState()->getWorldTransform(transform);
            btVector3 pos = transform.getOrigin();
            btQuaternion rot = transform.getRotation();
            rotation = XMMatrixRotationQuaternion(
                XMVectorSet(rot.x(), rot.y(), rot.z(), rot.w()));
            translation = XMMatrixTranslation(pos.x(), pos.y(), pos.z());
        }
        else
        {
            // 着地済み → 保存した位置を使用
            rotation = XMMatrixRotationQuaternion(
                XMVectorSet(gib.finalRot.x, gib.finalRot.y, gib.finalRot.z, gib.finalRot.w));
            translation = XMMatrixTranslation(gib.finalPos.x, gib.finalPos.y, gib.finalPos.z);
        }

        XMMATRIX world = scale * rotation * translation;

        XMFLOAT4 color = gib.color;
        if (gib.lifetime < SliceConstants::GIB_FADE_THRESHOLD)
            color.w = gib.lifetime;

        m_cube->Draw(world, view, proj,
            XMVECTORF32{ color.x, color.y, color.z, color.w });
    }
}