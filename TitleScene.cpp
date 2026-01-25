// ========================================
// TitleScene.cpp - タイトルシーンの実装
// ========================================

#include "TitleScene.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include <WICTextureLoader.h> 

#pragma comment(lib, "d3dcompiler.lib")

TitleScene::TitleScene()
    : m_screenWidth(0)
    , m_screenHeight(0)
    , m_time(0.0f)
    , m_waveSpeed(4.0f)
    , m_waveAmplitude(0.6f)
    , m_fireParticleSystem(nullptr)
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

    // === 炎パーティクルシステムを作成（追加）===
    m_fireParticleSystem = std::make_unique<FireParticleSystem>();
    m_fireParticleSystem->Initialize(device, 1000);  // 最大1000パーティクル

    // ベジェ曲線を設定（左下から右上へ）
    m_fireParticleSystem->SetBezierCurve(
        DirectX::XMFLOAT3(-1.2f, -1.0f, 0.0f),  // 開始点（左下）
        DirectX::XMFLOAT3(-0.6f, -0.3f, 0.0f),  // 制御点1
        DirectX::XMFLOAT3(0.0f, 0.3f, 0.0f),  // 制御点2
        DirectX::XMFLOAT3(0.8f, 1.0f, 0.0f)   // 終了点（右上）
    );

    // パーティクル放出を開始
    m_fireParticleSystem->SetEmissionRate(100.0f);  // 50パーティクル/秒
    m_fireParticleSystem->StartEmitting();

    // ========================================
    // ポストプロセス初期化
    // ========================================
    CreatePostProcessResources(device);
    CreatePostProcessShaders(device);

    m_blurStrength = 0.5f;

    OutputDebugStringA("[TITLE] TitleScene initialized with fire particles\n");
}

void TitleScene::Update(float deltaTime)
{
    // 時間を進める
    m_time += deltaTime;

    // == = 炎パーティクルシステムを更新（追加） == =
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
    // === パス1: レンダーテクスチャに3D描画 ===
    RenderToTexture(context);

    // === パス2: ブラーを適用して画面に描画 ===
    ApplyBlur(context, backBufferRTV, depthStencilView);
}

void TitleScene::CreateShaders(ID3D11Device* device)
{
    // ========================================
    // 頂点シェーダー
    // ========================================

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    // ★ ファイル名を変更
    HRESULT hr = D3DCompileFromFile(
        L"FlagVS_Standalone.hlsl",  // ← ここを変更
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

    // ★ ファイル名を変更
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
}

void TitleScene::CreateTexture(ID3D11Device* device)
{
    // === 画像ファイルから読み込み ===
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        device,
        L"flag_with_logo.png",  // ← ファイル名
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

void TitleScene::CreatePostProcessResources(ID3D11Device* device)
{
    // === 1) レンダーテクスチャを作成 ===
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

    // === 2) レンダーターゲットビューを作成 ===
    hr = device->CreateRenderTargetView(m_renderTexture.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create render target view");

    // === 3) シェーダーリソースビューを作成 ===
    hr = device->CreateShaderResourceView(m_renderTexture.Get(), nullptr, &m_renderTextureSRV);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create shader resource view");

    // === 4) フルスクリーンクアッド（四角形）の頂点データ ===
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

    // === 5) サンプラーステート作成 ===
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

    // === 1) 頂点シェーダーをコンパイル ===
    hr = D3DCompileFromFile(
        L"PostProcessVS.hlsl",  // ファイル名
        nullptr,
        nullptr,
        "main",                  // エントリーポイント
        "vs_5_0",                // シェーダーモデル
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vsBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA("[SHADER ERROR - PostProcessVS]\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile post-process vertex shader");
    }

    // 頂点シェーダーを作成
    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &m_postProcessVS
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create post-process vertex shader");

    // === 2) 入力レイアウト作成 ===
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_postProcessLayout
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create post-process input layout");

    // === 3) ピクセルシェーダーをコンパイル ===
    hr = D3DCompileFromFile(
        L"BlurPS.hlsl",  // ファイル名
        nullptr,
        nullptr,
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
            OutputDebugStringA("[SHADER ERROR - BlurPS]\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile blur pixel shader");
    }

    // ピクセルシェーダーを作成
    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &m_blurPS
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create blur pixel shader");

    OutputDebugStringA("[TITLE] Post-process shaders created\n");
}

void TitleScene::RenderToTexture(ID3D11DeviceContext* context)
{
    // === 1) レンダーターゲットをレンダーテクスチャに切り替え ===
    context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

    // === 2) クリア（黒）===
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    // === 3) 既存の3D描画処理（旗と炎）===

    // 行列設定
    MatrixBufferType matrixData;
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(1.8f, 1.2f, 1.0f);
    matrixData.world = scale;

    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    matrixData.view = DirectX::XMMatrixLookAtLH(eye, at, up);

    float aspectRatio = (float)m_screenWidth / (float)m_screenHeight;
    matrixData.projection = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(45.0f),
        aspectRatio,
        0.1f,
        100.0f
    );

    matrixData.world = DirectX::XMMatrixTranspose(matrixData.world);
    matrixData.view = DirectX::XMMatrixTranspose(matrixData.view);
    matrixData.projection = DirectX::XMMatrixTranspose(matrixData.projection);

    context->UpdateSubresource(m_matrixBuffer.Get(), 0, nullptr, &matrixData, 0, 0);

    // 時間バッファ
    TimeBufferType timeData;
    timeData.time = m_time;
    timeData.waveSpeed = m_waveSpeed;
    timeData.waveAmplitude = m_waveAmplitude;
    timeData.padding = 0.0f;
    context->UpdateSubresource(m_timeBuffer.Get(), 0, nullptr, &timeData, 0, 0);

    // シェーダー設定
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    context->VSSetConstantBuffers(1, 1, m_timeBuffer.GetAddressOf());
    context->PSSetShaderResources(0, 1, m_flagTexture.GetAddressOf());
    context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    // 旗を描画
    if (m_flagMesh)
    {
        m_flagMesh->Draw(context);
    }

    // 炎パーティクルを描画
    if (m_fireParticleSystem)
    {
        DirectX::XMMATRIX viewForParticles = DirectX::XMMatrixTranspose(matrixData.view);
        DirectX::XMMATRIX projectionForParticles = DirectX::XMMatrixTranspose(matrixData.projection);
        m_fireParticleSystem->Render(context, viewForParticles, projectionForParticles);
    }

    OutputDebugStringA("[RENDER] Rendered to texture\n");
}

// ========================================
// ApplyBlur() - ブラーを適用（修正版）
// ========================================

void TitleScene::ApplyBlur(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* backBufferRTV,
    ID3D11DepthStencilView* depthStencilView)
{
    // ========================================
    // 1) バックバッファに戻す（重要！）
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

    // === 2) ポストプロセス用シェーダーを設定 ===
    context->VSSetShader(m_postProcessVS.Get(), nullptr, 0);
    context->PSSetShader(m_blurPS.Get(), nullptr, 0);
    context->IASetInputLayout(m_postProcessLayout.Get());

    // === 3) レンダーテクスチャをシェーダーに渡す ===
    context->PSSetShaderResources(0, 1, m_renderTextureSRV.GetAddressOf());
    context->PSSetSamplers(0, 1, m_postProcessSampler.GetAddressOf());

    // === 4) フルスクリーンクアッドを描画 ===
    UINT stride = sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_fullscreenQuadVB.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 描画（4頂点）
    context->Draw(4, 0);

    // === 5) シェーダーリソースをクリア（重要！）===
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    OutputDebugStringA("[BLUR] Applied successfully to back buffer\n");
}