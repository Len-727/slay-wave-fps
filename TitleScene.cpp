// ========================================
// TitleScene.cpp - タイトルシーンの実装
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

    // 旗のメッシュを作成
    m_flagMesh = std::make_unique<FlagMesh>();
    m_flagMesh->Initialize(device, 32, 24);

    // シェーダーとテクスチャを作成
    CreateShaders(device);
    CreateBuffers(device);
    CreateTexture(device);

    //  ノイズテクスチャを生成
    CreateNoiseTexture(device);

    // === 血の滴りエフェクト初期化 ===
    CreateWhiteTexture(device);
    InitBloodDrips();

    // === 炎パーティクルシステムを作成（//）===
    m_fireParticleSystem = std::make_unique<FireParticleSystem>();
    m_fireParticleSystem->Initialize(device, 1000);  // 最大1000パーティクル

   
    // ========================================
    // ポストプロセス初期化
    // ========================================
    CreatePostProcessResources(device);
    CreatePostProcessShaders(device);

    m_blurStrength = 0.5f;

    ////  メニュー用スプライト初期化
    //m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(
    //    [&]() {
    //        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    //        device->GetImmediateContext(&context);
    //        return context.Get();
    //    }() 
    //);

    //  自動的に燃焼開始
    StartBurning();

    OutputDebugStringA("[TITLE] TitleScene initialized with fire particles\n");
}

void TitleScene::Update(float deltaTime)
{
    m_time += deltaTime;

    // === 状態別の更新 ===
    switch (m_menuState)
    {
    case MenuState::Burning:
        // 燻焼の更新
        if (m_isBurning && m_burnProgress < 1.0f)
        {
            m_burnProgress += m_burnSpeed * deltaTime;
            if (m_burnProgress >= 1.0f)
            {
                m_burnProgress = 1.0f;
                // 燃焼完了 → フェードへ
                m_menuState = MenuState::FadeToMenu;
                OutputDebugStringA("[MENU] Burn complete, fading to menu...\n");
            }
        }
        break;

    case MenuState::FadeToMenu:
        // メニューをフェードイン
        m_menuAlpha += deltaTime * 2.0f;  // 0.5秒でフェードイン
        if (m_menuAlpha >= 1.0f)
        {
            m_menuAlpha = 1.0f;
            m_menuState = MenuState::Menu;
            OutputDebugStringA("[MENU] Menu ready!\n");
        }

        // 血の滴り更新
        UpdateBloodDrips(deltaTime);

        break;

    case MenuState::Menu:
        // 発光アニメーション更新
        m_menuGlowTime += deltaTime;

        // 血の滴り更新
        UpdateBloodDrips(deltaTime);

        // === メニュー表示直後はキー入力を無視（0.5秒待つ）===
        if (m_menuGlowTime < 0.5f)
            break;

        // === キー入力処理 ===
        // 上キー
        if (GetAsyncKeyState(VK_UP) & 0x0001)
        {
            m_selectedMenuItem--;
            if (m_selectedMenuItem < 0)
                m_selectedMenuItem = 2;  // EXIT → PLAY にループ
            OutputDebugStringA("[MENU] Selection UP\n");
        }
        // 下キー
        if (GetAsyncKeyState(VK_DOWN) & 0x0001)
        {
            m_selectedMenuItem++;
            if (m_selectedMenuItem > 2)
                m_selectedMenuItem = 0;  // PLAY → EXIT にループ
            OutputDebugStringA("[MENU] Selection DOWN\n");
        }
        // Enterキー（決定）
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

    // 炎パーティクルシステムを更新
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
        // === FXあり：中間テクスチャ経由 ===
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        context->ClearRenderTargetView(m_fxSourceRTV.Get(), clearColor);

        // 背景シェーダーをFXソースに描画
        RenderTitleBackground(context, m_fxSourceRTV.Get(), depthStencilView);

        // メニューをFXソースに描画
        RenderMenu(context, m_fxSourceRTV.Get(), depthStencilView);

        // FXシェーダーでバックバッファに最終描画
        ApplyMenuFX(context, backBufferRTV, depthStencilView);
    }
    else if (m_menuState == MenuState::FadeToMenu)
    {
        // === フェード中：背景 + メニュー ===
        RenderTitleBackground(context, backBufferRTV, depthStencilView);
        RenderMenu(context, backBufferRTV, depthStencilView);
    }
    else
    {
        // === 燃焼中：旗の3D描画===
        RenderToTexture(context);
        ApplyBlur(context, backBufferRTV, depthStencilView);
    }
}
void TitleScene::CreateShaders(ID3D11Device* device)
{
    // ========================================
    // 頂点シェーダー
    // ========================================

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    // ファイル名を変更
    HRESULT hr = D3DCompileFromFile(
        L"FlagVS_Standalone.hlsl",  
        nullptr,
        nullptr,  // ← D3D_COMPILE_STANDARD_FILE_INCLUDE 不要
        "main",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vsBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA("[SHADER ERROR - VS]\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile vertex shader");
    }

    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &m_vertexShader
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create vertex shader");
    }

    OutputDebugStringA("[SHADER] Vertex shader compiled\n");

    // 入力レイアウト（変更なし）
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_inputLayout
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create input layout");
    }

    // ========================================
    // ピクセルシェーダー
    // ========================================

    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;

    // ファイル名を変更
    hr = D3DCompileFromFile(
        L"FlagPS_Standalone.hlsl",  // ← ここを変更
        nullptr,
        nullptr,  // ← D3D_COMPILE_STANDARD_FILE_INCLUDE 不要
        "main",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &psBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA("[SHADER ERROR - PS]\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile pixel shader");
    }

    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &m_pixelShader
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create pixel shader");
    }

    OutputDebugStringA("[SHADER] Pixel shader compiled\n");


    //  燻焼用ピクセルシェーダー
    Microsoft::WRL::ComPtr<ID3DBlob> burnPsBlob;

    hr = D3DCompileFromFile(
        L"BurnFlagPS.hlsl",
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &burnPsBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA("[SHADER ERROR - BurnPS]\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile burn pixel shader");
    }

    hr = device->CreatePixelShader(
        burnPsBlob->GetBufferPointer(),
        burnPsBlob->GetBufferSize(),
        nullptr,
        &m_burnPixelShader
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create burn pixel shader");
    }

    OutputDebugStringA("[SHADER] Burn pixel shader compiled\n");


}
void TitleScene::CreateBuffers(ID3D11Device* device)
{
    // === 行列用定数バッファ ===
    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = device->CreateBuffer(&matrixBufferDesc, nullptr, &m_matrixBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create matrix buffer");
    }

    // === 時間用定数バッファ ===
    D3D11_BUFFER_DESC timeBufferDesc = {};
    timeBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    timeBufferDesc.ByteWidth = sizeof(TimeBufferType);
    timeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = device->CreateBuffer(&timeBufferDesc, nullptr, &m_timeBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create time buffer");
    }

    // === 燻焼パラメータ用定数バッファ ===
    D3D11_BUFFER_DESC burnBufferDesc = {};
    burnBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    burnBufferDesc.ByteWidth = sizeof(BurnBufferType);
    burnBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = device->CreateBuffer(&burnBufferDesc, nullptr, &m_burnBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create burn buffer");
    }

    // === メニューFX用定数バッファ ===
    D3D11_BUFFER_DESC menuFxBufferDesc = {};
    menuFxBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    menuFxBufferDesc.ByteWidth = sizeof(MenuFXBufferType);
    menuFxBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = device->CreateBuffer(&menuFxBufferDesc, nullptr, &m_menuFxBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create menu FX buffer");
    }

    // === タイトル背景用定数バッファ ===
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
    // === 画像ファイルから読み込み ===
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        device,
        L"flag_with_logo.png",  // ファイル名
        nullptr,
        &m_flagTexture
    );

    if (FAILED(hr))
    {
        // 画像読み込み失敗時は白いテクスチャを作成
        OutputDebugStringA("[TEXTURE] Failed to load flag_with_logo.png, creating white texture\n");

        // 4×4の白いテクスチャ（フォールバック）
        uint32_t textureData[16];
        for (int i = 0; i < 16; i++)
        {
            textureData[i] = 0xFFFFFFFF;  // 白
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

    // === サンプラーステート作成 ===
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;  // ← WRAP から CLAMP に変更
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;  // ← WRAP から CLAMP に変更
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
    // プロシージャルノイズテクスチャを生成（256x256）
    const int width = 256;
    const int height = 256;

    std::vector<uint8_t> noiseData(width * height * 4);

    // ハッシュ関数（ノイズ生成用）
    auto hash = [](int x, int y) -> float {
        int n = x + y * 57;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
        };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // 複数のオクターブでノイズを重ねる
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

                // 4隅のランダム値
                float v00 = hash(ix, iy);
                float v10 = hash(ix + 1, iy);
                float v01 = hash(ix, iy + 1);
                float v11 = hash(ix + 1, iy + 1);

                // バイリニア補間
                float v0 = v00 + tx * (v10 - v00);
                float v1 = v01 + tx * (v11 - v01);
                float value = v0 + ty * (v1 - v0);

                noise += value * amplitude;
                maxValue += amplitude;

                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            // 0-1に正規化
            noise = (noise / maxValue + 1.0f) * 0.5f;

            // グラデーション（下から上へ燃える）を//
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

    // テクスチャ作成
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

    // SRV作成
    hr = device->CreateShaderResourceView(noiseTex.Get(), nullptr, &m_noiseTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create noise SRV");
    }

    OutputDebugStringA("[TITLE] Noise texture created for burn effect\n");
}

void TitleScene::CreatePostProcessResources(ID3D11Device* device)
{
    // レンダーテクスチャを作成
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_screenWidth;              // 画面幅
    texDesc.Height = m_screenHeight;            // 画面高さ
    texDesc.MipLevels = 1;                      // ミップマップなし
    texDesc.ArraySize = 1;                      // 配列サイズ1
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA 8bit
    texDesc.SampleDesc.Count = 1;               // マルチサンプルなし
    texDesc.Usage = D3D11_USAGE_DEFAULT;        // GPU読み書き
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &m_renderTexture);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create render texture");

    // レンダーターゲットビューを作成 
    hr = device->CreateRenderTargetView(m_renderTexture.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create render target view");

    //  シェーダーリソースビューを作成
    hr = device->CreateShaderResourceView(m_renderTexture.Get(), nullptr, &m_renderTextureSRV);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create shader resource view");

    // === FX用第2レンダーテクスチャ（同じ設定でもう1枚）===
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

    //  フルスクリーンクアッド（四角形）の頂点データ
    struct VertexPos2D
    {
        DirectX::XMFLOAT3 position;  // 位置（X, Y, Z）
        DirectX::XMFLOAT2 texcoord;  // UV座標
    };

    // 画面全体を覆う四角形（2つの三角形）
    VertexPos2D vertices[] =
    {
        // 位置（NDC座標: -1?1）      UV座標（0?1）
        { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },  // 左上
        { DirectX::XMFLOAT3(1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },  // 右上
        { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },  // 左下
        { DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },  // 右下
    };

    // 頂点バッファ作成
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;       // 不変（変更しない）
    vbDesc.ByteWidth = sizeof(vertices);         // サイズ
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // 頂点バッファ
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;                   // データ

    hr = device->CreateBuffer(&vbDesc, &vbData, &m_fullscreenQuadVB);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create fullscreen quad vertex buffer");

    //  サンプラーステート作成
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // 線形補間
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;    // 端はクランプ
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
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    // ========================================
    // 1) ポストプロセス頂点シェーダー
    // ========================================
    hr = D3DCompileFromFile(
        L"PostProcessVS.hlsl", nullptr, nullptr,
        "main", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &vsBlob, &errorBlob
    );
    if (FAILED(hr))
    {
        if (errorBlob) {
            OutputDebugStringA("[SHADER ERROR - PostProcessVS]\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile post-process vertex shader");
    }

    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, &m_postProcessVS
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create post-process vertex shader");

    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        &m_postProcessLayout
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create post-process input layout");

    // ========================================
    // 2) ブラーピクセルシェーダー
    // ========================================
    hr = D3DCompileFromFile(
        L"BlurPS.hlsl", nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &psBlob, &errorBlob
    );
    if (FAILED(hr))
    {
        if (errorBlob) {
            OutputDebugStringA("[SHADER ERROR - BlurPS]\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile blur pixel shader");
    }

    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
        nullptr, &m_blurPS
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create blur pixel shader");

    // ========================================
    // 3) メニューFXピクセルシェーダー（稲妻・ビネット等）
    // ========================================
    Microsoft::WRL::ComPtr<ID3DBlob> menuFxBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> menuFxErrorBlob;

    hr = D3DCompileFromFile(
        L"MenuPostProcessPS.hlsl", nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &menuFxBlob, &menuFxErrorBlob
    );
    if (FAILED(hr))
    {
        if (menuFxErrorBlob) {
            OutputDebugStringA("[SHADER ERROR - MenuFX]\n");
            OutputDebugStringA((char*)menuFxErrorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile menu FX shader");
    }

    hr = device->CreatePixelShader(
        menuFxBlob->GetBufferPointer(), menuFxBlob->GetBufferSize(),
        nullptr, &m_menuFxPS
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create menu FX shader");

    OutputDebugStringA("[SHADER] Menu FX shader compiled\n");

    // ========================================
    // 4) タイトル背景ピクセルシェーダー（うごめく闇）
    // ========================================
    Microsoft::WRL::ComPtr<ID3DBlob> titleBgBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> titleBgErrorBlob;

    hr = D3DCompileFromFile(
        L"TitleBackgroundPS.hlsl", nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &titleBgBlob, &titleBgErrorBlob
    );
    if (FAILED(hr))
    {
        if (titleBgErrorBlob) {
            OutputDebugStringA("[SHADER ERROR - TitleBG]\n");
            OutputDebugStringA((char*)titleBgErrorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile title BG shader");
    }

    hr = device->CreatePixelShader(
        titleBgBlob->GetBufferPointer(), titleBgBlob->GetBufferSize(),
        nullptr, &m_titleBgPS
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create title BG shader");

    OutputDebugStringA("[SHADER] Title background shader compiled\n");
    OutputDebugStringA("[TITLE] All post-process shaders created\n");
}

void TitleScene::RenderToTexture(ID3D11DeviceContext* context)
{
    // ========================================
    // レンダーターゲットをレンダーテクスチャに切り替え
    // ========================================
    context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

    // クリア（黒）
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    // ========================================
    // ビューポート設定
    // ========================================
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // ========================================
    // 燃焼パラメータを更新
    // ========================================
    BurnBufferType burnData;
    burnData.burnProgress = m_burnProgress;
    burnData.time = m_time;
    burnData.emberWidth = m_emberWidth;
    burnData.burnDirection = 0.0f;
    context->UpdateSubresource(m_burnBuffer.Get(), 0, nullptr, &burnData, 0, 0);

    // ========================================
    // シェーダー設定（PostProcessVS + BurnFlagPS）
    // ========================================
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);     // ← 変更！フルスクリーン用VS
    context->PSSetShader(m_burnPixelShader.Get(), nullptr, 0);   // 燃焼PSはそのまま
    context->IASetInputLayout(m_postProcessLayout.Get());         // ← 変更！ポストプロセス用レイアウト

    // 定数バッファ（燃焼パラメータ）
    context->PSSetConstantBuffers(0, 1, m_burnBuffer.GetAddressOf());

    // テクスチャ（旗テクスチャ + ノイズテクスチャ）
    ID3D11ShaderResourceView* textures[2] = { m_flagTexture.Get(), m_noiseTexture.Get() };
    context->PSSetShaderResources(0, 2, textures);
    context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    // ========================================
    // フルスクリーンクアッドを描画
    // ========================================
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);  // position + texcoord
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    // ========================================
    // シェーダーリソースをクリア
    // ========================================
    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRV);
}

// ========================================
// ApplyBlur() - ブラーを適用
// ========================================

void TitleScene::ApplyBlur(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* backBufferRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    // ========================================
    // バックバッファに戻す（重要！）
    // ========================================
    context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

    // ビューポートを設定（画面全体）
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // === ポストプロセス用シェーダーを設定 ===
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_blurPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    // === レンダーテクスチャをシェーダーに渡す ===
    context->PSSetShaderResources(0, 1, m_renderTextureSRV.GetAddressOf());
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    // === フルスクリーンクアッドを描画 ===
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 描画（4頂点）
    context->Draw(4, 0);

    // ===  シェーダーリソースをクリア===
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    //OutputDebugStringA("[BLUR] Applied successfully to back buffer\n");
}

void TitleScene::ApplyBlurTo(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* targetRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    // ApplyBlurと同じだが、出力先を指定できる
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
    // === SpriteBatchを初期化（初回のみ）===
    if (!m_spriteBatch)
    {
        m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);
    }

    // === フォントを読み込み（初回のみ）===
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

    // === 描画設定 ===
    context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

    // === SpriteBatch開始（アルファブレンド有効）===
    m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, nullptr);

    // === 背景のダークグラデーション ===
    if (m_whiteTexture)
    {
        RECT srcRect = { 0, 0, 1, 1 };

        // 暗い赤のビネット（画面全体）
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

        // 上部に濃い赤（血の溜まり）
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

    // === 血の滴りを描画 ===
    RenderBloodDrips();

    // 画面中央より少し下に配置
    float centerX = m_screenWidth / 2.0f;
    float startY = m_screenHeight * 0.55f;
    float itemSpacing = 75.0f;

    {
        const wchar_t* title = L"SLAY WAVE";
        DirectX::XMVECTOR titleSize = m_menuFont->MeasureString(title);
        float titleWidth = DirectX::XMVectorGetX(titleSize);
        float titleX = centerX - (titleWidth * 1.8f) / 2.0f;
        float titleY = m_screenHeight * 0.15f;

        // 赤い影（後ろにずらして描画）
        float titlePulse = sinf(m_menuGlowTime * 2.0f) * 0.5f + 0.5f;
        DirectX::XMVECTORF32 titleShadow = { 0.5f, 0.02f, 0.02f, m_menuAlpha * 0.5f };
        m_menuFont->DrawString(
            m_spriteBatch.get(), title,
            DirectX::XMFLOAT2(titleX + 3.0f, titleY + 3.0f),
            titleShadow, 0.0f, DirectX::XMFLOAT2(0, 0), 1.8f
        );

        // メインタイトル（鮮血の赤、脈動）
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

    // メニュー項目（DOOM風の命名）
    const wchar_t* menuItems[] = {
        L"PLAY",
        L"SETTINGS",
        L"EXIT"
    };

    

    for (int i = 0; i < 3; ++i)
    {
        // 文字列のサイズを取得
        DirectX::XMVECTOR textSize = m_menuFont->MeasureString(menuItems[i]);
        float textWidth = DirectX::XMVectorGetX(textSize);
        float textHeight = DirectX::XMVectorGetY(textSize);

        // 位置計算（中央揃え）
        float posX = centerX - textWidth / 2.0f;
        float posY = startY + i * itemSpacing;

        if (i == m_selectedMenuItem)
        {
            // ============================================
            // 選択中：血のように脈打つ赤い文字
            // ============================================

            // 心臓の鼓動風パルス（速い→遅い→速いの繰り返し）
            float heartbeat = sinf(m_menuGlowTime * 8.0f) * 0.5f + 0.5f;
            float pulse = heartbeat * heartbeat;  // 二乗で鋭いパルスに

            // 文字が微妙に震える（恐怖感）
            float shakeX = sinf(m_menuGlowTime * 25.0f) * pulse * 2.0f;
            float shakeY = cosf(m_menuGlowTime * 30.0f) * pulse * 1.5f;

            // === 背後の血の滲み（大きめに暗い赤で描画）===
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

            // === メインの文字（鮮血の赤）===
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

            // === 左側に血のマーカー（>>>ではなく赤い棒）===
            DirectX::XMVECTORF32 markerColor = { 1.0f, 0.1f, 0.05f, m_menuAlpha * (0.5f + pulse * 0.5f) };

            // 左側の血の線（複数の "|" で太い線に見せる）
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
            // 非選択：乾いた血の色（暗い赤褐色）
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

    // === 画面下部に「PRESS ENTER」の点滅テキスト ===
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
        0.6f  // 小さめのフォントサイズ
    );

    // === SpriteBatch終了 ===
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
// 血の滴り初期化
// ========================================
void TitleScene::InitBloodDrips()
{
    srand(42);  // 固定シードで毎回同じパターン

    for (int i = 0; i < MAX_BLOOD_DRIPS; ++i)
    {
        auto& drip = m_bloodDrips[i];
        drip.x = (float)(rand() % m_screenWidth);
        drip.y = -(float)(rand() % 200);  // 画面上部の外から開始
        drip.speed = 30.0f + (float)(rand() % 80);  // 30?110 px/s
        drip.length = 50.0f + (float)(rand() % 250);  // 50?300 px
        drip.width = 2.0f + (float)(rand() % 6);  // 2?8 px
        drip.alpha = 0.3f + (float)(rand() % 50) / 100.0f;  // 0.3?0.8
        drip.delay = (float)(rand() % 500) / 100.0f;  // 0?5秒の遅延
        drip.active = false;
    }
}

// ========================================
// 血の滴り更新
// ========================================
void TitleScene::UpdateBloodDrips(float deltaTime)
{
    for (int i = 0; i < MAX_BLOOD_DRIPS; ++i)
    {
        auto& drip = m_bloodDrips[i];

        // 遅延チェック
        if (!drip.active)
        {
            drip.delay -= deltaTime;
            if (drip.delay <= 0.0f)
            {
                drip.active = true;
            }
            continue;
        }

        // 落下
        drip.y += drip.speed * deltaTime;

        // 画面下を超えたらリセット
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
// 血の滴り描画
// ========================================
void TitleScene::RenderBloodDrips()
{
    if (!m_spriteBatch || !m_whiteTexture) return;

    for (int i = 0; i < MAX_BLOOD_DRIPS; ++i)
    {
        const auto& drip = m_bloodDrips[i];
        if (!drip.active) continue;

        // === メインの血筋 ===
        // 描画範囲を画面内にクリップ
        float topY = drip.y - drip.length;
        if (topY < 0.0f) topY = 0.0f;
        float bottomY = drip.y;
        if (bottomY > (float)m_screenHeight) bottomY = (float)m_screenHeight;

        float drawLength = bottomY - topY;
        if (drawLength <= 0.0f) continue;

        // 暗い血の色（筋ごとに微妙に色を変える）
        float rBase = 0.3f + (float)(i % 5) * 0.05f;
        float gBase = 0.1f + (float)(i % 4) * 0.03f;
        float bBase = 0.02f;

        // 矩形として描画（RECT指定）
        RECT srcRect = { 0, 0, 1, 1 };

        // メインの血筋
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

        // === 先端の膨らみ（滴の丸み）===
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

        // === 周囲の滲み（薄い赤の影）===
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
    // FXシェーダーが未作成なら通常のブラーにフォールバック
    if (!m_menuFxPS)
    {
        ApplyBlur(context, backBufferRTV, depthStencilView);
        return;
    }

    // ============================================
    // 1) バックバッファに描画先を設定
    // ============================================
    context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // ============================================
    // 2) メニューFXパラメータを更新
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
    // 3) シェーダー設定
    //    ※ m_renderTextureSRV を直接読む
    // ============================================
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_menuFxPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    // 定数バッファ
    context->PSSetConstantBuffers(0, 1, m_menuFxBuffer.GetAddressOf());

    // テクスチャ（メニュー描画済みのFXソース + ノイズ）
    ID3D11ShaderResourceView* textures[2] = { m_fxSourceSRV.Get(), m_noiseTexture.Get() };
    context->PSSetShaderResources(0, 2, textures);
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    // ============================================
    // 4) フルスクリーンクアッド描画
    // ============================================
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    // ============================================
    // 5) シェーダーリソースをクリア
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

    // パラメータ更新
    TitleBGBufferType bgData;
    bgData.time = m_time;
    bgData.screenWidth = (float)m_screenWidth;
    bgData.screenHeight = (float)m_screenHeight;
    bgData.intensity = min(m_time * 0.5f, 1.0f);  // 2秒でフェードイン

    context->UpdateSubresource(m_titleBgBuffer.Get(), 0, nullptr, &bgData, 0, 0);

    // シェーダー設定
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_titleBgPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    context->PSSetConstantBuffers(0, 1, m_titleBgBuffer.GetAddressOf());
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    // フルスクリーンクアッド描画
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);
}

// ========================================
// 画面リサイズ時にレンダーテクスチャを再作成
// ========================================
void TitleScene::OnResize(ID3D11Device* device, int newWidth, int newHeight)
{
    // サイズが同じなら何もしない
    if (newWidth == m_screenWidth && newHeight == m_screenHeight)
        return;

    OutputDebugStringA("[TITLE] OnResize called - recreating render textures\n");

    // 画面サイズを更新
    m_screenWidth = newWidth;
    m_screenHeight = newHeight;

    // === 古いリソースを解放 ===
    m_renderTexture.Reset();
    m_renderTargetView.Reset();
    m_renderTextureSRV.Reset();
    m_fxSourceTexture.Reset();
    m_fxSourceRTV.Reset();
    m_fxSourceSRV.Reset();

    // === 新しいサイズでレンダーテクスチャを再作成 ===
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_screenWidth;
    texDesc.Height = m_screenHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    // --- メインレンダーテクスチャ ---
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &m_renderTexture);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate render texture\n"); return; }

    hr = device->CreateRenderTargetView(m_renderTexture.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate RTV\n"); return; }

    hr = device->CreateShaderResourceView(m_renderTexture.Get(), nullptr, &m_renderTextureSRV);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate SRV\n"); return; }

    // --- FX用レンダーテクスチャ ---
    hr = device->CreateTexture2D(&texDesc, nullptr, &m_fxSourceTexture);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate FX texture\n"); return; }

    hr = device->CreateRenderTargetView(m_fxSourceTexture.Get(), nullptr, &m_fxSourceRTV);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate FX RTV\n"); return; }

    hr = device->CreateShaderResourceView(m_fxSourceTexture.Get(), nullptr, &m_fxSourceSRV);
    if (FAILED(hr)) { OutputDebugStringA("[TITLE] Failed to recreate FX SRV\n"); return; }

    // SpriteBatchをリセット（新しいサイズで再初期化させる）
    m_spriteBatch.reset();

    OutputDebugStringA("[TITLE] Render textures recreated for new size\n");
}