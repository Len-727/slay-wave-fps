// Game.cpp - 実装（基盤部分）
#include "Game.h"
#include <string>
//#include <stdexcept>
//#include <algorithm>

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
    m_showDebugWindow(true),
    m_showHitboxes(true),
    m_showHeadHitboxes(true),
    m_showBulletTrajectory(true),
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

    CreateRenderResources();  // 3D描画用の初期化

    //  === Bullet Physics 初期化  ===
    InitPhysics();

    // Initialize で初期化
    m_titleScene = std::make_unique<TitleScene>();
    m_titleScene->Initialize(m_d3dDevice.Get(),
        m_outputWidth, m_outputHeight);

}

void Game::InitPhysics()
{
    // 衝突設定
    m_collisionConfiguration =
        std::make_unique<btDefaultCollisionConfiguration>();

    // ディスパッチャー
    m_dispatcher =
        std::make_unique<btCollisionDispatcher>(m_collisionConfiguration.get());

    // ブロードフェーズ
    m_broadphase = std::make_unique<btDbvtBroadphase>();

    // ソルバー
    m_solver =
        std::make_unique<btSequentialImpulseConstraintSolver>();

    // ワールド作成
    m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(),
        m_broadphase.get(),
        m_solver.get(),
        m_collisionConfiguration.get()
    );

    OutputDebugStringA("Bullet Physics initialized!\n");
}

void Game::CleanupPhysics()
{
    // 全ての敵の物理ボディを削除
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

    OutputDebugStringA("[BULLET] All enemy physics bodies cleaned up\n");

    // Bullet Physics ワールドをクリーンアップ
    m_dynamicsWorld.reset();
    m_solver.reset();
    m_broadphase.reset();
    m_dispatcher.reset();
    m_collisionConfiguration.reset();

    OutputDebugStringA("Bullet Physics cleaned up!\n");
}

//  === RayPhysics  ===
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

    // === デバッグ: レイの開始位置と方向を出力 ===
    char debugBuffer[512];
    sprintf_s(debugBuffer, "[RAYCAST] Start:(%.2f, %.2f, %.2f) Dir:(%.2f, %.2f, %.2f) Dist:%.2f\n",
        start.x, start.y, start.z,
        direction.x, direction.y, direction.z,
        maxDistance);
    OutputDebugStringA(debugBuffer);

    //  方向を正規化
    float length = sqrtf(
        direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z
    );

    if (length < 0.0001f)
        return result;

    direction.x /= length;
    direction.y /= length;
    direction.z /= length;

    //  レイの始点と終点
    btVector3 rayStart(start.x, start.y, start.z);
    btVector3 rayEnd(
        start.x + direction.x * maxDistance,
        start.y + direction.y * maxDistance,
        start.z + direction.z * maxDistance
    );

    //  レイキャスト実行
    btCollisionWorld::ClosestRayResultCallback rayCallback(
        rayStart,
        rayEnd
    );

    //  全てのオブジェクトと衝突判定
    rayCallback.m_collisionFilterMask = -1;

    m_dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);

    if (rayCallback.hasHit())
    {
        result.hit = true;

        //  ヒット位置取得
        result.hitPoint.x = rayCallback.m_hitPointWorld.getX();
        result.hitPoint.y = rayCallback.m_hitPointWorld.getY();
        result.hitPoint.z = rayCallback.m_hitPointWorld.getZ();

        //  ヒット法線取得
        result.hitNormal.x = rayCallback.m_hitNormalWorld.getX();
        result.hitNormal.y = rayCallback.m_hitNormalWorld.getY();
        result.hitNormal.z = rayCallback.m_hitNormalWorld.getZ();

        // === UserPointer から敵IDを取得 ===
        const btCollisionObject* hitObj = rayCallback.m_collisionObject;
        void* userPtr = hitObj->getUserPointer();

        if (userPtr != nullptr)
        {
            // void* を int に変換
            int enemyID = (int)(intptr_t)userPtr - 1;

            // === デバッグ: UserPointerから取得したIDを出力 ===
            char debugID[256];
            sprintf_s(debugID, "[DEBUG] UserPointer EnemyID: %d\n", enemyID);
            OutputDebugStringA(debugID);

            // === デバッグ: 現在の敵リストを出力 ===
            const auto& enemies = m_enemySystem->GetEnemies();
            char debugCount[256];
            sprintf_s(debugCount, "[DEBUG] Total enemies in system: %zu\n", enemies.size());
            OutputDebugStringA(debugCount);

            // IDから敵を検索
            bool found = false;
            for (const auto& enemy : enemies)  // ← constに変更
            {
                // === デバッグ: 各敵の情報を出力 ===
                char debugEnemy[512];
                sprintf_s(debugEnemy, "[DEBUG] Checking enemy ID:%d, isAlive:%d, position:(%.2f, %.2f, %.2f)\n",
                    enemy.id, enemy.isAlive ? 1 : 0,
                    enemy.position.x, enemy.position.y, enemy.position.z);
                OutputDebugStringA(debugEnemy);

                if (enemy.id == enemyID && enemy.isAlive)
                {
                    // === 重要: const_castを使って非constポインタを取得 ===
                    result.hitEnemy = const_cast<Enemy*>(&enemy);
                    found = true;

                    char buffer[512];
                    sprintf_s(buffer,
                        "[BULLET] ★★★ HIT ENEMY! ★★★ ID:%d at (%.2f, %.2f, %.2f) - Enemy at (%.2f, %.2f, %.2f)\n",
                        enemyID,
                        result.hitPoint.x, result.hitPoint.y, result.hitPoint.z,
                        enemy.position.x, enemy.position.y, enemy.position.z);
                    OutputDebugStringA(buffer);
                    break;
                }
            }

            if (!found)
            {
                // IDが見つからない = 死んだ敵か壁
                char buffer[256];
                sprintf_s(buffer, "[BULLET] Hit dead enemy or wall (ID:%d) - Enemy not found or not alive\n", enemyID);
                OutputDebugStringA(buffer);
            }
        }
        else
        {
            // 壁に当たった
            char buffer[256];
            sprintf_s(buffer,
                "[BULLET] Raycast HIT WALL at (%.2f, %.2f, %.2f)\n",
                result.hitPoint.x, result.hitPoint.y, result.hitPoint.z);
            OutputDebugStringA(buffer);
        }
    }
    else
    {
        OutputDebugStringA("[BULLET] Raycast MISS\n");
    }

    return result;
}

void Game::AddEnemyPhysicsBody(Enemy& enemy)
{
    //  カプセル形状を生成
    EnemyTypeConfig config = GetEnemyConfig(enemy.type);

    float radius = config.bodyWidth / 2.0f;
    float totalHeight = config.bodyHeight;
    float cylinderHeight = totalHeight - 2.0f * radius;

    if (cylinderHeight < 0.0f)
        cylinderHeight = 0.0f;

    btCollisionShape* shape = new btCapsuleShape(radius, cylinderHeight);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        enemy.position.x,
        totalHeight / 2.0f,
        enemy.position.z
    ));

    //  モーションステート
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);

    //  剛体作成（静的: mass = 0）
    btScalar mass = 0.0f;
    btVector3 localInertia(0, 0, 0);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        mass,
        motionState,
        shape,
        localInertia
    );

    btRigidBody* body = new btRigidBody(rbInfo);

    //  キネマティックフラグを設定（レイキャストに当たるようにする）
    body->setCollisionFlags(
        body->getCollisionFlags() |
        btCollisionObject::CF_KINEMATIC_OBJECT
    );

    //  常にアクティブにする
    body->setActivationState(DISABLE_DEACTIVATION);

    // === 重要: 敵のIDを保存（ポインタではなくID） ===
    body->setUserPointer((void*)(intptr_t)(enemy.id + 1));  // ← 修正！int を void* にキャスト

    m_dynamicsWorld->addRigidBody(body);

    //  === マップに保存  ===
    m_enemyPhysicsBodies[enemy.id] = body;

    char buffer[256];
    sprintf_s(buffer, "[BULLET] Added body for enemy ID:%d at (%.2f, %.2f, %.2f)\n",
        enemy.id, enemy.position.x, enemy.position.y, enemy.position.z);
    OutputDebugStringA(buffer);
}

void Game::UpdateEnemyPhysicsBody(Enemy& enemy)
{
    // === デバッグ: 物理ボディの検索 ===
    char debugSearch[256];
    sprintf_s(debugSearch, "[PHYSICS UPDATE] Searching for enemy ID:%d physics body\n", enemy.id);
    OutputDebugStringA(debugSearch);

    auto it = m_enemyPhysicsBodies.find(enemy.id);
    if (it == m_enemyPhysicsBodies.end())
    {
        // === デバッグ: 物理ボディが見つからない ===
        char debugNotFound[256];
        sprintf_s(debugNotFound, "[PHYSICS UPDATE] ? Physics body NOT FOUND for enemy ID:%d\n", enemy.id);
        OutputDebugStringA(debugNotFound);

        // === デバッグ: 登録されている物理ボディのIDを表示 ===
        OutputDebugStringA("[PHYSICS UPDATE] Registered physics bodies:\n");
        for (const auto& pair : m_enemyPhysicsBodies)
        {
            char debugRegistered[256];
            sprintf_s(debugRegistered, "  - Enemy ID:%d\n", pair.first);
            OutputDebugStringA(debugRegistered);
        }
        return;
    }

    // === デバッグ: 物理ボディが見つかった ===
    char debugFound[256];
    sprintf_s(debugFound, "[PHYSICS UPDATE] ? Found physics body for enemy ID:%d\n", enemy.id);
    OutputDebugStringA(debugFound);

    btRigidBody* body = it->second;

    EnemyTypeConfig config = GetEnemyConfig(enemy.type);
    float totalHeight = config.bodyHeight;

    // === デバッグ: 更新前の位置 ===
    btTransform oldTransform;
    body->getMotionState()->getWorldTransform(oldTransform);
    btVector3 oldPos = oldTransform.getOrigin();

    char debugOldPos[256];
    sprintf_s(debugOldPos, "[PHYSICS UPDATE] Old position: (%.2f, %.2f, %.2f)\n",
        oldPos.x(), oldPos.y(), oldPos.z());
    OutputDebugStringA(debugOldPos);

    // === デバッグ: 新しい位置 ===
    char debugNewPos[256];
    sprintf_s(debugNewPos, "[PHYSICS UPDATE] New position: (%.2f, %.2f, %.2f)\n",
        enemy.position.x, totalHeight / 2.0f, enemy.position.z);
    OutputDebugStringA(debugNewPos);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        enemy.position.x,
        totalHeight / 2.0f,
        enemy.position.z
    ));

    body->setWorldTransform(transform);

    if (body->getMotionState())
    {
        body->getMotionState()->setWorldTransform(transform);
    }

    // === デバッグ: 更新完了 ===
    OutputDebugStringA("[PHYSICS UPDATE] ? Physics body updated successfully\n\n");
}

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
        OutputDebugStringA(debugBuffer);
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

    case GameState::PLAYING:
        UpdatePlaying();
        break;

    case GameState::GAMEOVER:
        UpdateGameOver();
        break;
    }

    UpdateFade();





}

void Game::Clear()
{
    // 画面を青でクリア
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Black);

    // ★重要: 深度バッファ(DEPTH) と ステンシルバッファ(STENCIL) の両方をクリアする
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
    }

    // レンダーターゲット作成
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr)) throw std::runtime_error("GetBuffer failed");

    hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr,
        m_renderTargetView.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateRenderTargetView failed");

    // 深度バッファ作成
    CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,
        backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateTexture2D failed");

    hr = m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), nullptr,
        m_depthStencilView.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateDepthStencilView failed");
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
    if (!m_weaponModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/PISTOL.fbx"))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to load weapon model (M1911)\n");
    }
    else
    {
        OutputDebugStringA("Game::CreateRenderResources - Weapon model (M1911) loaded successfully!\n");
    }

    //  壁武器用モデル読み込み
    m_pistolModel = std::make_unique<Model>();
    if (!m_pistolModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/PISTOL.fbx"))
    {
        OutputDebugStringA("Failed to load M1911 model for wall weapon\n");
    }


    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_d3dContext.Get());

  /*  m_testModel = std::make_unique<Model>();*/

    ////  プレイヤーモデルファイルを読み込む
    //if (!m_testModel->LoadFromFile(m_d3dDevice.Get(), "Assets/X_Bot.fbx"))
    //{
    //    OutputDebugStringA("Game::CreateRenderResources - Failed to load model!\n");
    //}
    //else
    //{
    //    OutputDebugStringA("Game::CreateRenderResources - Model loaded successfully!\n");
    //}

    //  === 敵NORMALモデル   ===
    m_enemyModel = std::make_unique<Model>();
    if (!m_enemyModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Y_Bot/Y_Bot.fbx"))
    {
        OutputDebugStringA("Failed to load enemy model!\n");
        //throw std::runtime_error("Failed to load enemy model");
    }
    else
    {
        OutputDebugStringA("Enemy model loaded successfully!\n");
    }

    //  === 敵RUNNERモデル  ===
    m_runnerModel = std::make_unique<Model>();
    if (!m_runnerModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Runner/Runner.fbx"))
    {
        OutputDebugStringA("Failed to load Runner model!\n");
    }
    else
    {
        OutputDebugStringA("Runner model loaded successfully!\n");
    }

    //  === 敵TANK   ===
    m_tankModel = std::make_unique<Model>();
    if (!m_tankModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Tank/Tank.fbx"))
    {
        OutputDebugStringA("Failed to load Tank model");
    }
    else
    {
        OutputDebugStringA("Tank model loaded successfully!\n");
    }

    //  ====================================
    //  === NORMALモデルのアニメーション   ===
    //  ====================================
    {
        OutputDebugStringA("=== Loading Y_Bot animations  ===\n");

        //  --- 歩行アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Walk.fbx", "Walk"))
        {
            OutputDebugStringA("Failed to load Walk animation\n");
        }

        //  --- 待機アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Idle.fbx", "Idle"))
        {
            OutputDebugStringA("Failed to load Idle animation\n");
        }

        //  --- 走りアニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Run.fbx", "Run"))
        {
            OutputDebugStringA("Failed to load Run animation\n");
        }

        //  --- 攻撃アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Attack.fbx", "Attack"))
        {
            OutputDebugStringA("Failed to load Attack animation");
        }

        //  --- 死亡アニメーション   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Death.fbx", "Death"))
        {
            OutputDebugStringA("Failed to load Death Animation");
        }
    }

    //  ===================================
    //  === RUNNERモデルアニメーション    ===
    //  ===================================
    {
        //  === RUNNERアニメーション読み込み   ===
        OutputDebugStringA("=== Loading Runner animations ===\n");

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Walk.fbx", "Walk"))
        {
            OutputDebugStringA("Failed to load Runner Walk animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Run.fbx", "Run"))
        {
            OutputDebugStringA("Failed to load Runner Run animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Attack.fbx", "Attack"))
        {
            OutputDebugStringA("Failed to load Runner Attack animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Idle.fbx", "Idle"))
        {
            OutputDebugStringA("Failed to load Runner Idle animation\n");
        }

        if (!m_runnerModel->LoadAnimation("Assets/Models/Runner/Runner_Death.fbx", "Death"))
        {
            OutputDebugStringA("Failed to load Runner Death animation\n");
        }
    }

    //  =========================
    //  === TANKアニメーション ===
    //  =========================
    {
        //  === Tank アニメーション読み込み ===
        OutputDebugStringA("=== Loading Tank animations ===\n");

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Walk.fbx", "Walk"))
        {
            OutputDebugStringA("Failed to load Tank Walk animation\n");
        }

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Attack.fbx", "Attack"))
        {
            OutputDebugStringA("Failed to load Tank Attack animation\n");
        }

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Idle.fbx", "Idle"))
        {
            OutputDebugStringA("Failed to load Tank Idle animation\n");
        }

        if (!m_tankModel->LoadAnimation("Assets/Models/Tank/Tank_Death.fbx", "Death"))
        {
            OutputDebugStringA("Failed to load Tank Death animation\n");
        }
    
    }

    //  === MapSystem   初期化 ===
    m_mapSystem = std::make_unique<MapSystem>();
    if (!m_mapSystem->Initialize(m_d3dContext.Get()))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to initialize MapSystem\n");
    }
    else
    {
        m_mapSystem->CreateDefaultMap();
        OutputDebugStringA("Game::CreateRenderResources - MapSystem initialized successfully\n");
    }


    //  === WeaponSystem 初期化    2025/11/14  ===
    m_weaponSpawnSystem = std::make_unique<WeaponSpawnSystem>();
    m_weaponSpawnSystem->CreateDefaultSpawns();

    //  --- テクスチャ読み込み   2025/11/19  ---
    if (!m_weaponSpawnSystem->InitializeTextures(m_d3dDevice.Get(), m_d3dContext.Get()))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to initialize weapon textures\n");
    }

    m_nearbyWeaponSpawn = nullptr;

    OutputDebugStringA("Game::CreateRenderResources - WeaponSpawnSystem initialized\n");
    
    //  === Shadow を初期化  ===
    m_shadow = std::make_unique<Shadow>();
    if (!m_shadow->Initialize(m_d3dDevice.Get()))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to initialize Shadow System!\n");
        // エラー処理
    }
    else
    {
        // 影の色を黒の半透明に設定
        m_shadow->SetShadowColor(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f));
    }

    //  === Effekseerの初期化   ===
    
      /*描画管理クラス(レンダラー)の作成
      第3引数は最大パーティクル数*/
   /* m_effekseerRenderer = EffekseerRendererDX11::Renderer::Create(
        m_d3dDevice.Get(),
        m_d3dContext.Get(),
        8000
    );*/

    //  エッフェクト管理クラス
    //m_effekseerManager = Effekseer::Manager::Create(8000);

    ////  描画設定の紐づけ
    //m_effekseerManager->SetSpriteRenderer(m_effekseerRenderer->CreateSpriteRenderer());
    //m_effekseerManager->SetRibbonRenderer(m_effekseerRenderer->CreateRibbonRenderer());
    //m_effekseerManager->SetRingRenderer(m_effekseerRenderer->CreateRingRenderer());
    //m_effekseerManager->SetTrackRenderer(m_effekseerRenderer->CreateTrackRenderer());
    //m_effekseerManager->SetModelRenderer(m_effekseerRenderer->CreateModelRenderer());

    ////  エッフェクトファイルの読み込み
    //// ※ パスは u"..." で囲んで UTF-16 文字列にする
    //m_effectBlood = Effekseer::Effect::Create(m_effekseerManager, u"Assets//Effects/Laser01.efkefc");

    //if (m_effectBlood == nullptr)
    //{
    //    OutputDebugStringA("Failed to load Effekseer effect!\n");
    //}
    //else
    //{
    //    OutputDebugStringA("Effekseer initialized successfully!\n");
    //}

    //  === Imgui   ===
    InitImGui();
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

        // 表示トグル
        ImGui::Text("Visual Debug:");
        ImGui::Checkbox("Show Body Hitboxes", &m_showHitboxes);
        ImGui::Checkbox("Show Head Hitboxes", &m_showHeadHitboxes);
        ImGui::Checkbox("Show Physics Capsules", &m_showPhysicsHitboxes);  // ← 追加
        ImGui::Checkbox("Show Bullet Trajectory", &m_showBulletTrajectory);

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
                    OutputDebugStringA("NORMAL config copied to clipboard!\n");
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
                    OutputDebugStringA("RUNNER config copied to clipboard!\n");
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
                    OutputDebugStringA("TANK config copied to clipboard!\n");
                }

                ImGui::TreePop();
            }

            // リセットボタン
            if (ImGui::Button("Reset All to Default"))
            {
                m_normalConfigDebug = GetEnemyConfig(EnemyType::NORMAL);
                m_runnerConfigDebug = GetEnemyConfig(EnemyType::RUNNER);
                m_tankConfigDebug = GetEnemyConfig(EnemyType::TANK);
                OutputDebugStringA("Reset all hitbox configs to default\n");
            }

            ImGui::Separator();
            ImGui::TextWrapped("Tip: Adjust values while looking at hitboxes, then click 'Copy to Clipboard' and paste into Entities.h");
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
    DirectX::XMFLOAT3 rayStart(playerPos.x, playerPos.y + 0.5f, playerPos.z);

    DirectX::XMFLOAT4 crossColor(0.0f, 1.0f, 0.0f, 1.0f);  // 緑
    float crossSize = 0.1f;

    // X軸の線
    primitiveBatch->DrawLine(
        DirectX::VertexPositionColor(
            DirectX::XMFLOAT3(rayStart.x - crossSize, rayStart.y, rayStart.z),
            crossColor
        ),
        DirectX::VertexPositionColor(
            DirectX::XMFLOAT3(rayStart.x + crossSize, rayStart.y, rayStart.z),
            crossColor
        )
    );

    // Y軸の線
    primitiveBatch->DrawLine(
        DirectX::VertexPositionColor(
            DirectX::XMFLOAT3(rayStart.x, rayStart.y - crossSize, rayStart.z),
            crossColor
        ),
        DirectX::VertexPositionColor(
            DirectX::XMFLOAT3(rayStart.x, rayStart.y + crossSize, rayStart.z),
            crossColor
        )
    );

    // Z軸の線
    primitiveBatch->DrawLine(
        DirectX::VertexPositionColor(
            DirectX::XMFLOAT3(rayStart.x, rayStart.y, rayStart.z - crossSize),
            crossColor
        ),
        DirectX::VertexPositionColor(
            DirectX::XMFLOAT3(rayStart.x, rayStart.y, rayStart.z + crossSize),
            crossColor
        )
    );

    primitiveBatch->End();
}


void Game::DrawEnemies()
{
    static int frameCount = 0;
    frameCount++;

    //	===	1秒ごとにデバッグ情報を出力	===
    if (frameCount % 60 == 0)
    {
        auto& enemies = m_enemySystem->GetEnemies();

        char debugMsg[512];
        sprintf_s(debugMsg, "=== Enemies: %zu ===\n", enemies.size());
        OutputDebugStringA(debugMsg);
    }

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    //  --- カメラシェイクを適用  ---
    DirectX::XMFLOAT3 shakeOffset(0.0f, 0.0f, 0.0f);
    if (m_cameraShakeTimer > 0.0f)
    {
        shakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
        shakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
        shakeOffset.z = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
    }

    DirectX::XMVECTOR cameraPosition = DirectX::XMVectorSet(
        playerPos.x + shakeOffset.x,
        playerPos.y + shakeOffset.y,
        playerPos.z + shakeOffset.z,
        0.0f
    );

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

    DirectX::XMFLOAT3 lightDir = { 1.0f, -1.0f, 1.0f };

    //	========================================================================
    //  インスタンスデータを作成
    //	========================================================================

    // === Normal（Y_Bot）用 ===
    std::vector<InstanceData> normalWalking;
    std::vector<InstanceData> normalAttacking;
    std::vector<InstanceData> normalWalkingHeadless;
    std::vector<InstanceData> normalAttackingHeadless;
    std::vector<InstanceData> normalDead;
    std::vector<InstanceData> normalDeadHeadless;

    // === Runner用 ===
    std::vector<InstanceData> runnerWalking;
    std::vector<InstanceData> runnerAttacking;
    std::vector<InstanceData> runnerWalkingHeadless;
    std::vector<InstanceData> runnerAttackingHeadless;
    std::vector<InstanceData> runnerDead;
    std::vector<InstanceData> runnerDeadHeadless;

    // === Tank用 ===
    std::vector<InstanceData> tankWalking;
    std::vector<InstanceData> tankAttacking;
    std::vector<InstanceData> tankWalkingHeadless;
    std::vector<InstanceData> tankAttackingHeadless;
    std::vector<InstanceData> tankDead;
    std::vector<InstanceData> tankDeadHeadless;

    //  === MIDBASS用    ===
    

    //  === 死亡アニメーション再生時間の取得    ===
    float deathDuration = m_enemyModel->GetAnimationDuration("Death");

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive && !enemy.isDying)
            continue;

        // ワールド行列を計算
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f);
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
                // ========================================================
                // アニメーション終了後 → インスタンシングで固定ポーズ
                // ========================================================
                InstanceData instance;
                DirectX::XMStoreFloat4x4(&instance.world, world);
                instance.color = enemy.color;

                //  === タイプ別に振り分け   ===
                switch (enemy.type)
                {
                case EnemyType::NORMAL:
                    if (enemy.headDestroyed)
                        normalDeadHeadless.push_back(instance);
                    else
                        normalDead.push_back(instance);
                    break;

                case EnemyType::RUNNER:
                    if (enemy.headDestroyed)
                        runnerDeadHeadless.push_back(instance);
                    else
                        runnerDead.push_back(instance);
                    break;

                case EnemyType::TANK:
                    if (enemy.headDestroyed)
                        tankDeadHeadless.push_back(instance);
                    else
                        tankDead.push_back(instance);
                    break;
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

            // 影を描画
            if (m_shadow)
            {
                m_shadow->RenderShadow(
                    m_d3dContext.Get(), //  コンテキスト
                    enemy.position,     //  位置  
                    1.5f,               //  サイズ
                    viewMatrix,         //  ビュー行列
                    projectionMatrix,   //  projection行列　　
                    lightDir,           //  光の方向
                    0.0f                //  地面の高さ
                );
            }

            continue;  // 生きている敵の処理はスキップ
        }

        // ========================================================================
        // 生きている敵はインスタンスリストへ追加
        // ========================================================================
        InstanceData instance;
        DirectX::XMStoreFloat4x4(&instance.world, world);

        //  === よろめき状態なら点滅させる   ===
        if (enemy.isStaggered)
        {
            float flash = sinf(enemy.staggerFlashTimer);    //  -1.0 - 1.0
            flash = (flash + 1.0f) * 0.5f;  //  0.0 - 1.0に変換

            //  オレンジ色に光る
            instance.color = DirectX::XMFLOAT4(
                1.0f,                    // R: 赤 MAX
                0.5f + flash * 0.5f,     // G: 0.5?1.0 で点滅
                0.0f,                    // B: 青 0
                1.0f
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
                    normalAttackingHeadless.push_back(instance);
                else
                    normalWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                    normalAttacking.push_back(instance);
                else
                    normalWalking.push_back(instance);
            }
            break;

        case EnemyType::RUNNER:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                    runnerAttackingHeadless.push_back(instance);
                else
                    runnerWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                    runnerAttacking.push_back(instance);
                else
                    runnerWalking.push_back(instance);
            }
            break;

        case EnemyType::TANK:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                    tankAttackingHeadless.push_back(instance);
                else
                    tankWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                    tankAttacking.push_back(instance);
                else
                    tankWalking.push_back(instance);
            }
            break;
        }

        // 影を描画
        if (m_shadow)
        {
            m_shadow->RenderShadow(
                m_d3dContext.Get(), //  コンテキスト
                enemy.position,     //  位置
                1.5f,               //  サイズ
                viewMatrix,         //  ビュー行列
                projectionMatrix,   //  プロジェクション行列
                lightDir,           //  光の方向
                0.0f                //  地面の高さ
            );
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
        m_enemyModel->SetBoneScale("Head", 1.0f);

        // Walking - 頭あり
        if (!normalWalking.empty())
        {
            float walkTime = fmod(m_accumulatedAnimTime,
                m_enemyModel->GetAnimationDuration("Walk"));
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), normalWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭あり
        if (!normalAttacking.empty())
        {
            float attackTime = fmod(m_accumulatedAnimTime,
                m_enemyModel->GetAnimationDuration("Attack"));
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), normalAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭あり
        if (!normalDead.empty())
        {
            float finalTime = deathDuration - 0.001f;
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), normalDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // 頭なし描画
        m_enemyModel->SetBoneScale("Head", 0.0f);

        // Walking - 頭なし
        if (!normalWalkingHeadless.empty())
        {
            float walkTime = fmod(m_accumulatedAnimTime,
                m_enemyModel->GetAnimationDuration("Walk"));
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), normalWalkingHeadless,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭なし
        if (!normalAttackingHeadless.empty())
        {
            float attackTime = fmod(m_accumulatedAnimTime,
                m_enemyModel->GetAnimationDuration("Attack"));
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), normalAttackingHeadless,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭なし
        if (!normalDeadHeadless.empty())
        {
            float finalTime = deathDuration - 0.001f;
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), normalDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_enemyModel->SetBoneScale("Head", 1.0f);
    }

    // ====================================================================
    // RUNNER の描画
    // ====================================================================
    if (m_runnerModel)
    {
        m_runnerModel->SetBoneScale("Head", 1.0f);

        // Walking - 頭あり
        if (!runnerWalking.empty())
        {
            float walkTime = fmod(m_accumulatedAnimTime,
                m_runnerModel->GetAnimationDuration("Walk"));
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), runnerWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭あり
        if (!runnerAttacking.empty())
        {
            float attackTime = fmod(m_accumulatedAnimTime,
                m_runnerModel->GetAnimationDuration("Attack"));
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), runnerAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭あり
        if (!runnerDead.empty())
        {
            float finalTime = m_runnerModel->GetAnimationDuration("Death") - 0.001f;
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), runnerDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // 頭なし描画
        m_runnerModel->SetBoneScale("Head", 0.0f);

        // Walking - 頭なし
        if (!runnerWalkingHeadless.empty())
        {
            float walkTime = fmod(m_accumulatedAnimTime,
                m_runnerModel->GetAnimationDuration("Walk"));
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), runnerWalkingHeadless,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭なし
        if (!runnerAttackingHeadless.empty())
        {
            float attackTime = fmod(m_accumulatedAnimTime,
                m_runnerModel->GetAnimationDuration("Attack"));
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), runnerAttackingHeadless,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭なし
        if (!runnerDeadHeadless.empty())
        {
            float finalTime = m_runnerModel->GetAnimationDuration("Death") - 0.001f;
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), runnerDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_runnerModel->SetBoneScale("Head", 1.0f);
    }

    // ====================================================================
    // TANK の描画
    // ====================================================================
    if (m_tankModel)
    {
        m_tankModel->SetBoneScale("Head", 1.0f);

        // Walking - 頭あり
        if (!tankWalking.empty())
        {
            float walkTime = fmod(m_accumulatedAnimTime,
                m_tankModel->GetAnimationDuration("Walk"));
            m_tankModel->DrawInstanced(m_d3dContext.Get(), tankWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭あり
        if (!tankAttacking.empty())
        {
            float attackTime = fmod(m_accumulatedAnimTime,
                m_tankModel->GetAnimationDuration("Attack"));
            m_tankModel->DrawInstanced(m_d3dContext.Get(), tankAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭あり
        if (!tankDead.empty())
        {
            float finalTime = m_tankModel->GetAnimationDuration("Death") - 0.001f;
            m_tankModel->DrawInstanced(m_d3dContext.Get(), tankDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // 頭なし描画
        m_tankModel->SetBoneScale("Head", 0.0f);

        // Walking - 頭なし
        if (!tankWalkingHeadless.empty())
        {
            float walkTime = fmod(m_accumulatedAnimTime,
                m_tankModel->GetAnimationDuration("Walk"));
            m_tankModel->DrawInstanced(m_d3dContext.Get(), tankWalkingHeadless,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭なし
        if (!tankAttackingHeadless.empty())
        {
            float attackTime = fmod(m_accumulatedAnimTime,
                m_tankModel->GetAnimationDuration("Attack"));
            m_tankModel->DrawInstanced(m_d3dContext.Get(), tankAttackingHeadless,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭なし
        if (!tankDeadHeadless.empty())
        {
            float finalTime = m_tankModel->GetAnimationDuration("Death") - 0.001f;
            m_tankModel->DrawInstanced(m_d3dContext.Get(), tankDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_tankModel->SetBoneScale("Head", 1.0f);
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

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying || enemy.health >= enemy.maxHealth)
            continue;

        float barWidth = 1.0f;
        float healthPercent = (float)enemy.health / enemy.maxHealth;

        DirectX::XMFLOAT3 barCenter = enemy.position;
        barCenter.y += 2.5f;

        //	背景
        DirectX::XMFLOAT4 bgColor(0.5f, 0.0f, 0.0f, 1.0f);
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x - barWidth / 2, barCenter.y, barCenter.z),
                bgColor
            ),
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x + barWidth / 2, barCenter.y, barCenter.z),
                bgColor
            )
        );

        //	HP部分
        DirectX::XMFLOAT4 hpColor(1.0f, 0.0f, 0.0f, 1.0f);
        float currentBarWidth = barWidth * healthPercent;
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x - barWidth / 2, barCenter.y + 0.05f, barCenter.z),
                hpColor
            ),
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x - barWidth / 2 + currentBarWidth, barCenter.y + 0.05f, barCenter.z),
                hpColor
            )
        );
    }



    primitiveBatch->End();
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
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
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

    // ビュー・プロジェクション行列の計算
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
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    // 重要：頂点カラーを有効化
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    for (const auto& particle : m_particleSystem->GetParticles())
    {
        float size = 0.1f; // サイズを大きくして見やすく 

        // より見やすい十字形で描画
        DirectX::XMFLOAT3 center = particle.position;

        // 横線
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x - size, center.y, center.z), particle.color),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x + size, center.y, center.z), particle.color)
        );

        // 縦線
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x, center.y - size, center.z), particle.color),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x, center.y + size, center.z), particle.color)
        );
    }

    primitiveBatch->End();
}


void Game::UpdateTitle()
{
    // デバッグ：フェード値を表示
    char debug[256];
    sprintf_s(debug, "TITLE - Press SPACE - Fade:%.2f", m_fadeAlpha);
    SetWindowTextA(m_window, debug);

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
        m_gameState = GameState::PLAYING;
        m_fadeAlpha = 1.0f;
        m_fadingIn = true;
        m_fadeActive = true;
    }
}



// ========================================
// より正確なデルタタイム版（推奨）
// ========================================


void Game::DrawWeapon()
{
    if (!m_weaponModel)
    {
        return;
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
    DirectX::XMVECTOR rightOffset = XMVectorScale(right, 0.25f + m_weaponSwayX * 0.1f);
    DirectX::XMVECTOR upOffset = XMVectorScale(up, -0.35f + m_weaponSwayY * 0.1f);
    DirectX::XMVECTOR forwardOffset = XMVectorScale(forward, 0.5f);

    DirectX::XMVECTOR weaponPos = cameraPosition;
    weaponPos = XMVectorAdd(weaponPos, rightOffset);
    weaponPos = XMVectorAdd(weaponPos, upOffset);
    weaponPos = XMVectorAdd(weaponPos, forwardOffset);

    // === スケール ===
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.25f, 0.25f, 0.25f);

    // === 回転 ===
    DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PIDIV2);  // Y軸で90度
    DirectX::XMMATRIX standUp = DirectX::XMMatrixRotationZ(-DirectX::XM_PIDIV2);   // Z軸で90度回転（立てる）
    DirectX::XMMATRIX tilt = DirectX::XMMatrixRotationX(-0.1f);                   // 少し傾ける
    DirectX::XMMATRIX modelRotation = DirectX::XMMatrixRotationY(DirectX::XM_PI);
    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(weaponPos);

    // 合成（standUp を追加）
    DirectX::XMMATRIX weaponWorld = scale * modelFix * standUp * tilt * modelRotation * cameraRotation * translation;

    m_weaponModel->Draw(
        m_d3dContext.Get(),
        weaponWorld,
        viewMatrix,
        projectionMatrix,
        DirectX::Colors::White
    );
}

// =================================================================
// 【最上位】描画全体の司令塔
// =================================================================
void Game::Render()
{
    Clear();

    switch (m_gameState)
    {
    case GameState::TITLE:
        RenderTitle();
        break;
    case GameState::PLAYING:
        RenderPlaying();
        break;
    case GameState::GAMEOVER:
        RenderGameOver();
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
    //  === ビュー・プロジェクション行列の計算   ===
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition;
    DirectX::XMVECTOR cameraTarget;

    //  グローリーキルカメラ
    if (m_gloryKillCameraActive)
    {
        //  カメラシェイクは無効か
        cameraPosition = DirectX::XMVectorSet(
            m_gloryKillCameraPos.x,
            m_gloryKillCameraPos.y,
            m_gloryKillCameraPos.z,
            0.0f
        );

        cameraTarget = DirectX::XMVectorSet(
            m_gloryKillCameraTarget.x,
            m_gloryKillCameraTarget.y,
            m_gloryKillCameraTarget.z,
            0.0f
        );
    }
    else
    {
        //  カメラシェイク適用
        DirectX::XMFLOAT3 shakeOffset(0.0f, 0.0f, 0.0f);
        if (m_cameraShakeTimer > 0.0f)
        {
            shakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.z = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
        }

        // カメラ位置にシェイクを加える
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
            DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
        );
    

        //  === マップ描画   ===
        if (m_mapSystem)
        {
            m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }


        //  === 壁武器テクスチャ描画  ===
        if (m_weaponSpawnSystem)
        {
            m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }
    

    // 最初に3D空間のものをすべて描画する
    DrawParticles();
    //DrawGrid();
    DrawEnemies();
    DrawBillboard();
    DrawWeapon();
    DrawWeaponSpawns();


    // === グローリーキル腕・ナイフ描画 ===
    if (m_gloryKillArmAnimActive && m_gloryKillArm && m_gloryKillKnife)
    {
        // 【重要】現在のカメラの位置とターゲットを使う
        DirectX::XMFLOAT3 camPos, camTarget;
        DirectX::XMStoreFloat3(&camPos, cameraPosition);
        DirectX::XMStoreFloat3(&camTarget, cameraTarget);

        // カメラの向きベクトルを計算
        DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(cameraTarget, cameraPosition);
        forward = DirectX::XMVector3Normalize(forward);

        // カメラの右ベクトルを計算
        DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMVECTOR right = DirectX::XMVector3Cross(worldUp, forward);
        right = DirectX::XMVector3Normalize(right);

        // カメラの上ベクトルを再計算
        DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

        // 腕の位置（カメラの前、画面内に固定）
        DirectX::XMVECTOR armWorldPos = cameraPosition;
        armWorldPos += forward * 0.8f;  // カメラの前
        armWorldPos += right * m_gloryKillArmPos.x * 0.4f;    // 右（アニメーション）
        armWorldPos += up * m_gloryKillArmPos.y;    // 上下

        // カメラの向きから回転を計算
        float cameraYaw = atan2f(
            DirectX::XMVectorGetX(forward),
            DirectX::XMVectorGetZ(forward)
        );

        // 腕のワールド行列（横向き、カメラに対して垂直）
        DirectX::XMMATRIX armWorld = DirectX::XMMatrixIdentity();
        // まず横向きに回転（90度）
        armWorld *= DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2);  // Z軸で90度回転（横向き）
        // カメラの向きに合わせる
        armWorld *= DirectX::XMMatrixRotationY(cameraYaw);
        // 位置を設定
        armWorld *= DirectX::XMMatrixTranslationFromVector(armWorldPos);

        // 腕を描画（肌色に近い色）
        DirectX::XMVECTOR skinColor = DirectX::XMVectorSet(0.8f, 0.6f, 0.5f, 1.0f);
        m_gloryKillArm->Draw(armWorld, viewMatrix, projectionMatrix, skinColor);

        // ナイフの位置（腕の先端、左側）
        DirectX::XMVECTOR knifeLocalOffset = DirectX::XMVectorSet(0.0f, 0.0f, 0.35f, 0.0f);  // 腕の先端

        // ナイフのワールド行列
        DirectX::XMMATRIX knifeWorld = DirectX::XMMatrixIdentity();
        // ナイフを横向きに
        knifeWorld *= DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2);  // 横向き
        knifeWorld *= DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);  // 先端を前に
        // 腕のローカル座標でオフセット
        knifeWorld *= DirectX::XMMatrixTranslationFromVector(knifeLocalOffset);
        // 腕と同じ回転
        knifeWorld *= DirectX::XMMatrixRotationY(cameraYaw);
        // 腕と同じ位置
        knifeWorld *= DirectX::XMMatrixTranslationFromVector(armWorldPos);

        // ナイフを描画（銀色）
        m_gloryKillKnife->Draw(knifeWorld, viewMatrix, projectionMatrix, DirectX::Colors::Silver);
    }


    //  === Effekseerの描画　===
    //if (m_effekseerManager != nullptr && m_effekseerRenderer != nullptr)
    //{
    //    //  変換用変数を用意
    //    Effekseer::Matrix44 efkView;
    //    Effekseer::Matrix44 efkProj;

    //    //  DirectXの行列をEffekseer用の変数にコピーする
    //    //  (reinterpret_castを使って「中身は同じ4x4の数字だよ」と伝えてコピーする)
    //    DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&efkView), viewMatrix);
    //    DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&efkProj), projectionMatrix);

    //    //  変数にしたのを渡す
    //    m_effekseerRenderer->SetCameraMatrix(efkView);
    //    m_effekseerRenderer->SetProjectionMatrix(efkProj);

    //    m_effekseerRenderer->BeginRendering();
    //    m_effekseerManager->Draw();
    //    m_effekseerRenderer->EndRendering();
    //}

    if (m_shadow)
    {
        DirectX::XMFLOAT3 lightDirection(0.5f, -1.0f, 0.5f);
        float groundY = 0.0f;

        const auto& enemies = m_enemySystem->GetEnemies();
        for (const auto& enemy : enemies)
        {
            if (!enemy.isAlive) continue;

            m_shadow->RenderShadow(
                m_d3dContext.Get(),
                enemy.position,          // 敵の位置
                1.5f,                    // 影のサイズ
                viewMatrix,
                projectionMatrix,
                lightDirection,
                groundY
            );
        }
    }
    // 最後にUIをすべて手前に描画する
    DrawUI();

    

    // ダメージエフェクトを一番上に重ねる
    if (m_player->IsDamaged())
    {
        RenderDamageFlash();
    }

    //  グローリーキルの画面フラッシュ
    //RenderGloryKillFlash();

    // === デバッグ描画 ===
    DrawHitboxes();      // 当たり判定可視化
    DrawDebugUI();       // ImGUIウィンドウ
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
    if (m_nearbyWeaponSpawn != nullptr)
    {
        // すでに持っている武器か？
        bool alreadyOwned = false;
        if (m_weaponSystem->GetPrimaryWeapon() == m_nearbyWeaponSpawn->weaponType ||
            (m_weaponSystem->HasSecondaryWeapon() &&
                m_weaponSystem->GetSecondaryWeapon() == m_nearbyWeaponSpawn->weaponType))
        {
            alreadyOwned = true;
        }

        m_uiSystem->DrawWeaponPrompt(
            primitiveBatch.get(),
            m_nearbyWeaponSpawn,
            m_player->GetPoints(),
            alreadyOwned
        );
    }


    primitiveBatch->End();

}


void Game::UpdatePlaying()
{
    // F1キーでデバッグウィンドウ切り替え
    static bool f1Pressed = false;
    if (GetAsyncKeyState(VK_F1) & 0x8000)
    {
        if (!f1Pressed)
        {
            m_showDebugWindow = !m_showDebugWindow;
            f1Pressed = true;
        }
    }
    else
    {
        f1Pressed = false;
    }

    float deltaTime = m_deltaTime * m_timeScale;
    m_accumulatedAnimTime += deltaTime * 0.5f;

    // === 敵に物理ボディを追加（まだ追加されてない敵のみ）===
    static bool enemiesInitialized = false;
    if (!enemiesInitialized && m_enemySystem->GetEnemies().size() > 0)
    {
        OutputDebugStringA("Initializing enemy physics bodies...\n");
        for (auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.isAlive)
            {
                AddEnemyPhysicsBody(enemy);
            }
        }
        enemiesInitialized = true;
        OutputDebugStringA("Enemy physics bodies initialized!\n");
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
        m_hitStopTimer -= deltaTime;

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

            // === カメラをターゲット敵に向ける ===
            m_gloryKillCameraActive = true;
            m_gloryKillCameraLerpTime = 0.0f;

            // カメラ位置：プレイヤーとターゲット敵の間、やや上から
            DirectX::XMFLOAT3 playerPosition = m_player->GetPosition();
            DirectX::XMFLOAT3 enemyPos = m_gloryKillTargetEnemy->position;

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
            m_gloryKillCameraPos.x = playerPosition.x - dx * 1.0f;
            m_gloryKillCameraPos.y = playerPosition.y - 0.3f;  // 少し上
            m_gloryKillCameraPos.z = playerPosition.z - dz * 1.0f;

            // カメラターゲット：敵の中心（胸の高さ）
            m_gloryKillCameraTarget = enemyPos;
            m_gloryKillCameraTarget.y += 1.0f;

            // 被写界深度を有効化
            m_gloryKillDOFActive = true;
            m_gloryKillDOFIntensity = 0.0f;  // 徐々に強くする

            // 敵を即座に倒す
            m_gloryKillTargetEnemy->isDying = true;
            m_gloryKillTargetEnemy->animationTime = 0.0f;
            m_gloryKillTargetEnemy->currentAnimation = "Death";
            m_gloryKillTargetEnemy->corpseTimer = 3.0f;

            // HP回復（最大HPを超えないように）
            int currentHP = m_player->GetHealth();
            int maxHP = m_player->GetMaxHealth();
            int healAmount = 30;
            int newHP = min(currentHP + healAmount, maxHP);
            m_player->SetHealth(newHP);

            // スコア換算
            m_score += 100;

            // 4. デバッグログ
            char buffer[256];
            sprintf_s(buffer, "[GLORY KILL] Enemy ID:%d killed! HP: %d→%d, Score: +100\n",
                m_gloryKillTargetEnemy->id, currentHP, newHP);
            OutputDebugStringA(buffer);


            // カメラシェイク（強め）
            m_cameraShake = 1.5f;      // 0.3f → 0.5f（強く）
            m_cameraShakeTimer = 0.5f;  // 0.2f → 0.3f（長く）

            // 無敵時間（1秒）
            m_gloryKillInvincibleTimer = 1.0f;

            // スローモーション（0.5秒間、0.3倍速）
            /*m_timeScale = 0.1f;*/
            
            // 血しぶきパーティクル（大量）
            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
            m_particleSystem->CreateBloodEffect(m_gloryKillTargetEnemy->position, upDir, 800);

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
        m_slowMoTimer -= deltaTime;
        if (m_slowMoTimer <= 0.0f)
        {
            m_timeScale = 1.0f;
            m_gloryKillCameraActive = false;
            m_gloryKillDOFActive = false;
            m_gloryKillDOFIntensity = 0.0f;
        }
    }

    //  --- カメラシェイク減衰   ---
    if (m_cameraShakeTimer > 0.0f)
    {
        m_cameraShakeTimer -= deltaTime;
        m_cameraShake *= 0.9f;
    }

    float deltatime = (1.0f / 60.0f) * m_timeScale;


    m_frameCount++;

    m_fpsTimer += deltatime;

    if (m_fpsTimer >= 1.0f)
    {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;
        //  Output ウィンドウに表示
        char fpsDebug[128];
        sprintf_s(fpsDebug, "=== FPS: %.1f ===\n", m_currentFPS);
        OutputDebugStringA(fpsDebug);
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }

    char debug[256];
    sprintf_s(debug, "Wave:%d | Points:%d | Health:%d | Reloading:%s",
        m_waveManager->GetCurrentWave(), m_player->GetPoints(), m_player->GetHealth(),
        m_weaponSystem->IsReloading() ? "YES" : "NO");  // ← リロード状態を表示
    SetWindowTextA(m_window, debug);

 
 if(!m_gloryKillCameraActive)   //  === プレイヤー移動 + 壁の当たり判定
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

    //  --- 壁・箱との当たり判定チェック  X---
    if (m_mapSystem && m_mapSystem->CheckCollision(testPositionX, playerRadius))
    {
        newPosition.x = oldPosition.x;
    }

    //  Z
    DirectX::XMFLOAT3 testPositionZ = oldPosition;
    testPositionZ.z = newPosition.z;

    if (m_mapSystem && m_mapSystem->CheckCollision(testPositionZ, playerRadius))
    {
        newPosition.z = oldPosition.z;
    }

    m_player->SetPosition(newPosition);
}

    
    //  === 近接攻撃の入力処理   ===
    static bool lastFKeyState = false;  //  前フレームのFキー状態
    bool isMeleeKeyDown = (GetAsyncKeyState('F') & 0x8000) != 0;   //  現在Fキーの状態

    //  Fキーが押された瞬間(前フレームは話されていて、今フレームは押されている)
    if (isMeleeKeyDown && !lastFKeyState)
    {
        //  クールダウン中でなければ近接攻撃を実行
        if (m_player->CanMeleeAttack())
        {
            PerformMeleeAttack();   //  近接攻撃実行
            m_player->StartMeleeAttackCooldown();   //  クールダウン開始

            OutputDebugStringA("[MELEE] Melee attack executed!\n");
        }
        else
        {
            // クールダウン中のログ
            char buffer[64];
            sprintf_s(buffer, "[MELEE] Cooldown: %.2fs\n",
                m_player->GetMeleeAttackCooldown());
            OutputDebugStringA(buffer);
        }
    }

    //  現在の状態を保存(次フレーム用)
    lastFKeyState = isMeleeKeyDown;


    //  === 壁武器購入システム
    m_nearbyWeaponSpawn = m_weaponSpawnSystem->CheckPlayerNearWeapon(
        m_player->GetPosition()
    );

    //  --- Eキーで入力  ---
    static bool eKeyPressed = false;
    if (GetAsyncKeyState('E') & 0x8000)
    {
        if (!eKeyPressed && m_nearbyWeaponSpawn != nullptr)
        {
            //  === 購入処理    ===

            //  既に持っている武器化
            bool already0wned = false;
            if (m_weaponSystem->GetPrimaryWeapon() == m_nearbyWeaponSpawn->weaponType ||
                (m_weaponSystem->HasSecondaryWeapon() &&
                    m_weaponSystem->GetSecondaryWeapon() == m_nearbyWeaponSpawn->weaponType))
            {
                already0wned = true;
            }

            if (already0wned)
            {
                //  --- 弾薬補充 ---
                int ammoCost = m_nearbyWeaponSpawn->cost / 2;

                if (m_player->GetPoints() >= ammoCost)
                {
                    //  ポイント消費
                    m_player->AddPoints(-ammoCost);

                    char msg[128];
                    sprintf_s(msg, "Ammo refilled! -%d points\n", ammoCost);
                    OutputDebugStringA(msg);
                }
                else
                {
                    OutputDebugStringA("Not enough points for ammo!\n");
                }
            }

            else
            {
                //  === 武器購入    ===
                int cost = m_nearbyWeaponSpawn->cost;

                if (m_player->GetPoints() >= cost)
                {
                    m_player->AddPoints(-cost);

                    m_weaponSystem->BuyWeapon(
                    m_nearbyWeaponSpawn->weaponType,
                        m_player->GetPointsRef()
                    );

                    char msg[128];
                    sprintf_s(msg, "Weapon purchased! -%d points\n", cost);
                    OutputDebugStringA(msg);
                }
                else
                {
                    OutputDebugStringA("Not enough points!\n");
                }

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
            m_weaponSystem->GetFireRateTimer() <= 0.0f)
        {
            auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());

            // === デバッグ: 取得した武器データを確認 ===
            char debugWeapon[512];
            sprintf_s(debugWeapon, "[DEBUG WEAPON] GetWeaponData returned: damage=%d, maxAmmo=%d, fireRate=%.2f, cost=%d\n",
                weapon.damage, weapon.maxAmmo, weapon.fireRate, weapon.cost);
            OutputDebugStringA(debugWeapon);

            //  弾を消費
            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() - 1);
            m_weaponSystem->SetFireRateTimer(weapon.fireRate);
            m_weaponSystem->SaveAmmoStatus();   //  弾薬状態を保存

            //  --- カメラリコイル（武器ごとに変える） ---
            WeaponType currentWeapon = m_weaponSystem->GetCurrentWeapon();
            switch (currentWeapon)
            {
            case WeaponType::PISTOL:
                //m_player->AddCameraRecoil(0.0, 0.0);
                m_cameraShake = 0.03f;
                m_cameraShakeTimer = 0.1f;
                break;

            case WeaponType::SHOTGUN:
                //m_player->AddCameraRecoil(0.08f, 0.03f);
                m_cameraShake = 0.15f;
                m_cameraShakeTimer = 0.2f;
                break;

            case WeaponType::RIFLE:
                //m_player->AddCameraRecoil(0.03f, 0.015f);
                m_cameraShake = 0.05f;
                m_cameraShakeTimer = 0.12f;
                break;

            case WeaponType::SNIPER:
                //m_player->AddCameraRecoil(0.1f, 0.04f);
                m_cameraShake = 0.2f;
                m_cameraShakeTimer = 0.25f;
                break;
            }


            //  銃口位置を計算
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            DirectX::XMFLOAT3 muzzlePosition;
            muzzlePosition.x = playerPos.x + 0.45f;
            muzzlePosition.y = playerPos.y - 0.15f;
            muzzlePosition.z = playerPos.z + 0.8f;

            m_particleSystem->CreateMuzzleFlash(muzzlePosition, m_player->GetRotation(), currentWeapon);

            //  ショットガンは複数の弾を発射
            int pellets = (m_weaponSystem->GetCurrentWeapon() == WeaponType::SHOTGUN) ? 8 : 1;
            for (int p = 0; p < pellets; p++)
            {
                // 射撃方向を計算
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

                //  カメラ(視点)空の位置から玉を発射
                DirectX::XMFLOAT3 rayStart = playerPos;
                rayStart.y = 1.5f;  // 固定で地面から0.5m（目の高さ）

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
                    // 弾道を記録
                    BulletTrace trace;
                    trace.start = rayStart;
                    trace.end = rayResult.hitPoint;  // ← rayResult.hitPoint
                    trace.lifetime = 0.5f;
                    m_bulletTraces.push_back(trace);

                    // UserPointer から敵を取得
                    Enemy* hitEnemy = rayResult.hitEnemy;

                    if (hitEnemy != nullptr)
                    {
                        // === 敵にヒット！ ===

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
                            OutputDebugStringA("[BULLET] HEADSHOT!\n");

                            hitEnemy->headDestroyed = true;
                            hitEnemy->bloodDirection = shotDir;

                            // 血のエフェクト（後方）
                            m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, shotDir, 300);

                            // 血のエフェクト（上方）
                            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                            m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, upDir, 300);

                            // 放射状の血
                            for (int i = 0; i < 12; i++)
                            {
                                float angle = (i / 12.0f) * 6.28318f;
                                DirectX::XMFLOAT3 radialDir;
                                radialDir.x = cosf(angle);
                                radialDir.y = 0.3f + (rand() % 100) / 200.0f;
                                radialDir.z = sinf(angle);
                                m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, radialDir, 50);
                            }

                            // ランダム方向の血
                            for (int i = 0; i < 4; i++)
                            {
                                DirectX::XMFLOAT3 randomDir;
                                randomDir.x = ((rand() % 200) - 100) / 100.0f;
                                randomDir.y = ((rand() % 100) + 50) / 100.0f;
                                randomDir.z = ((rand() % 200) - 100) / 100.0f;
                                m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, randomDir, 50);
                            }

                            m_particleSystem->CreateExplosion(hitEnemy->headPosition);

                            // ダメージ
                            hitEnemy->health = 0;

                            // 吹っ飛ばす
                            hitEnemy->isRagdoll = true;
                            hitEnemy->velocity.x = shotDir.x * 15.0f;
                            hitEnemy->velocity.y = 8.0f;
                            hitEnemy->velocity.z = shotDir.z * 15.0f;

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

                            // ヒットストップ
                            m_timeScale = 0.0f;
                            m_hitStopTimer = 0.15f;

                            // スローモーション
                            m_timeScale = 0.15f;
                            m_slowMoTimer = 0.05f;

                            // カメラシェイク
                            m_cameraShake = 0.3f;
                            m_cameraShakeTimer = 0.5f;
                        }
                        else
                        {
                            // === ボディショット処理 ===
                            OutputDebugStringA("[BULLET] Body shot\n");

                            // === デバッグ: ダメージ前のHP ===
                            char debugHP1[256];
                            sprintf_s(debugHP1, "[DEBUG] Enemy ID:%d HP BEFORE damage: %df\n",
                                hitEnemy->id, hitEnemy->health);
                            OutputDebugStringA(debugHP1);

                            // === デバッグ: 武器のダメージ ===
                            char debugDamage[256];
                            sprintf_s(debugDamage, "[DEBUG] Weapon damage: %d (WeaponType:%d)\n",
                                weapon.damage, (int)currentWeapon);
                            OutputDebugStringA(debugDamage);

                            m_particleSystem->CreateBloodEffect(rayResult.hitPoint, shotDir, 15);
                            hitEnemy->health -= weapon.damage;

                            // === デバッグ: ダメージ後のHP ===
                            char debugHP2[256];
                            sprintf_s(debugHP2, "[DEBUG] Enemy ID:%d HP AFTER damage: %d\n",
                                hitEnemy->id, hitEnemy->health);
                            OutputDebugStringA(debugHP2);

                            //// ヒットストップ（軽め）
                            //m_timeScale = 0.1f;
                            //m_hitStopTimer = 0.03f;

                            // カメラシェイク（軽め）
                            m_cameraShake = 0.05f;
                            m_cameraShakeTimer = 0.1f;

                            // 死亡時の吹っ飛ばし
                            if (hitEnemy->health <= 0.0f)
                            {
                                // === デバッグ: ラグドール処理に入った ===
                                OutputDebugStringA("[DEBUG] Entering ragdoll processing (HP <= 0.0f)\n");

                                hitEnemy->isRagdoll = true;
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
                                OutputDebugStringA("[DEBUG] NOT entering ragdoll (HP > 0.0f)\n");
                            }
                        }

                        // ノックバック
                        float knockbackStrength = isHeadShot ? 0.5f : 0.2f;
                        hitEnemy->position.x += shotDir.x * knockbackStrength;
                        hitEnemy->position.z += shotDir.z * knockbackStrength;

                        // 死亡処理
                        if (hitEnemy->health <= 0)
                        {
                            // === デバッグ: 死亡判定に入った ===
                            OutputDebugStringA("[DEBUG] Entering death check (HP <= 0)\n");

                            if (!hitEnemy->isDying)
                            {
                                // === デバッグ: 死亡処理実行 ===
                                char debugDeath[256];
                                sprintf_s(debugDeath, "[DEBUG] Setting isDying=true, isAlive=false for enemy ID:%d\n",
                                    hitEnemy->id);
                                OutputDebugStringA(debugDeath);

                                hitEnemy->isDying = true;
                                hitEnemy->isAlive = false;
                                hitEnemy->currentAnimation = "Death";
                                hitEnemy->animationTime = 0.0f;
                                hitEnemy->corpseTimer = 5.0f;

                                // === 追加: 死んだ瞬間に物理ボディを削除 ===
                                RemoveEnemyPhysicsBody(hitEnemy->id);
                            }

                            else
                            {
                                // === デバッグ: すでにisDying=true ===
                                OutputDebugStringA("[DEBUG] Enemy already dying (isDying=true)\n");
                            }

                            // ポイント加算
                            int waveBonus = m_waveManager->OnEnemyKilled();
                            int totalPoints = (isHeadShot ? 150 : 60) + waveBonus;
                            m_player->AddPoints(totalPoints);

                            // ダメージ表示
                            m_showDamageDisplay = true;
                            m_damageDisplayTimer = 2.0f;
                            m_damageDisplayPos = hitEnemy->position;
                            m_damageDisplayPos.y += 2.0f;
                            m_damageValue = isHeadShot ? 150 : 60;

                            // ウィンドウタイトル更新
                            char debug[256];
                            sprintf_s(debug, isHeadShot ? "HEADSHOT!! Points:%d" : "Hit! Points:%d",
                                m_player->GetPoints());
                            SetWindowTextA(m_window, debug);
                        }
                        else
                        {
                            // === デバッグ: 死亡判定に入らなかった ===
                            OutputDebugStringA("[DEBUG] NOT entering death check (HP > 0)\n");
                        }
                    }
                    else
                    {
                        // 敵に当たらなかった（壁など）
                        OutputDebugStringA("[BULLET] Hit wall or object\n");
                    }
                }
                else
                {
                    // 何にも当たらなかった
                    OutputDebugStringA("[BULLET] Complete miss\n");
                    // === デバッグ: 死亡判定に入らなかった ===
                    OutputDebugStringA("[DEBUG] NOT entering death check (HP > 0)\n");
                }
            }
}
        m_lastMouseState = currentMouseState;
    
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

    // リロードタイマー更新
    if (m_weaponSystem->IsReloading())
    {
        float newTimer = m_weaponSystem->GetReloadTimer() - deltaTime;
        m_weaponSystem->SetReloadTimer(newTimer);
        if (newTimer <= 0.0f)
        {
            // リロード完了
            int needed = m_weaponSystem->GetMaxAmmo() - m_weaponSystem->GetCurrentAmmo();
            int reload = min(needed, m_weaponSystem->GetReserveAmmo());

            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() + reload);
            m_weaponSystem->SetReserveAmmo(m_weaponSystem->GetReserveAmmo() - reload);
            m_weaponSystem->SetReloading(false);
            m_weaponSystem->SaveAmmoStatus();
        }
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


    //  グローリーキル中は敵の更新を停止
    if (!m_gloryKillCameraActive)
    {
        //  m_cameraPOs
        //  【型】DirectX::XMFLOAT3
        //  【意味】プレイヤーの現在位置
        //  【用途】敵がプレイヤーに向かって動くため
        m_enemySystem->Update(deltaTime, m_player->GetPosition());
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
            // 死亡アニメーション中の処理
            enemy.corpseTimer -= deltaTime;

            if (enemy.corpseTimer <= 0.0f)
            {
                // 死体タイマーが切れた = 完全に削除
                enemy.isAlive = false;
                enemy.isDying = false;

                // 物理ボディを削除（まだ残ってたら）
                RemoveEnemyPhysicsBody(enemy.id);
                continue;
            }
        }

        //  ========================================================================
        //  アニメーション更新
        //  ========================================================================
        if (m_enemyModel && m_enemyModel->HasAnimation(enemy.currentAnimation))
        {
            float duration = m_enemyModel->GetAnimationDuration(enemy.currentAnimation);

            if (enemy.isDying)
            {
                //  死亡中: 終端で完全に止める
                enemy.animationTime += deltaTime * 0.1f;
                if (enemy.animationTime >= duration)
                {
                    enemy.animationTime = duration - 0.001f;  // 終端で固定
                }
            }
            else
            {
                //  通常: ループさせる
                enemy.animationTime += deltaTime * 0.1f;
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
            m_particleSystem->CreateBloodEffect(
                neckPosition,
                enemy.bloodDirection,  //   弾が飛んできた方向
                3
            );
        }
    }
    UpdatePhysics(deltaTime);

    //  死んだ敵を削除 毎フレーム敵を削除しないと、配列か肥大化
    m_enemySystem->ClearDeadEnemies();

    //  ウェーブ管理
    m_waveManager->Update(1.0f / 60.0f, m_player->GetPosition(), m_enemySystem.get());
  

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


    // === 接触ダメージ処理 ===
// 【目的】敵に触れられたらプレイヤーがダメージを受ける
// 【仕組み】全ての敵をチェックして、接触していたらHP減少
    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        // 生きていて、かつ「死んでいない（isDying == false）」敵だけ攻撃してくる
        if (enemy.isAlive && !enemy.isDying && enemy.touchingPlayer)
        {
            if (m_gloryKillInvincibleTimer <= 0)
            {
                bool died = m_player->TakeDamage(10);

                if (died)
                {
                    m_gameState = GameState::GAMEOVER;
                }

            }

            break;
        }
    }

}


void Game::UpdateGameOver()
{
    // ゲームオーバー画面：Rキーでリスタート
    if (GetAsyncKeyState('R') & 0x8000)
    {
        // ゲーム状態をリセット
        m_score = 0;
        for (int i = 0; i < 3; i++)
        {
            //m_cubesDestroyed[i] = false;
        }
        
        m_gameState = GameState::TITLE;
    }

    char gameOverText[256];
    sprintf_s(gameOverText, "GAME OVER - Final Score: %d - Press R to Restart", m_score);
    SetWindowTextA(m_window, gameOverText);
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
            OutputDebugStringA("[RENDER ERROR] TitleScene render failed: ");
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
        }
    }
    else
    {
        // TitleScene がない場合はエラーメッセージを表示
        OutputDebugStringA("[RENDER WARNING] TitleScene is null\n");
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

void Game::RenderGameOver()
{

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

    OutputDebugStringA("ImGui initialized successfully!\n");
}

void Game::ShutdownImGui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Game::UpdatePhysics(float deltaTime)
{
    if (m_dynamicsWorld)
    {
        // 物理シミュレーションを1ステップ進める
        // これにより、手動で動かしたKinematic ObjectのAABB(境界ボックス)も更新されます
        m_dynamicsWorld->stepSimulation(deltaTime, 10);
    }
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
            OutputDebugStringA("[GLORY KILL] GLORY KILL EXECUTED!\n");

            //  即死させる
            hitEnemy->health = 0;

            //  弾薬を50%回復
            WeaponType currentWeapon = m_weaponSystem->GetCurrentWeapon();
            WeaponData weaponData = m_weaponSystem->GetWeaponData(currentWeapon);

            int currentAmmo = m_weaponSystem->GetReserveAmmo();
            int maxReserve = weaponData.maxAmmo;

            int ammoReward = (int)(maxReserve * 0.5f);  //  50%回復
            int newAmmo = currentAmmo + ammoReward;

            //  最大値を超えないように
            if (newAmmo > maxReserve)
                newAmmo = maxReserve;

            m_weaponSystem->SetReserveAmmo(newAmmo);

            char buffer[128];
            sprintf_s(buffer, "[GLORY KILL] Ammo recovered: +%d (Total: %d/%d)\n",
                ammoReward, newAmmo, maxReserve);
            OutputDebugStringA(buffer);


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

                m_particleSystem->CreateBloodEffect(
                    hitEnemy->position,
                    bloodDir,
                    150
                );
            }
        }
        else
        {
            const float normalMeleeDamage = 25.0f;  // 小ダメージ

            int oldHealth = hitEnemy->health;
            hitEnemy->health -= (int)normalMeleeDamage;

            char buffer[128];
            sprintf_s(buffer, "[MELEE] Normal hit! Damage:%.0f HP:%d->%d\n",
                normalMeleeDamage, oldHealth, hitEnemy->health);
            OutputDebugStringA(buffer);

            // 軽いフィードバック
            m_cameraShake = 0.08f;
            m_cameraShakeTimer = 0.15f;

            m_hitStopTimer = 0.03f;

            // 少量のパーティクル
            if (m_particleSystem)
            {
                XMFLOAT3 bloodDir(0.0f, 1.0f, 0.0f);

                m_particleSystem->CreateBloodEffect(
                    hitEnemy->position,
                    bloodDir,
                    30  // 少量
                );
            }
        }
    }
    else
    {
        //  空振り
        OutputDebugStringA("[MELEE] Missed!\n");

        m_cameraShake = 0.05f;
        m_cameraShakeTimer = 0.1f;
    }

}
