// ============================================================
//  GameCore.cpp
//  �G���W���R�A�i������/���C�����[�v/���\�[�X�쐬/���Z�b�g�j
//
//  �y�Ӗ��z
//  - �R���X�g���N�^/�f�X�g���N�^�i�����o�ϐ�������/����j
//  - Initialize(): �E�B���h�E�n���h����� + �S�V�X�e��������
//  - Tick(): ���C�����[�v�ideltaTime�v�Z �� Update �� Render�j
//  - Update(): GameState�ʂ�Update�֐����f�B�X�p�b�`
//  - Clear(): ���t���[���̃o�b�N�o�b�t�@�N���A
//  - CreateDevice/Resources: DirectX11�f�o�C�X�ƃ��\�[�X�쐬
//  - CreateRenderResources: �V�F�[�_�[/�e�N�X�`��/�G�t�F�N�g�쐬
//  - ResetGame(): �Q�[����Ԃ̊��S���Z�b�g
//  - �w���p�[: �V�F�[�_�[���[�h/�u���[���\�[�X/�t���X�N���[���`��
//
//  �y�݌v�Ӑ}�z
//  Game�N���X�́u���i�v�ɂ����镔���B
//  ���̃t�@�C��������΃A�v���P�[�V�����̋N�������s���I����
//  ���C�t�T�C�N�����c���ł���B
//  ����7�t�@�C���͑S�Ă�������Ăяo�����B
//
//  �y�Ăяo���֌W�}�z
//  main.cpp �� Game::Initialize() �� CreateDevice() �� CreateResources()
//                                                   �� CreateRenderResources()
//           �� Game::Tick() [���t���[��]
//               �� Update() �� UpdateTitle/Loading/Playing/GameOver/Ranking
//               �� Render() �� RenderTitle/Loading/Playing/GameOver/Ranking
// ============================================================

#include "Game.h"

using namespace DirectX;


// ============================================================
//  �R���X�g���N�^ - �S�����o�ϐ��̏�����
//
//  �y�d�v�z�����ł̏����������̓w�b�_�[(.h)�ł̐錾���ƈ�v������B
//  C++�ł̓����o�ϐ��͐錾���ɏ����������i�������q���X�g�̏����͖��֌W�j�B
//  �|�C���^�͑S��nullptr�ɁA���l�͑S��0/false�ɏ������B
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
    //   �p�t�H�[�}���X�J�E���^�[�̏�����
    QueryPerformanceFrequency(&m_performanceFrequency);
    QueryPerformanceCounter(&m_lastFrameTime);
    m_gibs.reserve(200);
    m_instWorlds.reserve(100);
    m_instColors.reserve(100);
    m_instAnims.reserve(100);
    m_instTimes.reserve(100);
    // ���C���X�^���V���O�p�o�b�t�@�\��
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
//  �f�X�g���N�^ - ImGui�̔j��
//
//  �y���Ӂzunique_ptr�ŊǗ����Ă��郊�\�[�X�͎�����������B
//  �����I�ȉ�����K�v�Ȃ̂�ImGui��Bullet Physics�̂݁B
// ============================================================
Game::~Game()
{
    CleanupPhysics();
}



// ============================================================
//  Initialize - �S�V�X�e���̏�����
//
//  �y�Ăяo���������d�v�I�z
//  1. �E�B���h�E�n���h���ƃT�C�Y��ۑ�
//  2. CreateDevice() �� DirectX11�f�o�C�X�쐬
//  3. CreateResources() �� �X���b�v�`�F�[����
//  4. CreateRenderResources() �� �V�F�[�_�[/�e�N�X�`��
//  5. InitPhysics() �� Bullet Physics���[���h
//  6. �e�Q�[���V�X�e��������
//
//  �y���ӁzInitPhysics()��CreateRenderResources�̌�ɌĂԂ��ƁB
//  �t�ɂ���ƃ��b�V���R���C�_�[�̌��f�[�^���������ŃN���b�V������B
// ============================================================
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = (width > 1) ? width : 1;
    m_outputHeight = (height > 1) ? height : 1;


    CreateDevice();
    CreateResources();

    //  === Bullet Physics ������  ===
    InitPhysics();

    CreateRenderResources();  // 3D�`��p�̏�����

    m_states = std::make_unique<DirectX::CommonStates>(m_d3dDevice.Get());



    // === ���p�[�e�B�N���p�\�t�g�~�e�N�X�`���𐶐� ===
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

                // ���S�����A�[�Ɍ������ē����ɂȂ�~
                if (dist > 0.5f) { texData[y * texSize + x] = 0; continue; }  // �O�����o�b�T��
                float alpha = 1.0f - (dist / 0.5f);  // 0?0.5��0?1�Ɉ����L�΂�
                alpha = alpha * alpha * alpha;  // �O��ŋ}���t�F�[�h

                uint8_t a = (uint8_t)(alpha * 255.0f);
                // RGBA: ��(255,255,255) �~ alpha
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

    // === �p�[�e�B�N���p�G�t�F�N�g�i�e�N�X�`���t���r���{�[�h�p�j===
    m_particleEffect = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_particleEffect->SetVertexColorEnabled(true);
    m_particleEffect->SetTextureEnabled(true);
    m_particleEffect->SetTexture(m_bloodParticleSRV.Get());

    // ���̓��C�A�E�g�쐬
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



    // Initialize �ŏ�����
    m_titleScene = std::make_unique<TitleScene>();
    m_titleScene->Initialize(m_d3dDevice.Get(),
        m_outputWidth, m_outputHeight);

}


// ============================================================
//  Tick - ���C�����[�v�i���t���[��1��Ă΂��j
//
//  �y�����z
//  1. QueryPerformanceCounter��deltaTime���v�Z
//  2. FPS���v���i1�b���ƂɍX�V�j
//  3. Update() �� �Q�[�����W�b�N�X�V
//  4. Render() �� �`��
// ============================================================
void Game::Tick()
{
    //  === deltaTime���v�Z    ===
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    float elapsedSeconds = static_cast<float>(currentTime.QuadPart - m_lastFrameTime.QuadPart)
        / static_cast<float>(m_performanceFrequency.QuadPart);

    m_lastFrameTime = currentTime;

    //  deltaTime���X�V(�ő�l�𐧌����āA�X�p�C�N��h��)
    m_deltaTime = min(elapsedSeconds, 0.1f);    //  �ő�0.1�b

    // === �f�o�b�O: deltaTime��FPS��\�� ===
    static float debugTimer = 0.0f;
    debugTimer += m_deltaTime;
    if (debugTimer >= 1.0f)  // 1�b���Ƃɕ\��
    {
        float currentFPS = 1.0f / m_deltaTime;
        char debugBuffer[256];
        sprintf_s(debugBuffer, "[DEBUG] deltaTime: %.4f, FPS: %.1f\n", m_deltaTime, currentFPS);
        ////OutputDebugStringA(debugBuffer);
        debugTimer = 0.0f;
    }

    // �Q�[����1�t���[������
    Update();  // �Q�[�����W�b�N
    Render();  // �`�揈��
}


// ============================================================
//  Update - GameState�ʂ�Update�f�B�X�p�b�`
//
//  �y�����z���݂�GameState�ɉ����ēK�؂�Update�֐����Ă�
//  TITLE �� UpdateTitle()
//  LOADING �� UpdateLoading()
//  PLAYING �� UpdatePlaying()
//  GAMEOVER �� UpdateGameOver()
//  RANKING �� UpdateRanking()
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
//  Clear - �o�b�N�o�b�t�@�Ɛ[�x�o�b�t�@�̃N���A
//
//  �y�w�i�F�z�_�[�N�O���[�i�S�V�b�N�ȕ��͋C�ɍ��킹��j
// ============================================================
void Game::Clear()
{
    // ��ʂ�ŃN���A
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Black);

    // //�d�v: �[�x�o�b�t�@(DEPTH) �� �X�e���V���o�b�t�@(STENCIL) �̗������N���A����
    m_d3dContext->ClearDepthStencilView(
        m_depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // �����̃t���O�𗧂Ă�
        1.0f, // Depth�����l (1.0 = �ŉ�)
        0     // Stencil�����l (0 = �����Ȃ�)
    );

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
        m_depthStencilView.Get());

    CD3D11_VIEWPORT viewport(0.0f, 0.0f,
        static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}


// ============================================================
//  OnWindowSizeChanged - �E�B���h�E�T�C�Y�ύX���̏���
// ============================================================
void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = (width > 1) ? width : 1;
    m_outputHeight = (height > 1) ? height : 1;
    CreateResources();
    m_uiSystem->OnScreenSizeChanged(m_outputWidth, m_outputHeight);
}



// ============================================================
//  CreateDevice - DirectX11�f�o�C�X�ƃR���e�L�X�g�̍쐬
//
//  �y�쐬������́z
//  - ID3D11Device�iGPU���\�[�X�̍쐬�Ɏg�p�j
//  - ID3D11DeviceContext�i�`��R�}���h�̔��s�Ɏg�p�j
//  - �t�B�[�`���[���x���m�F�iDX11_0�ȏ��v���j
// ============================================================
void Game::CreateDevice()
{
    UINT creationFlags = 0;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;  // �f�o�b�O���L��
#endif

    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,  // DirectX 11.1
        D3D_FEATURE_LEVEL_11_0,  // DirectX 11.0
        D3D_FEATURE_LEVEL_10_1,  // DirectX 10.1�i�t�H�[���o�b�N�j
        D3D_FEATURE_LEVEL_10_0,  // DirectX 10.0�i�t�H�[���o�b�N�j
    };

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // �f�t�H���g�A�_�v�^�[
        D3D_DRIVER_TYPE_HARDWARE,   // �n�[�h�E�F�A�iGPU�j�g�p
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
//  CreateResources - �X���b�v�`�F�[���ƃ����_�[�^�[�Q�b�g�̍쐬
//
//  �y�쐬������́z
//  - �X���b�v�`�F�[���i�_�u���o�b�t�@�����O�j
//  - �����_�[�^�[�Q�b�g�r���[�i�`���j
//  - �[�x�X�e���V���o�b�t�@
//  - �r���[�|�[�g�ݒ�
//  - CommonStates�i�u�����h/�T���v���[/�[�x�X�e�[�g�j
// ============================================================
void Game::CreateResources()
{
    // ���\�[�X�N���A
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(1, nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(m_outputHeight);

    if (m_swapChain)
    {
        // �X���b�v�`�F�[�����T�C�Y
        HRESULT hr = m_swapChain->ResizeBuffers(2, backBufferWidth, backBufferHeight,
            DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        if (FAILED(hr)) return;
    }
    else
    {
        // �X���b�v�`�F�[���쐬
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

        // === �K�E�V�A���u���[�p���\�[�X�쐬 ===
        CreateBlurResources();

        // [PERF] //OutputDebugStringA("[GAME] All resources created successfully\n");
    }

    // �����_�[�^�[�Q�b�g�쐬
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr)) throw std::runtime_error("GetBuffer failed");

    hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr,
        m_renderTargetView.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateRenderTargetView failed");

    // �[�x�o�b�t�@�쐬
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_R24G8_TYPELESS,  // // D24_UNORM_S8_UINT �� TYPELESS�ɕύX
        backBufferWidth,
        backBufferHeight,
        1,  // �z��T�C�Y
        1,  // �~�b�v���x��
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE  // // SRV�t���O//
    );

    hr = m_d3dDevice->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        m_depthTexture.ReleaseAndGetAddressOf()  // // �����o�[�ϐ��ɕۑ�
    );
    if (FAILED(hr)) throw std::runtime_error("CreateTexture2D failed");

    // �[�x�X�e���V���r���[�iDSV�j�̍쐬
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
        D3D11_DSV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_D24_UNORM_S8_UINT  // DSV�p�̃t�H�[�}�b�g
    );

    hr = m_d3dDevice->CreateDepthStencilView(
        m_depthTexture.Get(),
        &depthStencilViewDesc,
        m_depthStencilView.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) throw std::runtime_error("CreateDepthStencilView failed");

    // �[�x�V�F�[�_�[���\�[�X�r���[�iSRV�j�̍쐬
    CD3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc(
        D3D11_SRV_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS  // SRV�p�̃t�H�[�}�b�g
    );

    hr = m_d3dDevice->CreateShaderResourceView(
        m_depthTexture.Get(),
        &depthSRVDesc,
        m_depthSRV.ReleaseAndGetAddressOf()
    );
    if (FAILED(hr)) throw std::runtime_error("CreateShaderResourceView (Depth) failed");

    // �f�o�b�O���O
    char debugDepth[256];
    sprintf_s(debugDepth, "[DEPTH] SRV created: %p\n", m_depthSRV.Get());
    // [PERF] //OutputDebugStringA(debugDepth);
}


// ============================================================
//  CreateRenderResources - �V�F�[�_�[/�e�N�X�`��/�G�t�F�N�g�̍쐬
//
//  �y���ꂪ�Q�[���ŗL�̃��\�[�X�������̖{�́z
//  CreateDevice/CreateResources��DirectX11�̒�^�R�[�h�B
//  ���̊֐���Gothic Swarm�ŗL�̃��\�[�X��S�č쐬����B
//
//  �y�쐬������́z
//  - �G���f���iNormal/Runner/Tank/MidBoss/Boss�j�̃��[�h
//  - ����/�����f���̃��[�h
//  - �}�b�v�V�X�e���������iFBX + ���b�V���R���C�_�[�j
//  - �V�F�[�_�[�i�G�C���X�^���X�p/�|�X�g�v���Z�X�p�j
//  - �e�N�X�`���iUI/HUD/�G�t�F�N�g�p�j
//  - �p�[�e�B�N���V�X�e��������
//  - Effekseer������
//  - SpriteBatch/SpriteFont/BasicEffect
//  - �ؒf�����_�����O������
//  �Q�[�����g���S���\�[�X�̃��[�h���W��B
//  �����I�ɂ�ResourceManager �N���X�ɕ������ׂ��B
// ============================================================
void Game::CreateRenderResources()
{
    // DirectXTK�̊�{�I�u�W�F�N�g���쐬
    m_states = std::make_unique<DirectX::CommonStates>(m_d3dDevice.Get());
    m_effect = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());

    // �G�t�F�N�g�̐ݒ�
    m_effect->SetVertexColorEnabled(true);  // ���_�J���[���g�p

    // ���̓��C�A�E�g�̍쐬�iGPU�p�̐ݒ�j
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

    // �O���[���[�L���p�̘r�ƃi�C�t�i�v���~�e�B�u�j
    m_gloryKillArm = DirectX::GeometricPrimitive::CreateCylinder(m_d3dContext.Get(), 0.6f, 0.05f);  // �r�i����0.6�A����0.05�j
    m_gloryKillKnife = DirectX::GeometricPrimitive::CreateCone(m_d3dContext.Get(), 0.2f, 0.03f);   // �i�C�t�i����0.2�A����0.03�j

    m_weaponModel = std::make_unique<Model>();
    if (!m_weaponModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/SHOTGUN.fbx"))
    {
        OutputDebugStringA("Failed to load SHOTGUN model\n");
    }
    else
    {
        // �e�N�X�`�����蓮�œǂݍ���
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

    //  �Ǖ���p���f���ǂݍ���
    m_pistolModel = std::make_unique<Model>();
    if (!m_pistolModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/PISTOL.fbx"))
    {
        ////OutputDebugStringA("Failed to load M1911 model for wall weapon\n");
    }


    // �t�H���g������
    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_d3dContext.Get());
    m_font = std::make_unique<DirectX::SpriteFont>(m_d3dDevice.Get(), L"Assets/Fonts/segoeUI.spritefont");
    m_fontLarge = std::make_unique<DirectX::SpriteFont>(m_d3dDevice.Get(), L"Assets/Fonts/segoeUI_large.spritefont");

    //  �����N�e�N�X�`���ǂݍ���
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
    // �V�[���hHUD�e�N�X�`���ǂݍ���
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

        // �ߐڃA�C�R���i�Ȃ��Ă�HUD�͓����j
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

        // === �ėp���s�N�Z���e�N�X�`���쐬 ===
        {
            uint32_t whiteData = 0xFFFFFFFF;  // RGBA�S��255 = ��
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

      ////  �v���C���[���f���t�@�C����ǂݍ���
      //if (!m_testModel->LoadFromFile(m_d3dDevice.Get(), "Assets/X_Bot.fbx"))
      //{
      //    //OutputDebugStringA("Game::CreateRenderResources - Failed to load model!\n");
      //}
      //else
      //{
      //    //OutputDebugStringA("Game::CreateRenderResources - Model loaded successfully!\n");
      //}

      //  === �GNORMAL���f��   ===
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

    //  === �GRUNNER���f��  ===
    m_runnerModel = std::make_unique<Model>();
    if (!m_runnerModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Runner/Runner.fbx"))
    {
        ////OutputDebugStringA("Failed to load Runner model!\n");
    }
    else
    {
        ////OutputDebugStringA("Runner model loaded successfully!\n");
    }

    //  === �GTANK   ===
    m_tankModel = std::make_unique<Model>();
    if (!m_tankModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Tank/Tank.fbx"))
    {
        ////OutputDebugStringA("Failed to load Tank model");
    }
    else
    {
        ////OutputDebugStringA("Tank model loaded successfully!\n");
    }

    //  �A�[�����f��
    m_gloryKillArmModel = std::make_unique<Model>();
    if (!m_gloryKillArmModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/GloryKill/fps_armsKnife.fbx"))
    {
        ////OutputDebugStringA("Failed to load FPS arms model!\n");
    }
    else
    {
        ////OutputDebugStringA("FPS arms model loaded!\n");

        m_gloryKillArmModel->PrintBoneNames();



        // �e�N�X�`����ǂݍ���œK�p
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


    // �X�R�A�|�b�v�A�b�v�p�e�N�X�`��
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
    //  === NORMAL���f���̃A�j���[�V����   ===
    //  ====================================
    {
        ////OutputDebugStringA("=== Loading Y_Bot animations  ===\n");

        //  --- ���s�A�j���[�V����   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Walk.fbx", "Walk"))
        {
            ////OutputDebugStringA("Failed to load Walk animation\n");
        }

        //  --- �ҋ@�A�j���[�V����   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Idle.fbx", "Idle"))
        {
            ////OutputDebugStringA("Failed to load Idle animation\n");
        }

        //  --- ����A�j���[�V����   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Run.fbx", "Run"))
        {
            ////OutputDebugStringA("Failed to load Run animation\n");
        }

        //  --- �U���A�j���[�V����   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Attack.fbx", "Attack"))
        {
            ////OutputDebugStringA("Failed to load Attack animation");
        }

        //  --- ���S�A�j���[�V����   ---
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Death.fbx", "Death"))
        {
            ////OutputDebugStringA("Failed to load Death Animation");
        }
        //  �X�^���A�j���[�V����
        if (!m_enemyModel->LoadAnimation("Assets/Models/Normal/Normal_Stun.fbx", "Stun"))
        {

        }
    }

    //  ===================================
    //  === RUNNER���f���A�j���[�V����    ===
    //  ===================================
    {
        //  === RUNNER�A�j���[�V�����ǂݍ���   ===
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
    //  === TANK�A�j���[�V���� ===
    //  =========================
    {
        //  === Tank �A�j���[�V�����ǂݍ��� ===
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
        // MIDBOSS ���f���ǂݍ���
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

    // === BOSS���f���ǂݍ��� ===
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

    // === �����f���ǂݍ��� ===
    m_shieldModel = std::make_unique<Model>();
    if (!m_shieldModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Shield/02_Shield.fbx"))
    {
        //OutputDebugStringA("[SHIELD] Failed to load shield model!\n");
    }
    else
    {
        //OutputDebugStringA("[SHIELD] Shield model loaded successfully!\n");

        // �e�N�X�`���ǂݍ��݁iBaseColor�j
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
        // �[���r�l�b�g�e�N�X�`���ǂݍ���
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_d3dDevice.Get(),                        // �f�o�C�X
            L"Assets/Texture/low_health_vignette.png", // �t�@�C���p�X
            nullptr,                                   // �e�N�X�`�����\�[�X�i�s�v�j
            m_lowHealthVignetteSRV.GetAddressOf()      // SRV�i�V�F�[�_�[�ɓn���p�j
        );
        if (SUCCEEDED(hr))
            OutputDebugStringA("[Game] Low health vignette texture loaded!\n");
        else
            OutputDebugStringA("[Game] WARNING: low_health_vignette.png not found!\n");
    }

    // �_�b�V���p�X�s�[�h���C���e�N�X�`��
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

    //  === MapSystem   ������ ===
    m_mapSystem = std::make_unique<MapSystem>();
    if (!m_mapSystem->Initialize(m_d3dContext.Get(), m_d3dDevice.Get()))
    {
        //OutputDebugStringA("Game::CreateRenderResources - Failed to initialize MapSystem\n");
    }
    else
    {
        m_mapSystem->CreateDefaultMap();

        //  FBX�}�b�v�ǂݍ���
        if (m_mapSystem->LoadMapFBX(
            "Assets/Models/Map/Market AL_DANUBE.fbx",
            L"Assets/Models/Map/textures",
            MAP_SCALE))
        {
            OutputDebugStringA("FBX Map loaded!\n");

            // === FBX���b�V���R���C�_�[�쐬 ===
            const auto& tris = m_mapSystem->GetCollisionTriangles();
            if (!tris.empty())
            {
                //  btTriangleMesh �ɎO�p�`��o�^
                m_mapTriMesh = new btTriangleMesh();
                for (const auto& tri : tris)
                {
                    btVector3 v0(tri.v0.x, tri.v0.y, tri.v0.z);
                    btVector3 v1(tri.v1.x, tri.v1.y, tri.v1.z);
                    btVector3 v2(tri.v2.x, tri.v2.y, tri.v2.z);
                    m_mapTriMesh->addTriangle(v0, v1, v2);
                }

                //  BVH���b�V���`����쐬�i�ÓI�I�u�W�F�N�g��p�j
                m_mapMeshShape = new btBvhTriangleMeshShape(m_mapTriMesh, true);

                //  �ÓI���̂Ƃ��ēo�^�imass=0 = �����Ȃ��j
                btTransform meshTransform;
                meshTransform.setIdentity();

                btDefaultMotionState* ms = new btDefaultMotionState(meshTransform);
                btRigidBody::btRigidBodyConstructionInfo info(
                    0.0f, ms, m_mapMeshShape);
                m_mapMeshBody = new btRigidBody(info);

                // �}�b�v�̍��̂ɂ�userPointer��null�ɂ��Ă����i�G�Ƌ�ʁj
                m_mapMeshBody->setUserPointer(nullptr);

                m_dynamicsWorld->addRigidBody(m_mapMeshBody);

                char buf[256];
                sprintf_s(buf, "[PHYSICS] Mesh collider created: %zu triangles\n",
                    tris.size());
                OutputDebugStringA(buf);
            }

            // === �i�r�Q�[�V�����O���b�h�\�z ===
            BuildNavGrid();

            TestMeshSlice();

            //  �����v���~�e�B�u���`�悷��i���E�ǂ��c���j
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

    //  === WeaponSystem ������    2025/11/14  ===
    m_weaponSpawnSystem = std::make_unique<WeaponSpawnSystem>();
    m_weaponSpawnSystem->CreateDefaultSpawns();

    //  --- �e�N�X�`���ǂݍ���   2025/11/19  ---
    if (!m_weaponSpawnSystem->InitializeTextures(m_d3dDevice.Get(), m_d3dContext.Get()))
    {
        //OutputDebugStringA("Game::CreateRenderResources - Failed to initialize weapon textures\n");
    }

    m_nearbyWeaponSpawn = nullptr;

    //OutputDebugStringA("Game::CreateRenderResources - WeaponSpawnSystem initialized\n");

    //  === Shadow ��������  ===
    m_shadow = std::make_unique<Shadow>();
    if (!m_shadow->Initialize(m_d3dDevice.Get()))
    {
        //OutputDebugStringA("Game::CreateRenderResources - Failed to initialize Shadow System!\n");
        // �G���[����
    }
    else
    {
        // �e�̐F�����̔������ɐݒ�
        m_shadow->SetShadowColor(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f));
    }

    //  === StunRing�̏�����    ===
    m_stunRing = std::make_unique<StunRing>();
    if (!m_stunRing->Initialize(m_d3dDevice.Get()))
    {
        OutputDebugStringA("[StunRing] Failed to initialize!\n");
    }

    //  === KeyPrompt�̏�����   ===
    m_keyPrompt = std::make_unique<KeyPrompt>();
    if (!m_keyPrompt->Initialize(m_d3dDevice.Get(), L"Assets/Texture/HUD/f_prompt.png"))
    {
        OutputDebugStringA("[KeyPrompt] Failed to initialize!\n");
    }

    //  === TargetMarker�̏�����    ===
    m_targetMarker = std::make_unique<TargetMarker>();
    if (!m_targetMarker->Initialize(m_d3dDevice.Get()))
    {
        OutputDebugStringA("[TargetMarker] Failed to initialze");
    }

    //  === Effekseer�̏�����   ===

    // �`��Ǘ��N���X(�����_���[)�̍쐬
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

    // �C���X�^���V���O�p�o�b�t�@�쐬
    m_enemyModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_runnerModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_tankModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_midBossModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);
    m_bossModel->CreateInstanceBuffers(m_d3dDevice.Get(), 100);

    // �G�b�t�F�N�g�Ǘ��N���X
    m_effekseerManager = Effekseer::Manager::Create(8000);
    //  DirectX�͍�����W�Ȃ̂�Effekseer�ɋ�����
    m_effekseerManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

    // �`��ݒ�̕R�Â�
    m_effekseerManager->SetSpriteRenderer(m_effekseerRenderer->CreateSpriteRenderer());
    m_effekseerManager->SetRibbonRenderer(m_effekseerRenderer->CreateRibbonRenderer());
    m_effekseerManager->SetRingRenderer(m_effekseerRenderer->CreateRingRenderer());
    m_effekseerManager->SetTrackRenderer(m_effekseerRenderer->CreateTrackRenderer());
    m_effekseerManager->SetModelRenderer(m_effekseerRenderer->CreateModelRenderer());
    m_effekseerManager->SetTextureLoader(m_effekseerRenderer->CreateTextureLoader());
    m_effekseerManager->SetModelLoader(m_effekseerRenderer->CreateModelLoader());
    m_effekseerManager->SetMaterialLoader(m_effekseerRenderer->CreateMaterialLoader());

    // ���g���C���G�t�F�N�g�̓ǂݍ���
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


    // === �{�X�G�t�F�N�g�ǂݍ��� ===
    m_effectSlashRed = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/SlashRed.efkefc");
    // === �{�X�G�t�F�N�g�ǂݍ��� ===
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

    // �e���˃G�t�F�N�g
    m_effectGunFire = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/GunFire.efkefc");

    // �e�ۃq�b�g�G�t�F�N�g
    m_effectAttackImpact = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/AttackImpact.efkefc");

    // �{�X�X�|�[���G�t�F�N�g
    m_effectBossSpawn = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/BossSpawn.efkefc");

    // �G���X�|�[���G�t�F�N�g
    m_effectEnemySpawn = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/SpawnEnemy.efkefc");

    // �p���B�G�t�F�N�g
    m_effectParry = Effekseer::Effect::Create(
        m_effekseerManager, u"Assets/Effects/Parry.efkefc");


    InitSliceRendering();

    //  === Imgui   ===
    InitImGui();


}


// ============================================================
//  ResetGame - �Q�[����Ԃ̊��S���Z�b�g
//
//  �y�Ăяo���^�C�~���O�z
//  - �^�C�g����ʂ���SPACE�L�[�ŊJ�n��
//  - �Q�[���I�[�o�[����R�L�[�Ń��X�^�[�g��
//
//  �y���Z�b�g�Ώہz
//  - �v���C���[HP/�ʒu/�e��
//  - �G�̑S�폜 + �����{�f�B�폜
//  - �E�F�[�u�J�E���^�[
//  - �X�R�A/�R���{/�X�^�C�������N
//  - �S�G�t�F�N�g/�^�C�}�[/�t���O
//  - �����L���O�ۑ��t���O
// ============================================================
void Game::ResetGame()
{
    m_showTutorial = true;

    // === �v���C���[ ===
    m_player->SetHealth(100);
    m_healthPickups.clear();     //  �A�C�e�������Z�b�g
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
    m_player->SetRotation(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));   // ���������ʂɃ��Z�b�g
    m_player->GetPointsRef() = 0;  // �����|�C���g
    m_scoreDisplayValue = 0.0f;  // �����|�C���g�ɍ��킹��
    m_scoreFlashTimer = 0.0f;
    m_scoreShakeTimer = 0.0f;
    m_lastDisplayedScore = 0;

    // === �G��S���� ===
    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        RemoveEnemyPhysicsBody(enemy.id);
    }
    m_enemySystem->GetEnemies().clear();
    m_enemiesInitialized = false;

    // === �E�F�[�u ===
    m_waveManager->Reset();
    m_waveManager->SetPaused(false);

    // === ���� ===
    m_weaponSystem = std::make_unique<WeaponSystem>();

    // === �X�R�A�E�X�^�C�� ===
    m_score = 0;
    m_styleRank->Reset();

    //  �X�^�b�c���Z�b�g
    m_statKills = 0;
    m_statHeadshots = 0;
    m_statMeleeKills = 0;
    m_statMaxCombo = 0;
    m_statDamageDealt = 0;
    m_statDamageTaken = 0;
    m_statMaxStyleRank = 0;
    m_statSurvivalTime = 0.0f;
    m_parrySuccessCount = 0;

    // === ��ԃ��Z�b�g ===
    m_shieldState = ShieldState::Idle;
    m_shieldBashTimer = 0.0f;
    m_cameraShake = 0.0f;
    m_cameraShakeTimer = 0.0f;
    m_gloryKillActive = false;
    m_gloryKillCameraActive = false;
    m_gloryKillDOFActive = false;
    m_gloryKillTargetEnemy = nullptr;
    m_timeScale = 1.0f;

    // === �t�F�[�h ===
    m_fadeAlpha = 1.0f;
    m_fadingIn = true;
    m_fadeActive = true;

    // === �r�[���G�t�F�N�g��~ ===
    if (m_beamHandle >= 0 && m_effekseerManager != nullptr)
    {
        m_effekseerManager->StopEffect(m_beamHandle);
        m_beamHandle = -1;
    }

    // === Effekseer�S�G�t�F�N�g��~ ===
    if (m_effekseerManager != nullptr)
    {
        m_effekseerManager->StopAllEffects();
    }

    // === ��HP ===
    m_shieldHP = 100.0f;
    m_shieldDisplayHP = 100.0f;
    m_shieldGlowIntensity = 0.0f;
    m_chargeEnergy = 0.0f;
    m_chargeReady = false;

    // === �p�[�e�B�N�� ===
    if (m_particleSystem)
        //m_particleSystem->Clear();

    // === �e���g���[�X ===
        m_bulletTraces.clear();

    // === �X���[���[�V���� ===
    m_timeScale = 1.0f;
    m_slowMoTimer = 0.0f;

    m_meleeCharges = m_meleeMaxCharges;
    m_meleeRechargeTimer = 0.0f;

    // === �^�C�}�[�� ===
    m_typeWalkTimer[0] = m_typeWalkTimer[1] = m_typeWalkTimer[2] = m_typeWalkTimer[3] = m_typeWalkTimer[4] = 0.0f;
    m_typeAttackTimer[0] = m_typeAttackTimer[1] = m_typeAttackTimer[2] = m_typeAttackTimer[3] = m_typeAttackTimer[4] = 0.0f;

    //OutputDebugStringA("[GAME] === GAME RESET COMPLETE ===\n");
}



// ============================================================
//  CreateBlurResources - �u���[�|�X�g�v���Z�X�p���\�[�X�쐬
//
//  �y�쐬������́z
//  - �u���[�p�̃����_�[�^�[�Q�b�g�i�k���o�b�t�@�j
//  - �K�E�V�A���u���[�p�V�F�[�_�[
//  - �萔�o�b�t�@
//  �y�p�r�z�q�b�g�X�g�b�v��/���j���[�\�����̔w�i�ڂ���
// ============================================================
void Game::CreateBlurResources()
{
    // === �I�t�X�N���[�������_�[�^�[�Q�b�g�쐬 ===
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

    //  ���̃����_�����O�p�V�[���R�s�[�e�N�X�`��
    {
        D3D11_TEXTURE2D_DESC copyDesc = texDesc;  // offscreen�Ɠ����ݒ���R�s�[
        copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;  // SRV������OK(RTV�͕s�v)
        hr = m_d3dDevice->CreateTexture2D(&copyDesc, nullptr, m_sceneCopyTex.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("Failed to create scene copy texture");

        hr = m_d3dDevice->CreateShaderResourceView(m_sceneCopyTex.Get(), nullptr, m_sceneCopySRV.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("Failed to create scene copy SRV");

        OutputDebugStringA("[Fluid] Scene copy texture created\n");
    }

    // === �萔�o�b�t�@�쐬 ===
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(BlurParams);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_blurConstantBuffer.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to create blur constant buffer");

    // === �T���v���[�쐬 ===
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

    // �I�t�X�N���[���p�[�x�o�b�t�@�쐬
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

    // �I�t�X�N���[���p�[�x�X�e���V���r���[�쐬
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

    // �I�t�X�N���[���p�[�xSRV�쐬
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

    // === �V�F�[�_�[�ǂݍ��� ===
    m_fullscreenVS = LoadVertexShader(L"Assets/Shaders/FullscreenQuadVS.cso");
    m_blurPS = LoadPixelShader(L"Assets/Shaders/GaussianBlur.cso");

    //OutputDebugStringA("[BLUR] Blur resources created successfully\n");
}

// =================================================================
// LoadVertexShader - ���_�V�F�[�_�[���R���p�C�����ǂݍ���
// =================================================================

// ============================================================
//  LoadVertexShader - �R���p�C���ςݒ��_�V�F�[�_�[�̓ǂݍ���
//
//  �y�����z.cso�t�@�C���̃p�X
//  �y�߂�l�zID3D11VertexShader*�i���s����nullptr�j
//  �y���Ӂz.cso�̓v���R���p�C���ς݃V�F�[�_�[�iCSO = Compiled Shader Object�j
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
// LoadPixelShader - �s�N�Z���V�F�[�_�[���R���p�C�����ǂݍ���
// =================================================================

// ============================================================
//  LoadPixelShader - �R���p�C���ς݃s�N�Z���V�F�[�_�[�̓ǂݍ���
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
//  DrawFullscreenQuad - �t���X�N���[���l�p�`�̕`��
//
//  �y�p�r�z�|�X�g�v���Z�X�G�t�F�N�g�i�u���[/�r�l�b�g���j�̓K�p����
//  ��ʑS�̂𕢂��l�p�`��`�悵�A�s�N�Z���V�F�[�_�[��K�p����
// ============================================================
void Game::DrawFullscreenQuad()
{
    // ���_�o�b�t�@�s�v�i���_�V�F�[�_�[�Ŏ��������j
    m_d3dContext->IASetInputLayout(nullptr);
    m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ���_�V�F�[�_�[�ƃs�N�Z���V�F�[�_�[��ݒ�
    m_d3dContext->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
    m_d3dContext->PSSetShader(m_blurPS.Get(), nullptr, 0);

    // 3���_�ŉ�ʑS�̂𕢂��O�p�`��`��
    m_d3dContext->Draw(3, 0);

    // �V�F�[�_�[���N���A
    m_d3dContext->VSSetShader(nullptr, nullptr, 0);
    m_d3dContext->PSSetShader(nullptr, nullptr, 0);
}