// ========================================
// TitleScene.cpp - �^�C�g���V�[���̎���
// ========================================

#include "TitleScene.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include <WICTextureLoader.h> 
#include <vector>
#include <cstdlib>
#include <SpriteBatch.h>
#include <SpriteFont.h>

#pragma comment(lib, "d3dcompiler.lib")

TitleScene::TitleScene()
    : m_screenWidth(0)
    , m_screenHeight(0)
    , m_time(0.0f)
    , m_waveSpeed(4.0f)
    , m_waveAmplitude(0.6f)
    , m_fireParticleSystem(nullptr)
    , m_isBurning(false)
    , m_burnProgress(0.0f)
    , m_burnSpeed(0.05f)
    , m_emberWidth(0.05f)
    , m_menuState(MenuState::Burning)
    , m_selectedMenuItem(0)
    , m_menuAlpha(0.0f)
    , m_menuGlowTime(0.0f)
    , m_menuResult(MenuResult::None)
    , m_titleAlpha(0.0f)
    , m_titleVisible(true)
{
}

TitleScene::~TitleScene()
{
}

void TitleScene::Initialize(ID3D11Device* device, int screenWidth, int screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // ���̃��b�V�����쐬
    m_flagMesh = std::make_unique<FlagMesh>();
    m_flagMesh->Initialize(device, 32, 24);

    // �V�F�[�_�[�ƃe�N�X�`�����쐬
    CreateShaders(device);
    CreateBuffers(device);
    CreateTexture(device);

    //  �m�C�Y�e�N�X�`���𐶐�
    CreateNoiseTexture(device);

    // === ���̓H��G�t�F�N�g������ ===
    CreateWhiteTexture(device);
    InitBloodDrips();

    // === ���p�[�e�B�N���V�X�e�����쐬�i//�j===
    m_fireParticleSystem = std::make_unique<FireParticleSystem>();
    m_fireParticleSystem->Initialize(device, 1000);  // �ő�1000�p�[�e�B�N��

   
    // ========================================
    // �|�X�g�v���Z�X������
    // ========================================
    CreatePostProcessResources(device);
    CreatePostProcessShaders(device);

    m_blurStrength = 0.5f;

    ////  ���j���[�p�X�v���C�g������
    //m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(
    //    [&]() {
    //        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    //        device->GetImmediateContext(&context);
    //        return context.Get();
    //    }() 
    //);

    //  �����I�ɔR�ĊJ�n
    StartBurning();

    OutputDebugStringA("[TITLE] TitleScene initialized with fire particles\n");
}

void TitleScene::Update(float deltaTime)
{
    m_time += deltaTime;

    // === ��ԕʂ̍X�V ===
    switch (m_menuState)
    {
    case MenuState::Burning:
        // ���Ă̍X�V
        if (m_isBurning && m_burnProgress < 1.0f)
        {
            m_burnProgress += m_burnSpeed * deltaTime;
            if (m_burnProgress >= 1.0f)
            {
                m_burnProgress = 1.0f;
                // �R�Ċ��� �� �t�F�[�h��
                m_menuState = MenuState::FadeToMenu;
                OutputDebugStringA("[MENU] Burn complete, fading to menu...\n");
            }
        }
        break;

    case MenuState::FadeToMenu:
        // ���j���[���t�F�[�h�C��
        m_menuAlpha += deltaTime * 2.0f;  // 0.5�b�Ńt�F�[�h�C��
        if (m_menuAlpha >= 1.0f)
        {
            m_menuAlpha = 1.0f;
            m_menuState = MenuState::Menu;
            OutputDebugStringA("[MENU] Menu ready!\n");
        }

        // ���̓H��X�V
        UpdateBloodDrips(deltaTime);

        break;

    case MenuState::Menu:
        // �����A�j���[�V�����X�V
        m_menuGlowTime += deltaTime;

        // ���̓H��X�V
        UpdateBloodDrips(deltaTime);

        // === ���j���[�\������̓L�[���͂𖳎��i0.5�b�҂j===
        if (m_menuGlowTime < 0.5f)
            break;

        // === �L�[���͏��� ===
        // ��L�[
        if (GetAsyncKeyState(VK_UP) & 0x0001)
        {
            m_selectedMenuItem--;
            if (m_selectedMenuItem < 0)
                m_selectedMenuItem = 2;  // EXIT �� PLAY �Ƀ��[�v
            OutputDebugStringA("[MENU] Selection UP\n");
        }
        // ���L�[
        if (GetAsyncKeyState(VK_DOWN) & 0x0001)
        {
            m_selectedMenuItem++;
            if (m_selectedMenuItem > 2)
                m_selectedMenuItem = 0;  // PLAY �� EXIT �Ƀ��[�v
            OutputDebugStringA("[MENU] Selection DOWN\n");
        }
        // Enter�L�[�i����j
        if (GetAsyncKeyState(VK_RETURN) & 0x0001)
        {
            switch (m_selectedMenuItem)
            {
            case 0:
                m_menuResult = MenuResult::Play;
                OutputDebugStringA("[MENU] PLAY selected!\n");
                break;
            case 1:
                m_menuResult = MenuResult::Settings;
                OutputDebugStringA("[MENU] SETTINGS selected!\n");
                break;
            case 2:
                m_menuResult = MenuResult::Exit;
                OutputDebugStringA("[MENU] EXIT selected!\n");
                break;
            }
        }
        break;
    }

    // ���p�[�e�B�N���V�X�e�����X�V
    if (m_fireParticleSystem)
    {
        m_fireParticleSystem->Update(deltaTime);
    }
}

void TitleScene::Render(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* backBufferRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    {
        Microsoft::WRL::ComPtr<ID3D11Resource> backBufferResource;
        backBufferRTV->GetResource(&backBufferResource);

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBufferTex;
        backBufferResource.As(&backBufferTex);

        D3D11_TEXTURE2D_DESC bbDesc;
        backBufferTex->GetDesc(&bbDesc);

        if ((int)bbDesc.Width != m_screenWidth || (int)bbDesc.Height != m_screenHeight)
        {
            Microsoft::WRL::ComPtr<ID3D11Device> device;
            context->GetDevice(&device);
            OnResize(device.Get(), (int)bbDesc.Width, (int)bbDesc.Height);
        }
    }

    bool useFX = (m_menuState == MenuState::Menu && m_menuGlowTime > 0.3f && m_menuFxPS);

    if (useFX)
    {
        // === FX����F���ԃe�N�X�`���o�R ===
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        context->ClearRenderTargetView(m_fxSourceRTV.Get(), clearColor);

        // �w�i�V�F�[�_�[��FX�\�[�X�ɕ`��
        RenderTitleBackground(context, m_fxSourceRTV.Get(), depthStencilView);

        // ���j���[��FX�\�[�X�ɕ`��
        RenderMenu(context, m_fxSourceRTV.Get(), depthStencilView);

        // FX�V�F�[�_�[�Ńo�b�N�o�b�t�@�ɍŏI�`��
        ApplyMenuFX(context, backBufferRTV, depthStencilView);
    }
    else if (m_menuState == MenuState::FadeToMenu)
    {
        // === �t�F�[�h���F�w�i + ���j���[ ===
        RenderTitleBackground(context, backBufferRTV, depthStencilView);
        RenderMenu(context, backBufferRTV, depthStencilView);
    }
    else
    {
        // === �R�Ē��F����3D�`��===
        RenderToTexture(context);
        ApplyBlur(context, backBufferRTV, depthStencilView);
    }
}
void TitleScene::CreateShaders(ID3D11Device* device)
{
    // ========================================
    // �y�����z���V�F�[�_�[3�� .cso�i�v���R���p�C���ς݁j����ǂݍ���
    // �y���R�z�����^�C���R���p�C���͋N�����x����d3dcompiler_47.dll ���K�{�ɂȂ�B
    //         .cso �Ȃ�ǂݍ��ނ����Ȃ̂ō������z�z���y�B
    // ========================================

    HRESULT hr;  // �֐��̐��ۂ��󂯎��ϐ��i�^: HRESULT = long�j

    // --- blob: �R���p�C���ς݃o�C�i���̓��ꕨ ---
    // ComPtr = �X�}�[�g�|�C���^�B�X�R�[�v�O�Ŏ���������Ă����֗��Ȍ^�B
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // ========================================
    // ���_�V�F�[�_�[�iFlagVS�j
    // �y�����z�����b�V���̊e���_�̈ʒu���v�Z����
    // ========================================
    hr = D3DReadFileToBlob(
        L"Assets/Shaders/FlagVS.cso",  // �v���R���p�C���ς݃t�@�C���̃p�X
        &blob                           // �ǂݍ��݌��ʂ��󂯎��|�C���^
    );
    if (FAILED(hr))
    {
        OutputDebugStringA("[SHADER] FlagVS.cso load FAILED\n");
        throw std::runtime_error("Failed to load FlagVS.cso");
    }

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(),  // �o�C�i���f�[�^�̐擪�A�h���X
        blob->GetBufferSize(),     // �o�C�i���f�[�^�̃T�C�Y�i�o�C�g�j
        nullptr,                   // �N���X�����N�i�ʏ� nullptr�j
        &m_vertexShader            // �쐬���ꂽ�V�F�[�_�[�̏o�͐�
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create FlagVS");

    // ========================================
    // ���̓��C�A�E�g�i���_�f�[�^�̍\���� GPU �ɋ�����j
    // �y���R�zblob ���Ȃ��� InputLayout �����Ȃ��B
    //         .cso �� blob �ł� D3DCompileFromFile �� blob �ł������悤�Ɏg����B
    // ========================================
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        // �Z�}���e�B�b�N��, �C���f�b�N�X, �t�H�[�}�b�g, �X���b�g, �I�t�Z�b�g, ����, �X�e�b�v���[�g
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),         // �z��̗v�f���i= 3�j
        blob->GetBufferPointer(),  // ���_�V�F�[�_�[�̃o�C�g�R�[�h
        blob->GetBufferSize(),     // �o�C�g�R�[�h�̃T�C�Y
        &m_inputLayout             // �o�͐�
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create flag input layout");

    OutputDebugStringA("[SHADER] FlagVS loaded from CSO\n");

    // ========================================
    // �s�N�Z���V�F�[�_�[�iFlagPS�j
    // �y�����z���̊e�s�N�Z���̐F���v�Z����i�e�N�X�`���{���C�e�B���O�j
    // ========================================
    blob.Reset();  // �O�� blob ��������Ă���ė��p
    hr = D3DReadFileToBlob(L"Assets/Shaders/FlagPS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[SHADER] FlagPS.cso load FAILED\n");
        throw std::runtime_error("Failed to load FlagPS.cso");
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_pixelShader
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create FlagPS");

    OutputDebugStringA("[SHADER] FlagPS loaded from CSO\n");

    // ========================================
    // �R�ăs�N�Z���V�F�[�_�[�iBurnFlagPS�j
    // �y�����z�����R���鉉�o�̐F���v�Z����
    // ========================================
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/BurnFlagPS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[SHADER] BurnFlagPS.cso load FAILED\n");
        throw std::runtime_error("Failed to load BurnFlagPS.cso");
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_burnPixelShader
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create BurnFlagPS");

    OutputDebugStringA("[SHADER] BurnFlagPS loaded from CSO\n");
}

void TitleScene::CreateBuffers(ID3D11Device* device)
{
    // === �s��p�萔�o�b�t�@ ===
    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = device->CreateBuffer(&matrixBufferDesc, nullptr, &m_matrixBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create matrix buffer");
    }

    // === ���ԗp�萔�o�b�t�@ ===
    D3D11_BUFFER_DESC timeBufferDesc = {};
    timeBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    timeBufferDesc.ByteWidth = sizeof(TimeBufferType);
    timeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = device->CreateBuffer(&timeBufferDesc, nullptr, &m_timeBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create time buffer");
    }

    // === ���ăp�����[�^�p�萔�o�b�t�@ ===
    D3D11_BUFFER_DESC burnBufferDesc = {};
    burnBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    burnBufferDesc.ByteWidth = sizeof(BurnBufferType);
    burnBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = device->CreateBuffer(&burnBufferDesc, nullptr, &m_burnBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create burn buffer");
    }

    // === ���j���[FX�p�萔�o�b�t�@ ===
    D3D11_BUFFER_DESC menuFxBufferDesc = {};
    menuFxBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    menuFxBufferDesc.ByteWidth = sizeof(MenuFXBufferType);
    menuFxBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = device->CreateBuffer(&menuFxBufferDesc, nullptr, &m_menuFxBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create menu FX buffer");
    }

    // === �^�C�g���w�i�p�萔�o�b�t�@ ===
    D3D11_BUFFER_DESC titleBgBufferDesc = {};
    titleBgBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    titleBgBufferDesc.ByteWidth = sizeof(TitleBGBufferType);
    titleBgBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = device->CreateBuffer(&titleBgBufferDesc, nullptr, &m_titleBgBuffer);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create title BG buffer");
}

void TitleScene::CreateTexture(ID3D11Device* device)
{
    // === �摜�t�@�C������ǂݍ��� ===
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        device,
        L"flag_with_logo.png",  // �t�@�C����
        nullptr,
        &m_flagTexture
    );

    if (FAILED(hr))
    {
        // �摜�ǂݍ��ݎ��s���͔����e�N�X�`�����쐬
        OutputDebugStringA("[TEXTURE] Failed to load flag_with_logo.png, creating white texture\n");

        // 4�~4�̔����e�N�X�`���i�t�H�[���o�b�N�j
        uint32_t textureData[16];
        for (int i = 0; i < 16; i++)
        {
            textureData[i] = 0xFFFFFFFF;  // ��
        }

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = 4;
        texDesc.Height = 4;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_IMMUTABLE;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = textureData;
        initData.SysMemPitch = 4 * sizeof(uint32_t);

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        hr = device->CreateTexture2D(&texDesc, &initData, &texture);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create fallback texture");
        }

        hr = device->CreateShaderResourceView(texture.Get(), nullptr, &m_flagTexture);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create shader resource view");
        }
    }
    else
    {
        OutputDebugStringA("[TEXTURE] Successfully loaded flag_with_logo.png\n");
    }

    // === �T���v���[�X�e�[�g�쐬 ===
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;  // �� WRAP ���� CLAMP �ɕύX
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;  // �� WRAP ���� CLAMP �ɕύX
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, &m_samplerState);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create sampler state");
    }
}

void TitleScene::CreateNoiseTexture(ID3D11Device* device)
{
    // �v���V�[�W�����m�C�Y�e�N�X�`���𐶐��i256x256�j
    const int width = 256;
    const int height = 256;

    std::vector<uint8_t> noiseData(width * height * 4);

    // �n�b�V���֐��i�m�C�Y�����p�j
    auto hash = [](int x, int y) -> float {
        int n = x + y * 57;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
        };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // �����̃I�N�^�[�u�Ńm�C�Y���d�˂�
            float noise = 0.0f;
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float maxValue = 0.0f;

            for (int octave = 0; octave < 4; ++octave)
            {
                float fx = x * frequency / 32.0f;
                float fy = y * frequency / 32.0f;

                int ix = (int)fx;
                int iy = (int)fy;
                float tx = fx - ix;
                float ty = fy - iy;

                // 4���̃����_���l
                float v00 = hash(ix, iy);
                float v10 = hash(ix + 1, iy);
                float v01 = hash(ix, iy + 1);
                float v11 = hash(ix + 1, iy + 1);

                // �o�C���j�A���
                float v0 = v00 + tx * (v10 - v00);
                float v1 = v01 + tx * (v11 - v01);
                float value = v0 + ty * (v1 - v0);

                noise += value * amplitude;
                maxValue += amplitude;

                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            // 0-1�ɐ��K��
            noise = (noise / maxValue + 1.0f) * 0.5f;

            // �O���f�[�V�����i�������֔R����j��//
            float gradient = (float)y / height;
            noise = noise * 0.7f + gradient * 0.3f;

            uint8_t value = (uint8_t)(noise * 255);

            int idx = (y * width + x) * 4;
            noiseData[idx + 0] = value;  // R
            noiseData[idx + 1] = value;  // G
            noiseData[idx + 2] = value;  // B
            noiseData[idx + 3] = 255;    // A
        }
    }

    // �e�N�X�`���쐬
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = noiseData.data();
    initData.SysMemPitch = width * 4;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> noiseTex;
    HRESULT hr = device->CreateTexture2D(&texDesc, &initData, &noiseTex);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create noise texture");
    }

    // SRV�쐬
    hr = device->CreateShaderResourceView(noiseTex.Get(), nullptr, &m_noiseTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create noise SRV");
    }

    OutputDebugStringA("[TITLE] Noise texture created for burn effect\n");
}

void TitleScene::CreatePostProcessResources(ID3D11Device* device)
{
    // �����_�[�e�N�X�`�����쐬
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_screenWidth;              // ��ʕ�
    texDesc.Height = m_screenHeight;            // ��ʍ���
    texDesc.MipLevels = 1;                      // �~�b�v�}�b�v�Ȃ�
    texDesc.ArraySize = 1;                      // �z��T�C�Y1
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA 8bit
    texDesc.SampleDesc.Count = 1;               // �}���`�T���v���Ȃ�
    texDesc.Usage = D3D11_USAGE_DEFAULT;        // GPU�ǂݏ���
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &m_renderTexture);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create render texture");

    // �����_�[�^�[�Q�b�g�r���[���쐬 
    hr = device->CreateRenderTargetView(m_renderTexture.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create render target view");

    //  �V�F�[�_�[���\�[�X�r���[���쐬
    hr = device->CreateShaderResourceView(m_renderTexture.Get(), nullptr, &m_renderTextureSRV);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create shader resource view");

    // === FX�p��2�����_�[�e�N�X�`���i�����ݒ�ł���1���j===
    Microsoft::WRL::ComPtr<ID3D11Texture2D> fxTex;
    hr = device->CreateTexture2D(&texDesc, nullptr, &fxTex);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create FX source texture");

    m_fxSourceTexture = fxTex;

    hr = device->CreateRenderTargetView(m_fxSourceTexture.Get(), nullptr, &m_fxSourceRTV);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create FX source RTV");

    hr = device->CreateShaderResourceView(m_fxSourceTexture.Get(), nullptr, &m_fxSourceSRV);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create FX source SRV");

    OutputDebugStringA("[TITLE] FX source texture created\n");

    //  �t���X�N���[���N�A�b�h�i�l�p�`�j�̒��_�f�[�^
    struct VertexPos2D
    {
        DirectX::XMFLOAT3 position;  // �ʒu�iX, Y, Z�j
        DirectX::XMFLOAT2 texcoord;  // UV���W
    };

    // ��ʑS�̂𕢂��l�p�`�i2�̎O�p�`�j
    VertexPos2D vertices[] =
    {
        // �ʒu�iNDC���W: -1?1�j      UV���W�i0?1�j
        { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },  // ����
        { DirectX::XMFLOAT3(1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },  // �E��
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },  // ����
        { DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },  // �E��
    };

    // ���_�o�b�t�@�쐬
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;       // �s�ρi�ύX���Ȃ��j
    vbDesc.ByteWidth = sizeof(vertices);         // �T�C�Y
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // ���_�o�b�t�@
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;                   // �f�[�^

    hr = device->CreateBuffer(&vbDesc, &vbData, &m_fullscreenQuadVB);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create fullscreen quad vertex buffer");

    //  �T���v���[�X�e�[�g�쐬
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // ���`���
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;    // �[�̓N�����v
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, &m_postProcessSampler);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create post-process sampler");

    OutputDebugStringA("[TITLE] Post-process resources created\n");
}

void TitleScene::CreatePostProcessShaders(ID3D11Device* device)
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // ========================================
    // �|�X�g�v���Z�X ���_�V�F�[�_�[
    // ========================================
    hr = D3DReadFileToBlob(L"Assets/Shaders/PostProcessVS.cso", &blob);
    if (FAILED(hr))
        throw std::runtime_error("Failed to load PostProcessVS.cso");

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_postProcessVS);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create post-process vertex shader");

    // ���̓��C�A�E�g�iPostProcessVS �� CSO ����쐬�j
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        &m_postProcessLayout);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create post-process input layout");

    OutputDebugStringA("[SHADER] PostProcessVS loaded from CSO\n");

    // ========================================
    // �u���[ �s�N�Z���V�F�[�_�[
    // ========================================
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/BlurPS.cso", &blob);
    if (FAILED(hr))
        throw std::runtime_error("Failed to load BlurPS.cso");

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_blurPS);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create blur pixel shader");

    OutputDebugStringA("[SHADER] BlurPS loaded from CSO\n");

    // ========================================
    // ���j���[ FX �s�N�Z���V�F�[�_�[
    // ========================================
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/MenuPostProcessPS.cso", &blob);
    if (FAILED(hr))
        throw std::runtime_error("Failed to load MenuPostProcessPS.cso");

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_menuFxPS);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create menu FX shader");

    OutputDebugStringA("[SHADER] MenuPostProcessPS loaded from CSO\n");

    // ========================================
    // �^�C�g���w�i �s�N�Z���V�F�[�_�[
    // ========================================
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/TitleBackgroundPS.cso", &blob);
    if (FAILED(hr))
        throw std::runtime_error("Failed to load TitleBackgroundPS.cso");

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_titleBgPS);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create title BG shader");

    OutputDebugStringA("[SHADER] TitleBackgroundPS loaded from CSO\n");
    OutputDebugStringA("[TITLE] All post-process shaders loaded from CSO\n");
}

void TitleScene::RenderToTexture(ID3D11DeviceContext* context)
{
    // ========================================
    // �����_�[�^�[�Q�b�g�������_�[�e�N�X�`���ɐ؂�ւ�
    // ========================================
    context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

    // �N���A�i���j
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    // ========================================
    // �r���[�|�[�g�ݒ�
    // ========================================
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // ========================================
    // �R�ăp�����[�^���X�V
    // ========================================
    BurnBufferType burnData;
    burnData.burnProgress = m_burnProgress;
    burnData.time = m_time;
    burnData.emberWidth = m_emberWidth;
    burnData.burnDirection = 0.0f;
    context->UpdateSubresource(m_burnBuffer.Get(), 0, nullptr, &burnData, 0, 0);

    // ========================================
    // �V�F�[�_�[�ݒ�iPostProcessVS + BurnFlagPS�j
    // ========================================
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_burnPixelShader.Get(), nullptr, 0);   // �R��PS�͂��̂܂�
    context->IASetInputLayout(m_postProcessLayout.Get());

    // �萔�o�b�t�@�i�R�ăp�����[�^�j
    context->PSSetConstantBuffers(0, 1, m_burnBuffer.GetAddressOf());

    // �e�N�X�`���i���e�N�X�`�� + �m�C�Y�e�N�X�`���j
    ID3D11ShaderResourceView* textures[2] = { m_flagTexture.Get(), m_noiseTexture.Get() };
    context->PSSetShaderResources(0, 2, textures);
    context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    // ========================================
    // �t���X�N���[���N�A�b�h��`��
    // ========================================
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);  // position + texcoord
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    // ========================================
    // �V�F�[�_�[���\�[�X���N���A
    // ========================================
    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRV);
}

// ========================================
// ApplyBlur() - �u���[��K�p
// ========================================

void TitleScene::ApplyBlur(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* backBufferRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    // ========================================
    // �o�b�N�o�b�t�@�ɖ߂��i�d�v�I�j
    // ========================================
    context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

    // �r���[�|�[�g��ݒ�i��ʑS�́j
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // === �|�X�g�v���Z�X�p�V�F�[�_�[��ݒ� ===
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_blurPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    // === �����_�[�e�N�X�`�����V�F�[�_�[�ɓn�� ===
    context->PSSetShaderResources(0, 1, m_renderTextureSRV.GetAddressOf());
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    // === �t���X�N���[���N�A�b�h��`�� ===
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // �`��i4���_�j
    context->Draw(4, 0);

    // ===  �V�F�[�_�[���\�[�X���N���A===
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    //OutputDebugStringA("[BLUR] Applied successfully to back buffer\n");
}

void TitleScene::ApplyBlurTo(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* targetRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    // ApplyBlur�Ɠ��������A�o�͐���w��ł���
    context->OMSetRenderTargets(1, &targetRTV, depthStencilView);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_blurPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    context->PSSetShaderResources(0, 1, m_renderTextureSRV.GetAddressOf());
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);
}

void TitleScene::StartBurning()
{
    m_isBurning = true;
    OutputDebugStringA("[BURN] Started burning!\n");
}

void TitleScene::StopBurning()
{
    m_isBurning = false;
    OutputDebugStringA("[BURN] Stopped burning.\n");
}

void TitleScene::ResetBurn()
{
    m_burnProgress = 0.0f;
    m_isBurning = false;
    OutputDebugStringA("[BURN] Reset burn progress.\n");
}



void TitleScene::RenderMenu(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* backBufferRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    // === SpriteBatch���������i����̂݁j===
    if (!m_spriteBatch)
    {
        m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
    }

    // === �t�H���g��ǂݍ��݁i����̂݁j===
    if (!m_menuFont)
    {
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        context->GetDevice(&device);

        try
        {
            m_menuFont = std::make_unique<DirectX::SpriteFont>(
                device.Get(),
                L"Assets/Fonts/menu_font.spritefont"
            );
        }
        catch (...)
        {
            OutputDebugStringA("[MENU] Failed to load font\n");
            return;
        }
    }

    // === �`��ݒ� ===
    context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

    // === SpriteBatch�J�n�i�A���t�@�u�����h�L���j===
    m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, nullptr);

    // === �w�i�̃_�[�N�O���f�[�V���� ===
    if (m_whiteTexture)
    {
        RECT srcRect = { 0, 0, 1, 1 };

        // �Â��Ԃ̃r�l�b�g�i��ʑS�́j
        DirectX::XMVECTORF32 bgColor = { 0.06f, 0.01f, 0.01f, m_menuAlpha * 0.85f };
        m_spriteBatch->Draw(
            m_whiteTexture.Get(),
            DirectX::XMFLOAT2(0, 0),
            &srcRect,
            bgColor,
            0.0f,
            DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2((float)m_screenWidth, (float)m_screenHeight)
        );

        // �㕔�ɔZ���ԁi���̗��܂�j
        DirectX::XMVECTORF32 topBloodColor = { 0.15f, 0.02f, 0.02f, m_menuAlpha * 0.6f };
        m_spriteBatch->Draw(
            m_whiteTexture.Get(),
            DirectX::XMFLOAT2(0, 0),
            &srcRect,
            topBloodColor,
            0.0f,
            DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2((float)m_screenWidth, (float)m_screenHeight * 0.15f)
        );


    }

    // === ���̓H���`�� ===
    RenderBloodDrips();

    // ��ʒ�����菭�����ɔz�u
    float centerX = m_screenWidth / 2.0f;
    float startY = m_screenHeight * 0.55f;
    float itemSpacing = 75.0f;

    {
        const wchar_t* title = L"SLAY WAVE";
        DirectX::XMVECTOR titleSize = m_menuFont->MeasureString(title);
        float titleWidth = DirectX::XMVectorGetX(titleSize);
        float titleX = centerX - (titleWidth * 1.8f) / 2.0f;
        float titleY = m_screenHeight * 0.15f;

        // �Ԃ��e�i���ɂ��炵�ĕ`��j
        float titlePulse = sinf(m_menuGlowTime * 2.0f) * 0.5f + 0.5f;
        DirectX::XMVECTORF32 titleShadow = { 0.5f, 0.02f, 0.02f, m_menuAlpha * 0.5f };
        m_menuFont->DrawString(
            m_spriteBatch.get(), title,
            DirectX::XMFLOAT2(titleX + 3.0f, titleY + 3.0f),
            titleShadow, 0.0f, DirectX::XMFLOAT2(0, 0), 1.8f
        );

        // ���C���^�C�g���i�N���̐ԁA�����j
        float tR = 0.75f + titlePulse * 0.25f;
        float tG = 0.04f + titlePulse * 0.06f;
        float tB = 0.02f + titlePulse * 0.03f;
        DirectX::XMVECTORF32 titleColor = { tR, tG, tB, m_menuAlpha };
        m_menuFont->DrawString(
            m_spriteBatch.get(), title,
            DirectX::XMFLOAT2(titleX, titleY),
            titleColor, 0.0f, DirectX::XMFLOAT2(0, 0), 1.8f
        );
    }

    // ���j���[���ځiDOOM���̖����j
    const wchar_t* menuItems[] = {
        L"PLAY",
        L"SETTINGS",
        L"EXIT"
    };

    

    for (int i = 0; i < 3; ++i)
    {
        // ������̃T�C�Y���擾
        DirectX::XMVECTOR textSize = m_menuFont->MeasureString(menuItems[i]);
        float textWidth = DirectX::XMVectorGetX(textSize);
        float textHeight = DirectX::XMVectorGetY(textSize);

        // �ʒu�v�Z�i���������j
        float posX = centerX - textWidth / 2.0f;
        float posY = startY + i * itemSpacing;

        if (i == m_selectedMenuItem)
        {
            // ============================================
            // �I�𒆁F���̂悤�ɖ��łԂ�����
            // ============================================

            // �S���̌ۓ����p���X�i�������x���������̌J��Ԃ��j
            float heartbeat = sinf(m_menuGlowTime * 8.0f) * 0.5f + 0.5f;
            float pulse = heartbeat * heartbeat;  // ���ŉs���p���X��

            // �����������ɐk����i���|���j
            float shakeX = sinf(m_menuGlowTime * 25.0f) * pulse * 2.0f;
            float shakeY = cosf(m_menuGlowTime * 30.0f) * pulse * 1.5f;

            // === �w��̌��̟��݁i�傫�߂ɈÂ��Ԃŕ`��j===
            float bleedSize = 1.05f + pulse * 0.03f;
            DirectX::XMVECTORF32 bleedColor = { 0.5f, 0.03f, 0.02f, m_menuAlpha * 0.6f };

            m_menuFont->DrawString(
                m_spriteBatch.get(),
                menuItems[i],
                DirectX::XMFLOAT2(posX - 2.0f + shakeX, posY - 2.0f + shakeY),
                bleedColor,
                0.0f,
                DirectX::XMFLOAT2(0, 0),
                bleedSize
            );

            // === ���C���̕����i�N���̐ԁj===
            float r = 0.85f + pulse * 0.15f;
            float g = 0.06f + pulse * 0.08f;
            float b = 0.02f + pulse * 0.03f;
            DirectX::XMVECTORF32 mainColor = { r, g, b, m_menuAlpha };

            m_menuFont->DrawString(
                m_spriteBatch.get(),
                menuItems[i],
                DirectX::XMFLOAT2(posX + shakeX, posY + shakeY),
                mainColor
            );

            // === �����Ɍ��̃}�[�J�[�i>>>�ł͂Ȃ��Ԃ��_�j===
            DirectX::XMVECTORF32 markerColor = { 1.0f, 0.1f, 0.05f, m_menuAlpha * (0.5f + pulse * 0.5f) };

            // �����̌��̐��i������ "|" �ő������Ɍ�����j
            float barX = posX - 50.0f;
            m_menuFont->DrawString(
                m_spriteBatch.get(),
                L"I",
                DirectX::XMFLOAT2(barX + shakeX, posY + shakeY),
                markerColor
            );
            m_menuFont->DrawString(
                m_spriteBatch.get(),
                L"I",
                DirectX::XMFLOAT2(barX + 5.0f + shakeX, posY + shakeY),
                markerColor
            );
        }
        else
        {
            // ============================================
            // ��I���F���������̐F�i�Â��Ԋ��F�j
            // ============================================
            DirectX::XMVECTORF32 driedBloodColor = {
            0.35f, 0.06f, 0.04f, m_menuAlpha * 0.7f
            };

            m_menuFont->DrawString(
                m_spriteBatch.get(),
                menuItems[i],
                DirectX::XMFLOAT2(posX, posY),
                driedBloodColor
            );
        }
    }

    // === ��ʉ����ɁuPRESS ENTER�v�̓_�Ńe�L�X�g ===
    float blinkAlpha = (sinf(m_menuGlowTime * 3.0f) + 1.0f) / 2.0f;
    DirectX::XMVECTORF32 hintColor = { 0.5f, 0.08f, 0.05f, m_menuAlpha * blinkAlpha * 0.6f };

    const wchar_t* hint = L"- PRESS SPACE -";
    DirectX::XMVECTOR hintSize = m_menuFont->MeasureString(hint);
    float hintX = m_screenWidth - DirectX::XMVectorGetX(hintSize) * 0.6f - 30.0f;
    float hintY = m_screenHeight * 0.92f;

    m_menuFont->DrawString(
        m_spriteBatch.get(),
        hint,
        DirectX::XMFLOAT2(hintX, hintY),
        hintColor,
        0.0f,
        DirectX::XMFLOAT2(0, 0),
        0.6f  // �����߂̃t�H���g�T�C�Y
    );

    // === SpriteBatch�I�� ===
    m_spriteBatch->End();
}

void TitleScene::CreateWhiteTexture(ID3D11Device* device)
{
    uint32_t white = 0xFFFFFFFF;

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
    initData.pSysMem = &white;
    initData.SysMemPitch = 4;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &tex);
    if (SUCCEEDED(hr))
    {
        device->CreateShaderResourceView(tex.Get(), nullptr, &m_whiteTexture);
        OutputDebugStringA("[TITLE] White texture created for blood drips\n");
    }
}

// ========================================
// ���̓H�菉����
// ========================================
void TitleScene::InitBloodDrips()
{
    srand(42);  // �Œ�V�[�h�Ŗ��񓯂��p�^�[��

    for (int i = 0; i < MAX_BLOOD_DRIPS; ++i)
    {
        auto& drip = m_bloodDrips[i];
        drip.x = (float)(rand() % m_screenWidth);
        drip.y = -(float)(rand() % 200);  // ��ʏ㕔�̊O����J�n
        drip.speed = 30.0f + (float)(rand() % 80);  // 30?110 px/s
        drip.length = 50.0f + (float)(rand() % 250);  // 50?300 px
        drip.width = 2.0f + (float)(rand() % 6);  // 2?8 px
        drip.alpha = 0.3f + (float)(rand() % 50) / 100.0f;  // 0.3?0.8
        drip.delay = (float)(rand() % 500) / 100.0f;  // 0?5�b�̒x��
        drip.active = false;
    }
}

// ========================================
// ���̓H��X�V
// ========================================
void TitleScene::UpdateBloodDrips(float deltaTime)
{
    for (int i = 0; i < MAX_BLOOD_DRIPS; ++i)
    {
        auto& drip = m_bloodDrips[i];

        // �x���`�F�b�N
        if (!drip.active)
        {
            drip.delay -= deltaTime;
            if (drip.delay <= 0.0f)
            {
                drip.active = true;
            }
            continue;
        }

        // ����
        drip.y += drip.speed * deltaTime;

        // ��ʉ��𒴂����烊�Z�b�g
        if (drip.y - drip.length > m_screenHeight)
        {
            drip.y = -(float)(rand() % 100);
            drip.speed = 30.0f + (float)(rand() % 80);
            drip.length = 50.0f + (float)(rand() % 250);
            drip.delay = (float)(rand() % 300) / 100.0f;
            drip.active = false;
        }
    }
}

// ========================================
// ���̓H��`��
// ========================================
void TitleScene::RenderBloodDrips()
{
    if (!m_spriteBatch || !m_whiteTexture) return;

    for (int i = 0; i < MAX_BLOOD_DRIPS; ++i)
    {
        const auto& drip = m_bloodDrips[i];
        if (!drip.active) continue;

        // === ���C���̌��� ===
        // �`��͈͂���ʓ��ɃN���b�v
        float topY = drip.y - drip.length;
        if (topY < 0.0f) topY = 0.0f;
        float bottomY = drip.y;
        if (bottomY > (float)m_screenHeight) bottomY = (float)m_screenHeight;

        float drawLength = bottomY - topY;
        if (drawLength <= 0.0f) continue;

        // �Â����̐F�i�؂��Ƃɔ����ɐF��ς���j
        float rBase = 0.3f + (float)(i % 5) * 0.05f;
        float gBase = 0.1f + (float)(i % 4) * 0.03f;
        float bBase = 0.02f;

        // ��`�Ƃ��ĕ`��iRECT�w��j
        RECT srcRect = { 0, 0, 1, 1 };

        // ���C���̌���
        DirectX::XMVECTORF32 bloodColor = { rBase, gBase, bBase, drip.alpha * m_menuAlpha };

        m_spriteBatch->Draw(
            m_whiteTexture.Get(),
            DirectX::XMFLOAT2(drip.x, topY),
            &srcRect,
            bloodColor,
            0.0f,
            DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2(drip.width, drawLength)
        );

        // === ��[�̖c��݁i�H�̊ۂ݁j===
        if (drip.y > 0.0f && drip.y < (float)m_screenHeight)
        {
            float tipSize = drip.width * 1.8f;
            DirectX::XMVECTORF32 tipColor = { rBase + 0.2f, gBase * 0.5f, 0.02f, drip.alpha * m_menuAlpha * 0.8f };

            m_spriteBatch->Draw(
                m_whiteTexture.Get(),
                DirectX::XMFLOAT2(drip.x - tipSize * 0.25f, drip.y - tipSize * 0.5f),
                &srcRect,
                tipColor,
                0.0f,
                DirectX::XMFLOAT2(0, 0),
                DirectX::XMFLOAT2(tipSize, tipSize)
            );
        }

        // === ���̟͂��݁i�����Ԃ̉e�j===
        DirectX::XMVECTORF32 smearColor = { rBase * 0.5f, gBase * 0.3f, 0.01f, drip.alpha * m_menuAlpha * 0.2f };

        m_spriteBatch->Draw(
            m_whiteTexture.Get(),
            DirectX::XMFLOAT2(drip.x - drip.width * 0.5f, topY),
            &srcRect,
            smearColor,
            0.0f,
            DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2(drip.width * 2.0f, drawLength)
        );
    }
}

void TitleScene::ApplyMenuFX(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* backBufferRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    // FX�V�F�[�_�[�����쐬�Ȃ�ʏ�̃u���[�Ƀt�H�[���o�b�N
    if (!m_menuFxPS)
    {
        ApplyBlur(context, backBufferRTV, depthStencilView);
        return;
    }

    // ============================================
    // 1) �o�b�N�o�b�t�@�ɕ`����ݒ�
    // ============================================
    context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // ============================================
    // 2) ���j���[FX�p�����[�^���X�V
    // ============================================
    float heartbeat = sinf(m_menuGlowTime * 8.0f) * 0.5f + 0.5f;
    heartbeat = heartbeat * heartbeat;

    MenuFXBufferType fxData;
    fxData.time = m_time;
    fxData.shakeIntensity = heartbeat * 0.3f;
    fxData.vignetteIntensity = 0.8f + heartbeat * 0.2f;
    fxData.lightningIntensity = 0.6f;
    fxData.chromaticAmount = 0.002f + heartbeat * 0.003f;
    fxData.screenWidth = (float)m_screenWidth;
    fxData.screenHeight = (float)m_screenHeight;
    fxData.heartbeat = heartbeat;

    context->UpdateSubresource(m_menuFxBuffer.Get(), 0, nullptr, &fxData, 0, 0);

    // ============================================
    // 3) �V�F�[�_�[�ݒ�
    //    �� m_renderTextureSRV �𒼐ړǂ�
    // ============================================
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_menuFxPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    // �萔�o�b�t�@
    context->PSSetConstantBuffers(0, 1, m_menuFxBuffer.GetAddressOf());

    // �e�N�X�`���i���j���[�`��ς݂�FX�\�[�X + �m�C�Y�j
    ID3D11ShaderResourceView* textures[2] = { m_fxSourceSRV.Get(), m_noiseTexture.Get() };
    context->PSSetShaderResources(0, 2, textures);
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    // ============================================
    // 4) �t���X�N���[���N�A�b�h�`��
    // ============================================
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    // ============================================
    // 5) �V�F�[�_�[���\�[�X���N���A
    // ============================================
    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRV);
}

void TitleScene::RenderTitleBackground(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* targetRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    if (!m_titleBgPS) return;

    context->OMSetRenderTargets(1, &targetRTV, depthStencilView);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // �p�����[�^�X�V
    TitleBGBufferType bgData;
    bgData.time = m_time;
    bgData.screenWidth = (float)m_screenWidth;
    bgData.screenHeight = (float)m_screenHeight;
    bgData.intensity = min(m_time * 0.5f, 1.0f);  // 2�b�Ńt�F�[�h�C��

    context->UpdateSubresource(m_titleBgBuffer.Get(), 0, nullptr, &bgData, 0, 0);

    // �V�F�[�_�[�ݒ�
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_titleBgPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    context->PSSetConstantBuffers(0, 1, m_titleBgBuffer.GetAddressOf());
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    // �t���X�N���[���N�A�b�h�`��
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);
}

// ========================================
// ��ʃ��T�C�Y���Ƀ����_�[�e�N�X�`�����č쐬
// ========================================
void TitleScene::OnResize(ID3D11Device* device, int newWidth, int newHeight)
{
    // �T�C�Y�������Ȃ牽�����Ȃ�
    if (newWidth == m_screenWidth && newHeight == m_screenHeight)
        return;

    OutputDebugStringA("[TITLE] OnResize called - recreating render textures\n");

    // ��ʃT�C�Y���X�V
    m_screenWidth = newWidth;
    m_screenHeight = newHeight;

    // === �Â����\�[�X����� ===
    m_renderTexture.Reset();
    m_renderTargetView.Reset();
    m_renderTextureSRV.Reset();
    m_fxSourceTexture.Reset();
    m_fxSourceRTV.Reset();
    m_fxSourceSRV.Reset();

    // === �V�����T�C�Y�Ń����_�[�e�N�X�`�����č쐬 ===
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_screenWidth;
    texDesc.Height = m_screenHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    // --- ���C�������_�[�e�N�X�`�� ---
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &m_renderTexture);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate render texture\n"); return; }

    hr = device->CreateRenderTargetView(m_renderTexture.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate RTV\n"); return; }

    hr = device->CreateShaderResourceView(m_renderTexture.Get(), nullptr, &m_renderTextureSRV);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate SRV\n"); return; }

    // --- FX�p�����_�[�e�N�X�`�� ---
    hr = device->CreateTexture2D(&texDesc, nullptr, &m_fxSourceTexture);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate FX texture\n"); return; }

    hr = device->CreateRenderTargetView(m_fxSourceTexture.Get(), nullptr, &m_fxSourceRTV);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate FX RTV\n"); return; }

    hr = device->CreateShaderResourceView(m_fxSourceTexture.Get(), nullptr, &m_fxSourceSRV);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate FX SRV\n"); return; }

    // SpriteBatch�����Z�b�g�i�V�����T�C�Y�ōď�����������j
    m_spriteBatch.reset();

    OutputDebugStringA("[TITLE] Render textures recreated for new size\n");
}