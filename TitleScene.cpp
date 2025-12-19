// ========================================
// TitleScene.cpp - タイトルシーンの実装
// ========================================

#include "TitleScene.h"
#include <d3dcompiler.h>
#include <stdexcept>

#pragma comment(lib, "d3dcompiler.lib")

TitleScene::TitleScene()
    : m_screenWidth(0)
    , m_screenHeight(0)
    , m_time(0.0f)
    , m_waveSpeed(5.0f)
    , m_waveAmplitude(0.8f)
{
}

TitleScene::~TitleScene()
{
}

void TitleScene::Initialize(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    int screenWidth,
    int screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // === 旗メッシュを作成 ===
    m_flagMesh = std::make_unique<FlagMesh>();
    m_flagMesh->Initialize(device, 32, 24);  // 32×24のグリッド

    // === シェーダーを作成 ===
    CreateShaders(device);

    // === 定数バッファを作成 ===
    CreateBuffers(device);

    // === テクスチャを作成 ===
    CreateTexture(device);
}

void TitleScene::Update(float deltaTime)
{
    // 時間を進める
    m_time += deltaTime;
}

void TitleScene::Render(ID3D11DeviceContext* context)
{
    // === 行列を設定 ===
    MatrixBufferType matrixData;

    // ワールド行列（単位行列）
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(2.5f, 1.5f, 1.0f);
    matrixData.world = scale;

    // ビュー行列（カメラは原点から正面を見る）
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    matrixData.view = DirectX::XMMatrixLookAtLH(eye, at, up);

    // プロジェクション行列（透視投影）
    float aspectRatio = (float)m_screenWidth / (float)m_screenHeight;
    matrixData.projection = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(45.0f),
        aspectRatio,
        0.1f,
        100.0f
    );

    // 転置（DirectXMathの行列はシェーダーと異なる形式）
    matrixData.world = DirectX::XMMatrixTranspose(matrixData.world);
    matrixData.view = DirectX::XMMatrixTranspose(matrixData.view);
    matrixData.projection = DirectX::XMMatrixTranspose(matrixData.projection);

    // 定数バッファを更新
    context->UpdateSubresource(m_matrixBuffer.Get(), 0, nullptr, &matrixData, 0, 0);

    // === 時間バッファを設定 ===
    TimeBufferType timeData;
    timeData.time = m_time;
    timeData.waveSpeed = m_waveSpeed;
    timeData.waveAmplitude = m_waveAmplitude;
    timeData.padding = 0.0f;

    context->UpdateSubresource(m_timeBuffer.Get(), 0, nullptr, &timeData, 0, 0);

    // === シェーダーをセット ===
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());

    // === 定数バッファをセット ===
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());
    context->VSSetConstantBuffers(1, 1, m_timeBuffer.GetAddressOf());

    // === テクスチャをセット ===
    context->PSSetShaderResources(0, 1, m_flagTexture.GetAddressOf());
    context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    // === 旗を描画 ===
    m_flagMesh->Draw(context);
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
    // TODO: 実際のテクスチャを読み込む
    // とりあえずダミーテクスチャを作成

    // 4×4の白いテクスチャ
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
    HRESULT hr = device->CreateTexture2D(&texDesc, &initData, &texture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create texture");
    }

    // シェーダーリソースビューを作成
    hr = device->CreateShaderResourceView(texture.Get(), nullptr, &m_flagTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create shader resource view");
    }

    // サンプラーステートを作成
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, &m_samplerState);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create sampler state");
    }
}