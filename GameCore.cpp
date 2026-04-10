// ============================================================
//  GameCore.cpp
//  エンジンコア（初期化/メインループ/リソース作成/リセット）
//
//  【責務】
//  - コンストラクタ/デストラクタ（メンバ変数初期化/解放）
//  - Initialize(): ウィンドウハンドル受取 + 全システム初期化
//  - Tick(): メインループ（deltaTime計算 → Update → Render）
//  - Update(): GameState別にUpdate関数をディスパッチ
//  - Clear(): 毎フレームのバックバッファクリア
//  - CreateDevice/Resources: DirectX11デバイスとリソース作成
//  - CreateRenderResources: シェーダー/テクスチャ/エフェクト作成
//  - ResetGame(): ゲーム状態の完全リセット
//  - ヘルパー: シェーダーロード/ブラーリソース/フルスクリーン描画
//
//  【設計意図】
//  Gameクラスの「骨格」にあたる部分。
//  このファイルを見ればアプリケーションの起動→実行→終了の
//  ライフサイクルが把握できる。
//  他の7ファイルは全てここから呼び出される。
//
//  【呼び出し関係図】
//  main.cpp → Game::Initialize() → CreateDevice() → CreateResources()
//                                                   → CreateRenderResources()
//           → Game::Tick() [毎フレーム]
//               → Update() → UpdateTitle/Loading/Playing/GameOver/Ranking
//               → Render() → RenderTitle/Loading/Playing/GameOver/Ranking
// ============================================================

#include "Game.h"

using namespace DirectX;


// ============================================================
//  コンストラクタ - 全メンバ変数の初期化
//
//  【重要】ここでの初期化順序はヘッダー(.h)での宣言順と一致させる。
//  C++ではメンバ変数は宣言順に初期化される（初期化子リストの順序は無関係）。
//  ポインタは全てnullptrに、数値は全て0/falseに初期化。
// ============================================================
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
    m_gloryKillRange(5.0f),
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


// ============================================================
//  デストラクタ - ImGuiの破棄
//
//  【注意】unique_ptrで管理しているリソースは自動解放される。
//  明示的な解放が必要なのはImGuiとBullet Physicsのみ。
// ============================================================
Game::~Game()
{
    CleanupPhysics();
}



// ============================================================
//  Initialize - 全システムの初期化
//
//  【呼び出し順序が重要！】
//  1. ウィンドウハンドルとサイズを保存
//  2. CreateDevice() → DirectX11デバイス作成
//  3. CreateResources() → スワップチェーン等
//  4. CreateRenderResources() → シェーダー/テクスチャ
//  5. InitPhysics() → Bullet Physicsワールド
//  6. 各ゲームシステム初期化
//
//  【注意】InitPhysics()はCreateRenderResourcesの後に呼ぶこと。
//  逆にするとメッシュコライダーの元データが未準備でクラッシュする。
// ============================================================
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


// ============================================================
//  Tick - メインループ（毎フレーム1回呼ばれる）
//
//  【処理】
//  1. QueryPerformanceCounterでdeltaTimeを計算
//  2. FPSを計測（1秒ごとに更新）
//  3. Update() → ゲームロジック更新
//  4. Render() → 描画
// ============================================================
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


// ============================================================
//  Update - GameState別のUpdateディスパッチ
//
//  【役割】現在のGameStateに応じて適切なUpdate関数を呼ぶ
//  TITLE → UpdateTitle()
//  LOADING → UpdateLoading()
//  PLAYING → UpdatePlaying()
//  GAMEOVER → UpdateGameOver()
//  RANKING → UpdateRanking()
// ============================================================
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


// ============================================================
//  Clear - バックバッファと深度バッファのクリア
//
//  【背景色】ダークグレー（ゴシックな雰囲気に合わせる）
// ============================================================
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


// ============================================================
//  OnWindowSizeChanged - ウィンドウサイズ変更時の処理
// ============================================================
void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = (width > 1) ? width : 1;
    m_outputHeight = (height > 1) ? height : 1;
    CreateResources();
    m_uiSystem->OnScreenSizeChanged(m_outputWidth, m_outputHeight);
}



// ============================================================
//  CreateDevice - DirectX11デバイスとコンテキストの作成
//
//  【作成するもの】
//  - ID3D11Device（GPUリソースの作成に使用）
//  - ID3D11DeviceContext（描画コマンドの発行に使用）
//  - フィーチャーレベル確認（DX11_0以上を要求）
// ============================================================
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


// ============================================================
//  CreateResources - スワップチェーンとレンダーターゲットの作成
//
//  【作成するもの】
//  - スワップチェーン（ダブルバッファリング）
//  - レンダーターゲットビュー（描画先）
//  - 深度ステンシルバッファ
//  - ビューポート設定
//  - CommonStates（ブレンド/サンプラー/深度ステート）
// ============================================================
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
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE  // // SRVフラグ//
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


// ============================================================
//  CreateRenderResources - シェーダー/テクスチャ/エフェクトの作成
//
//  【これがゲーム固有のリソース初期化の本体】
//  CreateDevice/CreateResourcesはDirectX11の定型コード。
//  この関数がGothic Swarm固有のリソースを全て作成する。
//
//  【作成するもの】
//  - 敵モデル（Normal/Runner/Tank/MidBoss/Boss）のロード
//  - 武器/盾モデルのロード
//  - マップシステム初期化（FBX + メッシュコライダー）
//  - シェーダー（敵インスタンス用/ポストプロセス用）
//  - テクスチャ（UI/HUD/エフェクト用）
//  - パーティクルシステム初期化
//  - Effekseer初期化
//  - SpriteBatch/SpriteFont/BasicEffect
//  - 切断レンダリング初期化
//  ゲームが使う全リソースのロードが集約。
//  将来的にはResourceManager クラスに分離すべき。
// ============================================================
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

    DirectX::CreateWICTextureFromFile(
        m_d3dDevice.Get(),
        L"Assets/Texture/HUD/wave_banner_normal.png",
        nullptr,
        m_waveBannerNormalSRV.ReleaseAndGetAddressOf());

    DirectX::CreateWICTextureFromFile(
        m_d3dDevice.Get(),
        L"Assets/Texture/HUD/wave_banner_boss.png",
        nullptr,
        m_waveBannerBossSRV.ReleaseAndGetAddressOf());

    DirectX::CreateWICTextureFromFile(
        m_d3dDevice.Get(),
        L"Assets/Texture/HUD/score_blood_backdrop.png",
        nullptr,
        m_scoreBackdropSRV.ReleaseAndGetAddressOf());

    DirectX::CreateWICTextureFromFile(
        m_d3dDevice.Get(),
        L"Assets/Texture/HUD/claw_damage.png",
        nullptr,
        m_clawDamageSRV.ReleaseAndGetAddressOf());

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
            MAP_SCALE))
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
                MAP_SCALE
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

    //  === KeyPromptの初期化   ===
    m_keyPrompt = std::make_unique<KeyPrompt>();
    if (!m_keyPrompt->Initialize(m_d3dDevice.Get(), L"Assets/Texture/HUD/f_prompt.png"))
    {
        OutputDebugStringA("[KeyPrompt] Failed to initialize!\n");
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


// ============================================================
//  ResetGame - ゲーム状態の完全リセット
//
//  【呼び出しタイミング】
//  - タイトル画面からSPACEキーで開始時
//  - ゲームオーバーからRキーでリスタート時
//
//  【リセット対象】
//  - プレイヤーHP/位置/弾薬
//  - 敵の全削除 + 物理ボディ削除
//  - ウェーブカウンター
//  - スコア/コンボ/スタイルランク
//  - 全エフェクト/タイマー/フラグ
//  - ランキング保存フラグ
// ============================================================
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
        m_player->SetPosition(DirectX::XMFLOAT3(spawnX, groundY + Player::EYE_HEIGHT, spawnZ));

        char buf[256];
        sprintf_s(buf, "[SPAWN] Player at (%.2f, %.2f, %.2f) groundY=%.2f\n",
            spawnX, groundY + Player::EYE_HEIGHT, spawnZ, groundY);
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



// ============================================================
//  CreateBlurResources - ブラーポストプロセス用リソース作成
//
//  【作成するもの】
//  - ブラー用のレンダーターゲット（縮小バッファ）
//  - ガウシアンブラー用シェーダー
//  - 定数バッファ
//  【用途】ヒットストップ時/メニュー表示時の背景ぼかし
// ============================================================
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
    m_fullscreenVS = LoadVertexShader(L"Assets/Shaders/FullscreenQuadVS.cso");
    m_blurPS = LoadPixelShader(L"Assets/Shaders/GaussianBlur.cso");

    //OutputDebugStringA("[BLUR] Blur resources created successfully\n");
}

// =================================================================
// LoadVertexShader - 頂点シェーダーをコンパイル＆読み込み
// =================================================================

// ============================================================
//  LoadVertexShader - コンパイル済み頂点シェーダーの読み込み
//
//  【引数】.csoファイルのパス
//  【戻り値】ID3D11VertexShader*（失敗時はnullptr）
//  【注意】.csoはプリコンパイル済みシェーダー（CSO = Compiled Shader Object）
// ============================================================
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

// ============================================================
//  LoadPixelShader - コンパイル済みピクセルシェーダーの読み込み
// ============================================================
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


// ============================================================
//  DrawFullscreenQuad - フルスクリーン四角形の描画
//
//  【用途】ポストプロセスエフェクト（ブラー/ビネット等）の適用時に
//  画面全体を覆う四角形を描画し、ピクセルシェーダーを適用する
// ============================================================
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