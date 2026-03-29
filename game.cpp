// Game.cpp - 実装（基盤部分）

#include "Game.h"
#include <string>
#include <d3dcompiler.h>
#include <WICTextureLoader.h>
//#include <stdexcept>
//#include <algorithm>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// DirectXTK用
#pragma comment(lib, "DirectXTK.lib")

//  Effekseer
#pragma comment(lib, "Effekseer.lib")

//  Bullet Physics
#pragma comment(lib, "BulletDynamics.lib")
#pragma comment(lib, "BulletCollision.lib")
#pragma comment(lib, "LinearMath.lib")

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;



Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(1280),
    m_outputHeight(720),

    m_cube(nullptr),
    m_lastMouseState(false),
    m_score(0),
    m_damageDisplayTimer(0.0f),
    m_showDamageDisplay(false),
    m_damageValue(0),
    m_gameState(GameState::TITLE),
    m_fadeAlpha(0.0f),
    m_fadingIn(false),
    m_fadeActive(false),
    m_weaponSwayX(0.0f),
    m_weaponSwayY(0.0f),
    m_lastCameraRotX(0.0f),
    m_lastCameraRotY(0.0f),
    m_showMuzzleFlash(false),
    m_muzzleFlashTimer(0.0f),
    m_weaponSystem(std::make_unique<WeaponSystem>()),
    m_enemySystem(std::make_unique<EnemySystem>()),
    m_particleSystem(std::make_unique<ParticleSystem>()),
    m_waveManager(std::make_unique<WaveManager>()),
    m_styleRank(std::make_unique<StyleRankSystem>()),
    m_rankingSystem(std::make_unique<RankingSystem>()),
    m_player(std::make_unique<Player>()),
    m_uiSystem(std::make_unique<UISystem>(1280, 720)),
    m_shadow(nullptr),
    m_fpsTimer(0.0f),
    m_frameCount(0),
    m_currentFPS(60.0f),
    m_timeScale(1.0f),
    m_slowMoTimer(0.0f),
    m_cameraShake(0.0f),
    m_cameraShakeTimer(0.0f),
    m_hitStopTimer(0.0f),
    m_showDebugWindow(false),
    m_showHitboxes(false),
    m_showHeadHitboxes(false),
    m_showBulletTrajectory(false),
    m_showPhysicsHitboxes(false),
    m_debugRunnerSpeed(2.0f),
    m_debugTankSpeed(0.5f),
    m_debugRunnerHP(50.0f),
    m_debugTankHP(300.0f),
    m_debugHeadRadius(0.3f),
    m_useDebugHitboxes(false),
    m_normalConfigDebug(GetEnemyConfig(EnemyType::NORMAL)),
    m_runnerConfigDebug(GetEnemyConfig(EnemyType::RUNNER)),
    m_tankConfigDebug(GetEnemyConfig(EnemyType::TANK)),
    m_midbossConfigDebug(GetEnemyConfig(EnemyType::MIDBOSS)),
    m_bossConfigDebug(GetEnemyConfig(EnemyType::BOSS)),
    m_deltaTime(0.0f),
    m_accumulatedAnimTime(0.0f),
    m_gloryKillTargetID(-1),
    m_gloryKillRange(2.5f),
    m_gloryKillActive(false),
    m_gloryKillInvincibleTimer(0.0f),
    m_gloryKillFlashTimer(0.0f),
    m_gloryKillFlashAlpha(0.0f),
    m_gloryKillTargetEnemy(nullptr),
    m_gloryKillCameraActive(false),
    m_gloryKillCameraPos(0.0f, 0.0f, 0.0f),
    m_gloryKillCameraTarget(0.0f, 0.0f, 0.0f),
    m_gloryKillCameraLerpTime(0.0f),
    m_gloryKillDOFActive(false),
    m_gloryKillDOFIntensity(0.0f),
    m_gloryKillArmAnimActive(false),
    m_gloryKillArmAnimTime(0.0f),
    m_gloryKillArmPos(0.0f, 0.0f, 0.0f),
    m_gloryKillArmRot(0.0f, 0.0f, 0.0f)

{
   //   パフォーマンスカウンターの初期化
    QueryPerformanceFrequency(&m_performanceFrequency);
    QueryPerformanceCounter(&m_lastFrameTime);
    m_gibs.reserve(200);
    m_instWorlds.reserve(100);
    m_instColors.reserve(100);
    m_instAnims.reserve(100);
    m_instTimes.reserve(100);
    // 旧インスタンシング用バッファ予約
    m_normalDead.reserve(40);
    m_normalDeadHeadless.reserve(40);
    m_runnerDead.reserve(40);
    m_runnerDeadHeadless.reserve(40);
    m_tankAttackingHeadless.reserve(20);
    m_tankDead.reserve(20);
    m_tankDeadHeadless.reserve(20);
    m_midBossWalking.reserve(5);
    m_midBossAttacking.reserve(5);
    m_midBossWalkingHeadless.reserve(5);
    m_midBossAttackingHeadless.reserve(5);
    m_midBossDead.reserve(5);
    m_midBossDeadHeadless.reserve(5);
    m_bossWalking.reserve(3);
    m_bossAttackingJump.reserve(3);
    m_bossAttackingSlash.reserve(3);
    m_bossAttackingJumpHeadless.reserve(3);
    m_bossAttackingSlashHeadless.reserve(3);
    m_bossWalkingHeadless.reserve(3);
    m_bossDead.reserve(3);
    m_bossDeadHeadless.reserve(3);
}

Game::~Game()
{
    CleanupPhysics();
}


void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = (width > 1) ? width : 1;
    m_outputHeight = (height > 1) ? height : 1;

   
    CreateDevice();
    CreateResources();

    //  === Bullet Physics 初期化  ===
    InitPhysics();

    CreateRenderResources();  // 3D描画用の初期化

    m_states = std::make_unique<DirectX::CommonStates>(m_d3dDevice.Get());

    

    // === 血パーティクル用ソフト円テクスチャを生成 ===
    {
        const int texSize = 64;
        std::vector<uint32_t> texData(texSize * texSize);
        float center = texSize / 2.0f;

        for (int y = 0; y < texSize; y++)
        {
            for (int x = 0; x < texSize; x++)
            {
                float dx = (x - center) / center;
                float dy = (y - center) / center;
                float dist = sqrtf(dx * dx + dy * dy);

                // 中心が白、端に向かって透明になる円
                if (dist > 0.5f) { texData[y * texSize + x] = 0; continue; }  // 外半分バッサリ
                float alpha = 1.0f - (dist / 0.5f);  // 0?0.5を0?1に引き伸ばす
                alpha = alpha * alpha * alpha;  // 三乗で急速フェード

                uint8_t a = (uint8_t)(alpha * 255.0f);
                // RGBA: 白(255,255,255) × alpha
                texData[y * texSize + x] = (a << 24) | 0x00FFFFFF;
            }
        }

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = texSize;
        texDesc.Height = texSize;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = texData.data();
        initData.SysMemPitch = texSize * sizeof(uint32_t);

        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
        m_d3dDevice->CreateTexture2D(&texDesc, &initData, tex.GetAddressOf());
        m_d3dDevice->CreateShaderResourceView(tex.Get(), nullptr, m_bloodParticleSRV.GetAddressOf());
    }

    // === パーティクル用エフェクト（テクスチャ付きビルボード用）===
    m_particleEffect = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_particleEffect->SetVertexColorEnabled(true);
    m_particleEffect->SetTextureEnabled(true);
    m_particleEffect->SetTexture(m_bloodParticleSRV.Get());

    // 入力レイアウト作成
    void const* shaderByteCode;
    size_t byteCodeLength;
    m_particleEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    m_d3dDevice->CreateInputLayout(
        DirectX::VertexPositionColorTexture::InputElements,
        DirectX::VertexPositionColorTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_particleInputLayout.GetAddressOf()
    );

    m_bloodSystem = std::make_unique<BloodSystem>();
    m_bloodSystem->Initialize(
        m_d3dDevice.Get(),
        m_d3dContext.Get(),
        m_states.get(),
        m_bloodParticleSRV.Get(),
        m_particleEffect.get(),
        m_particleInputLayout.Get());

    m_gpuParticles = std::make_unique<GPUParticleSystem>();
    m_gpuParticles->Initialize(m_d3dDevice.Get(), m_d3dContext.Get(), 4096,
        m_outputWidth, m_outputHeight);

    

    // Initialize で初期化
    m_titleScene = std::make_unique<TitleScene>();
    m_titleScene->Initialize(m_d3dDevice.Get(),
        m_outputWidth, m_outputHeight);

}




void Game::TestMeshSlice()
{
    // === テスト用の箱メッシュ（2x2x2の立方体） ===
    std::vector<SliceVertex> vertices = {
        // 前面 (Z+)
        {{ -1, -1,  1 }, { 0, 0, 1 }, { 0, 1 }},
        {{  1, -1,  1 }, { 0, 0, 1 }, { 1, 1 }},
        {{  1,  1,  1 }, { 0, 0, 1 }, { 1, 0 }},
        {{ -1,  1,  1 }, { 0, 0, 1 }, { 0, 0 }},
        // 後面 (Z-)
        {{  1, -1, -1 }, { 0, 0,-1 }, { 0, 1 }},
        {{ -1, -1, -1 }, { 0, 0,-1 }, { 1, 1 }},
        {{ -1,  1, -1 }, { 0, 0,-1 }, { 1, 0 }},
        {{  1,  1, -1 }, { 0, 0,-1 }, { 0, 0 }},
        // 上面 (Y+)
        {{ -1,  1,  1 }, { 0, 1, 0 }, { 0, 1 }},
        {{  1,  1,  1 }, { 0, 1, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 0, 1, 0 }, { 1, 0 }},
        {{ -1,  1, -1 }, { 0, 1, 0 }, { 0, 0 }},
        // 下面 (Y-)
        {{ -1, -1, -1 }, { 0,-1, 0 }, { 0, 1 }},
        {{  1, -1, -1 }, { 0,-1, 0 }, { 1, 1 }},
        {{  1, -1,  1 }, { 0,-1, 0 }, { 1, 0 }},
        {{ -1, -1,  1 }, { 0,-1, 0 }, { 0, 0 }},
        // 右面 (X+)
        {{  1, -1,  1 }, { 1, 0, 0 }, { 0, 1 }},
        {{  1, -1, -1 }, { 1, 0, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 1, 0, 0 }, { 1, 0 }},
        {{  1,  1,  1 }, { 1, 0, 0 }, { 0, 0 }},
        // 左面 (X-)
        {{ -1, -1, -1 }, {-1, 0, 0 }, { 0, 1 }},
        {{ -1, -1,  1 }, {-1, 0, 0 }, { 1, 1 }},
        {{ -1,  1,  1 }, {-1, 0, 0 }, { 1, 0 }},
        {{ -1,  1, -1 }, {-1, 0, 0 }, { 0, 0 }},
    };

    std::vector<uint32_t> indices = {
         0, 1, 2,  0, 2, 3,   // 前面
         4, 5, 6,  4, 6, 7,   // 後面
         8, 9,10,  8,10,11,   // 上面
        12,13,14, 12,14,15,   // 下面
        16,17,18, 16,18,19,   // 右面
        20,21,22, 20,22,23,   // 左面
    };

    // === Y=0の水平面で切断 ===
    DirectX::XMFLOAT3 planePoint = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 planeNormal = { 0.0f, 1.0f, 0.0f };  // 上向き

    OutputDebugStringA("[TEST] Slicing cube at Y=0...\n");

    SliceResult result = MeshSlicer::Slice(vertices, indices, planePoint, planeNormal);

    if (result.success)
    {
        char buf[512];
        sprintf_s(buf,
            "[TEST] Slice SUCCESS!\n"
            "  Upper: %zu vertices, %zu triangles\n"
            "  Lower: %zu vertices, %zu triangles\n"
            "  Cross points: %zu\n",
            result.upperVertices.size(),
            result.upperIndices.size() / 3,
            result.lowerVertices.size(),
            result.lowerIndices.size() / 3,
            result.crossPoints.size());
        OutputDebugStringA(buf);
    }
    else
    {
        OutputDebugStringA("[TEST] Slice FAILED - no split occurred\n");
    }
}

// ============================================
//  切断結果からGPUバッファを作成
// ============================================
Game::SlicedPiece Game::CreateSlicedPiece(
    const std::vector<SliceVertex>& vertices,
    const std::vector<uint32_t>& indices,
    DirectX::XMFLOAT3 position,
    DirectX::XMFLOAT3 velocity)
{
    SlicedPiece piece;
    piece.active = false;

    if (vertices.empty() || indices.empty()) return piece;

    // DirectXTKのVertexPositionNormalTextureに変換
    std::vector<DirectX::VertexPositionNormalTexture> dxVertices;
    dxVertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        DirectX::VertexPositionNormalTexture dxv;
        dxv.position = v.position;
        dxv.normal = v.normal;
        dxv.textureCoordinate = v.uv;
        dxVertices.push_back(dxv);
    }

    // 頂点バッファ作成
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = (UINT)(dxVertices.size() * sizeof(DirectX::VertexPositionNormalTexture));
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

    // ランダムな回転速度
    piece.angularVel = {
        ((float)rand() / RAND_MAX - 0.5f) * 5.0f,
        ((float)rand() / RAND_MAX - 0.5f) * 5.0f,
        ((float)rand() / RAND_MAX - 0.5f) * 5.0f
    };

    piece.lifetime = 5.0f;
    piece.active = true;

    return piece;
}

// ============================================
//  切断テスト実行（プレイヤーの前のキューブを切る）
// ============================================
void Game::ExecuteSliceTest()
{
    // プレイヤーの前方3mにキューブを配置
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();
    float fx = sinf(playerRot.y);
    float fz = cosf(playerRot.y);

    float cubeX = playerPos.x + fx * 3.0f;
    float cubeY = playerPos.y - 0.9f;  // 体の中心あたり
    float cubeZ = playerPos.z + fz * 3.0f;

    // テスト用キューブの頂点データ
    std::vector<SliceVertex> vertices = {
        {{ -1, -1,  1 }, { 0, 0, 1 }, { 0, 1 }},
        {{  1, -1,  1 }, { 0, 0, 1 }, { 1, 1 }},
        {{  1,  1,  1 }, { 0, 0, 1 }, { 1, 0 }},
        {{ -1,  1,  1 }, { 0, 0, 1 }, { 0, 0 }},
        {{  1, -1, -1 }, { 0, 0,-1 }, { 0, 1 }},
        {{ -1, -1, -1 }, { 0, 0,-1 }, { 1, 1 }},
        {{ -1,  1, -1 }, { 0, 0,-1 }, { 1, 0 }},
        {{  1,  1, -1 }, { 0, 0,-1 }, { 0, 0 }},
        {{ -1,  1,  1 }, { 0, 1, 0 }, { 0, 1 }},
        {{  1,  1,  1 }, { 0, 1, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 0, 1, 0 }, { 1, 0 }},
        {{ -1,  1, -1 }, { 0, 1, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, { 0,-1, 0 }, { 0, 1 }},
        {{  1, -1, -1 }, { 0,-1, 0 }, { 1, 1 }},
        {{  1, -1,  1 }, { 0,-1, 0 }, { 1, 0 }},
        {{ -1, -1,  1 }, { 0,-1, 0 }, { 0, 0 }},
        {{  1, -1,  1 }, { 1, 0, 0 }, { 0, 1 }},
        {{  1, -1, -1 }, { 1, 0, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 1, 0, 0 }, { 1, 0 }},
        {{  1,  1,  1 }, { 1, 0, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, {-1, 0, 0 }, { 0, 1 }},
        {{ -1, -1,  1 }, {-1, 0, 0 }, { 1, 1 }},
        {{ -1,  1,  1 }, {-1, 0, 0 }, { 1, 0 }},
        {{ -1,  1, -1 }, {-1, 0, 0 }, { 0, 0 }},
    };

    std::vector<uint32_t> indices = {
         0, 1, 2,  0, 2, 3,
         4, 5, 6,  4, 6, 7,
         8, 9,10,  8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23,
    };

    // 水平面で切断（Y=0 = キューブの真ん中）
    DirectX::XMFLOAT3 planePoint = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 planeNormal = { 0.0f, 1.0f, 0.0f };

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

// ============================================
//  敵を盾の軌道で切断する
//  enemy:     切断される敵
//  shieldDir: 盾の飛行方向（切断面の算出に使う）
// ============================================
void Game::SliceEnemyWithShield(Enemy& enemy, DirectX::XMFLOAT3 shieldDir)
{
    // --- 敵タイプに対応するモデルを選択 ---
    Model* enemyModel = nullptr;
    switch (enemy.type)
    {
    case EnemyType::NORMAL: enemyModel = m_enemyModel.get(); break;
    case EnemyType::RUNNER: enemyModel = m_runnerModel.get(); break;
    case EnemyType::TANK:   enemyModel = m_tankModel.get(); break;
    default: return;  // BOSS/MIDBOSSは切断しない
    }
    if (!enemyModel || enemyModel->GetMeshes().empty()) return;

    // --- モデルの全メッシュから頂点を収集 ---
    std::vector<SliceVertex> allVertices;
    std::vector<uint32_t> allIndices;

    // モデルのスケール（描画時と同じ 0.01f）
    const float modelScale = 0.01f;

    for (int mi = 0; mi < (int)enemyModel->GetMeshes().size(); mi++)
    {
        const auto& mesh = enemyModel->GetMeshes()[mi];
        uint32_t baseVertex = (uint32_t)allVertices.size();

        // まず生の頂点のY範囲をチェック
        float rawMinY = 99999.0f, rawMaxY = -99999.0f;
        for (const auto& mv : mesh.vertices)
        {
            float y = mv.position.y * modelScale;
            if (y < rawMinY) rawMinY = y;
            if (y > rawMaxY) rawMaxY = y;
        }
        float rawHeight = rawMaxY - rawMinY;

        // Y範囲が十分（0.1m以上）なら生の頂点を使う
        // 小さすぎる（ボーン空間に圧縮されてる）ならGetBindPoseVerticesを使う
        bool needBoneTransform = (rawHeight < 0.1f);

        if (needBoneTransform)
        {
            OutputDebugStringA("[SLICE] Using GetBindPoseVertices (raw verts compressed)\n");
            std::vector<ModelVertex> bindVerts = enemyModel->GetBindPoseVertices(mi);
            for (const auto& mv : bindVerts)
            {
                SliceVertex sv;
                sv.position.x = mv.position.x * modelScale;
                sv.position.y = mv.position.y * modelScale;
                sv.position.z = mv.position.z * modelScale;
                sv.normal = mv.normal;
                sv.uv = mv.texCoord;
                allVertices.push_back(sv);
            }
        }
        else
        {
            OutputDebugStringA("[SLICE] Using animated vertices\n");

            std::vector<ModelVertex> animVerts =
                enemyModel->GetAnimatedVertices(mi, enemy.currentAnimation, enemy.animationTime);

            for (const auto& mv : animVerts)
            {
                SliceVertex sv;
                sv.position.x = mv.position.x * modelScale;
                sv.position.y = mv.position.y * modelScale;
                sv.position.z = mv.position.z * modelScale;
                sv.normal = mv.normal;
                sv.uv = mv.texCoord;
                allVertices.push_back(sv);
            }
        }

        for (const auto& idx : mesh.indices)
        {
            allIndices.push_back(baseVertex + idx);
        }
    }

    if (allVertices.empty()) return;

    // =======================================
    // === 徹底デバッグ ===
    // =======================================
    {
        float vMinX = 99999, vMaxX = -99999;
        float vMinY = 99999, vMaxY = -99999;
        float vMinZ = 99999, vMaxZ = -99999;
        float uvMinX = 99999, uvMaxX = -99999;
        float uvMinY = 99999, uvMaxY = -99999;
        int zeroUV = 0;
        for (const auto& v : allVertices)
        {
            if (v.position.x < vMinX) vMinX = v.position.x;
            if (v.position.x > vMaxX) vMaxX = v.position.x;
            if (v.position.y < vMinY) vMinY = v.position.y;
            if (v.position.y > vMaxY) vMaxY = v.position.y;
            if (v.position.z < vMinZ) vMinZ = v.position.z;
            if (v.position.z > vMaxZ) vMaxZ = v.position.z;
            if (v.uv.x < uvMinX) uvMinX = v.uv.x;
            if (v.uv.x > uvMaxX) uvMaxX = v.uv.x;
            if (v.uv.y < uvMinY) uvMinY = v.uv.y;
            if (v.uv.y > uvMaxY) uvMaxY = v.uv.y;
            if (v.uv.x == 0.0f && v.uv.y == 0.0f) zeroUV++;
        }

        char dbg[1024];
        sprintf_s(dbg,
            "[SLICE FULL DEBUG] ===========================\n"
            "  Enemy: type=%d id=%d anim='%s' time=%.3f\n"
            "  Vertices: %zu\n"
            "  XYZ: (%.3f~%.3f, %.3f~%.3f, %.3f~%.3f)\n"
            "  UV range: U(%.3f~%.3f) V(%.3f~%.3f)\n"
            "  Zero UV: %d / %zu\n"
            "  Texture: %s\n",
            (int)enemy.type, enemy.id,
            enemy.currentAnimation.c_str(), enemy.animationTime,
            allVertices.size(),
            vMinX, vMaxX, vMinY, vMaxY, vMinZ, vMaxZ,
            uvMinX, uvMaxX, uvMinY, uvMaxY,
            zeroUV, allVertices.size(),
            enemyModel->GetTexture() ? "YES" : "NO");
        OutputDebugStringA(dbg);
    }

    // --- 切断面の計算 ---
    EnemyTypeConfig config = GetEnemyConfig(enemy.type);

    // 盾が敵のどの高さに当たったかで切断位置を決定
    //    盾のY - 敵の足元Y = 敵の体のどこに当たったか
    // 盾→敵のレイキャストで正確な衝突点を取得
    float hitRelativeY = config.bodyHeight * 0.4f;  // デフォルト（腰）

    btVector3 rayFrom(m_thrownShieldPos.x, m_thrownShieldPos.y, m_thrownShieldPos.z);
    btVector3 rayTo(enemy.position.x,
        enemy.position.y + config.bodyHeight * 0.5f,
        enemy.position.z);

    // 敵のカプセルだけに当てるコールバック
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
            if (!ptr) return 1.0f;  // マップメッシュはスキップ
            int id = (int)(intptr_t)ptr - 1;
            if (id != targetID) return 1.0f;  // 他の敵もスキップ
            return ClosestRayResultCallback::addSingleResult(result, normalInWorldSpace);
        }
    };

    if (m_dynamicsWorld)
    {
        EnemyRayCallback callback(rayFrom, rayTo, enemy.id);
        m_dynamicsWorld->rayTest(rayFrom, rayTo, callback);

        if (callback.hasHit())
        {
            // 衝突点のY座標から敵のローカル高さを計算
            hitRelativeY = callback.m_hitPointWorld.getY() - enemy.position.y;
        }
    }

    // === モデルの実際のY範囲を取得 ===
    float modelMinY = 99999.0f, modelMaxY = -99999.0f;
    for (const auto& v : allVertices)
    {
        if (v.position.y < modelMinY) modelMinY = v.position.y;
        if (v.position.y > modelMaxY) modelMaxY = v.position.y;
    }
    float modelHeight = modelMaxY - modelMinY;

    // hitRelativeYをモデルのY範囲にマッピング
    // hitRelativeY は 敵のbodyHeight(config) の範囲 → モデルのY範囲に変換
    float hitRatio = hitRelativeY / config.bodyHeight;  // 0.0?1.0（体の下?上）
    hitRatio = max(0.2f, min(0.8f, hitRatio));           // 20%?80%にクランプ
    float sliceHeight = modelMinY + modelHeight * hitRatio;

    // === デバッグ ===
    {
        char dbg[512];
        sprintf_s(dbg,
            "[SLICE DEBUG] Type=%d, verts=%zu, tris=%zu\n"
            "  Model Y range: %.4f ~ %.4f (height=%.4f)\n"
            "  hitRelativeY=%.4f, hitRatio=%.2f, sliceHeight=%.4f\n",
            (int)enemy.type, allVertices.size(), allIndices.size() / 3,
            modelMinY, modelMaxY, modelHeight,
            hitRelativeY, hitRatio, sliceHeight);
        OutputDebugStringA(dbg);
    }

    // 盾の飛行方向から切断面の傾きを計算
    //    盾のY成分が大きいほど斜めに切れる
    DirectX::XMFLOAT3 sliceNormal;
    sliceNormal.x = -shieldDir.z * 0.3f;  // 盾のXZ方向で少し傾ける
    sliceNormal.y = 1.0f;                   // 基本は水平
    sliceNormal.z = shieldDir.x * 0.3f;

    // 法線を正規化
    float nLen = sqrtf(sliceNormal.x * sliceNormal.x +
        sliceNormal.y * sliceNormal.y +
        sliceNormal.z * sliceNormal.z);
    sliceNormal.x /= nLen;
    sliceNormal.y /= nLen;
    sliceNormal.z /= nLen;

    DirectX::XMFLOAT3 planePoint = { 0.0f, sliceHeight, 0.0f };

    // --- 切断実行 ---
    SliceResult result = MeshSlicer::Slice(allVertices, allIndices, planePoint, sliceNormal);

    if (!result.success)
    {
        OutputDebugStringA("[SLICE] Failed - model could not be split\n");
        return;
    }

    // --- 敵の位置に2パーツを生成 ---
    float centerY = enemy.position.y + config.bodyHeight * 0.5f;

    // テクスチャを取得
    ID3D11ShaderResourceView* tex = enemyModel->GetTexture();

    // 上パーツ
    DirectX::XMFLOAT3 upperVel = {
        sliceNormal.x * 3.0f + shieldDir.x * 2.0f,
        3.0f + sliceNormal.y * 2.0f,
        sliceNormal.z * 3.0f + shieldDir.z * 2.0f
    };
    SlicedPiece upper = CreateSlicedPiece(
        result.upperVertices, result.upperIndices,
        { enemy.position.x, centerY, enemy.position.z },
        upperVel);
    if (upper.active)
    {
        upper.texture = tex;
        m_slicedPieces.push_back(std::move(upper));
    }

    // 下パーツ
    DirectX::XMFLOAT3 lowerVel = {
        -sliceNormal.x * 2.0f,
        -1.0f,
        -sliceNormal.z * 2.0f
    };
    SlicedPiece lower = CreateSlicedPiece(
        result.lowerVertices, result.lowerIndices,
        { enemy.position.x, centerY, enemy.position.z },
        lowerVel);
    if (lower.active)
    {
        lower.texture = tex;
        m_slicedPieces.push_back(std::move(lower));
    }

    // --- 元の敵を非表示 ---
    enemy.isExploded = true;

    // --- 血エフェクト ---
    DirectX::XMFLOAT3 hitPos = { enemy.position.x, centerY, enemy.position.z };
    m_gpuParticles->EmitSplash(hitPos, shieldDir, 60, 8.0f);
    m_gpuParticles->EmitMist(hitPos, 200, 4.0f);
    m_gpuParticles->Emit(hitPos, 100, 6.0f);
    m_bloodSystem->OnEnemyKilled(enemy.position, m_player->GetPosition());

    char buf[256];
    sprintf_s(buf, "[SLICE] Enemy ID:%d sliced! upper=%zu lower=%zu tris\n",
        enemy.id,
        result.upperIndices.size() / 3,
        result.lowerIndices.size() / 3);
    OutputDebugStringA(buf);
}

// ============================================
//  切断ピースの更新（物理演算）
// ============================================
void Game::UpdateSlicedPieces(float deltaTime)
{
    float gravity = -9.8f;

    for (auto& piece : m_slicedPieces)
    {
        if (!piece.active) continue;

        // 重力
        piece.velocity.y += gravity * deltaTime;

        // 位置更新
        piece.position.x += piece.velocity.x * deltaTime;
        piece.position.y += piece.velocity.y * deltaTime;
        piece.position.z += piece.velocity.z * deltaTime;

        // 回転更新
        piece.rotation.x += piece.angularVel.x * deltaTime;
        piece.rotation.y += piece.angularVel.y * deltaTime;
        piece.rotation.z += piece.angularVel.z * deltaTime;

        // 床で止まる
        float floorY = GetMeshFloorHeight(piece.position.x, piece.position.z,
            piece.position.y);
        if (piece.position.y < floorY)
        {
            piece.position.y = floorY;
            piece.velocity.y = 0.0f;
            piece.velocity.x *= 0.9f;
            piece.velocity.z *= 0.9f;
            piece.angularVel.x *= 0.95f;
            piece.angularVel.y *= 0.95f;
            piece.angularVel.z *= 0.95f;
        }

        // 寿命
        piece.lifetime -= deltaTime;
        if (piece.lifetime <= 0.0f)
            piece.active = false;
    }

    // 死んだピースを削除
    m_slicedPieces.erase(
        std::remove_if(m_slicedPieces.begin(), m_slicedPieces.end(),
            [](const SlicedPiece& p) { return !p.active; }),
        m_slicedPieces.end());
}

// ============================================
//  切断ピースの描画
// ============================================
void Game::DrawSlicedPieces(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
{
    if (m_slicedPieces.empty()) return;
    if (!m_sliceEffectTex) return;

    m_d3dContext->RSSetState(m_sliceNoCullRS.Get());

    UINT stride = sizeof(DirectX::VertexPositionNormalTexture);
    UINT offset = 0;

    for (const auto& piece : m_slicedPieces)
    {
        if (!piece.active) continue;

        DirectX::XMMATRIX world =
            DirectX::XMMatrixRotationX(piece.rotation.x) *
            DirectX::XMMatrixRotationY(piece.rotation.y) *
            DirectX::XMMatrixRotationZ(piece.rotation.z) *
            DirectX::XMMatrixTranslation(
                piece.position.x, piece.position.y, piece.position.z);

        float alpha = (piece.lifetime < 1.0f) ? piece.lifetime : 1.0f;

        if (piece.texture)
        {
            m_sliceEffectTex->SetTexture(piece.texture);
            m_sliceEffectTex->SetDiffuseColor(DirectX::XMVectorSet(1, 1, 1, alpha));
            m_sliceEffectTex->SetWorld(world);
            m_sliceEffectTex->SetView(view);
            m_sliceEffectTex->SetProjection(proj);
            m_sliceEffectTex->Apply(m_d3dContext.Get());
            m_d3dContext->IASetInputLayout(m_sliceInputLayoutTex.Get());
        }
        else
        {
            m_sliceEffectNoTex->SetDiffuseColor(DirectX::XMVectorSet(0.8f, 0.2f, 0.2f, alpha));
            m_sliceEffectNoTex->SetWorld(world);
            m_sliceEffectNoTex->SetView(view);
            m_sliceEffectNoTex->SetProjection(proj);
            m_sliceEffectNoTex->Apply(m_d3dContext.Get());
            m_d3dContext->IASetInputLayout(m_sliceInputLayoutNoTex.Get());
        }

        ID3D11Buffer* vb = piece.vertexBuffer.Get();
        m_d3dContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        m_d3dContext->IASetIndexBuffer(piece.indexBuffer.Get(),
            DXGI_FORMAT_R32_UINT, 0);
        m_d3dContext->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_d3dContext->DrawIndexed(piece.indexCount, 0, 0);
    }

    m_d3dContext->RSSetState(nullptr);
}


//  SpawnGibs: 肉片をBullet物理で生成 
void Game::SpawnGibs(DirectX::XMFLOAT3 position, int count, float power)
{
    // 上限100個 ? 超えたら古いのを削除
    while (m_gibs.size() > 100)
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
    DirectX::XMFLOAT4 gibColors[] = {
        { 0.85f, 0.10f, 0.05f, 1.0f },  // 暗い赤
        { 0.95f, 0.15f, 0.05f, 1.0f },  // 血の赤
        { 0.75f, 0.05f, 0.05f, 1.0f },  // 深い赤
        { 0.90f, 0.25f, 0.10f, 1.0f },  // 肉色
        { 0.70f, 0.08f, 0.08f, 1.0f },  // ほぼ黒赤
        { 1.00f, 0.20f, 0.10f, 1.0f },  // 明るい赤
    };
    int colorCount = 6;

    for (int i = 0; i < count; i++)
    {
        // サイズ大きく（1.5?2倍に）
        float size;
        if (i < 3)
            size = 0.25f + (float)rand() / RAND_MAX * 0.2f;    // 大パーツ（胴体片）
        else if (i < 7)
            size = 0.14f + (float)rand() / RAND_MAX * 0.12f;   // 中パーツ（手足）
        else
            size = 0.07f + (float)rand() / RAND_MAX * 0.08f;   // 小パーツ（肉片）

        // Box形状
        btCollisionShape* shape = new btBoxShape(btVector3(size, size, size));

        // 初期位置（敵の位置からランダムオフセット）
        float offsetX = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
        float offsetY = 0.5f + (float)rand() / RAND_MAX * 1.0f;  // 体の高さ範囲
        float offsetZ = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(
            position.x + offsetX,
            position.y + offsetY,
            position.z + offsetZ
        ));

        // ランダム回転
        btQuaternion rotation(
            (float)rand() / RAND_MAX * 3.14f,
            (float)rand() / RAND_MAX * 3.14f,
            (float)rand() / RAND_MAX * 3.14f
        );
        transform.setRotation(rotation);

        // 動的剛体（mass > 0）
        btScalar mass = 0.5f + (float)rand() / RAND_MAX * 1.5f;
        btVector3 localInertia(0, 0, 0);
        shape->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState* motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
        rbInfo.m_restitution = 0.3f;    // 少し弾む
        rbInfo.m_friction = 0.8f;       // 地面で止まる

        btRigidBody* body = new btRigidBody(rbInfo);

        // 爆発的な力を加える（放射状 + 上向き）
        float angleXZ = ((float)rand() / RAND_MAX) * 6.28f;  // ランダム方向
        float upForce = power * (0.6f + (float)rand() / RAND_MAX * 0.8f);
        btVector3 impulse(
            cosf(angleXZ) * power * (0.5f + (float)rand() / RAND_MAX),
            upForce,
            sinf(angleXZ) * power * (0.5f + (float)rand() / RAND_MAX)
        );
        body->applyCentralImpulse(impulse);

        // ランダム回転力も加える（グルグル回る）
        btVector3 torque(
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f,
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f,
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f
        );
        body->applyTorqueImpulse(torque);

        m_dynamicsWorld->addRigidBody(body);

        // Gibを保存
        Gib gib;
        gib.body = body;
        gib.shape = shape;
        gib.lifetime = 3.0f + (float)rand() / RAND_MAX * 2.0f;  // 3?5秒で消える
        gib.size = size;
        gib.color = gibColors[i % colorCount];
        gib.hasLanded = false;
        m_gibs.push_back(gib);
    }
}

void Game::UpdateGibs(float deltaTime)
{
    for (auto it = m_gibs.begin(); it != m_gibs.end();)
    {
        it->lifetime -= deltaTime;

        if (it->lifetime <= 0.0f)
        {
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
            // 着地したら物理シミュレーションから外す（GPUに任せるだけ）
            if (!it->hasLanded)
            {
                btTransform t;
                it->body->getMotionState()->getWorldTransform(t);
                float y = t.getOrigin().getY();
                btVector3 vel = it->body->getLinearVelocity();
                float speed = vel.length();

                if (y < 0.5f && speed < 3.0f)
                {
                    it->hasLanded = true;

                    // 最終位置を保存
                    it->finalPos = { t.getOrigin().getX(), t.getOrigin().getY(), t.getOrigin().getZ() };
                    btQuaternion rot = t.getRotation();
                    it->finalRot = { rot.x(), rot.y(), rot.z(), rot.w() };

                    // 物理ワールドから除去（CPU負荷激減）
                    m_dynamicsWorld->removeRigidBody(it->body);
                    delete it->body->getMotionState();
                    delete it->body;
                    delete it->shape;
                    it->body = nullptr;
                    it->shape = nullptr;

                    DirectX::XMFLOAT3 landPos = it->finalPos;
                    DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                    //m_particleSystem->CreateBloodEffect(landPos, upDir, 5);
                }
            }
            ++it;
        }
    }
}

//  DrawGibs: 肉片キューブをBullet位置で描画 
void Game::DrawGibs(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
{
    if (m_gibs.empty()) return;

    // 全gibのワールド行列を一括計算、m_cubeで1体ずつ描画
    // ※ 着地済みは保存した位置を使う（物理不要）
    for (const auto& gib : m_gibs)
    {
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(gib.size, gib.size, gib.size);
        DirectX::XMMATRIX rotation;
        DirectX::XMMATRIX translation;

        if (gib.body != nullptr)
        {
            // まだ飛んでる → 物理から取得
            btTransform transform;
            gib.body->getMotionState()->getWorldTransform(transform);
            btVector3 pos = transform.getOrigin();
            btQuaternion rot = transform.getRotation();
            rotation = DirectX::XMMatrixRotationQuaternion(
                DirectX::XMVectorSet(rot.x(), rot.y(), rot.z(), rot.w()));
            translation = DirectX::XMMatrixTranslation(pos.x(), pos.y(), pos.z());
        }
        else
        {
            // 着地済み → 保存した位置を使用
            rotation = DirectX::XMMatrixRotationQuaternion(
                DirectX::XMVectorSet(gib.finalRot.x, gib.finalRot.y, gib.finalRot.z, gib.finalRot.w));
            translation = DirectX::XMMatrixTranslation(gib.finalPos.x, gib.finalPos.y, gib.finalPos.z);
        }

        DirectX::XMMATRIX world = scale * rotation * translation;

        DirectX::XMFLOAT4 color = gib.color;
        if (gib.lifetime < 1.0f)
            color.w = gib.lifetime;

        m_cube->Draw(world, view, proj, DirectX::XMVECTORF32{ color.x, color.y, color.z, color.w });
    }
}




void Game::Tick()
{
    //  === deltaTimeを計算    ===
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    float elapsedSeconds = static_cast<float>(currentTime.QuadPart - m_lastFrameTime.QuadPart)
        / static_cast<float>(m_performanceFrequency.QuadPart);

    m_lastFrameTime = currentTime;

    //  deltaTimeを更新(最大値を制限して、スパイクを防ぐ)
    m_deltaTime = min(elapsedSeconds, 0.1f);    //  最大0.1秒

    // === デバッグ: deltaTimeとFPSを表示 ===
    static float debugTimer = 0.0f;
    debugTimer += m_deltaTime;
    if (debugTimer >= 1.0f)  // 1秒ごとに表示
    {
        float currentFPS = 1.0f / m_deltaTime;
        char debugBuffer[256];
        sprintf_s(debugBuffer, "[DEBUG] deltaTime: %.4f, FPS: %.1f\n", m_deltaTime, currentFPS);
        ////OutputDebugStringA(debugBuffer);
        debugTimer = 0.0f;
    }

    // ゲームの1フレーム処理
    Update();  // ゲームロジック
    Render();  // 描画処理
}

void Game::Update()
{

    switch (m_gameState)
    {
    case GameState::TITLE:
        UpdateTitle();
        break;

    case GameState::LOADING:
        UpdateLoading();
        break;

    case GameState::PLAYING:
        UpdatePlaying();
        break;

    case GameState::GAMEOVER:
        UpdateGameOver();
        break;

    case GameState::RANKING:
        UpdateRanking();
        break;
    }

    UpdateFade();





}

void Game::Clear()
{
    // 画面を青でクリア
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Black);

    // //重要: 深度バッファ(DEPTH) と ステンシルバッファ(STENCIL) の両方をクリアする
    m_d3dContext->ClearDepthStencilView(
        m_depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // 両方のフラグを立てる
        1.0f, // Depth初期値 (1.0 = 最奥)
        0     // Stencil初期値 (0 = 履歴なし)
    );

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
        m_depthStencilView.Get());

    CD3D11_VIEWPORT viewport(0.0f, 0.0f,
        static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = (width > 1) ? width : 1;
    m_outputHeight = (height > 1) ? height : 1;
    CreateResources();
    m_uiSystem->OnScreenSizeChanged(m_outputWidth, m_outputHeight);
}


void Game::CreateDevice()
{
    UINT creationFlags = 0;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;  // デバッグ情報有効
#endif

    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,  // DirectX 11.1
        D3D_FEATURE_LEVEL_11_0,  // DirectX 11.0
        D3D_FEATURE_LEVEL_10_1,  // DirectX 10.1（フォールバック）
        D3D_FEATURE_LEVEL_10_0,  // DirectX 10.0（フォールバック）
    };

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // デフォルトアダプター
        D3D_DRIVER_TYPE_HARDWARE,   // ハードウェア（GPU）使用
        nullptr,
        creationFlags,
        featureLevels,
        4,
        D3D11_SDK_VERSION,
        device.GetAddressOf(),
        nullptr,
        context.GetAddressOf()
    );

    if (FAILED(hr))
        throw std::runtime_error("D3D11CreateDevice failed");

    device.As(&m_d3dDevice);
    context.As(&m_d3dContext);
}

void Game::CreateResources()
{
    // リソースクリア
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(1, nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(m_outputHeight);

    if (m_swapChain)
    {
        // スワップチェーンリサイズ
        HRESULT hr = m_swapChain->ResizeBuffers(2, backBufferWidth, backBufferHeight,
            DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        if (FAILED(hr)) return;
    }
    else
    {
        // スワップチェーン作成
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = backBufferWidth;
        swapChainDesc.BufferDesc.Height = backBufferHeight;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = m_window;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;

        ComPtr<IDXGIDevice> dxgiDevice;
        m_d3dDevice.As(&dxgiDevice);

        ComPtr<IDXGIAdapter> dxgiAdapter;
        dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());

        ComPtr<IDXGIFactory> dxgiFactory;
        dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));

        HRESULT hr = dxgiFactory->CreateSwapChain(m_d3dDevice.Get(),
            &swapChainDesc, m_swapChain.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("CreateSwapChain failed");

        // === ガウシアンブラー用リソース作成 ===
        CreateBlurResources();

        // [PERF] //OutputDebugStringA("[GAME] All resources created successfully\n");
    }

    // レンダーターゲット作成
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr)) throw std::runtime_error("GetBuffer failed");

    hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr,
        m_renderTargetView.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateRenderTargetView failed");

    // 深度バッファ作成
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_R24G8_TYPELESS,  // // D24_UNORM_S8_UINT → TYPELESSに変更
        backBufferWidth,
        backBufferHeight,
        1,  // 配列サイズ
        1,  // ミップレベル
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE  // // SRVフラグ追加
    );

    hr = m_d3dDevice->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        m_depthTexture.ReleaseAndGetAddressOf()  // // メンバー変数に保存
    );
    if (FAILED(hr)) throw std::runtime_error("CreateTexture2D failed");

    // 深度ステンシルビュー（DSV）の作成
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
        D3D11_DSV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_D24_UNORM_S8_UINT  // DSV用のフォーマット
    );

    hr = m_d3dDevice->CreateDepthStencilView(
        m_depthTexture.Get(),
        &depthStencilViewDesc,
        m_depthStencilView.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) throw std::runtime_error("CreateDepthStencilView failed");

    // 深度シェーダーリソースビュー（SRV）の作成
    CD3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc(
        D3D11_SRV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS  // SRV用のフォーマット
    );

    hr = m_d3dDevice->CreateShaderResourceView(
        m_depthTexture.Get(),
        &depthSRVDesc,
        m_depthSRV.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) throw std::runtime_error("CreateShaderResourceView (Depth) failed");

    // デバッグログ
    char debugDepth[256];
    sprintf_s(debugDepth, "[DEPTH] SRV created: %p\n", m_depthSRV.Get());
    // [PERF] //OutputDebugStringA(debugDepth);
}

void Game::CreateRenderResources()
{
    // DirectXTKの基本オブジェクトを作成
    m_states = std::make_unique<DirectX::CommonStates>(m_d3dDevice.Get());
    m_effect = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());

    // エフェクトの設定
    m_effect->SetVertexColorEnabled(true);  // 頂点カラーを使用

    // 入力レイアウトの作成（GPU用の設定）
    void const* shaderByteCode;
    size_t byteCodeLength;
    m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    HRESULT hr = m_d3dDevice->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,
        DirectX::VertexPositionColor::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_inputLayout.GetAddressOf()
    );

    if (FAILED(hr))
        throw std::runtime_error("CreateInputLayout failed");

    m_cube = DirectX::GeometricPrimitive::CreateCube(m_d3dContext.Get());

    // グローリーキル用の腕とナイフ（プリミティブ）
    m_gloryKillArm = DirectX::GeometricPrimitive::CreateCylinder(m_d3dContext.Get(), 0.6f, 0.05f);  // 腕（長さ0.6、太さ0.05）
    m_gloryKillKnife = DirectX::GeometricPrimitive::CreateCone(m_d3dContext.Get(), 0.2f, 0.03f);   // ナイフ（長さ0.2、太さ0.03）

    m_weaponModel = std::make_unique<Model>();
    if (!m_weaponModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/SHOTGUN.fbx"))
    {
        OutputDebugStringA("Failed to load SHOTGUN model\n");
    }
    else
    {
        // テクスチャを手動で読み込み
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shotgunTexture;
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_d3dDevice.Get(),
            L"Assets/Models/Gun/SHOTGUNTextuer/Color.png",
            nullptr,
            shotgunTexture.ReleaseAndGetAddressOf()
        );
        if (SUCCEEDED(hr))
        {
            m_weaponModel->SetTexture(shotgunTexture.Get());
            ////OutputDebugStringA("SHOTGUN texture loaded!\n");
        }
        else
        {
            ////OutputDebugStringA("Failed to load SHOTGUN texture\n");
        }
    }

    //  壁武器用モデル読み込み
    m_pistolModel = std::make_unique<Model>();
    if (!m_pistolModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/PISTOL.fbx"))
    {
        ////OutputDebugStringA("Failed to load M1911 model for wall weapon\n");
    }


    // フォント初期化
    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_d3dContext.Get());
    m_font = std::make_unique<DirectX::SpriteFont>(m_d3dDevice.Get(), L"Assets/Fonts/segoeUI.spritefont");
    m_fontLarge = std::make_unique<DirectX::SpriteFont>(m_d3dDevice.Get(), L"Assets/Fonts/segoeUI_large.spritefont");

    //  ランクテクスチャ読み込み
    {
        const wchar_t* rankFiles[7] = {
            L"Assets/Texture/Rank/rank_D.png",
            L"Assets/Texture/Rank/rank_C.png",
            L"Assets/Texture/Rank/rank_B.png",
            L"Assets/Texture/Rank/rank_A.png",
            L"Assets/Texture/Rank/rank_S.png",
            L"Assets/Texture/Rank/rank_SS.png",
            L"Assets/Texture/Rank/rank_SSS.png"
        };

        m_rankTexturesLoaded = true;

        for (int i = 0; i < 7; i++)
        {
            HRESULT hr = DirectX::CreateWICTextureFromFile(
                m_d3dDevice.Get(),
                rankFiles[i],
                nullptr,
                m_rankTextures[i].ReleaseAndGetAddressOf()
            );

            char buf[256];
            if (FAILED(hr))
            {
                sprintf_s(buf, "[RANK_TEX] FAILED index=%d\n", i);
                ////OutputDebugStringA(buf);
                m_rankTexturesLoaded = false;
            }
            else
            {
                sprintf_s(buf, "[RANK_TEX] Loaded index=%d OK\n", i);
                ////OutputDebugStringA(buf);
            }
        }
    }

    {
        const wchar_t* digitFiles[10] = {
            L"Assets/Texture/Combo/combo_0.png",
            L"Assets/Texture/Combo/combo_1.png",
            L"Assets/Texture/Combo/combo_2.png",
            L"Assets/Texture/Combo/combo_3.png",
            L"Assets/Texture/Combo/combo_4.png",
            L"Assets/Texture/Combo/combo_5.png",
            L"Assets/Texture/Combo/combo_6.png",
            L"Assets/Texture/Combo/combo_7.png",
            L"Assets/Texture/Combo/combo_8.png",
            L"Assets/Texture/Combo/combo_9.png"
        };

        m_comboTexturesLoaded = true;

        for (int i = 0; i < 10; i++)
        {
            HRESULT hr = DirectX::CreateWICTextureFromFile(
                m_d3dDevice.Get(),
                digitFiles[i],
                nullptr,
                m_comboDigitTex[i].ReleaseAndGetAddressOf()
            );
            if (FAILED(hr))
            {
                char buf[256];
                sprintf_s(buf, "[COMBO_TEX] FAILED digit=%d\n", i);
                ////OutputDebugStringA(buf);
                m_comboTexturesLoaded = false;
            }
        }

        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_d3dDevice.Get(),
            L"Assets/Texture/Combo/combo_COMBO.png",
            nullptr,
            m_comboLabelTex.ReleaseAndGetAddressOf()
        );
        if (FAILED(hr))
        {
            ////OutputDebugStringA("[COMBO_TEX] FAILED COMBO label\n");
            m_comboTexturesLoaded = false;
        }

        //if (m_comboTexturesLoaded)
            ////OutputDebugStringA("[COMBO_TEX] All 11 textures loaded OK\n");
    }

    // ==============================================
    // シールドHUDテクスチャ読み込み
    // ==============================================
    {
        m_shieldHudLoaded = true;

        struct TexEntry {
            const wchar_t* path;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* target;
            const char* name;
        };

        TexEntry hudTextures[] = {
            { L"Assets/Texture/HUD/shield_frame.png",       &m_shieldHudFrame,      "shield_frame" },
            { L"Assets/Texture/HUD/shield_fill_blue.png",   &m_shieldHudFillBlue,   "shield_fill_blue" },
            { L"Assets/Texture/HUD/shield_fill_danger.png", &m_shieldHudFillDanger,  "shield_fill_danger" },
            { L"Assets/Texture/HUD/shield_fill_guard.png",  &m_shieldHudFillGuard,   "shield_fill_guard" },
            { L"Assets/Texture/HUD/shield_glow.png",        &m_shieldHudGlow,        "shield_glow" },
            { L"Assets/Texture/HUD/shield_crack.png",       &m_shieldHudCrack,       "shield_crack" },
            { L"Assets/Texture/HUD/shield_parry_flash.png", &m_shieldHudParryFlash,  "shield_parry_flash" },
            { L"Assets/Texture/HUD/shield_icon.png",        &m_shieldHudIcon,        "shield_icon" },
            { L"Assets/Texture/HUD/hp_frame.png",           &m_hpHudFrame,           "hp_frame" },
            { L"Assets/Texture/HUD/hp_fill_green.png",      &m_hpHudFillGreen,       "hp_fill_green" },
            { L"Assets/Texture/HUD/hp_fill_critical.png",   &m_hpHudFillCritical,    "hp_fill_critical" },
        };

        for (auto& tex : hudTextures)
        {
            HRESULT hr = DirectX::CreateWICTextureFromFile(
                m_d3dDevice.Get(),
                tex.path,
                nullptr,
                tex.target->ReleaseAndGetAddressOf()
            );

            char buf[256];
            if (FAILED(hr))
            {
                sprintf_s(buf, "[HUD_TEX] FAILED: %s\n", tex.name);
                ////OutputDebugStringA(buf);
                m_shieldHudLoaded = false;
            }
            else
            {
                sprintf_s(buf, "[HUD_TEX] Loaded: %s OK\n", tex.name);
               // //OutputDebugStringA(buf);
            }
        }

        // 近接アイコン（なくてもHUDは動く）
        {
            HRESULT hr = DirectX::CreateWICTextureFromFile(
                m_d3dDevice.Get(),
                L"Assets/Texture/HUD/melee_icon.png",
                nullptr,
                m_meleeIconTexture.ReleaseAndGetAddressOf());
           // if (FAILED(hr))
                ////OutputDebugStringA("[HUD_TEX] melee_icon not found (optional)\n");
            //else
                ////OutputDebugStringA("[HUD_TEX] Loaded: melee_icon OK\n");
        }

        // === 汎用白ピクセルテクスチャ作成 ===
        {
            uint32_t whiteData = 0xFFFFFFFF;  // RGBA全部255 = 白
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = 1;
            desc.Height = 1;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = &whiteData;
            initData.SysMemPitch = 4;

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            HRESULT hr = m_d3dDevice->CreateTexture2D(&desc, &initData, &tex);
            if (SUCCEEDED(hr))
            {
                hr = m_d3dDevice->CreateShaderResourceView(tex.Get(), nullptr, &m_whitePixel);
                //if (SUCCEEDED(hr))
                    //////OutputDebugStringA("[HUD_TEX] White pixel created OK\n");
            }
        }

        //if (m_shieldHudLoaded)
            ////OutputDebugStringA("[HUD_TEX] All HUD textures loaded OK!\n");
    }

  /*  m_testModel = std::make_unique<Model>();*/

    ////  プレイヤーモデルファイルを読み込む
    //if (!m_testModel->LoadFromFile(m_d3dDevice.Get(), "Assets/X_Bot.fbx"))
    //{
    //    //OutputDebugStringA("Game::CreateRenderResources - Failed to load model!\n");
    //}
    //else
    //{
    //    //OutputDebugStringA("Game::CreateRenderResources - Model loaded successfully!\n");
    //}

    //  === 敵NORMALモデル   ===
    m_enemyModel = std::make_unique<Model>();
    if (!m_enemyModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Normal/Normal.fbx"))
    {
        ////OutputDebugStringA("Failed to load enemy model!\n");
        //throw std::runtime_error("Failed to load enemy model");
    }
    else
    {
        ////OutputDebugStringA("Enemy model loaded successfully!\n");
    }

    //  === 敵RUNNERモデル  ===
    m_runnerModel = std::make_unique<Model>();
    if (!m_runnerModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Runner/Runner.fbx"))
    {
        ////OutputDebugStringA("Failed to load Runner model!\n");
    }
    else
    {
        ////OutputDebugStringA("Runner model loaded successfully!\n");
    }

    //  === 敵TANK   ===
    m_tankModel = std::make_unique<Model>();
    if (!m_tankModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Tank/Tank.fbx"))
    {
        ////OutputDebugStringA("Failed to load Tank model");
    }
    else
    {
        ////OutputDebugStringA("Tank model loaded successfully!\n");
    }

    //  アームモデル
    m_gloryKillArmModel = std::make_unique<Model>();
    if (!m_gloryKillArmModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/GloryKill/fps_armsKnife.fbx"))
    {
        ////OutputDebugStringA("Failed to load FPS arms model!\n");
    }
    else
    {
        ////OutputDebugStringA("FPS arms model loaded!\n");

        m_gloryKillArmModel->PrintBoneNames();

        

        // テクスチャを読み込んで適用
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_d3dDevice.Get(),
            L"Assets/Models/GloryKill/Textures/arm1Color.png",
            nullptr,
            m_armDiffuseTexture.ReleaseAndGetAddressOf()
        );
        if (SUCCEEDED(hr))
        {
            m_gloryKillArmModel->SetTexture(m_armDiffuseTexture.Get());
            ////OutputDebugStringA("Arm texture loaded!\n");
        }
        else
        {
            ////OutputDebugStringA("Failed to load arm texture!\n");
        }
    }


    // スコアポップアップ用テクスチャ
    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/score_glow.png", nullptr, m_scoreGlow.ReleaseAndGetAddressOf());
    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/score_glow_red.png", nullptr, m_scoreGlowRed.ReleaseAndGetAddressOf());
    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/score_glow_blue.png", nullptr, m_scoreGlowBlue.ReleaseAndGetAddressOf());
    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/score_burst.png", nullptr, m_scoreBurst.ReleaseAndGetAddressOf());
    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/score_skull.png", nullptr, m_scoreSkull.ReleaseAndGetAddressOf());
    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/score_headshot.png", nullptr, m_scoreHeadshot.ReleaseAndGetAddressOf());


    m_gloryKillKnifeModel = std::make_unique<Model>();
    if (!m_gloryKillKnifeModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/GloryKill/Knife.fbx"))
    {
        ////OutputDebugStringA("Failed to load knife model!\n");
    }
    

    //  ====================================
    //  === NORMALモデルのアニメーション   ===
    //  ====================================
    {
        ////OutputDebugStringA("=== Loading Y_Bot animations  ===\n");

        //  --- 歩行アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Walk.fbx", "Walk"))
        {
            ////OutputDebugStringA("Failed to load Walk animation\n");
        }

        //  --- 待機アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Idle.fbx", "Idle"))
        {
            ////OutputDebugStringA("Failed to load Idle animation\n");
        }

        //  --- 走りアニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Run.fbx", "Run"))
        {
            ////OutputDebugStringA("Failed to load Run animation\n");
        }

        //  --- 攻撃アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Attack.fbx", "Attack"))
        {
            ////OutputDebugStringA("Failed to load Attack animation");
        }

        //  --- 死亡アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Death.fbx", "Death"))
        {
            ////OutputDebugStringA("Failed to load Death Animation");
        }
        //  スタンアニメーション
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Stun.fbx", "Stun"))
        {

        }
    }

    //  ===================================
    //  === RUNNERモデルアニメーション    ===
    //  ===================================
    {
        //  === RUNNERアニメーション読み込み   ===
        ////OutputDebugStringA("=== Loading Runner animations ===\n");

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Walk.fbx", "Walk"))
        {
            //OutputDebugStringA("Failed to load Runner Walk animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Run.fbx", "Run"))
        {
            //OutputDebugStringA("Failed to load Runner Run animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Attack.fbx", "Attack"))
        {
            //OutputDebugStringA("Failed to load Runner Attack animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Idle.fbx", "Idle"))
        {
            //OutputDebugStringA("Failed to load Runner Idle animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Death.fbx", "Death"))
        {
            //OutputDebugStringA("Failed to load Runner Death animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Stun.fbx", "Stun"))
        {
        }
    }

    //  =========================
    //  === TANKアニメーション ===
    //  =========================
    {
        //  === Tank アニメーション読み込み ===
        //OutputDebugStringA("=== Loading Tank animations ===\n");

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Walk.fbx", "Walk"))
        {
            //OutputDebugStringA("Failed to load Tank Walk animation\n");
        }

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Attack.fbx", "Attack"))
        {
            //OutputDebugStringA("Failed to load Tank Attack animation\n");
        }

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Idle.fbx", "Idle"))
        {
            //OutputDebugStringA("Failed to load Tank Idle animation\n");
        }

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Death.fbx", "Death"))
        {
            //OutputDebugStringA("Failed to load Tank Death animation\n");
        }
        if (m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Stun.fbx", "Stun"))
        {

        }


    // ====================================================================
    // MIDBOSS モデル読み込み
    // ====================================================================
        m_midBossModel = std::make_unique<Model>();
        if (!m_midBossModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/MidBoss/midboss.fbx"))
        {
            OutputDebugStringA("[MODEL] MIDBOSS model load FAILED\n");
        }
        else
        {
            //OutputDebugStringA("[MODEL] MIDBOSS model loaded OK\n");

            if (!m_midBossModel->LoadAnimation("Assets/Models/MidBoss/midboss_walking.fbx", "Walk"))
                OutputDebugStringA("[ANIM] MIDBOSS Walk FAILED\n");

            if (!m_midBossModel->LoadAnimation("Assets/Models/MidBoss/midboss_attack.fbx", "Attack"))
                OutputDebugStringA("[ANIM] MIDBOSS Attack FAILED\n");

            if (!m_midBossModel->LoadAnimation("Assets/Models/MidBoss/midboss_Idle.fbx", "Idle"))
                OutputDebugStringA("[ANIM] MIDBOSS Idle FAILED\n");

            if (!m_midBossModel->LoadAnimation("Assets/Models/MidBoss/midboss_death.fbx", "Death"))
                OutputDebugStringA("[ANIM] MIDBOSS Death FAILED\n");
            if (!m_midBossModel->LoadAnimation("Assets/Models/MidBoss/midBoss_stun.fbx", "Stun")) {}
        }
    
    }

    // === BOSSモデル読み込み ===
    m_bossModel = std::make_unique<Model>();
    if (!m_bossModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Boss/boss.fbx")) {
        //OutputDebugStringA("[MODEL] BOSS model load FAILED\n");
    }
    else {
        //OutputDebugStringA("[MODEL] BOSS model loaded OK\n");
        m_bossModel->LoadAnimation("Assets/Models/Boss/boss_walk.fbx", "Walk");
        m_bossModel->LoadAnimation("Assets/Models/Boss/boss_attack_jump.fbx", "AttackJump");
        m_bossModel->LoadAnimation("Assets/Models/Boss/boss_attack_roundingup.fbx", "AttackSlash");
        m_bossModel->LoadAnimation("Assets/Models/Boss/boss_Idle.fbx", "Idle");
        m_bossModel->LoadAnimation("Assets/Models/Boss/boss_death.fbx", "Death");
        m_bossModel->LoadAnimation("Assets/Models/Boss/boss_stun.fbx", "Stun");
    }

    // === 盾モデル読み込み ===
    m_shieldModel = std::make_unique<Model>();
    if (!m_shieldModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Shield/02_Shield.fbx"))
    {
        //OutputDebugStringA("[SHIELD] Failed to load shield model!\n");
    }
    else
    {
        //OutputDebugStringA("[SHIELD] Shield model loaded successfully!\n");

        // テクスチャ読み込み（BaseColor）
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shieldTex;
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_d3dDevice.Get(),
            L"Assets/Models/Shield/Textury/02_Shield_BaseColor.png",
            nullptr,
            shieldTex.ReleaseAndGetAddressOf()
        );
        if (SUCCEEDED(hr))
        {
            m_shieldModel->SetTexture(shieldTex.Get());
            //OutputDebugStringA("[SHIELD] Texture loaded!\n");
        }
    }

    {
        // 充血ビネットテクスチャ読み込み
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_d3dDevice.Get(),                        // デバイス
            L"Assets/Texture/low_health_vignette.png", // ファイルパス
            nullptr,                                   // テクスチャリソース（不要）
            m_lowHealthVignetteSRV.GetAddressOf()      // SRV（シェーダーに渡す用）
        );
        if (SUCCEEDED(hr))
            OutputDebugStringA("[Game] Low health vignette texture loaded!\n");
        else
            OutputDebugStringA("[Game] WARNING: low_health_vignette.png not found!\n");
    }

    // ダッシュ用スピードラインテクスチャ
    {
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_d3dDevice.Get(),
            L"Assets/Texture/dash_speedline.png",
            nullptr,
            m_dashSpeedlineSRV.GetAddressOf());
        if (SUCCEEDED(hr))
            OutputDebugStringA("[Game] Dash speedline texture loaded!\n");
    }

    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/wave_banner_normal.png", nullptr, m_waveBannerNormalSRV.ReleaseAndGetAddressOf());
    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/wave_banner_boss.png", nullptr, m_waveBannerBossSRV.ReleaseAndGetAddressOf());

    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/score_blood_backdrop.png", nullptr, m_scoreBackdropSRV.ReleaseAndGetAddressOf());

    DirectX::CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Texture/HUD/claw_damage.png", nullptr, m_clawDamageSRV.ReleaseAndGetAddressOf());
    //  === MapSystem   初期化 ===
    m_mapSystem = std::make_unique<MapSystem>();
    if (!m_mapSystem->Initialize(m_d3dContext.Get(), m_d3dDevice.Get()))
    {
        //OutputDebugStringA("Game::CreateRenderResources - Failed to initialize MapSystem\n");
    }
    else
    {
        m_mapSystem->CreateDefaultMap();

        //  FBXマップ読み込み
        if (m_mapSystem->LoadMapFBX(
            "Assets/Models/Map/Market AL_DANUBE.fbx",
            L"Assets/Models/Map/textures",
            1.0f))
        {
            OutputDebugStringA("FBX Map loaded!\n");

            // === FBXメッシュコライダー作成 ===
            const auto& tris = m_mapSystem->GetCollisionTriangles();
            if (!tris.empty())
            {
                //  btTriangleMesh に三角形を登録
                m_mapTriMesh = new btTriangleMesh();
                for (const auto& tri : tris)
                {
                    btVector3 v0(tri.v0.x, tri.v0.y, tri.v0.z);
                    btVector3 v1(tri.v1.x, tri.v1.y, tri.v1.z);
                    btVector3 v2(tri.v2.x, tri.v2.y, tri.v2.z);
                    m_mapTriMesh->addTriangle(v0, v1, v2);
                }

                //  BVHメッシュ形状を作成（静的オブジェクト専用）
                m_mapMeshShape = new btBvhTriangleMeshShape(m_mapTriMesh, true);

                //  静的剛体として登録（mass=0 = 動かない）
                btTransform meshTransform;
                meshTransform.setIdentity();

                btDefaultMotionState* ms = new btDefaultMotionState(meshTransform);
                btRigidBody::btRigidBodyConstructionInfo info(
                    0.0f, ms, m_mapMeshShape);
                m_mapMeshBody = new btRigidBody(info);

                // マップの剛体にはuserPointerをnullにしておく（敵と区別）
                m_mapMeshBody->setUserPointer(nullptr);

                m_dynamicsWorld->addRigidBody(m_mapMeshBody);

                char buf[256];
                sprintf_s(buf, "[PHYSICS] Mesh collider created: %zu triangles\n",
                    tris.size());
                OutputDebugStringA(buf);
            }

            // === ナビゲーショングリッド構築 ===
            BuildNavGrid();

            TestMeshSlice();

            //  既存プリミティブも描画する（床・壁を残す）
            m_mapSystem->SetDrawPrimitives(false);
            m_mapSystem->SetMapTransform(
                XMFLOAT3(0.0f, -0.1f, 0.0f),
                0.0f,
                1.0f
            );
        }
        
        
        //OutputDebugStringA("Game::CreateRenderResources - MapSystem initialized successfully\n");
    }

    m_furRenderer = std::make_unique<FurRenderer>();
    m_furReady = m_furRenderer->Initialize(m_d3dDevice.Get());
    if (!m_furReady)
    {
        //OutputDebugStringA("[FUR] FurRenderer init FAILED\n");
    }

    //  === WeaponSystem 初期化    2025/11/14  ===
    m_weaponSpawnSystem = std::make_unique<WeaponSpawnSystem>();
    m_weaponSpawnSystem->CreateDefaultSpawns();

    //  --- テクスチャ読み込み   2025/11/19  ---
    if (!m_weaponSpawnSystem->InitializeTextures(m_d3dDevice.Get(), m_d3dContext.Get()))
    {
        //OutputDebugStringA("Game::CreateRenderResources - Failed to initialize weapon textures\n");
    }

    m_nearbyWeaponSpawn = nullptr;

    //OutputDebugStringA("Game::CreateRenderResources - WeaponSpawnSystem initialized\n");
    
    //  === Shadow を初期化  ===
    m_shadow = std::make_unique<Shadow>();
    if (!m_shadow->Initialize(m_d3dDevice.Get()))
    {
        //OutputDebugStringA("Game::CreateRenderResources - Failed to initialize Shadow System!\n");
        // エラー処理
    }
    else
    {
        // 影の色を黒の半透明に設定
        m_shadow->SetShadowColor(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f));
    }

    //  === StunRingの初期化    ===
    m_stunRing = std::make_unique<StunRing>();
    if (!m_stunRing->Initialize(m_d3dDevice.Get()))
    {
        OutputDebugStringA("[StunRing] Failed to initialize!\n");
    }

    //  === TargetMarkerの初期化    ===
    m_targetMarker = std::make_unique<TargetMarker>();
    if (!m_targetMarker->Initialize(m_d3dDevice.Get()))
    {
        OutputDebugStringA("[TargetMarker] Failed to initialze");
    }

    //  === Effekseerの初期化   ===
    
    // 描画管理クラス(レンダラー)の作成
    m_effekseerRenderer = EffekseerRendererDX11::Renderer::Create(
        m_d3dDevice.Get(),
        m_d3dContext.Get(),
        8000
    );

    m_enemyModel->LoadCustomShaders(m_d3dDevice.Get());
    m_runnerModel->LoadCustomShaders(m_d3dDevice.Get());
    m_tankModel->LoadCustomShaders(m_d3dDevice.Get());
    m_midBossModel->LoadCustomShaders(m_d3dDevice.Get());
    m_bossModel->LoadCustomShaders(m_d3dDevice.Get());

    // インスタンシング用バッファ作成
    m_enemyModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_runnerModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_tankModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_midBossModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_bossModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);

    // エッフェクト管理クラス
    m_effekseerManager = Effekseer::Manager::Create(8000);
    //  DirectXは左手座標なのでEffekseerに教える
    m_effekseerManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

    // 描画設定の紐づけ
    m_effekseerManager->SetSpriteRenderer(m_effekseerRenderer->CreateSpriteRenderer());
    m_effekseerManager->SetRibbonRenderer(m_effekseerRenderer->CreateRibbonRenderer());
    m_effekseerManager->SetRingRenderer(m_effekseerRenderer->CreateRingRenderer());
    m_effekseerManager->SetTrackRenderer(m_effekseerRenderer->CreateTrackRenderer());
    m_effekseerManager->SetModelRenderer(m_effekseerRenderer->CreateModelRenderer());
    m_effekseerManager->SetTextureLoader(m_effekseerRenderer->CreateTextureLoader());
    m_effekseerManager->SetModelLoader(m_effekseerRenderer->CreateModelLoader());
    m_effekseerManager->SetMaterialLoader(m_effekseerRenderer->CreateMaterialLoader());

    // 盾トレイルエフェクトの読み込み
    m_effectShieldTrail = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/ShieldTrail.efkefc");

    if (m_effectShieldTrail == nullptr)
    {
        //OutputDebugStringA("[EFFEKSEER] Failed to load ShieldTrail effect!\n");
    }
    else
    {
        //OutputDebugStringA("[EFFEKSEER] ShieldTrail loaded successfully!\n");
    }


    // === ボスエフェクト読み込み ===
    m_effectSlashRed = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/SlashRed.efkefc");
    // === ボスエフェクト読み込み ===
    m_effectSlashRed = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/SlashRed.efkefc");

    m_effectSlashGreen = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/SlashGreen.efkefc");

    m_effectGroundSlam = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/GroundSlam.efkefc");

    m_effectBeamRed = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/BeamRed.efkefc");

    m_effectBeamGreen = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/BeamGreen.efkefc");

    // 銃発射エフェクト
    m_effectGunFire = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/GunFire.efkefc");

    // 弾丸ヒットエフェクト
    m_effectAttackImpact = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/AttackImpact.efkefc");

    // ボススポーンエフェクト
    m_effectBossSpawn = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/BossSpawn.efkefc");

    // 雑魚スポーンエフェクト
    m_effectEnemySpawn = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/SpawnEnemy.efkefc");

    // パリィエフェクト
    m_effectParry = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/Parry.efkefc");


    InitSliceRendering();

    //  === Imgui   ===
    InitImGui();

    
}

void Game::InitSliceRendering()
{
    // テクスチャ有り Effect
    m_sliceEffectTex = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_sliceEffectTex->SetLightingEnabled(true);
    m_sliceEffectTex->SetLightEnabled(0, true);
    m_sliceEffectTex->SetLightDirection(0, DirectX::XMVectorSet(0, -1, 0.3f, 0));
    m_sliceEffectTex->SetLightDiffuseColor(0, DirectX::XMVectorSet(1, 0.9f, 0.8f, 1));
    m_sliceEffectTex->SetAmbientLightColor(DirectX::XMVectorSet(0.4f, 0.4f, 0.4f, 1));
    m_sliceEffectTex->SetTextureEnabled(true);

    // テクスチャ無し Effect
    m_sliceEffectNoTex = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_sliceEffectNoTex->SetLightingEnabled(true);
    m_sliceEffectNoTex->SetLightEnabled(0, true);
    m_sliceEffectNoTex->SetLightDirection(0, DirectX::XMVectorSet(0, -1, 0.3f, 0));
    m_sliceEffectNoTex->SetLightDiffuseColor(0, DirectX::XMVectorSet(1, 0.9f, 0.8f, 1));
    m_sliceEffectNoTex->SetAmbientLightColor(DirectX::XMVectorSet(0.4f, 0.4f, 0.4f, 1));
    m_sliceEffectNoTex->SetTextureEnabled(false);

    // InputLayout（テクスチャ有り）
    void const* shaderByteCode;
    size_t byteCodeLength;
    m_sliceEffectTex->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    m_d3dDevice->CreateInputLayout(
        DirectX::VertexPositionNormalTexture::InputElements,
        DirectX::VertexPositionNormalTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_sliceInputLayoutTex.ReleaseAndGetAddressOf());

    // InputLayout（テクスチャ無し）
    m_sliceEffectNoTex->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    m_d3dDevice->CreateInputLayout(
        DirectX::VertexPositionNormalTexture::InputElements,
        DirectX::VertexPositionNormalTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_sliceInputLayoutNoTex.ReleaseAndGetAddressOf());

    // 両面描画ラスタライザ
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    m_d3dDevice->CreateRasterizerState(&rsDesc, m_sliceNoCullRS.ReleaseAndGetAddressOf());
}

void Game::DrawDebugUI()
{
    // ImGui新フレーム開始
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // === デバッグウィンドウ ===
    if (m_showDebugWindow)
    {
        
        ImGui::Begin("Gothic Swarm Debug", &m_showDebugWindow, ImGuiWindowFlags_AlwaysAutoResize);

        // FPS表示
        ImGui::Text("FPS: %.1f", m_currentFPS);
        ImGui::Separator();

        // Wave情報
        ImGui::Text("Wave: %d", m_waveManager->GetCurrentWave());

        // === タイプ別カウント表示 ===
        int totalEnemies = 0;
        int normalCount = 0;
        int runnerCount = 0;
        int tankCount = 0;



        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if (!enemy.isAlive && !enemy.isDying)
                continue;

            totalEnemies++;

            switch (enemy.type)
            {
            case EnemyType::NORMAL:
                normalCount++;
                break;
            case EnemyType::RUNNER:
                runnerCount++;
                break;
            case EnemyType::TANK:
                tankCount++;
                break;
            }
        }

        ImGui::Checkbox("Draw NavGrid", &m_debugDrawNavGrid);

        // === ライト調整 ===
        if (m_mapSystem && ImGui::CollapsingHeader("Map Lighting"))
        {
            ImGui::Text("--- Main Light ---");
            ImGui::SliderFloat3("Direction 0", &m_mapSystem->m_lightDir0.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color 0", &m_mapSystem->m_lightColor0.x);

            ImGui::Text("--- Fill Light ---");
            ImGui::SliderFloat3("Direction 1", &m_mapSystem->m_lightDir1.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color 1", &m_mapSystem->m_lightColor1.x);

            ImGui::Text("--- Ambient ---");
            ImGui::ColorEdit3("Ambient", &m_mapSystem->m_ambientColor.x);

            // プリセット
            if (ImGui::Button("Preset: Gothic Dark"))
            {
                m_mapSystem->m_lightDir0 = { 0.0f, -1.0f, 0.3f };
                m_mapSystem->m_lightColor0 = { 0.6f, 0.45f, 0.3f, 1.0f };
                m_mapSystem->m_lightDir1 = { -0.5f, -0.3f, -0.7f };
                m_mapSystem->m_lightColor1 = { 0.15f, 0.15f, 0.25f, 1.0f };
                m_mapSystem->m_ambientColor = { 0.08f, 0.06f, 0.05f, 1.0f };
            }
            ImGui::SameLine();
            if (ImGui::Button("Preset: Bright"))
            {
                m_mapSystem->m_lightDir0 = { 0.3f, -0.8f, 0.5f };
                m_mapSystem->m_lightColor0 = { 1.0f, 0.95f, 0.9f, 1.0f };
                m_mapSystem->m_lightDir1 = { -0.5f, -0.3f, -0.7f };
                m_mapSystem->m_lightColor1 = { 0.4f, 0.4f, 0.5f, 1.0f };
                m_mapSystem->m_ambientColor = { 0.25f, 0.25f, 0.3f, 1.0f };
            }
            ImGui::SameLine();
            if (ImGui::Button("Preset: Blood Moon"))
            {
                m_mapSystem->m_lightDir0 = { 0.2f, -0.6f, 0.4f };
                m_mapSystem->m_lightColor0 = { 0.8f, 0.2f, 0.1f, 1.0f };
                m_mapSystem->m_lightDir1 = { -0.5f, -0.3f, -0.7f };
                m_mapSystem->m_lightColor1 = { 0.1f, 0.05f, 0.15f, 1.0f };
                m_mapSystem->m_ambientColor = { 0.1f, 0.02f, 0.02f, 1.0f };
            }
        }



        ImGui::Text("Total Enemies: %d", totalEnemies);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  Normal: %d", normalCount);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  Runner: %d", runnerCount);
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "  Tank: %d", tankCount);
        ImGui::Separator();

        // プレイヤー情報
        ImGui::Text("Health: %d", m_player->GetHealth());
        ImGui::Text("Points: %d", m_player->GetPoints());

        // === プレイヤー位置情報 ===
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();
        ImGui::Text("Player Pos: (%.2f, %.2f, %.2f)", playerPos.x, playerPos.y, playerPos.z);
        ImGui::Text("Ray Start Y: %.2f", playerPos.y + 0.5f);
        ImGui::Text("Camera Rot: (%.2f, %.2f)",
            DirectX::XMConvertToDegrees(playerRot.x),
            DirectX::XMConvertToDegrees(playerRot.y));
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Glory Kill Model"))
        {
            ImGui::Checkbox("Always Show Arm (Debug)", &m_debugShowGloryKillArm);
            ImGui::Separator();

            ImGui::Text("=== ARM ===");
            ImGui::SliderFloat("Arm Scale", &m_gloryKillArmScale, 0.001f, 0.1f);
            ImGui::SliderFloat("Arm Forward", &m_gloryKillArmOffset.x, 0.0f, 2.0f);
            ImGui::SliderFloat("Arm Up/Down", &m_gloryKillArmOffset.y, -1.0f, 1.0f);
            ImGui::SliderFloat("Arm Left/Right", &m_gloryKillArmOffset.z, -1.0f, 1.0f);

            ImGui::Text("--- Base Rotation (Fix Model) ---");
            ImGui::SliderFloat("Arm BaseRotX", &m_gloryKillArmBaseRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm BaseRotY", &m_gloryKillArmBaseRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm BaseRotZ", &m_gloryKillArmBaseRot.z, -3.14f, 3.14f);

            ImGui::Text("--- Anim Rotation (For Motion) ---");
            ImGui::SliderFloat("Arm AnimRotX", &m_gloryKillArmAnimRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm AnimRotY", &m_gloryKillArmAnimRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm AnimRotZ", &m_gloryKillArmAnimRot.z, -3.14f, 3.14f);

            ImGui::Separator();
            ImGui::Text("=== KNIFE ===");
            ImGui::SliderFloat("Knife Scale", &m_gloryKillKnifeScale, 0.001f, 0.1f);
            ImGui::SliderFloat("Knife Forward", &m_gloryKillKnifeOffset.x, -0.5f, 0.5f);
            ImGui::SliderFloat("Knife Up/Down", &m_gloryKillKnifeOffset.y, -0.5f, 0.5f);
            ImGui::SliderFloat("Knife Left/Right", &m_gloryKillKnifeOffset.z, -0.5f, 0.5f);

            ImGui::Text("--- Base Rotation ---");
            ImGui::SliderFloat("Knife BaseRotX", &m_gloryKillKnifeBaseRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife BaseRotY", &m_gloryKillKnifeBaseRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife BaseRotZ", &m_gloryKillKnifeBaseRot.z, -3.14f, 3.14f);

            ImGui::Text("--- Anim Rotation ---");
            ImGui::SliderFloat("Knife AnimRotX", &m_gloryKillKnifeAnimRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife AnimRotY", &m_gloryKillKnifeAnimRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife AnimRotZ", &m_gloryKillKnifeAnimRot.z, -3.14f, 3.14f);
        }

        // 表示トグル
        ImGui::Text("Visual Debug:");
        ImGui::Checkbox("Show Body Hitboxes", &m_showHitboxes);
        ImGui::Checkbox("Show Head Hitboxes", &m_showHeadHitboxes);
        ImGui::Checkbox("Show Physics Capsules", &m_showPhysicsHitboxes);
        ImGui::Checkbox("Show Bullet Trajectory", &m_showBulletTrajectory);
        ImGui::SliderFloat("Ray Start Y", &m_rayStartY, 0.5f, 3.0f, "%.2f");

        if (ImGui::TreeNode("Weapon Transform"))
        {
            ImGui::SliderFloat("Scale", &m_weaponScale, 0.001f, 0.5f, "%.4f");
            ImGui::SliderFloat("Rot X", &m_weaponRotX, -3.14f, 3.14f, "%.2f");
            ImGui::SliderFloat("Rot Y", &m_weaponRotY, -3.14f, 3.14f, "%.2f");
            ImGui::SliderFloat("Rot Z", &m_weaponRotZ, -3.14f, 3.14f, "%.2f");
            ImGui::Separator();
            ImGui::SliderFloat("Right", &m_weaponOffsetRight, -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Up", &m_weaponOffsetUp, -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Forward", &m_weaponOffsetForward, 0.0f, 2.0f, "%.2f");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Weapon Bob"))
        {
            ImGui::SliderFloat("Speed", &m_weaponBobSpeed, 5.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Strength", &m_weaponBobStrength, 0.005f, 0.06f, "%.3f");
            ImGui::Text("Bob: %.4f  Impact: %.4f", m_weaponBobAmount, m_weaponLandingImpact);
            ImGui::TreePop();
        }

        // 凡例を追加
        if (m_showHitboxes || m_showHeadHitboxes || m_showPhysicsHitboxes)
        {
            ImGui::Separator();
            ImGui::Text("Color Legend:");
            if (m_showHitboxes)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  Red = Normal Body");
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  Yellow = Runner Body");
                ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "  Blue = Tank Body");
            }
            if (m_showPhysicsHitboxes)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "  Green = Physics Capsule");
            }
            if (m_showHeadHitboxes)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "  Magenta = Head");
            }
        }
        ImGui::Separator();

        // ===================================================
        // === NEW: 当たり判定調整タブ ===
        // ===================================================
        if (ImGui::CollapsingHeader("Hitbox Tuning", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // === デバッグ値を使うかトグル ===
            ImGui::Checkbox("Use Debug Hitboxes (Real-time)", &m_useDebugHitboxes);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                m_useDebugHitboxes ? "Using DEBUG values" : "Using Entities.h values");
            ImGui::Separator();

            // === NORMAL 調整 ===
            if (ImGui::TreeNode("NORMAL Hitbox"))
            {
                ImGui::Text("Body:");
                ImGui::SliderFloat("Body Width##Normal", &m_normalConfigDebug.bodyWidth, 0.1f, 2.0f);
                ImGui::SliderFloat("Body Height##Normal", &m_normalConfigDebug.bodyHeight, 0.5f, 3.0f);

                ImGui::Text("Head:");
                ImGui::SliderFloat("Head Height##Normal", &m_normalConfigDebug.headHeight, 0.5f, 3.0f);
                ImGui::SliderFloat("Head Radius##Normal", &m_normalConfigDebug.headRadius, 0.1f, 1.0f);

                // 現在の値を表示
                ImGui::Text("Current Values:");
                ImGui::Text("  Body: %.2f x %.2f", m_normalConfigDebug.bodyWidth, m_normalConfigDebug.bodyHeight);
                ImGui::Text("  Head: Y=%.2f, R=%.2f", m_normalConfigDebug.headHeight, m_normalConfigDebug.headRadius);

                // Entities.hにコピーするコード生成
                if (ImGui::Button("Copy to Clipboard##Normal"))
                {
                    char buffer[512];
                    sprintf_s(buffer,
                        "case EnemyType::NORMAL:\n"
                        "    config.bodyWidth = %.2ff;\n"
                        "    config.bodyHeight = %.2ff;\n"
                        "    config.headHeight = %.2ff;\n"
                        "    config.headRadius = %.2ff;\n"
                        "    break;",
                        m_normalConfigDebug.bodyWidth,
                        m_normalConfigDebug.bodyHeight,
                        m_normalConfigDebug.headHeight,
                        m_normalConfigDebug.headRadius
                    );
                    ImGui::SetClipboardText(buffer);
                    //OutputDebugStringA("NORMAL config copied to clipboard!\n");
                }

                ImGui::TreePop();
            }

            // === RUNNER 調整（同様に変更）===
            if (ImGui::TreeNode("RUNNER Hitbox"))
            {
                ImGui::Text("Body:");
                ImGui::SliderFloat("Body Width##Runner", &m_runnerConfigDebug.bodyWidth, 0.1f, 2.0f);
                ImGui::SliderFloat("Body Height##Runner", &m_runnerConfigDebug.bodyHeight, 0.5f, 3.0f);

                ImGui::Text("Head:");
                ImGui::SliderFloat("Head Height##Runner", &m_runnerConfigDebug.headHeight, 0.5f, 3.0f);
                ImGui::SliderFloat("Head Radius##Runner", &m_runnerConfigDebug.headRadius, 0.1f, 1.0f);

                ImGui::Text("Current Values:");
                ImGui::Text("  Body: %.2f x %.2f", m_runnerConfigDebug.bodyWidth, m_runnerConfigDebug.bodyHeight);
                ImGui::Text("  Head: Y=%.2f, R=%.2f", m_runnerConfigDebug.headHeight, m_runnerConfigDebug.headRadius);

                if (ImGui::Button("Copy to Clipboard##Runner"))
                {
                    char buffer[512];
                    sprintf_s(buffer,
                        "case EnemyType::RUNNER:\n"
                        "    config.bodyWidth = %.2ff;\n"
                        "    config.bodyHeight = %.2ff;\n"
                        "    config.headHeight = %.2ff;\n"
                        "    config.headRadius = %.2ff;\n"
                        "    break;",
                        m_runnerConfigDebug.bodyWidth,
                        m_runnerConfigDebug.bodyHeight,
                        m_runnerConfigDebug.headHeight,
                        m_runnerConfigDebug.headRadius
                    );
                    ImGui::SetClipboardText(buffer);
                    //OutputDebugStringA("RUNNER config copied to clipboard!\n");
                }



                ImGui::TreePop();
            }

            // === TANK 調整（同様に変更）===
            if (ImGui::TreeNode("TANK Hitbox"))
            {
                ImGui::Text("Body:");
                ImGui::SliderFloat("Body Width##Tank", &m_tankConfigDebug.bodyWidth, 0.1f, 2.0f);
                ImGui::SliderFloat("Body Height##Tank", &m_tankConfigDebug.bodyHeight, 0.5f, 3.0f);

                ImGui::Text("Head:");
                ImGui::SliderFloat("Head Height##Tank", &m_tankConfigDebug.headHeight, 0.5f, 3.0f);
                ImGui::SliderFloat("Head Radius##Tank", &m_tankConfigDebug.headRadius, 0.1f, 1.0f);

                ImGui::Text("Current Values:");
                ImGui::Text("  Body: %.2f x %.2f", m_tankConfigDebug.bodyWidth, m_tankConfigDebug.bodyHeight);
                ImGui::Text("  Head: Y=%.2f, R=%.2f", m_tankConfigDebug.headHeight, m_tankConfigDebug.headRadius);

                if (ImGui::Button("Copy to Clipboard##Tank"))
                {
                    char buffer[512];
                    sprintf_s(buffer,
                        "case EnemyType::TANK:\n"
                        "    config.bodyWidth = %.2ff;\n"
                        "    config.bodyHeight = %.2ff;\n"
                        "    config.headHeight = %.2ff;\n"
                        "    config.headRadius = %.2ff;\n"
                        "    break;",
                        m_tankConfigDebug.bodyWidth,
                        m_tankConfigDebug.bodyHeight,
                        m_tankConfigDebug.headHeight,
                        m_tankConfigDebug.headRadius
                    );
                    ImGui::SetClipboardText(buffer);
                    //OutputDebugStringA("TANK config copied to clipboard!\n");
                }

                ImGui::TreePop();
            }

            // === MIDBOSS 調整 ===
            if (ImGui::TreeNode("MIDBOSS Hitbox"))
            {
                ImGui::Text("Body:");
                ImGui::SliderFloat("Body Width##MidBoss", &m_midbossConfigDebug.bodyWidth, 0.1f, 3.0f);
                ImGui::SliderFloat("Body Height##MidBoss", &m_midbossConfigDebug.bodyHeight, 0.5f, 5.0f);
                ImGui::Text("Head:");
                ImGui::SliderFloat("Head Height##MidBoss", &m_midbossConfigDebug.headHeight, 0.5f, 5.0f);
                ImGui::SliderFloat("Head Radius##MidBoss", &m_midbossConfigDebug.headRadius, 0.1f, 2.0f);
                ImGui::Text("  Body: %.2f x %.2f", m_midbossConfigDebug.bodyWidth, m_midbossConfigDebug.bodyHeight);
                ImGui::Text("  Head: Y=%.2f, R=%.2f", m_midbossConfigDebug.headHeight, m_midbossConfigDebug.headRadius);
                if (ImGui::Button("Copy to Clipboard##MidBoss"))
                {
                    char buffer[512];
                    sprintf_s(buffer,
                        "case EnemyType::MIDBOSS:\n"
                        "    config.bodyWidth = %.2ff;\n"
                        "    config.bodyHeight = %.2ff;\n"
                        "    config.headHeight = %.2ff;\n"
                        "    config.headRadius = %.2ff;\n"
                        "    break;",
                        m_midbossConfigDebug.bodyWidth, m_midbossConfigDebug.bodyHeight,
                        m_midbossConfigDebug.headHeight, m_midbossConfigDebug.headRadius);
                    ImGui::SetClipboardText(buffer);
                }
                ImGui::TreePop();
            }

            // === BOSS 調整 ===
            if (ImGui::TreeNode("BOSS Hitbox"))
            {
                ImGui::Text("Body:");
                ImGui::SliderFloat("Body Width##Boss", &m_bossConfigDebug.bodyWidth, 0.1f, 4.0f);
                ImGui::SliderFloat("Body Height##Boss", &m_bossConfigDebug.bodyHeight, 0.5f, 8.0f);
                ImGui::Text("Head:");
                ImGui::SliderFloat("Head Height##Boss", &m_bossConfigDebug.headHeight, 0.5f, 8.0f);
                ImGui::SliderFloat("Head Radius##Boss", &m_bossConfigDebug.headRadius, 0.1f, 3.0f);
                ImGui::Text("  Body: %.2f x %.2f", m_bossConfigDebug.bodyWidth, m_bossConfigDebug.bodyHeight);
                ImGui::Text("  Head: Y=%.2f, R=%.2f", m_bossConfigDebug.headHeight, m_bossConfigDebug.headRadius);
                if (ImGui::Button("Copy to Clipboard##Boss"))
                {
                    char buffer[512];
                    sprintf_s(buffer,
                        "case EnemyType::BOSS:\n"
                        "    config.bodyWidth = %.2ff;\n"
                        "    config.bodyHeight = %.2ff;\n"
                        "    config.headHeight = %.2ff;\n"
                        "    config.headRadius = %.2ff;\n"
                        "    break;",
                        m_bossConfigDebug.bodyWidth, m_bossConfigDebug.bodyHeight,
                        m_bossConfigDebug.headHeight, m_bossConfigDebug.headRadius);
                    ImGui::SetClipboardText(buffer);
                }
                ImGui::TreePop();
            }

            if (ImGui::Button("Apply Hitbox to Physics"))
            {
                // 全敵の物理ボディを再作成
                for (auto& enemy : m_enemySystem->GetEnemies())
                {
                    if (!enemy.isAlive) continue;
                    RemoveEnemyPhysicsBody(enemy.id);
                    AddEnemyPhysicsBody(enemy);
                }
                //OutputDebugStringA("[DEBUG] Rebuilt all physics bodies with current hitbox values!\n");
            }

            // リセットボタン
            if (ImGui::Button("Reset All to Default"))
            {
                m_normalConfigDebug = GetEnemyConfig(EnemyType::NORMAL);
                m_runnerConfigDebug = GetEnemyConfig(EnemyType::RUNNER);
                m_tankConfigDebug = GetEnemyConfig(EnemyType::TANK);
                m_midbossConfigDebug = GetEnemyConfig(EnemyType::MIDBOSS);
                m_bossConfigDebug = GetEnemyConfig(EnemyType::BOSS);
                //OutputDebugStringA("Reset all hitbox configs to default\n");
            }

            ImGui::Separator();
            ImGui::TextWrapped("Tip: Adjust values while looking at hitboxes, then click 'Copy to Clipboard' and paste into Entities.h");
        }

        if (ImGui::TreeNode("Enemy Attack Timing"))
        {
            ImGui::Text("--- NORMAL ---");
            ImGui::SliderFloat("Hit Time##normal", &m_enemySystem->m_normalAttackHitTime, 0.1f, 10.0f, "%.2f");
            ImGui::SliderFloat("Duration##normal", &m_enemySystem->m_normalAttackDuration, 0.5f, 10.0f, "%.2f");

            ImGui::Text("--- RUNNER ---");
            ImGui::SliderFloat("Hit Time##runner", &m_enemySystem->m_runnerAttackHitTime, 0.05f, 10.0f, "%.2f");
            ImGui::SliderFloat("Duration##runner", &m_enemySystem->m_runnerAttackDuration, 0.3f, 10.0f, "%.2f");

            ImGui::Text("--- TANK ---");
            ImGui::SliderFloat("Hit Time##tank", &m_enemySystem->m_tankAttackHitTime, 0.3f, 10.0f, "%.2f");
            ImGui::SliderFloat("Duration##tank", &m_enemySystem->m_tankAttackDuration, 1.0f, 10.0f, "%.2f");

            ImGui::Separator();
            if (ImGui::TreeNode("Animation Speed"))
            {
                const char* typeNames[] = { "NORMAL", "RUNNER", "TANK", "MIDBOSS", "BOSS" };

                if (ImGui::TreeNode("Idle"))
                {
                    for (int i = 0; i < 5; i++)
                        ImGui::SliderFloat(typeNames[i], &m_enemySystem->m_animSpeed_Idle[i], 0.01f, 5.0f, "%.2f");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Walk"))
                {
                    for (int i = 0; i < 5; i++)
                        ImGui::SliderFloat(typeNames[i], &m_enemySystem->m_animSpeed_Walk[i], 0.01f, 5.0f, "%.2f");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Run"))
                {
                    for (int i = 0; i < 5; i++)
                        ImGui::SliderFloat(typeNames[i], &m_enemySystem->m_animSpeed_Run[i], 0.01f, 5.0f, "%.2f");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Attack"))
                {
                    for (int i = 0; i < 5; i++)
                        ImGui::SliderFloat(typeNames[i], &m_enemySystem->m_animSpeed_Attack[i], 0.01f, 5.0f, "%.2f");
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Death"))
                {
                    for (int i = 0; i < 5; i++)
                        ImGui::SliderFloat(typeNames[i], &m_enemySystem->m_animSpeed_Death[i], 0.01f, 5.0f, "%.2f");
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::Text("=== FBX Animation Durations (raw) ===");
            if (m_enemyModel)
            {
                ImGui::Text("NORMAL - Walk:%.2f  Attack:%.2f  Death:%.2f  Idle:%.2f  Run:%.2f",
                    m_enemyModel->GetAnimationDuration("Walk"),
                    m_enemyModel->GetAnimationDuration("Attack"),
                    m_enemyModel->GetAnimationDuration("Death"),
                    m_enemyModel->GetAnimationDuration("Idle"),
                    m_enemyModel->GetAnimationDuration("Run"));
            }
            if (m_runnerModel)
            {
                ImGui::Text("RUNNER - Walk:%.2f  Attack:%.2f  Death:%.2f  Idle:%.2f  Run:%.2f",
                    m_runnerModel->GetAnimationDuration("Walk"),
                    m_runnerModel->GetAnimationDuration("Attack"),
                    m_runnerModel->GetAnimationDuration("Death"),
                    m_runnerModel->GetAnimationDuration("Idle"),
                    m_runnerModel->GetAnimationDuration("Run"));
            }
            if (m_tankModel)
            {
                ImGui::Text("TANK   - Walk:%.2f  Attack:%.2f  Death:%.2f  Idle:%.2f  Run:%.2f",
                    m_tankModel->GetAnimationDuration("Walk"),
                    m_tankModel->GetAnimationDuration("Attack"),
                    m_tankModel->GetAnimationDuration("Death"),
                    m_tankModel->GetAnimationDuration("Idle"),
                    m_tankModel->GetAnimationDuration("Run"));
            }
            if (m_midBossModel)
            {
                ImGui::Text("MIDBOSS- Walk:%.2f  Attack:%.2f  Death:%.2f",
                    m_midBossModel->GetAnimationDuration("Walk"),
                    m_midBossModel->GetAnimationDuration("Attack"),
                    m_midBossModel->GetAnimationDuration("Death"));
            }
            if (m_bossModel)
            {
                ImGui::Text("BOSS   - Walk:%.2f  AtkJump:%.2f  AtkSlash:%.2f  Death:%.2f",
                    m_bossModel->GetAnimationDuration("Walk"),
                    m_bossModel->GetAnimationDuration("AttackJump"),
                    m_bossModel->GetAnimationDuration("AttackSlash"),
                    m_bossModel->GetAnimationDuration("Death"));
            }

            ImGui::Separator();
            ImGui::Text("=== Debug Spawn ===");

            // 全敵クリア
           // 全敵クリア＋ウェーブ停止
            if (ImGui::Button("Clear All Enemies"))
            {
                for (auto& e : m_enemySystem->GetEnemies())
                    RemoveEnemyPhysicsBody(e.id);
                m_enemySystem->GetEnemies().clear();
                m_waveManager->SetPaused(true);  // ウェーブ停止
            }
            ImGui::SameLine();
            if (ImGui::Button("Resume Waves"))
            {
                m_waveManager->SetPaused(false);  // ウェーブ再開
            }
            ImGui::SameLine();

            // 1体だけスポーン
            DirectX::XMFLOAT3 pPos = m_player->GetPosition();
            if (ImGui::Button("+ NORMAL"))
                m_enemySystem->SpawnEnemyOfType(EnemyType::NORMAL, pPos);
            ImGui::SameLine();
            if (ImGui::Button("+ RUNNER"))
                m_enemySystem->SpawnEnemyOfType(EnemyType::RUNNER, pPos);
            ImGui::SameLine();
            if (ImGui::Button("+ TANK"))
                m_enemySystem->SpawnEnemyOfType(EnemyType::TANK, pPos);

            ImGui::Separator();

            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type == EnemyType::MIDBOSS || enemy.type == EnemyType::BOSS) continue;
                if (enemy.currentAnimation != "Attack") continue;

                // タイプ別の値を取得
                float hitTime, duration;
                const char* typeName;
                ImVec4 typeColor;
                switch (enemy.type)
                {
                case EnemyType::RUNNER:
                    hitTime = m_enemySystem->m_runnerAttackHitTime;
                    duration = m_enemySystem->m_runnerAttackDuration;
                    typeName = "RUNNER";
                    typeColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    break;
                case EnemyType::TANK:
                    hitTime = m_enemySystem->m_tankAttackHitTime;
                    duration = m_enemySystem->m_tankAttackDuration;
                    typeName = "TANK";
                    typeColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
                    break;
                default:
                    hitTime = m_enemySystem->m_normalAttackHitTime;
                    duration = m_enemySystem->m_normalAttackDuration;
                    typeName = "NORMAL";
                    typeColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    break;
                }

                char label[64];
                sprintf_s(label, "[%s] ID:%d", typeName, enemy.id);
                ImGui::TextColored(typeColor, "%s", label);
                ImGui::Text("  --- Attack Timeline ---");

                // フェーズ構築（ボスと完全に同じ構造）
                struct PhaseInfo {
                    const char* name;
                    float duration;
                    ImVec4 color;
                    bool isDamagePhase;
                };

                float hitDuration = 0.05f;  // 打撃判定の長さ
                float recovery = duration - hitTime - hitDuration;
                if (recovery < 0.01f) recovery = 0.01f;

                std::vector<PhaseInfo> phases = {
                    {"WINDUP",   hitTime,     ImVec4(0.4f, 0.4f, 0.4f, 1), false},
                    {"HIT!",     hitDuration, ImVec4(1.0f, 0.0f, 0.0f, 1), true },
                    {"RECOVERY", recovery,    ImVec4(0.2f, 0.8f, 0.2f, 1), false},
                };

                // 現在のオフセットを計算
                float currentOffset = enemy.animationTime;

                // タイムラインバー描画
                float totalDuration = 0;
                for (auto& p : phases) totalDuration += p.duration;

                float barWidth = 280.0f;
                float barHeight = 20.0f;
                ImVec2 barStart = ImGui::GetCursorScreenPos();
                barStart.x += 20.0f;
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                float xOffset = 0;
                for (auto& p : phases)
                {
                    float w = (p.duration / totalDuration) * barWidth;
                    ImU32 col = ImGui::ColorConvertFloat4ToU32(p.color);

                    if (p.isDamagePhase)
                    {
                        float blink = sinf((float)ImGui::GetTime() * 10.0f) * 0.3f + 0.7f;
                        ImVec4 blinkColor = p.color;
                        blinkColor.w = blink;
                        col = ImGui::ColorConvertFloat4ToU32(blinkColor);
                    }

                    drawList->AddRectFilled(
                        ImVec2(barStart.x + xOffset, barStart.y),
                        ImVec2(barStart.x + xOffset + w, barStart.y + barHeight),
                        col);

                    if (w > 30)
                    {
                        drawList->AddText(
                            ImVec2(barStart.x + xOffset + 2, barStart.y + 3),
                            IM_COL32(255, 255, 255, 220), p.name);
                    }

                    xOffset += w;
                }

                drawList->AddRect(
                    barStart,
                    ImVec2(barStart.x + barWidth, barStart.y + barHeight),
                    IM_COL32(200, 200, 200, 180));

                float markerX = barStart.x + (currentOffset / totalDuration) * barWidth;
                markerX = (std::min)(markerX, barStart.x + barWidth);
                markerX = (std::max)(markerX, barStart.x);

                drawList->AddLine(
                    ImVec2(markerX, barStart.y - 2),
                    ImVec2(markerX, barStart.y + barHeight + 2),
                    IM_COL32(255, 255, 255, 255), 2.0f);

                drawList->AddTriangleFilled(
                    ImVec2(markerX - 5, barStart.y - 6),
                    ImVec2(markerX + 5, barStart.y - 6),
                    ImVec2(markerX, barStart.y - 1),
                    IM_COL32(255, 255, 0, 255));

                ImGui::Dummy(ImVec2(barWidth + 30, barHeight + 10));

                ImGui::Text("    Time: %.2f / %.2f sec", currentOffset, totalDuration);

                float elapsed = 0;
                for (auto& p : phases)
                {
                    if (currentOffset >= elapsed && currentOffset < elapsed + p.duration)
                    {
                        if (p.isDamagePhase)
                        {
                            ImGui::TextColored(ImVec4(1, 0, 0, 1),
                                "    >>> DAMAGE ACTIVE! <<< Parry NOW!");
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1),
                                "    [%s] No damage yet", p.name);
                        }
                        break;
                    }
                    elapsed += p.duration;
                }

                ImGui::TextColored(
                    enemy.attackJustLanded ? ImVec4(1, 0, 0, 1) : ImVec4(0.4f, 0.4f, 0.4f, 1),
                    "    attackJustLanded: %s",
                    enemy.attackJustLanded ? "TRUE (HIT FRAME!)" : "false");

                ImGui::Spacing();
            }

            ImGui::TreePop();
        }


        // =============================================
        // Boss/MidBoss AI Debug
        // =============================================
        if (ImGui::CollapsingHeader("Boss AI Debug"))
        {
            if (ImGui::CollapsingHeader("Boss Attack Timing"))
            {
                ImGui::Text("--- Melee ---");
                ImGui::SliderFloat("Melee Hit Time", &m_enemySystem->m_bossMeleeHitTime, 0.1f, 5.0f);
                ImGui::SliderFloat("Melee Duration", &m_enemySystem->m_bossMeleeDuration, 0.5f, 5.0f);

                ImGui::Text("--- Jump Slam ---");
                ImGui::SliderFloat("Jump Windup", &m_enemySystem->m_bossJumpWindupTime, 0.1f, 2.0f);
                ImGui::SliderFloat("Jump Air Time", &m_enemySystem->m_bossJumpAirTime, 0.2f, 2.0f);
                ImGui::SliderFloat("Jump Height", &m_enemySystem->m_bossJumpHeight, 2.0f, 20.0f);
                ImGui::SliderFloat("Slam Recovery", &m_enemySystem->m_bossSlamRecoveryTime, 0.1f, 3.0f);

                ImGui::Text("--- Slash ---");
                ImGui::SliderFloat("Slash Windup", &m_enemySystem->m_bossSlashWindupTime, 0.1f, 2.0f);
                ImGui::SliderFloat("Slash Fire", &m_enemySystem->m_bossSlashFireTime, 0.05f, 1.0f);
                ImGui::SliderFloat("Slash Recovery", &m_enemySystem->m_bossSlashRecoveryTime, 0.1f, 3.0f);

                ImGui::Text("--- Beam ---");
                ImGui::SliderFloat("Roar Windup", &m_enemySystem->m_bossRoarWindupTime, 0.1f, 5.0f);
                ImGui::SliderFloat("Roar Fire", &m_enemySystem->m_bossRoarFireTime, 1.0f, 10.0f);
                ImGui::SliderFloat("Roar Recovery", &m_enemySystem->m_bossRoarRecoveryTime, 0.5f, 5.0f);

                ImGui::Text("--- General ---");
                ImGui::SliderFloat("Attack Cooldown", &m_enemySystem->m_bossAttackCooldown, 0.5f, 10.0f);
            }

            // フェーズ名変換用
            auto phaseName = [](BossAttackPhase p) -> const char* {
                switch (p) {
                case BossAttackPhase::IDLE:           return "IDLE";
                case BossAttackPhase::JUMP_WINDUP:    return "JUMP_WINDUP";
                case BossAttackPhase::JUMP_AIR:       return "JUMP_AIR";
                case BossAttackPhase::JUMP_SLAM:      return "JUMP_SLAM";
                case BossAttackPhase::SLAM_RECOVERY:  return "SLAM_RECOVERY";
                case BossAttackPhase::SLASH_WINDUP:    return "SLASH_WINDUP";
                case BossAttackPhase::SLASH_FIRE:      return "SLASH_FIRE";
                case BossAttackPhase::SLASH_RECOVERY:  return "SLASH_RECOVERY";
                case BossAttackPhase::ROAR_WINDUP:     return "ROAR_WINDUP";
                case BossAttackPhase::ROAR_FIRE:       return "ROAR_FIRE";
                case BossAttackPhase::ROAR_RECOVERY:   return "ROAR_RECOVERY";
                default: return "UNKNOWN";
                }
                };

            int bossCount = 0;
            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive) continue;
                if (enemy.type != EnemyType::MIDBOSS && enemy.type != EnemyType::BOSS) continue;

                bossCount++;
                const char* typeName = (enemy.type == EnemyType::BOSS) ? "BOSS" : "MIDBOSS";
                ImVec4 headerColor = (enemy.type == EnemyType::BOSS)
                    ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
                    : ImVec4(1.0f, 0.6f, 0.0f, 1.0f);

                ImGui::TextColored(headerColor, "=== %s (ID:%d) ===", typeName, enemy.id);

                // 基本情報
                ImGui::Text("  HP: %.0f / %.0f", enemy.health, (float)GetEnemyConfig(enemy.type).health);
                ImGui::Text("  Pos: (%.1f, %.1f, %.1f)", enemy.position.x, enemy.position.y, enemy.position.z);

                // プレイヤーとの距離
                DirectX::XMFLOAT3 pPos = m_player->GetPosition();
                float dx = pPos.x - enemy.position.x;
                float dz = pPos.z - enemy.position.z;
                float dist = sqrtf(dx * dx + dz * dz);
                ImGui::Text("  Distance to Player: %.1f", dist);

                // AIフェーズ（色分け）
                BossAttackPhase phase = enemy.bossPhase;
                ImVec4 phaseColor(0.5f, 0.5f, 0.5f, 1.0f); // グレー
                if (phase == BossAttackPhase::IDLE) phaseColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                else if (phase == BossAttackPhase::JUMP_AIR || phase == BossAttackPhase::JUMP_SLAM)
                    phaseColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);  // オレンジ
                else if (phase == BossAttackPhase::SLASH_FIRE)
                    phaseColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // 赤
                else if (phase == BossAttackPhase::ROAR_FIRE)
                    phaseColor = ImVec4(0.8f, 0.2f, 1.0f, 1.0f);  // 紫
                else if (phase == BossAttackPhase::SLAM_RECOVERY ||
                    phase == BossAttackPhase::SLASH_RECOVERY ||
                    phase == BossAttackPhase::ROAR_RECOVERY)
                    phaseColor = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);  // 緑（チャンス）

                ImGui::TextColored(phaseColor, "  Phase: %s", phaseName(phase));
                ImGui::Text("  Phase Timer: %.2f s", enemy.bossPhaseTimer);
                ImGui::Text("  Cooldown: %.2f s", enemy.bossAttackCooldown);
                ImGui::Text("  Attack Count: %d", enemy.bossAttackCount);

                // スタン情報
                float stunPercent = (enemy.maxStunValue > 0) ? enemy.stunValue / enemy.maxStunValue : 0;
                ImGui::Text("  Stun: %.0f / %.0f", enemy.stunValue, enemy.maxStunValue);
                ImGui::ProgressBar(stunPercent, ImVec2(200, 14), "");
                if (enemy.isStaggered)
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  *** STAGGERED ***");
                {
                    // モデルからアニメの実際の長さを取得
                    float actualDuration = 0.0f;
                    if (enemy.type == EnemyType::BOSS && m_bossModel)
                        actualDuration = m_bossModel->GetAnimationDuration(enemy.currentAnimation);
                    else if (enemy.type == EnemyType::MIDBOSS && m_midBossModel)
                        actualDuration = m_midBossModel->GetAnimationDuration(enemy.currentAnimation);

                    ImGui::Text("  Animation: %s (t=%.2f / %.2f)",
                        enemy.currentAnimation.c_str(), enemy.animationTime, actualDuration);
                }
                // ビーム情報（MIDBOSSのみ）
                if (enemy.type == EnemyType::MIDBOSS)
                {
                    ImGui::TextColored(
                        enemy.bossBeamParriable ? ImVec4(0.0f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                        "  Beam: %s", enemy.bossBeamParriable ? "GREEN (Parriable)" : "RED (Guard only)");
                }

                // =============================================
                // 攻撃タイムライン可視化
                // =============================================
                // ★ 近接攻撃のタイムライン（IDLE中にAttack/AttackSlash）
                if (enemy.bossPhase == BossAttackPhase::IDLE &&
                    (enemy.currentAnimation == "Attack" || enemy.currentAnimation == "AttackSlash"))
                {
                    ImGui::Spacing();
                    ImGui::Text("  --- Melee Attack Timeline ---");

                    struct PhaseInfo {
                        const char* name;
                        float duration;
                        ImVec4 color;
                        bool isDamagePhase;
                    };

                    float hitTime = m_enemySystem->m_bossMeleeHitTime;
                    float totalDuration = m_enemySystem->m_bossMeleeDuration;
                    float hitDuration = 0.05f;
                    float recovery = totalDuration - hitTime - hitDuration;
                    if (recovery < 0.01f) recovery = 0.01f;

                    std::vector<PhaseInfo> phases = {
                        {"WINDUP",   hitTime,     ImVec4(0.4f, 0.4f, 0.4f, 1), false},
                        {"HIT!",     hitDuration, ImVec4(1.0f, 0.0f, 0.0f, 1), true },
                        {"RECOVERY", recovery,    ImVec4(0.2f, 0.8f, 0.2f, 1), false},
                    };

                    float currentOffset = enemy.animationTime;
                    float total = 0;
                    for (auto& p : phases) total += p.duration;

                    float barWidth = 280.0f;
                    float barHeight = 20.0f;
                    ImVec2 barStart = ImGui::GetCursorScreenPos();
                    barStart.x += 20.0f;
                    ImDrawList* drawList = ImGui::GetWindowDrawList();

                    float xOffset = 0;
                    for (auto& p : phases)
                    {
                        float w = (p.duration / total) * barWidth;
                        ImU32 col = ImGui::ColorConvertFloat4ToU32(p.color);
                        if (p.isDamagePhase)
                        {
                            float blink = sinf((float)ImGui::GetTime() * 10.0f) * 0.3f + 0.7f;
                            ImVec4 bc = p.color; bc.w = blink;
                            col = ImGui::ColorConvertFloat4ToU32(bc);
                        }
                        drawList->AddRectFilled(
                            ImVec2(barStart.x + xOffset, barStart.y),
                            ImVec2(barStart.x + xOffset + w, barStart.y + barHeight), col);
                        if (w > 30)
                            drawList->AddText(ImVec2(barStart.x + xOffset + 2, barStart.y + 3),
                                IM_COL32(255, 255, 255, 220), p.name);
                        xOffset += w;
                    }

                    drawList->AddRect(barStart,
                        ImVec2(barStart.x + barWidth, barStart.y + barHeight),
                        IM_COL32(200, 200, 200, 180));

                    float markerX = barStart.x + (currentOffset / total) * barWidth;
                    markerX = (std::min)(markerX, barStart.x + barWidth);
                    markerX = (std::max)(markerX, barStart.x);
                    drawList->AddLine(
                        ImVec2(markerX, barStart.y - 2),
                        ImVec2(markerX, barStart.y + barHeight + 2),
                        IM_COL32(255, 255, 255, 255), 2.0f);
                    drawList->AddTriangleFilled(
                        ImVec2(markerX - 5, barStart.y - 6),
                        ImVec2(markerX + 5, barStart.y - 6),
                        ImVec2(markerX, barStart.y - 1),
                        IM_COL32(255, 255, 0, 255));

                    ImGui::Dummy(ImVec2(barWidth + 30, barHeight + 10));
                    ImGui::Text("    Time: %.2f / %.2f sec", currentOffset, total);

                    float elapsed = 0;
                    for (auto& p : phases)
                    {
                        if (currentOffset >= elapsed && currentOffset < elapsed + p.duration)
                        {
                            if (p.isDamagePhase)
                                ImGui::TextColored(ImVec4(1, 0, 0, 1), "    >>> DAMAGE ACTIVE! <<< Parry NOW!");
                            else
                                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "    [%s] No damage yet", p.name);
                            break;
                        }
                        elapsed += p.duration;
                    }

                    ImGui::TextColored(
                        enemy.attackJustLanded ? ImVec4(1, 0, 0, 1) : ImVec4(0.4f, 0.4f, 0.4f, 1),
                        "    attackJustLanded: %s",
                        enemy.attackJustLanded ? "TRUE (HIT FRAME!)" : "false");
                }


                ImGui::Separator();
            }

            if (bossCount == 0)
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No BOSS/MIDBOSS alive");

            // プロジェクタイル情報
            if (!m_bossProjectiles.empty())
            {
                ImGui::Text("Active Projectiles: %d", (int)m_bossProjectiles.size());
                for (int i = 0; i < (int)m_bossProjectiles.size() && i < 5; i++)
                {
                    auto& p = m_bossProjectiles[i];
                    ImGui::TextColored(
                        p.isParriable ? ImVec4(0.0f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                        "  [%d] %s pos(%.1f,%.1f) life=%.1f",
                        i, p.isParriable ? "GREEN" : "RED",
                        p.position.x, p.position.z, p.lifetime);
                }
            }

            // シールド状態
            ImGui::Separator();
            const char* shieldNames[] = { "Idle", "Parrying", "Guarding", "Throwing", "Charging", "Broken" };
            int si = (int)m_shieldState;
            if (si >= 0 && si < 6)
                ImGui::Text("Shield State: %s", shieldNames[si]);
            ImGui::Text("Shield HP: %.0f / %.0f", m_shieldHP, m_shieldMaxHP);
            ImGui::Text("Beam Handle: %d", m_beamHandle);


            // =============================================
            // パリィタイミング
            // =============================================
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Parry Timing", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // パリィウィンドウ（リアルタイム調整）
                ImGui::SliderFloat("Parry Window (sec)", &m_parryWindowDuration, 0.05f, 0.5f, "%.3f");

                // 直前のパリィ結果
                float timeSinceAttempt = m_gameTime - m_lastParryAttemptTime;
                float timeSinceResult = m_gameTime - m_lastParryResultTime;

                if (timeSinceAttempt < 3.0f)
                {
                    ImGui::TextColored(
                        m_lastParryWasSuccess ? ImVec4(0, 1, 0.3f, 1) : ImVec4(1, 0.3f, 0, 1),
                        "Last Parry: %s (%.2fs ago)",
                        m_lastParryWasSuccess ? "SUCCESS!" : "MISS",
                        timeSinceAttempt);
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Last Parry: ---");
                }

                ImGui::Text("Success: %d / Fail: %d", m_parrySuccessCount, m_parryFailCount);

                // パリィウィンドウ可視化バー
                float parryRatio = (m_shieldState == ShieldState::Parrying)
                    ? m_parryWindowTimer / m_parryWindowDuration : 0.0f;
                ImGui::Text("Parry Window:");
                ImGui::SameLine();
                ImVec4 barColor = (parryRatio > 0) ? ImVec4(0, 1, 0.3f, 1) : ImVec4(0.3f, 0.3f, 0.3f, 1);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                ImGui::ProgressBar(parryRatio, ImVec2(150, 16), parryRatio > 0 ? "ACTIVE" : "");
                ImGui::PopStyleColor();
            }

            // =============================================
            // ボス攻撃パラメータ調整
            // =============================================
            if (ImGui::CollapsingHeader("Boss Attack Tuning"))
            {
                ImGui::Text("--- Jump Slam ---");
                ImGui::SliderFloat("Slam Radius (BOSS)", &m_slamRadiusBoss, 2.0f, 15.0f);
                ImGui::SliderFloat("Slam Radius (MID)", &m_slamRadiusMidBoss, 2.0f, 12.0f);
                ImGui::SliderFloat("Slam Damage (BOSS)", &m_slamDamageBoss, 10.0f, 100.0f);
                ImGui::SliderFloat("Slam Damage (MID)", &m_slamDamageMidBoss, 5.0f, 60.0f);
                ImGui::SliderFloat("Slam Stun on Parry", &m_slamStunDamage, 10.0f, 100.0f);

                ImGui::Separator();
                ImGui::Text("--- Slash Projectile ---");
                ImGui::SliderFloat("Slash Speed", &m_slashSpeed, 5.0f, 30.0f);
                ImGui::SliderFloat("Slash Damage", &m_slashDamage, 10.0f, 80.0f);
                ImGui::SliderFloat("Slash Hit Radius", &m_slashHitRadius, 0.5f, 3.0f);
                ImGui::SliderFloat("Slash Stun on Parry", &m_slashStunOnParry, 10.0f, 100.0f);

                ImGui::Separator();
                ImGui::Text("--- Beam ---");
                ImGui::SliderFloat("Beam Width", &m_beamWidth, 0.5f, 5.0f);
                ImGui::SliderFloat("Beam Length", &m_beamLength, 5.0f, 40.0f);
                ImGui::SliderFloat("Beam DPS", &m_beamDPS, 5.0f, 50.0f);
                ImGui::SliderFloat("Beam Stun on Parry", &m_beamStunOnParry, 10.0f, 80.0f);
            }

            // =============================================
            // AI タイミング調整
            // =============================================
            if (ImGui::CollapsingHeader("AI Phase Timing"))
            {
                ImGui::Text("--- Jump Attack ---");
                ImGui::SliderFloat("Jump Windup", &m_jumpWindupTime, 0.1f, 2.0f, "%.2f sec");
                ImGui::SliderFloat("Jump Air Time", &m_jumpAirTime, 0.2f, 2.0f, "%.2f sec");
                ImGui::SliderFloat("Slam Recovery", &m_slamRecoveryTime, 0.5f, 4.0f, "%.2f sec");

                ImGui::Separator();
                ImGui::Text("--- Slash Attack ---");
                ImGui::SliderFloat("Slash Windup", &m_slashWindupTime, 0.2f, 2.0f, "%.2f sec");
                ImGui::SliderFloat("Slash Recovery", &m_slashRecoveryTime, 0.3f, 3.0f, "%.2f sec");

                ImGui::Separator();
                ImGui::Text("--- Roar Beam ---");
                ImGui::SliderFloat("Roar Windup", &m_roarWindupTime, 0.5f, 5.0f, "%.2f sec");
                ImGui::SliderFloat("Roar Fire Time", &m_roarFireTime, 1.0f, 6.0f, "%.2f sec");
                ImGui::SliderFloat("Roar Recovery", &m_roarRecoveryTime, 0.5f, 4.0f, "%.2f sec");

                ImGui::Separator();
                ImGui::SliderFloat("Attack Cooldown", &m_bossAttackCooldownBase, 1.0f, 8.0f, "%.1f sec");

                if (ImGui::Button("Copy Current Values"))
                {
                    char buf[512];
                    sprintf_s(buf,
                        "// Jump: Windup=%.2f Air=%.2f Recovery=%.2f\n"
                        "// Slash: Windup=%.2f Recovery=%.2f\n"
                        "// Roar: Windup=%.2f Fire=%.2f Recovery=%.2f\n"
                        "// Parry Window=%.3f Cooldown=%.1f\n",
                        m_jumpWindupTime, m_jumpAirTime, m_slamRecoveryTime,
                        m_slashWindupTime, m_slashRecoveryTime,
                        m_roarWindupTime, m_roarFireTime, m_roarRecoveryTime,
                        m_parryWindowDuration, m_bossAttackCooldownBase);
                    ImGui::SetClipboardText(buf);
                    //OutputDebugStringA("[DEBUG] Boss timing values copied!\n");
                }
            }

        }

        if (ImGui::CollapsingHeader("Melee Charge System"))
        {
            // 現在の状態表示
            ImGui::Text("Charges: %d / %d", m_meleeCharges, m_meleeMaxCharges);

            // チャージゲージ（視覚的）
            float chargeRatio = (float)m_meleeCharges / (float)m_meleeMaxCharges;
            ImGui::ProgressBar(chargeRatio, ImVec2(-1, 20),
                m_meleeCharges > 0 ? "READY" : "EMPTY");

            // リチャージタイマー
            if (m_meleeCharges < m_meleeMaxCharges)
            {
                float rechargeProgress = m_meleeRechargeTimer / m_meleeRechargeTime;
                ImGui::ProgressBar(rechargeProgress, ImVec2(-1, 14), "Recharging...");
            }

            ImGui::Separator();

            // 調整パラメータ
            ImGui::SliderInt("Max Charges", &m_meleeMaxCharges, 1, 6);
            ImGui::SliderFloat("Recharge Time (sec)", &m_meleeRechargeTime, 1.0f, 15.0f);
            ImGui::SliderInt("Ammo per Punch", &m_meleeAmmoRefill, 1, 20);

            // 即リセットボタン
            if (ImGui::Button("Refill Charges"))
            {
                m_meleeCharges = m_meleeMaxCharges;
                m_meleeRechargeTimer = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Empty Charges"))
            {
                m_meleeCharges = 0;
            }
        }

        ImGui::End();
    }

    // ImGui描画
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Game::DrawCapsule(
    DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    const DirectX::XMFLOAT3& center,
    float radius,
    float cylinderHeight,
    const DirectX::XMFLOAT4& color)
{
    // カプセルは「シリンダー + 上下の半球」
    // 簡易版として、輪郭だけ描画

    const int segments = 16;  // 分割数
    const float pi = 3.14159f;

    // === 1. シリンダー部分（縦の線） ===
    float halfCylinder = cylinderHeight / 2.0f;

    for (int i = 0; i < segments; i++)
    {
        float angle = (float)i / segments * 2.0f * pi;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;

        DirectX::XMFLOAT3 bottom(center.x + x, center.y - halfCylinder, center.z + z);
        DirectX::XMFLOAT3 top(center.x + x, center.y + halfCylinder, center.z + z);

        batch->DrawLine(
            DirectX::VertexPositionColor(bottom, color),
            DirectX::VertexPositionColor(top, color)
        );
    }

    // === 2. シリンダーの上下の円 ===
    for (int i = 0; i < segments; i++)
    {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;

        // 下の円
        DirectX::XMFLOAT3 p1_bottom(
            center.x + cosf(angle1) * radius,
            center.y - halfCylinder,
            center.z + sinf(angle1) * radius
        );
        DirectX::XMFLOAT3 p2_bottom(
            center.x + cosf(angle2) * radius,
            center.y - halfCylinder,
            center.z + sinf(angle2) * radius
        );

        batch->DrawLine(
            DirectX::VertexPositionColor(p1_bottom, color),
            DirectX::VertexPositionColor(p2_bottom, color)
        );

        // 上の円
        DirectX::XMFLOAT3 p1_top(
            center.x + cosf(angle1) * radius,
            center.y + halfCylinder,
            center.z + sinf(angle1) * radius
        );
        DirectX::XMFLOAT3 p2_top(
            center.x + cosf(angle2) * radius,
            center.y + halfCylinder,
            center.z + sinf(angle2) * radius
        );

        batch->DrawLine(
            DirectX::VertexPositionColor(p1_top, color),
            DirectX::VertexPositionColor(p2_top, color)
        );
    }

    // === 3. 上下の半球（簡易版：弧を描画） ===
    for (int i = 0; i < segments / 2; i++)
    {
        float angle1 = (float)i / (segments / 2) * pi / 2.0f;
        float angle2 = (float)(i + 1) / (segments / 2) * pi / 2.0f;

        // 下の半球（4方向）
        for (int j = 0; j < 4; j++)
        {
            float dir = (float)j / 4.0f * 2.0f * pi;

            DirectX::XMFLOAT3 p1(
                center.x + cosf(dir) * cosf(angle1) * radius,
                center.y - halfCylinder - sinf(angle1) * radius,
                center.z + sinf(dir) * cosf(angle1) * radius
            );
            DirectX::XMFLOAT3 p2(
                center.x + cosf(dir) * cosf(angle2) * radius,
                center.y - halfCylinder - sinf(angle2) * radius,
                center.z + sinf(dir) * cosf(angle2) * radius
            );

            batch->DrawLine(
                DirectX::VertexPositionColor(p1, color),
                DirectX::VertexPositionColor(p2, color)
            );
        }

        // 上の半球（4方向）
        for (int j = 0; j < 4; j++)
        {
            float dir = (float)j / 4.0f * 2.0f * pi;

            DirectX::XMFLOAT3 p1(
                center.x + cosf(dir) * cosf(angle1) * radius,
                center.y + halfCylinder + sinf(angle1) * radius,
                center.z + sinf(dir) * cosf(angle1) * radius
            );
            DirectX::XMFLOAT3 p2(
                center.x + cosf(dir) * cosf(angle2) * radius,
                center.y + halfCylinder + sinf(angle2) * radius,
                center.z + sinf(dir) * cosf(angle2) * radius
            );

            batch->DrawLine(
                DirectX::VertexPositionColor(p1, color),
                DirectX::VertexPositionColor(p2, color)
            );
        }
    }
}

void Game::DrawHitboxes()
{
    if (!m_showHitboxes && !m_showHeadHitboxes && !m_showPhysicsHitboxes)
        return;

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    auto context = m_d3dContext.Get();
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive)
            continue;


        // === 体の当たり判定（タイプ別色分け） ===
        if (m_showHitboxes)
        {
            // === 設定を取得（デバッグモードなら m_XXXConfigDebug を使う）===
            EnemyTypeConfig config;

            if (m_useDebugHitboxes)
            {
                // デバッグ値を使う
                switch (enemy.type)
                {
                case EnemyType::NORMAL:
                    config = m_normalConfigDebug;
                    break;
                case EnemyType::RUNNER:
                    config = m_runnerConfigDebug;
                    break;
                case EnemyType::TANK:
                    config = m_tankConfigDebug;
                    break;
                case EnemyType::MIDBOSS:
                    config = m_midbossConfigDebug;
                    break;
                case EnemyType::BOSS:
                    config = m_bossConfigDebug;
                    break;
                }
            }
            else
            {
                // Entities.h の値を使う
                config = GetEnemyConfig(enemy.type);
            }

            float width = config.bodyWidth;
            float height = config.bodyHeight;

            // === タイプ別に色を変える ===
            DirectX::XMFLOAT4 bodyColor;
            switch (enemy.type)
            {
            case EnemyType::NORMAL:
                bodyColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
                break;
            case EnemyType::RUNNER:
                bodyColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            case EnemyType::TANK:
                bodyColor = DirectX::XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);
                break;
            }

            // 下の四角
            DirectX::XMFLOAT3 p1(enemy.position.x - width / 2, enemy.position.y, enemy.position.z - width / 2);
            DirectX::XMFLOAT3 p2(enemy.position.x + width / 2, enemy.position.y, enemy.position.z - width / 2);
            DirectX::XMFLOAT3 p3(enemy.position.x + width / 2, enemy.position.y, enemy.position.z + width / 2);
            DirectX::XMFLOAT3 p4(enemy.position.x - width / 2, enemy.position.y, enemy.position.z + width / 2);

            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p1, bodyColor), DirectX::VertexPositionColor(p2, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p2, bodyColor), DirectX::VertexPositionColor(p3, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p3, bodyColor), DirectX::VertexPositionColor(p4, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p4, bodyColor), DirectX::VertexPositionColor(p1, bodyColor));

            // 上の四角
            DirectX::XMFLOAT3 p5(enemy.position.x - width / 2, enemy.position.y + height, enemy.position.z - width / 2);
            DirectX::XMFLOAT3 p6(enemy.position.x + width / 2, enemy.position.y + height, enemy.position.z - width / 2);
            DirectX::XMFLOAT3 p7(enemy.position.x + width / 2, enemy.position.y + height, enemy.position.z + width / 2);
            DirectX::XMFLOAT3 p8(enemy.position.x - width / 2, enemy.position.y + height, enemy.position.z + width / 2);

            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p5, bodyColor), DirectX::VertexPositionColor(p6, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p6, bodyColor), DirectX::VertexPositionColor(p7, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p7, bodyColor), DirectX::VertexPositionColor(p8, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p8, bodyColor), DirectX::VertexPositionColor(p5, bodyColor));

            // 縦線
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p1, bodyColor), DirectX::VertexPositionColor(p5, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p2, bodyColor), DirectX::VertexPositionColor(p6, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p3, bodyColor), DirectX::VertexPositionColor(p7, bodyColor));
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(p4, bodyColor), DirectX::VertexPositionColor(p8, bodyColor));
        }

        // === 頭の当たり判定（球） ===
        if (m_showHeadHitboxes && !enemy.headDestroyed)
        {
            // === 設定を取得（デバッグモードなら m_XXXConfigDebug を使う）===
            EnemyTypeConfig config;

            if (m_useDebugHitboxes)
            {
                // デバッグ値を使う
                switch (enemy.type)
                {
                case EnemyType::NORMAL:
                    config = m_normalConfigDebug;
                    break;
                case EnemyType::RUNNER:
                    config = m_runnerConfigDebug;
                    break;
                case EnemyType::TANK:
                    config = m_tankConfigDebug;
                    break;
                case EnemyType::MIDBOSS:
                    config = m_midbossConfigDebug;
                    break;
                case EnemyType::BOSS:
                    config = m_bossConfigDebug;
                    break;
                }
            }
            else
            {
                // Entities.h の値を使う
                config = GetEnemyConfig(enemy.type);
            }

            DirectX::XMFLOAT3 headPos(
                enemy.position.x,
                enemy.position.y + config.headHeight,
                enemy.position.z
            );
            float headRadius = config.headRadius;

            DirectX::XMFLOAT4 headColor(1.0f, 0.0f, 0.0f, 1.0f);  // 赤

            // 球を円で表現（簡易版）
            int segments = 16;
            for (int i = 0; i < segments; i++)
            {
                float angle1 = (i / (float)segments) * 6.28318f;
                float angle2 = ((i + 1) / (float)segments) * 6.28318f;

                // XZ平面の円
                DirectX::XMFLOAT3 p1(headPos.x + cosf(angle1) * headRadius, headPos.y, headPos.z + sinf(angle1) * headRadius);
                DirectX::XMFLOAT3 p2(headPos.x + cosf(angle2) * headRadius, headPos.y, headPos.z + sinf(angle2) * headRadius);
                primitiveBatch->DrawLine(DirectX::VertexPositionColor(p1, headColor), DirectX::VertexPositionColor(p2, headColor));

                // XY平面の円
                DirectX::XMFLOAT3 p3(headPos.x + cosf(angle1) * headRadius, headPos.y + sinf(angle1) * headRadius, headPos.z);
                DirectX::XMFLOAT3 p4(headPos.x + cosf(angle2) * headRadius, headPos.y + sinf(angle2) * headRadius, headPos.z);
                primitiveBatch->DrawLine(DirectX::VertexPositionColor(p3, headColor), DirectX::VertexPositionColor(p4, headColor));

                // YZ平面の円
                DirectX::XMFLOAT3 p5(headPos.x, headPos.y + cosf(angle1) * headRadius, headPos.z + sinf(angle1) * headRadius);
                DirectX::XMFLOAT3 p6(headPos.x, headPos.y + cosf(angle2) * headRadius, headPos.z + sinf(angle2) * headRadius);
                primitiveBatch->DrawLine(DirectX::VertexPositionColor(p5, headColor), DirectX::VertexPositionColor(p6, headColor));
            }


        }

        // ========================================
        // === ここに追加: Bullet Physics カプセル ===
        // ========================================
        if (m_showPhysicsHitboxes)
        {
            auto it = m_enemyPhysicsBodies.find(enemy.id);
            if (it != m_enemyPhysicsBodies.end())
            {
                btRigidBody* body = it->second;
                btCollisionShape* shape = body->getCollisionShape();

                if(shape&& shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
                {
                    btCapsuleShape* capsule = static_cast<btCapsuleShape*>(shape);

                    float radius = capsule->getRadius();
                    float cylinderHeight = capsule->getHalfHeight() * 2.0f;

                    btTransform transform = body->getWorldTransform();
                    btVector3 origin = transform.getOrigin();

                    DirectX::XMFLOAT3 center(origin.x(), origin.y(), origin.z());
                    DirectX::XMFLOAT4 capsuleColor(0.0f, 1.0f, 0.0f, 1.0f);

                    DrawCapsule(primitiveBatch.get(), center, radius, cylinderHeight, capsuleColor);
                }
            }
        }

    }

    // === 弾の軌跡を描画 ===
    if (m_showBulletTrajectory)
    {
        for (const auto& trace : m_bulletTraces)
        {
            // 時間経過で色を変える
            float alpha = trace.lifetime / 0.5f;  // 0.5秒で消える
            DirectX::XMFLOAT4 traceColor(1.0f, 0.5f, 0.0f, alpha);  // オレンジ

            primitiveBatch->DrawLine(
                DirectX::VertexPositionColor(trace.start, traceColor),
                DirectX::VertexPositionColor(trace.end, traceColor)
            );
        }
    }

    // === レイの開始位置を可視化（緑の十字マーク）===
    if (m_showBulletTrajectory)
    {
        // 発射位置（実際の射撃と同じ計算）
        DirectX::XMFLOAT3 laserStart(playerPos.x, playerPos.y, playerPos.z);
        // 視線方向（実際の射撃と同じ計算）
        float dirX = sinf(playerRot.y) * cosf(playerRot.x);
        float dirY = -sinf(playerRot.x);
        float dirZ = cosf(playerRot.y) * cosf(playerRot.x);

        // 50m先まで線を伸ばす
        float laserLength = 50.0f;
        DirectX::XMFLOAT3 laserEnd(
            laserStart.x + dirX * laserLength,
            laserStart.y + dirY * laserLength,
            laserStart.z + dirZ * laserLength
        );

        // 赤いレーザー線
        DirectX::XMFLOAT4 laserColor(1.0f, 0.0f, 0.0f, 0.8f);
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(laserStart, laserColor),
            DirectX::VertexPositionColor(laserEnd, laserColor)
        );
    }

    //// X軸の線
    //primitiveBatch->DrawLine(
    //    DirectX::VertexPositionColor(
    //        DirectX::XMFLOAT3(rayStart.x - crossSize, rayStart.y, rayStart.z),
    //        crossColor
    //    ),
    //    DirectX::VertexPositionColor(
    //        DirectX::XMFLOAT3(rayStart.x + crossSize, rayStart.y, rayStart.z),
    //        crossColor
    //    )
    //);

    //// Y軸の線
    //primitiveBatch->DrawLine(
    //    DirectX::VertexPositionColor(
    //        DirectX::XMFLOAT3(rayStart.x, rayStart.y - crossSize, rayStart.z),
    //        crossColor
    //    ),
    //    DirectX::VertexPositionColor(
    //        DirectX::XMFLOAT3(rayStart.x, rayStart.y + crossSize, rayStart.z),
    //        crossColor
    //    )
    //);

    //// Z軸の線
    //primitiveBatch->DrawLine(
    //    DirectX::VertexPositionColor(
    //        DirectX::XMFLOAT3(rayStart.x, rayStart.y, rayStart.z - crossSize),
    //        crossColor
    //    ),
    //    DirectX::VertexPositionColor(
    //        DirectX::XMFLOAT3(rayStart.x, rayStart.y, rayStart.z + crossSize),
    //        crossColor
    //    )
    //);

    primitiveBatch->End();
}


void Game::DrawBulletTracers()
{
    if (m_bulletTraces.empty())
        return;

    // --- カメラ情報の取得 ---
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR camTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(camPos, camTarget, upVec);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    auto context = m_d3dContext.Get();

    // --- 加算ブレンドを設定（光が重なって明るくなる） ---
    context->OMSetBlendState(m_states->Additive(), nullptr, 0xFFFFFFFF);

    // --- 深度書き込みOFF（他のオブジェクトに隠れるが、自分は深度を汚さない） ---
    context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    batch->Begin();

    for (const auto& trace : m_bulletTraces)
    {
        // --- フェード率（1.0=生まれた瞬間 → 0.0=消える瞬間） ---
        float alpha = trace.lifetime / trace.maxLifetime;
        alpha = alpha * alpha;  // 二乗で急速にフェード

        if (alpha < 0.01f)
            continue;

        // --- ビルボード計算 ---
        // 弾道の方向ベクトル
        DirectX::XMVECTOR vStart = DirectX::XMLoadFloat3(&trace.start);
        DirectX::XMVECTOR vEnd = DirectX::XMLoadFloat3(&trace.end);
        DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(vEnd, vStart);
        DirectX::XMVECTOR dirNorm = DirectX::XMVector3Normalize(dir);

        // FPS視点用：始点を銃口位置（カメラの右下前方）にオフセット
        // カメラの右方向を計算
        DirectX::XMVECTOR camDir = DirectX::XMVectorSubtract(camTarget, camPos);
        camDir = DirectX::XMVector3Normalize(camDir);
        DirectX::XMVECTOR camRight = DirectX::XMVector3Cross(upVec, camDir);
        camRight = DirectX::XMVector3Normalize(camRight);

        // 銃口 = カメラから右に0.3m、下に0.2m、前に1.5m
        vStart = DirectX::XMVectorAdd(camPos, DirectX::XMVectorScale(camDir, 1.5f));
        vStart = DirectX::XMVectorAdd(vStart, DirectX::XMVectorScale(camRight, 0.3f));
        vStart = DirectX::XMVectorAdd(vStart, DirectX::XMVectorScale(upVec, -0.2f));

        // 弾道の中点からカメラへのベクトル
        DirectX::XMVECTOR midpoint = DirectX::XMVectorScale(
            DirectX::XMVectorAdd(vStart, vEnd), 0.5f
        );
        DirectX::XMVECTOR toCam = DirectX::XMVectorSubtract(camPos, midpoint);
        toCam = DirectX::XMVector3Normalize(toCam);

        // 外積で「幅の方向」を求める（カメラに正対する板になる）
        DirectX::XMVECTOR right = DirectX::XMVector3Cross(dirNorm, toCam);
        float rightLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(right));

        // カメラが弾道と一直線の場合のフォールバック
        if (rightLen < 0.001f)
        {
            right = DirectX::XMVector3Cross(dirNorm, upVec);
        }
        right = DirectX::XMVector3Normalize(right);

        // 幅のオフセット
        float halfW = trace.width * 0.5f;
        DirectX::XMVECTOR offset = DirectX::XMVectorScale(right, halfW);

        // --- 4頂点を計算 ---
        DirectX::XMFLOAT3 v0, v1, v2, v3;
        DirectX::XMStoreFloat3(&v0, DirectX::XMVectorSubtract(vStart, offset));
        DirectX::XMStoreFloat3(&v1, DirectX::XMVectorAdd(vStart, offset));
        DirectX::XMStoreFloat3(&v2, DirectX::XMVectorAdd(vEnd, offset));
        DirectX::XMStoreFloat3(&v3, DirectX::XMVectorSubtract(vEnd, offset));

        // --- 色（加算ブレンドなのでRGBが直接明るさ） ---
        DirectX::XMFLOAT4 coreColor = {
            trace.color.x,       // フル明るさ（加算で重なる）
            trace.color.y,
            trace.color.z,
            alpha                // フェードはalphaだけで制御
        };
        DirectX::XMFLOAT4 tailColor = {
            trace.color.x * 0.3f,
            trace.color.y * 0.3f,
            trace.color.z * 0.3f,
            alpha * 0.5f
        };

        // --- クワッド描画（三角形2枚） ---
        // 三角形1: v0(始点左) - v1(始点右) - v2(終点右)
        batch->DrawTriangle(
            DirectX::VertexPositionColor(v0, tailColor),
            DirectX::VertexPositionColor(v1, tailColor),
            DirectX::VertexPositionColor(v2, coreColor)
        );
        // 三角形2: v0(始点左) - v2(終点右) - v3(終点左)
        batch->DrawTriangle(
            DirectX::VertexPositionColor(v0, tailColor),
            DirectX::VertexPositionColor(v2, coreColor),
            DirectX::VertexPositionColor(v3, coreColor)
        );

        // --- 中央にもう1枚明るいコア線（さらに細い白い芯） ---
        float coreHalfW = halfW * 0.3f;
        DirectX::XMVECTOR coreOffset = DirectX::XMVectorScale(right, coreHalfW);

        DirectX::XMFLOAT3 c0, c1, c2, c3;
        DirectX::XMStoreFloat3(&c0, DirectX::XMVectorSubtract(vStart, coreOffset));
        DirectX::XMStoreFloat3(&c1, DirectX::XMVectorAdd(vStart, coreOffset));
        DirectX::XMStoreFloat3(&c2, DirectX::XMVectorAdd(vEnd, coreOffset));
        DirectX::XMStoreFloat3(&c3, DirectX::XMVectorSubtract(vEnd, coreOffset));

        // 芯は白く明るい
        DirectX::XMFLOAT4 whiteCore = { 1.0f, 1.0f, 1.0f, alpha };
        DirectX::XMFLOAT4 whiteTail = { 0.3f, 0.3f, 0.3f, alpha * 0.5f };

        batch->DrawTriangle(
            DirectX::VertexPositionColor(c0, whiteTail),
            DirectX::VertexPositionColor(c1, whiteTail),
            DirectX::VertexPositionColor(c2, whiteCore)
        );
        batch->DrawTriangle(
            DirectX::VertexPositionColor(c0, whiteTail),
            DirectX::VertexPositionColor(c2, whiteCore),
            DirectX::VertexPositionColor(c3, whiteCore)
        );
    }

    batch->End();

    // --- ブレンド状態を元に戻す ---
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
}


void Game::DrawEnemies(DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix, bool skipGloryKillTarget)
{
    static int frameCount = 0;
    frameCount++;

    ////	===	1秒ごとにデバッグ情報を出力	===
    //if (frameCount % 60 == 0)
    //{
    //    auto& enemies = m_enemySystem->GetEnemies();

    //    char debugMsg[512];
    //    sprintf_s(debugMsg, "=== Enemies: %zu ===\n", enemies.size());
    //    //OutputDebugStringA(debugMsg);
    //}

    //  深度書き込みを強制的に有効化
    m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    DirectX::XMFLOAT3 lightDir = { 1.0f, -1.0f, 1.0f };

    //	========================================================================
    //  インスタンスデータを作成
    //	========================================================================

    m_normalDead.clear();
    m_normalDeadHeadless.clear();
    m_runnerDead.clear();
    m_runnerDeadHeadless.clear();
    m_tankAttackingHeadless.clear();
    m_tankDead.clear();
    m_tankDeadHeadless.clear();
    m_midBossWalking.clear();
    m_midBossAttacking.clear();
    m_midBossWalkingHeadless.clear();
    m_midBossAttackingHeadless.clear();
    m_midBossDead.clear();
    m_midBossDeadHeadless.clear();
    m_bossWalking.clear();
    m_bossAttackingJump.clear();
    m_bossAttackingSlash.clear();
    m_bossAttackingJumpHeadless.clear();
    m_bossAttackingSlashHeadless.clear();
    m_bossWalkingHeadless.clear();
    m_bossDead.clear();
    m_bossDeadHeadless.clear();

    //  === 死亡アニメーション再生時間の取得    ===
    float deathDuration = m_enemyModel->GetAnimationDuration("Death");

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive && !enemy.isDying)
            continue;

        if (enemy.isExploded)
            continue;

        if (skipGloryKillTarget && m_gloryKillTargetEnemy && enemy.id == m_gloryKillTargetEnemy->id)
            continue;

        // ワールド行列を計算
        float s = 0.01f;
        if (enemy.type == EnemyType::BOSS) s = 0.015f;  // 1.5倍
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(s, s, s);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(enemy.rotationY);
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
            enemy.position.x,
            enemy.position.y,
            enemy.position.z
        );
        DirectX::XMMATRIX world = scale * rotation * translation;

        // ========================================================================
        //  死亡中の処理分岐
        // ========================================================================
        if (enemy.isDying)
        {
            // アニメーション終了判定（終端から0.1秒以内なら終了とみなす）
            bool animationFinished = (enemy.animationTime >= deathDuration - 0.1f);

            if (animationFinished)
            {
                // MIDBOSS/BOSSだけインスタンスデータが必要
                if (enemy.type == EnemyType::MIDBOSS || enemy.type == EnemyType::BOSS ||
                    (enemy.type == EnemyType::TANK && enemy.headDestroyed && enemy.currentAnimation == "Attack"))
                {
                    InstanceData instance;
                    DirectX::XMStoreFloat4x4(&instance.world, world);

                    if (enemy.isStaggered)
                    {
                        //  リムライト用: Color.a にスタガー情報を仕込む
                         // a > 1.5 → PSでスタガーと判定
                         // frac(a) → パルスアニメーションの位相
                        float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                        float rimBase = 2.0f;
                        switch (enemy.type)
                        {
                        case EnemyType::NORMAL:  rimBase = 2.0f; break;
                        case EnemyType::RUNNER:  rimBase = 2.0f; break;
                        case EnemyType::TANK:    rimBase = 3.0f; break;
                        case EnemyType::MIDBOSS: rimBase = 4.0f; break;
                        case EnemyType::BOSS:    rimBase = 5.0f; break;
                        }
                        float rimAlpha = rimBase + phase * 0.1f;
                        // 色は通常色を維持（紫のリムライトはシェーダーが足す）
                        instance.color = DirectX::XMFLOAT4(
                            enemy.color.x,
                            enemy.color.y,
                            enemy.color.z,
                            rimAlpha
                        );
                    }
                    else
                    {
                        instance.color = enemy.color;
                    }

                    switch (enemy.type)
                    {
                    case EnemyType::TANK:
                        m_tankAttackingHeadless.push_back(instance);
                        break;
                    case EnemyType::MIDBOSS:
                        if (enemy.headDestroyed)
                        {
                            if (enemy.currentAnimation == "Attack")
                                m_midBossAttackingHeadless.push_back(instance);
                            else
                                m_midBossWalkingHeadless.push_back(instance);
                        }
                        else
                        {
                            if (enemy.currentAnimation == "Attack")
                                m_midBossAttacking.push_back(instance);
                            else
                                m_midBossWalking.push_back(instance);
                        }
                        break;
                    case EnemyType::BOSS:
                        if (enemy.headDestroyed)
                        {
                            if (enemy.currentAnimation == "AttackJump")
                                m_bossAttackingJumpHeadless.push_back(instance);
                            else if (enemy.currentAnimation == "AttackSlash")
                                m_bossAttackingSlashHeadless.push_back(instance);
                            else
                                m_bossWalkingHeadless.push_back(instance);
                        }
                        else
                        {
                            if (enemy.currentAnimation == "AttackJump")
                                m_bossAttackingJump.push_back(instance);
                            else if (enemy.currentAnimation == "AttackSlash")
                                m_bossAttackingSlash.push_back(instance);
                            else
                                m_bossWalking.push_back(instance);
                        }
                        break;
                    }
                }


            }
            else
            {
                // ========================================================
                // アニメーション再生中 → 個別描画
                // ========================================================
                float headScale = enemy.headDestroyed ? 0.0f : 1.0f;
                m_enemyModel->SetBoneScale("Head", headScale);

                m_enemyModel->DrawAnimated(
                    m_d3dContext.Get(),
                    world,
                    viewMatrix,
                    projectionMatrix,
                    DirectX::XMLoadFloat4(&enemy.color),
                    "Death",
                    enemy.animationTime  // ← 個別の時間
                );

                m_enemyModel->SetBoneScale("Head", 1.0f);
            }

            // 影を描画（近い敵だけ）
            if (m_shadow)
            {
                float sdx = enemy.position.x - playerPos.x;
                float sdz = enemy.position.z - playerPos.z;
                float shadowDist2 = sdx * sdx + sdz * sdz;
                if (shadowDist2 < 400.0f)  // 20m以内だけ影描画
                {
                    m_shadow->RenderShadow(
                        m_d3dContext.Get(),
                        enemy.position,
                        1.5f,
                        viewMatrix,
                        projectionMatrix,
                        lightDir,
                        0.0f
                    );
                }
            }

            

            continue;  // 生きている敵の処理はスキップ
        }
        //=== スタンリング描画 ===
        if (m_stunRing && enemy.isStaggered && enemy.isAlive && !enemy.isDying)
        {
            float sdx = enemy.position.x - playerPos.x;
            float sdz = enemy.position.z - playerPos.z;
            float distSq = sdx * sdx + sdz * sdz;

            //  20m以上は描画しない、15?20mはフェードアウト
            if (distSq < 300.0f)  // 20m以内
            {
                float ringSize = 1.5f;
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  ringSize = 1.2f;  break;
                case EnemyType::RUNNER:  ringSize = 1.0f;  break;
                case EnemyType::TANK:    ringSize = 1.8f;  break;
                case EnemyType::MIDBOSS: ringSize = 2.2f;  break;
                case EnemyType::BOSS:    ringSize = 3.0f;  break;
                }

                //  15?20mで徐々に縮小（フェードアウト）
                float dist = sqrtf(distSq);
                float fadeStart = 12.0f;  // ここからフェード開始
                float fadeEnd = 17.3f;    // ここで完全に消える
                if (dist > fadeStart)
                {
                    float fade = 1.0f - (dist - fadeStart) / (fadeEnd - fadeStart);
                    ringSize *= fade;  // 遠いほど小さく→自然に消える
                }

                DirectX::XMFLOAT3 ringPos = enemy.position;
                ringPos.y += 0.9f;

                m_stunRing->Render(
                    m_d3dContext.Get(),
                    ringPos,
                    ringSize,
                    enemy.staggerTimer,
                    viewMatrix,
                    projectionMatrix
                );
            }

            // 敵タイプ別のリングサイズ
        }

        // ========================================================================
        // 生きている敵はインスタンスリストへ追加
        // ========================================================================
        InstanceData instance;
        DirectX::XMStoreFloat4x4(&instance.world, world);

        //  === よろめき状態なら点滅させる   ===
        if (enemy.isStaggered)
        {
            //  リムライト用: Color.a にスタガー情報を仕込む
             // a > 1.5 → PSでスタガーと判定
             // frac(a) → パルスアニメーションの位相
            float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
            float rimBase = 2.0f;
            switch (enemy.type)
            {
            case EnemyType::NORMAL:  rimBase = 2.0f; break;
            case EnemyType::RUNNER:  rimBase = 2.0f; break;
            case EnemyType::TANK:    rimBase = 3.0f; break;
            case EnemyType::MIDBOSS: rimBase = 4.0f; break;
            case EnemyType::BOSS:    rimBase = 5.0f; break;
            }
            float rimAlpha = rimBase + phase * 0.1f;
            // 色は通常色を維持（紫のリムライトはシェーダーが足す）
            instance.color = DirectX::XMFLOAT4(
            enemy.color.x,
            enemy.color.y,
            enemy.color.z,
            rimAlpha
            );
        }
        else
        {
            instance.color = enemy.color;
        }

        switch (enemy.type)
        {
        case EnemyType::NORMAL:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                {}//normalAttackingHeadless.push_back(instance);
                else
                {}//}normalWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                {}//}normalAttacking.push_back(instance);
                else
                {}//normalWalking.push_back(instance);
            }
            break;

        case EnemyType::RUNNER:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                {}//runnerAttackingHeadless.push_back(instance);
                else
                {}//runnerWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                {}//runnerAttacking.push_back(instance);
                else
                {}//runnerWalking.push_back(instance);
            }
            break;

        case EnemyType::TANK:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                    m_tankAttackingHeadless.push_back(instance);
                else
                {}//tankWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                {}//tankAttacking.push_back(instance);
                else
                {}//tankWalking.push_back(instance);
            }
            break;

            //MIDBOSS
            case EnemyType::MIDBOSS:
            //  スタガー中は個別描画
            if (enemy.isStaggered)
            {
                std::string stunAnim = m_midBossModel->HasAnimation("Stun") ? "Stun" : "Idle";
                float animTime = m_midBossModel->HasAnimation("Stun")
                    ? enemy.animationTime
                    : 0.0f;
                m_midBossModel->DrawAnimated(
                    m_d3dContext.Get(),
                    world, viewMatrix, projectionMatrix,
                    DirectX::XMLoadFloat4(&instance.color),
                    stunAnim, animTime
                );
            }
            else if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                    m_midBossAttackingHeadless.push_back(instance);
                else
                    m_midBossWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                    m_midBossAttacking.push_back(instance);
                else
                    m_midBossWalking.push_back(instance);
            }
            break;

        case EnemyType::BOSS:
            //  スタガー中はインスタンシングせず個別描画（Idleで静止）
            if (enemy.isStaggered)
            {
                std::string stunAnim = m_bossModel->HasAnimation("Stun") ? "Stun" : "Idle";
                float animTime = m_bossModel->HasAnimation("Stun")
                    ? enemy.animationTime
                    : 0.0f;  // Idleの先頭フレームで固定
                m_bossModel->DrawAnimated(
                    m_d3dContext.Get(),
                    world, viewMatrix, projectionMatrix,
                    DirectX::XMLoadFloat4(&instance.color),
                    stunAnim, animTime
                );
            }
            else if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "AttackJump")
                    m_bossAttackingJumpHeadless.push_back(instance);
                else if (enemy.currentAnimation == "AttackSlash")
                    m_bossAttackingSlashHeadless.push_back(instance);
                else
                    m_bossWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "AttackJump")
                    m_bossAttackingJump.push_back(instance);
                else if (enemy.currentAnimation == "AttackSlash")
                    m_bossAttackingSlash.push_back(instance);
                else
                    m_bossWalking.push_back(instance);
            }
            break;
        }

        // 影を描画（近い敵だけ）
        if (m_shadow)
        {
            float sdx = enemy.position.x - playerPos.x;
            float sdz = enemy.position.z - playerPos.z;
            float shadowDist2 = sdx * sdx + sdz * sdz;
            if (shadowDist2 < 400.0f)  // 20m以内だけ影描画
            {
                m_shadow->RenderShadow(
                    m_d3dContext.Get(),
                    enemy.position,
                    1.5f,
                    viewMatrix,
                    projectionMatrix,
                    lightDir,
                    0.0f
                );
            }
        }
    }

// ========================================================================
// インスタンシング描画（タイプ別）
// ========================================================================

// ====================================================================
// NORMAL（Y_Bot）の描画
// ====================================================================
    if (m_enemyModel)
    {
         // ================================================================
         // [PERF] NORMAL: Head ON -  インスタンシング描画
         // ================================================================
        m_enemyModel->SetBoneScale("Head", 1.0f);
        {
            {
                m_instWorlds.clear();   // サイズ0にするだけ。メモリは解放しない！
                m_instColors.clear();
                m_instAnims.clear();
                m_instTimes.clear();

                for (const auto& enemy : m_enemySystem->GetEnemies())
                {
                    if (!enemy.isAlive || enemy.isDying) continue;
                    if (enemy.type != EnemyType::NORMAL) continue;
                    if (enemy.headDestroyed) continue;

                    float s = 0.01f;
                    m_instWorlds.push_back(
                        DirectX::XMMatrixScaling(s, s, s)
                        * DirectX::XMMatrixRotationY(enemy.rotationY)
                        * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                    );

                    DirectX::XMFLOAT4 finalColor = enemy.color;
                    if (enemy.isStaggered)
                    {
                        float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                        finalColor.w = 2.0f + phase;
                    }
                    m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                    m_instAnims.push_back(enemy.currentAnimation);
                    m_instTimes.push_back(enemy.animationTime);
                }

                if (!m_instWorlds.empty())
                {
                    m_enemyModel->DrawInstanced_Custom(
                        m_d3dContext.Get(), viewMatrix, projectionMatrix,
                        m_instWorlds, m_instColors, m_instAnims, m_instTimes);
                }
            }
        }

        // Dead - head on (instanced)
        if (!m_normalDead.empty())
        {
            float finalTime = deathDuration - 0.001f;
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), m_normalDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_enemyModel->SetBoneScale("Head", 0.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::NORMAL) continue;
                if (!enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_enemyModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - headless (instanced)
        if (!m_normalDeadHeadless.empty())
        {
            float finalTime = deathDuration - 0.001f;
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), m_normalDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_enemyModel->SetBoneScale("Head", 1.0f);
    }


    // ====================================================================
    // RUNNER
    // ====================================================================
    if (m_runnerModel)
    {
        // ================================================================
        // [PERF] RUNNER: Head ON - walking+attacking unified loop
        // ================================================================
        m_runnerModel->SetBoneScale("Head", 1.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::RUNNER) continue;
                if (enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_runnerModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - head on (instanced)
        if (!m_runnerDead.empty())
        {
            float finalTime = m_runnerModel->GetAnimationDuration("Death") - 0.001f;
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), m_runnerDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // ================================================================
        // [PERF] RUNNER: Headless - walking+attacking unified loop
        // ================================================================
        m_runnerModel->SetBoneScale("Head", 0.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::RUNNER) continue;
                if (!enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_runnerModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - headless (instanced)
        if (!m_runnerDeadHeadless.empty())
        {
            float finalTime = m_runnerModel->GetAnimationDuration("Death") - 0.001f;
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), m_runnerDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_runnerModel->SetBoneScale("Head", 1.0f);
    }


    // ====================================================================
    // TANK
    // ====================================================================
    if (m_tankModel)
    {
        // ================================================================
        // [PERF] TANK: Head ON - walking+attacking unified loop
        // ================================================================
        m_tankModel->SetBoneScale("Head", 1.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::TANK) continue;
                if (enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_tankModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - head on (instanced)
        if (!m_tankDead.empty())
        {
            float finalTime = m_tankModel->GetAnimationDuration("Death") - 0.001f;
            m_tankModel->DrawInstanced(m_d3dContext.Get(), m_tankDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // ================================================================
        // [PERF] TANK: Headless - walking unified loop
        // ================================================================
        m_tankModel->SetBoneScale("Head", 0.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::TANK) continue;
                if (!enemy.headDestroyed) continue;
                if (enemy.currentAnimation == "Attack") continue;  // // 元のロジック維持

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_tankModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Attacking headless (instanced with shared timer)
        if (!m_tankAttackingHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[2],
                m_tankModel->GetAnimationDuration("Attack"));
            m_tankModel->DrawInstanced(m_d3dContext.Get(), m_tankAttackingHeadless,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - headless (instanced)
        if (!m_tankDeadHeadless.empty())
        {
            float finalTime = m_tankModel->GetAnimationDuration("Death") - 0.001f;
            m_tankModel->DrawInstanced(m_d3dContext.Get(), m_tankDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_tankModel->SetBoneScale("Head", 1.0f);
    }

    // ====================================================================
    // MIDBOSS の描画
    // ====================================================================
    if (m_midBossModel)
    {
        m_midBossModel->SetBoneScale("Head", 1.0f);

        // Walking - 頭あり
        if (!m_midBossWalking.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[3],
                m_midBossModel->GetAnimationDuration("Walk"));
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭あり
        if (!m_midBossAttacking.empty())
        {
            float duration = m_midBossModel->GetAnimationDuration("Attack");
            float attackTime = fmod(m_typeAttackTimer[3], duration);

            // ビーム発射中 or リカバリー中 → 最終フレーム固定
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive &&
                    (enemy.bossPhase == BossAttackPhase::ROAR_FIRE ||
                        enemy.bossPhase == BossAttackPhase::ROAR_RECOVERY))
                {
                    attackTime = duration - 0.01f;  // 最後のフレーム
                    break;
                }
            }

            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭あり
        if (!m_midBossDead.empty())
        {
            float finalTime = m_midBossModel->GetAnimationDuration("Death") - 0.001f;
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // 頭なし描画
        m_midBossModel->SetBoneScale("Head", 0.0f);

        if (!m_midBossAttacking.empty())
        {
            float duration = m_midBossModel->GetAnimationDuration("Attack");
            float attackTime = fmod(m_typeAttackTimer[3], duration);

            // ビーム発射中 or リカバリー中 → 最終フレーム固定
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive &&
                    (enemy.bossPhase == BossAttackPhase::ROAR_FIRE ||
                        enemy.bossPhase == BossAttackPhase::ROAR_RECOVERY))
                {
                    attackTime = duration - 0.01f;  // 最後のフレーム
                    break;
                }
            }

            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        if (!m_midBossAttackingHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[3],
                m_midBossModel->GetAnimationDuration("Attack"));
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossAttackingHeadless,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        if (!m_midBossDeadHeadless.empty())
        {
            float finalTime = m_midBossModel->GetAnimationDuration("Death") - 0.001f;
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_midBossModel->SetBoneScale("Head", 1.0f);
    }

    // ====================================================================
    // BOSS の描画
    // ====================================================================
    if (m_bossModel)
    {
        m_bossModel->SetBoneScale("Head", 1.0f);

        // Walking - 頭あり
        if (!m_bossWalking.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[4],
                m_bossModel->GetAnimationDuration("Walk"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // ジャンプ叩き
        if (!m_bossAttackingJump.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackJump"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingJump,
                viewMatrix, projectionMatrix, "AttackJump", attackTime);
        }

        // 殴り上げ斬撃
        if (!m_bossAttackingSlash.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackSlash"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingSlash,
                viewMatrix, projectionMatrix, "AttackSlash", attackTime);
        }

        // Dead - 頭あり
        if (!m_bossDead.empty())
        {
            float finalTime = m_bossModel->GetAnimationDuration("Death") - 0.001f;
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // --- Headless variants ---
        m_bossModel->SetBoneScale("Head", 0.0f);

        if (!m_bossWalkingHeadless.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[4],
                m_bossModel->GetAnimationDuration("Walk"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossWalkingHeadless,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        if (!m_bossAttackingJumpHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackJump"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingJumpHeadless,
                viewMatrix, projectionMatrix, "AttackJump", attackTime);
        }

        if (!m_bossAttackingSlashHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackSlash"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingSlashHeadless,
                viewMatrix, projectionMatrix, "AttackSlash", attackTime);
        }


        if (!m_bossDeadHeadless.empty())
        {
            float finalTime = m_bossModel->GetAnimationDuration("Death") - 0.001f;
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_bossModel->SetBoneScale("Head", 1.0f);
    }

    // === ターゲットマーカー描画 ===               // ★ここに追加（4377行目）
    if (m_targetMarker && m_shieldState == ShieldState::Guarding && m_guardLockTargetID >= 0)
    {
        float markerSize = 1.5f;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.id == m_guardLockTargetID && enemy.isAlive)
            {
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  markerSize = 1.2f;  break;
                case EnemyType::RUNNER:  markerSize = 1.0f;  break;
                case EnemyType::TANK:    markerSize = 1.8f;  break;
                case EnemyType::MIDBOSS: markerSize = 2.5f;  break;
                case EnemyType::BOSS:    markerSize = 3.5f;  break;
                }
                break;
            }
        }

        DirectX::XMFLOAT3 markerPos = m_guardLockTargetPos;
        markerPos.y += 1.0f;

        m_targetMarker->Render(
            m_d3dContext.Get(),
            markerPos,
            markerSize,
            m_gameTime,
            viewMatrix,
            projectionMatrix
        );
    }

    //	========================================================================
    //	HPバーを描画
    //	========================================================================
    auto context = m_d3dContext.Get();
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // 両面描画（ビルボードの裏表問題を回避）
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;  // カリングしない
    rsDesc.DepthClipEnable = TRUE;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> noCullRS;
    m_d3dDevice->CreateRasterizerState(&rsDesc, &noCullRS);
    context->RSSetState(noCullRS.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // カメラの右方向・上方向を計算（ビルボード用）
    //DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMFLOAT3 pRot = m_player->GetRotation();
    DirectX::XMVECTOR camDir = DirectX::XMVectorSet(
        sinf(pRot.y) * cosf(pRot.x),
        -sinf(pRot.x),
        cosf(pRot.y) * cosf(pRot.x), 0.0f
    );
    DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0, 1, 0, 0);
    DirectX::XMVECTOR camRight = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(worldUp, camDir)
    );
    DirectX::XMVECTOR camUp = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(camDir, camRight)
    );

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying || enemy.health >= enemy.maxHealth
            || enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS)
            continue;
        if (enemy.isExploded)
            continue;

        float barWidth = 1.0f;
        float barHeight = 0.08f;
        float healthPercent = (float)enemy.health / enemy.maxHealth;

        // バーの中心（敵の頭上）
        DirectX::XMVECTOR center = DirectX::XMVectorSet(
            enemy.position.x, enemy.position.y + 2.5f, enemy.position.z, 0.0f
        );

        // 右方向と上方向のオフセット
        DirectX::XMVECTOR halfRight = DirectX::XMVectorScale(camRight, barWidth * 0.5f);
        DirectX::XMVECTOR halfUp = DirectX::XMVectorScale(camUp, barHeight * 0.5f);

        // === 背景バー（暗い赤、全幅）===
        DirectX::XMFLOAT3 bg0, bg1, bg2, bg3;
        DirectX::XMStoreFloat3(&bg0, DirectX::XMVectorSubtract(DirectX::XMVectorSubtract(center, halfRight), halfUp));
        DirectX::XMStoreFloat3(&bg1, DirectX::XMVectorSubtract(DirectX::XMVectorAdd(center, halfRight), halfUp));
        DirectX::XMStoreFloat3(&bg2, DirectX::XMVectorAdd(DirectX::XMVectorAdd(center, halfRight), halfUp));
        DirectX::XMStoreFloat3(&bg3, DirectX::XMVectorAdd(DirectX::XMVectorSubtract(center, halfRight), halfUp));

        DirectX::XMFLOAT4 bgColor(0.2f, 0.0f, 0.0f, 0.8f);
        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(bg0, bgColor),
            DirectX::VertexPositionColor(bg1, bgColor),
            DirectX::VertexPositionColor(bg2, bgColor)
        );
        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(bg0, bgColor),
            DirectX::VertexPositionColor(bg2, bgColor),
            DirectX::VertexPositionColor(bg3, bgColor)
        );

        // === HPバー（赤→黄→緑のグラデ、HP割合分の幅）===
       // 背景より手前に描画（Zファイティング回避）
        DirectX::XMVECTOR hpCenter = DirectX::XMVectorSubtract(center, DirectX::XMVectorScale(camDir, 0.01f));

        DirectX::XMVECTOR hpLeftBottom = DirectX::XMVectorSubtract(DirectX::XMVectorSubtract(hpCenter, halfRight), halfUp);
        DirectX::XMVECTOR hpRightBottom = DirectX::XMVectorSubtract(DirectX::XMVectorAdd(DirectX::XMVectorSubtract(hpCenter, halfRight), DirectX::XMVectorScale(camRight, barWidth * healthPercent)), halfUp);
        DirectX::XMVECTOR hpRightTop = DirectX::XMVectorAdd(DirectX::XMVectorAdd(DirectX::XMVectorSubtract(hpCenter, halfRight), DirectX::XMVectorScale(camRight, barWidth * healthPercent)), halfUp);
        DirectX::XMVECTOR hpLeftTop = DirectX::XMVectorAdd(DirectX::XMVectorSubtract(hpCenter, halfRight), halfUp);

        DirectX::XMFLOAT3 hp0, hp1, hp2, hp3;
        DirectX::XMStoreFloat3(&hp0, hpLeftBottom);
        DirectX::XMStoreFloat3(&hp1, hpRightBottom);
        DirectX::XMStoreFloat3(&hp2, hpRightTop);
        DirectX::XMStoreFloat3(&hp3, hpLeftTop);

        // HP割合で色を変える（高=緑、中=黄、低=赤）
        DirectX::XMFLOAT4 hpColor;
        if (healthPercent > 0.5f)
        {
            float t = (healthPercent - 0.5f) * 2.0f;
            hpColor = { 1.0f - t, 1.0f, 0.0f, 1.0f };  // 黄→緑
        }
        else
        {
            float t = healthPercent * 2.0f;
            hpColor = { 1.0f, t, 0.0f, 1.0f };  // 赤→黄
        }

        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(hp0, hpColor),
            DirectX::VertexPositionColor(hp1, hpColor),
            DirectX::VertexPositionColor(hp2, hpColor)
        );
        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(hp0, hpColor),
            DirectX::VertexPositionColor(hp2, hpColor),
            DirectX::VertexPositionColor(hp3, hpColor)
        );

        // === 枠線（白） ===
        DirectX::XMFLOAT4 frameColor(0.8f, 0.8f, 0.8f, 0.6f);
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg0, frameColor),
            DirectX::VertexPositionColor(bg1, frameColor)
        );
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg1, frameColor),
            DirectX::VertexPositionColor(bg2, frameColor)
        );
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg2, frameColor),
            DirectX::VertexPositionColor(bg3, frameColor)
        );
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg3, frameColor),
            DirectX::VertexPositionColor(bg0, frameColor)
        );    
    }

    // === 収縮リング式・攻撃予告 ===
    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying || enemy.isExploded)
            continue;

        if ((enemy.currentAnimation == "Attack" || enemy.currentAnimation == "AttackSlash") && !enemy.attackJustLanded)
        {
            float hitTime;
            switch (enemy.type)
            {
            case EnemyType::RUNNER:  hitTime = m_enemySystem->m_runnerAttackHitTime; break;
            case EnemyType::TANK:    hitTime = m_enemySystem->m_tankAttackHitTime;   break;
            case EnemyType::MIDBOSS:
            case EnemyType::BOSS:    hitTime = m_enemySystem->m_bossMeleeHitTime;    break;
            default:                 hitTime = m_enemySystem->m_normalAttackHitTime;  break;
            }

            float timeToHit = hitTime - enemy.animationTime;
            float warningWindow = 1.0f;

            if (timeToHit > 0.0f && timeToHit < warningWindow)
            {
                float urgency = 1.0f - (timeToHit / warningWindow);

                // --- リング中心（敵の胸あたり） ---
                DirectX::XMVECTOR ringCenter = DirectX::XMVectorSet(
                    enemy.position.x, enemy.position.y + 1.5f, enemy.position.z, 0.0f
                );

                // --- 収縮リングの半径（大→小に縮む） ---
                float maxRadius = 2.5f;
                float minRadius = 0.5f;  // 敵の体に重なるサイズ
                float ringRadius = maxRadius - urgency * (maxRadius - minRadius);

                // --- 色の遷移 ---
                DirectX::XMFLOAT4 ringColor;
                float alpha;
                float parryZone = 0.25f;

                if (timeToHit < parryZone)
                {
                    // // 緑 = 「今パリィ！」
                    float pulse = (sinf(m_gameTime * 30.0f) + 1.0f) * 0.5f;
                    alpha = 0.8f + pulse * 0.2f;
                    ringColor = { 0.0f, 1.0f, 0.3f, alpha };
                }
                else if (timeToHit < 0.5f)
                {
                    // 赤 = 「もうすぐ！」
                    float pulse = (sinf(m_gameTime * 16.0f) + 1.0f) * 0.5f;
                    alpha = 0.5f + urgency * 0.3f + pulse * 0.2f;
                    ringColor = { 1.0f, 0.1f, 0.0f, alpha };
                }
                else
                {
                    // 暗い赤 = 「攻撃が来るぞ」
                    alpha = 0.2f + urgency * 0.3f;
                    ringColor = { 0.8f, 0.2f, 0.0f, alpha };
                }

                // --- 六角形リング描画（カメラ正対ビルボード） ---
                const int SEGMENTS = 6;
                float rotation = m_gameTime * 1.5f;  // ゆっくり回転

                DirectX::XMFLOAT3 ringVerts[SEGMENTS];
                for (int s = 0; s < SEGMENTS; s++)
                {
                    float angle = rotation + (float)s / SEGMENTS * 6.2832f;
                    DirectX::XMVECTOR pt = ringCenter;
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camRight, cosf(angle) * ringRadius));
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camUp, sinf(angle) * ringRadius));
                    DirectX::XMStoreFloat3(&ringVerts[s], pt);
                }

                // 外周リング線
                for (int s = 0; s < SEGMENTS; s++)
                {
                    int next = (s + 1) % SEGMENTS;
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(ringVerts[s], ringColor),
                        DirectX::VertexPositionColor(ringVerts[next], ringColor)
                    );
                }

                // --- 内側固定リング（ターゲットサイズ = パリィタイミング表示）---
                DirectX::XMFLOAT4 innerColor = { ringColor.x * 0.4f, ringColor.y * 0.4f, ringColor.z * 0.4f, alpha * 0.3f };
                DirectX::XMFLOAT3 innerVerts[SEGMENTS];
                for (int s = 0; s < SEGMENTS; s++)
                {
                    float angle = rotation + (float)s / SEGMENTS * 6.2832f;
                    DirectX::XMVECTOR pt = ringCenter;
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camRight, cosf(angle) * minRadius));
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camUp, sinf(angle) * minRadius));
                    DirectX::XMStoreFloat3(&innerVerts[s], pt);
                }
                for (int s = 0; s < SEGMENTS; s++)
                {
                    int next = (s + 1) % SEGMENTS;
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(innerVerts[s], innerColor),
                        DirectX::VertexPositionColor(innerVerts[next], innerColor)
                    );
                }

                // --- 収束線（外リング→内リングを繋ぐ6本のガイド線）---
                DirectX::XMFLOAT4 lineColor = { ringColor.x, ringColor.y, ringColor.z, alpha * 0.15f };
                for (int s = 0; s < SEGMENTS; s++)
                {
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(ringVerts[s], lineColor),
                        DirectX::VertexPositionColor(innerVerts[s], innerColor)
                    );
                }

                // --- パリィ圏内：内リングを明るく強調 ---
                if (timeToHit < parryZone)
                {
                    DirectX::XMFLOAT4 glowColor = { 0.0f, 1.0f, 0.3f, 0.6f };
                    for (int s = 0; s < SEGMENTS; s++)
                    {
                        int next = (s + 1) % SEGMENTS;
                        // 内リングの三角形で塗りつぶし（中心→辺）
                        DirectX::XMFLOAT3 ctr;
                        DirectX::XMStoreFloat3(&ctr, ringCenter);
                        DirectX::XMFLOAT4 centerGlow = { 0.0f, 1.0f, 0.3f, 0.15f };
                        primitiveBatch->DrawTriangle(
                            DirectX::VertexPositionColor(ctr, centerGlow),
                            DirectX::VertexPositionColor(innerVerts[s], glowColor),
                            DirectX::VertexPositionColor(innerVerts[next], glowColor)
                        );
                    }
                }
            }
        }
    }

    primitiveBatch->End();

    // ラスタライザを元に戻す
    context->RSSetState(nullptr);
}

void Game::DrawSingleEnemy(const Enemy& enemy, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
    if (!enemy.isAlive && !enemy.isDying)
        return;

    if (enemy.isExploded)
        return;

    // ワールド行列を計算
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f);
    DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(enemy.rotationY);
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
        enemy.position.x,
        enemy.position.y,
        enemy.position.z
    );
    DirectX::XMMATRIX world = scale * rotation * translation;

    // 適切なモデルを選択
    Model* model = nullptr;
    switch (enemy.type)
    {
    case EnemyType::NORMAL:
        model = m_enemyModel.get();
        break;
    case EnemyType::RUNNER:
        model = m_runnerModel.get();
        break;
    case EnemyType::TANK:
        model = m_tankModel.get();
        break;
    }

    if (!model) return;

    // 深度テストを有効化
    m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    // アニメーション描画
    std::string animName = enemy.currentAnimation;
    if (enemy.headDestroyed)
    {
        model->SetBoneScaleByPrefix("Head", 0.0f);
    }

    model->DrawAnimated(
        m_d3dContext.Get(),
        world,
        viewMatrix,
        projectionMatrix,
        DirectX::XMLoadFloat4(&enemy.color),  // XMFLOAT4 → XMVECTOR
        animName,
        enemy.animationTime
    );

    // 頭のスケールをリセット
    if (enemy.headDestroyed)
    {
        model->SetBoneScaleByPrefix("Head", 1.0f);
    }
}

//  回転対応版：レイとOBB（回転する箱）の交差判定
float Game::CheckRayIntersection(
    DirectX::XMFLOAT3 rayStart,
    DirectX::XMFLOAT3 rayDir,
    DirectX::XMFLOAT3 enemyPos,
    float enemyRotationY,
    EnemyType enemyType)
{
    using namespace DirectX;

    // === 設定を取得（デバッグモード対応）===
    EnemyTypeConfig config;

    if (m_useDebugHitboxes)
    {
        switch (enemyType)
        {
        case EnemyType::NORMAL:
            config = m_normalConfigDebug;
            break;
        case EnemyType::RUNNER:
            config = m_runnerConfigDebug;
            break;
        case EnemyType::TANK:
            config = m_tankConfigDebug;
            break;
        case EnemyType::MIDBOSS:
            config = m_midbossConfigDebug;
            break;
        case EnemyType::BOSS:
            config = m_bossConfigDebug;
            break;
        }
    }
    else
    {
        config = GetEnemyConfig(enemyType);
    }

    float width = config.bodyWidth;
    float height = config.bodyHeight;

    // === レイをローカル座標系に変換 ===
    XMVECTOR vRayOrigin = XMLoadFloat3(&rayStart);
    XMVECTOR vRayDir = XMLoadFloat3(&rayDir);
    XMVECTOR vEnemyPos = XMLoadFloat3(&enemyPos);

    // 平行移動（敵を原点に）
    XMVECTOR vLocalOrigin = XMVectorSubtract(vRayOrigin,vEnemyPos);

    // 逆回転（敵の回転の逆をレイにかける）
    XMMATRIX matInvRot = XMMatrixRotationY(-enemyRotationY);
    vLocalOrigin = XMVector3TransformCoord(vLocalOrigin, matInvRot);
    vRayDir = XMVector3TransformNormal(vRayDir, matInvRot);

    // Float3に戻す
    XMFLOAT3 localOrigin, localDir;
    XMStoreFloat3(&localOrigin, vLocalOrigin);
    XMStoreFloat3(&localDir, vRayDir);

    // === AABB（軸平行ボックス）との判定 ===
    // 箱の最小点・最大点（足元がY=0になるように）
    float minX = -width / 2.0f;
    float minY = 0.0f;
    float minZ = -width / 2.0f;  // ← Z軸も追加

    float maxX = width / 2.0f;
    float maxY = height;
    float maxZ = width / 2.0f;  // ← Z軸も追加

    // スラブ法（Slab Method）による判定
    float tMin = 0.0f;
    float tMax = 10000.0f;

    // === X軸チェック ===
    if (fabs(localDir.x) < 1e-6f)
    {
        if (localOrigin.x < minX || localOrigin.x > maxX)
            return -1.0f;
    }
    else
    {
        float t1 = (minX - localOrigin.x) / localDir.x;
        float t2 = (maxX - localOrigin.x) / localDir.x;
        if (t1 > t2) std::swap(t1, t2);
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // === Y軸チェック ===
    if (fabs(localDir.y) < 1e-6f)
    {
        if (localOrigin.y < minY || localOrigin.y > maxY)
            return -1.0f;
    }
    else
    {
        float t1 = (minY - localOrigin.y) / localDir.y;
        float t2 = (maxY - localOrigin.y) / localDir.y;
        if (t1 > t2) std::swap(t1, t2);
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // === Z軸チェック（これが抜けてた！）===
    if (fabs(localDir.z) < 1e-6f)
    {
        if (localOrigin.z < minZ || localOrigin.z > maxZ)
            return -1.0f;
    }
    else
    {
        float t1 = (minZ - localOrigin.z) / localDir.z;
        float t2 = (maxZ - localOrigin.z) / localDir.z;
        if (t1 > t2) std::swap(t1, t2);
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // 後ろの敵には当たらない
    if (tMin < 0.0f) return -1.0f;

    return tMin;  // 衝突距離
}



void Game::DrawBillboard()
{
    if (!m_showDamageDisplay)
        return;

    // 既存のDrawCubesと同じ描画設定
   // 位置と回転を変数に保存
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
    );

    // 小さな黄色いキューブをダメージ表示位置に描画
    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f) *  // 小さくする
        DirectX::XMMatrixTranslation(m_damageDisplayPos.x,
            m_damageDisplayPos.y,
            m_damageDisplayPos.z);

    m_cube->Draw(world, viewMatrix, projectionMatrix, DirectX::Colors::Yellow);
}

void Game::DrawParticles()
{
    if (m_particleSystem->IsEmpty())
        return;

    auto context = m_d3dContext.Get();

    // === カメラ行列 ===
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
    );

    // === テクスチャ付きエフェクト ===
    m_particleEffect->SetView(viewMatrix);
    m_particleEffect->SetProjection(projectionMatrix);
    m_particleEffect->SetWorld(DirectX::XMMatrixIdentity());
    m_particleEffect->Apply(context);
    context->IASetInputLayout(m_particleInputLayout.Get());

    // === アルファブレンド + 加算ブレンド ===
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_states->AlphaBlend(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    // === ビルボード方向 ===
    DirectX::XMFLOAT4X4 viewF;
    DirectX::XMStoreFloat4x4(&viewF, viewMatrix);
    DirectX::XMFLOAT3 right(viewF._11, viewF._21, viewF._31);
    DirectX::XMFLOAT3 up(viewF._12, viewF._22, viewF._32);

    // === テクスチャ付きビルボード描画 ===
    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColorTexture>>(context);
    batch->Begin();

    for (const auto& particle : m_particleSystem->GetParticles())
    {
        float s = particle.size;
        DirectX::XMFLOAT3 c = particle.position;

        // 4頂点 + UV座標
        DirectX::XMFLOAT3 tl(
            c.x - right.x * s + up.x * s,
            c.y - right.y * s + up.y * s,
            c.z - right.z * s + up.z * s);
        DirectX::XMFLOAT3 tr(
            c.x + right.x * s + up.x * s,
            c.y + right.y * s + up.y * s,
            c.z + right.z * s + up.z * s);
        DirectX::XMFLOAT3 bl(
            c.x - right.x * s - up.x * s,
            c.y - right.y * s - up.y * s,
            c.z - right.z * s - up.z * s);
        DirectX::XMFLOAT3 br(
            c.x + right.x * s - up.x * s,
            c.y + right.y * s - up.y * s,
            c.z + right.z * s - up.z * s);

        DirectX::VertexPositionColorTexture vTL(tl, particle.color, DirectX::XMFLOAT2(0, 0));
        DirectX::VertexPositionColorTexture vTR(tr, particle.color, DirectX::XMFLOAT2(1, 0));
        DirectX::VertexPositionColorTexture vBL(bl, particle.color, DirectX::XMFLOAT2(0, 1));
        DirectX::VertexPositionColorTexture vBR(br, particle.color, DirectX::XMFLOAT2(1, 1));

        batch->DrawTriangle(vTL, vTR, vBL);
        batch->DrawTriangle(vTR, vBR, vBL);
    }

    batch->End();

    // === 元に戻す ===
    context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
}

void Game::UpdateTitle()
{
    // デバッグ：フェード値を表示
    /*char debug[256];
    sprintf_s(debug, "TITLE - Press SPACE - Fade:%.2f", m_fadeAlpha);*/
    //SetWindowTextA(m_window, debug);

    // === TitleScene の更新 ===
    if (m_titleScene)
    {
        // デルタタイムの計算（簡易版）
        float deltaTime = 1.0f / 60.0f;  // 60FPS想定

        m_titleScene->Update(deltaTime);
    }

    // === フェード更新 ===
    UpdateFade();

    // タイトル画面：Spaceキーでゲーム開始
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        ResetGame();

        // ロード画面の初期化
        m_loadingTimer = 0.0f;
        m_loadingPhase = 0;
        m_loadingBarTarget = 0.0f;
        m_loadingBarCurrent = 0.0f;

        m_gameState = GameState::LOADING;
    }

    // Lキーでランキング画面
    if (GetAsyncKeyState('L') & 0x8000)
    {
        m_rankingTimer = 0.0f;
        m_newRecordRank = -1;
        m_gameState = GameState::RANKING;
    }
}


void Game::UpdateLoading()
{
    float dt = 1.0f / 60.0f;
    m_loadingTimer += dt;

    // === フェーズ進行（時間でメッセージとバーの目標値を切り替え） ===
    if (m_loadingTimer < 0.5f)
    {
        m_loadingPhase = 0;       // "INITIALIZING SYSTEMS..."
        m_loadingBarTarget = 0.1f;
    }
    else if (m_loadingTimer < 1.2f)
    {
        m_loadingPhase = 1;       // "LOADING ARSENAL..."
        m_loadingBarTarget = 0.3f;
    }
    else if (m_loadingTimer < 1.8f)
    {
        m_loadingPhase = 2;       // "SPAWNING ENTITIES..."
        m_loadingBarTarget = 0.55f;
    }
    else if (m_loadingTimer < 2.5f)
    {
        m_loadingPhase = 3;       // "CALIBRATING GOTHIC FREQUENCIES..."
        m_loadingBarTarget = 0.78f;
    }
    else if (m_loadingTimer < 3.0f)
    {
        m_loadingPhase = 4;       // "READY"
        m_loadingBarTarget = 1.0f;
    }

    // === バーのスムーズ補間 ===
    float lerpSpeed = 3.0f * dt;
    m_loadingBarCurrent += (m_loadingBarTarget - m_loadingBarCurrent) * lerpSpeed;

    // ぴったり目標に近づいたらスナップ
    if (fabsf(m_loadingBarCurrent - m_loadingBarTarget) < 0.001f)
        m_loadingBarCurrent = m_loadingBarTarget;

    // === READY後、Enterキーで遷移 ===
    if (m_loadingPhase == 4)
    {
        m_loadingBarCurrent = 1.0f;

        static bool enterPressed = false;
        if (GetAsyncKeyState(VK_RETURN) & 0x8000)
        {
            if (!enterPressed)
            {
                m_fadeAlpha = 1.0f;
                m_fadingIn = true;
                m_fadeActive = true;
                m_gameState = GameState::PLAYING;
                enterPressed = true;
            }
        }
        else
        {
            enterPressed = false;
        }
    }
}


void Game::DrawWeapon()
{
    if (!m_weaponModel)
    {
        return;
    }

    //  パリィ中は銃を非表示にする
    if (m_shieldState == ShieldState::Parrying ||
        m_shieldState == ShieldState::Guarding ||
        m_shieldBashTimer > 0.0f)
    {
        return; //  盾が出てる間は表示しない
    }

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        sinf(playerRot.y) * cosf(playerRot.x),
        -sinf(playerRot.x),
        cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );

    DirectX::XMVECTOR right = DirectX::XMVectorSet(
        cosf(playerRot.y),
        0.0f,
        -sinf(playerRot.y),
        0.0f
    );

    DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

    // === FPS風の位置（右下）===
    DirectX::XMVECTOR rightOffset = XMVectorScale(right,
        m_weaponOffsetRight + m_weaponSwayX * 0.1f);
    DirectX::XMVECTOR upOffset = XMVectorScale(up,
        m_weaponOffsetUp + m_weaponSwayY * 0.1f
        + m_weaponBobAmount + m_weaponLandingImpact
        + m_weaponKickUp + m_reloadAnimOffset);
    DirectX::XMVECTOR forwardOffset = XMVectorScale(forward,
        m_weaponOffsetForward - m_weaponKickBack);

    DirectX::XMVECTOR weaponPos = cameraPosition;
    weaponPos = XMVectorAdd(weaponPos, rightOffset);
    weaponPos = XMVectorAdd(weaponPos, upOffset);
    weaponPos = XMVectorAdd(weaponPos, forwardOffset);

    // === スケール ===
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(m_weaponScale, m_weaponScale, m_weaponScale);

    // === 回転（シンプル3軸） ===
    DirectX::XMMATRIX modelRotation = DirectX::XMMatrixRotationRollPitchYaw(
        m_weaponRotX - m_weaponKickRot + m_reloadAnimTilt,
        m_weaponRotY, m_weaponRotZ
    );
    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(weaponPos);

    // 合成
    DirectX::XMMATRIX weaponWorld = scale * modelRotation * cameraRotation * translation;

    m_weaponModel->Draw(
        m_d3dContext.Get(),
        weaponWorld,
        viewMatrix,
        projectionMatrix,
        DirectX::Colors::White
    );
}

// =================================================================
// 盾を左手に描画（DOOM: The Dark Ages スタイル）
// =================================================================
void Game::DrawShield()
{
    if (!m_shieldModel)
        return;

    // // 盾投げ中は別の描画（ワールド空間で飛んでる盾）
    if (m_shieldState == ShieldState::Throwing)
    {
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

        DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
        DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
            playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
            playerPos.y - sinf(playerRot.x),
            playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
            0.0f
        );
        DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

        float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
        DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
        );

        // ワールド空間での盾の位置
        float shieldScale = 0.25f;
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(shieldScale, shieldScale, shieldScale);

        // 回転: 元の向き補正 + 高速スピン
        DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PI);
        DirectX::XMMATRIX standUp = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
        DirectX::XMMATRIX spin = DirectX::XMMatrixRotationY(m_thrownShieldSpin);

        // 飛ぶ方向に向ける
        float yaw = atan2f(m_thrownShieldDir.x, m_thrownShieldDir.z);
        float pitch = -asinf(m_thrownShieldDir.y);
        DirectX::XMMATRIX faceDir = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);

        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
            m_thrownShieldPos.x, m_thrownShieldPos.y, m_thrownShieldPos.z);

        DirectX::XMMATRIX shieldWorld = scale * modelFix * standUp * spin * faceDir * translation;

        m_shieldModel->Draw(
            m_d3dContext.Get(),
            shieldWorld,
            viewMatrix,
            projectionMatrix,
            DirectX::Colors::White
        );
        return;  // 通常描画はスキップ
    }

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
    );

    // カメラの方向ベクトル
    DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        sinf(playerRot.y) * cosf(playerRot.x),
        -sinf(playerRot.x),
        cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR right = DirectX::XMVectorSet(
        cosf(playerRot.y), 0.0f, -sinf(playerRot.y), 0.0f
    );
    DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

    // === 盾の位置アニメーション ===
    // パリィ中: 左下 → 正面中央にスライド
    float guardProgress = 0.0f;  // 0.0=待機位置、1.0=正面ガード位置
    if (m_shieldBashTimer > 0.0f)
    {
        float t = 1.0f - (m_shieldBashTimer / m_shieldBashDuration);
        // 最初の20%: サッと正面に構える（イージングで素早く）
        // 最後の30%: ゆっくり戻す
        // 中間50%: 正面キープ
        if (t < 0.2f)
        {
            // 0→1 に素早く（easeOut）
            float p = t / 0.2f;
            guardProgress = 1.0f - (1.0f - p) * (1.0f - p);  // easeOutQuad
        }
        else if (t < 0.7f)
        {
            // 正面キープ
            guardProgress = 1.0f;
        }
        else
        {
            // 1→0 にゆっくり戻す
            float p = (t - 0.7f) / 0.3f;
            guardProgress = 1.0f - p * p;  // easeInQuad
        }
    }

    // === 待機位置（左下）===
    float restRight = -0.35f;    // 左側
    float restUp = -0.30f;       // 下
    float restForward = 0.45f;   // 前

    // === ガード位置（正面中央）===
    float guardRight = -0.10f;   // ほぼ中央（少し左）
    float guardUp = -0.30f;      // 目線の少し下
    float guardForward = 0.40f;  // 少し前に出して小さく見せる

    // 線形補間（lerp）で待機→ガードをスムーズに遷移
    float currentRight = restRight + (guardRight - restRight) * guardProgress;
    float currentUp = restUp + (guardUp - restUp) * guardProgress;
    float currentForward = restForward + (guardForward - restForward) * guardProgress;

    DirectX::XMVECTOR rightOffset = XMVectorScale(right, currentRight + m_weaponSwayX * 0.05f * (1.0f - guardProgress));
    float bobEffect = (m_weaponBobAmount + m_weaponLandingImpact) * (1.0f - guardProgress);
    DirectX::XMVECTOR upOffset = XMVectorScale(up, currentUp + m_weaponSwayY * 0.05f * (1.0f - guardProgress) + bobEffect);
    DirectX::XMVECTOR forwardOffset = XMVectorScale(forward, currentForward);

    DirectX::XMVECTOR shieldPos = cameraPosition;
    shieldPos = XMVectorAdd(shieldPos, rightOffset);
    shieldPos = XMVectorAdd(shieldPos, upOffset);
    shieldPos = XMVectorAdd(shieldPos, forwardOffset);

    // === スケール ===
    float shieldScale = 0.25f;
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(shieldScale, shieldScale, shieldScale);

    // === 回転 ===
    DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PI);
    DirectX::XMMATRIX standUp = DirectX::XMMatrixIdentity();
    // ガード中は盾をまっすぐ、待機中は少し傾ける
    float tiltAngle = 0.1f * (1.0f - guardProgress);
    DirectX::XMMATRIX tilt = DirectX::XMMatrixRotationX(tiltAngle);

    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(shieldPos);

    // 合成（bashTilt は削除、tilt に統合済み）
    DirectX::XMMATRIX shieldWorld = scale * modelFix * standUp * tilt * cameraRotation * translation;

    m_shieldModel->Draw(
        m_d3dContext.Get(),
        shieldWorld,
        viewMatrix,
        projectionMatrix,
        DirectX::Colors::White
    );
}

//  描画全体
void Game::Render()
{
    Clear();

    switch (m_gameState)
    {
    case GameState::TITLE:
        RenderTitle();
        break;

    case GameState::LOADING:
        RenderLoading();
        break;

    case GameState::PLAYING:
        RenderPlaying();
        break;
    case GameState::GAMEOVER:
        RenderGameOver();
        break;
    case GameState::RANKING:
        RenderRanking();
        break;
    }

    RenderFade();
    m_swapChain->Present(1, 0);
}

// =================================================================
// 【ゲーム中】の描画処理
// =================================================================
void Game::RenderPlaying()
{
    // デバッグログ
   /* char debugRender[256];
    sprintf_s(debugRender, "[RENDER] DOFActive=%d, Intensity=%.2f, OffscreenRTV=%p\n",
        m_gloryKillDOFActive, m_gloryKillDOFIntensity, m_offscreenRTV.Get());
    //OutputDebugStringA(debugRender);*/

    if (m_gloryKillDOFActive && m_offscreenRTV)
    {
        // ============================================================
        // === グローリーキル中：ブラー処理 ===
        // ============================================================

        // オフスクリーンに通常描画
        ID3D11RenderTargetView* offscreenRTV = m_offscreenRTV.Get();
        m_d3dContext->OMSetRenderTargets(1, &offscreenRTV, m_offscreenDepthStencilView.Get());  // // 変更

        // オフスクリーンをクリア
        DirectX::XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_d3dContext->ClearRenderTargetView(m_offscreenRTV.Get(), clearColor);
        m_d3dContext->ClearDepthStencilView(
            m_offscreenDepthStencilView.Get(),  // // 変更
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f,
            0
        );

        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        //  ビューポート設定
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_outputWidth);
        viewport.Height = static_cast<float>(m_outputHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_d3dContext->RSSetViewports(1, &viewport);

        // カメラ設定
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

        DirectX::XMFLOAT3 shakeOffset(0.0f, 0.0f, 0.0f);
        if (m_cameraShakeTimer > 0.0f)
        {
            shakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.z = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
        }

        DirectX::XMVECTOR cameraPosition;
        DirectX::XMVECTOR cameraTarget;

        if (m_gloryKillCameraActive)
        {
            cameraPosition = DirectX::XMLoadFloat3(&m_gloryKillCameraPos);
            cameraTarget = DirectX::XMLoadFloat3(&m_gloryKillCameraTarget);
        }
        else
        {
            cameraPosition = DirectX::XMVectorSet(
               playerPos.x + shakeOffset.x,
               playerPos.y + shakeOffset.y,
               playerPos.z + shakeOffset.z,
               0.0f
               );

            cameraTarget = DirectX::XMVectorSet(
                playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
                playerPos.y - sinf(playerRot.x),
                playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
                0.0f
            );
        }

        DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

        float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
        DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
        );
        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        // マップ描画
        if (m_mapSystem)
        {
            m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }

        DrawNavGridDebug(viewMatrix, projectionMatrix);
        DrawSlicedPieces(viewMatrix, projectionMatrix);

        //  地面の苔を描画
           /* if (m_furRenderer && m_furReady)
            {
                m_furRenderer->DrawGroundMoss(
                    m_d3dContext.Get(),
                    viewMatrix,
                    projectionMatrix,
                    m_accumulatedAnimTime 
                );
            }*/

        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        //// 武器スポーンのテクスチャ描画
        //if (m_weaponSpawnSystem)
        //{
        //    m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        //}
        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        // 3D描画（優先順位順）
        //DrawParticles();
        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        DrawEnemies(viewMatrix, projectionMatrix, true);// trueでターゲット敵をスキップ
        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);



        //DrawBillboard();
        DrawWeapon();
        DrawShield();

        

        //DrawWeaponSpawns();

        DrawGibs(viewMatrix, projectionMatrix);

        // グローリーキル腕・ナイフ描画（FBXモデル版）
        if ((m_gloryKillArmAnimActive || m_debugShowGloryKillArm) && m_gloryKillArmModel)
        {
            DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(cameraTarget, cameraPosition);
            forward = DirectX::XMVector3Normalize(forward);

            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR right = DirectX::XMVector3Cross(worldUp, forward);
            right = DirectX::XMVector3Normalize(right);
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // === アームの位置計算（調整変数使用） ===
            DirectX::XMVECTOR armWorldPos = cameraPosition;
            armWorldPos += forward * m_gloryKillArmOffset.x;  // 前方
            armWorldPos += up * m_gloryKillArmOffset.y;       // 上下
            armWorldPos += right * m_gloryKillArmOffset.z;    // 左右

            float cameraYaw = atan2f(
                DirectX::XMVectorGetX(forward),
                DirectX::XMVectorGetZ(forward)
            );

            // === アームのワールド行列 ===
            DirectX::XMMATRIX armScale = DirectX::XMMatrixScaling(
                m_gloryKillArmScale, m_gloryKillArmScale, m_gloryKillArmScale);

            // ベース回転（モデルを正しい向きに補正）
            DirectX::XMMATRIX baseRotX = DirectX::XMMatrixRotationX(m_gloryKillArmBaseRot.x);
            DirectX::XMMATRIX baseRotY = DirectX::XMMatrixRotationY(m_gloryKillArmBaseRot.y);
            DirectX::XMMATRIX baseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmBaseRot.z);
            DirectX::XMMATRIX baseRot = baseRotX * baseRotY * baseRotZ;

            // アニメーション回転（グローリーキルの動き）
            DirectX::XMMATRIX animRotX = DirectX::XMMatrixRotationX(m_gloryKillArmAnimRot.x);
            DirectX::XMMATRIX animRotY = DirectX::XMMatrixRotationY(m_gloryKillArmAnimRot.y);
            DirectX::XMMATRIX animRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmAnimRot.z);
            DirectX::XMMATRIX animRot = animRotX * animRotY * animRotZ;

            // カメラ追従回転
            DirectX::XMMATRIX cameraRot = DirectX::XMMatrixRotationY(cameraYaw);
            DirectX::XMMATRIX armTrans = DirectX::XMMatrixTranslationFromVector(armWorldPos);

            // 合成：スケール → ベース補正 → アニメーション → カメラ追従 → 移動
            DirectX::XMMATRIX armWorld = armScale * baseRot * animRot * cameraRot * armTrans;

            m_gloryKillArmModel->SetBoneScaleByPrefix("L_", 0.0f);

            m_d3dContext->RSSetState(m_states->CullNone());
            
            m_gloryKillArmModel->DrawWithBoneScale(
                m_d3dContext.Get(),
                armWorld,
                viewMatrix,
                projectionMatrix,
                DirectX::Colors::White
            );


            // ============================================
            // === ナイフ描画 ===
            // ============================================
            if (m_gloryKillKnifeModel)
            {
                // オフセットベクトルを作成（ローカル空間）
                DirectX::XMVECTOR offset = DirectX::XMVectorSet(
                    m_gloryKillKnifeOffset.z,   // X = Left/Right
                    m_gloryKillKnifeOffset.y,   // Y = Up/Down  
                    m_gloryKillKnifeOffset.x,   // Z = Forward
                    0.0f
                );

                // アニメーション回転 + カメラ回転でオフセットを変換
                DirectX::XMMATRIX offsetRotation = animRot * cameraRot;
                DirectX::XMVECTOR rotatedOffset = DirectX::XMVector3TransformNormal(offset, offsetRotation);

                // 腕の位置に変換後のオフセットを加える
                DirectX::XMVECTOR knifeWorldPos = DirectX::XMVectorAdd(armWorldPos, rotatedOffset);
                // ナイフのスケール
                DirectX::XMMATRIX knifeScale = DirectX::XMMatrixScaling(
                    m_gloryKillKnifeScale, m_gloryKillKnifeScale, m_gloryKillKnifeScale);

                // ナイフのベース回転
                DirectX::XMMATRIX knifeBaseRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeBaseRot.x);
                DirectX::XMMATRIX knifeBaseRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeBaseRot.y);
                DirectX::XMMATRIX knifeBaseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeBaseRot.z);
                DirectX::XMMATRIX knifeBaseRot = knifeBaseRotX * knifeBaseRotY * knifeBaseRotZ;

                // ナイフのアニメーション回転
                DirectX::XMMATRIX knifeAnimRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeAnimRot.x);
                DirectX::XMMATRIX knifeAnimRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeAnimRot.y);
                DirectX::XMMATRIX knifeAnimRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeAnimRot.z);
                DirectX::XMMATRIX knifeAnimRot = knifeAnimRotX * knifeAnimRotY * knifeAnimRotZ;

                // ナイフの移動行列
                DirectX::XMMATRIX knifeTrans = DirectX::XMMatrixTranslationFromVector(knifeWorldPos);

                // 合成
                DirectX::XMMATRIX knifeWorld = knifeScale * knifeBaseRot * knifeAnimRot * cameraRot * knifeTrans;

                m_gloryKillKnifeModel->Draw(
                    m_d3dContext.Get(),
                    knifeWorld,
                    viewMatrix,
                    projectionMatrix,
                    DirectX::Colors::White
                );
            }

            // === グローリーキルターゲットの敵を最後に描画（腕の後ろに表示） ===
            if (m_gloryKillTargetEnemy)
            {
                DrawSingleEnemy(*m_gloryKillTargetEnemy, viewMatrix, projectionMatrix);
            }
            
        }
        m_bloodSystem->DrawScreenBlood(m_outputWidth, m_outputHeight);
        DrawHealthPickups(viewMatrix, projectionMatrix);
        m_bloodSystem->DrawBloodDecals(viewMatrix, projectionMatrix, m_player->GetPosition());
       
        m_gpuParticles->Draw(viewMatrix, projectionMatrix, m_player->GetPosition());
        //  流体レンダリング: シーンをコピーしてからDrawFluid
        //m_d3dContext->CopyResource(m_sceneCopyTex.Get(), m_offscreenTexture.Get());
        /*m_gpuParticles->DrawFluid(viewMatrix, projectionMatrix, m_player->GetPosition(),
            nullptr, m_offscreenRTV.Get());*/

        // UI描画（オフスクリーンへ）
        DrawUI();
        DrawScorePopups();
        RenderWaveBanner();
        RenderScoreHUD();

        //  最終画面にブラーを適用して描画
        ID3D11RenderTargetView* finalRTV = m_renderTargetView.Get();
        m_d3dContext->OMSetRenderTargets(1, &finalRTV, nullptr);

        // 最終画面をクリア
        m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

        // ブラーパラメータを設定
        BlurParams params;
        params.texelSize = DirectX::XMFLOAT2(1.0f / m_outputWidth, 1.0f / m_outputHeight);
        params.blurStrength = m_gloryKillDOFIntensity * 5.0f;
        params.focalDepth = m_gloryKillFocalDepth;  // // padding → focalDepthに変更

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        m_d3dContext->Map(m_blurConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &params, sizeof(BlurParams));
        m_d3dContext->Unmap(m_blurConstantBuffer.Get(), 0);

        // シェーダーリソースとサンプラーを設定(深度テクスチャ追加）
        ID3D11ShaderResourceView* srvs[2] = {
        m_offscreenSRV.Get(),      // t0: カラー
        m_offscreenDepthSRV.Get()  // t1: 深度 
        };
        m_d3dContext->PSSetShaderResources(0, 2, srvs);

        ID3D11SamplerState* sampler = m_linearSampler.Get();
        m_d3dContext->PSSetSamplers(0, 1, &sampler);

        ID3D11Buffer* cb = m_blurConstantBuffer.Get();
        m_d3dContext->PSSetConstantBuffers(0, 1, &cb);

        // レンダーステート設定
        m_d3dContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
        m_d3dContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
        m_d3dContext->RSSetState(m_states->CullNone());

        // フルスクリーン四角形を描画（ブラー適用）
        DrawFullscreenQuad();

        // リソースをクリア（深度も含める）
        ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
        m_d3dContext->PSSetShaderResources(0, 2, nullSRVs);
    }
    else
    {
        // ============================================================
        // === 通常時：ブラーなし ===
        // ============================================================

        Clear();

        // カメラ設定
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

        DirectX::XMFLOAT3 shakeOffset(0.0f, 0.0f, 0.0f);
        if (m_cameraShakeTimer > 0.0f)
        {
            shakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.z = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
        }

        DirectX::XMVECTOR cameraPosition;
        DirectX::XMVECTOR cameraTarget;

        if (m_gloryKillCameraActive)
        {
            cameraPosition = DirectX::XMLoadFloat3(&m_gloryKillCameraPos);
            cameraTarget = DirectX::XMLoadFloat3(&m_gloryKillCameraTarget);
        }
        else
        {
            cameraPosition = DirectX::XMVectorSet(
                playerPos.x + shakeOffset.x,
                playerPos.y + shakeOffset.y,
                playerPos.z + shakeOffset.z,
                0.0f
            );

            cameraTarget = DirectX::XMVectorSet(
                playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
                playerPos.y - sinf(playerRot.x),
                playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
                0.0f
            );
        }

        DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

        float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
        DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
        );

        // マップ描画
        if (m_mapSystem)
        {
            m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }

        DrawNavGridDebug(viewMatrix, projectionMatrix);
        DrawSlicedPieces(viewMatrix, projectionMatrix);

        ////  地面の苔を描画
        //    if (m_furRenderer && m_furReady)
        //    {
        //        m_furRenderer->DrawGroundMoss(
        //            m_d3dContext.Get(),
        //            viewMatrix,
        //            projectionMatrix,
        //            m_accumulatedAnimTime  // 経過時間（風の揺れ用）
        //        );
        //    }

        //// 武器スポーンのテクスチャ描画
        //if (m_weaponSpawnSystem)
        //{
        //    m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        //}

        // 3D描画（優先順位順）
        //DrawParticles();
        DrawEnemies(viewMatrix, projectionMatrix);
        //DrawBillboard();
        DrawWeapon();
        DrawShield();



        //  シールドトレイル描画
        //  Effekseer描画
        if (m_effekseerRenderer != nullptr && m_effekseerManager != nullptr)
        {
            Effekseer::Matrix44 efkView;
            Effekseer::Matrix44 efkProj;

            DirectX::XMFLOAT4X4 viewF, projF;
            DirectX::XMStoreFloat4x4(&viewF, viewMatrix);
            DirectX::XMStoreFloat4x4(&projF, projectionMatrix);
            memcpy(&efkView, &viewF, sizeof(float) * 16);
            memcpy(&efkProj, &projF, sizeof(float) * 16);

            m_effekseerRenderer->SetCameraMatrix(efkView);
            m_effekseerRenderer->SetProjectionMatrix(efkProj);

            m_effekseerRenderer->BeginRendering();
            m_effekseerManager->Draw();
            m_effekseerRenderer->EndRendering();
        }

        //DrawWeaponSpawns();

        DrawGibs(viewMatrix, projectionMatrix);

        // グローリーキル腕・ナイフ描画（FBXモデル版）
        if ((m_gloryKillArmAnimActive || m_debugShowGloryKillArm) && m_gloryKillArmModel)
        {
            DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(cameraTarget, cameraPosition);
            forward = DirectX::XMVector3Normalize(forward);

            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR right = DirectX::XMVector3Cross(worldUp, forward);
            right = DirectX::XMVector3Normalize(right);
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // === アームの位置計算（調整変数使用） ===
            DirectX::XMVECTOR armWorldPos = cameraPosition;
            armWorldPos += forward * m_gloryKillArmOffset.x;  // 前方
            armWorldPos += up * m_gloryKillArmOffset.y;       // 上下
            armWorldPos += right * m_gloryKillArmOffset.z;    // 左右

            float cameraYaw = atan2f(
                DirectX::XMVectorGetX(forward),
                DirectX::XMVectorGetZ(forward)
            );

            // === アームのワールド行列 ===
            DirectX::XMMATRIX armScale = DirectX::XMMatrixScaling(
                m_gloryKillArmScale, m_gloryKillArmScale, m_gloryKillArmScale);

            // ベース回転（モデルを正しい向きに補正）
            DirectX::XMMATRIX baseRotX = DirectX::XMMatrixRotationX(m_gloryKillArmBaseRot.x);
            DirectX::XMMATRIX baseRotY = DirectX::XMMatrixRotationY(m_gloryKillArmBaseRot.y);
            DirectX::XMMATRIX baseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmBaseRot.z);
            DirectX::XMMATRIX baseRot = baseRotX * baseRotY * baseRotZ;

            // アニメーション回転（グローリーキルの動き）
            DirectX::XMMATRIX animRotX = DirectX::XMMatrixRotationX(m_gloryKillArmAnimRot.x);
            DirectX::XMMATRIX animRotY = DirectX::XMMatrixRotationY(m_gloryKillArmAnimRot.y);
            DirectX::XMMATRIX animRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmAnimRot.z);
            DirectX::XMMATRIX animRot = animRotX * animRotY * animRotZ;

            // カメラ追従回転
            DirectX::XMMATRIX cameraRot = DirectX::XMMatrixRotationY(cameraYaw);
            DirectX::XMMATRIX armTrans = DirectX::XMMatrixTranslationFromVector(armWorldPos);

            // 合成：スケール → ベース補正 → アニメーション → カメラ追従 → 移動
            DirectX::XMMATRIX armWorld = armScale * baseRot * animRot * cameraRot * armTrans;

            m_gloryKillArmModel->SetBoneScaleByPrefix("L_", 0.0f);

            m_d3dContext->RSSetState(m_states->CullNone());

            m_gloryKillArmModel->DrawWithBoneScale(
                m_d3dContext.Get(),
                armWorld,
                viewMatrix,
                projectionMatrix,
                DirectX::Colors::White
            );

            // ============================================
            // === ナイフ描画 ===
            // ============================================
            if (m_gloryKillKnifeModel)
            {
                // オフセットベクトルを作成（ローカル空間）
                DirectX::XMVECTOR offset = DirectX::XMVectorSet(
                    m_gloryKillKnifeOffset.z,   // X = Left/Right
                    m_gloryKillKnifeOffset.y,   // Y = Up/Down  
                    m_gloryKillKnifeOffset.x,   // Z = Forward
                    0.0f
                );

                // アニメーション回転 + カメラ回転でオフセットを変換
                DirectX::XMMATRIX offsetRotation = animRot * cameraRot;
                DirectX::XMVECTOR rotatedOffset = DirectX::XMVector3TransformNormal(offset, offsetRotation);

                // 腕の位置に変換後のオフセットを加える
                DirectX::XMVECTOR knifeWorldPos = DirectX::XMVectorAdd(armWorldPos, rotatedOffset);

                // ナイフのスケール
                DirectX::XMMATRIX knifeScale = DirectX::XMMatrixScaling(
                    m_gloryKillKnifeScale, m_gloryKillKnifeScale, m_gloryKillKnifeScale);

                // ナイフのベース回転
                DirectX::XMMATRIX knifeBaseRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeBaseRot.x);
                DirectX::XMMATRIX knifeBaseRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeBaseRot.y);
                DirectX::XMMATRIX knifeBaseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeBaseRot.z);
                DirectX::XMMATRIX knifeBaseRot = knifeBaseRotX * knifeBaseRotY * knifeBaseRotZ;

                // ナイフのアニメーション回転
                DirectX::XMMATRIX knifeAnimRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeAnimRot.x);
                DirectX::XMMATRIX knifeAnimRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeAnimRot.y);
                DirectX::XMMATRIX knifeAnimRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeAnimRot.z);
                DirectX::XMMATRIX knifeAnimRot = knifeAnimRotX * knifeAnimRotY * knifeAnimRotZ;

                // ナイフの移動行列
                DirectX::XMMATRIX knifeTrans = DirectX::XMMatrixTranslationFromVector(knifeWorldPos);

                // 合成
                DirectX::XMMATRIX knifeWorld = knifeScale * knifeBaseRot * knifeAnimRot * cameraRot * knifeTrans;

                m_gloryKillKnifeModel->Draw(
                    m_d3dContext.Get(),
                    knifeWorld,
                    viewMatrix,
                    projectionMatrix,
                    DirectX::Colors::White
                );
            }

            if (m_gloryKillTargetEnemy)
            {
                DrawSingleEnemy(*m_gloryKillTargetEnemy, viewMatrix, projectionMatrix);
            }
            
        }
        m_bloodSystem->DrawScreenBlood(m_outputWidth, m_outputHeight);
        DrawHealthPickups(viewMatrix, projectionMatrix);
        m_bloodSystem->DrawBloodDecals(viewMatrix, projectionMatrix, m_player->GetPosition());

        m_gpuParticles->Draw(viewMatrix, projectionMatrix, m_player->GetPosition());        //  流体レンダリング: バックバッファをコピーしてからDrawFluid
       /* m_gpuParticles->DrawFluid(viewMatrix, projectionMatrix, m_player->GetPosition(),
            nullptr, m_renderTargetView.Get());*/

        // UI描画
        DrawUI();
        DrawScorePopups();
        RenderWaveBanner();
        RenderScoreHUD();

    }


    //  瀕死日ネット
    RenderLowHealthVignette();

    RenderClawDamage();

    //  スピードライン
    RenderSpeedLines();

    //  ダッシュオーバーレイ
    //RenderDashOverlay();

    //  弾切れ警告
    RenderReloadWarning();

    // デバッグ描画
    DrawHitboxes();
    DrawBulletTracers();
    DrawDebugUI();
}

// 【UI】すべてのUIを描画する
void Game::DrawUI()
{
    // --- UI描画のための共通設定 ---
    auto context = m_d3dContext.Get();
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f, (float)m_outputWidth, (float)m_outputHeight, 0.0f, 0.1f, 1.0f);

    m_effect->SetProjection(projection);
    m_effect->SetView(DirectX::XMMatrixIdentity());
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // UISystemに全ての描画を委譲
    m_uiSystem->DrawAll(primitiveBatch.get(),
        m_player.get(),
        m_weaponSystem.get(),
        m_waveManager.get());

    // === 壁武器の購入UI ===
    //if (m_nearbyWeaponSpawn != nullptr)
    //{
    //    // すでに持っている武器か？
    //    bool alreadyOwned = false;
    //    if (m_weaponSystem->GetPrimaryWeapon() == m_nearbyWeaponSpawn->weaponType ||
    //        (m_weaponSystem->HasSecondaryWeapon() &&
    //            m_weaponSystem->GetSecondaryWeapon() == m_nearbyWeaponSpawn->weaponType))
    //    {
    //        alreadyOwned = true;
    //    }

    //    m_uiSystem->DrawWeaponPrompt(
    //        primitiveBatch.get(),
    //        m_nearbyWeaponSpawn,
    //        m_player->GetPoints(),
    //        alreadyOwned
    //    );
    //}


    primitiveBatch->End();

    /*{
        char debugStyle[512];
        sprintf_s(debugStyle, "[STYLE_HUD] m_styleRank=%s, m_spriteBatch=%s, m_fontLarge=%s, m_font=%s, m_states=%s\n",
            m_styleRank ? "OK" : "NULL",
            m_spriteBatch ? "OK" : "NULL",
            m_fontLarge ? "OK" : "NULL",
            m_font ? "OK" : "NULL",
            m_states ? "OK" : "NULL");
        //OutputDebugStringA(debugStyle);

        if (m_styleRank)
        {
            sprintf_s(debugStyle, "[STYLE_HUD] Rank=%s, Points=%.1f, Combo=%d\n",
                m_styleRank->GetRankName(),
                m_styleRank->GetStylePoints(),
                m_styleRank->GetComboCount());
            //OutputDebugStringA(debugStyle);
        }

        sprintf_s(debugStyle, "[STYLE_HUD] Screen: %d x %d, RankPos: (%.0f, %.0f)\n",
            m_outputWidth, m_outputHeight,
            (float)m_outputWidth - 120.0f, 30.0f);
        //OutputDebugStringA(debugStyle);
    }*/


    // =============================================
    // スタイルランク HUD表示（テクスチャ版）
    // =============================================
    if (m_styleRank && m_spriteBatch && m_states)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // --- ランクテクスチャ描画 ---
        int rankIndex = static_cast<int>(m_styleRank->GetRank());

        if (m_rankTexturesLoaded && rankIndex >= 0 && rankIndex < 7
            && m_rankTextures[rankIndex]
            && m_styleRank->GetStylePoints() > 0.0f)
        {
            float rankX = m_outputWidth - 160.0f;
            float rankY = 10.0f;

            RECT destRect;
            destRect.left = (LONG)rankX;
            destRect.top = (LONG)rankY;
            destRect.right = (LONG)(rankX + 128.0f);
            destRect.bottom = (LONG)(rankY + 128.0f);

            m_spriteBatch->Draw(
                m_rankTextures[rankIndex].Get(),
                destRect,
                DirectX::Colors::White
            );

            // --- コンボ数（ランク画像の下）---
            int combo = m_styleRank->GetComboCount();
            if (combo > 1 && m_comboTexturesLoaded)
            {
                // コンボ数を文字列に変換（例: 12 → "12"）
                char comboStr[16];
                sprintf_s(comboStr, "%d", combo);
                int numDigits = (int)strlen(comboStr);

                // 各桁の表示サイズ
                float digitW = 40.0f;
                float digitH = 40.0f;
                float startX = rankX;
                float startY = rankY + 130.0f;

                // 数字を1桁ずつテクスチャで描画
                for (int i = 0; i < numDigits; i++)
                {
                    int digit = comboStr[i] - '0';  // 文字→数値変換
                    if (digit >= 0 && digit <= 9 && m_comboDigitTex[digit])
                    {
                        RECT dRect;
                        dRect.left = (LONG)(startX + i * digitW);
                        dRect.top = (LONG)startY;
                        dRect.right = (LONG)(startX + i * digitW + digitW);
                        dRect.bottom = (LONG)(startY + digitH);

                        m_spriteBatch->Draw(
                            m_comboDigitTex[digit].Get(),
                            dRect,
                            DirectX::Colors::White
                        );
                    }
                }

                // "COMBO" ラベル（数字の右に表示）
                if (m_comboLabelTex)
                {
                    float labelX = startX + numDigits * digitW + 4.0f;
                    RECT lRect;
                    lRect.left = (LONG)labelX;
                    lRect.top = (LONG)(startY + 2.0f);
                    lRect.right = (LONG)(labelX + 80.0f);
                    lRect.bottom = (LONG)(startY + digitH - 2.0f);

                    m_spriteBatch->Draw(
                        m_comboLabelTex.Get(),
                        lRect,
                        DirectX::Colors::White
                    );
                }
            }
        }

            // ==============================================
            // HP & シールドバー テクスチャHUD描画
            // ==============================================
            if (m_shieldHudLoaded)
            {
                // --- バーの位置・サイズ（ピクセル座標）---
                float barW = 260.0f;    // バーの幅
                float barH = 6.0f;     // バーの高さ
                float barX = 30.0f;     // 左端
                float hpBarY = (float)m_outputHeight - 50.0f;   // HPバーのY位置
                float shieldBarY = hpBarY + barH + 14.0f;        // シールドバー（HPの下）

                // アイコンサイズ
                float iconSize = 18.0f;

                // ─────────────────────────
                // HPバー
                // ─────────────────────────
                {
                    float hpPercent = (float)m_player->GetHealth() / (float)m_player->GetMaxHealth();
                    hpPercent = max(0.0f, min(1.0f, hpPercent));

                    // ゲージ中身（HP割合でsourceRectを制限）
                    auto* fillTex = (hpPercent < 0.25f && m_hpHudFillCritical)
                        ? m_hpHudFillCritical.Get()
                        : m_hpHudFillGreen.Get();

                    if (fillTex)
                    {
                        // sourceRect: テクスチャの左端から割合分だけ切り取る
                        RECT srcRect = { 0, 0, (LONG)(512 * hpPercent), 16 };
                        // destRect: 画面上の表示位置（割合分の幅）
                        RECT destRect = {
                            (LONG)barX,
                            (LONG)hpBarY,
                            (LONG)(barX + barW * hpPercent),
                            (LONG)(hpBarY + barH)
                        };
                        m_spriteBatch->Draw(fillTex, destRect, &srcRect, DirectX::Colors::White);
                    }

                    // フレーム（常に全幅で表示）
                    if (m_hpHudFrame)
                    {
                        RECT frameRect = {
                            (LONG)barX, (LONG)hpBarY,
                            (LONG)(barX + barW), (LONG)(hpBarY + barH)
                        };
                        m_spriteBatch->Draw(m_hpHudFrame.Get(), frameRect, DirectX::Colors::White);
                    }
                }

                // ─────────────────────────
                // シールドバー
                // ─────────────────────────
                {
                    // 表示用HPを滑らかに追従
                    float shieldPercent = m_shieldDisplayHP / m_shieldMaxHP;
                    shieldPercent = max(0.0f, min(1.0f, shieldPercent));

                    // ① グロウ（ガード/パリィ中のみ）
                    if (m_shieldGlowIntensity > 0.01f && m_shieldHudGlow)
                    {
                        float glowExpand = 6.0f;
                        RECT glowRect = {
                            (LONG)(barX - glowExpand),
                            (LONG)(shieldBarY - glowExpand),
                            (LONG)(barX + barW + glowExpand),
                            (LONG)(shieldBarY + barH + glowExpand)
                        };
                        // 透明度をグロウ強度で制御
                        float a = m_shieldGlowIntensity;
                        DirectX::XMVECTORF32 glowColor = { a, a, a, a };
                        m_spriteBatch->Draw(m_shieldHudGlow.Get(), glowRect, glowColor);
                    }

                    // ゲージ中身（状態で切替）
                    ID3D11ShaderResourceView* shieldFillTex = nullptr;
                    if (m_shieldState == ShieldState::Broken)
                    {
                        shieldFillTex = nullptr;  // 壊れてる → 空
                    }
                    else if (shieldPercent < 0.25f)
                    {
                        shieldFillTex = m_shieldHudFillDanger.Get();  // 危険 → 赤
                    }
                    else if (m_shieldState == ShieldState::Guarding || m_shieldState == ShieldState::Parrying)
                    {
                        shieldFillTex = m_shieldHudFillGuard.Get();   // ガード中 → 水色
                    }
                    else
                    {
                        shieldFillTex = m_shieldHudFillBlue.Get();    // 通常 → 青
                    }

                    if (shieldFillTex && shieldPercent > 0.0f)
                    {
                        RECT srcRect = { 0, 0, (LONG)(512 * shieldPercent), 16 };
                        RECT destRect = {
                            (LONG)barX,
                            (LONG)shieldBarY,
                            (LONG)(barX + barW * shieldPercent),
                            (LONG)(shieldBarY + barH)
                        };
                        m_spriteBatch->Draw(shieldFillTex, destRect, &srcRect, DirectX::Colors::White);
                    }

                    // フレーム
                    if (m_shieldHudFrame)
                    {
                        RECT frameRect = {
                            (LONG)barX, (LONG)shieldBarY,
                            (LONG)(barX + barW), (LONG)(shieldBarY + barH)
                        };
                        m_spriteBatch->Draw(m_shieldHudFrame.Get(), frameRect, DirectX::Colors::White);
                    }

                    // ヒビ割れオーバーレイ（壊れた時のみ）
                    if (m_shieldState == ShieldState::Broken && m_shieldHudCrack)
                    {
                        RECT crackRect = {
                            (LONG)barX, (LONG)shieldBarY,
                            (LONG)(barX + barW), (LONG)(shieldBarY + barH)
                        };
                        // 点滅させる
                        float blink = (sinf(m_shieldBrokenTimer * 10.0f) + 1.0f) * 0.5f;
                        DirectX::XMVECTORF32 crackColor = { 1.0f, 1.0f, 1.0f, 0.5f + blink * 0.5f };
                        m_spriteBatch->Draw(m_shieldHudCrack.Get(), crackRect, crackColor);
                    }

                    // パリィ成功フラッシュ
                    if (m_parryFlashTimer > 0.0f && m_shieldHudParryFlash)
                    {
                        float flashAlpha = m_parryFlashTimer / 0.3f;  // 0.3秒かけてフェードアウト
                        RECT flashRect = {
                            (LONG)(barX - 8),
                            (LONG)(shieldBarY - 6),
                            (LONG)(barX + barW + 8),
                            (LONG)(shieldBarY + barH + 6)
                        };
                        DirectX::XMVECTORF32 flashColor = { flashAlpha, flashAlpha, flashAlpha, flashAlpha };
                        m_spriteBatch->Draw(m_shieldHudParryFlash.Get(), flashRect, flashColor);
                    }

                    // シールドアイコン（バー左横）
                    if (m_shieldHudIcon)
                    {
                        RECT iconRect = {
                            (LONG)(barX + barW + 6),
                            (LONG)(shieldBarY + (barH - iconSize) * 0.5f),
                            (LONG)(barX + barW + 6 + iconSize),
                            (LONG)(shieldBarY + (barH + iconSize) * 0.5f)
                        };
                        m_spriteBatch->Draw(m_shieldHudIcon.Get(), iconRect, DirectX::Colors::White);
                    }
                }

                //  チャージエネルギーゲージ
                if (m_chargeEnergy > 0.0f && m_shieldHudFillGuard
                    && (m_shieldState == ShieldState::Guarding || m_shieldState == ShieldState::Charging))
                {
                    float centerX = m_outputWidth * 0.5f;
                    float centerY = m_outputHeight * 0.5f;
                    float eBarW = 60.0f;
                    float eBarH = 3.0f;
                    float eBarY = centerY + 25.0f;

                    float filledW = eBarW * m_chargeEnergy;
                    float left = centerX - eBarW * 0.5f;
                    float right = centerX + eBarW * 0.5f;

                    // 外枠（白、1px大きく）
                    RECT frameRect = {
                        (LONG)(left - 1), (LONG)(eBarY - 1),
                        (LONG)(right + 1), (LONG)(eBarY + eBarH + 1)
                    };
                    DirectX::XMVECTORF32 frameColor = { 0.8f, 0.8f, 0.8f, 0.7f };
                    m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), frameRect, frameColor);

                    // 背景（暗い）
                    RECT bgRect = {
                        (LONG)left, (LONG)eBarY,
                        (LONG)right, (LONG)(eBarY + eBarH)
                    };
                    DirectX::XMVECTORF32 bgColor = { 0.05f, 0.05f, 0.1f, 0.8f };
                    m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), bgRect, bgColor);

                    // エネルギー（明るいシアン、満タンで白く輝く）
                    RECT fillRect = {
                        (LONG)left, (LONG)eBarY,
                        (LONG)(left + filledW), (LONG)(eBarY + eBarH)
                    };
                    if (m_chargeReady)
                    {
                        float pulse = 0.8f + sinf(m_chargeEnergy * 20.0f) * 0.2f;
                        DirectX::XMVECTORF32 readyColor = { pulse, pulse, 1.0f, 1.0f };
                        m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), fillRect, readyColor);
                    }
                    else
                    {
                        DirectX::XMVECTORF32 fillColor = { 0.3f, 0.7f, 1.0f, 0.9f };
                        m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), fillRect, fillColor);
                    }
                }

                // =============================================
                // 近接チャージゲージ（シールドバー横 + 縦ストック）
                // =============================================
                if (m_whitePixel)
                {
                    float meleeW = 28.0f;
                    float meleeFullH = 44.0f;
                    float meleeX = barX + barW + 12.0f;
                    float meleeY = shieldBarY + barH - meleeFullH;

                    // === 背景（真っ黒） ===
                    RECT meleeBgRect = {
                        (LONG)meleeX, (LONG)meleeY,
                        (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH)
                    };
                    DirectX::XMVECTORF32 bgColor2 = { 0.06f, 0.06f, 0.06f, 0.85f };
                    m_spriteBatch->Draw(m_whitePixel.Get(), meleeBgRect, bgColor2);

                    // === ゲージ中身 ===
                    float gaugePercent = 1.0f;
                    if (m_meleeCharges < m_meleeMaxCharges && m_meleeRechargeTime > 0.0f)
                    {
                        gaugePercent = m_meleeRechargeTimer / m_meleeRechargeTime;
                    }

                    if (m_meleeCharges > 0)
                    {
                        RECT meleeFillRect = {
                            (LONG)(meleeX + 2), (LONG)meleeY,
                            (LONG)(meleeX + meleeW - 2), (LONG)(meleeY + meleeFullH)
                        };
                        float pulse = 0.8f + sinf(m_gameTime * 4.0f) * 0.2f;
                        DirectX::XMVECTORF32 fullColor = { pulse, 0.6f * pulse, 0.1f, 1.0f };
                        m_spriteBatch->Draw(m_whitePixel.Get(), meleeFillRect, fullColor);

                        // リチャージ中なら白っぽい進捗を重ねる
                        if (m_meleeCharges < m_meleeMaxCharges && m_meleeRechargeTime > 0.0f)
                        {
                            float progress = m_meleeRechargeTimer / m_meleeRechargeTime;
                            float fillH = meleeFullH * progress;
                            RECT progRect = {
                                (LONG)(meleeX + 2), (LONG)(meleeY + meleeFullH - fillH),
                                (LONG)(meleeX + meleeW - 2), (LONG)(meleeY + meleeFullH)
                            };
                            DirectX::XMVECTORF32 progColor = { 1.0f, 0.9f, 0.6f, 0.3f };
                            m_spriteBatch->Draw(m_whitePixel.Get(), progRect, progColor);
                        }
                    }
                    else
                    {
                        if (gaugePercent > 0.0f)
                        {
                            float fillH = meleeFullH * gaugePercent;
                            RECT meleeFillRect = {
                                (LONG)(meleeX + 2), (LONG)(meleeY + meleeFullH - fillH),
                                (LONG)(meleeX + meleeW - 2), (LONG)(meleeY + meleeFullH)
                            };
                            DirectX::XMVECTORF32 fillColor2 = { 1.0f, 0.6f, 0.1f, 0.4f };
                            m_spriteBatch->Draw(m_whitePixel.Get(), meleeFillRect, fillColor2);
                        }
                    }

                    // === スキルアイコン ===
                    if (m_meleeIconTexture)
                    {
                        float mIconSize = 32.0f;
                        float iconX = meleeX + (meleeW - mIconSize) * 0.5f;
                        float iconY = meleeY + (meleeFullH - mIconSize) * 0.5f;
                        RECT iconRect = {
                            (LONG)iconX, (LONG)iconY,
                            (LONG)(iconX + mIconSize), (LONG)(iconY + mIconSize)
                        };
                        float iconAlpha = (m_meleeCharges > 0) ? 1.0f : 0.3f;
                        DirectX::XMVECTORF32 iconColor = { iconAlpha, iconAlpha, iconAlpha, iconAlpha };
                        m_spriteBatch->Draw(m_meleeIconTexture.Get(), iconRect, iconColor);
                    }

                    // === 外枠（白ピクセルで薄いグレー枠） ===
                    {
                        DirectX::XMVECTORF32 frameColor = { 0.4f, 0.4f, 0.4f, 0.6f };
                        // 上辺
                        RECT top = { (LONG)meleeX, (LONG)meleeY, (LONG)(meleeX + meleeW), (LONG)(meleeY + 1) };
                        m_spriteBatch->Draw(m_whitePixel.Get(), top, frameColor);
                        // 下辺
                        RECT bot = { (LONG)meleeX, (LONG)(meleeY + meleeFullH - 1), (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH) };
                        m_spriteBatch->Draw(m_whitePixel.Get(), bot, frameColor);
                        // 左辺
                        RECT lft = { (LONG)meleeX, (LONG)meleeY, (LONG)(meleeX + 1), (LONG)(meleeY + meleeFullH) };
                        m_spriteBatch->Draw(m_whitePixel.Get(), lft, frameColor);
                        // 右辺
                        RECT rgt = { (LONG)(meleeX + meleeW - 1), (LONG)meleeY, (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH) };
                        m_spriteBatch->Draw(m_whitePixel.Get(), rgt, frameColor);
                    }

                    // === ストックバー ===
                    {
                        float stockX = meleeX + meleeW + 4.0f;
                        float stockW = 6.0f;
                        float stockH = meleeFullH;
                        float divider = 1.0f;

                        RECT stockBgRect = {
                            (LONG)stockX, (LONG)meleeY,
                            (LONG)(stockX + stockW), (LONG)(meleeY + stockH)
                        };
                        DirectX::XMVECTORF32 bgColor3 = { 0.06f, 0.06f, 0.06f, 0.7f };
                        m_spriteBatch->Draw(m_whitePixel.Get(), stockBgRect, bgColor3);

                        float sectionH = (stockH - divider * (m_meleeMaxCharges - 1)) / (float)m_meleeMaxCharges;

                        for (int i = 0; i < m_meleeMaxCharges; i++)
                        {
                            float sy = meleeY + stockH - (i + 1) * sectionH - i * divider;
                            RECT secRect = {
                                (LONG)(stockX + 1), (LONG)sy,
                                (LONG)(stockX + stockW - 1), (LONG)(sy + sectionH)
                            };

                            if (i < m_meleeCharges)
                            {
                                DirectX::XMVECTORF32 stockColor = { 1.0f, 0.6f, 0.1f, 1.0f };
                                m_spriteBatch->Draw(m_whitePixel.Get(), secRect, stockColor);
                            }
                            else
                            {
                                DirectX::XMVECTORF32 emptyColor = { 0.15f, 0.15f, 0.15f, 0.4f };
                                m_spriteBatch->Draw(m_whitePixel.Get(), emptyColor, emptyColor);
                            }
                        }
                    }
                }

            }

            // =============================================
            // チュートリアル操作説明（常時表示）
            // =============================================
            if (m_showTutorial && m_font)
            {
                float screenW = (float)m_outputWidth;
                float screenH = (float)m_outputHeight;

                float panelW = 320.0f;
                float panelH = 260.0f;
                float panelX = screenW - panelW - 20.0f;
                float panelY = screenH - panelH - 80.0f;

                // 半透明の黒背景
                RECT bgRect = {
                    (LONG)panelX, (LONG)panelY,
                    (LONG)(panelX + panelW), (LONG)(panelY + panelH)
                };
                DirectX::XMVECTORF32 bgColor = { 0.0f, 0.0f, 0.0f, 0.6f };
                m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

                // 枠線（上）
                RECT borderTop = { (LONG)panelX, (LONG)panelY, (LONG)(panelX + panelW), (LONG)(panelY + 2) };
                DirectX::XMVECTORF32 borderColor = { 0.8f, 0.15f, 0.0f, 0.8f };
                m_spriteBatch->Draw(m_whitePixel.Get(), borderTop, borderColor);
                // 枠線（下）
                RECT borderBot = { (LONG)panelX, (LONG)(panelY + panelH - 2), (LONG)(panelX + panelW), (LONG)(panelY + panelH) };
                m_spriteBatch->Draw(m_whitePixel.Get(), borderBot, borderColor);
                // 枠線（左）
                RECT borderLeft = { (LONG)panelX, (LONG)panelY, (LONG)(panelX + 2), (LONG)(panelY + panelH) };
                m_spriteBatch->Draw(m_whitePixel.Get(), borderLeft, borderColor);
                // 枠線（右）
                RECT borderRight = { (LONG)(panelX + panelW - 2), (LONG)panelY, (LONG)(panelX + panelW), (LONG)(panelY + panelH) };
                m_spriteBatch->Draw(m_whitePixel.Get(), borderRight, borderColor);

                // テキスト色
                DirectX::XMVECTORF32 titleColor = { 0.9f, 0.3f, 0.0f, 1.0f };
                DirectX::XMVECTORF32 keyColor = { 1.0f, 0.85f, 0.2f, 1.0f };
                DirectX::XMVECTORF32 descColor = { 0.85f, 0.85f, 0.85f, 1.0f };

                float x = panelX + 15.0f;
                float y = panelY + 12.0f;
                float lineH = 26.0f;

                // タイトル
                m_font->DrawString(m_spriteBatch.get(), L"- CONTROLS -",
                    DirectX::XMFLOAT2(panelX + panelW * 0.5f - 50.0f, y), titleColor);
                y += lineH + 6.0f;

                // 操作一覧
                struct { const wchar_t* key; const wchar_t* desc; } controls[] = {
                    { L"W A S D",       L"Move" },
                    { L"Mouse",         L"Look" },
                    { L"Left Click",    L"Shoot" },
                    { L"Right Click",   L"Shield / Parry" },
                    { L"Side Button",   L"Melee" },
                    { L"E",             L"Shield Throw" },
                    { L"Space",         L"Jump" },
                    { L"F1",            L"Debug" },
                };

                for (const auto& ctrl : controls)
                {
                    m_font->DrawString(m_spriteBatch.get(), ctrl.key,
                        DirectX::XMFLOAT2(x, y), keyColor);
                    m_font->DrawString(m_spriteBatch.get(), ctrl.desc,
                        DirectX::XMFLOAT2(x + 150.0f, y), descColor);
                    y += lineH;
                }
            }

            // =============================================
            // ボスHPバー（画面上部中央）
            // =============================================
            {
                // 生きてるBOSS/MIDBOSSを探す
                const Enemy* bossEnemy = nullptr;
                for (const auto& enemy : m_enemySystem->GetEnemies())
                {
                    if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS)
                        && enemy.isAlive && !enemy.isDying)
                    {
                        // BOSSを優先（両方いる場合）
                        if (!bossEnemy || enemy.type == EnemyType::BOSS)
                            bossEnemy = &enemy;
                    }
                }

                if (bossEnemy && m_whitePixel)
                {
                    float screenW = (float)m_outputWidth;

                    // バーのサイズと位置
                    float barW = 500.0f;
                    float barH = 14.0f;
                    float barX = (screenW - barW) * 0.5f;  // 画面中央
                    float barY = 40.0f;                      // 上端から40px

                    // HP割合
                    float hpPercent = (float)bossEnemy->health / (float)bossEnemy->maxHealth;
                    if (hpPercent < 0.0f) hpPercent = 0.0f;
                    if (hpPercent > 1.0f) hpPercent = 1.0f;

                    // 背景（暗い）
                    RECT bgRect = { (LONG)barX, (LONG)barY, (LONG)(barX + barW), (LONG)(barY + barH) };
                    DirectX::XMVECTORF32 bgColor = { 0.1f, 0.0f, 0.0f, 0.7f };
                    m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

                    //  残像バー（白・ゆっくり減る）
                    float trailW = barW * m_bossHpTrail;
                    RECT trailRect = { (LONG)barX, (LONG)barY, (LONG)(barX + trailW), (LONG)(barY + barH) };
                    DirectX::XMVECTORF32 trailColor = { 1.0f, 1.0f, 1.0f, 0.4f };
                    m_spriteBatch->Draw(m_whitePixel.Get(), trailRect, trailColor);

                    //  表示HPバー（ぬるっと減る）
                    float fillW = barW * m_bossHpDisplay;
                    RECT fillRect = { (LONG)barX, (LONG)barY, (LONG)(barX + fillW), (LONG)(barY + barH) };

                    DirectX::XMVECTORF32 fillColor;
                    if (m_bossHpDisplay > 0.5f)
                        fillColor = { 0.8f, 0.1f, 0.0f, 0.9f };
                    else if (m_bossHpDisplay > 0.25f)
                        fillColor = { 0.9f, 0.4f, 0.0f, 0.9f };
                    else
                        fillColor = { 1.0f, 0.8f, 0.0f, 0.9f };

                    m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, fillColor);

                    // 枠線（4辺）
                    DirectX::XMVECTORF32 frameColor = { 0.6f, 0.6f, 0.6f, 0.8f };
                    RECT top = { (LONG)barX, (LONG)barY, (LONG)(barX + barW), (LONG)(barY + 1) };
                    RECT bot = { (LONG)barX, (LONG)(barY + barH - 1), (LONG)(barX + barW), (LONG)(barY + barH) };
                    RECT lft = { (LONG)barX, (LONG)barY, (LONG)(barX + 1), (LONG)(barY + barH) };
                    RECT rgt = { (LONG)(barX + barW - 1), (LONG)barY, (LONG)(barX + barW), (LONG)(barY + barH) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), top, frameColor);
                    m_spriteBatch->Draw(m_whitePixel.Get(), bot, frameColor);
                    m_spriteBatch->Draw(m_whitePixel.Get(), lft, frameColor);
                    m_spriteBatch->Draw(m_whitePixel.Get(), rgt, frameColor);

                    // ボス名テキスト
                    if (m_font)
                    {
                        const wchar_t* bossName = (bossEnemy->type == EnemyType::BOSS) ? L"BOSS" : L"MIDBOSS";
                        m_font->DrawString(m_spriteBatch.get(), bossName,
                            DirectX::XMFLOAT2(barX, barY - 22.0f),
                            DirectX::XMVECTORF32{ 0.9f, 0.2f, 0.0f, 1.0f });
                    }
                }
            }


        m_spriteBatch->End();
    }



}


void Game::UpdatePlaying()
{

    \
    // F1キーでデバッグウィンドウ切り替え + マウスキャプチャ連動
    static bool f1Pressed = false;
    if (GetAsyncKeyState(VK_F1) & 0x8000)
    {
        if (!f1Pressed)
        {
            m_showDebugWindow = !m_showDebugWindow;

            if (m_showDebugWindow)
            {
                // ImGui表示 → カーソル出す（UI操作用）
                m_player->SetMouseCaptured(false);
                ShowCursor(TRUE);
            }
            else
            {
                // ImGui閉じる → カーソル消す（FPS視点に戻る）
                m_player->SetMouseCaptured(true);
                ShowCursor(FALSE);
            }

            f1Pressed = true;
        }
    }
    else
    {
        f1Pressed = false;
    }

    // F3キーで切断テスト
    if (GetAsyncKeyState(VK_F3) & 1)
    {
        ExecuteSliceTest();
    }

    float deltaTime = m_deltaTime * m_timeScale;
    //  タイプ別にImGui速度で進める
    for (int i = 0; i < 5; i++)
    {
        m_typeWalkTimer[i] += deltaTime * m_enemySystem->m_animSpeed_Walk[i];
        m_typeAttackTimer[i] += deltaTime * m_enemySystem->m_animSpeed_Attack[i];
    }

    m_gameTime += deltaTime;

    //  スタイルランクシ更新
    m_styleRank->Update(deltaTime);

    //  最高コンボ更新
    int currentCombo = m_styleRank->GetComboCount();
    if (currentCombo > m_statMaxCombo)
        m_statMaxCombo = currentCombo;

    //  最高スタイルランク更新
    int currentStyleRank = static_cast<int>(m_styleRank->GetRank());
    if (currentStyleRank > m_statMaxStyleRank)
        m_statMaxStyleRank = currentStyleRank;

    //  生存時間加算
    m_statSurvivalTime += deltaTime;

    // === デバッグ：ウィンドウタイトルにランク表示 ===
    {
        char titleBuf[256];
        sprintf_s(titleBuf, "Gothic Swarm | Rank: %s | Points: %.0f | Combo: %d",
            m_styleRank->GetRankName(),
            m_styleRank->GetStylePoints(), 
            m_styleRank->GetComboCount());
        //SetWindowTextA(m_window, titleBuf);
    }

    UpdateGloryKillAnimation(deltaTime);

    // === 敵に物理ボディを追加（まだ追加されてない敵のみ）===
    
    if (!m_enemiesInitialized && m_enemySystem->GetEnemies().size() > 0)
    {
        //OutputDebugStringA("Initializing enemy physics bodies...\n");
        for (auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.isAlive)
            {
                AddEnemyPhysicsBody(enemy);
            }
        }
        m_enemiesInitialized = true;
        //OutputDebugStringA("Enemy physics bodies initialized!\n");
    }

    // === 弾の軌跡を更新 ===
    for (auto it = m_bulletTraces.begin(); it != m_bulletTraces.end();)
    {
        it->lifetime -= deltaTime;
        if (it->lifetime <= 0.0f)
        {
            it = m_bulletTraces.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //  --- ヒットストップ処理   ---
    if (m_hitStopTimer > 0.0f)
    {
        m_hitStopTimer -= m_deltaTime;  // timeScaleに影響されない実時間

        if (m_hitStopTimer <= 0.0f)
        {
            m_hitStopTimer = 0.0f;

            if (m_slowMoTimer <= 0.0f)
                m_timeScale = 1.0f;
        }
    }

    // 無敵時間の更新
    if (m_gloryKillInvincibleTimer > 0.0f)
    {
        m_gloryKillInvincibleTimer -= deltaTime;
        if (m_gloryKillInvincibleTimer < 0.0f)
            m_gloryKillInvincibleTimer = 0.0f;
    }

    //  グローリーキルカメラ更新
    if (m_gloryKillCameraActive)
    {
        m_gloryKillCameraLerpTime += m_deltaTime * 3.0f;
        if (m_gloryKillCameraLerpTime > 1.0f)
            m_gloryKillCameraLerpTime = 1.0f;

        //  被写界深度の強度を徐々に上げる
        if (m_gloryKillDOFActive)
        {
            m_gloryKillDOFIntensity += m_deltaTime * 4.0f;
            if (m_gloryKillDOFIntensity > 1.0f)
                m_gloryKillDOFIntensity = 1.0f;
        }
    }
    else
    {
        // カメラが無効化されたらDOFも無効化
        if (m_gloryKillDOFActive)
        {
            m_gloryKillDOFActive = false;
            m_gloryKillDOFIntensity = 0.0f;

            /*char debugDOF[128];
            sprintf_s(debugDOF, "[DOF] Deactivated! Camera ended.\n");*/
            //OutputDebugStringA(debugDOF);
        }
    }

    // グローリーキル腕アニメーション更新（横からこめかみへ）
    if (m_gloryKillArmAnimActive)
    {
        // アニメーション時間を進める（0.0～1.0）
        m_gloryKillArmAnimTime += m_deltaTime * 1.5f;  // 0.4秒で完了

        if (m_gloryKillArmAnimTime >= 1.0f)
        {
            m_gloryKillArmAnimTime = 1.0f;
            m_gloryKillArmAnimActive = false;
        }

        // イージング関数（滑らかな動き）
        float t = m_gloryKillArmAnimTime;
        float easeInOut = t < 0.5f
            ? 2.0f * t * t
            : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        // 腕の位置を計算（横から刺す）
        // 0.0: 右側（画面外） → 0.4: 中央（敵のこめかみ） → 1.0: 元の位置
         // 腕の位置を計算（横から刺す）
        if (t < 0.5f)
        {
            // 右から中央へ（刺す）0.0～0.5
            float t1 = t / 0.5f;  // 0.0～1.0に正規化
            m_gloryKillArmPos.x = 1.5f - easeInOut * 2.0f;  // 右から左へ
            m_gloryKillArmPos.y = 0.0f;  // 中央の高さ
            m_gloryKillArmPos.z = 0.5f;  // カメラの前
        }
        else
        {
            // 中央から右へ（戻る）0.5～1.0
            float t2 = (t - 0.5f) / 0.5f;  // 0.0～1.0に正規化
            m_gloryKillArmPos.x = -0.5f + t2 * 2.0f;  // 左から右へ
            m_gloryKillArmPos.y = 0.0f;
            m_gloryKillArmPos.z = 0.5f;
        }

        // 腕の回転（横向き）
        m_gloryKillArmRot.x = 0.0f;  // X軸回転なし
        m_gloryKillArmRot.y = DirectX::XM_PIDIV2;  // 90度（横向き）
        m_gloryKillArmRot.z = 0.0f;  // Z軸回転なし
    }

    // 画面フラッシュタイマーの更新
    if (m_gloryKillFlashTimer > 0.0f)
    {
        m_gloryKillFlashTimer -= m_deltaTime;
        if (m_gloryKillFlashTimer < 0.0f)
        {
            m_gloryKillFlashTimer = 0.0f;
            m_gloryKillFlashAlpha = 0.0f;
        }
        else
        {
            // 正規化された時間（0.0～1.0）
            float normalizedTime = m_gloryKillFlashTimer / 0.3f;  // 0.15f → 0.3f（長めに）

            // イージング関数：quadratic ease-out（二次関数）
            // 最初は速く減少、最後はゆっくり減少
            float easeOut = 1.0f - (1.0f - normalizedTime) * (1.0f - normalizedTime);

            m_gloryKillFlashAlpha = easeOut;
        }
    }

    // グローリーキル可能な敵を探す（常に）
    m_gloryKillTargetEnemy = nullptr;
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    float closestDist = m_gloryKillRange;

    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying)
            continue;

        if (!enemy.isStaggered)
            continue;

        // 距離を計算
        float dx = enemy.position.x - playerPos.x;
        float dz = enemy.position.z - playerPos.z;
        float dist = sqrtf(dx * dx + dz * dz);

        if (dist < closestDist)
        {
            closestDist = dist;
            m_gloryKillTargetEnemy = &enemy;
        }
    }

    // Fキーが押されたか検出
    static bool fKeyPressed = false;
    bool fKeyDown = (GetAsyncKeyState('F') & 0x8000) != 0;

    if (fKeyDown && !fKeyPressed)
    {
        // Fキーが押された瞬間
        if (m_gloryKillTargetEnemy != nullptr)
        {
            // === グローリーキル実行！ ===

            m_gloryKillActive = true;
            // デバッグログ追加
            /*char debugGK[256];
            sprintf_s(debugGK, "[GLORY KILL] Executed! Target ID=%d, DOF will activate next frame\n", m_gloryKillTargetID);
            //OutputDebugStringA(debugGK);*/

            // === カメラをターゲット敵に向ける ===
            m_gloryKillCameraActive = false;
            m_gloryKillCameraLerpTime = 0.0f;

            // カメラ位置：プレイヤーとターゲット敵の間、やや上から
            DirectX::XMFLOAT3 playerPosition = m_player->GetPosition();
            DirectX::XMFLOAT3 enemyPos = m_gloryKillTargetEnemy->position;

            // === 敵をプレイヤーの目の前に移動 ===
            DirectX::XMFLOAT3 playerRot = m_player->GetRotation();
            float forwardX = sinf(playerRot.y);
            float forwardZ = cosf(playerRot.y);

            // プレイヤーの前方 0.8m の位置に敵を配置
            float targetDistance = 0.8f;  // この値を小さくするとより近くなる
            m_gloryKillTargetEnemy->position.x = playerPosition.x + forwardX * targetDistance;
            m_gloryKillTargetEnemy->position.z = playerPosition.z + forwardZ * targetDistance;
            m_gloryKillTargetEnemy->position.y = playerPosition.y - 1.7f;

            // プレイヤーから敵への方向ベクトル
            float dx = enemyPos.x - playerPosition.x;
            float dz = enemyPos.z - playerPosition.z;
            float distance = sqrtf(dx * dx + dz * dz);

            // 正規化
            if (distance > 0.0f)
            {
                dx /= distance;
                dz /= distance;
            }

            // カメラ位置：プレイヤーの少し後ろ、少し上
            m_gloryKillCameraPos.x = playerPosition.x - dx;
            m_gloryKillCameraPos.y = playerPosition.y;  // 少し上
            m_gloryKillCameraPos.z = playerPosition.z - dz;

            // カメラターゲット：敵の中心（胸の高さ）
            m_gloryKillCameraTarget = enemyPos;
            m_gloryKillCameraTarget.y += 1.0f;

            // 被写界深度を有効化
            m_gloryKillDOFActive = true;
            m_gloryKillDOFIntensity = 0.0f;  // 徐々に強くする
            // デバッグログ追加
            /*char debugDOF[256];
            sprintf_s(debugDOF, "[GLORY KILL] DOF Activated! Active=%d, Intensity=%.2f, FocalDepth=%.2f\n",
                m_gloryKillDOFActive, m_gloryKillDOFIntensity, m_gloryKillFocalDepth);
            //OutputDebugStringA(debugDOF);*/


            //  終点距離を計算(プレイヤーから敵までの距離)
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            DirectX::XMVECTOR playerVec = DirectX::XMLoadFloat3(&playerPos);
            DirectX::XMVECTOR enemyVec = DirectX::XMLoadFloat3(&m_gloryKillTargetEnemy->position);
            DirectX::XMVECTOR distVec = DirectX::XMVectorSubtract(enemyVec, playerVec);
            float enemyDistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distVec));
            m_gloryKillFocalDepth = enemyDistance;

            // デバッグログ（焦点距離も表示）
            /*char debugFocal[256];
            sprintf_s(debugFocal, "[DOF] Focal depth set to: %.2f meters\n", m_gloryKillFocalDepth);
            //OutputDebugStringA(debugFocal);*/

            /*char cdebugDOF[256];
            sprintf_s(cdebugDOF, "[GLORY KILL] DOF Activated! Active=%d, Intensity=%.2f, Offscreen=%p\n",
                m_gloryKillDOFActive, m_gloryKillDOFIntensity, m_offscreenRTV.Get());
            //OutputDebugStringA(cdebugDOF);*/

            // 敵を即座に倒す
            m_gloryKillTargetEnemy->isDying = true;
            m_gloryKillTargetEnemy->animationTime = 0.0f;
            m_gloryKillTargetEnemy->currentAnimation = "Death";
            m_gloryKillTargetEnemy->corpseTimer = 3.0f;

            //  追加: MIDBOSSのビームエフェクトを停止
            if ((m_gloryKillTargetEnemy->type == EnemyType::MIDBOSS ||
                m_gloryKillTargetEnemy->type == EnemyType::BOSS) &&
                m_beamHandle >= 0 && m_effekseerManager != nullptr)
            {
                m_effekseerManager->StopEffect(m_beamHandle);
                m_beamHandle = -1;
            }

            // スクリーンブラッド
            m_bloodSystem->OnGloryKill(m_gloryKillTargetEnemy->position);
            m_gpuParticles->EmitSplash(m_gloryKillTargetEnemy->position, XMFLOAT3(0.0f, 1.0f, 0.0f), 50, 10.0f);
            m_gpuParticles->EmitMist(m_gloryKillTargetEnemy->position, 150, 3.0f);
            m_gpuParticles->Emit(m_gloryKillTargetEnemy->position, 100, 8.0f);

            // HP回復（最大HPを超えないように）
            int currentHP = m_player->GetHealth();
            int maxHP = m_player->GetMaxHealth();
            int healAmount = 30;
            int newHP = min(currentHP + healAmount, maxHP);
            m_player->SetHealth(newHP);

            //  wavemanagerに加算
            int waveBonus = m_waveManager->OnEnemyKilled();
            if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);

            //  ポイント加算
            int totalPoints = 200 + waveBonus;  //  グローリーキル = 200points
            m_player->AddPoints(totalPoints);
            SpawnScorePopup(totalPoints, ScorePopupType::GLORY_KILL);

            // スコア換算
            m_score += 100;

            // 弾補充（マガジン満タン + 予備弾追加）
            /*m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetMaxAmmo());
            m_weaponSystem->SetReserveAmmo(m_weaponSystem->GetReserveAmmo() + 30);*/

            // 4. デバッグログ
            /*char buffer[256];
            sprintf_s(buffer, "[GLORY KILL] Enemy ID:%d killed! HP: %d→%d, Score: +100\n",
                m_gloryKillTargetEnemy->id, currentHP, newHP);
            //OutputDebugStringA(buffer);*/


            // カメラシェイク（強め）
            m_cameraShake = 1.5f;      // 0.3f → 0.5f（強く）
            m_cameraShakeTimer = 0.5f;  // 0.2f → 0.3f（長く）

            // 無敵時間（1秒）
            m_gloryKillInvincibleTimer = 1.0f;

            // スローモーション（0.5秒間、0.3倍速）
            /*m_timeScale = 0.1f;*/
            
            // 血しぶきパーティクル（大量）
            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
            //m_particleSystem->CreateBloodEffect(m_gloryKillTargetEnemy->position, upDir, 800);

            // グローリーキル腕アニメーション開始（横からこめかみへ）
            m_gloryKillArmAnimActive = true;
            m_gloryKillArmAnimTime = 0.0f;
            m_gloryKillArmPos = DirectX::XMFLOAT3(2.0f, 0.3f, 0.3f);  // 初期位置（右側）
            m_gloryKillArmRot = DirectX::XMFLOAT3(0.0f, DirectX::XM_PIDIV2, -0.2f);  // 初期回転（横向き）

            // カメラタイマー（0.4秒）
            m_slowMoTimer = 0.4f;
        }
    }

    fKeyPressed = fKeyDown;


    //  --- スローモーション更新  ---
    if (m_slowMoTimer > 0.0f)
    {
        m_slowMoTimer -= m_deltaTime;
        if (m_slowMoTimer <= 0.0f)
        {
            m_timeScale = 1.0f;
            m_gloryKillCameraActive = false;
        }
    }

    //  --- カメラシェイク減衰   ---
    if (m_cameraShakeTimer > 0.0f)
    {
        m_cameraShakeTimer -= m_deltaTime;
        m_cameraShake *= 0.9f;
    }

    // === ダッシュ演出の更新 ===
    {
        // WASDのどれかが押されてるかチェック（移動中のみダッシュ）
        bool isMoving = (GetAsyncKeyState('W') & 0x8000) ||
            (GetAsyncKeyState('S') & 0x8000) ||
            (GetAsyncKeyState('A') & 0x8000) ||
            (GetAsyncKeyState('D') & 0x8000);

        bool shiftHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        m_isSprinting = shiftHeld && isMoving;

        if (m_isSprinting)
        {
            // FOVを広げる（既存のシステムを利用）
            m_targetFOV = 85.0f;

            // スピードラインを少し出す（既存のシステム）
            m_speedLineAlpha = 0.5f;

            // テクスチャオーバーレイをフェードイン
            m_dashOverlayAlpha += deltaTime * 4.0f;
            if (m_dashOverlayAlpha > 0.35f) m_dashOverlayAlpha = 0.35f;
        }
        else
        {
            // ダッシュしてない時はFOVを戻す
            // ※ 他のシステム（盾チャージ等）でFOVを変えてなければ
            if (m_targetFOV == 85.0f)
                m_targetFOV = 70.0f;

            // オーバーレイをフェードアウト
            m_dashOverlayAlpha -= deltaTime * 6.0f;
            if (m_dashOverlayAlpha < 0.0f) m_dashOverlayAlpha = 0.0f;
        }
    }

    if (m_currentFOV < m_targetFOV)
        m_currentFOV = min(m_targetFOV, m_currentFOV + deltaTime * 360.0f);
    else if (m_currentFOV > m_targetFOV)
        m_currentFOV = max(m_targetFOV, m_currentFOV - deltaTime * 240.0f);

    if (m_shieldState != ShieldState::Charging && !m_isSprinting)
        m_speedLineAlpha = max(0.0f, m_speedLineAlpha - deltaTime * 5.0f);

    float deltatime = (1.0f / 60.0f) * m_timeScale;


    m_frameCount++;

    m_fpsTimer += deltatime;

    if (m_fpsTimer >= 1.0f)
    {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;
        //  Output ウィンドウに表示
        /*char fpsDebug[128];
        sprintf_s(fpsDebug, "=== FPS: %.1f ===\n", m_currentFPS);
        //OutputDebugStringA(fpsDebug);*/
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }

    if (m_fpsTimer >= 1.0f)
    {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;

        // ウィンドウタイトル更新（1秒に1回だけ）
        char debug[256];
        sprintf_s(debug, "FPS:%.0f | Wave:%d | Points:%d | HP:%d",
            m_currentFPS, m_waveManager->GetCurrentWave(),
            m_player->GetPoints(), m_player->GetHealth());
        SetWindowTextA(m_window, debug);

        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }

 
    if (!m_gloryKillCameraActive && m_shieldState != ShieldState::Charging)   //  === プレイヤー移動（チャージ中はスキップ）
 {
    //  --- 移動前の位置を保存   ---
    DirectX::XMFLOAT3 oldPosition = m_player->GetPosition();
    //  プレイヤーの移動・回転
    m_player->Update(m_window);
    //移動後の位置を取得
    DirectX::XMFLOAT3 newPosition = m_player->GetPosition();
    //  壁判定用の半径
    const float playerRadius = 0.5f;

    //  X方向の移動をチェック
    DirectX::XMFLOAT3 testPositionX = oldPosition;
    testPositionX.x = newPosition.x;

    //  --- メッシュコライダーとの壁判定  X ---
    if (CheckMeshCollision(testPositionX, playerRadius))
    {
        newPosition.x = oldPosition.x;
    }

    //  Z
    DirectX::XMFLOAT3 testPositionZ = oldPosition;
    testPositionZ.z = newPosition.z;

    if (CheckMeshCollision(testPositionZ, playerRadius))
    {
        newPosition.z = oldPosition.z;
    }

    //  床の高さに合わせてY座標を更新
    {
        float floorY = GetMeshFloorHeight(newPosition.x, newPosition.z, newPosition.y - 1.8f);
        newPosition.y = floorY + 1.8f;  // 1.8 = 目の高さ
    }

    // === 敵との押し戻し ===
    {
        float moveDx = newPosition.x - oldPosition.x;
        float moveDz = newPosition.z - oldPosition.z;
        bool isMoving = (moveDx * moveDx + moveDz * moveDz) > 0.0001f;

        if (isMoving)
        {
            const float pushRadius = 0.8f;  // プレイヤー+敵の接触距離
            const float pushStrength = 0.15f; // 押し戻しの強さ（大きいほど硬い壁に近い）

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;

                float ex = enemy.position.x - newPosition.x;
                float ez = enemy.position.z - newPosition.z;
                float dist = sqrtf(ex * ex + ez * ez);

                // 敵の体の大きさを考慮
                float enemyRadius = GetEnemyConfig(enemy.type).bodyWidth * 0.5f;
                float minDist = playerRadius + enemyRadius;

                if (dist < minDist && dist > 0.01f)
                {
                    // 押し戻し方向（敵→プレイヤー）
                    float nx = -ex / dist;
                    float nz = -ez / dist;

                    // めり込み量
                    float overlap = minDist - dist;

                    // プレイヤーを押し戻す（100%ブロックではなく、一部すり抜け可能）
                    newPosition.x += nx * overlap * pushStrength;
                    newPosition.z += nz * overlap * pushStrength;
                }
            }
        }
    }

    m_player->SetPosition(newPosition);

    // === 武器ボブ（カメラは動かさない） ===
    {
        float dx = newPosition.x - oldPosition.x;
        float dz = newPosition.z - oldPosition.z;
        float moveDist = sqrtf(dx * dx + dz * dz);

        if (moveDist > 0.001f)
        {
            m_isPlayerMoving = true;
            m_weaponBobTimer += deltaTime * m_weaponBobSpeed;

            float currentSin = sinf(m_weaponBobTimer);
            m_weaponBobAmount = currentSin * m_weaponBobStrength;

            // 着地検知：sin正→負 = 足が着いた瞬間
            if (m_prevWeaponBobSin > 0.0f && currentSin <= 0.0f)
            {
                m_weaponLandingImpact = -0.015f;  // 武器がガクッと沈む
            }
            m_prevWeaponBobSin = currentSin;
        }
        else
        {
            m_isPlayerMoving = false;
            m_weaponBobAmount *= 0.85f;
        }

        m_weaponLandingImpact *= 0.82f;
    }
}


    
    //  === 近接攻撃の入力処理   ===
    static bool lastFKeyState = false;  //  前フレームのFキー状態
    bool isMeleeKeyDown = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;

    //  Fキーが押された瞬間(前フレームは話されていて、今フレームは押されている)
    if (isMeleeKeyDown && !lastFKeyState)
    {
        //  クールダウン中でなければ近接攻撃を実行
        if (m_player->CanMeleeAttack() && m_meleeCharges > 0)
        {
            PerformMeleeAttack();
            m_player->StartMeleeAttackCooldown();
            m_meleeCharges--;  // チャージ消費

            //OutputDebugStringA("[MELEE] Melee attack executed!\n");
        }
        else
        {
            // クールダウン中のログ
            /*char buffer[64];
            sprintf_s(buffer, "[MELEE] Cooldown: %.2fs\n",
                m_player->GetMeleeAttackCooldown());*/
            //OutputDebugStringA(buffer);
        }
    }

    //  現在の状態を保存(次フレーム用)
    lastFKeyState = isMeleeKeyDown;


    //  === 壁武器購入システム
    m_nearbyWeaponSpawn = m_weaponSpawnSystem->CheckPlayerNearWeapon(
        m_player->GetPosition()
    );

    //  --- Eキーで盾投げ / 呼び戻し ---
    static bool eKeyPressed = false;
    if (GetAsyncKeyState('E') & 0x8000)
    {
        if (!eKeyPressed)
        {
            if (m_shieldState == ShieldState::Throwing)
            {
                // 投げてる最中にもう一度E → 呼び戻し
                m_thrownShieldReturning = true;
                //OutputDebugStringA("[SHIELD] Recall shield!\n");
            }
            else if (m_shieldState != ShieldState::Broken
                && m_shieldState != ShieldState::Charging)
            {
                // 盾投げ発動！
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

                m_thrownShieldDir = {
                    sinf(playerRot.y) * cosf(playerRot.x),
                    -sinf(playerRot.x),
                    cosf(playerRot.y) * cosf(playerRot.x)
                };

                float yaw = playerRot.y;
                float rightX = cosf(yaw);   // カメラの右方向X
                float rightZ = -sinf(yaw);  // カメラの右方向Z

                m_thrownShieldPos = {
                   playerPos.x + m_thrownShieldDir.x * 0.5f + rightX * 0.3f,
                   playerPos.y + m_thrownShieldDir.y * 0.5f - 0.3f,
                   playerPos.z + m_thrownShieldDir.z * 0.5f + rightZ * 0.3f
                };

                m_thrownShieldDist = 0.0f;
                m_thrownShieldReturning = false;
                m_thrownShieldSpin = 0.0f;
                m_thrownShieldHitEnemies.clear();
                m_thrownShieldReturnHitEnemies.clear();
                m_shieldState = ShieldState::Throwing;

            
                //  Effekseerトレイル再生（1回だけ再生してハンドルを保持）
                if (m_effectShieldTrail != nullptr)
                {
                    m_shieldTrailHandle = m_effekseerManager->Play(
                        m_effectShieldTrail,
                        m_thrownShieldPos.x,
                        m_thrownShieldPos.y,
                        m_thrownShieldPos.z
                    );
                }

                //OutputDebugStringA("[SHIELD] Shield thrown!\n");
            }
            eKeyPressed = true;
        }
    }
    else
    {
        eKeyPressed = false;
    }



    // --- 1回押した時だけ反応するキー入力の管理 ---
    static std::map<int, bool> keyWasPressed;
    auto IsFirstKeyPress = [&](int vk_code) {
        bool isPressed = GetAsyncKeyState(vk_code) & 0x8000;
        if (isPressed && !keyWasPressed[vk_code]) {
            keyWasPressed[vk_code] = true;
            return true;
        }
        if (!isPressed) {
            keyWasPressed[vk_code] = false;
        }
        return false;
        };

    if (IsFirstKeyPress('1') && !m_weaponSystem->IsReloading())
    {
        // プレイヤーがピストルを所持しているか確認し、持っていればそのスロットに切り替える
        if (m_weaponSystem->GetPrimaryWeapon() == WeaponType::PISTOL)
        {
            m_weaponSystem->SetCurrentWeaponSlot(0);
            m_weaponSystem->SwitchWeapon(WeaponType::PISTOL);
        }
        else if (m_weaponSystem->HasSecondaryWeapon() &&
                 m_weaponSystem->GetSecondaryWeapon() == WeaponType::PISTOL)
        {
            m_weaponSystem->SetCurrentWeaponSlot(1);
            m_weaponSystem->SwitchWeapon(WeaponType::PISTOL);
        }
        // 注: ピストルを両方のスロットから売ってしまった場合は何も起こらない
    }
    if (IsFirstKeyPress('2') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::SHOTGUN, m_player->GetPointsRef());
    }
    if (IsFirstKeyPress('3') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::RIFLE, m_player->GetPointsRef());
    }
    if (IsFirstKeyPress('4') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::SNIPER, m_player->GetPointsRef());
    }

    // Qキーで武器スワップ
    if (IsFirstKeyPress('Q') && m_weaponSystem->HasSecondaryWeapon() && !m_weaponSystem->IsReloading()) {
        int newSlot = 1 - m_weaponSystem->GetCurrentWeaponSlot();
        m_weaponSystem->SetCurrentWeaponSlot(newSlot);
        WeaponType newWeapon = (newSlot == 0) ?
            m_weaponSystem->GetPrimaryWeapon() :
            m_weaponSystem->GetSecondaryWeapon();
        m_weaponSystem->SwitchWeapon(newWeapon);
    }


    //  カメラ回転の変化量から武器の揺れを計算
    float rotationDeltaX = m_player->GetRotation().x - m_lastCameraRotX;
    float rotationDeltaY = m_player->GetRotation().y - m_lastCameraRotY;

    //  スウェイ強度調整
    float swayStrength = 0.5f;
    m_weaponSwayX += rotationDeltaY * swayStrength; //  左右回転で横揺れ
    m_weaponSwayY += rotationDeltaX * swayStrength; //  上下回転で横揺れ

    //  減衰効果
    m_weaponSwayX *= 0.9f;
    m_weaponSwayY *= 0.9f;

    // === 発砲リコイル減衰（バネ式で滑らかに戻る） ===
    m_weaponKickBack *= 0.85f;   // 後退 → 元に戻る
    m_weaponKickUp *= 0.85f;   // 跳ね上がり → 元に戻る
    m_weaponKickRot *= 0.85f;   // 回転 → 元に戻る

    // === ボスHPバー演出更新 ===
    {
        float targetHp = 0.0f;
        bool bossAlive = false;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS)
                && enemy.isAlive && !enemy.isDying)
            {
                targetHp = (float)enemy.health / (float)enemy.maxHealth;
                bossAlive = true;
                break;
            }
        }

        if (bossAlive)
        {
            if (targetHp < 0.0f) targetHp = 0.0f;
            float displaySpeed = 3.0f;
            m_bossHpDisplay += (targetHp - m_bossHpDisplay) * displaySpeed * m_deltaTime;
            float trailSpeed = 1.0f;
            m_bossHpTrail += (targetHp - m_bossHpTrail) * trailSpeed * m_deltaTime;
            if (m_bossHpDisplay < targetHp) m_bossHpDisplay = targetHp;
            if (m_bossHpTrail < m_bossHpDisplay) m_bossHpTrail = m_bossHpDisplay;
        }
        else
        {
            m_bossHpDisplay = 1.0f;
            m_bossHpTrail = 1.0f;
        }
    }

    //// === カメラ反動（上方向にキックしてゆっくり戻る） ===
    //if (fabsf(m_cameraRecoilVelocity) > 0.0001f || fabsf(m_cameraRecoilX) > 0.0001f)
    //{
    //    // カメラを上に動かす
    //    m_cameraRecoilX += m_cameraRecoilVelocity;

    //    // 速度を減衰（反動の勢いが収まる）
    //    m_cameraRecoilVelocity *= 0.8f;

    //    // 元の位置に戻すバネ力
    //    m_cameraRecoilX *= 0.88f;

    //    // 実際にカメラの角度を変更
    //    DirectX::XMFLOAT3 rot = m_player->GetRotation();
    //    rot.x -= m_cameraRecoilX * 0.3f;  // X回転 = 上下（マイナスで上を向く）
    //    m_player->SetRotation(rot);
    //}

    //  前フレームの回転を保持
    m_lastCameraRotX = m_player->GetRotation().x;
    m_lastCameraRotY = m_player->GetRotation().y;


    //  連射タイマー更新
    if (m_weaponSystem->GetFireRateTimer() > 0.0f) {
        m_weaponSystem->SetFireRateTimer(m_weaponSystem->GetFireRateTimer() - 1.0f / 60.0f);
    }

    //  === 射撃処理    ===
    if (!m_gloryKillCameraActive && m_player->IsMouseCaptured())
    {
        bool currentMouseState = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (currentMouseState && !m_lastMouseState &&
            !m_weaponSystem->IsReloading() &&
            m_weaponSystem->GetCurrentAmmo() > 0 &&
            m_weaponSystem->GetFireRateTimer() <= 0.0f &&
            m_shieldState != ShieldState::Guarding &&
            m_shieldState != ShieldState::Charging)
        {
            auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());

            // === デバッグ: 取得した武器データを確認 ===
            /*char debugWeapon[512];
            sprintf_s(debugWeapon, "[DEBUG WEAPON] GetWeaponData returned: damage=%d, maxAmmo=%d, fireRate=%.2f, cost=%d\n",
                weapon.damage, weapon.maxAmmo, weapon.fireRate, weapon.cost);
            //OutputDebugStringA(debugWeapon);*/

            //  弾を消費
            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() - 1);
            m_weaponSystem->SetFireRateTimer(weapon.fireRate);
            m_weaponSystem->SaveAmmoStatus();   //  弾薬状態を保存

            //  --- カメラリコイル（武器ごとに変える） ---
            WeaponType currentWeapon = m_weaponSystem->GetCurrentWeapon();
            switch (currentWeapon)
            {
            case WeaponType::PISTOL:
                m_cameraShake = 0.03f;
                m_cameraShakeTimer = 0.1f;
                m_weaponKickBack = 0.15f;
                m_weaponKickUp = 0.06f;
                m_weaponKickRot = 0.08f;
                break;

            case WeaponType::SHOTGUN:
                m_cameraShake = 0.15f;
                m_cameraShakeTimer = 0.2f;
                m_weaponKickBack = 0.4f;
                m_weaponKickUp = 0.15f;
                m_weaponKickRot = 0.2f;
                break;

            case WeaponType::RIFLE:
                m_cameraShake = 0.05f;
                m_cameraShakeTimer = 0.12f;
                m_weaponKickBack = 0.1f;
                m_weaponKickUp = 0.04f;
                m_weaponKickRot = 0.06f;
                break;

            case WeaponType::SNIPER:
                m_cameraShake = 0.2f;
                m_cameraShakeTimer = 0.25f;
                m_weaponKickBack = 0.5f;
                m_weaponKickUp = 0.2f;
                m_weaponKickRot = 0.25f;
                break;
            }

            
            //  銃口位置を「カメラ基準」で計算する（DrawWeaponと同じ方式）
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

            // forward（DrawWeaponと同じ）
            DirectX::XMVECTOR forward = DirectX::XMVectorSet(
                sinf(playerRot.y) * cosf(playerRot.x),
                -sinf(playerRot.x),
                cosf(playerRot.y) * cosf(playerRot.x), 0.0f);

            // right（DrawWeaponと同じ）
            DirectX::XMVECTOR right = DirectX::XMVectorSet(
                cosf(playerRot.y), 0.0f, -sinf(playerRot.y), 0.0f);

            // up = forward × right（//ワールド固定じゃなくカメラの上！）
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // 銃口 = プレイヤー位置 + 前0.8 + 右0.45 + 下0.15
            DirectX::XMVECTOR muzzleVec = DirectX::XMLoadFloat3(&playerPos);
            muzzleVec = DirectX::XMVectorAdd(muzzleVec, DirectX::XMVectorScale(forward, 0.8f));
            muzzleVec = DirectX::XMVectorAdd(muzzleVec, DirectX::XMVectorScale(right, 0.45f));
            muzzleVec = DirectX::XMVectorAdd(muzzleVec, DirectX::XMVectorScale(up, -0.15f));

            DirectX::XMFLOAT3 muzzlePosition;
            DirectX::XMStoreFloat3(&muzzlePosition, muzzleVec);

            //m_particleSystem->CreateMuzzleFlash(muzzlePosition, m_player->GetRotation(), currentWeapon);

            if (m_effectGunFire != nullptr && m_effekseerManager != nullptr)
            {
                auto handle = m_effekseerManager->Play(
                    m_effectGunFire,
                    muzzlePosition.x, muzzlePosition.y, muzzlePosition.z);

                // プレイヤーの視点方向に回転させる
                DirectX::XMFLOAT3 rot = m_player->GetRotation();
                Effekseer::Matrix43 mat;
                mat.Indentity();

                // Y軸回転（左右の向き）とX軸回転（上下の向き）
                float cosY = cosf(rot.y), sinY = sinf(rot.y);
                float cosX = cosf(rot.x), sinX = sinf(rot.x);

                mat.Value[0][0] = cosY;
                mat.Value[0][1] = 0;
                mat.Value[0][2] = -sinY;

                mat.Value[1][0] = sinX * sinY;
                mat.Value[1][1] = cosX;
                mat.Value[1][2] = sinX * cosY;

                mat.Value[2][0] = cosX * sinY;
                mat.Value[2][1] = -sinX;
                mat.Value[2][2] = cosX * cosY;

                mat.Value[3][0] = muzzlePosition.x;
                mat.Value[3][1] = muzzlePosition.y;
                mat.Value[3][2] = muzzlePosition.z;

                m_effekseerManager->SetMatrix(handle, mat);
                m_effekseerManager->SetScale(handle, 0.25f, 0.25f, 0.25f);
            }

            //  ショットガンは複数の弾を発射
            int pellets = (m_weaponSystem->GetCurrentWeapon() == WeaponType::SHOTGUN) ? 8 : 1;
            for (int p = 0; p < pellets; p++)
            {
                // 射撃方向を計算
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

                //  カメラ(視点)空の位置から玉を発射
                DirectX::XMFLOAT3 rayStart = playerPos;

                //  射撃方向
                DirectX::XMFLOAT3 rayDir(
                    sinf(playerRot.y) * cosf(playerRot.x),
                    -sinf(playerRot.x),
                    cosf(playerRot.y) * cosf(playerRot.x)
                );

                // 散弾の場合はランダムに広がる
                DirectX::XMFLOAT3 shotDir = rayDir;
                if (m_weaponSystem->GetCurrentWeapon() == WeaponType::SHOTGUN)
                {
                    float spread = 0.1f;
                    shotDir.x += ((float)rand() / RAND_MAX - 0.5f) * spread;
                    shotDir.y += ((float)rand() / RAND_MAX - 0.5f) * spread;
                    shotDir.z += ((float)rand() / RAND_MAX - 0.5f) * spread;
                }

                // === Bullet Physics でレイキャスト ===
                auto rayResult = RaycastPhysics(rayStart, shotDir, 100.0f);

                if (rayResult.hit)
                {
                    // === ヒット成功！ ===
                    
                    // 弾道を記録
                    BulletTrace trace;
                    trace.start = rayStart;
                    trace.end = rayResult.hitPoint;
                    trace.lifetime = 0.3f;
                    trace.maxLifetime = 0.3f;
                    trace.width = 0.15f;

                    switch (currentWeapon)
                    {
                    case WeaponType::PISTOL:
                        trace.color = { 1.0f, 0.8f, 0.2f, 1.0f };
                        break;
                    case WeaponType::SHOTGUN:
                        trace.color = { 1.0f, 0.5f, 0.1f, 1.0f };
                        break;
                    case WeaponType::RIFLE:
                        trace.color = { 0.3f, 0.8f, 1.0f, 1.0f };
                        break;
                    case WeaponType::SNIPER:
                        trace.color = { 1.0f, 0.2f, 0.2f, 1.0f };
                        trace.lifetime = 0.5f;
                        trace.maxLifetime = 0.5f;
                        trace.width = 0.25f;
                        break;
                    default:
                        trace.color = { 1.0f, 0.6f, 0.0f, 1.0f };
                        break;
                    }
                    m_bulletTraces.push_back(trace);

                    // UserPointer から敵を取得
                    Enemy* hitEnemy = rayResult.hitEnemy;

                    if (hitEnemy != nullptr)
                    {
                        // === 敵にヒット！ ===

                        // ヒットエフェクト（弾が当たった位置に再生）
                        if (m_effectAttackImpact != nullptr && m_effekseerManager != nullptr)
                        {
                            m_effekseerManager->Play(
                                m_effectAttackImpact,
                                rayResult.hitPoint.x,
                                rayResult.hitPoint.y,
                                rayResult.hitPoint.z);
                        }

                        // 設定取得（デバッグモード対応）
                        EnemyTypeConfig config;
                        if (m_useDebugHitboxes)
                        {
                            switch (hitEnemy->type)
                            {
                            case EnemyType::NORMAL:
                                config = m_normalConfigDebug;
                                break;
                            case EnemyType::RUNNER:
                                config = m_runnerConfigDebug;
                                break;
                            case EnemyType::TANK:
                                config = m_tankConfigDebug;
                                break;
                            case EnemyType::MIDBOSS:
                                config = m_midbossConfigDebug;
                                break;
                            case EnemyType::BOSS:
                                config = m_bossConfigDebug;
                                break;
                            }
                        }
                        else
                        {
                            config = GetEnemyConfig(hitEnemy->type);
                        }

                        // 頭の位置計算
                        hitEnemy->headPosition.x = hitEnemy->position.x;
                        hitEnemy->headPosition.y = hitEnemy->position.y + config.headHeight;
                        hitEnemy->headPosition.z = hitEnemy->position.z;

                        // ヘッドショット判定
                        float dx = rayResult.hitPoint.x - hitEnemy->headPosition.x;
                        float dy = rayResult.hitPoint.y - hitEnemy->headPosition.y;
                        float dz = rayResult.hitPoint.z - hitEnemy->headPosition.z;
                        float distanceToHead = sqrtf(dx * dx + dy * dy + dz * dz);

                        bool isHeadShot = (distanceToHead < config.headRadius);

                        if (isHeadShot && !hitEnemy->headDestroyed)
                        {
                            // === ヘッドショット処理 ===
                            ////OutputDebugStringA("[BULLET] HEADSHOT!\n");

                            if (hitEnemy->type != EnemyType::BOSS &&
                                hitEnemy->type != EnemyType::MIDBOSS &&
                                hitEnemy->type != EnemyType::TANK)
                            {
                                hitEnemy->headDestroyed = true;
                            }
                            hitEnemy->bloodDirection = shotDir;

                            // 血のエフェクト（後方）
                            //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, shotDir, 300);

                            // 血のエフェクト（上方）
                            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                            //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, upDir, 300);

                            // 放射状の血
                            for (int i = 0; i < 12; i++)
                            {
                                float angle = (i / 12.0f) * 6.28318f;
                                DirectX::XMFLOAT3 radialDir;
                                radialDir.x = cosf(angle);
                                radialDir.y = 0.3f + (rand() % 100) / 200.0f;
                                radialDir.z = sinf(angle);
                                //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, radialDir, 50);
                            }

                            // ランダム方向の血
                            for (int i = 0; i < 4; i++)
                            {
                                DirectX::XMFLOAT3 randomDir;
                                randomDir.x = ((rand() % 200) - 100) / 100.0f;
                                randomDir.y = ((rand() % 100) + 50) / 100.0f;
                                randomDir.z = ((rand() % 200) - 100) / 100.0f;
                                //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, randomDir, 50);
                            }

                            //m_particleSystem->CreateExplosion(hitEnemy->headPosition);

                            // ダメージ
                            if (hitEnemy->type == EnemyType::BOSS || hitEnemy->type == EnemyType::MIDBOSS || hitEnemy->type == EnemyType::TANK)
                            {
                                hitEnemy->health -= weapon.damage * 3;
                                m_statDamageDealt += weapon.damage * 3;  //  与ダメ記録
                            }
                            else
                            {
                                hitEnemy->health = 0;
                            }

                            if (hitEnemy->type != EnemyType::BOSS && hitEnemy->type != EnemyType::MIDBOSS && hitEnemy->type != EnemyType::TANK)
                            {
                                // 雑魚だけ吹っ飛ばす
                                hitEnemy->isRagdoll = true;
                                hitEnemy->velocity.x = shotDir.x * 15.0f;
                                hitEnemy->velocity.y = 8.0f;
                                hitEnemy->velocity.z = shotDir.z * 15.0f;

                                SpawnGibs(hitEnemy->position, 8, 15.0f);
                            }

                            // Effekseer エフェクト
                            /*if (m_effekseerManager != nullptr && m_effectBlood != nullptr)
                            {
                                auto handle = m_effekseerManager->Play(
                                    m_effectBlood,
                                    hitEnemy->headPosition.x,
                                    hitEnemy->headPosition.y,
                                    hitEnemy->headPosition.z
                                );
                                m_effekseerManager->SetScale(handle, 2.0f, 2.0f, 2.0f);
                            }*/

                            //  完全停止からスロー
                            m_timeScale = 0.0f;          // 完全停止
                            m_hitStopTimer = 0.04f;      // 0.08秒間フリーズ
                            m_slowMoTimer = 0.0f;
                            

                            // カメラシェイク
                            m_cameraShake = 0.3f;
                            m_cameraShakeTimer = 0.5f;
                        }
                        else
                        {
                            // === ボディショット処理 ===
                            ////OutputDebugStringA("[BULLET] Body shot\n");

                            // === デバッグ: ダメージ前のHP ===
                            //char debugHP1[256];
                            //sprintf_s(debugHP1, "[DEBUG] Enemy ID:%d HP BEFORE damage: %df\n",
                            //    hitEnemy->id, hitEnemy->health);
                            ////OutputDebugStringA(debugHP1);

                            //// === デバッグ: 武器のダメージ ===
                            //char debugDamage[256];
                            //sprintf_s(debugDamage, "[DEBUG] Weapon damage: %d (WeaponType:%d)\n",
                            //    weapon.damage, (int)currentWeapon);
                            ////OutputDebugStringA(debugDamage);

                            //m_particleSystem->CreateBloodEffect(rayResult.hitPoint, shotDir, 15);
                            m_gpuParticles->EmitSplash(rayResult.hitPoint, shotDir, 15, 4.0f);
                            m_gpuParticles->EmitMist(rayResult.hitPoint, 30, 1.5f);
                            hitEnemy->health -= weapon.damage;
                            m_statDamageDealt += weapon.damage;  //  与ダメ記録

                            // === デバッグ: ダメージ後のHP ===
                            /*char debugHP2[256];
                            sprintf_s(debugHP2, "[DEBUG] Enemy ID:%d HP AFTER damage: %d\n",
                                hitEnemy->id, hitEnemy->health);
                            //OutputDebugStringA(debugHP2);*/


                            // カメラシェイク（強化）
                            m_cameraShake = 0.15f;
                            m_cameraShakeTimer = 0.15f;

                            // 死亡時の吹っ飛ばし
                            if (hitEnemy->health <= 0.0f)
                            {
                                // === デバッグ: ラグドール処理に入った ===
                                ////OutputDebugStringA("[DEBUG] Entering ragdoll processing (HP <= 0.0f)\n");

                                hitEnemy->isRagdoll = true;

                                //  カメラシェイク
                                m_cameraShake = 0.25f;
                                m_cameraShakeTimer = 0.2;

                                m_bloodSystem->OnEnemyKilled(hitEnemy->position, m_player->GetPosition());

                                float knockbackPower = 10.0f;

                                switch (currentWeapon)
                                {
                                case WeaponType::PISTOL:
                                    knockbackPower = 8.0f;
                                    break;
                                case WeaponType::SHOTGUN:
                                    knockbackPower = 25.0f;
                                    break;
                                case WeaponType::RIFLE:
                                    knockbackPower = 12.0f;
                                    break;
                                case WeaponType::SNIPER:
                                    knockbackPower = 20.0f;
                                    break;
                                }

                                hitEnemy->velocity.x = shotDir.x * knockbackPower;
                                hitEnemy->velocity.y = 5.0f;
                                hitEnemy->velocity.z = shotDir.z * knockbackPower;
                            }
                            else
                            {
                                // === デバッグ: ラグドール処理に入らなかった ===
                               ////OutputDebugStringA("[DEBUG] NOT entering ragdoll (HP > 0.0f)\n");
                            }
                        }

                        // ノックバック
                        /*float knockbackStrength = isHeadShot ? 0.5f : 0.2f;
                        hitEnemy->position.x += shotDir.x * knockbackStrength;
                        hitEnemy->position.z += shotDir.z * knockbackStrength;*/

                        // 死亡処理
                        if (hitEnemy->health <= 0)
                        {
                            // === デバッグ: 死亡判定に入った ===
                            ////OutputDebugStringA("[DEBUG] Entering death check (HP <= 0)\n");

                            if (!hitEnemy->isDying)
                            {
                                // === デバッグ: 死亡処理実行 ===
                                /*char debugDeath[256];
                                sprintf_s(debugDeath, "[DEBUG] Setting isDying=true, isAlive=false for enemy ID:%d\n",
                                    hitEnemy->id);*/
                                /*//OutputDebugStringA(debugDeath);*/

                                hitEnemy->isDying = true;
                                hitEnemy->isAlive = false;
                                hitEnemy->currentAnimation = "Death";
                                hitEnemy->animationTime = 0.0f;
                                // 変更後（ボスは早く消す）:
                                if (hitEnemy->type == EnemyType::BOSS || hitEnemy->type == EnemyType::MIDBOSS)
                                    hitEnemy->corpseTimer = 2.0f;  // ボスは2秒で消す
                                else
                                    hitEnemy->corpseTimer = 5.0f;

                                // ===  死んだ瞬間に物理ボディを削除 ===
                                RemoveEnemyPhysicsBody(hitEnemy->id);

                                m_bloodSystem->OnEnemyKilled(hitEnemy->position, m_player->GetPosition());
                                m_gpuParticles->EmitSplash(hitEnemy->position, XMFLOAT3(hitEnemy->position.x - m_player->GetPosition().x, hitEnemy->position.y - m_player->GetPosition().y, hitEnemy->position.z - m_player->GetPosition().z), 40, 7.0f);
                                m_gpuParticles->EmitMist(rayResult.hitPoint, 100, 2.0f);
                                m_gpuParticles->Emit(hitEnemy->position, 60, 5.0f);

                                // MIDBOSSが死んだらビーム停止
                                if (hitEnemy->type == EnemyType::MIDBOSS &&
                                    m_beamHandle >= 0 && m_effekseerManager != nullptr)
                                {
                                    m_effekseerManager->StopEffect(m_beamHandle);
                                    m_beamHandle = -1;
                                }

                                // ポイント加算
                                int waveBonus = m_waveManager->OnEnemyKilled();
                                if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);
                                int totalPoints = (isHeadShot ? 150 : 60) + waveBonus;
                                m_player->AddPoints(totalPoints);
                                SpawnScorePopup(totalPoints, isHeadShot ? ScorePopupType::HEADSHOT : ScorePopupType::KILL);

                                //  スタイルランク：キル通知
                                m_styleRank->NotifyKill(isHeadShot);

                                //  スタッツ記録
                                m_statKills++;
                                if (isHeadShot) m_statHeadshots++;

                                // ダメージ表示
                                m_showDamageDisplay = true;
                                m_damageDisplayTimer = 2.0f;
                                m_damageDisplayPos = hitEnemy->position;
                                m_damageDisplayPos.y += 2.0f;
                                m_damageValue = isHeadShot ? 150 : 60;
                            }

                            else
                            {
                                // === デバッグ: すでにisDying=true ===
                                /*//OutputDebugStringA("[DEBUG] Enemy already dying (isDying=true)\n");*/
                            }

                            

                            // ウィンドウタイトル更新
                            /*char debug[256];
                            sprintf_s(debug, isHeadShot ? "HEADSHOT!! Points:%d" : "Hit! Points:%d",
                                m_player->GetPoints());*/
                            //SetWindowTextA(m_window, debug);
                        }
                        else
                        {
                            // === デバッグ: 死亡判定に入らなかった ===
                            /*//OutputDebugStringA("[DEBUG] NOT entering death check (HP > 0)\n");*/
                        }
                    }
                    else
                    {
                        // 敵に当たらなかった（壁など）
                       /* //OutputDebugStringA("[BULLET] Hit wall or object\n");*/
                       // 敵に当たらなかった（壁など）→ 壁に血痕を貼る
                     
                    }
                }
                else
                {
                    // 何にも当たらなかった
                    // 法線がほぼ水平 → 壁ヒット
                    
                }
            }
}
        m_lastMouseState = currentMouseState;
    
}

    // === 弾切れ警告の更新 ===
    {

        bool outOfAmmo = (m_weaponSystem->GetCurrentAmmo() == 0) &&
            !m_weaponSystem->IsReloading();

        if (outOfAmmo)
        {
            // 警告フェードイン
            m_reloadWarningTimer += deltaTime;
            m_reloadWarningAlpha += deltaTime * 5.0f;
            if (m_reloadWarningAlpha > 1.0f) m_reloadWarningAlpha = 1.0f;
        }
        else
        {
            // 警告フェードアウト
            m_reloadWarningAlpha -= deltaTime * 8.0f;
            if (m_reloadWarningAlpha < 0.0f)
            {
                m_reloadWarningAlpha = 0.0f;
                m_reloadWarningTimer = 0.0f;
            }
        }
    }

    // Rキーでリロード開始
    if (IsFirstKeyPress('R') &&
        m_weaponSystem->GetCurrentAmmo() < m_weaponSystem->GetMaxAmmo() &&
        m_weaponSystem->GetReserveAmmo() > 0 &&
        !m_weaponSystem->IsReloading())
    {
        m_weaponSystem->SetReloading(true);
        auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());
        m_weaponSystem->SetReloadTimer(weapon.reloadTime);
    }

    // === リロード中の処理 ===
    if (m_weaponSystem->IsReloading())
    {
        // リロード残り時間を減らす
        float newTimer = m_weaponSystem->GetReloadTimer() - deltaTime;
        m_weaponSystem->SetReloadTimer(newTimer);

        // 現在の武器のリロード時間を取得して、進行度（0→1）を計算
        auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());
        float totalTime = weapon.reloadTime;                // 例：ショットガンなら1.0秒
        float progress = 1.0f - (newTimer / totalTime);     // 0.0（開始）→ 1.0（完了）
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        m_reloadAnimProgress = progress;

        // --- フェーズアニメーション ---
        // フェーズ1（0?30%）: 武器を下に下げる
        if (progress < 0.3f)
        {
            float t = progress / 0.3f;    // このフェーズ内の進行度（0→1）
            t = t * t;                     // イーズイン：最初ゆっくり→だんだん速く
            m_reloadAnimOffset = t * -0.4f; // 最大0.4m下に下げる
            m_reloadAnimTilt = t * 0.5f;    // 最大0.5rad手前に傾ける
        }
        // フェーズ2（30?70%）: 画面下で停止（マガジン交換中のイメージ）
        else if (progress < 0.7f)
        {
            m_reloadAnimOffset = -0.4f;     // 下がったまま
            m_reloadAnimTilt = 0.5f;        // 傾いたまま
        }
        // フェーズ3（70?100%）: 武器を元の位置に戻す
        else
        {
            float t = (progress - 0.7f) / 0.3f;         // このフェーズ内の進行度（0→1）
            t = 1.0f - (1.0f - t) * (1.0f - t);         // イーズアウト：最初速い→だんだんゆっくり
            m_reloadAnimOffset = -0.4f * (1.0f - t);     // 下から元の位置へ
            m_reloadAnimTilt = 0.5f * (1.0f - t);        // 傾きも元に戻る
        }

        // リロード完了判定
        if (newTimer <= 0.0f)
        {
            // 弾を補充する
            int needed = m_weaponSystem->GetMaxAmmo() - m_weaponSystem->GetCurrentAmmo();
            int reload = min(needed, m_weaponSystem->GetReserveAmmo());
            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() + reload);
            m_weaponSystem->SetReserveAmmo(m_weaponSystem->GetReserveAmmo() - reload);
            m_weaponSystem->SetReloading(false);
            m_weaponSystem->SaveAmmoStatus();

            // アニメーションをリセット
            m_reloadAnimProgress = 0.0f;
            m_reloadAnimOffset = 0.0f;
            m_reloadAnimTilt = 0.0f;
        }
    }
    else
    {
        // リロードしてない時：残ってる動きを滑らかにゼロへ戻す
        // powを使ってフレームレートに依存しない減衰にする
        float decay = powf(0.85f, m_deltaTime * 60.0f);  // 60FPS基準で同じ速度
        m_reloadAnimOffset *= decay;
        m_reloadAnimTilt *= decay;
    }
    

    if (m_showDamageDisplay)
    {
        m_damageDisplayTimer -= deltaTime; // 60FPS想定
        if (m_damageDisplayTimer <= 0.0f)
        {
            m_showDamageDisplay = false;
        }
    }

    //  アニメーションを進める
   /* if (m_effekseerManager != nullptr)
    {
        m_effekseerManager->Update(1.0f / 60.0f);
    }*/

    m_particleSystem->Update(deltaTime);

    m_bloodSystem->Update(deltaTime);

    //  グローリーキル中は敵の更新を停止
    if (!m_gloryKillCameraActive)
    {
        // === 敵の旧座標を保存（パス追従の基準に使う） ===
        struct EnemyOldPos { int id; float x, z; };
        std::vector<EnemyOldPos> enemyOldPositions;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.isAlive && !enemy.isDying && !enemy.isRagdoll)
                enemyOldPositions.push_back({ enemy.id, enemy.position.x, enemy.position.z });
        }
        //  【意味】プレイヤーの現在位置
        //  【用途】敵がプレイヤーに向かって動くため
        m_enemySystem->Update(deltaTime, m_player->GetPosition());

        if (m_navGrid.IsReady())
        {
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                // 死亡中・ラグドール・スタガー中・ボス系はスキップ
                if (!enemy.isAlive || enemy.isDying || enemy.isRagdoll
                    || enemy.isStaggered
                    || enemy.type == EnemyType::MIDBOSS
                    || enemy.type == EnemyType::BOSS)
                    continue;

                // --- 経路再計算タイマー ---
                enemy.aiPathTimer -= deltaTime;
                if (enemy.aiPathTimer <= 0.0f)
                {
                    enemy.aiPathTimer = 0.5f + (float)(enemy.id % 5) * 0.1f;

                    enemy.aiPath = m_navGrid.FindPath(
                        enemy.position.x, enemy.position.z,
                        playerPos.x, playerPos.z);
                    enemy.aiPathIndex = 0;

                    if (enemy.aiPath.size() > 1)
                        enemy.aiPathIndex = 1;
                }

                // パスが無い or 短い → EnemySystemのまま
                if (enemy.aiPath.empty() || enemy.aiPathIndex >= (int)enemy.aiPath.size())
                {
                    enemy.aiControlled = false;
                    continue;
                }
                // プレイヤーが近い → 直接移動
                float dxP = playerPos.x - enemy.position.x;
                float dzP = playerPos.z - enemy.position.z;
                if (sqrtf(dxP * dxP + dzP * dzP) < 3.0f)
                {
                    enemy.aiPath.clear();
                    enemy.aiControlled = false;
                    continue;
                }

                // 旧座標を取得
                float oldX = enemy.position.x;
                float oldZ = enemy.position.z;
                for (const auto& op : enemyOldPositions)
                {
                    if (op.id == enemy.id) { oldX = op.x; oldZ = op.z; break; }
                }

                // EnemySystemの移動を取り消し、パスに沿って移動
                DirectX::XMFLOAT3& waypoint = enemy.aiPath[enemy.aiPathIndex];

                float dx = waypoint.x - oldX;
                float dz = waypoint.z - oldZ;
                float dist = sqrtf(dx * dx + dz * dz);

                if (dist > 0.1f)
                {
                    enemy.aiControlled = true;  // A*が制御中

                    float nx = dx / dist;
                    float nz = dz / dist;

                    // プレイヤーとの距離で速度を決定
                    float distToPlayer = sqrtf(dxP * dxP + dzP * dzP);
                    float runThreshold = 8.0f;

                    float walkSpeed = 2.5f;
                    float runSpeed = 5.0f;
                    switch (enemy.type)
                    {
                    case EnemyType::NORMAL: walkSpeed = 2.5f; runSpeed = 4.5f; break;
                    case EnemyType::RUNNER: walkSpeed = 3.0f; runSpeed = 6.0f; break;
                    case EnemyType::TANK:   walkSpeed = 1.5f; runSpeed = 2.5f; break;
                    }

                    bool shouldRun = (distToPlayer > runThreshold);
                    float speed = shouldRun ? runSpeed : walkSpeed;
                    speed *= m_enemySystem->m_waveSpeedMult;

                    enemy.position.x = oldX + nx * speed * deltaTime;
                    enemy.position.z = oldZ + nz * speed * deltaTime;
                    enemy.rotationY = atan2f(nx, nz) + 3.14159f;

                    // velocityをEnemySystemと整合させる
                    float velMag = shouldRun ? 2.0f : 0.5f;
                    enemy.velocity.x = nx * velMag;
                    enemy.velocity.z = nz * velMag;

                    // アニメーション設定
                    std::string moveAnim = shouldRun ? "Run" : "Walk";
                    if (enemy.currentAnimation != moveAnim &&
                        enemy.currentAnimation != "Death" &&
                        enemy.currentAnimation != "Stun")
                    {
                        enemy.currentAnimation = moveAnim;
                        enemy.animationTime = 0.0f;
                    }
                }
               
                // ウェイポイントに近づいたら次へ
                float dxW = waypoint.x - enemy.position.x;
                float dzW = waypoint.z - enemy.position.z;
                if (sqrtf(dxW * dxW + dzW * dzW) < 1.0f)
                {
                    enemy.aiPathIndex++;
                }
            }
        }
    }

    // =============================================
     // === 敵同士の重なり押し出し（A*移動後） ===
     // =============================================
     {
         auto& enemies = m_enemySystem->GetEnemies();
         const float collisionRadius = 0.5f;
         const float minDist = collisionRadius * 2.0f;  // 1.0m以内で押し出し
         const float pushStr = 0.08f;

         for (size_t i = 0; i < enemies.size(); i++)
         {
             if (!enemies[i].isAlive || enemies[i].isDying) continue;

             for (size_t j = i + 1; j < enemies.size(); j++)
             {
                 if (!enemies[j].isAlive || enemies[j].isDying) continue;

                 float dx = enemies[j].position.x - enemies[i].position.x;
                 float dz = enemies[j].position.z - enemies[i].position.z;
                 float dist = sqrtf(dx * dx + dz * dz);

                 if (dist < minDist && dist > 0.001f)
                 {
                     float nx = dx / dist;
                     float nz = dz / dist;
                     float overlap = (minDist - dist) * pushStr;

                     enemies[i].position.x -= nx * overlap;
                     enemies[i].position.z -= nz * overlap;
                     enemies[j].position.x += nx * overlap;
                     enemies[j].position.z += nz * overlap;
                 }
             }
         }
    }

    //  === 敵の「移動・回転・アニメーション更新・死亡」をまとめて制御するループ   ===
    //DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (enemy.isAlive && !enemy.isDying)
        {
            UpdateEnemyPhysicsBody(enemy);
        }

        // === 死亡処理 ===
        if (enemy.isDying)
        {
            enemy.corpseTimer -= deltaTime;
            if (enemy.corpseTimer <= 0.0f)
            {
                enemy.isAlive = false;
                enemy.isDying = false;
                RemoveEnemyPhysicsBody(enemy.id);

                continue;
            }
        }

        //  ========================================
        //  アニメーション更新
        //  ========================================
        if (enemy.type != EnemyType::BOSS && enemy.type != EnemyType::MIDBOSS &&
            m_enemyModel && m_enemyModel->HasAnimation(enemy.currentAnimation))
        {
            float duration = m_enemyModel->GetAnimationDuration(enemy.currentAnimation);
            // ↑ ここで duration が定義される

            if (enemy.isDying)
            {
                //  死亡中: 終端で止める（タイプ別速度）
                int typeIdx = 0;
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  typeIdx = 0; break;
                case EnemyType::RUNNER:  typeIdx = 1; break;
                case EnemyType::TANK:    typeIdx = 2; break;
                case EnemyType::MIDBOSS: typeIdx = 3; break;
                case EnemyType::BOSS:    typeIdx = 4; break;
                }
                float deathSpeed = m_enemySystem->m_animSpeed_Death[typeIdx];
                enemy.animationTime += deltaTime * deathSpeed;
                if (enemy.animationTime >= duration)  // ← ここで使える
                {
                    enemy.animationTime = duration - 0.001f;
                }
            }
            else
            {
                //  通常: ループ（タイプ×アニメ別速度）
                int typeIdx = 0;
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  typeIdx = 0; break;
                case EnemyType::RUNNER:  typeIdx = 1; break;
                case EnemyType::TANK:    typeIdx = 2; break;
                case EnemyType::MIDBOSS: typeIdx = 3; break;
                case EnemyType::BOSS:    typeIdx = 4; break;
                }

                float animSpeed = 0.1f;
                if (enemy.currentAnimation == "Idle")        animSpeed = m_enemySystem->m_animSpeed_Idle[typeIdx];
                else if (enemy.currentAnimation == "Walk")   animSpeed = m_enemySystem->m_animSpeed_Walk[typeIdx];
                else if (enemy.currentAnimation == "Run")    animSpeed = m_enemySystem->m_animSpeed_Run[typeIdx];
                else if (enemy.currentAnimation == "Attack") animSpeed = 0.0f;
                else if (enemy.currentAnimation == "AttackJump") animSpeed = 0.0f;
                else if (enemy.currentAnimation == "AttackSlash") animSpeed = 0.0f;
                else                                         animSpeed = 0.1f;

                enemy.animationTime += deltaTime * animSpeed;
                if (enemy.animationTime >= duration)
                {
                    enemy.animationTime = 0.0f;
                }
            }
        }

        //  === 首から血を噴き出す   ===
        if (enemy.headDestroyed && enemy.isAlive)
        {
            // 敵の向いている方向の前方ベクトル
            float forwardX = -sinf(enemy.rotationY);
            float forwardZ = -cosf(enemy.rotationY);

            // 首は体の中心から前方に少しずらした位置
            DirectX::XMFLOAT3 neckPosition;
            neckPosition.x = enemy.position.x + forwardX * 1.8f;  
            neckPosition.z = enemy.position.z + forwardZ * 1.8f;

            // アニメーション進行度で高さを調整
            if (enemy.isDying && m_enemyModel && m_enemyModel->HasAnimation("Death"))
            {
                float deathDuration = m_enemyModel->GetAnimationDuration("Death");
                float progress = enemy.animationTime / deathDuration;
                progress = min(progress, 1.0f);

                // 立ってる時: 1.5m → 倒れた時: 0.3m
                float neckHeight = 1.5f - (progress * 1.2f);
                neckPosition.y = enemy.position.y + neckHeight;
            }
            else
            {
                // 通常時
                neckPosition.y = enemy.position.y + 1.5f;
            }

            // 血を噴出（保存した弾の方向を使う）
            //m_particleSystem->CreateBloodEffect(
            //    neckPosition,
            //    enemy.bloodDirection,  //   弾が飛んできた方向
            //    3
            //);
        }
    }
    UpdatePhysics(deltaTime);

    //  死んだ敵を削除 毎フレーム敵を削除しないと、配列か肥大化
    m_enemySystem->ClearDeadEnemies();

    //  ウェーブ管理
    m_waveManager->Update(1.0f / 60.0f, m_player->GetPosition(), m_enemySystem.get());

    // === ウェーブバナー検出 ===
    {
        int currentWave = m_waveManager->GetCurrentWave();
        if (currentWave != m_lastWaveNumber)
        {
            m_lastWaveNumber = currentWave;
            m_waveBannerTimer = m_waveBannerDuration;
            m_waveBannerNumber = currentWave;
            m_waveBannerIsBoss = (currentWave % 2 == 0) || (currentWave % 3 == 0);
        }
        if (m_waveBannerTimer > 0.0f)
            m_waveBannerTimer -= 1.0f / 60.0f;
    }

    // === スコアHUD更新 ===
    {
        float dt = 1.0f / 60.0f;
        int realScore = m_player->GetPoints();

        // スコアが増えたら演出発火
        if (realScore > m_lastDisplayedScore)
        {
            int diff = realScore - m_lastDisplayedScore;
            m_scoreFlashTimer = 0.4f;
            m_scoreShakeTimer = 0.2f;
        }
        m_lastDisplayedScore = realScore;

        // 表示値をスムーズに追従（スロットマシン風に回る）
        float diff = (float)realScore - m_scoreDisplayValue;
        if (fabsf(diff) < 1.0f)
            m_scoreDisplayValue = (float)realScore;
        else
            m_scoreDisplayValue += diff * 8.0f * dt;

        if (m_scoreFlashTimer > 0.0f) m_scoreFlashTimer -= dt;
        if (m_scoreShakeTimer > 0.0f) m_scoreShakeTimer -= dt;
    }

    // === 爪痕エフェクト検出（背後からの攻撃のみ） ===
    {
        static int lastHP = 100;
        int curHP = m_player->GetHealth();
        if (curHP < lastHP && curHP > 0)
        {
            // プレイヤーの前方ベクトル
            float playerYaw = m_player->GetRotation().y;
            float forwardX = sinf(playerYaw);
            float forwardZ = cosf(playerYaw);
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

            // 一番近い生きてる敵を探す
            float closestDist = 999.0f;
            bool isBehind = false;
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                float dx = enemy.position.x - playerPos.x;
                float dz = enemy.position.z - playerPos.z;
                float dist = sqrtf(dx * dx + dz * dz);
                if (dist < closestDist && dist < 10.0f)
                {
                    closestDist = dist;
                    // 内積: 正=前方、負=背後
                    float dot = forwardX * dx + forwardZ * dz;
                    isBehind = (dot < 0.0f);
                }
            }

            if (isBehind)
                m_clawTimer = m_clawDuration;
        }
        lastHP = curHP;

        if (m_clawTimer > 0.0f)
            m_clawTimer -= 1.0f / 60.0f;
    }

    //  全敵のY座標を床の高さに合わせる
    //  全敵のY座標をメッシュ床の高さに合わせる
    {
        for (auto& enemy : m_enemySystem->GetEnemiesMutable())
        {
            if (enemy.isAlive)
            {
                //  ボスのジャンプ中はY座標を上書きしない
                if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS) &&
                    (enemy.bossPhase == BossAttackPhase::JUMP_AIR ||
                        enemy.bossPhase == BossAttackPhase::JUMP_SLAM))
                    continue;

                enemy.position.y = GetMeshFloorHeight(
                    enemy.position.x, enemy.position.z, enemy.position.y);
            }
        }
    }

    // ボススポーンエフェクト
    if (m_effectBossSpawn != nullptr && m_effekseerManager != nullptr)
    {
        if (m_waveManager->DidMidBossSpawn())
        {
            // MIDBOSSの位置を探してエフェクト再生
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive)
                {
                    auto h = m_effekseerManager->Play(m_effectBossSpawn,
                        enemy.position.x, enemy.position.y, enemy.position.z);
                    m_effekseerManager->SetScale(h, 3.0f, 3.0f, 3.0f);
                    break;
                }
            }
        }
        if (m_waveManager->DidBossSpawn())
        {
            // BOSSの位置を探してエフェクト再生
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::BOSS && enemy.isAlive)
                {
                    auto h = m_effekseerManager->Play(m_effectBossSpawn,
                        enemy.position.x, enemy.position.y, enemy.position.z);
                    m_effekseerManager->SetScale(h, 5.0f, 5.0f, 5.0f);
                    break;
                }
            }
        }
    }

    // 雑魚敵スポーンエフェクト
    if (m_effectEnemySpawn != nullptr && m_effekseerManager != nullptr)
    {
        for (auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.justSpawned)
            {
                // ボス系は BossSpawn で出すからスキップ
                if (enemy.type != EnemyType::MIDBOSS && enemy.type != EnemyType::BOSS)
                {
                    auto h = m_effekseerManager->Play(m_effectEnemySpawn,
                        enemy.position.x, enemy.position.y, enemy.position.z);
                    m_effekseerManager->SetScale(h, 1.5f, 1.5f, 1.5f);
                }

                enemy.justSpawned = false;
            }
        }
    }
  

    //  新しくスポーンされた敵に物理ボディを追加
    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (enemy.isAlive)
        {
            //  子の敵がまだ物理ボディを持っていないか確認
            if (m_enemyPhysicsBodies.find(enemy.id) == m_enemyPhysicsBodies.end())
            {
                AddEnemyPhysicsBody(enemy);
            }
        }
    }


    // ==============================================
    // シールドシステム入力処理
    // ==============================================
    static bool rmbPressed = false;
    bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    switch (m_shieldState)
    {
        // ─── 通常状態 ───
    case ShieldState::Idle:
    {
        // 右クリック押した瞬間 → パリィウィンドウ開始
        if (rmbDown && !rmbPressed)
        {
            m_shieldState = ShieldState::Parrying;
            m_parryWindowTimer = m_parryWindowDuration;  // 0.15秒
            m_lastParryAttemptTime = m_gameTime;
            m_shieldBashTimer = m_shieldBashDuration;    // 盾アニメ開始
            //OutputDebugStringA("[SHIELD] State: PARRYING (window open)\n");
        }

        // 盾を下ろすアニメ（ゆっくり戻す）
        m_shieldGuardBlend = max(0.0f, m_shieldGuardBlend - deltaTime * 5.0f);

        //  ガードしてないときはエネルギーリセット
        m_chargeEnergy = 0.0f;
        m_chargeReady = false;

        // シールドHP回復（非ガード時、遅延後）
        if (m_shieldRegenDelayTimer > 0.0f)
        {
            m_shieldRegenDelayTimer -= deltaTime;
        }
        else if (m_shieldHP < m_shieldMaxHP)
        {
            m_shieldHP = min(m_shieldMaxHP, m_shieldHP + m_shieldRegenRate * deltaTime);
        }
        break;
    }

    // ─── パリィ受付中（右クリ押した直後の短い時間）───
    case ShieldState::Parrying:
    {
        m_parryWindowTimer -= deltaTime;

        // 盾を構えるアニメ（素早く上げる）
        m_shieldGuardBlend = min(1.0f, m_shieldGuardBlend + deltaTime * 10.0f);

        if (m_parryWindowTimer <= 0.0f)
        {
            // パリィウィンドウ終了
            if (rmbDown)
            {
                // まだ押してる → ガードに移行
                m_shieldState = ShieldState::Guarding;
                m_isGuarding = true;
                //OutputDebugStringA("[SHIELD] State: GUARDING (hold)\n");
            }
            else
            {
                // もう離した → タップだった、Idleに戻る
                m_shieldState = ShieldState::Idle;
                //OutputDebugStringA("[SHIELD] State: IDLE (parry tap ended)\n");
            }
        }
        break;
    }

    // ─── ガード中（長押し）───
    case ShieldState::Guarding:
    {
        // 盾を表示し続ける
        m_shieldGuardBlend = min(1.0f, m_shieldGuardBlend + deltaTime * 10.0f);

        // 盾アニメを表示位置でキープ
        m_shieldBashTimer = m_shieldBashDuration * 0.5f;

        //  チャージエネルギー蓄積
        if (m_chargeEnergy < 1.0f)
        {
            m_chargeEnergy += m_chargeEnergyRate * deltaTime;
            if (m_chargeEnergy >= 1.0f)
            {
                m_chargeEnergy = 1.0f;
                m_chargeReady = true;
                //OutputDebugStringA("[SHIELD] Charge energy FULL! Ready to charge!\n");
            }
        }

        // === 毎フレーム: ベストターゲットを追跡 ===
        {
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            float camYaw = m_player->GetRotation().y;
            float camPitch = m_player->GetRotation().x;
        
            DirectX::XMFLOAT3 lookDir;
            lookDir.x = sinf(camYaw) * cosf(camPitch);
            lookDir.y = -sinf(camPitch);
            lookDir.z = cosf(camYaw) * cosf(camPitch);
        
            float bestScore = -1.0f;
            m_guardLockTargetID = -1;
        
            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
        
                float dx = enemy.position.x - playerPos.x;
                float dy = enemy.position.y - playerPos.y;
                float dz = enemy.position.z - playerPos.z;
                float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        
                if (dist < 1.0f || dist > 50.0f) continue;
        
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;
        
                float dot = nx * lookDir.x + ny * lookDir.y + nz * lookDir.z;
        
                if (dot > 0.7f)
                {
                    float score = dot / dist;
                    if (score > bestScore)
                    {
                        bestScore = score;
                        m_guardLockTargetID = enemy.id;
                        m_guardLockTargetPos = enemy.position;
                    }
                }
            }
        }
        
        bool lmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (lmbDown && m_chargeReady && m_guardLockTargetID >= 0)
        {
            // ロックオン成功 → チャージ開始！
            m_shieldState = ShieldState::Charging;
            m_chargeTimer = 0.0f;
            m_chargeEnergy = 0.0f;
            m_chargeReady = false;
            m_chargeHasTarget = true;
            m_chargeTargetEnemyID = m_guardLockTargetID;
            m_chargeTarget = m_guardLockTargetPos;
        
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            float dx = m_guardLockTargetPos.x - playerPos.x;
            float dz = m_guardLockTargetPos.z - playerPos.z;
            float len = sqrtf(dx * dx + dz * dz);
            if (len > 0.01f)
                m_chargeDirection = { dx / len, 0.0f, dz / len };
            else
            {
                float camYaw = m_player->GetRotation().y;
                m_chargeDirection = { sinf(camYaw), 0.0f, cosf(camYaw) };
            }
        
            float distToTarget = len;
            m_chargeDuration = min(0.8f, max(0.2f, distToTarget / m_chargeSpeed));
        }

        if (!rmbDown)
        {
            // 右クリック離す → Idleに戻る
            m_shieldState = ShieldState::Idle;
            m_isGuarding = false;
            m_guardLockTargetID = -1;
            //OutputDebugStringA("[SHIELD] State: IDLE (guard released)\n");
        }
        break;
    }

    // ??? シールドチャージ突進中 ???
    case ShieldState::Charging:
    {
        m_chargeTimer += deltaTime;

        //  FOV拡大＋スピードライン
        m_targetFOV = 100.0f;
        m_speedLineAlpha = 0.8f;

        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        playerPos.x += m_chargeDirection.x * m_chargeSpeed * deltaTime;
        playerPos.z += m_chargeDirection.z * m_chargeSpeed * deltaTime;
        m_player->SetPosition(playerPos);

        float dxTarget = m_chargeTarget.x - playerPos.x;
        float dzTarget = m_chargeTarget.z - playerPos.z;
        float distToTarget = sqrtf(dxTarget * dxTarget + dzTarget * dzTarget);

        bool reachedTarget = (distToTarget < 2.0f);  // within 2m of target
        bool timeUp = (m_chargeTimer >= m_chargeDuration);

        if (m_mapSystem && m_mapSystem->CheckCollision(playerPos, 0.5f))
        {
            reachedTarget = true; 
            //OutputDebugStringA("[CHARGE] Hit wall! Exploding here.\n");
        }

        if (reachedTarget || timeUp)
        {
            
            DirectX::XMFLOAT3 blastCenter = m_player->GetPosition();
            float blastRadius = 5.0f; 

            int killCount = 0;
            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;

                float dx = enemy.position.x - blastCenter.x;
                float dz = enemy.position.z - blastCenter.z;
                float dist = sqrtf(dx * dx + dz * dz);

                if (dist > blastRadius) continue;  

                if (enemy.type == EnemyType::NORMAL || enemy.type == EnemyType::RUNNER)
                {
                    enemy.health = 0;
                    enemy.isDying = true;
                    enemy.isAlive = false;
                    enemy.isRagdoll = true;
                    enemy.isExploded = true;
                    enemy.corpseTimer = 3.0f;

                    RemoveEnemyPhysicsBody(enemy.id);

                    // スクリーンブラッド
                    {
                        DirectX::XMFLOAT3 pPos = m_player->GetPosition();
                        float bx = enemy.position.x - pPos.x;
                        float bz = enemy.position.z - pPos.z;
                        float bDist = sqrtf(bx * bx + bz * bz);
                        if (bDist < 8.0f)
                        {
                            float intensity = 1.0f - (bDist / 8.0f);
                            m_bloodSystem->OnExplosionKill(enemy.position, m_player->GetPosition());
                            m_gpuParticles->EmitSplash(enemy.position, XMFLOAT3(enemy.position.x - m_player->GetPosition().x, enemy.position.y - m_player->GetPosition().y, enemy.position.z - m_player->GetPosition().z), 50, 9.0f);
                            m_gpuParticles->EmitMist(enemy.position, 150, 3.5f);
                            m_gpuParticles->Emit(enemy.position, 80, 7.0f);
                        }
                    }

                    int waveBonus = m_waveManager->OnEnemyKilled();
                    if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);
                    m_player->AddPoints(100 + waveBonus);
                    SpawnScorePopup(100 + waveBonus, ScorePopupType::MELEE_KILL);

                    //  肉片を生成（8個、爆発力15）
                    SpawnGibs(enemy.position, 12, 18.0f);

                  
                    DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                    m_particleSystem->CreateBloodEffect(enemy.position, upDir, 400);
                    m_particleSystem->CreateExplosion(enemy.position);

                   
                    for (int a = 0; a < 6; a++)
                    {
                        float rad = a * 1.047f; 
                        DirectX::XMFLOAT3 radDir = { cosf(rad), 0.3f, sinf(rad) };
                        m_particleSystem->CreateBloodEffect(enemy.position, radDir, 40);
                    }

                    killCount++;
                    m_statKills++;       //  総キル
                    m_statMeleeKills++;  //  近接キル
                }
                else
                {
                    // TANK / MIDBOSS / BOSS へのダメージ
                    enemy.health -= 80;
                    m_statDamageDealt += 80;

                    // 死ななくても血を出す
                    DirectX::XMFLOAT3 hitDir = {
                        enemy.position.x - blastCenter.x,
                        0.5f,  // やや上向き
                        enemy.position.z - blastCenter.z
                    };
                    m_gpuParticles->EmitSplash(enemy.position, hitDir, 40, 7.0f);
                    m_gpuParticles->EmitMist(enemy.position, 100, 2.5f);
                    m_gpuParticles->Emit(enemy.position, 30, 4.0f);

                    //  タンクだけ確定よろめき
                    if (enemy.type == EnemyType::TANK && !enemy.isStaggered)
                    {
                        enemy.isStaggered = true;
                        enemy.staggerFlashTimer = 0.0f;
                        enemy.staggerTimer = 0.0f;
                        enemy.stunValue = 0.0f;
                        enemy.currentAnimation = "Stun";
                        enemy.animationTime = 0.0f;
                    }
                    else
                    {
                        // MIDBOSS/BOSS はスタン蓄積のみ
                        enemy.stunValue += 60.0f;
                        if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                        {
                            enemy.isStaggered = true;
                            enemy.staggerFlashTimer = 0.0f;
                            enemy.staggerTimer = 0.0f;
                            enemy.stunValue = 0.0f;
                            enemy.currentAnimation = "Stun";
                            enemy.animationTime = 0.0f;
                        }
                    }

                    if (enemy.health <= 0)
                    {
                        enemy.health = 0;
                        enemy.isDying = true;
                        enemy.isAlive = false;
                        enemy.isRagdoll = true;
                        enemy.corpseTimer = 3.0f;
                        RemoveEnemyPhysicsBody(enemy.id);
                        m_bloodSystem->OnMeleeKill(enemy.position);
                        m_gpuParticles->EmitSplash(enemy.position,
                            XMFLOAT3(enemy.position.x - m_player->GetPosition().x,
                                enemy.position.y - m_player->GetPosition().y,
                                enemy.position.z - m_player->GetPosition().z), 45, 8.0f);
                        m_gpuParticles->EmitMist(enemy.position, 120, 2.5f);
                        m_gpuParticles->Emit(enemy.position, 70, 6.0f);
                        m_statKills++;
                        m_statMeleeKills++;

                        int waveBonus = m_waveManager->OnEnemyKilled();
                        m_player->AddPoints(150 + waveBonus);
                    }
                }
            }

            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
            //m_particleSystem->CreateBloodEffect(blastCenter, upDir, 300);
            //m_particleSystem->CreateExplosion(blastCenter);

            /*char buf[128];
            sprintf_s(buf, "[CHARGE] AOE EXPLOSION! %d grunts obliterated!\n", killCount);*/
            //OutputDebugStringA(buf);

            // Camera shake (kills数に応じて強さ変化)
            m_cameraShake = 0.3f + killCount * 0.15f;      // 1体:0.45  3体:0.75  5体:1.05
            m_cameraShakeTimer = 0.3f;

            // Slow-mo (短めでキレのある演出)
            if (killCount > 0)
            {
                m_timeScale = 0.15f;
                m_slowMoTimer = 0.02f + killCount * 0.02f;

                m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetMaxAmmo());
                int curReserve = m_weaponSystem->GetReserveAmmo();
                int maxReserve = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon()).reserveAmmo;
                int newReserve = curReserve + killCount * 10;
                if (newReserve > maxReserve) newReserve = maxReserve;
                m_weaponSystem->SetReserveAmmo(newReserve);
            }

            //  FOVを戻す
            m_targetFOV = 70.0f;
            m_speedLineAlpha = 0.0f;

            m_shieldState = ShieldState::Guarding;
            m_isGuarding = true;
            m_chargeHasTarget = false;
            m_chargeTargetEnemyID = -1;
        }

        m_shieldGuardBlend = 1.0f;
        break;
    }

    case ShieldState::Throwing:
    {
        //  ガード不可（盾が手元にない）
        m_shieldGuardBlend = max(0.0f, m_shieldGuardBlend - deltaTime * 10.0f);

        // 回転（見た目のスピン）
        m_thrownShieldSpin += deltaTime * 20.0f;


        if (!m_thrownShieldReturning)
        {
            // === 前進（直線） ===
            float move = m_thrownShieldSpeed * deltaTime;
            m_thrownShieldPos.x += m_thrownShieldDir.x * move;
            m_thrownShieldPos.y += m_thrownShieldDir.y * move;
            m_thrownShieldPos.z += m_thrownShieldDir.z * move;
            m_thrownShieldDist += move;

            // 最大距離で折り返し
            if (m_thrownShieldDist >= m_thrownShieldMaxDist)
            {
                m_thrownShieldReturning = true;
                //OutputDebugStringA("[SHIELD] Max distance, returning!\n");
            }
        }
        else
        {
            // === 戻り（プレイヤーに向かう） ===
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            float dx = playerPos.x - m_thrownShieldPos.x;
            float dy = playerPos.y - m_thrownShieldPos.y;
            float dz = playerPos.z - m_thrownShieldPos.z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);

            if (dist < 2.0f)
            {
                // プレイヤーに到達 → 盾復活！
                m_shieldState = ShieldState::Idle;

                //  Effekseerトレイル停止
                if (m_shieldTrailHandle >= 0)
                {
                    m_effekseerManager->StopEffect(m_shieldTrailHandle);
                    m_shieldTrailHandle = -1;
                }

                //OutputDebugStringA("[SHIELD] Shield caught! Back to Idle.\n");
                break;
            }

            // 方向を正規化
            float invDist = 1.0f / dist;
            dx *= invDist;
            dy *= invDist;
            dz *= invDist;

            // 戻り速度は少し速く（50）
            float returnSpeed = 50.0f * deltaTime;
            m_thrownShieldPos.x += dx * returnSpeed;
            m_thrownShieldPos.y += dy * returnSpeed;
            m_thrownShieldPos.z += dz * returnSpeed;
        }

        //  Effekseer: SetBaseMatrixで盾に追従
        if (m_shieldTrailHandle >= 0 && m_effekseerManager != nullptr)
        {
            m_effekseerManager->SetLocation(m_shieldTrailHandle,
                Effekseer::Vector3D(
                    m_thrownShieldPos.x,
                    m_thrownShieldPos.y,
                    m_thrownShieldPos.z));
        }

        

        // === 敵との当たり判定 ===
        for (auto& enemy : m_enemySystem->GetEnemies())
        {
            if (!enemy.isAlive || enemy.isDying) continue;

            float ex = enemy.position.x - m_thrownShieldPos.x;
            float ey = (enemy.position.y + 0.8f) - m_thrownShieldPos.y;
            float ez = enemy.position.z - m_thrownShieldPos.z;
            float distSq = ex * ex + ey * ey + ez * ez;
            float hitR = m_thrownShieldHitRadius;

            if (distSq < hitR * hitR)
            {
                // 重複チェック（行きと帰りで別管理）
                auto& hitSet = m_thrownShieldReturning
                    ? m_thrownShieldReturnHitEnemies
                    : m_thrownShieldHitEnemies;

                if (hitSet.find(enemy.id) == hitSet.end())
                {
                    hitSet.insert(enemy.id);

                    // ダメージ
                    enemy.health -= (int)m_thrownShieldDamage;
                    enemy.staggerFlashTimer = 0.15f;

                    //  死亡チェック
                    if (enemy.health <= 0)
                    {
                        enemy.health = 0;
                        enemy.isDying = true;
                        enemy.corpseTimer = 3.0f;

                        // === メッシュ切断 ===
                        SliceEnemyWithShield(enemy, m_thrownShieldDir);

                        // 切断後に物理ボディ削除＆isAlive=false
                        enemy.isAlive = false;
                        RemoveEnemyPhysicsBody(enemy.id);
                        int waveBonus = m_waveManager->OnEnemyKilled();
                        if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);
                        m_player->AddPoints(100 + waveBonus);
                        SpawnScorePopup(100 + waveBonus, ScorePopupType::SHIELD_KILL);
                    }

                    // スタン
                    enemy.stunValue = enemy.maxStunValue;

                    // 血エフェクト
                    DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                    //m_particleSystem->CreateBloodEffect(enemy.position, upDir, 80);
                    //  盾投げヒット血エフェクト（GPU）
                    {
                        // 盾の飛行方向を血の飛散方向に使う
                        DirectX::XMFLOAT3 splashDir = m_thrownShieldDir;
                        splashDir.y += 0.3f;  // 少し上向きに飛ばす

                        // 敵の位置（少し上＝胴体）からスプラッシュ
                        DirectX::XMFLOAT3 hitPos = enemy.position;
                        hitPos.y += 0.8f;

                        m_gpuParticles->EmitSplash(hitPos, splashDir, 25, 6.0f);
                        m_gpuParticles->EmitMist(hitPos, 40, 1.5f);
                    }

                    // カメラシェイク（軽め）
                    m_cameraShake = 0.15f;
                    m_cameraShakeTimer = 0.1f;

                    // ヒットストップ（短め）
                    m_hitStopTimer = 0.03f;

                    /*char msg[128];
                    sprintf_s(msg, "[SHIELD THROW] Hit enemy ID:%d! HP:%d\n",
                        enemy.id, enemy.health);*/
                    //OutputDebugStringA(msg);
                }
            }
        }

        break;
    }

    // ─── 盾破壊（一時使用不能）───
    case ShieldState::Broken:
    {
        m_shieldGuardBlend = max(0.0f, m_shieldGuardBlend - deltaTime * 3.0f);
        m_isGuarding = false;
        m_shieldBrokenTimer -= deltaTime;

        if (m_shieldBrokenTimer <= 0.0f)
        {
            // 復帰！
            m_shieldState = ShieldState::Idle;
            m_shieldHP = m_shieldMaxHP * 0.3f;  // 30%で復帰
            //OutputDebugStringA("[SHIELD] State: IDLE (shield restored!)\n");
        }
        break;
    }
    }

    rmbPressed = rmbDown;

    // === シールドHUD アニメーション更新 ===
    {
        // 表示HPを実際のHPに滑らかに追従させる
        // 減る時はゆっくり（ダメージ演出）、回復時は速い
        float lerpSpeed = (m_shieldDisplayHP > m_shieldHP) ? 3.0f : 8.0f;
        m_shieldDisplayHP += (m_shieldHP - m_shieldDisplayHP) * min(1.0f, lerpSpeed * deltaTime);

        // グロウ強度の更新（ガード/パリィで光る）
        float targetGlow = 0.0f;
        if (m_shieldState == ShieldState::Guarding)
            targetGlow = 0.7f;
        else if (m_shieldState == ShieldState::Parrying)
            targetGlow = 1.0f;
        m_shieldGlowIntensity += (targetGlow - m_shieldGlowIntensity) * min(1.0f, 12.0f * deltaTime);
    }

    // GPU血パーティクルに地面の高さを伝える
    {
        DirectX::XMFLOAT3 pPos = m_player->GetPosition();
        float groundY = GetMeshFloorHeight(pPos.x, pPos.z, 0.0f);
        m_gpuParticles->SetFloorY(groundY + 0.02f);  // 地面のちょっと上
    }

    m_gpuParticles->Update(deltaTime);

    UpdateSlicedPieces(deltaTime);

    //  回復アイテム更新
    UpdateHealthPickups(deltaTime);
    UpdateScorePopups(deltaTime);


    // パリィ成功エフェクトタイマー
    if (m_parryFlashTimer > 0.0f)
    {
        m_parryFlashTimer -= deltaTime;
    }

    // 盾バッシュタイマー（Idle時のみ減少、Guarding時はキープ）
    if (m_shieldState != ShieldState::Guarding && m_shieldState != ShieldState::Charging && m_shieldBashTimer > 0.0f)
    {
        m_shieldBashTimer -= deltaTime;
        if (m_shieldBashTimer < 0.0f)
            m_shieldBashTimer = 0.0f;
    }

    // === 接触ダメージ判定 ===
    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying)
            continue;

        if ((enemy.type == EnemyType::MIDBOSS) &&
            enemy.bossPhase != BossAttackPhase::ROAR_FIRE &&
            m_beamHandle >= 0 && m_effekseerManager != nullptr)
        {
            m_effekseerManager->StopEffect(m_beamHandle);
            m_beamHandle = -1;
        }

        //  === ボス特殊攻撃の処理 ===
        if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS) &&
            enemy.bossPhase != BossAttackPhase::IDLE &&
            enemy.attackJustLanded)
        {
            // --- ジャンプ叩きつけ：範囲ダメージ ---
            if (enemy.bossPhase == BossAttackPhase::JUMP_SLAM)
            {
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                float dx = playerPos.x - enemy.position.x;
                float dz = playerPos.z - enemy.position.z;
                float dist = sqrtf(dx * dx + dz * dz);

                float slamRadius = (enemy.type == EnemyType::BOSS) ? m_slamRadiusBoss : m_slamRadiusMidBoss;
                float slamDamage = ((enemy.type == EnemyType::BOSS) ? m_slamDamageBoss : m_slamDamageMidBoss) * enemy.damageMultiplier;  //  ウェーブ倍率

                if (dist < slamRadius)
                {
                    // パリィ/ガード判定
                    if (m_shieldState == ShieldState::Parrying)
                    {
                        // ジャンプ叩きはパリィ可能
                        m_shieldState = ShieldState::Idle;
                        m_parrySuccess = true;
                        // パリィ時ヒットストップ
                        m_hitStopTimer = 0.06f;
                        m_timeScale = 0.0f;
                        m_slowMoTimer = 0.0f;   // 残留slowMo防止
                        
                        // パリィ成功→近接チャージ回復
                        if (m_meleeCharges < m_meleeMaxCharges)
                            m_meleeCharges++;

                        m_lastParryResultTime = m_gameTime;
                        m_lastParryWasSuccess = true;
                        m_parrySuccessCount++;
                        m_player->AddPoints(150);
                        SpawnScorePopup(150, ScorePopupType::PARRY);
                        m_styleRank->NotifyParry();
                        m_parryFlashTimer = 0.3f;

                         // パリィエフェクト再生
                        if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                        {
                            DirectX::XMFLOAT3 pos = m_player->GetPosition();
                            DirectX::XMFLOAT3 rot = m_player->GetRotation();
                            float fx = pos.x + sinf(rot.y) * 1.0f;
                            float fz = pos.z + cosf(rot.y) * 1.0f;
                            auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                            m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                        }

                        float stunDamage = m_slamStunDamage;
                        enemy.stunValue += stunDamage;
                        if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                        {
                            enemy.isStaggered = true;
                            enemy.staggerFlashTimer = 0.0f;
                            enemy.staggerTimer = 0.0f;
                            enemy.stunValue = 0.0f;
                            enemy.currentAnimation = "Stun";
                            enemy.animationTime = 0.0f;
                        }

                        m_cameraShake = 0.2f;
                        m_cameraShakeTimer = 0.3f;
                        m_hitStopTimer = 0.1f;
                        m_timeScale = 0.2f;
                        m_slowMoTimer = 0.4f;

                        //OutputDebugStringA("[BOSS] JUMP SLAM PARRIED!\n");
                    }
                    else if (m_shieldState == ShieldState::Guarding)
                    {
                        // ガード：HPダメージなし、盾HPのみ消費
                        m_shieldHP -= slamDamage * 0.5f;
                        m_shieldRegenDelayTimer = m_shieldRegenDelay;
                        m_cameraShake = 0.1f;
                        m_cameraShakeTimer = 0.15f;

                        if (m_shieldHP <= 0.0f)
                        {
                            m_shieldHP = 0.0f;
                            m_shieldState = ShieldState::Broken;
                            m_shieldBrokenTimer = m_shieldBrokenDuration;
                        }
                    }
                    else
                    {
                        // ノーガード：フルダメージ
                        bool died = m_player->TakeDamage((int)slamDamage);
                        m_statDamageTaken += (int)slamDamage;  //  被ダメ記録
                        m_cameraShake = 0.3f;
                        m_cameraShakeTimer = 0.3f;
                        if (died) m_gameState = GameState::GAMEOVER;

                        //OutputDebugStringA("[BOSS] JUMP SLAM HIT! (no guard)\n");
                    }
                }

                // 叩きつけで画面揺れ（距離に関係なく）
                m_cameraShake = (std::max)(m_cameraShake, 0.08f);
                m_cameraShakeTimer = (std::max)(m_cameraShakeTimer, 0.2f);

                // 地面エフェクトはここで後で追加（Effekseer）
                if (m_particleSystem)
                {
                    //m_particleSystem->CreateExplosion(enemy.position);
                }

                //  衝撃波エフェクト追加
                if (m_effectGroundSlam != nullptr && m_effekseerManager != nullptr)
                {
                    m_effekseerManager->Play(
                        m_effectGroundSlam,
                        enemy.position.x, enemy.position.y + 0.1f, enemy.position.z);
                }

                enemy.attackJustLanded = false;
                continue;  // このenemyの処理は終わり
            }

            // --- 斬撃3本発射（BOSS） ---
            if (enemy.bossPhase == BossAttackPhase::SLASH_FIRE)
            {
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

                // 敵→プレイヤーの方向
                float dx = playerPos.x - enemy.position.x;
                float dz = playerPos.z - enemy.position.z;
                float dist = sqrtf(dx * dx + dz * dz);
                if (dist < 0.1f) dist = 0.1f;
                float nx = dx / dist;
                float nz = dz / dist;

                // 3本の斬撃を扇状に発射
                // 角度オフセット: -15度, 0度, +15度
                float angles[5] = { -0.70f, -0.35f, 0.0f, 0.35f, 0.70f };  // ラジアン（約15度）

                for (int i = 0; i < 5; i++)
                {
                    BossProjectile proj;
                    // 方向を回転
                    float cosA = cosf(angles[i]);
                    float sinA = sinf(angles[i]);
                    proj.direction.x = nx * cosA - nz * sinA;
                    proj.direction.y = 0.0f;
                    proj.direction.z = nx * sinA + nz * cosA;

                    // 発射位置（ボスの前方1m）
                    proj.position.x = enemy.position.x + proj.direction.x * 1.0f;
                    proj.position.y = enemy.position.y + 1.0f;
                    proj.position.z = enemy.position.z + proj.direction.z * 1.0f;

                    proj.speed = m_slashSpeed;
                    proj.lifetime = 3.0f;
                    proj.damage = m_slashDamage * enemy.damageMultiplier;
                    proj.ownerID = enemy.id;
                    proj.isActive = true;

                    // 真ん中の1本だけパリィ可能（緑）
                    proj.isParriable = (rand() % 2 == 0);

                    // エフェクト再生＆ハンドル保存
                    Effekseer::EffectRef slashEffect = proj.isParriable
                        ? m_effectSlashGreen : m_effectSlashRed;
                    if (slashEffect != nullptr && m_effekseerManager != nullptr)
                    {
                        Effekseer::Handle h = m_effekseerManager->Play(
                            slashEffect,
                            proj.position.x, proj.position.y, proj.position.z);
                        float angle = atan2f(proj.direction.x, proj.direction.z);
                        m_effekseerManager->SetRotation(h, 0.0f, angle, 0.0f);
                        m_effekseerManager->SetScale(h, 0.6f, 0.3f, 0.3f);
                        proj.effectHandle = h;  
                    }

                    m_bossProjectiles.push_back(proj);

                   /* char buf[128];
                    sprintf_s(buf, "[BOSS] Slash projectile %d fired! Parriable: %s\n",
                        i, proj.isParriable ? "YES(green)" : "NO(red)");*/
                    //OutputDebugStringA(buf);
                }

                enemy.attackJustLanded = false;
                continue;
            }

            // --- 咆哮ビーム（MIDBOSS） ---
            if (enemy.bossPhase == BossAttackPhase::ROAR_FIRE)
            {
                // ビームエフェクト再生（初回のみ
                Effekseer::EffectRef beamEffect = enemy.bossBeamParriable
                    ? m_effectBeamGreen : m_effectBeamRed;
                if (beamEffect != nullptr && m_beamHandle < 0 && m_effekseerManager != nullptr)
                {
                    m_beamHandle = m_effekseerManager->Play(
                        beamEffect,
                        enemy.position.x, enemy.position.y + 1.5f, enemy.position.z);
                    m_effekseerManager->SetScale(m_beamHandle, 0.5f, 0.5f, 0.5f);
                }

                // ビームの位置と向きを毎フレーム更新
                if (m_beamHandle >= 0 && m_effekseerManager != nullptr)
                {
                    m_effekseerManager->SetLocation(m_beamHandle,
                        Effekseer::Vector3D(enemy.position.x, enemy.position.y + 1.5f, enemy.position.z));
                    float angle = atan2f(
                        enemy.bossTargetPos.x - enemy.position.x,
                        enemy.bossTargetPos.z - enemy.position.z) + 3.14159f;
                    m_effekseerManager->SetRotation(m_beamHandle, 0.0f, angle, 0.0f);
                }


                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

                //  ビーム到達遅延（発射から3.5秒後にプレイヤーに届く）
                float beamArrivalTime = 2.0f;  // ROAR_FIRE開始からの到達時間
                bool beamReached = (enemy.bossPhaseTimer >= beamArrivalTime);

                // ビームの方向計算（エフェクト表示用、到達前も必要）
                float bdx = enemy.bossTargetPos.x - enemy.position.x;
                float bdz = enemy.bossTargetPos.z - enemy.position.z;
                float bDist = sqrtf(bdx * bdx + bdz * bdz);
                if (bDist < 0.1f) bDist = 0.1f;
                float bnx = bdx / bDist;
                float bnz = bdz / bDist;

                float px = playerPos.x - enemy.position.x;
                float pz = playerPos.z - enemy.position.z;

                float projDist = px * bnx + pz * bnz;
                float perpDist = fabs(px * (-bnz) + pz * bnx);

                float beamWidth = m_beamWidth;
                float beamLength = m_beamLength;
                float beamDPS = m_beamDPS;

                // // ビーム到達後のみダメージ判定
                if (beamReached && projDist > 0.0f && projDist < beamLength && perpDist < beamWidth)
                {
                    float frameDamage = beamDPS * (1.0f / 60.0f) * enemy.damageMultiplier;  //  ウェーブ倍率

                    if (m_shieldState == ShieldState::Parrying && enemy.bossBeamParriable)
                    {
                        // 緑ビーム → パリィ成功！
                        m_shieldState = ShieldState::Idle;
                        m_parrySuccess = true;
                        // パリィ時ヒットストップ
                        m_hitStopTimer = 0.06f;
                        m_timeScale = 0.0f;
                        m_slowMoTimer = 0.0f;   // 残留slowMo防止

                        // パリィ成功→近接チャージ回復
                        if (m_meleeCharges < m_meleeMaxCharges)
                            m_meleeCharges++;

                        m_parryFlashTimer = 0.3f;
                        m_player->AddPoints(150);
                        SpawnScorePopup(150, ScorePopupType::PARRY);
                        m_styleRank->NotifyParry();

                        // パリィエフェクト再生
                        if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                        {
                            DirectX::XMFLOAT3 pos = m_player->GetPosition();
                            DirectX::XMFLOAT3 rot = m_player->GetRotation();
                            float fx = pos.x + sinf(rot.y) * 1.0f;
                            float fz = pos.z + cosf(rot.y) * 1.0f;
                            auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                            m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                        }

                        enemy.stunValue += m_beamStunOnParry * 2.0f;
                        if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                        {
                            enemy.isStaggered = true;
                            enemy.staggerFlashTimer = 0.0f;
                            enemy.staggerTimer = 0.0f;
                            enemy.stunValue = 0.0f;
                            enemy.currentAnimation = "Stun";
                            enemy.animationTime = 0.0f;
                        }

                        // パリィ成功でビーム中断！
                        enemy.bossPhase = BossAttackPhase::ROAR_RECOVERY;
                        enemy.bossPhaseTimer = 0.0f;

                        m_cameraShake = 0.25f;
                        m_cameraShakeTimer = 0.3f;
                        m_hitStopTimer = 0.15f;
                        m_timeScale = 0.15f;
                        m_slowMoTimer = 0.5f;

                        if (m_beamHandle >= 0 && m_effekseerManager != nullptr)
                        {
                            m_effekseerManager->StopEffect(m_beamHandle);
                            m_beamHandle = -1;
                        }

                        //OutputDebugStringA("[MIDBOSS] BEAM PARRIED!\n");
                    }
                    else if (m_shieldState == ShieldState::Parrying && !enemy.bossBeamParriable)
                    {
                        // 赤ビーム：パリィ不可→ガード扱い、HPダメージなし
                        m_shieldHP -= frameDamage * 0.3f;
                        m_shieldRegenDelayTimer = m_shieldRegenDelay;
                    }
                    else if (m_shieldState == ShieldState::Guarding)
                    {
                        // ガード：HPダメージなし、盾HPのみ消費
                        m_shieldHP -= frameDamage * 0.3f;
                        m_shieldRegenDelayTimer = m_shieldRegenDelay;

                        if (m_shieldHP <= 0.0f)
                        {
                            m_shieldHP = 0.0f;
                            m_shieldState = ShieldState::Broken;
                            m_shieldBrokenTimer = m_shieldBrokenDuration;
                        }
                    }
                    else
                    {
                        m_statDamageTaken += (int)(std::max)(1.0f, frameDamage);  //被ダメ
                        bool died = m_player->TakeDamage((int)(std::max)(1.0f, frameDamage));
                        if (died) m_gameState = GameState::GAMEOVER;
                    }
                }

                continue;
            }

            // 上記以外のボス攻撃フェーズは通常のattackJustLandedにフォールスルー
            enemy.attackJustLanded = false;
            continue;
        }

        // 攻撃が「今まさに当たった瞬間」のみ判定
        if (enemy.attackJustLanded)
        {
            if (m_gloryKillInvincibleTimer > 0)
                break;

            // パリィウィンドウ中 → パリィ成功！
            if (m_shieldState == ShieldState::Parrying)
            {
                // パリィ成功 → シールドHP消費なし
                m_shieldState = ShieldState::Idle;
                m_parrySuccess = true;
                // パリィ時ヒットストップ
                m_hitStopTimer = 0.06f;
                m_timeScale = 0.0f;
                m_slowMoTimer = 0.0f;   // 残留slowMo防止

                // パリィ成功→近接チャージ回復
                if (m_meleeCharges < m_meleeMaxCharges)
                    m_meleeCharges++;

                m_lastParryResultTime = m_gameTime;
                m_lastParryWasSuccess = true;
                m_parrySuccessCount++;
                m_player->AddPoints(75);
                SpawnScorePopup(75, ScorePopupType::PARRY);
                m_styleRank->NotifyParry();
                m_parryFlashTimer = 0.3f;

                // パリィエフェクト再生
                if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                {
                    DirectX::XMFLOAT3 pos = m_player->GetPosition();
                    DirectX::XMFLOAT3 rot = m_player->GetRotation();
                    float fx = pos.x + sinf(rot.y) * 1.0f;
                    float fz = pos.z + cosf(rot.y) * 1.0f;
                    auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                    m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                }

                if (enemy.type == EnemyType::NORMAL || enemy.type == EnemyType::RUNNER)
                {
                    enemy.isStaggered = true;
                    enemy.staggerFlashTimer = 0.0f;
                    enemy.staggerTimer = 0.0f;
                    enemy.currentAnimation = "Stun";
                    enemy.animationTime = 0.0f;

                   /* char buf[128];
                    sprintf_s(buf, "[PARRY] SUCCESS! Enemy ID:%d STAGGERED instantly!\n", enemy.id);*/
                    //OutputDebugStringA(buf);
                }
                else
                {
                    float stunDamage = 40.0f;
                    enemy.stunValue += stunDamage;

                    /*char buf[256];
                    sprintf_s(buf, "[PARRY] SUCCESS! Enemy ID:%d Stun: %.0f/%.0f\n",
                        enemy.id, enemy.stunValue, enemy.maxStunValue);*/
                    //OutputDebugStringA(buf);

                    if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                    {
                        enemy.isStaggered = true;
                        enemy.staggerFlashTimer = 0.0f;
                        enemy.staggerTimer = 0.0f;
                        enemy.stunValue = 0.0f;
                        enemy.currentAnimation = "Stun";
                        enemy.animationTime = 0.0f;

                        /*sprintf_s(buf, "[PARRY] STUN BREAK! Enemy ID:%d is now STAGGERED!\n", enemy.id);*/
                        //OutputDebugStringA(buf);
                    }
                }

                // フィードバック
                m_cameraShake = 0.15f;
                m_cameraShakeTimer = 0.2f;
                m_hitStopTimer = 0.08f;
                m_timeScale = 0.3f;
                m_slowMoTimer = 0.3f;

                if (m_particleSystem)
                {
                    XMFLOAT3 sparkDir(0.0f, 1.0f, 0.0f);
                    /*m_particleSystem->CreateBloodEffect(
                        enemy.position, sparkDir, 30);*/
                }

                //OutputDebugStringA("[PARRY] === TIMING PARRY! ===\n");
                break;
            }
            else if (m_shieldState == ShieldState::Guarding)
            {
                float rawDamage;
                switch (enemy.type)
                {
                case EnemyType::RUNNER: rawDamage = 8.0f;  break;
                case EnemyType::TANK:   rawDamage = 25.0f; break;
                default:                rawDamage = 10.0f; break;
                }
                rawDamage *= enemy.damageMultiplier;

                // // HPダメージなし！盾HPのみ消費
                m_shieldHP -= rawDamage * 0.5f;
                m_shieldRegenDelayTimer = m_shieldRegenDelay;

                m_cameraShake = 0.05f;
                m_cameraShakeTimer = 0.1f;

                if (m_shieldHP <= 0.0f)
                {
                    m_shieldHP = 0.0f;
                    m_shieldState = ShieldState::Broken;
                    m_shieldBrokenTimer = m_shieldBrokenDuration;
                    m_isGuarding = false;
                }
                break;
            }
            else
            {
                
                float rawDamage;
                switch (enemy.type)
                {
                case EnemyType::RUNNER: rawDamage = 8.0f;  break;
                case EnemyType::TANK:   rawDamage = 25.0f; break;
                default:                rawDamage = 10.0f; break;
                }
                rawDamage *= enemy.damageMultiplier;  //  ウェーブ倍率

                bool died = m_player->TakeDamage((int)rawDamage);
                m_statDamageTaken += (int)rawDamage;  //  被ダメ記録

                m_cameraShake = 0.1f;
                m_cameraShakeTimer = 0.15f;

                //OutputDebugStringA("[ENEMY] Hit player! No guard - full damage!\n");

                if (died) m_gameState = GameState::GAMEOVER;
                break;
            }

            enemy.attackJustLanded = false;
        }
        // 継続ダメージ（密着し続けてる場合、一定間隔でダメージ）
        else if (enemy.touchingPlayer && m_shieldState == ShieldState::Idle)
        {
            if (m_gloryKillInvincibleTimer <= 0)
            {
                
            }
        }
    }


        // ================================================== =
        // ボスプロジェクタイル更新 & 当たり判定
        // ===================================================
    {
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

        for (auto& proj : m_bossProjectiles)
        {
            if (!proj.isActive) continue;

            // --- 移動 ---
            proj.position.x += proj.direction.x * proj.speed * deltaTime;
            proj.position.z += proj.direction.z * proj.speed * deltaTime;
            
            //  エフェクト追従
            if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
            {
                m_effekseerManager->SetLocation(proj.effectHandle,
                    Effekseer::Vector3D(proj.position.x, proj.position.y, proj.position.z));
            }

            // --- 寿命 ---
            proj.lifetime -= deltaTime;
            if (proj.lifetime <= 0.0f)
            {
                // エフェクト停止
                if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                {
                    m_effekseerManager->StopEffect(proj.effectHandle);
                    proj.effectHandle = -1;
                }

                proj.isActive = false;
                continue;
            }

            // --- プレイヤーとの当たり判定 ---
            float dx = playerPos.x - proj.position.x;
            float dz = playerPos.z - proj.position.z;
            float dist = sqrtf(dx * dx + dz * dz);

            float hitRadius = m_slashHitRadius;  // 斬撃の当たり判定の幅

            if (dist < hitRadius)
            {
                // ========================================
                // パリィ可能（緑）＋ パリィ状態 → パリィ成功！
                // ========================================
                if (proj.isParriable && m_shieldState == ShieldState::Parrying)
                {
                    m_shieldState = ShieldState::Idle;
                    m_parrySuccess = true;
                    // パリィ時ヒットストップ
                    m_hitStopTimer = 0.06f;
                    m_timeScale = 0.0f;
                    m_slowMoTimer = 0.0f;   // 残留slowMo防止

                    // パリィ成功→近接チャージ回復
                    if (m_meleeCharges < m_meleeMaxCharges)
                        m_meleeCharges++;

                    m_lastParryResultTime = m_gameTime;
                    m_lastParryWasSuccess = true;
                    m_parrySuccessCount++;

                    // パリィエフェクト再生
                    if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                    {
                        DirectX::XMFLOAT3 pos = m_player->GetPosition();
                        DirectX::XMFLOAT3 rot = m_player->GetRotation();
                        float fx = pos.x + sinf(rot.y) * 1.0f;
                        float fz = pos.z + cosf(rot.y) * 1.0f;
                        auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                        m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                    }

                    m_parryFlashTimer = 0.3f;

                    // 発射元のボスにスタンダメージ
                    for (auto& enemy : m_enemySystem->GetEnemies())
                    {
                        if (enemy.id == proj.ownerID && enemy.isAlive)
                        {
                            enemy.stunValue += m_slashStunOnParry;
                            if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                            {
                                enemy.isStaggered = true;
                                enemy.staggerFlashTimer = 0.0f;
                                enemy.staggerTimer = 0.0f;
                                enemy.stunValue = 0.0f;
                                enemy.currentAnimation = "Stun";
                                enemy.animationTime = 0.0f;
                                //OutputDebugStringA("[PROJECTILE] PARRY -> BOSS STAGGERED!\n");
                            }
                            break;
                        }
                    }

                    m_cameraShake = 0.2f;
                    m_cameraShakeTimer = 0.25f;
                    m_hitStopTimer = 0.1f;
                    m_timeScale = 0.2f;
                    m_slowMoTimer = 0.4f;
                    m_player->AddPoints(150);
                    SpawnScorePopup(150, ScorePopupType::PARRY);
                    m_styleRank->NotifyParry();

                    // エフェクト停止
                    if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                    {
                        m_effekseerManager->StopEffect(proj.effectHandle);
                        proj.effectHandle = -1;
                    }

                    proj.isActive = false;
                    //OutputDebugStringA("[PROJECTILE] GREEN SLASH PARRIED!\n");
                }
                // ========================================
                // ガード中 → ダメージ軽減（赤もガード可能）
                // ========================================
                else if (m_shieldState == ShieldState::Guarding)
                {
                    float reduced = proj.damage * (1.0f - m_guardDamageReduction);
                    m_statDamageTaken += (int)reduced;  //  被ダメ記録
                    bool died = m_player->TakeDamage((int)reduced);
                    m_shieldHP -= proj.damage * 0.4f;
                    m_shieldRegenDelayTimer = m_shieldRegenDelay;

                    m_cameraShake = 0.08f;
                    m_cameraShakeTimer = 0.1f;

                    if (m_shieldHP <= 0.0f)
                    {
                        m_shieldHP = 0.0f;
                        m_shieldState = ShieldState::Broken;
                        m_shieldBrokenTimer = m_shieldBrokenDuration;
                    }
                    if (died) m_gameState = GameState::GAMEOVER;

                    // エフェクト停止
                    if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                    {
                        m_effekseerManager->StopEffect(proj.effectHandle);
                        proj.effectHandle = -1;
                    }

                    proj.isActive = false;

                   /* char buf[64];
                    sprintf_s(buf, "[PROJECTILE] %s SLASH GUARDED!\n",
                        proj.isParriable ? "GREEN" : "RED");*/
                    //OutputDebugStringA(buf);
                }
                // ========================================
                // ノーガード → フルダメージ
                // ========================================
                else
                {
                    m_statDamageTaken += (int)proj.damage;  //  被ダメ記録
                    bool died = m_player->TakeDamage((int)proj.damage);
                    m_cameraShake = 0.15f;
                    m_cameraShakeTimer = 0.2f;
                    if (died) m_gameState = GameState::GAMEOVER;

                    // エフェクト停止
                    if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                    {
                        m_effekseerManager->StopEffect(proj.effectHandle);
                        proj.effectHandle = -1;
                    }

                    proj.isActive = false;

                    /*char buf[64];
                    sprintf_s(buf, "[PROJECTILE] %s SLASH HIT! (no guard)\n",
                        proj.isParriable ? "GREEN" : "RED");*/
                    //OutputDebugStringA(buf);
                }
            }
        }

        // --- 消えたプロジェクタイルを削除 ---
        m_bossProjectiles.erase(
            std::remove_if(m_bossProjectiles.begin(), m_bossProjectiles.end(),
                [](const BossProjectile& p) { return !p.isActive; }),
            m_bossProjectiles.end()
        );
    }


    //  Effekseer更新
    if (m_effekseerManager != nullptr)
    {
        m_effekseerManager->Update(1.0f);
    }

    auto& enemies = m_enemySystem->GetEnemies();
enemies.erase(
    std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e) { return !e.isAlive && !e.isDying; }),
    enemies.end()
);

}

void Game::ResetGame()
{
    m_showTutorial = true;

    // === プレイヤー ===
    m_player->SetHealth(100);
    m_healthPickups.clear();     //  アイテムもリセット
    m_rankingSaved = false;
    m_nameInputActive = false;
    m_nameLength = 0;
    m_nameKeyWasDown = false;
    m_lastWaveNumber = 0;
    m_waveBannerTimer = 0.0f;
    m_pickupSpawnTimer = 0.0f;   // 
    m_clawTimer = 0.0f;
    m_scorePopups.clear();
    {
        float spawnX = 0.0f;
        float spawnZ = -0.5f;
        float groundY = GetMeshFloorHeight(spawnX, spawnZ, 0.0f);
        m_player->SetPosition(DirectX::XMFLOAT3(spawnX, groundY + 1.8f, spawnZ));

        char buf[256];
        sprintf_s(buf, "[SPAWN] Player at (%.2f, %.2f, %.2f) groundY=%.2f\n",
            spawnX, groundY + 1.8f, spawnZ, groundY);
        OutputDebugStringA(buf);
    }
    m_player->SetRotation(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));   // 向きも正面にリセット
    m_player->GetPointsRef() = 0;  // 初期ポイント
    m_scoreDisplayValue = 0.0f;  // 初期ポイントに合わせる
    m_scoreFlashTimer = 0.0f;
    m_scoreShakeTimer = 0.0f;
    m_lastDisplayedScore = 0;

    // === 敵を全消去 ===
    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        RemoveEnemyPhysicsBody(enemy.id);
    }
    m_enemySystem->GetEnemies().clear();
    m_enemiesInitialized = false;

    // === ウェーブ ===
    m_waveManager->Reset();
    m_waveManager->SetPaused(false);

    // === 武器 ===
    m_weaponSystem = std::make_unique<WeaponSystem>();

    // === スコア・スタイル ===
    m_score = 0;
    m_styleRank->Reset();

    //  スタッツリセット
    m_statKills = 0;
    m_statHeadshots = 0;
    m_statMeleeKills = 0;
    m_statMaxCombo = 0;
    m_statDamageDealt = 0;
    m_statDamageTaken = 0;
    m_statMaxStyleRank = 0;
    m_statSurvivalTime = 0.0f;
    m_parrySuccessCount = 0;

    // === 状態リセット ===
    m_shieldState = ShieldState::Idle;
    m_shieldBashTimer = 0.0f;
    m_cameraShake = 0.0f;
    m_cameraShakeTimer = 0.0f;
    m_gloryKillActive = false;
    m_gloryKillCameraActive = false;
    m_gloryKillDOFActive = false;
    m_gloryKillTargetEnemy = nullptr;
    m_timeScale = 1.0f;

    // === フェード ===
    m_fadeAlpha = 1.0f;
    m_fadingIn = true;
    m_fadeActive = true;

    // === ビームエフェクト停止 ===
    if (m_beamHandle >= 0 && m_effekseerManager != nullptr)
    {
        m_effekseerManager->StopEffect(m_beamHandle);
        m_beamHandle = -1;
    }

    // === Effekseer全エフェクト停止 ===
    if (m_effekseerManager != nullptr)
    {
        m_effekseerManager->StopAllEffects();
    }

    // === 盾HP ===
    m_shieldHP = 100.0f;
    m_shieldDisplayHP = 100.0f;
    m_shieldGlowIntensity = 0.0f;
    m_chargeEnergy = 0.0f;
    m_chargeReady = false;

    // === パーティクル ===
    if (m_particleSystem)
        //m_particleSystem->Clear();

    // === 弾痕トレース ===
    m_bulletTraces.clear();

    // === スローモーション ===
    m_timeScale = 1.0f;
    m_slowMoTimer = 0.0f;

    m_meleeCharges = m_meleeMaxCharges;
    m_meleeRechargeTimer = 0.0f;

    // === タイマー類 ===
    m_typeWalkTimer[0] = m_typeWalkTimer[1] = m_typeWalkTimer[2] = m_typeWalkTimer[3] = m_typeWalkTimer[4] = 0.0f;
    m_typeAttackTimer[0] = m_typeAttackTimer[1] = m_typeAttackTimer[2] = m_typeAttackTimer[3] = m_typeAttackTimer[4] = 0.0f;

    //OutputDebugStringA("[GAME] === GAME RESET COMPLETE ===\n");
}


void Game::UpdateGameOver()
{
    // ゲームオーバーからの経過時間を加算（全演出のタイミングはこれで制御）
    m_gameOverTimer += 1.0f / 60.0f;

    // 初回だけスコアとウェーブを保存（リザルト表示用）
    if (m_gameOverWave == 0)
    {
        m_gameOverScore = m_player->GetPoints();
        m_gameOverWave = m_waveManager->GetCurrentWave();

        //  スタッツをリザルト用にコピー
        m_goKills = m_statKills;
        m_goHeadshots = m_statHeadshots;
        m_goMeleeKills = m_statMeleeKills;
        m_goMaxCombo = m_statMaxCombo;
        m_goParryCount = m_parrySuccessCount;
        m_goDamageDealt = m_statDamageDealt;
        m_goDamageTaken = m_statDamageTaken;
        m_goMaxStyleRank = m_statMaxStyleRank;
        m_goSurvivalTime = m_statSurvivalTime;

        //  各スタッツにボーナスポイントを計算
        m_goStatScores[0] = m_goKills * 10;                         // キル × 10pt
        m_goStatScores[1] = m_goHeadshots * 25;                     // HS × 25pt
        m_goStatScores[2] = m_goMeleeKills * 30;                    // 近接 × 30pt
        m_goStatScores[3] = m_goMaxCombo * 20;                      // コンボ × 20pt
        m_goStatScores[4] = m_goParryCount * 50;                    // パリィ × 50pt
        m_goStatScores[5] = (int)(m_goSurvivalTime) * 2;            // 生存秒 × 2pt
        m_goStatScores[6] = m_goDamageDealt / 10;                   // 与ダメ÷10
        m_goStatScores[7] = (m_goDamageTaken == 0) ? 1000 :         // ノーダメ=1000pt!
            (int)(5000.0f / (float)(m_goDamageTaken + 1));
        m_goStatScores[8] = m_goMaxStyleRank * 200;                 // ランク × 200pt

        //  合計スコア
        m_goTotalScore = 0;
        for (int i = 0; i < 9; i++)
            m_goTotalScore += m_goStatScores[i];

        //  合計スコアでランク判定
        if (m_goTotalScore >= 8000)
            m_gameOverRank = 3; // S
        else if (m_goTotalScore >= 5000)
            m_gameOverRank = 2; // A
        else if (m_goTotalScore >= 2500)
            m_gameOverRank = 1; // B
        else
            m_gameOverRank = 0; // C

        // カウントアップ初期化
        for (int i = 0; i < 9; i++)
            m_goStatCountUp[i] = 0.0f;

        // === 名前入力の準備（まだ保存しない） ===
        if (!m_rankingSaved && !m_nameInputActive)
        {
            memset(m_nameBuffer, 0, sizeof(m_nameBuffer));
            m_nameLength = 0;
            m_nameKeyTimer = 0.0f;
        }
    }

    // スコアカウントアップ演出（1.5秒後から2秒かけて0→最終スコア）
    if (m_gameOverTimer > 1.5f)
    {
        float countProgress = (m_gameOverTimer - 1.5f) / 2.0f;
        if (countProgress > 1.0f) countProgress = 1.0f;
        m_gameOverCountUp = 1.0f - powf(2.0f, -10.0f * countProgress);
    }

    //  スタッツの段階的カウントアップ（0.15秒ずつずらして9行）
    for (int i = 0; i < 9; i++)
    {
        float startTime = 1.5f + i * 0.15f;
        if (m_gameOverTimer > startTime)
        {
            float progress = (m_gameOverTimer - startTime) / 0.6f;
            if (progress > 1.0f) progress = 1.0f;
            m_goStatCountUp[i] = 1.0f - powf(2.0f, -10.0f * progress);
        }
    }

    // スコアカウントアップ演出（1.5秒後から2秒かけて0→最終スコア）
    if (m_gameOverTimer > 1.5f)
    {
        float countProgress = (m_gameOverTimer - 1.5f) / 2.0f;  // 0→1（2秒間）
        if (countProgress > 1.0f) countProgress = 1.0f;
        // EaseOutExpo：最初ガーッと上がって、後半タメが効く
        m_gameOverCountUp = 1.0f - powf(2.0f, -10.0f * countProgress);
    }

    // ノイズトランジション（死亡直後に画面がザザッとノイズで切り替わる）
    if (m_gameOverTimer < 0.5f)
    {
        // 0→0.5秒で0→1に進行（ノイズが広がる）
        m_gameOverNoiseT = m_gameOverTimer / 0.5f;
    }
    else
    {
        m_gameOverNoiseT = 1.0f;  // 完了
    }

    // =============================================
    // 名前入力の処理（4.5秒後に開始）
    // =============================================
    if (m_gameOverTimer > 4.5f && !m_rankingSaved)
    {
        // --- 名前入力モード開始 ---
        if (!m_nameInputActive)
        {
            m_nameInputActive = true;
            memset(m_nameBuffer, 0, sizeof(m_nameBuffer));
            m_nameLength = 0;
            m_nameKeyWasDown = false;
            OutputDebugStringA("[RANKING] Name input started\n");
        }

        // --- 今フレームで何かキーが押されてるか調べる ---
        bool anyKeyDown = false;
        int pressedKey = 0;         // 押されたキーコード
        bool isBackspace = false;
        bool isEnter = false;

        // Enter チェック
        if (GetAsyncKeyState(VK_RETURN) & 0x8000)
        {
            anyKeyDown = true;
            isEnter = true;
        }

        // Backspace チェック
        if (GetAsyncKeyState(VK_BACK) & 0x8000)
        {
            anyKeyDown = true;
            isBackspace = true;
        }

        // A-Z チェック
        if (!anyKeyDown)
        {
            for (int key = 'A'; key <= 'Z'; key++)
            {
                if (GetAsyncKeyState(key) & 0x8000)
                {
                    anyKeyDown = true;
                    pressedKey = key;
                    break;
                }
            }
        }

        // 0-9 チェック
        if (!anyKeyDown)
        {
            for (int key = '0'; key <= '9'; key++)
            {
                if (GetAsyncKeyState(key) & 0x8000)
                {
                    anyKeyDown = true;
                    pressedKey = key;
                    break;
                }
            }
        }

        // --- 「押した瞬間」だけ反応する ---
        // 前フレームで押されてなくて、今フレームで押された = 新規入力
        if (anyKeyDown && !m_nameKeyWasDown)
        {
            if (isEnter)
            {
                // 名前が空なら "NONAME" をセット
                if (m_nameLength == 0)
                {
                    strcpy_s(m_nameBuffer, "NONAME");
                    m_nameLength = 6;
                }

                // ランキングに保存
                RankingEntry entry;
                entry.score = m_goTotalScore;
                entry.wave = m_gameOverWave;
                entry.kills = m_goKills;
                entry.headshots = m_goHeadshots;
                entry.rank = m_gameOverRank;
                entry.survivalTime = m_goSurvivalTime;
                strcpy_s(entry.name, m_nameBuffer);

                m_newRecordRank = m_rankingSystem->AddEntry(entry);
                m_rankingSaved = true;
                m_nameInputActive = false;

                char buf[128];
                sprintf_s(buf, "[RANKING] Saved! Name: %s, Rank: %d\n",
                    m_nameBuffer, m_newRecordRank + 1);
                OutputDebugStringA(buf);
            }
            else if (isBackspace)
            {
                // 1文字削除
                if (m_nameLength > 0)
                {
                    m_nameLength--;
                    m_nameBuffer[m_nameLength] = '\0';
                }
            }
            else if (pressedKey != 0 && m_nameLength < 15)
            {
                // 文字追加（A-Z or 0-9）
                m_nameBuffer[m_nameLength] = (char)pressedKey;
                m_nameLength++;
                m_nameBuffer[m_nameLength] = '\0';
            }
        }

        // --- 今フレームの状態を記録（次フレームの比較用） ---
        m_nameKeyWasDown = anyKeyDown;

        return;  // 名前入力中は R/L キーをブロック
    }

    // 2.5秒経過後にRキーでリスタート可能（誤操作防止）
    if (m_gameOverTimer > 4.5f && (GetAsyncKeyState('R') & 0x8000))
    {
        ResetGame();
        m_gameOverTimer = 0.0f;
        m_gameOverScore = 0;
        m_gameOverWave = 0;
        m_gameOverCountUp = 0.0f;
        m_gameOverRank = 0;
        m_gameOverNoiseT = 0.0f;
        m_gameState = GameState::TITLE;

       
    }

    // Lキーでランキング画面へ
    if (m_gameOverTimer > 4.5f && (GetAsyncKeyState('L') & 0x8000))
    {
        m_rankingTimer = 0.0f;
        m_gameState = GameState::RANKING;
    }
}

void Game::UpdateFade()
{
    if (!m_fadeActive)
        return;

    float fadeSpeed = 2.0f * (1.0f / 60.0f);

    if (m_fadingIn)
    {
        m_fadeAlpha -= fadeSpeed;
        if (m_fadeAlpha <= 0.0f)
        {
            m_fadeAlpha = 0.0f;
            m_fadeActive = false;
        }
    }
    else
    {
        m_fadeAlpha += fadeSpeed;
        if (m_fadeAlpha >= 1.0f)
        {
            m_fadeAlpha = 1.0f;
            m_fadeActive = false;
        }
    }
}


void Game::RenderDamageFlash()
{
    auto context = m_d3dContext.Get();

    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    float alpha = m_player->GetDamageTimer();  // タイマーに応じてフェードアウト
    DirectX::XMFLOAT4 bloodColor(0.8f, 0.0f, 0.0f, alpha * 0.3f);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面端を赤く
    float borderSize = 100.0f;

    // 上
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight - borderSize, 1.0f), bloodColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(-halfWidth, halfHeight - borderSize, 1.0f), bloodColor);

    primitiveBatch->DrawQuad(v1, v2, v3, v4);


    primitiveBatch->End();
}

void Game::RenderLowHealthVignette()
{
    // テクスチャが読み込めていなければスキップ
    if (!m_lowHealthVignetteSRV || !m_player) return;

    // === HP比率を計算 ===
    int   hp = m_player->GetHealth();
    int   maxHp = 100;  // 最大HPに合わせて変更してね
    float ratio = (float)hp / (float)maxHp;  // 1.0=満タン, 0.0=瀕死

    // HP70%以上なら何も表示しない
    if (ratio > 0.8f) return;

    // === 強度を計算（0.0?1.0） ===
    // HP70%で0.0、HP0%で1.0
    float intensity = 1.0f - (ratio / 0.7f);
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    // === 心拍パルス（HP30%以下で発動） ===
    float pulse = 1.0f;
    if (ratio < 0.3f)
    {
        // 心拍リズム: HP低いほど速い（1.5?3.5Hz）
        float heartRate = 1.5f + (1.0f - ratio / 0.3f) * 2.0f;

        // sin^8 で鋭い「ドクッ」パルスを作る
        float t = sinf(m_accumulatedAnimTime * heartRate * 3.14159f);
        t = t * t;  // sin^2
        t = t * t;  // sin^4
        t = t * t;  // sin^8（鋭いスパイク）

        // パルスの振れ幅（HP低いほど大きい）
        float pulseStrength = 0.15f + (1.0f - ratio / 0.3f) * 0.4f;
        pulse = 1.0f + t * pulseStrength;
    }

    // === 最終アルファ ===
    // intensity（0?1）× pulse（1?1.5くらい）
    float alpha = intensity * pulse;
    if (alpha > 1.0f) alpha = 1.0f;

    // === SpriteBatchで画面全体にテクスチャを描画 ===
    auto context = m_d3dContext.Get();

    // ブレンドステート: アルファブレンド
    // （テクスチャのアルファ × 頂点カラーのアルファ で合成される）
    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,   // ソート方式
        m_states->NonPremultiplied()         // // PNG用（プリマルチプライドじゃない）
    );

    // 画面全体に描画
    RECT destRect = { 0, 0, (LONG)m_outputWidth, (LONG)m_outputHeight };

    // 色の調整:
    // - RGBで赤の濃さを制御
    // - Aでアルファ（不透明度）を制御
    DirectX::XMVECTORF32 tintColor = { {
        1.0f,          // R: そのまま
        0.0f,          // G: 緑を消す（より赤く）
        0.0f,          // B: 青を消す
        alpha * 1.7f  // A: HPに連動（最大85%）
    } };

    m_spriteBatch->Draw(
        m_lowHealthVignetteSRV.Get(),  // テクスチャ
        destRect,                       // 描画先（画面全体）
        tintColor                       // 色とアルファ
    );

    m_spriteBatch->End();
}

//  スピードライン描画 
void Game::RenderSpeedLines()
{
    if (m_speedLineAlpha <= 0.01f) return;

    auto context = m_d3dContext.Get();

    // 2D描画セットアップ（RenderDamageFlashと同じパターン）
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    batch->Begin();

    float halfW = m_outputWidth * 0.5f;
    float halfH = m_outputHeight * 0.5f;
    float alpha = m_speedLineAlpha;

    int lineCount = 28;  // 放射状の線の数

    for (int i = 0; i < lineCount; i++)
    {
        // 放射角度
        float angle = (float)i / lineCount * 6.283f;

        // ランダムなゆらぎ（毎フレーム変わって動いてる感じ）
        float jitter = ((float)(rand() % 1000) / 1000.0f) * 0.15f;
        angle += jitter;

        float cosA = cosf(angle);
        float sinA = sinf(angle);

        // 外側の点（画面端の外まで伸ばす）
        float outerDist = halfW * 1.3f;
        float outerX = cosA * outerDist;
        float outerY = sinA * outerDist;

        // 内側の点（画面中央に向かって）
        // ランダムな長さで「流れてる」感じ
        float innerRatio = 0.45f + ((float)(rand() % 1000) / 1000.0f) * 0.25f;
        float innerX = cosA * outerDist * innerRatio;
        float innerY = sinA * outerDist * innerRatio;

        // 線の太さ（外側が太く、内側が細い）
        float thickness = 2.5f + ((float)(rand() % 1000) / 1000.0f) * 3.0f;
        float perpX = -sinA * thickness;
        float perpY = cosA * thickness;

        // 色（白、外側は不透明、内側は透明）
        float lineAlpha = alpha * (0.3f + ((float)(rand() % 1000) / 1000.0f) * 0.5f);
        DirectX::XMFLOAT4 outerColor(1.0f, 1.0f, 1.0f, lineAlpha);
        DirectX::XMFLOAT4 innerColor(1.0f, 1.0f, 1.0f, 0.0f);

        // 三角形2枚で1本のスピードラインを描画
        DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(outerX + perpX, outerY + perpY, 1.0f), outerColor);
        DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(outerX - perpX, outerY - perpY, 1.0f), outerColor);
        DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(innerX, innerY, 1.0f), innerColor);

        batch->DrawTriangle(v1, v2, v3);
    }

    batch->End();
}

void Game::RenderDashOverlay()
{
    // テクスチャがないorアルファ0なら描画しない
    if (!m_dashSpeedlineSRV || m_dashOverlayAlpha <= 0.01f) return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    RECT destRect = { 0, 0, (LONG)m_outputWidth, (LONG)m_outputHeight };

    // 回転させてバリエーションを出す（毎フレーム微妙に回転）
    DirectX::XMVECTORF32 tintColor = { {
        0.9f, 0.9f, 1.0f,
        m_dashOverlayAlpha
    } };

    m_spriteBatch->Draw(
        m_dashSpeedlineSRV.Get(),
        destRect,
        tintColor
    );

    m_spriteBatch->End();
}

void Game::RenderReloadWarning()
{
    // === リロード中の進捗バーも表示 ===
    bool isReloading = m_weaponSystem->IsReloading();
    bool showWarning = (m_reloadWarningAlpha > 0.01f) || isReloading;

    if (!showWarning) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // =============================================
    // "RELOAD" 警告テキスト（弾切れ時）
    // =============================================
    if (m_reloadWarningAlpha > 0.01f && !isReloading)
    {
        // 脈動（パルス）: sinで明滅
        float pulse = 0.5f + sinf(m_reloadWarningTimer * 6.0f) * 0.5f;
        float alpha = m_reloadWarningAlpha * (0.5f + pulse * 0.5f);

        // スケール弾み（登場時にバウンス）
        float scaleT = m_reloadWarningTimer;
        float scale = 1.0f;
        if (scaleT < 0.3f)
        {
            float t = scaleT / 0.3f;
            scale = 1.0f + sinf(t * 3.14159f) * 0.3f;  // 登場時に1.3倍→1.0倍
        }

        // --- メインテキスト ---
        const wchar_t* text = L"[  R E L O A D  ]";

        DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(text);
        float textW = DirectX::XMVectorGetX(textSize) * scale;
        float textH = DirectX::XMVectorGetY(textSize) * scale;
        float textX = (W - textW) * 0.5f;
        float textY = H * 0.58f;

        // グロー（影として2回描画）
        DirectX::XMVECTORF32 glowColor = { 0.3f, 0.0f, 0.0f, alpha * 0.6f };
        for (int dx = -2; dx <= 2; dx += 4)
        {
            for (int dy = -2; dy <= 2; dy += 4)
            {
                m_fontLarge->DrawString(m_spriteBatch.get(), text,
                    DirectX::XMFLOAT2(textX + dx, textY + dy),
                    glowColor, 0.0f,
                    DirectX::XMFLOAT2(0, 0),
                    scale);
            }
        }

        // メインテキスト（赤）
        DirectX::XMVECTORF32 mainColor = { 0.95f, 0.15f, 0.1f, alpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), text,
            DirectX::XMFLOAT2(textX, textY),
            mainColor, 0.0f,
            DirectX::XMFLOAT2(0, 0),
            scale);

        // --- サブテキスト ---
        const wchar_t* subText = L"Press  R";

        DirectX::XMVECTOR subSize = m_font->MeasureString(subText);
        float subW = DirectX::XMVectorGetX(subSize);
        float subX = (W - subW) * 0.5f;
        float subY = textY + textH + 8.0f;

        float subPulse = 0.3f + pulse * 0.7f;
        DirectX::XMVECTORF32 subColor = { 0.7f, 0.7f, 0.7f, alpha * subPulse };
        m_font->DrawString(m_spriteBatch.get(), subText,
            DirectX::XMFLOAT2(subX, subY), subColor);
    }

    // =============================================
    //  リロード進捗バー（リロード中）
    // =============================================
    if (isReloading && m_whitePixel)
    {
        float progress = m_reloadAnimProgress;  // 0?1

        float barW = W * 0.25f;
        float barH = 4.0f;
        float barX = (W - barW) * 0.5f;
        float barY = H * 0.62f;

        // 背景（暗い）
        RECT bgRect = {
            (LONG)barX, (LONG)barY,
            (LONG)(barX + barW), (LONG)(barY + barH)
        };
        DirectX::XMVECTORF32 bgColor = { 0.15f, 0.15f, 0.15f, 0.8f };
        m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

        // 進捗（白→赤のグラデーション）
        float fillW = barW * progress;
        RECT fillRect = {
            (LONG)barX, (LONG)barY,
            (LONG)(barX + fillW), (LONG)(barY + barH)
        };
        // 完了に近づくほど白くなる
        float r = 1.0f;
        float g = 0.3f + progress * 0.7f;
        float b = 0.3f + progress * 0.7f;
        DirectX::XMVECTORF32 fillColor = { r, g, b, 0.9f };
        m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, fillColor);

        // テキスト "RELOADING..."
        const wchar_t* reloadText = L"RELOADING...";
        DirectX::XMVECTOR rSize = m_font->MeasureString(reloadText);
        float rW = DirectX::XMVectorGetX(rSize);
        float rX = (W - rW) * 0.5f;
        float rY = barY - 24.0f;

        DirectX::XMVECTORF32 reloadColor = { 0.8f, 0.8f, 0.8f, 0.9f };
        m_font->DrawString(m_spriteBatch.get(), reloadText,
            DirectX::XMFLOAT2(rX, rY), reloadColor);
    }

    m_spriteBatch->End();
}

void Game::RenderWaveBanner()
{
    if (m_waveBannerTimer <= 0.0f) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;
    float t = m_waveBannerTimer / m_waveBannerDuration;  // 1→0

    // フェードイン/アウト
    float alpha = 1.0f;
    if (t > 0.85f) alpha = (1.0f - t) / 0.15f;      // 登場
    else if (t < 0.2f) alpha = t / 0.2f;              // 消滅

    // スライドイン（右から中央へ）
    float slideOffset = 0.0f;
    float slideT = 1.0f - t;
    if (slideT < 0.12f)
        slideOffset = (1.0f - slideT / 0.12f) * 300.0f;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // === バナー画像 ===
    auto& bannerSRV = m_waveBannerIsBoss ? m_waveBannerBossSRV : m_waveBannerNormalSRV;
    if (bannerSRV)
    {
        float bannerH = H * 0.12f;
        float bannerY = (H - bannerH) * 0.5f;
        RECT destRect = {
            (LONG)(0 + slideOffset * 0.3f),
            (LONG)bannerY,
            (LONG)(W + slideOffset * 0.3f),
            (LONG)(bannerY + bannerH)
        };
        DirectX::XMVECTORF32 bannerColor = { 1.0f, 1.0f, 1.0f, alpha };
        m_spriteBatch->Draw(bannerSRV.Get(), destRect, bannerColor);
    }

    // === ウェーブ番号（大きくスライドイン） ===
    if (m_fontLarge)
    {
        wchar_t numText[16];
        swprintf_s(numText, L"%d", m_waveBannerNumber);

        DirectX::XMVECTOR numSize = m_fontLarge->MeasureString(numText);
        float numW = DirectX::XMVectorGetX(numSize);
        float numX = (W - numW) * 0.5f + slideOffset + W * 0.16f;
        float numY = H * 0.5f - DirectX::XMVectorGetY(numSize) * 0.49f;

        // グロー
        DirectX::XMVECTORF32 glowColor = m_waveBannerIsBoss
            ? DirectX::XMVECTORF32{ 0.4f, 0.02f, 0.0f, alpha * 0.6f }
        : DirectX::XMVECTORF32{ 0.4f, 0.35f, 0.0f, alpha * 0.6f };
        for (int dx = -3; dx <= 3; dx += 3)
            for (int dy = -3; dy <= 3; dy += 3)
                m_fontLarge->DrawString(m_spriteBatch.get(), numText,
                    DirectX::XMFLOAT2(numX + dx, numY + dy), glowColor);

        // メイン
        DirectX::XMVECTORF32 numColor = m_waveBannerIsBoss
            ? DirectX::XMVECTORF32{ 1.0f, 0.2f, 0.1f, alpha }
        : DirectX::XMVECTORF32{ 1.0f, 0.85f, 0.2f, alpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), numText,
            DirectX::XMFLOAT2(numX, numY), numColor);
    }

    m_spriteBatch->End();
}


void Game::RenderScoreHUD()
{
    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // === 血しぶき背景 ===
    if (m_scoreBackdropSRV)
    {
        float bgW = W * 0.22f;
        float bgH = H * 0.07f;
        float bgX = 10.0f;
        float bgY = 10.0f;

        // 加算時にちょっと揺れる
        float shakeX = 0.0f, shakeY = 0.0f;
        if (m_scoreShakeTimer > 0.0f)
        {
            float intensity = m_scoreShakeTimer / 0.2f;
            shakeX = sinf(m_scoreShakeTimer * 80.0f) * 3.0f * intensity;
            shakeY = cosf(m_scoreShakeTimer * 60.0f) * 2.0f * intensity;
        }

        RECT bgRect = {
            (LONG)(bgX + shakeX),
            (LONG)(bgY + shakeY),
            (LONG)(bgX + bgW + shakeX),
            (LONG)(bgY + bgH + shakeY)
        };

        // 加算時に少し明るくなる
        float flashBright = 1.0f;
        if (m_scoreFlashTimer > 0.0f)
            flashBright = 1.0f + (m_scoreFlashTimer / 0.4f) * 0.8f;

        DirectX::XMVECTORF32 bgColor = {
            flashBright, flashBright, flashBright, 0.9f
        };
        m_spriteBatch->Draw(m_scoreBackdropSRV.Get(), bgRect, bgColor);

        // === スコア数字 ===
        if (m_fontLarge)
        {
            wchar_t scoreText[32];
            swprintf_s(scoreText, L"%d", (int)m_scoreDisplayValue);

            DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(scoreText);
            float textH = DirectX::XMVectorGetY(textSize);
            float textX = bgX + 15.0f + shakeX;
            float textY = bgY + (bgH - textH) * 0.5f + shakeY;

            // グロー（赤い影）
            DirectX::XMVECTORF32 glowColor = { 0.5f, 0.02f, 0.0f, 0.6f };
            for (int dx = -2; dx <= 2; dx += 4)
                for (int dy = -2; dy <= 2; dy += 4)
                    m_fontLarge->DrawString(m_spriteBatch.get(), scoreText,
                        DirectX::XMFLOAT2(textX + dx, textY + dy), glowColor);

            // メイン（加算時は赤→白に光る）
            float flash = (m_scoreFlashTimer > 0.0f) ? m_scoreFlashTimer / 0.4f : 0.0f;
            DirectX::XMVECTORF32 mainColor = {
                0.95f,
                0.85f + flash * 0.15f,
                0.8f + flash * 0.2f,
                1.0f
            };
            m_fontLarge->DrawString(m_spriteBatch.get(), scoreText,
                DirectX::XMFLOAT2(textX, textY), mainColor);
        }

        // === "PTS" ラベル（小さく右に） ===
        if (m_font)
        {
            wchar_t scoreText[32];
            swprintf_s(scoreText, L"%d", (int)m_scoreDisplayValue);
            DirectX::XMVECTOR scoreSize = m_fontLarge->MeasureString(scoreText);
            float scoreW = DirectX::XMVectorGetX(scoreSize);

            float ptsX = bgX + 15.0f + scoreW + 8.0f + shakeX;
            float ptsY = bgY + bgH * 0.5f - 5.0f + shakeY;

            DirectX::XMVECTORF32 ptsColor = { 0.6f, 0.15f, 0.1f, 0.8f };
            m_font->DrawString(m_spriteBatch.get(), L"PTS",
                DirectX::XMFLOAT2(ptsX, ptsY), ptsColor);
        }
    }

    m_spriteBatch->End();
}

void Game::RenderClawDamage()
{
    if (m_clawTimer <= 0.0f || !m_clawDamageSRV) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;
    float t = m_clawTimer / m_clawDuration;  // 1→0

    // スラッシュアニメーション: 右上から右下へスライド
    // t=1(開始): 画面外の上 → t=0.5: 画面中央 → t=0: 画面下でフェードアウト
    float slideProgress = 1.0f - t;  // 0→1

    // テクスチャサイズ
    float clawW = W * 0.5f;
    float clawH = H * 1.2f;

    // X位置: 画面右寄り
    float clawX = (W - clawW) * 0.5f;

    // Y位置: 上から下へスライド（高速で引っかく感じ）
    float startY = -clawH * 0.8f;   // 画面上の外
    float endY = H * 0.1f;          // 画面中央付近
    float clawY;

    if (slideProgress < 0.15f)
    {
        // 最初の15%: 高速スラッシュ（上→中央）
        float p = slideProgress / 0.15f;
        p = p * p * (3.0f - 2.0f * p);  // スムーズステップ
        clawY = startY + (endY - startY) * p;
    }
    else
    {
        // 残り: 中央に留まってフェードアウト
        clawY = endY;
    }

    // アルファ: 一気に出て、じわっと消える
    float alpha;
    if (slideProgress < 0.15f)
        alpha = slideProgress / 0.15f;          // フェードイン
    else if (slideProgress < 0.4f)
        alpha = 1.0f;                            // フル表示
    else
        alpha = 1.0f - (slideProgress - 0.4f) / 0.6f;  // フェードアウト

    alpha = max(0.0f, min(1.0f, alpha));

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    RECT destRect = {
        (LONG)clawX,
        (LONG)clawY,
        (LONG)(clawX + clawW),
        (LONG)(clawY + clawH)
    };

    DirectX::XMVECTORF32 color = { 1.0f, 1.0f, 1.0f, alpha * 0.9f };
    m_spriteBatch->Draw(m_clawDamageSRV.Get(), destRect, color);

    m_spriteBatch->End();
}


void Game::RenderGloryKillFlash()
{
    return;

    // グローリーキルフラッシュが無効なら何もしない
    if (m_gloryKillFlashTimer <= 0.0f || m_gloryKillFlashAlpha <= 0.0f)
        return;

    auto context = m_d3dContext.Get();

    // 2D描画用の設定
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // PrimitiveBatchを作成
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // 白いフラッシュの色（透明度はm_gloryKillFlashAlpha）
    DirectX::XMFLOAT4 flashColor(1.0f, 1.0f, 1.0f, m_gloryKillFlashAlpha * 0.01f);

    // 画面のサイズ（中心からの距離）
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面全体を覆う四角形
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), flashColor);   // 左上
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), flashColor);    // 右上
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, -halfHeight, 1.0f), flashColor);   // 右下
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(-halfWidth, -halfHeight, 1.0f), flashColor);  // 左下

    // 四角形を描画
    primitiveBatch->DrawQuad(v1, v2, v3, v4);

    primitiveBatch->End();
}

void Game::RenderTitle()
{
    // === 画面クリア ===
    Clear();

    // === タイトルシーンを描画 ===
    if (m_titleScene)
    {
        try
        {
            m_titleScene->Render(
                m_d3dContext.Get(),
                m_renderTargetView.Get(),    // バックバッファ
                m_depthStencilView.Get()     // 深度バッファ
            );
        }
        catch (const std::exception& e)
        {
            //OutputDebugStringA("[RENDER ERROR] TitleScene render failed: ");
            //OutputDebugStringA(e.what());
            //OutputDebugStringA("\n");
        }
    }
    else
    {
        // TitleScene がない場合はエラーメッセージを表示
        //OutputDebugStringA("[RENDER WARNING] TitleScene is null\n");
    }

    // === UI描画（オプション）===
    // タイトルテキストなどを追加する場合
    /*
    m_spriteBatch->Begin();

    DirectX::XMFLOAT2 titlePos(
        m_outputWidth / 2.0f,
        m_outputHeight / 2.0f
    );

    m_fontLarge->DrawString(
        m_spriteBatch.get(),
        L"GOTHIC SWARM",
        titlePos,
        DirectX::Colors::White,
        0.0f,
        DirectX::XMFLOAT2(0.5f, 0.5f)
    );

    DirectX::XMFLOAT2 startPos(
        m_outputWidth / 2.0f,
        m_outputHeight / 2.0f + 100.0f
    );

    m_font->DrawString(
        m_spriteBatch.get(),
        L"Press ENTER to Start",
        startPos,
        DirectX::Colors::White,
        0.0f,
        DirectX::XMFLOAT2(0.5f, 0.5f)
    );

    m_spriteBatch->End();
    */

    // === フェード描画 ===
    RenderFade();
}

void Game::RenderLoading()
{
    // === 画面クリア（真っ黒） ===
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), black);

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // =========================================
    // ロードメッセージ
    // =========================================
    const wchar_t* messages[] = {
        L"INITIALIZING SYSTEMS...",
        L"LOADING ARSENAL...",
        L"SPAWNING ENTITIES...",
        L"CALIBRATING GOTHIC FREQUENCIES...",
        L"READY"
    };

    const wchar_t* msg = messages[m_loadingPhase];

    // メッセージの色（最終フェーズだけ赤く）
    DirectX::XMVECTORF32 msgColor;
    if (m_loadingPhase == 4)
        msgColor = { 0.9f, 0.15f, 0.1f, 1.0f };   // 赤（READY）
    else
        msgColor = { 0.6f, 0.6f, 0.6f, 1.0f };     // グレー

    // 中央下あたりに表示
    DirectX::XMVECTOR msgSize = m_font->MeasureString(msg);
    float msgW = DirectX::XMVectorGetX(msgSize);
    float msgX = (W - msgW) * 0.5f;
    float msgY = H * 0.58f;

    m_font->DrawString(m_spriteBatch.get(), msg,
        DirectX::XMFLOAT2(msgX, msgY), msgColor);

    // =========================================
    // プログレスバー
    // =========================================
    if (m_whitePixel)
    {
        float barW = W * 0.5f;       // バーの最大幅（画面幅の50%）
        float barH = 6.0f;            // バーの高さ
        float barX = (W - barW) * 0.5f;
        float barY = H * 0.65f;

        // --- 背景（暗いグレー） ---
        RECT bgRect = {
            (LONG)barX,
            (LONG)barY,
            (LONG)(barX + barW),
            (LONG)(barY + barH)
        };
        DirectX::XMVECTORF32 bgColor = { 0.15f, 0.15f, 0.15f, 1.0f };
        m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

        // --- 進捗バー（赤） ---
        float fillW = barW * m_loadingBarCurrent;
        RECT fillRect = {
            (LONG)barX,
            (LONG)barY,
            (LONG)(barX + fillW),
            (LONG)(barY + barH)
        };

        // パルス効果（微妙に明滅）
        float pulse = 0.7f + sinf(m_loadingTimer * 4.0f) * 0.3f;
        DirectX::XMVECTORF32 barColor = { 0.8f * pulse, 0.1f * pulse, 0.05f * pulse, 1.0f };
        m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, barColor);

        // --- パーセント表示 ---
        int percent = (int)(m_loadingBarCurrent * 100.0f);
        wchar_t percentText[16];
        swprintf_s(percentText, L"%d%%", percent);

        DirectX::XMVECTOR pSize = m_font->MeasureString(percentText);
        float pW = DirectX::XMVectorGetX(pSize);
        float pX = (W - pW) * 0.5f;
        float pY = barY + barH + 10.0f;

        DirectX::XMVECTORF32 percentColor = { 0.5f, 0.5f, 0.5f, 0.8f };
        m_font->DrawString(m_spriteBatch.get(), percentText,
            DirectX::XMFLOAT2(pX, pY), percentColor);
    }

    // =========================================
    // タイトルロゴ（画面上部）
    // =========================================
    if (m_fontLarge)
    {
        const wchar_t* title = L"GOTHIC SWARM";
        DirectX::XMVECTOR titleSize = m_fontLarge->MeasureString(title);
        float titleW = DirectX::XMVectorGetX(titleSize);
        float titleX = (W - titleW) * 0.5f;
        float titleY = H * 0.3f;

        // じわっとフェードイン
        float titleAlpha = (m_loadingTimer < 0.8f) ? (m_loadingTimer / 0.8f) : 1.0f;
        DirectX::XMVECTORF32 titleColor = { 0.9f, 0.1f, 0.05f, titleAlpha };

        m_fontLarge->DrawString(m_spriteBatch.get(), title,
            DirectX::XMFLOAT2(titleX, titleY), titleColor);
    }

    // =========================================
    //  点滅するドット演出（...のアニメーション）
    // =========================================
    if (m_loadingPhase < 4)
    {
        // 0.5秒ごとにドットが増える（1?3個）
        int dots = ((int)(m_loadingTimer * 2.0f) % 3) + 1;
        wchar_t dotText[8] = L"";
        for (int i = 0; i < dots; i++)
            wcscat_s(dotText, L".");

        float dotX = msgX + msgW + 2.0f;
        DirectX::XMVECTORF32 dotColor = { 0.4f, 0.4f, 0.4f, 0.6f };
        m_font->DrawString(m_spriteBatch.get(), dotText,
            DirectX::XMFLOAT2(dotX, msgY), dotColor);
    }

    // =========================================
    // "Press Enter" 表示（READY後）
    // =========================================
    if (m_loadingPhase == 4)
    {
        const wchar_t* pressText = L"- - Press Enter - -";

        // 点滅
        float blink = (sinf(m_loadingTimer * 4.0f) + 1.0f) * 0.5f;
        float alpha = 0.4f + blink * 0.6f;

        DirectX::XMVECTOR peSize = m_font->MeasureString(pressText);
        float peW = DirectX::XMVectorGetX(peSize);
        float peX = (W - peW) * 0.5f;
        float peY = H * 0.75f;

        DirectX::XMVECTORF32 peColor = { 0.8f, 0.8f, 0.8f, alpha };
        m_font->DrawString(m_spriteBatch.get(), pressText,
            DirectX::XMFLOAT2(peX, peY), peColor);
    }

    m_spriteBatch->End();
}

void Game::RenderGameOver()
{
    // =============================================
    // イージング関数（UI演出の心臓部）
    // 全部 t=0.0（開始）→ t=1.0（完了）で値を返す
    // =============================================

    // EaseOutQuad: じわっと減速（オーバーレイ向き）
    auto EaseOutQuad = [](float t) -> float {
        return 1.0f - (1.0f - t) * (1.0f - t);
        };

    // EaseOutCubic: もう少し強い減速（スライドイン向き）
    auto EaseOutCubic = [](float t) -> float {
        float u = 1.0f - t;
        return 1.0f - u * u * u;
        };

    // EaseOutBack: 目的地を行き過ぎてバネで戻る（装飾線向き）
    auto EaseOutBack = [](float t) -> float {
        float c = 1.70158f;   // 行き過ぎ量（大きいほどオーバーシュート）
        float u = t - 1.0f;
        return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
        };

    // EaseOutBounce: バウンド（YOU DIED向き）
    auto EaseOutBounce = [](float t) -> float {
        if (t < 1.0f / 2.75f)
            return 7.5625f * t * t;
        else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        }
        else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        }
        else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
        };

    // 経過時間のショートカット
    float timer = m_gameOverTimer;
    float screenW = (float)m_outputWidth;
    float screenH = (float)m_outputHeight;
    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    // =============================================
    // Layer 0: ノイズトランジション（死亡直後に走査線ノイズ）
    // =============================================
    if (m_gameOverNoiseT < 1.0f && m_whitePixel && m_spriteBatch && m_states)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // ランダムな水平ラインを描画（走査線ノイズ風）
        float noiseAlpha = 1.0f - m_gameOverNoiseT;  // だんだん消える
        int numLines = (int)(60 * (1.0f - m_gameOverNoiseT));  // ライン数も減る

        for (int i = 0; i < numLines; i++)
        {
            // 疑似ランダム（フレームごとに変わる）
            float seed = sinf((float)i * 127.1f + timer * 311.7f) * 43758.5453f;
            seed = seed - floorf(seed);  // 0?1に正規化

            float lineY = seed * screenH;
            float lineH = 1.0f + seed * 3.0f;  // 1?4pxの太さ

            // ランダムなX方向オフセット（画面がズレる感じ）
            float offsetX = sinf(seed * 100.0f + timer * 50.0f) * 20.0f * noiseAlpha;

            RECT noiseRect = {
                (LONG)(offsetX), (LONG)lineY,
                (LONG)(screenW + offsetX), (LONG)(lineY + lineH)
            };

            // 白い走査線
            DirectX::XMVECTORF32 noiseColor = { 1.0f, 1.0f, 1.0f, noiseAlpha * 0.3f * seed };
            m_spriteBatch->Draw(m_whitePixel.Get(), noiseRect, noiseColor);
        }

        // 画面全体に赤いフラッシュ（一瞬だけ）
        if (m_gameOverNoiseT < 0.15f)
        {
            float flashAlpha = (0.15f - m_gameOverNoiseT) / 0.15f;
            RECT fullScreen = { 0, 0, (LONG)screenW, (LONG)screenH };
            DirectX::XMVECTORF32 flashColor = { 0.8f, 0.0f, 0.0f, flashAlpha * 0.5f };
            m_spriteBatch->Draw(m_whitePixel.Get(), fullScreen, flashColor);
        }

        m_spriteBatch->End();
    }

    auto context = m_d3dContext.Get();

    // =============================================
    // Layer 1: 赤黒ビネット（画面全体を覆う暗幕）
    // =============================================
    {
        // じわっと暗くなる（EaseOutQuadで自然な減速）
        float overlayT = min(timer / 1.5f, 1.0f);
        float overlayAlpha = EaseOutQuad(overlayT) * 0.88f;

        DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
            screenW, screenH, 0.1f, 10.0f);

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(DirectX::XMMatrixIdentity());
        m_effect->SetVertexColorEnabled(true);
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());

        auto primBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
        primBatch->Begin();

        float hw = screenW * 0.5f;
        float hh = screenH * 0.5f;

        // 中央は赤黒、端はもっと暗い（ビネット効果）
        // 中央の色（やや赤い）
        DirectX::XMFLOAT4 centerColor(0.12f, 0.0f, 0.0f, overlayAlpha * 0.7f);
        // 端の色（ほぼ真っ黒）
        DirectX::XMFLOAT4 edgeColor(0.03f, 0.0f, 0.0f, overlayAlpha);

        // 中心点
        DirectX::VertexPositionColor vc(DirectX::XMFLOAT3(0, 0, 1.0f), centerColor);

        // 四隅
        DirectX::VertexPositionColor vTL(DirectX::XMFLOAT3(-hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vTR(DirectX::XMFLOAT3(hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBR(DirectX::XMFLOAT3(hw, -hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBL(DirectX::XMFLOAT3(-hw, -hh, 1.0f), edgeColor);

        // 中心から四隅への三角形4枚（グラデーション＝ビネット）
        primBatch->DrawTriangle(vc, vTL, vTR);  // 上
        primBatch->DrawTriangle(vc, vTR, vBR);  // 右
        primBatch->DrawTriangle(vc, vBR, vBL);  // 下
        primBatch->DrawTriangle(vc, vBL, vTL);  // 左

        primBatch->End();
    }

    // =============================================
    // Layer 2: SpriteBatchでテキスト＋装飾
    // =============================================
    if (m_spriteBatch && m_states && m_font && m_whitePixel)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // --- 装飾線（上）: 0.3秒後に左右から中央へ伸びる ---
        if (timer > 0.3f)
        {
            float lineT = min((timer - 0.3f) / 0.5f, 1.0f);
            // EaseOutBackで行き過ぎてバネで戻る
            float lineProgress = EaseOutBack(lineT);
            float lineHalfW = 220.0f * lineProgress;  // 最大片側220px
            float lineY = centerY - 340.0f;             // YOU DIEDの上

            // 左側の線（中央から左へ伸びる）
            RECT leftLine = {
                (LONG)(centerX - lineHalfW), (LONG)lineY,
                (LONG)centerX, (LONG)(lineY + 2)
            };
            // 右側の線（中央から右へ伸びる）
            RECT rightLine = {
                (LONG)centerX, (LONG)lineY,
                (LONG)(centerX + lineHalfW), (LONG)(lineY + 2)
            };

            DirectX::XMVECTORF32 lineColor = { 0.7f, 0.15f, 0.0f, 0.9f * lineT };
            m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
            m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

            // 中央の小さなダイヤ装飾（ゴシック感）
            RECT diamond = {
                (LONG)(centerX - 4), (LONG)(lineY - 3),
                (LONG)(centerX + 4), (LONG)(lineY + 5)
            };
            m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
        }

        // --- 「YOU DIED」: 0.6秒後にバウンドしながら上から落ちる ---
        if (timer > 0.6f)
        {
            float diedT = min((timer - 0.6f) / 0.8f, 1.0f);
            // EaseOutBounceでドスンと落ちてバウンド
            float bounceT = EaseOutBounce(diedT);
            float textAlpha = min((timer - 0.6f) * 3.0f, 1.0f);

            // 上からスライド（-80px → 0px）
            float slideY = (1.0f - bounceT) * -80.0f;
            float diedY = centerY - 320.0f + slideY;
            
            //  グロー（発光）：同じテキストを少しずらして半透明で複数回描く
            if (m_fontLarge)
            {
                // テキスト幅を測って手動で中央揃え
                DirectX::XMVECTOR diedSize = m_fontLarge->MeasureString(L"YOU DIED");
                float diedW = DirectX::XMVectorGetX(diedSize);
                float diedX = centerX - diedW * 0.5f;

                // グロー（4方向にずらして半透明で描く→発光して見える）
                float glowAlpha = textAlpha * 0.25f;
                DirectX::XMVECTORF32 glowColor = { 1.0f, 0.2f, 0.0f, glowAlpha };
                float glowSize = 3.0f;
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX - glowSize, diedY), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX + glowSize, diedY), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY - glowSize), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY + glowSize), glowColor);

                // 本体テキスト
                DirectX::XMVECTORF32 diedColor = { 0.95f, 0.15f, 0.05f, textAlpha };
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY), diedColor);
            }
            else
            {
                // fontLargeが無い場合は通常フォント×2倍スケール
                DirectX::XMVECTORF32 diedColor = { 0.95f, 0.15f, 0.05f, textAlpha };
                m_font->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(centerX, diedY), diedColor, 0.0f,
                    DirectX::XMFLOAT2(0.5f, 0.5f), 2.0f);
            }
        }

        // ===================================================================
        //  2カラムレイアウト
        //  左パネル: スタッツ9行   右パネル: ランク + トータルスコア
        // ===================================================================

        // --- パネル座標 ---
        const float leftL = 55.0f;
        const float leftR = 605.0f;
        const float rightL = 645.0f;
        const float rightR = 1225.0f;
        const float rightCX = (rightL + rightR) * 0.5f;
        const float panelTop = 125.0f;
        const float panelBot = 555.0f;

        // --- パネル背景（半透明ダークボックス）---
        if (timer > 0.8f)
        {
            float bgT = min((timer - 0.8f) / 0.4f, 1.0f);
            float bgA = EaseOutCubic(bgT) * 0.2f;

            RECT leftBg = { (LONG)leftL,  (LONG)panelTop, (LONG)leftR,  (LONG)panelBot };
            RECT rightBg = { (LONG)rightL, (LONG)panelTop, (LONG)rightR, (LONG)panelBot };
            DirectX::XMVECTORF32 bgCol = { 0.05f, 0.01f, 0.0f, bgA };
            m_spriteBatch->Draw(m_whitePixel.Get(), leftBg, bgCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), rightBg, bgCol);

            // パネル枠線（暗い赤）
            float borderA = bgT * 0.35f;
            DirectX::XMVECTORF32 borderCol = { 0.5f, 0.1f, 0.0f, borderA };
            RECT bTop1 = { (LONG)leftL, (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + 1) };
            RECT bBot1 = { (LONG)leftL, (LONG)(panelBot - 1), (LONG)leftR, (LONG)panelBot };
            RECT bLft1 = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + 1), (LONG)panelBot };
            RECT bRgt1 = { (LONG)(leftR - 1), (LONG)panelTop, (LONG)leftR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), bTop1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bBot1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bLft1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bRgt1, borderCol);

            RECT bTop2 = { (LONG)rightL, (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + 1) };
            RECT bBot2 = { (LONG)rightL, (LONG)(panelBot - 1), (LONG)rightR, (LONG)panelBot };
            RECT bLft2 = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + 1), (LONG)panelBot };
            RECT bRgt2 = { (LONG)(rightR - 1), (LONG)panelTop, (LONG)rightR, (LONG)panelBot };

            //  コーナー装飾（L字型、各パネルの四隅）
            float cornerLen = 20.0f;
            float cornerThick = 2.0f;
            DirectX::XMVECTORF32 cornerCol = { 0.8f, 0.2f, 0.0f, borderA * 1.5f };

            // 左パネル四隅
            // 左上
            RECT cLT_H = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + cornerLen), (LONG)(panelTop + cornerThick) };
            RECT cLT_V = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + cornerThick), (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), cLT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cLT_V, cornerCol);
            // 右上
            RECT cRT_H = { (LONG)(leftR - cornerLen), (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + cornerThick) };
            RECT cRT_V = { (LONG)(leftR - cornerThick), (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), cRT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cRT_V, cornerCol);
            // 左下
            RECT cLB_H = { (LONG)leftL, (LONG)(panelBot - cornerThick), (LONG)(leftL + cornerLen), (LONG)panelBot };
            RECT cLB_V = { (LONG)leftL, (LONG)(panelBot - cornerLen), (LONG)(leftL + cornerThick), (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), cLB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cLB_V, cornerCol);
            // 右下
            RECT cRB_H = { (LONG)(leftR - cornerLen), (LONG)(panelBot - cornerThick), (LONG)leftR, (LONG)panelBot };
            RECT cRB_V = { (LONG)(leftR - cornerThick), (LONG)(panelBot - cornerLen), (LONG)leftR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), cRB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cRB_V, cornerCol);

            // 右パネル四隅
            RECT c2LT_H = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + cornerLen), (LONG)(panelTop + cornerThick) };
            RECT c2LT_V = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + cornerThick), (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LT_V, cornerCol);
            RECT c2RT_H = { (LONG)(rightR - cornerLen), (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + cornerThick) };
            RECT c2RT_V = { (LONG)(rightR - cornerThick), (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RT_V, cornerCol);
            RECT c2LB_H = { (LONG)rightL, (LONG)(panelBot - cornerThick), (LONG)(rightL + cornerLen), (LONG)panelBot };
            RECT c2LB_V = { (LONG)rightL, (LONG)(panelBot - cornerLen), (LONG)(rightL + cornerThick), (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LB_V, cornerCol);
            RECT c2RB_H = { (LONG)(rightR - cornerLen), (LONG)(panelBot - cornerThick), (LONG)rightR, (LONG)panelBot };
            RECT c2RB_V = { (LONG)(rightR - cornerThick), (LONG)(panelBot - cornerLen), (LONG)rightR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RB_V, cornerCol);

            m_spriteBatch->Draw(m_whitePixel.Get(), bTop2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bBot2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bLft2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bRgt2, borderCol);
        }

        // ======================
        //  左パネル: スタッツ9行
        // ======================
        if (timer > 1.2f)
        {
            const wchar_t* styleRankNames[] = { L"D", L"C", L"B", L"A", L"S", L"SS", L"SSS" };

            const float tableL = leftL + 20.0f;
            const float colVal = leftR - 140.0f;
            const float colBonus = leftR - 20.0f;
            const float statStartY = panelTop + 20.0f;
            const float rowH = 25.0f;

            const wchar_t* labels[] = {
                L"KILLS", L"HEADSHOTS", L"MELEE KILLS", L"MAX COMBO",
                L"PARRIES", L"SURVIVAL", L"DMG DEALT", L"DMG TAKEN", L"BEST STYLE"
            };
            int rawValues[] = {
                m_goKills, m_goHeadshots, m_goMeleeKills, m_goMaxCombo,
                m_goParryCount, 0, m_goDamageDealt, m_goDamageTaken, m_goMaxStyleRank
            };

            for (int i = 0; i < 9; i++)
            {
                float rowStart = 1.2f + i * 0.12f;
                if (timer <= rowStart) continue;

                float t = min((timer - rowStart) / 0.3f, 1.0f);
                float ease = EaseOutCubic(t);
                float alpha = ease;
                float slideUp = (1.0f - ease) * 8.0f;

                float y = statStartY + i * rowH + slideUp;
                
                

                // ラベル（左揃え）
                DirectX::XMVECTORF32 labelColor = { 0.6f, 0.6f, 0.6f, alpha };
                m_font->DrawString(m_spriteBatch.get(), labels[i],
                    DirectX::XMFLOAT2(tableL, y), labelColor);

                // ドットリーダー
                DirectX::XMVECTOR labelSize = m_font->MeasureString(labels[i]);
                float labelEnd = tableL + DirectX::XMVectorGetX(labelSize);
                float dotS = labelEnd + 6.0f;
                float dotE = colVal - 70.0f;
                if (dotE > dotS && m_whitePixel)
                {
                    for (float dx = dotS; dx < dotE; dx += 6.0f)
                    {
                        RECT dot = { (LONG)dx, (LONG)(y + 8), (LONG)(dx + 2), (LONG)(y + 10) };
                        DirectX::XMVECTORF32 dotColor = { 0.35f, 0.35f, 0.35f, alpha * 0.5f };
                        m_spriteBatch->Draw(m_whitePixel.Get(), dot, dotColor);
                    }
                }

                // 値（右揃え）
                wchar_t valBuf[64];
                if (i == 5)
                {
                    int sec = (int)(m_goSurvivalTime * m_goStatCountUp[i]);
                    swprintf_s(valBuf, L"%d:%02d", sec / 60, sec % 60);
                }
                else if (i == 8)
                {
                    int dr = (int)(m_goMaxStyleRank * m_goStatCountUp[i]);
                    if (dr > 6) dr = 6;
                    swprintf_s(valBuf, L"%s", styleRankNames[dr]);
                }
                else
                {
                    swprintf_s(valBuf, L"%d", (int)(rawValues[i] * m_goStatCountUp[i]));
                }

                DirectX::XMVECTORF32 valColor = { 1.0f, 1.0f, 1.0f, alpha };
                DirectX::XMVECTOR valSize = m_font->MeasureString(valBuf);
                float valW = DirectX::XMVectorGetX(valSize);
                m_font->DrawString(m_spriteBatch.get(), valBuf,
                    DirectX::XMFLOAT2(colVal - valW, y), valColor);

                // ボーナスポイント（右揃え、金色）
                int dispScore = (int)(m_goStatScores[i] * m_goStatCountUp[i]);
                wchar_t sBuf[64];
                swprintf_s(sBuf, L"+%d", dispScore);
                DirectX::XMVECTORF32 bonusColor = { 1.0f, 0.85f, 0.2f, alpha };
                DirectX::XMVECTOR sSize = m_font->MeasureString(sBuf);
                float sW = DirectX::XMVectorGetX(sSize);
                m_font->DrawString(m_spriteBatch.get(), sBuf,
                    DirectX::XMFLOAT2(colBonus - sW, y), bonusColor);
            }

            // --- 左パネル下部: WAVE表示 ---
            float waveStart = 1.2f + 9 * 0.12f + 0.2f;
            if (timer > waveStart)
            {
                float wt = min((timer - waveStart) / 0.4f, 1.0f);
                float wa = EaseOutCubic(wt);

                float waveY = statStartY + 9 * rowH + 20.0f;
                wchar_t waveBuf[64];
                swprintf_s(waveBuf, L"WAVE  %d", m_gameOverWave);
                DirectX::XMVECTORF32 waveCol = { 0.8f, 0.3f, 0.1f, wa };
                m_font->DrawString(m_spriteBatch.get(), waveBuf,
                    DirectX::XMFLOAT2(tableL, waveY), waveCol);
            }
        }

        // =====================================
        //  右パネル: ランク + トータルスコア
        // =====================================

        // --- 「RANK」ラベル ---
        if (timer > 2.0f)
        {
            float rlT = min((timer - 2.0f) / 0.3f, 1.0f);
            float rlA = EaseOutCubic(rlT);
            DirectX::XMVECTORF32 rlCol = { 0.5f, 0.5f, 0.5f, rlA * 0.7f };
            DirectX::XMVECTOR rlSize = m_font->MeasureString(L"RANK");
            float rlW = DirectX::XMVectorGetX(rlSize);
            m_font->DrawString(m_spriteBatch.get(), L"RANK",
                DirectX::XMFLOAT2(rightCX - rlW * 0.5f, panelTop + 25.0f), rlCol);
        }

        // // ランク登場フラッシュ（文字の形に沿って放射状に光る）
        if (timer > 2.15f && timer < 4.0f && m_fontLarge)
        {
            float flashT = (timer - 2.15f) / 1.85f;  // 0→1（1.85秒かけてゆっくり減衰）
            float flashA = (1.0f - flashT) * (1.0f - flashT);
            float intensity = (m_gameOverRank >= 2) ? 1.0f : 0.5f;

            const wchar_t* rankNames[] = { L"C", L"B", L"A", L"S" };
            const wchar_t* rn = rankNames[m_gameOverRank];
            float rankY = panelTop + 70.0f;

            DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rn);
            float rW = DirectX::XMVectorGetX(rSize);
            float rH = DirectX::XMVectorGetY(rSize);
            float rX = rightCX - rW * 0.5f;

            // フラッシュ色
            DirectX::XMFLOAT3 flashRGB;
            if (m_gameOverRank == 3)      flashRGB = { 1.0f, 0.4f, 0.1f };
            else if (m_gameOverRank == 2) flashRGB = { 1.0f, 0.9f, 0.3f };
            else if (m_gameOverRank == 1) flashRGB = { 0.5f, 0.7f, 1.0f };
            else                          flashRGB = { 0.8f, 0.8f, 0.8f };

            // === 外側レイヤー: 16方向に大きく放射 ===
            float outerDist = 12.0f + (1.0f - flashT) * 60.0f;
            float outerDirs[][2] = {
                {1,0},{-1,0},{0,1},{0,-1},
                {0.707f,0.707f},{-0.707f,0.707f},{0.707f,-0.707f},{-0.707f,-0.707f},
                {0.92f,0.38f},{-0.92f,0.38f},{0.92f,-0.38f},{-0.92f,-0.38f},
                {0.38f,0.92f},{-0.38f,0.92f},{0.38f,-0.92f},{-0.38f,-0.92f}
            };
            for (int d = 0; d < 16; d++)
            {
                float dx = outerDirs[d][0] * outerDist;
                float dy = outerDirs[d][1] * outerDist;
                DirectX::XMVECTORF32 gc = { flashRGB.x, flashRGB.y, flashRGB.z, flashA * intensity * 0.2f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === 中間レイヤー: 8方向 ===
            float midDist = 5.0f + (1.0f - flashT) * 30.0f;
            for (int d = 0; d < 8; d++)
            {
                float dx = outerDirs[d][0] * midDist;
                float dy = outerDirs[d][1] * midDist;
                DirectX::XMVECTORF32 gc = { flashRGB.x, flashRGB.y, flashRGB.z, flashA * intensity * 0.35f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === 内側レイヤー: 8方向（明るい）===
            float innerDist = 2.0f + (1.0f - flashT) * 12.0f;
            for (int d = 0; d < 8; d++)
            {
                float dx = outerDirs[d][0] * innerDist;
                float dy = outerDirs[d][1] * innerDist;
                DirectX::XMVECTORF32 gc = {
                    min(flashRGB.x + 0.3f, 1.0f),
                    min(flashRGB.y + 0.3f, 1.0f),
                    min(flashRGB.z + 0.2f, 1.0f),
                    flashA * intensity * 0.5f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === S/Aランク: 背景にぼんやり光の円 ===
            if (m_gameOverRank >= 2 && m_whitePixel)
            {
                float glowRadius = 80.0f + (1.0f - flashT) * 120.0f;
                float cx = rightCX;
                float cy = rankY + rH * 0.5f;
                // 同心円的に3層の光
                for (int ring = 0; ring < 3; ring++)
                {
                    float r = glowRadius * (0.4f + ring * 0.3f);
                    float ringA = flashA * intensity * 0.08f / (ring + 1);
                    RECT glowRect = {
                        (LONG)(cx - r), (LONG)(cy - r),
                        (LONG)(cx + r), (LONG)(cy + r)
                    };
                    DirectX::XMVECTORF32 glowCol = { flashRGB.x, flashRGB.y, flashRGB.z, ringA };
                    m_spriteBatch->Draw(m_whitePixel.Get(), glowRect, glowCol);
                }
            }
        }


        // --- ランク文字（大きく中央に）---
        if (timer > 2.2f)
        {
            float rankT = min((timer - 2.2f) / 0.5f, 1.0f);
            float rankEase = EaseOutBack(rankT);
            float rankAlpha = min((timer - 2.2f) * 3.0f, 1.0f);

            const wchar_t* rankNames[] = { L"C", L"B", L"A", L"S" };
            const wchar_t* rankName = rankNames[m_gameOverRank];

            DirectX::XMVECTORF32 rankColors[] = {
                { 0.5f, 0.5f, 0.5f, rankAlpha },
                { 0.3f, 0.5f, 1.0f, rankAlpha },
                { 1.0f, 0.85f, 0.0f, rankAlpha },
                { 1.0f, 0.2f, 0.0f, rankAlpha },
            };
            DirectX::XMVECTORF32 rankColor = rankColors[m_gameOverRank];

            float rankY = panelTop + 70.0f;

            if (m_fontLarge)
            {
                // グロー（A/Sランク）
                if (m_gameOverRank >= 2)
                {
                    float glowPulse = (sinf(timer * 4.0f) + 1.0f) * 0.5f;
                    DirectX::XMVECTORF32 glowColor = { rankColor.f[0], rankColor.f[1], rankColor.f[2],
                        rankAlpha * 0.2f * (0.5f + glowPulse * 0.5f) };
                    float glow = 5.0f;
                    DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rankName);
                    float rW = DirectX::XMVectorGetX(rSize);
                    float rH = DirectX::XMVectorGetY(rSize);
                    float rX = rightCX - rW * 0.5f;
                    DirectX::XMFLOAT2 glowOrigin(rW * 0.5f, rH * 0.5f);
                    DirectX::XMFLOAT2 glowCenter(rightCX, rankY + rH * 0.5f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x - glow, glowCenter.y), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x + glow, glowCenter.y), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x, glowCenter.y - glow), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x, glowCenter.y + glow), glowColor, 0.0f, glowOrigin, 2.0f);
                }

                DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rankName);
                float rW = DirectX::XMVectorGetX(rSize);
                float rH = DirectX::XMVectorGetY(rSize);
                // 2倍スケールで描画（originを中央にして拡大）
                DirectX::XMFLOAT2 origin(rW * 0.5f, rH * 0.5f);
                DirectX::XMFLOAT2 pos(rightCX, rankY + rH * 0.5f);
                m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                    pos, rankColor, 0.0f, origin, 2.0f);
            }
            else
            {
                DirectX::XMVECTOR rSize = m_font->MeasureString(rankName);
                float rW = DirectX::XMVectorGetX(rSize);
                m_font->DrawString(m_spriteBatch.get(), rankName,
                    DirectX::XMFLOAT2(rightCX - rW * 0.5f, rankY), rankColor);
            }
        }

        // --- 右パネル: 区切り線 ---
        if (timer > 2.5f)
        {
            float sepT = min((timer - 2.5f) / 0.4f, 1.0f);
            float sepE = EaseOutCubic(sepT);
            float sepY = panelTop + 220.0f;
            float sepHW = 200.0f * sepE;
            RECT sepLine = {
                (LONG)(rightCX - sepHW), (LONG)sepY,
                (LONG)(rightCX + sepHW), (LONG)(sepY + 1)
            };
            DirectX::XMVECTORF32 sepCol = { 0.5f, 0.12f, 0.0f, sepE * 0.6f };
            m_spriteBatch->Draw(m_whitePixel.Get(), sepLine, sepCol);
        }

        // --- 右パネル: TOTAL SCORE ---
        if (timer > 2.7f)
        {
            float tsT = min((timer - 2.7f) / 0.4f, 1.0f);
            float tsA = EaseOutCubic(tsT);

            float scoreY = panelTop + 240.0f;
            DirectX::XMVECTORF32 tsLabelCol = { 0.6f, 0.6f, 0.6f, tsA * 0.8f };
            DirectX::XMVECTOR tsLabelSize = m_font->MeasureString(L"TOTAL SCORE");
            float tsLabelW = DirectX::XMVectorGetX(tsLabelSize);
            m_font->DrawString(m_spriteBatch.get(), L"TOTAL SCORE",
                DirectX::XMFLOAT2(rightCX - tsLabelW * 0.5f, scoreY), tsLabelCol);

            int dispTotal = (int)(m_goTotalScore * m_gameOverCountUp);
            wchar_t tBuf[64];
            swprintf_s(tBuf, L"%d", dispTotal);
            DirectX::XMVECTORF32 tCol = { 1.0f, 0.85f, 0.0f, tsA };
            float numY = scoreY + 25.0f;

            if (m_fontLarge)
            {
                DirectX::XMVECTOR ts = m_fontLarge->MeasureString(tBuf);
                float tw = DirectX::XMVectorGetX(ts);
                m_fontLarge->DrawString(m_spriteBatch.get(), tBuf,
                    DirectX::XMFLOAT2(rightCX - tw * 0.5f, numY), tCol);
            }
            else
            {
                DirectX::XMVECTOR ts = m_font->MeasureString(tBuf);
                float tw = DirectX::XMVectorGetX(ts);
                m_font->DrawString(m_spriteBatch.get(), tBuf,
                    DirectX::XMFLOAT2(rightCX - tw * 0.5f, numY), tCol);
            }
        }

        // --- パーティクル（A/Sランクで右パネル周辺に火の粉）---
        if (timer > 3.5f && m_gameOverRank >= 2 && m_whitePixel)
        {
            float particleAlpha = min((timer - 3.5f) * 2.0f, 1.0f);
            int numParticles = (m_gameOverRank == 3) ? 30 : 15;

            for (int i = 0; i < numParticles; i++)
            {
                float seed1 = sinf((float)i * 127.1f + timer * 0.7f) * 0.5f + 0.5f;
                float seed2 = cosf((float)i * 311.7f + timer * 0.5f) * 0.5f + 0.5f;
                float seed3 = sinf((float)i * 269.5f + timer * 1.3f) * 0.5f + 0.5f;

                float px = rightCX + (seed1 - 0.5f) * 500.0f;
                float py = panelTop + seed2 * (panelBot - panelTop);
                float pSize = 2.0f + seed3 * 4.0f;

                float flickerAlpha = particleAlpha * (0.3f + seed3 * 0.7f);
                DirectX::XMVECTORF32 pColor = {
                    0.9f + seed1 * 0.1f,
                    0.3f + seed2 * 0.4f,
                    0.0f,
                    flickerAlpha
                };

                RECT pRect = {
                    (LONG)(px - pSize * 0.5f), (LONG)(py - pSize * 0.5f),
                    (LONG)(px + pSize * 0.5f), (LONG)(py + pSize * 0.5f)
                };
                m_spriteBatch->Draw(m_whitePixel.Get(), pRect, pColor);
            }
        }

        // --- Press R to Restart（右パネル下）---
        if (timer > 4.0f && m_rankingSaved)
        {
            float restartT = min((timer - 4.0f) / 0.3f, 1.0f);
            float restartAlpha = EaseOutCubic(restartT);
            float blink = (sinf(timer * 3.0f) + 1.0f) * 0.5f;
            float finalAlpha = restartAlpha * (0.4f + blink * 0.6f);

            DirectX::XMVECTORF32 restartColor = { 0.6f, 0.6f, 0.6f, finalAlpha };
            DirectX::XMVECTOR restartSize = m_font->MeasureString(L"Press R to Restart");
            float restartW = DirectX::XMVectorGetX(restartSize);
            m_font->DrawString(m_spriteBatch.get(), L"Press R to Restart",
                DirectX::XMFLOAT2(rightCX - restartW * 0.5f, panelBot + 15.0f), restartColor);
        }

        // --- L: RANKING 案内テキスト ---
        if (timer > 4.0f && m_rankingSaved)
        {
            float lT = min((timer - 4.0f) / 0.3f, 1.0f);
            float lAlpha = EaseOutCubic(lT);
            float lBlink = (sinf(timer * 2.5f) + 1.0f) * 0.5f;
            float lFinal = lAlpha * (0.3f + lBlink * 0.5f);

            DirectX::XMVECTORF32 lColor = { 0.8f, 0.6f, 0.2f, lFinal };
            DirectX::XMVECTOR lSize = m_font->MeasureString(L"L: RANKING");
            float lW = DirectX::XMVectorGetX(lSize);
            m_font->DrawString(m_spriteBatch.get(), L"L: RANKING",
                DirectX::XMFLOAT2(rightCX - lW * 0.5f, panelBot + 40.0f), lColor);
        }

        // =============================================
        // 名前入力 UI
        // =============================================
        if (m_nameInputActive)
        {
            float inputY = panelBot + 75.0f;
            float inputCenterX = rightCX;

            // --- 入力ボックス背景 ---
            RECT inputBg = {
                (LONG)(inputCenterX - 140), (LONG)(inputY - 5),
                (LONG)(inputCenterX + 140), (LONG)(inputY + 35)
            };
            DirectX::XMVECTORF32 bgColor = { 0.0f, 0.0f, 0.0f, 0.7f };
            m_spriteBatch->Draw(m_whitePixel.Get(), inputBg, bgColor);

            // --- 入力ボックス枠 ---
            float borderPulse = sinf(timer * 3.0f) * 0.3f + 0.7f;
            DirectX::XMVECTORF32 borderColor = { 0.9f, 0.6f, 0.1f, borderPulse };

            // 上枠
            RECT borderTop = { inputBg.left, inputBg.top, inputBg.right, inputBg.top + 2 };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderTop, borderColor);
            // 下枠
            RECT borderBot = { inputBg.left, inputBg.bottom - 2, inputBg.right, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderBot, borderColor);
            // 左枠
            RECT borderL = { inputBg.left, inputBg.top, inputBg.left + 2, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderL, borderColor);
            // 右枠
            RECT borderR = { inputBg.right - 2, inputBg.top, inputBg.right, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderR, borderColor);

            // --- "ENTER YOUR NAME" ラベル ---
            DirectX::XMVECTORF32 labelColor = { 1.0f, 0.8f, 0.3f, 1.0f };
            DirectX::XMVECTOR labelSize = m_font->MeasureString(L"ENTER YOUR NAME");
            float labelW = DirectX::XMVectorGetX(labelSize);
            m_font->DrawString(m_spriteBatch.get(), L"ENTER YOUR NAME",
                DirectX::XMFLOAT2(inputCenterX - labelW * 0.5f, inputY - 28.0f), labelColor);

            // --- 入力テキスト表示 ---
            wchar_t wideNameBuf[32] = {};
            for (int c = 0; c < m_nameLength && c < 15; c++)
                wideNameBuf[c] = (wchar_t)m_nameBuffer[c];
            wideNameBuf[m_nameLength] = L'\0';

            // 点滅カーソルを追加
            float cursorBlink = sinf(timer * 6.0f);
            if (cursorBlink > 0.0f && m_nameLength < 15)
            {
                wideNameBuf[m_nameLength] = L'_';
                wideNameBuf[m_nameLength + 1] = L'\0';
            }

            DirectX::XMVECTORF32 nameColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            DirectX::XMVECTOR nameSize = m_font->MeasureString(wideNameBuf);
            float nameW = DirectX::XMVectorGetX(nameSize);
            m_font->DrawString(m_spriteBatch.get(), wideNameBuf,
                DirectX::XMFLOAT2(inputCenterX - nameW * 0.5f, inputY + 3.0f), nameColor);

            // --- "PRESS ENTER TO CONFIRM" ---
            DirectX::XMVECTORF32 hintColor = { 0.6f, 0.6f, 0.5f, 0.6f + sinf(timer * 2.0f) * 0.2f };
            DirectX::XMVECTOR hintSize = m_font->MeasureString(L"ENTER: OK    BACKSPACE: DELETE");
            float hintW = DirectX::XMVectorGetX(hintSize);
            m_font->DrawString(m_spriteBatch.get(), L"ENTER: OK    BACKSPACE: DELETE",
                DirectX::XMFLOAT2(inputCenterX - hintW * 0.5f, inputY + 42.0f), hintColor);
        }

        m_spriteBatch->End();
    }
}

void Game::RenderFade()
{
    if (m_fadeAlpha <= 0.0f)
        return;

    // フェード用の2D描画設定
    auto context = m_d3dContext.Get();

    // 2D用の単位行列
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());

    // 頂点カラーを無効化して単色描画モードに
    m_effect->SetVertexColorEnabled(false);
    DirectX::XMVECTOR diffuseColor = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    m_effect->SetDiffuseColor(diffuseColor);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // フルスクリーンの四角形を描画
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    DirectX::XMFLOAT4 fadeColor(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面全体を覆う四角形
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, -halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(halfWidth, -halfHeight, 1.0f), fadeColor);

    // 三角形2つで四角形を構成
    primitiveBatch->DrawTriangle(v1, v2, v3);
    primitiveBatch->DrawTriangle(v1, v3, v4);
 
    primitiveBatch->End();

    // 頂点カラーを再度有効化
    m_effect->SetVertexColorEnabled(true);
}

void Game::DrawWeaponSpawns()
{
    if (!m_weaponSpawnSystem)
        return;

    //  ビュー・プロジェクション行列
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y -  sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
    DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );


    //  === 全ての壁武器を描画   ===
    for (const auto& spawn : m_weaponSpawnSystem->GetSpawns())
    {
        //  ワールド行列
        DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            DirectX::XMMatrixTranslation(spawn.position.x, spawn.position.y, spawn.position.z);

        //  色(武騎手によって変える)
        DirectX::XMVECTOR color;
        switch (spawn.weaponType)
        {
        case WeaponType::SHOTGUN:
            color = DirectX::Colors::Orange;
            break;

        case WeaponType::RIFLE:
            color = DirectX::Colors::Green;
            break;

        case WeaponType::SNIPER:
            color = DirectX::Colors::Blue;
            break;

        default:
            color = DirectX::Colors::Gray;
            break;
        }

        m_cube->Draw(world, viewMatrix, projectionMatrix, color);
        // 肉片描画
        DrawGibs(viewMatrix, projectionMatrix);
    }

}

void Game::InitImGui()
{
    // ImGui初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // スタイル設定
    ImGui::StyleColorsDark();

    // Win32 + DX11 バックエンド初期化
    ImGui_ImplWin32_Init(m_window);
    ImGui_ImplDX11_Init(m_d3dDevice.Get(), m_d3dContext.Get());

    //OutputDebugStringA("ImGui initialized successfully!\n");
}

void Game::ShutdownImGui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}



void Game::UpdateGloryKillAnimation(float deltaTime)
{
    if (!m_gloryKillArmAnimActive)
        return;

    // アニメーション時間を進める
    m_gloryKillArmAnimTime += deltaTime / GLORY_KILL_ANIM_DURATION;

    if (m_gloryKillArmAnimTime >= 1.0f)
    {
        // アニメーション終了
        m_gloryKillArmAnimActive = false;
        m_gloryKillArmAnimTime = 0.0f;
        m_gloryKillArmAnimRot = { 0.0f, 0.0f, 0.0f };
        m_gloryKillKnifeAnimRot = { 0.0f, 0.0f, 0.0f };
        return;
    }

    float t = m_gloryKillArmAnimTime;

    // === フェーズ分け ===
    // Phase 1: 構え (0.0 ~ 0.2) - 腕を右に引いて横向きに
    // Phase 2: 溜め (0.2 ~ 0.35) - 止まる
    // Phase 3: 突き刺し (0.35 ~ 0.5) - 素早く左へ突く
    // Phase 4: 停止 (0.5 ~ 0.75) - 刺さった状態
    // Phase 5: 引き抜き (0.75 ~ 1.0) - 戻す

    float armRotY = 0.0f;   // 腕の横方向回転
    float armRotZ = 0.0f;   // 腕の傾き
    float knifeRotX = 0.0f; // ナイフの角度

    if (t < 0.2f)
    {
        // Phase 1: 構え - 腕を右に引く
        float phase = t / 0.2f;  // 0→1
        phase = phase * phase;   // イージング（加速）

        armRotY = phase * 0.8f;  // 右に引く
        armRotZ = phase * 0.3f;  // 少し傾ける
        knifeRotX = phase * (-0.5f); // ナイフを横向きに
    }
    else if (t < 0.35f)
    {
        // Phase 2: 溜め - 止まる（緊張感）
        armRotY = 0.8f;
        armRotZ = 0.3f;
        knifeRotX = -0.5f;
    }
    else if (t < 0.5f)
    {
        // Phase 3: 突き刺し！ - 素早く左へ
        float phase = (t - 0.35f) / 0.15f;  // 0→1
        phase = 1.0f - (1.0f - phase) * (1.0f - phase);  // イージング（減速）

        armRotY = 0.8f - phase * 1.6f;  // 右から左へ振る（0.8 → -0.8）
        armRotZ = 0.3f - phase * 0.5f;  // 傾きを戻しながら
        knifeRotX = -0.5f + phase * 0.3f;
    }
    else if (t < 0.75f)
    {
        // Phase 4: 停止 - 刺さった状態で止まる
        armRotY = -0.8f;
        armRotZ = -0.2f;
        knifeRotX = -0.2f;
    }
    else
    {
        // Phase 5: 引き抜き - 元に戻す
        float phase = (t - 0.75f) / 0.25f;  // 0→1
        phase = phase * phase;  // イージング

        armRotY = -0.8f * (1.0f - phase);
        armRotZ = -0.2f * (1.0f - phase);
        knifeRotX = -0.2f * (1.0f - phase);
    }

    // AnimRot に適用
    m_gloryKillArmAnimRot.y = armRotY;
    m_gloryKillArmAnimRot.z = armRotZ;
    m_gloryKillKnifeAnimRot.x = knifeRotX;
}

void Game::PerformMeleeAttack()
{
    using namespace DirectX;

    //  プレイヤーの位置と向きを取得
    XMFLOAT3 playerPos = m_player->GetPosition();
    float cameraRotY = m_player->GetCameraRotationY();  //  Y軸回転(左右)

    //  前方ベクトルを計算
    // カメラの向きから前方ベクトルを作成
    XMFLOAT3 forward;
    forward.x = sinf(cameraRotY);   //  X方向成分
    forward.y = 0.0f;   //  Y方向は０（水平方向のみ）
    forward.z = cosf(cameraRotY);   //  Z方向成分

    //  レイキャストの開始位置と方向
    const float meleeRange = 2.0f;  //  近接攻撃の射程(2メートル)

    //  レイキャストを実行(Bullet Physic)
    RaycastResult result = RaycastPhysics(
        playerPos,  // 開始位置
        forward,    // 方向
        meleeRange  // 最大距離
    );

    //  敵に当たったか確認
    if (result.hit && result.hitEnemy)
    {
        Enemy* hitEnemy = result.hitEnemy;

        //  グローリー判定(敵がよろめいてる状態)
        if (hitEnemy->isStaggered)
        {
            //  グローリーキル
            //OutputDebugStringA("[GLORY KILL] GLORY KILL EXECUTED!\n");

            //  アニメーション開始
            m_gloryKillArmAnimActive = true;
            m_gloryKillArmAnimTime = 0.0f;

            //  即死させる
            hitEnemy->health = 0;

            //  弾薬を50%回復（上限あり）
            WeaponType currentWeapon = m_weaponSystem->GetCurrentWeapon();
            WeaponData weaponData = m_weaponSystem->GetWeaponData(currentWeapon);

            int currentAmmo = m_weaponSystem->GetReserveAmmo();
            int maxReserve = weaponData.reserveAmmo;
            int ammoReward = (int)(maxReserve * 0.5f);  // 50%回復
            int newAmmo = currentAmmo + ammoReward;
            if (newAmmo > maxReserve) newAmmo = maxReserve;  // 上限

            m_weaponSystem->SetReserveAmmo(newAmmo);

            /*char buffer[128];
            sprintf_s(buffer, "[GLORY KILL] Ammo recovered: +%d (Total: %d/%d)\n",
                ammoReward, newAmmo, maxReserve);*/
            //OutputDebugStringA(buffer);


            //  カメラシェイク
            m_cameraShake = 0.3f;   //  通常の２倍
            m_cameraShakeTimer = 0.3f;

            //  ヒットストップ
            m_hitStopTimer = 0.1f;  //  通常の２倍

            //  スローモーション
            m_timeScale = 0.2f; //  20%スロー
            m_slowMoTimer = 0.8f;   //  0.8秒間

            //  パーティクル
            if (m_particleSystem)
            {
                XMFLOAT3 bloodDir(0.0f, 1.0f, 0.0f);

                /*m_particleSystem->CreateBloodEffect(
                    hitEnemy->position,
                    bloodDir,
                    150
                );*/
            }
        }
        else
        {
            const float normalMeleeDamage = 25.0f;  // 小ダメージ

            int oldHealth = hitEnemy->health;
            hitEnemy->health -= (int)normalMeleeDamage;
            m_statDamageDealt += (int)normalMeleeDamage;  //  与ダメ記録

            // 弾補充（上限あり）
            int currentAmmo = m_weaponSystem->GetReserveAmmo();
            WeaponData wd = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());
            int newAmmo = currentAmmo + m_meleeAmmoRefill;
            if (newAmmo > wd.reserveAmmo) newAmmo = wd.reserveAmmo;
            m_weaponSystem->SetReserveAmmo(newAmmo);
            /*char buffer[128];
            sprintf_s(buffer, "[MELEE] Normal hit! Damage:%.0f HP:%d->%d\n",
                normalMeleeDamage, oldHealth, hitEnemy->health);*/
            //OutputDebugStringA(buffer);

            // 軽いフィードバック
            m_cameraShake = 0.08f;
            m_cameraShakeTimer = 0.15f;

            m_hitStopTimer = 0.03f;

            // 少量のパーティクル
            if (m_particleSystem)
            {
                XMFLOAT3 bloodDir(0.0f, 1.0f, 0.0f);

                //m_particleSystem->CreateBloodEffect(
                //    hitEnemy->position,
                //    bloodDir,
                //    30  // 少量
                //);
            }
        }
    }
    else
    {
        //  空振り
        //OutputDebugStringA("[MELEE] Missed!\n");

        m_cameraShake = 0.05f;
        m_cameraShakeTimer = 0.1f;
    }

}

// =================================================================
// CreateBlurResources - ガウシアンブラー用のリソースを作成
// =================================================================
void Game::CreateBlurResources()
{
    // === オフスクリーンレンダーターゲット作成 ===
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_outputWidth;
    texDesc.Height = m_outputHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    HRESULT hr = m_d3dDevice->CreateTexture2D(&texDesc, nullptr, m_offscreenTexture.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create offscreen texture");

    hr = m_d3dDevice->CreateRenderTargetView(m_offscreenTexture.Get(), nullptr, m_offscreenRTV.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create offscreen RTV");

    hr = m_d3dDevice->CreateShaderResourceView(m_offscreenTexture.Get(), nullptr, m_offscreenSRV.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create offscreen SRV");

    //  流体レンダリング用シーンコピーテクスチャ
    {
        D3D11_TEXTURE2D_DESC copyDesc = texDesc;  // offscreenと同じ設定をコピー
        copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;  // SRVだけでOK(RTVは不要)
        hr = m_d3dDevice->CreateTexture2D(&copyDesc, nullptr, m_sceneCopyTex.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("Failed to create scene copy texture");

        hr = m_d3dDevice->CreateShaderResourceView(m_sceneCopyTex.Get(), nullptr, m_sceneCopySRV.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("Failed to create scene copy SRV");

        OutputDebugStringA("[Fluid] Scene copy texture created\n");
    }

    // === 定数バッファ作成 ===
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(BlurParams);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_blurConstantBuffer.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create blur constant buffer");

    // === サンプラー作成 ===
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_d3dDevice->CreateSamplerState(&samplerDesc, m_linearSampler.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create sampler state");

    // オフスクリーン用深度バッファ作成
    CD3D11_TEXTURE2D_DESC offscreenDepthDesc(
        DXGI_FORMAT_R24G8_TYPELESS,
        m_outputWidth,
        m_outputHeight,
        1,
        1,
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE
    );

    hr = m_d3dDevice->CreateTexture2D(
        &offscreenDepthDesc,
        nullptr,
        m_offscreenDepthTexture.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) throw std::runtime_error("Failed to create offscreen depth texture");

    // オフスクリーン用深度ステンシルビュー作成
    CD3D11_DEPTH_STENCIL_VIEW_DESC offscreenDSVDesc(
        D3D11_DSV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_D24_UNORM_S8_UINT
    );

    hr = m_d3dDevice->CreateDepthStencilView(
        m_offscreenDepthTexture.Get(),
        &offscreenDSVDesc,
        m_offscreenDepthStencilView.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) throw std::runtime_error("Failed to create offscreen DSV");

    // オフスクリーン用深度SRV作成
    CD3D11_SHADER_RESOURCE_VIEW_DESC offscreenDepthSRVDesc(
        D3D11_SRV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    );

    hr = m_d3dDevice->CreateShaderResourceView(
        m_offscreenDepthTexture.Get(),
        &offscreenDepthSRVDesc,
        m_offscreenDepthSRV.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) throw std::runtime_error("Failed to create offscreen depth SRV");

    /*char debugOffscreenDepth[256];
    sprintf_s(debugOffscreenDepth, "[OFFSCREEN DEPTH] Created: %p\n", m_offscreenDepthSRV.Get());*/
    //OutputDebugStringA(debugOffscreenDepth);

    // === シェーダー読み込み ===
    m_fullscreenVS = LoadVertexShader(L"FullscreenVS.cso");
    m_blurPS = LoadPixelShader(L"GaussianBlur.cso");

    //OutputDebugStringA("[BLUR] Blur resources created successfully\n");
}

// =================================================================
// LoadVertexShader - 頂点シェーダーをコンパイル＆読み込み
// =================================================================
ID3D11VertexShader* Game::LoadVertexShader(const wchar_t* filename)
{
    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

    // Load compiled shader (.cso file)
    HRESULT hr = D3DReadFileToBlob(filename, shaderBlob.GetAddressOf());

    if (FAILED(hr))
    {
        /*char errorMsg[256];
        sprintf_s(errorMsg, "Failed to load vertex shader: %ls\n", filename);*/
        //OutputDebugStringA(errorMsg);
        throw std::runtime_error("Failed to load vertex shader");
    }

    ID3D11VertexShader* shader = nullptr;
    hr = m_d3dDevice->CreateVertexShader(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        nullptr,
        &shader
    );

    if (FAILED(hr))
        throw std::runtime_error("Failed to create vertex shader");

    return shader;
}
// =================================================================
// LoadPixelShader - ピクセルシェーダーをコンパイル＆読み込み
// =================================================================
ID3D11PixelShader* Game::LoadPixelShader(const wchar_t* filename)
{
    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

    // Load compiled shader (.cso file)
    HRESULT hr = D3DReadFileToBlob(filename, shaderBlob.GetAddressOf());

    if (FAILED(hr))
    {
        /*char errorMsg[256];
        sprintf_s(errorMsg, "Failed to load pixel shader: %ls\n", filename);*/
        //OutputDebugStringA(errorMsg);
        throw std::runtime_error("Failed to load pixel shader");
    }

    ID3D11PixelShader* shader = nullptr;
    hr = m_d3dDevice->CreatePixelShader(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        nullptr,
        &shader
    );

    if (FAILED(hr))
        throw std::runtime_error("Failed to create pixel shader");

    return shader;
}

void Game::DrawFullscreenQuad()
{
    // 頂点バッファ不要（頂点シェーダーで自動生成）
    m_d3dContext->IASetInputLayout(nullptr);
    m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点シェーダーとピクセルシェーダーを設定
    m_d3dContext->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
    m_d3dContext->PSSetShader(m_blurPS.Get(), nullptr, 0);

    // 3頂点で画面全体を覆う三角形を描画
    m_d3dContext->Draw(3, 0);

    // シェーダーをクリア
    m_d3dContext->VSSetShader(nullptr, nullptr, 0);
    m_d3dContext->PSSetShader(nullptr, nullptr, 0);
}

// ============================================================
//  回復アイテムをランダム位置にスポーン
// ============================================================
void Game::SpawnHealthPickup()
{
    int activeCount = 0;
    for (auto& p : m_healthPickups)
        if (p.isActive) activeCount++;
    if (activeCount >= m_maxPickups) return;

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    HealthPickup pickup;

    float angle = ((float)rand() / RAND_MAX) * 6.2832f;
    float dist = 8.0f + ((float)rand() / RAND_MAX) * 17.0f;

    pickup.position.x = playerPos.x + cosf(angle) * dist;
    pickup.position.y = GetMeshFloorHeight(pickup.position.x, pickup.position.z, 0.0f) + 0.5f;
    pickup.position.z = playerPos.z + sinf(angle) * dist;

    //  マップ内にクランプ（歩行エリア内のみ）
    pickup.position.x = (std::max)(-31.0f, (std::min)(42.0f, pickup.position.x));
    pickup.position.z = (std::max)(-8.0f, (std::min)(22.0f, pickup.position.z));

    pickup.healAmount = 20 + (rand() % 3) * 5;
    pickup.lifetime = 30.0f;
    pickup.maxLifetime = 30.0f;
    pickup.bobTimer = ((float)rand() / RAND_MAX) * 6.28f;
    pickup.spinAngle = 0.0f;
    pickup.isActive = true;

    m_healthPickups.push_back(pickup);
}

// ============================================================
//  回復アイテムの更新（寿命・拾い判定・演出）
// ============================================================
void Game::UpdateHealthPickups(float deltaTime)
{
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    for (auto& pickup : m_healthPickups)
    {
        if (!pickup.isActive) continue;

        // 寿命カウントダウン
        pickup.lifetime -= deltaTime;
        if (pickup.lifetime <= 0.0f)
        {
            pickup.isActive = false;
            continue;
        }

        // ボブ＆回転演出
        pickup.bobTimer += deltaTime * 3.0f;
        pickup.spinAngle += deltaTime * 2.0f;

        // === 拾い判定 ===
        float dx = playerPos.x - pickup.position.x;
        float dz = playerPos.z - pickup.position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < m_pickupRadius * m_pickupRadius)
        {
            int currentHP = m_player->GetHealth();
            int maxHP = m_player->GetMaxHealth();

            // 最大HPなら拾わない（もったいないから残す）
            if (currentHP < maxHP)
            {
                int newHP = (std::min)(currentHP + pickup.healAmount, maxHP);
                m_player->SetHealth(newHP);
                pickup.isActive = false;

                // 拾った演出：軽いカメラシェイク（心地よい）
                m_cameraShake = 0.02f;
                m_cameraShakeTimer = 0.1f;
            }
        }
    }

    // 死んだアイテムを削除
    m_healthPickups.erase(
        std::remove_if(m_healthPickups.begin(), m_healthPickups.end(),
            [](const HealthPickup& p) { return !p.isActive; }),
        m_healthPickups.end());

    // 定期スポーン
    m_pickupSpawnTimer += deltaTime;
    if (m_pickupSpawnTimer >= m_pickupSpawnInterval)
    {
        m_pickupSpawnTimer = 0.0f;
        SpawnHealthPickup();
    }
}

// ============================================================
//  回復アイテム描画（緑の十字ビルボード）
// ============================================================
void Game::DrawHealthPickups(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
{
    if (m_healthPickups.empty()) return;

    auto* context = m_d3dContext.Get();

    m_effect->SetView(view);
    m_effect->SetProjection(proj);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    batch->Begin();

    // カメラのRight/Upベクトル取得
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, view);
    DirectX::XMFLOAT4X4 ivF;
    DirectX::XMStoreFloat4x4(&ivF, invView);
    DirectX::XMFLOAT3 camRight(ivF._11, ivF._12, ivF._13);
    DirectX::XMFLOAT3 camUp(ivF._21, ivF._22, ivF._23);

    for (auto& pickup : m_healthPickups)
    {
        if (!pickup.isActive) continue;

        // ボブ演出（上下に浮遊）
        float bobY = sinf(pickup.bobTimer) * 0.2f;

        // フェード（残り5秒で点滅）
        float alpha = 1.0f;
        if (pickup.lifetime < 5.0f)
            alpha = (sinf(pickup.lifetime * 8.0f) * 0.5f + 0.5f);

        // 緑色（明るめ）
        DirectX::XMFLOAT4 color(0.1f, 1.0f, 0.2f, alpha);

        DirectX::XMFLOAT3 center = pickup.position;
        center.y += bobY + 0.5f;

        float size = 0.4f;

        // --- 十字の縦棒 ---
        DirectX::XMFLOAT3 vTop, vBot, vTopR, vBotR;
        vTop.x = center.x + camUp.x * size;
        vTop.y = center.y + camUp.y * size;
        vTop.z = center.z + camUp.z * size;
        vBot.x = center.x - camUp.x * size;
        vBot.y = center.y - camUp.y * size;
        vBot.z = center.z - camUp.z * size;
        vTopR.x = center.x + camUp.x * size + camRight.x * size * 0.3f;
        vTopR.y = center.y + camUp.y * size + camRight.y * size * 0.3f;
        vTopR.z = center.z + camUp.z * size + camRight.z * size * 0.3f;
        vBotR.x = center.x - camUp.x * size + camRight.x * size * 0.3f;
        vBotR.y = center.y - camUp.y * size + camRight.y * size * 0.3f;
        vBotR.z = center.z - camUp.z * size + camRight.z * size * 0.3f;

        DirectX::XMFLOAT3 vTopL, vBotL;
        vTopL.x = center.x + camUp.x * size - camRight.x * size * 0.3f;
        vTopL.y = center.y + camUp.y * size - camRight.y * size * 0.3f;
        vTopL.z = center.z + camUp.z * size - camRight.z * size * 0.3f;
        vBotL.x = center.x - camUp.x * size - camRight.x * size * 0.3f;
        vBotL.y = center.y - camUp.y * size - camRight.y * size * 0.3f;
        vBotL.z = center.z - camUp.z * size - camRight.z * size * 0.3f;

        batch->DrawTriangle(
            DirectX::VertexPositionColor(vTopL, color),
            DirectX::VertexPositionColor(vTopR, color),
            DirectX::VertexPositionColor(vBotR, color));
        batch->DrawTriangle(
            DirectX::VertexPositionColor(vTopL, color),
            DirectX::VertexPositionColor(vBotR, color),
            DirectX::VertexPositionColor(vBotL, color));

        // --- 十字の横棒 ---
        DirectX::XMFLOAT3 hL, hR, hLU, hRU, hLD, hRD;
        hL.x = center.x - camRight.x * size;
        hL.y = center.y - camRight.y * size;
        hL.z = center.z - camRight.z * size;
        hR.x = center.x + camRight.x * size;
        hR.y = center.y + camRight.y * size;
        hR.z = center.z + camRight.z * size;

        hLU.x = hL.x + camUp.x * size * 0.3f;
        hLU.y = hL.y + camUp.y * size * 0.3f;
        hLU.z = hL.z + camUp.z * size * 0.3f;
        hRU.x = hR.x + camUp.x * size * 0.3f;
        hRU.y = hR.y + camUp.y * size * 0.3f;
        hRU.z = hR.z + camUp.z * size * 0.3f;
        hLD.x = hL.x - camUp.x * size * 0.3f;
        hLD.y = hL.y - camUp.y * size * 0.3f;
        hLD.z = hL.z - camUp.z * size * 0.3f;
        hRD.x = hR.x - camUp.x * size * 0.3f;
        hRD.y = hR.y - camUp.y * size * 0.3f;
        hRD.z = hR.z - camUp.z * size * 0.3f;

        batch->DrawTriangle(
            DirectX::VertexPositionColor(hLU, color),
            DirectX::VertexPositionColor(hRU, color),
            DirectX::VertexPositionColor(hRD, color));
        batch->DrawTriangle(
            DirectX::VertexPositionColor(hLU, color),
            DirectX::VertexPositionColor(hRD, color),
            DirectX::VertexPositionColor(hLD, color));
    }

    batch->End();
}

void Game::SpawnScorePopup(int points, ScorePopupType type)
{
    ScorePopup popup;
    float cx = m_outputWidth * 0.5f + 120.0f;
    float cy = m_outputHeight * 0.5f - 60.0f;

    // タイプごとに出現位置を少しバラす（画面中央付近）
    float randX = ((float)rand() / RAND_MAX - 0.5f) * 60.0f;
    float randY = ((float)rand() / RAND_MAX - 0.5f) * 30.0f;

    switch (type)
    {
    case ScorePopupType::HEADSHOT:
        popup.screenX = cx + randX;
        popup.screenY = cy - 40.0f + randY;   // 少し上
        popup.maxTime = 1.6f;
        break;
    case ScorePopupType::GLORY_KILL:
        popup.screenX = cx + randX;
        popup.screenY = cy - 30.0f + randY;
        popup.maxTime = 2.0f;                  // 長めに表示
        break;
    case ScorePopupType::WAVE_BONUS:
        popup.screenX = cx;
        popup.screenY = cy - 80.0f;            // 上の方
        popup.maxTime = 2.0f;
        break;
    default:
        popup.screenX = cx + randX;
        popup.screenY = cy - 20.0f + randY;
        popup.maxTime = 1.2f;
        break;
    }

    popup.points = points;
    popup.type = type;
    popup.timer = 0.0f;
    popup.scale = 1.0f;
    popup.alpha = 1.0f;
    popup.offsetY = 0.0f;
    popup.burstAngle = ((float)rand() / RAND_MAX) * 6.28f;
    popup.isActive = true;

    m_scorePopups.push_back(popup);
}

void Game::UpdateScorePopups(float deltaTime)
{
    for (auto& popup : m_scorePopups)
    {
        if (!popup.isActive) continue;

        popup.timer += deltaTime;
        float t = popup.timer / popup.maxTime;  // 0.0?1.0

        if (t >= 1.0f)
        {
            popup.isActive = false;
            continue;
        }

        // === アニメーション ===

        // Phase 1 (0?0.1): パンチイン（大きく出て縮む）
        if (t < 0.1f)
        {
            float p = t / 0.1f;
            popup.scale = 2.0f - p * 1.0f;    // 2.0 → 1.0
            popup.alpha = 1.0f;
        }
        // Phase 2 (0.1?0.7): 安定表示＋ゆっくり上昇
        else if (t < 0.7f)
        {
            popup.scale = 1.0f;
            popup.alpha = 1.0f;
        }
        // Phase 3 (0.7?1.0): フェードアウト＋加速上昇
        else
        {
            float fadeT = (t - 0.7f) / 0.3f;   // 0→1
            popup.scale = 1.0f - fadeT * 0.3f;  // 少し縮む
            popup.alpha = 1.0f - fadeT;          // 透明に
        }

        // 上昇（イージング付き）
        popup.offsetY = t * t * 120.0f;

        // バースト回転
        popup.burstAngle += deltaTime * 1.5f;
    }

    // 消えたポップアップを削除
    m_scorePopups.erase(
        std::remove_if(m_scorePopups.begin(), m_scorePopups.end(),
            [](const ScorePopup& p) { return !p.isActive; }),
        m_scorePopups.end());
}

void Game::DrawScorePopups()
{
    if (m_scorePopups.empty() || !m_spriteBatch || !m_fontLarge) return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied());

    for (auto& popup : m_scorePopups)
    {
        if (!popup.isActive) continue;

        float x = popup.screenX;
        float y = popup.screenY - popup.offsetY;
        float s = popup.scale;
        float a = popup.alpha;

        // === グロウ/バースト背景 ===
        ID3D11ShaderResourceView* glowTex = nullptr;
        DirectX::XMVECTORF32 glowColor = { 1, 1, 1, a * 0.6f };

        switch (popup.type)
        {
        case ScorePopupType::HEADSHOT:
            // バースト + グロウ両方
            if (m_scoreBurst.Get())
            {
                float burstSize = 160.0f * s;
                RECT burstRect = {
                    (LONG)(x - burstSize * 0.5f), (LONG)(y - burstSize * 0.5f),
                    (LONG)(x + burstSize * 0.5f), (LONG)(y + burstSize * 0.5f)
                };
                DirectX::XMVECTORF32 burstColor = { 1, 0.9f, 0.3f, a * 0.5f };
                m_spriteBatch->Draw(m_scoreBurst.Get(), burstRect, nullptr,
                    burstColor, popup.burstAngle,
                    DirectX::XMFLOAT2(64, 64));
            }
            glowTex = m_scoreGlow.Get();
            glowColor = { 1, 0.85f, 0.2f, a * 0.7f };
            break;

        case ScorePopupType::GLORY_KILL:
            glowTex = m_scoreGlowRed.Get();
            glowColor = { 1, 0.3f, 0.2f, a * 0.8f };
            break;

        case ScorePopupType::WAVE_BONUS:
            glowTex = m_scoreGlowBlue.Get();
            glowColor = { 0.7f, 0.5f, 1, a * 0.6f };
            break;

        case ScorePopupType::PARRY:
            glowTex = m_scoreGlowBlue.Get();
            glowColor = { 0.2f, 1.0f, 0.4f, a * 0.6f };
            break;

        default:
            glowTex = m_scoreGlow.Get();
            glowColor = { 1, 0.9f, 0.4f, a * 0.5f };
            break;
        }

        // グロウ描画
        if (glowTex)
        {
            float glowSize = 120.0f * s;
            RECT glowRect = {
                (LONG)(x - glowSize * 0.5f), (LONG)(y - glowSize * 0.5f),
                (LONG)(x + glowSize * 0.5f), (LONG)(y + glowSize * 0.5f)
            };
            m_spriteBatch->Draw(glowTex, glowRect, glowColor);
        }

        // === アイコン（ヘッドショット/グローリーキル） ===
        ID3D11ShaderResourceView* iconTex = nullptr;
        if (popup.type == ScorePopupType::HEADSHOT)
            iconTex = m_scoreHeadshot.Get();
        else if (popup.type == ScorePopupType::GLORY_KILL)
            iconTex = m_scoreSkull.Get();

        if (iconTex)
        {
            float iconSize = 32.0f * s;
            RECT iconRect = {
                (LONG)(x - 70.0f * s - iconSize), (LONG)(y - iconSize * 0.5f),
                (LONG)(x - 70.0f * s),             (LONG)(y + iconSize * 0.5f)
            };
            DirectX::XMVECTORF32 iconColor = { 1, 1, 1, a };
            m_spriteBatch->Draw(iconTex, iconRect, iconColor);
        }

        // === スコアテキスト ===
        wchar_t buf[32];
        swprintf_s(buf, L"+%d", popup.points);

        // テキストサイズ測定（中央揃え用）
        DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(buf);
        float tw = DirectX::XMVectorGetX(textSize);
        float th = DirectX::XMVectorGetY(textSize);

        DirectX::XMFLOAT2 textPos(x - tw * 0.5f * s, y - th * 0.5f * s);

        // テキスト色（タイプ別）
        DirectX::XMVECTORF32 textColor;
        DirectX::XMVECTORF32 shadowColor = { 0, 0, 0, a * 0.8f };

        switch (popup.type)
        {
        case ScorePopupType::HEADSHOT:
            textColor = { 1.0f, 0.85f, 0.1f, a };   // 金色
            break;
        case ScorePopupType::GLORY_KILL:
            textColor = { 1.0f, 0.2f, 0.15f, a };    // 赤
            break;
        case ScorePopupType::WAVE_BONUS:
            textColor = { 0.7f, 0.5f, 1.0f, a };     // 紫
            break;
        case ScorePopupType::SHIELD_KILL:
            textColor = { 0.3f, 0.8f, 1.0f, a };     // 水色
            break;
        case ScorePopupType::PARRY:
            textColor = { 0.2f, 1.0f, 0.4f, a };
        default:
            textColor = { 1.0f, 1.0f, 1.0f, a };     // 白
            break;
        }

        // 影（2px オフセット）
        DirectX::XMFLOAT2 shadowPos(textPos.x + 2.0f, textPos.y + 2.0f);
        m_fontLarge->DrawString(m_spriteBatch.get(), buf, shadowPos,
            shadowColor, 0.0f, DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2(s, s));

        // 本体テキスト
        m_fontLarge->DrawString(m_spriteBatch.get(), buf, textPos,
            textColor, 0.0f, DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2(s, s));

        // === ラベルテキスト（HEADSHOT! / GLORY KILL! 等） ===
        const wchar_t* label = nullptr;
        switch (popup.type)
        {
        case ScorePopupType::HEADSHOT:    label = L"HEADSHOT";    break;
        case ScorePopupType::GLORY_KILL:  label = L"GLORY KILL";  break;
        case ScorePopupType::WAVE_BONUS:  label = L"WAVE CLEAR";  break;
        case ScorePopupType::SHIELD_KILL: label = L"SHIELD KILL"; break;
        case ScorePopupType::PARRY:       label = L"PARRY";       break;
        default: break;
        }

        if (label && m_font)
        {
            DirectX::XMVECTOR labelSize = m_font->MeasureString(label);
            float lw = DirectX::XMVectorGetX(labelSize);
            DirectX::XMFLOAT2 labelPos(x - lw * 0.5f, y + th * 0.5f * s + 4.0f);

            m_font->DrawString(m_spriteBatch.get(), label, labelPos,
                textColor, 0.0f, DirectX::XMFLOAT2(0, 0),
                DirectX::XMFLOAT2(0.9f * s, 0.9f * s));
        }
    }

    m_spriteBatch->End();
}

// =====================================================================
// UpdateRanking() - ランキング画面の更新
// 【役割】演出タイマー進行＋キー入力
// =====================================================================
// =====================================================================
// UpdateRanking() - ランキング画面の更新
// =====================================================================
void Game::UpdateRanking()
{
    m_rankingTimer += 1.0f / 60.0f;

    // ESCキーでタイトルに戻る
    if (m_rankingTimer > 0.5f && (GetAsyncKeyState(VK_ESCAPE) & 0x8000))
    {
        m_gameState = GameState::TITLE;
        m_rankingTimer = 0.0f;
    }

    // Rキーでリスタート
    if (m_rankingTimer > 0.5f && (GetAsyncKeyState('R') & 0x8000))
    {
        ResetGame();
        m_gameOverTimer = 0.0f;
        m_gameOverScore = 0;
        m_gameOverWave = 0;
        m_gameOverCountUp = 0.0f;
        m_gameOverRank = 0;
        m_gameOverNoiseT = 0.0f;
        m_gameState = GameState::TITLE;
    }
}

// =====================================================================
// RenderRanking() - ランキング画面の描画
// 【修正点】
//   - テキスト色を明るくして視認性アップ
//   - NAME列を追加
//   - テキストに影（シャドウ）を付けて読みやすく
// =====================================================================
void Game::RenderRanking()
{
    // =============================================
    // イージング関数
    // =============================================
    auto EaseOutQuad = [](float t) -> float {
        return 1.0f - (1.0f - t) * (1.0f - t);
        };

    auto EaseOutBack = [](float t) -> float {
        float c = 1.70158f;
        float u = t - 1.0f;
        return 1.0f + (c + 1.0f) * u * u * u + c * u * u;
        };

    auto EaseOutCubic = [](float t) -> float {
        float u = 1.0f - t;
        return 1.0f - u * u * u;
        };

    auto EaseOutBounce = [](float t) -> float {
        if (t < 1.0f / 2.75f)
            return 7.5625f * t * t;
        else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        }
        else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        }
        else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
        };

    // テキストに影を描くヘルパー（読みやすさ大幅アップ）
    // 本体の2px右下に黒い影を描いてから本体を描く
    auto DrawTextWithShadow = [&](DirectX::SpriteFont* font,
        const wchar_t* text, DirectX::XMFLOAT2 pos,
        DirectX::XMVECTORF32 color)
        {
            // 影（黒・半透明）
            DirectX::XMVECTORF32 shadowColor = { 0.0f, 0.0f, 0.0f, color.f[3] * 0.8f };
            font->DrawString(m_spriteBatch.get(), text,
                DirectX::XMFLOAT2(pos.x + 2.0f, pos.y + 2.0f), shadowColor);
            // 本体
            font->DrawString(m_spriteBatch.get(), text, pos, color);
        };

    float timer = m_rankingTimer;
    float screenW = (float)m_outputWidth;
    float screenH = (float)m_outputHeight;
    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    auto context = m_d3dContext.Get();

    // =============================================
    // Layer 0: 背景（より濃いビネット）
    // =============================================
    {
        float overlayT = (std::min)(timer / 0.8f, 1.0f);
        float overlayAlpha = EaseOutQuad(overlayT) * 0.95f;  // // 0.92→0.95 より暗く

        DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
            screenW, screenH, 0.1f, 10.0f);

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(DirectX::XMMatrixIdentity());
        m_effect->SetVertexColorEnabled(true);
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());

        auto primBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
        primBatch->Begin();

        float hw = screenW * 0.5f;
        float hh = screenH * 0.5f;

        // // 中央もかなり暗くして文字との対比を強くする
        DirectX::XMFLOAT4 centerColor(0.06f, 0.0f, 0.02f, overlayAlpha * 0.85f);
        DirectX::XMFLOAT4 edgeColor(0.01f, 0.0f, 0.0f, overlayAlpha);

        DirectX::VertexPositionColor vc(DirectX::XMFLOAT3(0, 0, 1.0f), centerColor);
        DirectX::VertexPositionColor vTL(DirectX::XMFLOAT3(-hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vTR(DirectX::XMFLOAT3(hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBR(DirectX::XMFLOAT3(hw, -hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBL(DirectX::XMFLOAT3(-hw, -hh, 1.0f), edgeColor);

        primBatch->DrawTriangle(vc, vTL, vTR);
        primBatch->DrawTriangle(vc, vTR, vBR);
        primBatch->DrawTriangle(vc, vBR, vBL);
        primBatch->DrawTriangle(vc, vBL, vTL);

        primBatch->End();
    }

    // =============================================
    // Layer 1: SpriteBatchで全UI描画
    // =============================================
    if (!m_spriteBatch || !m_states || !m_font || !m_whitePixel)
        return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->AlphaBlend(),
        nullptr,
        m_states->DepthNone()
    );

    const auto& entries = m_rankingSystem->GetEntries();
    int entryCount = (int)entries.size();

    // =============================================
    // 装飾線（上）
    // =============================================
    if (timer > 0.2f)
    {
        float lineT = (std::min)((timer - 0.2f) / 0.5f, 1.0f);
        float lineProgress = EaseOutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;  // // 280→320 少し長く
        float lineY = 55.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 2)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 2)
        };

        // // 色をより明るい赤金に
        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.95f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

        RECT diamond = {
            (LONG)(centerX - 5), (LONG)(lineY - 4),
            (LONG)(centerX + 5), (LONG)(lineY + 6)
        };
        m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
    }

    // =============================================
    // タイトル "LEADERBOARD" ? バウンスで落ちてくる
    // =============================================
    if (timer > 0.3f && m_fontLarge)
    {
        float titleT = (std::min)((timer - 0.3f) / 0.6f, 1.0f);
        float bounceT = EaseOutBounce(titleT);
        float titleAlpha = (std::min)((timer - 0.3f) * 4.0f, 1.0f);

        float slideY = (1.0f - bounceT) * -60.0f;
        float titleY = 65.0f + slideY;

        DirectX::XMVECTOR titleSize = m_fontLarge->MeasureString(L"LEADERBOARD");
        float titleW = DirectX::XMVectorGetX(titleSize);
        float titleX = centerX - titleW * 0.5f;

        // // グロー色をより明るく
        float glowAlpha = titleAlpha * 0.3f;
        DirectX::XMVECTORF32 glowColor = { 1.0f, 0.3f, 0.0f, glowAlpha };
        for (int dx = -2; dx <= 2; dx += 4)
        {
            for (int dy = -2; dy <= 2; dy += 4)
            {
                m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
                    DirectX::XMFLOAT2(titleX + dx, titleY + dy), glowColor);
            }
        }

        // // 影を追加
        DirectX::XMVECTORF32 shadowColor = { 0.0f, 0.0f, 0.0f, titleAlpha * 0.7f };
        m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
            DirectX::XMFLOAT2(titleX + 3, titleY + 3), shadowColor);

        // // 本体色を明るいゴールドに
        DirectX::XMVECTORF32 titleColor = { 1.0f, 0.9f, 0.75f, titleAlpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
            DirectX::XMFLOAT2(titleX, titleY), titleColor);
    }

    // =============================================
    // 装飾線（タイトルの下）
    // =============================================
    if (timer > 0.4f)
    {
        float lineT = (std::min)((timer - 0.4f) / 0.5f, 1.0f);
        float lineProgress = EaseOutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;
        float lineY = 115.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 1)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 1)
        };

        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.8f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);
    }

    // =============================================
    // ヘッダー行: #  NAME  SCORE  WAVE  KILLS
    // =============================================
    if (timer > 0.6f)
    {
        float headerAlpha = (std::min)((timer - 0.6f) * 3.0f, 1.0f);
        // // ヘッダー色をより明るい金色に（白だと内容と紛らわしい）
        DirectX::XMVECTORF32 headerColor = { 1.0f, 0.75f, 0.35f, headerAlpha * 0.9f };

        float headerY = 130.0f;

        // // 列の配置を変更（NAME列を追加）
        float colNum = centerX - 310.0f;   // #
        float colName = centerX - 250.0f;   // NAME  //新規
        float colScore = centerX - 60.0f;    // SCORE
        float colWave = centerX + 100.0f;   // WAVE
        float colKills = centerX + 210.0f;   // KILLS

        DrawTextWithShadow(m_font.get(), L"#",
            DirectX::XMFLOAT2(colNum, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"NAME",
            DirectX::XMFLOAT2(colName, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"SCORE",
            DirectX::XMFLOAT2(colScore, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"WAVE",
            DirectX::XMFLOAT2(colWave, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"KILLS",
            DirectX::XMFLOAT2(colKills, headerY), headerColor);
    }

    // =============================================
    // 各エントリー
    // =============================================
    float entryStartTime = 0.8f;
    float entryDelay = 0.12f;
    float entryBaseY = 160.0f;
    float entryHeight = 42.0f;

    // 列のX座標（ヘッダーと同じ）
    float colNum = centerX - 310.0f;
    float colName = centerX - 250.0f;
    float colScore = centerX - 60.0f;
    float colWave = centerX + 100.0f;
    float colKills = centerX + 210.0f;

    for (int i = 0; i < entryCount && i < 10; i++)
    {
        float startT = entryStartTime + i * entryDelay;

        if (timer < startT)
            continue;

        // --- スライドインアニメーション ---
        float slideT = (std::min)((timer - startT) / 0.5f, 1.0f);
        float slideProgress = EaseOutCubic(slideT);

        float slideX = (1.0f - slideProgress) * -300.0f;
        float alpha = slideProgress;

        float rowY = entryBaseY + i * entryHeight;

        bool isNewRecord = (i == m_newRecordRank);

        // --- 背景バー ---
        {
            float barAlpha = alpha * 0.25f;  // // 0.15→0.25 バーをより目立たせる

            if (isNewRecord)
            {
                float pulse = sinf(timer * 4.0f) * 0.5f + 0.5f;
                barAlpha = alpha * (0.3f + pulse * 0.2f);
            }

            if (i == 0)
                barAlpha = alpha * 0.3f;

            RECT bar = {
                (LONG)(centerX - 330.0f + slideX), (LONG)(rowY - 2),
                (LONG)(centerX + 330.0f + slideX), (LONG)(rowY + entryHeight - 8)
            };

            DirectX::XMVECTORF32 barColor;
            if (isNewRecord)
                barColor = { 1.0f, 0.8f, 0.1f, barAlpha };
            else if (i == 0)
                barColor = { 1.0f, 0.75f, 0.2f, barAlpha };
            else if (i == 1)
                barColor = { 0.75f, 0.75f, 0.85f, barAlpha };
            else if (i == 2)
                barColor = { 0.75f, 0.5f, 0.2f, barAlpha };
            else
                barColor = { 0.4f, 0.2f, 0.15f, barAlpha };

            m_spriteBatch->Draw(m_whitePixel.Get(), bar, barColor);
        }

        // --- 順位の色（ 全体的に明るく） ---
        DirectX::XMVECTORF32 rankNumColor;
        if (i == 0)
            rankNumColor = { 1.0f, 0.9f, 0.3f, alpha };      // 1位: 明るい金
        else if (i == 1)
            rankNumColor = { 0.9f, 0.9f, 1.0f, alpha };       // 2位: 明るい銀
        else if (i == 2)
            rankNumColor = { 1.0f, 0.65f, 0.3f, alpha };      // 3位: 明るい銅
        else
            rankNumColor = { 0.85f, 0.8f, 0.7f, alpha };      //  他: 明るいベージュ

        if (isNewRecord)
        {
            float pulse = sinf(timer * 5.0f) * 0.5f + 0.5f;
            rankNumColor = { 1.0f, 0.8f + pulse * 0.2f, 0.2f + pulse * 0.3f, alpha };
        }

        // --- テキスト色（ 明るく） ---
        DirectX::XMVECTORF32 textColor;
        if (isNewRecord)
        {
            float pulse = sinf(timer * 5.0f) * 0.5f + 0.5f;
            textColor = { 1.0f, 0.9f + pulse * 0.1f, 0.6f + pulse * 0.3f, alpha };
        }
        else
        {
            textColor = { 1.0f, 0.95f, 0.85f, alpha };  // // 明るいクリーム色
        }

        const RankingEntry& e = entries[i];

        // --- 順位番号 ---
        wchar_t rankBuf[16];
        swprintf_s(rankBuf, L"%d.", i + 1);
        DrawTextWithShadow(m_font.get(), rankBuf,
            DirectX::XMFLOAT2(colNum + slideX, rowY), rankNumColor);

        // ---  名前（新規列） ---
        wchar_t nameBuf[32] = {};
        if (e.name[0] != '\0')
        {
            // char[16] → wchar_t に変換
            for (int c = 0; c < 15 && e.name[c] != '\0'; c++)
                nameBuf[c] = (wchar_t)e.name[c];
        }
        else
        {
            wcscpy_s(nameBuf, L"---");  // 名前なし（旧データ互換）
        }

        // 名前は順位と同じ色系統（1位金, 2位銀...）
        DrawTextWithShadow(m_font.get(), nameBuf,
            DirectX::XMFLOAT2(colName + slideX, rowY), rankNumColor);

        // --- スコア（カウントアップ演出） ---
        float countUpT = (std::min)((timer - startT) / 1.0f, 1.0f);
        float easedCount = EaseOutQuad(countUpT);
        int displayScore = (int)(e.score * easedCount);

        wchar_t scoreBuf[32];
        swprintf_s(scoreBuf, L"%d", displayScore);
        DrawTextWithShadow(m_font.get(), scoreBuf,
            DirectX::XMFLOAT2(colScore + slideX, rowY), textColor);

        // --- ウェーブ ---
        wchar_t waveBuf[16];
        swprintf_s(waveBuf, L"W%d", e.wave);
        DrawTextWithShadow(m_font.get(), waveBuf,
            DirectX::XMFLOAT2(colWave + slideX, rowY), textColor);

        // --- キル数 ---
        wchar_t killBuf[16];
        swprintf_s(killBuf, L"%d", e.kills);
        DrawTextWithShadow(m_font.get(), killBuf,
            DirectX::XMFLOAT2(colKills + slideX, rowY), textColor);

        // --- 新記録マーク ---
        if (isNewRecord && slideT > 0.5f)
        {
            float newAlpha = (std::min)((slideT - 0.5f) * 4.0f, 1.0f);
            float pulse = sinf(timer * 6.0f) * 0.5f + 0.5f;

            DirectX::XMVECTORF32 newColor = {
                1.0f, 0.85f + pulse * 0.15f, 0.0f, newAlpha
            };

            DrawTextWithShadow(m_font.get(), L"NEW!",
                DirectX::XMFLOAT2(colKills + 70.0f + slideX, rowY), newColor);
        }
    }

    // =============================================
    // エントリー0件
    // =============================================
    if (entryCount == 0 && timer > 1.0f)
    {
        float emptyAlpha = (std::min)((timer - 1.0f) * 2.0f, 1.0f);
        DirectX::XMVECTORF32 emptyColor = { 0.8f, 0.7f, 0.5f, emptyAlpha };

        const wchar_t* emptyText = L"NO RECORDS YET";
        DirectX::XMVECTOR emptySize = m_font->MeasureString(emptyText);
        float emptyW = DirectX::XMVectorGetX(emptySize);

        DrawTextWithShadow(m_font.get(), emptyText,
            DirectX::XMFLOAT2(centerX - emptyW * 0.5f, centerY - 20.0f), emptyColor);
    }

    // =============================================
    // 装飾線（下部）
    // =============================================
    if (timer > 1.5f)
    {
        float lineT = (std::min)((timer - 1.5f) / 0.5f, 1.0f);
        float lineProgress = EaseOutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;
        float lineY = entryBaseY + 10 * entryHeight + 10.0f;
        if (lineY > screenH - 80.0f) lineY = screenH - 80.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 1)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 1)
        };

        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.6f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

        RECT diamond = {
            (LONG)(centerX - 3), (LONG)(lineY - 2),
            (LONG)(centerX + 3), (LONG)(lineY + 3)
        };
        m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
    }

    // =============================================
    // フッター: 操作案内
    // =============================================
    if (timer > 2.0f)
    {
        float footerAlpha = (std::min)((timer - 2.0f) * 2.0f, 1.0f);
        float pulse = sinf(timer * 2.0f) * 0.15f + 0.85f;

        //  フッター色をもっと明るく
        DirectX::XMVECTORF32 footerColor = { 0.85f, 0.75f, 0.55f, footerAlpha * pulse };

        float footerY = screenH - 50.0f;

        const wchar_t* backText = L"ESC: BACK     R: RESTART";
        DirectX::XMVECTOR backSize = m_font->MeasureString(backText);
        float backW = DirectX::XMVectorGetX(backSize);

        DrawTextWithShadow(m_font.get(), backText,
            DirectX::XMFLOAT2(centerX - backW * 0.5f, footerY), footerColor);
    }

    m_spriteBatch->End();
}