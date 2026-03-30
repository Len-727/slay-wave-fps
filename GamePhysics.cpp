// ============================================================
//  GamePhysics.cpp
//  Bullet Physics統合・レイキャスト・ナビグリッド構築
//
//  【責務】
//  - Bullet Physicsワールドの初期化/破棄
//  - 敵のキネマティック剛体の///更新/削除
//  - レイキャストによる射撃判定（敵のみ/マップのみ）
//  - メッシュコライダーによる壁衝突判定
//  - A*パスファインディング用ナビグリッドの自動構築
//
//  【設計意図】
//  game.cppから物理演算系を分離。
//  物理関連の変更が他の描画/UIコードに影響しないようにする。
//  Bullet Physicsへの依存もこのファイルに集約。
// ============================================================

#include "Game.h"

using namespace DirectX;

// ============================================================
//  InitPhysics - Bullet Physicsワールドの初期化
//
//  【呼び出し元】Game::Initialize()
//  【処理順序】レンダリングリソース作成後に呼ぶこと
//    （初期化順序を間違えるとクラッシュする）
// ============================================================
void Game::InitPhysics()
{
    // 衝突検出の設定（デフォルトで十分）
    m_collisionConfiguration =
        std::make_unique<btDefaultCollisionConfiguration>();

    // 衝突ペアの管理（ブロードフェーズで絞った後の精密判定）
    m_dispatcher =
        std::make_unique<btCollisionDispatcher>(m_collisionConfiguration.get());

    // 空間分割（AABB重複の高速検出）
    m_broadphase = std::make_unique<btDbvtBroadphase>();

    // 拘束ソルバー（今回はキネマティックのみなので軽量）
    m_solver =
        std::make_unique<btSequentialImpulseConstraintSolver>();

    // ワールド作成（全コンポーネントを接続）
    m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(),
        m_broadphase.get(),
        m_solver.get(),
        m_collisionConfiguration.get()
    );
}

// ============================================================
//  CleanupPhysics - 全物理リソースの安全な解放
//
//  【解放順序が重要】
//  1. マップメッシュ剛体 → 2. 敵の剛体 → 3. ワールド本体
//  逆順にするとダングリングポインタでクラッシュする
// ============================================================
void Game::CleanupPhysics()
{
    // --- マップメッシュコライダーの解放 ---
    if (m_mapMeshBody)
    {
        if (m_dynamicsWorld) m_dynamicsWorld->removeRigidBody(m_mapMeshBody);
        delete m_mapMeshBody->getMotionState();
        delete m_mapMeshBody;
        m_mapMeshBody = nullptr;
    }
    if (m_mapMeshShape) { delete m_mapMeshShape; m_mapMeshShape = nullptr; }
    if (m_mapTriMesh) { delete m_mapTriMesh; m_mapTriMesh = nullptr; }

    // --- 全敵の物理ボディを削除 ---
    for (auto& pair : m_enemyPhysicsBodies)
    {
        btRigidBody* body = pair.second;

        if (m_dynamicsWorld)
        {
            m_dynamicsWorld->removeRigidBody(body);
        }

        if (body->getMotionState())
        {
            delete body->getMotionState();
        }
        if (body->getCollisionShape())
        {
            delete body->getCollisionShape();
        }
        delete body;
    }
    m_enemyPhysicsBodies.clear();

    // --- Bullet Physicsワールド本体を解放 ---
    // 作成の逆順で破棄（依存関係を壊さない）
    m_dynamicsWorld.reset();
    m_solver.reset();
    m_broadphase.reset();
    m_dispatcher.reset();
    m_collisionConfiguration.reset();
}

// ============================================================
//  RaycastPhysics - レイキャストで敵との衝突判定
//
//  【用途】射撃判定（銃弾の軌道上に敵がいるか）
//  【仕組み】
//    - EnemyOnlyRayCallback で壁(userPtr==null)を無視
//    - userPointerにID+1を格納（0がnullptrと区別できるよう）
//  【戻り値】RaycastResult（ヒット位置・法線・敵ポインタ）
// ============================================================
Game::RaycastResult Game::RaycastPhysics(
    DirectX::XMFLOAT3 start,
    DirectX::XMFLOAT3 direction,
    float maxDistance)
{
    RaycastResult result;
    result.hit = false;
    result.hitEnemy = nullptr;
    result.hitPoint = { 0, 0, 0 };
    result.hitNormal = { 0, 0, 0 };

    // 方向ベクトルを正規化（長さ1にする）
    float length = sqrtf(
        direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z
    );

    // ゼロベクトルチェック（0除算防止）
    constexpr float DIRECTION_EPSILON = 0.0001f;
    if (length < DIRECTION_EPSILON)
        return result;

    direction.x /= length;
    direction.y /= length;
    direction.z /= length;

    // レイの始点と終点をBullet形式に変換
    btVector3 rayStart(start.x, start.y, start.z);
    btVector3 rayEnd(
        start.x + direction.x * maxDistance,
        start.y + direction.y * maxDistance,
        start.z + direction.z * maxDistance
    );

    // --- カスタムコールバック: 敵のカプセルだけに当たるようにする ---
    // マップメッシュ(userPtr==nullptr)はスキップ
    struct EnemyOnlyRayCallback : public btCollisionWorld::ClosestRayResultCallback
    {
        EnemyOnlyRayCallback(const btVector3& from, const btVector3& to)
            : ClosestRayResultCallback(from, to) {
        }

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
            bool normalInWorldSpace) override
        {
            if (rayResult.m_collisionObject->getUserPointer() == nullptr)
                return 1.0f; // マップメッシュ → 無視して続行

            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    EnemyOnlyRayCallback rayCallback(rayStart, rayEnd);
    m_dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);

    if (rayCallback.hasHit())
    {
        result.hit = true;

        // ヒット位置と法線を取得
        result.hitPoint.x = rayCallback.m_hitPointWorld.getX();
        result.hitPoint.y = rayCallback.m_hitPointWorld.getY();
        result.hitPoint.z = rayCallback.m_hitPointWorld.getZ();

        result.hitNormal.x = rayCallback.m_hitNormalWorld.getX();
        result.hitNormal.y = rayCallback.m_hitNormalWorld.getY();
        result.hitNormal.z = rayCallback.m_hitNormalWorld.getZ();

        // --- UserPointerから敵IDを復元 ---
        // 格納時: (void*)(intptr_t)(id + 1) なので、復元時は -1
        const btCollisionObject* hitObj = rayCallback.m_collisionObject;
        void* userPtr = hitObj->getUserPointer();

        if (userPtr != nullptr)
        {
            int enemyID = (int)(intptr_t)userPtr - 1;

            // IDから敵を検索（線形探索 → 敵数が多い場合はmap化を検討）
            const auto& enemies = m_enemySystem->GetEnemies();
            for (const auto& enemy : enemies)
            {
                if (enemy.id == enemyID && enemy.isAlive)
                {
                    // const_cast: GetEnemies()がconst参照を返すため
                    // 【改善案】GetEnemiesMutable()を使うべきだが、
                    //   射撃判定の呼び出し元でmutable版に切り替える必要がある
                    result.hitEnemy = const_cast<Enemy*>(&enemy);
                    break;
                }
            }
        }
    }

    return result;
}

// ============================================================
//  AddEnemyPhysicsBody - 敵のキネマティック剛体を//
//
//  【キネマティック剛体とは】
//  mass=0で自分では動かないが、レイキャストに当たる。
//  毎フレームUpdateEnemyPhysicsBodyで位置を手動更新する。
//  （ダイナミック剛体だと敵同士が弾き合って制御不能になるため）
// ============================================================
void Game::AddEnemyPhysicsBody(Enemy& enemy)
{
    // デバッグ用ヒットボックスとデフォルト値を切り替え
    EnemyTypeConfig config;
    if (m_useDebugHitboxes)
    {
        switch (enemy.type)
        {
        case EnemyType::NORMAL:  config = m_normalConfigDebug;  break;
        case EnemyType::RUNNER:  config = m_runnerConfigDebug;  break;
        case EnemyType::TANK:    config = m_tankConfigDebug;    break;
        case EnemyType::MIDBOSS: config = m_midbossConfigDebug; break;
        case EnemyType::BOSS:    config = m_bossConfigDebug;    break;
        default: config = GetEnemyConfig(enemy.type); break;
        }
    }
    else
    {
        config = GetEnemyConfig(enemy.type);
    }

    // カプセル形状を作成（半径 + 円柱高さ）
    float radius = config.bodyWidth / 2.0f;
    float totalHeight = config.bodyHeight;
    float cylinderHeight = totalHeight - 2.0f * radius;

    if (cylinderHeight < 0.0f)
        cylinderHeight = 0.0f;

    btCollisionShape* shape = new btCapsuleShape(radius, cylinderHeight);

    // 初期位置（足元ではなく体の中心）
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        enemy.position.x,
        enemy.position.y + totalHeight / 2.0f,
        enemy.position.z
    ));

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);

    // mass=0 → 静的/キネマティックオブジェクト
    constexpr btScalar KINEMATIC_MASS = 0.0f;
    btVector3 localInertia(0, 0, 0);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        KINEMATIC_MASS,
        motionState,
        shape,
        localInertia
    );

    btRigidBody* body = new btRigidBody(rbInfo);

    // キネマティックフラグ設定（手動で位置更新するオブジェクト）
    body->setCollisionFlags(
        body->getCollisionFlags() |
        btCollisionObject::CF_KINEMATIC_OBJECT
    );

    // スリープさせない（毎フレーム位置が変わるため）
    body->setActivationState(DISABLE_DEACTIVATION);

    // ID+1をUserPointerに格納（0=nullptrと区別するため+1）
    body->setUserPointer((void*)(intptr_t)(enemy.id + 1));

    m_dynamicsWorld->addRigidBody(body);
    m_enemyPhysicsBodies[enemy.id] = body;
}

// ============================================================
//  CheckMeshCollision - 球vsメッシュの衝突判定
//
//  【用途】プレイヤーの壁めり込み防止
//  【仕組み】テスト用の球を作って、マップメッシュとの
//    接触ペアテスト(contactPairTest)で判定
// ============================================================
bool Game::CheckMeshCollision(DirectX::XMFLOAT3 position, float radius)
{
    if (!m_dynamicsWorld || !m_mapMeshBody) return false;

    // テスト用の球を作る（プレイヤーの体を模擬）
    btSphereShape sphere(radius);

    btCollisionObject testObj;
    testObj.setCollisionShape(&sphere);

    // 目の高さ(position.y)から体の中心に下げる
    constexpr float EYE_TO_CENTER_OFFSET = 0.9f;
    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(
        position.x,
        position.y - EYE_TO_CENTER_OFFSET,
        position.z));
    testObj.setWorldTransform(t);

    // Bulletに「この球とマップメッシュ、重なってる？」と聞く
    struct ContactCallback : public btCollisionWorld::ContactResultCallback
    {
        bool hit = false;

        btScalar addSingleResult(
            btManifoldPoint& cp,
            const btCollisionObjectWrapper* colObj0Wrap,
            int partId0, int index0,
            const btCollisionObjectWrapper* colObj1Wrap,
            int partId1, int index1) override
        {
            hit = true;
            return 0; // 0を返すと探索を早期終了（1つ見つかれば十分）
        }
    };

    ContactCallback callback;
    m_dynamicsWorld->contactPairTest(&testObj, m_mapMeshBody, callback);

    return callback.hit;
}

// ============================================================
//  GetMeshFloorHeight - メッシュの床の高さをレイキャストで取得
//
//  【用途】プレイヤー/敵/パーティクルのY座標を地面に合わせる
//  【重要】全てのY座標ロジックはこの関数を経由すること！
//    Y=0のハードコードは禁止（FBXマップの床は平坦ではない）
//  【仕組み】真上(Y=100)から真下(Y=-100)にレイを飛ばし、
//    マップメッシュだけに当たった点のYを返す
// ============================================================
float Game::GetMeshFloorHeight(float x, float z, float defaultY)
{
    if (!m_dynamicsWorld || !m_mapMeshBody) return defaultY;

    constexpr float RAY_START_Y = 100.0f;
    constexpr float RAY_END_Y = -100.0f;

    btVector3 rayFrom(x, RAY_START_Y, z);
    btVector3 rayTo(x, RAY_END_Y, z);

    // マップメッシュだけに当たるカスタムコールバック
    // （敵のカプセル等を無視する）
    struct MapOnlyRayCallback : public btCollisionWorld::ClosestRayResultCallback
    {
        btRigidBody* mapBody;

        MapOnlyRayCallback(const btVector3& from, const btVector3& to, btRigidBody* map)
            : ClosestRayResultCallback(from, to), mapBody(map) {
        }

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
            bool normalInWorldSpace) override
        {
            // マップメッシュ以外は無視
            if (rayResult.m_collisionObject != mapBody)
                return 1.0f;

            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    MapOnlyRayCallback rayCallback(rayFrom, rayTo, m_mapMeshBody);
    m_dynamicsWorld->rayTest(rayFrom, rayTo, rayCallback);

    if (rayCallback.hasHit())
    {
        return rayCallback.m_hitPointWorld.getY();
    }

    return defaultY;
}

// ============================================================
//  BuildNavGrid - ナビゲーショングリッドの自動構築
//
//  【仕組み】
//  1. FBXメッシュのAABBからグリッド範囲を自動計算
//  2. 全マスについて CheckMeshCollision で壁判定
//  3. 段差が maxStepHeight を超えるマスも壁扱い
//
//  【パフォーマンス注意】
//  マス数×CheckMeshCollision なので、起動時に1回だけ呼ぶ。
//  cellSize=1.0m で概ね 50×60=3000マス → 約0.5秒
// ============================================================
void Game::BuildNavGrid()
{
    // メッシュの三角形データからAABBを計算
    const auto& tris = m_mapSystem->GetCollisionTriangles();
    if (tris.empty()) {
        OutputDebugStringA("[NavGrid] No triangles, skipping.\n");
        return;
    }

    constexpr float FLOAT_MAX = 999999.0f;
    float meshMinX = FLOAT_MAX, meshMaxX = -FLOAT_MAX;
    float meshMinY = FLOAT_MAX, meshMaxY = -FLOAT_MAX;
    float meshMinZ = FLOAT_MAX, meshMaxZ = -FLOAT_MAX;

    for (const auto& tri : tris)
    {
        auto check = [&](const DirectX::XMFLOAT3& v) {
            if (v.x < meshMinX) meshMinX = v.x;
            if (v.x > meshMaxX) meshMaxX = v.x;
            if (v.y < meshMinY) meshMinY = v.y;
            if (v.y > meshMaxY) meshMaxY = v.y;
            if (v.z < meshMinZ) meshMinZ = v.z;
            if (v.z > meshMaxZ) meshMaxZ = v.z;
            };
        check(tri.v0); check(tri.v1); check(tri.v2);
    }

    // マージンを//（端の歩行判定が途切れないように）
    constexpr float GRID_MARGIN = 2.0f;
    constexpr float CELL_SIZE = 1.0f;
    constexpr float WALK_TEST_RADIUS = 0.4f;

    float mapMinX = meshMinX - GRID_MARGIN;
    float mapMaxX = meshMaxX + GRID_MARGIN;
    float mapMinZ = meshMinZ - GRID_MARGIN;
    float mapMaxZ = meshMaxZ + GRID_MARGIN;

    // 地面の高さ = AABBの下端から少し上（壁の胴体あたり）
    constexpr float TEST_HEIGHT_OFFSET = 1.0f;
    float testY = meshMinY + TEST_HEIGHT_OFFSET;

    {
        char buf[512];
        sprintf_s(buf, "[NavGrid] Using mesh AABB: X(%.1f~%.1f) Z(%.1f~%.1f) testY=%.1f\n",
            mapMinX, mapMaxX, mapMinZ, mapMaxZ, testY);
        OutputDebugStringA(buf);
    }

    m_navGrid.Initialize(mapMinX, mapMaxX, mapMinZ, mapMaxZ,
        CELL_SIZE, WALK_TEST_RADIUS);

    if (!m_mapMeshBody)
    {
        OutputDebugStringA("[NavGrid] WARNING: No mesh collider! Grid is all walkable.\n");
        return;
    }

    // --- 全マスを1つずつチェック ---
    int blockedCount = 0;
    int width = m_navGrid.GetWidth();
    int height = m_navGrid.GetHeight();

    float baseGroundY = GetMeshFloorHeight(0.0f, 0.0f, meshMinY);
    constexpr float MAX_STEP_HEIGHT = 1.5f; // これ以上の段差は登れない

    {
        char buf[256];
        sprintf_s(buf, "[NavGrid] Base ground Y: %.2f, max step: %.1f\n",
            baseGroundY, MAX_STEP_HEIGHT);
        OutputDebugStringA(buf);
    }

    for (int gz = 0; gz < height; gz++)
    {
        for (int gx = 0; gx < width; gx++)
        {
            float wx = m_navGrid.GridToWorldX(gx);
            float wz = m_navGrid.GridToWorldZ(gz);

            bool blocked = false;

            // 壁があるか（球vsメッシュ衝突判定）
            DirectX::XMFLOAT3 testPos = { wx, testY, wz };
            if (CheckMeshCollision(testPos, WALK_TEST_RADIUS))
            {
                blocked = true;
            }

            // 地面が高すぎないか（高低差判定）
            if (!blocked)
            {
                float cellGroundY = GetMeshFloorHeight(wx, wz, baseGroundY);
                if (cellGroundY > baseGroundY + MAX_STEP_HEIGHT)
                {
                    blocked = true;
                }
            }

            if (blocked)
            {
                m_navGrid.SetBlocked(gx, gz, true);
                blockedCount++;
            }
        }
    }

    char buf[256];
    sprintf_s(buf, "[NavGrid] Built: %d/%d cells blocked (%.1f%%)\n",
        blockedCount, width * height,
        100.0f * blockedCount / (width * height));
    OutputDebugStringA(buf);
}

// ============================================================
//  UpdateEnemyPhysicsBody - 敵のBullet剛体位置を同期
//
//  【毎フレーム呼ぶ】EnemySystemが敵を動かした後、
//  Bullet側の剛体位置を合わせる（逆ではない！）
//  ゲームロジック → Bullet剛体 の一方通行。
// ============================================================
void Game::UpdateEnemyPhysicsBody(Enemy& enemy)
{
    auto it = m_enemyPhysicsBodies.find(enemy.id);
    if (it == m_enemyPhysicsBodies.end())
        return;

    btRigidBody* body = it->second;

    // デバッグ用ヒットボックスとデフォルト値を切り替え
    EnemyTypeConfig config;
    if (m_useDebugHitboxes)
    {
        switch (enemy.type)
        {
        case EnemyType::NORMAL:  config = m_normalConfigDebug;  break;
        case EnemyType::RUNNER:  config = m_runnerConfigDebug;  break;
        case EnemyType::TANK:    config = m_tankConfigDebug;    break;
        case EnemyType::MIDBOSS: config = m_midbossConfigDebug; break;
        case EnemyType::BOSS:    config = m_bossConfigDebug;    break;
        default: config = GetEnemyConfig(enemy.type); break;
        }
    }
    else
    {
        config = GetEnemyConfig(enemy.type);
    }

    float totalHeight = config.bodyHeight;

    // 足元位置 + 高さの半分 = 体の中心
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        enemy.position.x,
        enemy.position.y + totalHeight / 2.0f,
        enemy.position.z
    ));

    body->setWorldTransform(transform);

    if (body->getMotionState())
    {
        body->getMotionState()->setWorldTransform(transform);
    }
}

// ============================================================
//  RemoveEnemyPhysicsBody - 敵の剛体をワールドから削除
//
//  【呼び出しタイミング】敵が死亡してClearDeadEnemiesされる前
//  【メモリ管理】shape, motionState, bodyの3つをdelete
// ============================================================
void Game::RemoveEnemyPhysicsBody(int enemyID)
{
    auto it = m_enemyPhysicsBodies.find(enemyID);
    if (it == m_enemyPhysicsBodies.end())
        return;

    btRigidBody* body = it->second;
    m_dynamicsWorld->removeRigidBody(body);

    delete body->getMotionState();
    delete body->getCollisionShape();
    delete body;

    m_enemyPhysicsBodies.erase(it);
}

// ============================================================
//  UpdatePhysics - 物理シミュレーションの1ステップ実行
//
//  【注意】近接チャージの自動回復もここに含まれている
//  （本来はUpdatePlayingに移すべきだが、物理更新と同タイミングで
//   実行する必要があるため暫定的にここに配置）
// ============================================================
void Game::UpdatePhysics(float deltaTime)
{
    if (m_dynamicsWorld)
    {
        // 物理シミュレーションを1ステップ進める
        // maxSubSteps=10: フレーム落ちしても最大10回分まで追いつく
        constexpr int MAX_SUB_STEPS = 10;
        m_dynamicsWorld->stepSimulation(deltaTime, MAX_SUB_STEPS);
        UpdateGibs(deltaTime);

        // 近接チャージ自動回復
        if (m_meleeCharges < m_meleeMaxCharges)
        {
            m_meleeRechargeTimer += deltaTime;
            if (m_meleeRechargeTimer >= m_meleeRechargeTime)
            {
                m_meleeCharges++;
                m_meleeRechargeTimer = 0.0f;
            }
        }
    }
}

// ============================================================
//  DrawNavGridDebug - ナビグリッドの壁マスをデバッグ描画
//
//  【表示条件】m_debugDrawNavGrid == true のとき
//  【最適化】プレイヤーから25m以内のマスだけ描画
//    （2乗比較で sqrt を回避）
// ============================================================
void Game::DrawNavGridDebug(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
{
    if (!m_debugDrawNavGrid || !m_navGrid.IsReady()) return;

    int width = m_navGrid.GetWidth();
    int height = m_navGrid.GetHeight();

    DirectX::XMFLOAT3 pPos = m_player->GetPosition();

    // 描画範囲の定数化（sqrtを避けて2乗比較）
    constexpr float DRAW_RANGE = 25.0f;
    constexpr float DRAW_RANGE_SQ = DRAW_RANGE * DRAW_RANGE;

    for (int gz = 0; gz < height; gz++)
    {
        for (int gx = 0; gx < width; gx++)
        {
            if (!m_navGrid.IsBlocked(gx, gz)) continue;

            float wx = m_navGrid.GridToWorldX(gx);
            float wz = m_navGrid.GridToWorldZ(gz);

            // 距離チェック（2乗比較でsqrt回避）
            float dx = wx - pPos.x;
            float dz = wz - pPos.z;
            if (dx * dx + dz * dz > DRAW_RANGE_SQ) continue;

            // そのマスの地面の高さを取得（壁の上を蓋にする）
            float cellY = GetMeshFloorHeight(wx, wz, pPos.y - 1.8f);

            float cellSize = m_navGrid.GetCellSize();

            // 赤い柱を地面から3m上まで立てる
            constexpr float DEBUG_PILLAR_HEIGHT = 3.0f;
            constexpr float DEBUG_CELL_SCALE = 0.8f;

            DirectX::XMMATRIX world =
                DirectX::XMMatrixScaling(cellSize * DEBUG_CELL_SCALE, DEBUG_PILLAR_HEIGHT, cellSize * DEBUG_CELL_SCALE) *
                DirectX::XMMatrixTranslation(wx, cellY + DEBUG_PILLAR_HEIGHT / 2.0f, wz);

            constexpr float DEBUG_WALL_ALPHA = 0.5f;
            m_cube->Draw(world, view, proj,
                DirectX::XMVECTORF32{ 1.0f, 0.0f, 0.0f, DEBUG_WALL_ALPHA });
        }
    }
}