#include "FireParticleSystem.h"
#include <d3dcompiler.h>
#include <random>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// ========================================
// ランダム生成器
// ========================================

static std::random_device rd;
static std::mt19937 gen(rd());

static float RandomFloat(float min, float max)
{
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

// ========================================
// コンストラクタ / デストラクタ
// ========================================

FireParticleSystem::FireParticleSystem()
    : m_maxParticles(1000)
    , m_isEmitting(false)
    , m_emissionRate(100.0f)
    , m_emissionAccumulator(0.0f)
{
}

FireParticleSystem::~FireParticleSystem()
{
}

// ========================================
// 初期化
// ========================================

void FireParticleSystem::Initialize(ID3D11Device* device, int maxParticles)
{
    m_maxParticles = maxParticles;
    m_particles.reserve(m_maxParticles);

    // デフォルトのベジェ曲線（左下から右上）
    SetBezierCurve(
        XMFLOAT3(-1.0f, -1.0f, 0.0f),
        XMFLOAT3(-0.5f, 0.0f, 0.0f),
        XMFLOAT3(0.5f, 0.5f, 0.0f),
        XMFLOAT3(1.0f, 1.0f, 0.0f)
    );

    CreateBuffers(device);
    CreateShaders(device);
    CreateTexture(device);
    CreateBlendState(device);
}

// ========================================
// ベジェ曲線設定
// ========================================

void FireParticleSystem::SetBezierCurve(
    XMFLOAT3 p0,
    XMFLOAT3 p1,
    XMFLOAT3 p2,
    XMFLOAT3 p3)
{
    m_bezierCurve.SetControlPoints(p0, p1, p2, p3);
}

// ========================================
// 放出制御
// ========================================

void FireParticleSystem::StartEmitting()
{
    m_isEmitting = true;
}

void FireParticleSystem::StopEmitting()
{
    m_isEmitting = false;
}

void FireParticleSystem::SetEmissionRate(float particlesPerSecond)
{
    m_emissionRate = particlesPerSecond;
}

// ========================================
// パーティクル生成
// ========================================

void FireParticleSystem::EmitParticle()
{
    if (m_particles.size() >= m_maxParticles)
        return;

    FireParticle particle;

    particle.curveT = 0.0f;
    particle.position = m_bezierCurve.GetPosition(0.0f);

    XMFLOAT3 tangent = m_bezierCurve.GetTangent(0.0f);
    float speed = RandomFloat(0.5f, 1.0f);
    particle.velocity = XMFLOAT3(
        tangent.x * speed,
        tangent.y * speed,
        tangent.z * speed
    );

    float r = RandomFloat(0.8f, 1.0f);
    float g = RandomFloat(0.3f, 0.8f);
    float b = RandomFloat(0.0f, 0.2f);
    float a = RandomFloat(0.7f, 1.0f);
    particle.color = XMFLOAT4(r, g, b, a);

    particle.size = RandomFloat(0.1f, 0.3f);
    particle.lifetime = RandomFloat(2.0f, 3.0f);
    particle.age = 0.0f;
    particle.active = true;

    m_particles.push_back(particle);
}

// ========================================
// 更新
// ========================================

void FireParticleSystem::Update(float deltaTime)
{
    if (m_isEmitting)
    {
        m_emissionAccumulator += deltaTime * m_emissionRate;

        while (m_emissionAccumulator >= 1.0f)
        {
            EmitParticle();
            m_emissionAccumulator -= 1.0f;
        }
    }

    for (auto it = m_particles.begin(); it != m_particles.end();)
    {
        if (it->active)
        {
            UpdateParticle(*it, deltaTime);

            if (it->age >= it->lifetime)
            {
                it->active = false;
            }
        }

        if (!it->active)
        {
            it = m_particles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void FireParticleSystem::UpdateParticle(FireParticle& particle, float deltaTime)
{
    particle.age += deltaTime;

    particle.curveT += deltaTime * 0.3f;
    if (particle.curveT > 1.0f)
        particle.curveT = 1.0f;

    particle.position = m_bezierCurve.GetPosition(particle.curveT);

    particle.position.x += RandomFloat(-0.02f, 0.02f);
    particle.position.y += RandomFloat(-0.01f, 0.03f);

    particle.size += deltaTime * 0.05f;

    float lifeRatio = particle.age / particle.lifetime;
    particle.color.w = 1.0f - lifeRatio;
    particle.color.y = (1.0f - lifeRatio) * 0.6f;
    particle.color.x = 1.0f - lifeRatio * 0.3f;
}

// ========================================
// バッファ作成
// ========================================

void FireParticleSystem::CreateBuffers(ID3D11Device* device)
{
    struct ParticleVertex
    {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
        XMFLOAT4 color;
        float size;
    };

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(ParticleVertex) * 4 * m_maxParticles;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device->CreateBuffer(&vertexBufferDesc, nullptr, &m_vertexBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create particle vertex buffer");
    }

    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    matrixBufferDesc.ByteWidth = sizeof(XMMATRIX) * 2;
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = device->CreateBuffer(&matrixBufferDesc, nullptr, &m_matrixBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create matrix buffer");
    }

    //OutputDebugStringA("[FIRE] Buffers created\n");
}

// ========================================
// シェーダー作成
// ========================================
void FireParticleSystem::CreateShaders(ID3D11Device* device)
{

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;  // .cso バイナリの入れ物

    // ========================================
    // 頂点シェーダー（FireParticleVS）
    // 【役割】各パーティクル頂点の位置をビルボード展開する
    // ========================================
    hr = D3DReadFileToBlob(L"Assets/Shaders/FireParticleVS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[FIRE] FireParticleVS.cso load FAILED\n");
        throw std::runtime_error("Failed to load FireParticleVS.cso");
    }

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_vertexShader
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create FireParticle VS");

    // ========================================
    // 入力レイアウト（頂点データの構造を GPU に教える）
    // ========================================
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "PSIZE",    0, DXGI_FORMAT_R32_FLOAT,           0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        &m_inputLayout
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create fire particle input layout");

    OutputDebugStringA("[FIRE] FireParticleVS loaded from CSO\n");

    // ========================================
    // ピクセルシェーダー（FireParticlePS）
    // 【役割】各パーティクルの色とアルファを計算する
    // ========================================
    blob.Reset();  // 前の blob を解放してから再利用
    hr = D3DReadFileToBlob(L"Assets/Shaders/FireParticlePS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[FIRE] FireParticlePS.cso load FAILED\n");
        throw std::runtime_error("Failed to load FireParticlePS.cso");
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &m_pixelShader
    );
    if (FAILED(hr))
        throw std::runtime_error("Failed to create FireParticle PS");

    OutputDebugStringA("[FIRE] FireParticlePS loaded from CSO\n");
}

void FireParticleSystem::CreateTexture(ID3D11Device* device)
{
    const int SIZE = 64;
    uint32_t textureData[SIZE * SIZE];

    for (int y = 0; y < SIZE; y++)
    {
        for (int x = 0; x < SIZE; x++)
        {
            float fx = (x - SIZE / 2.0f) / (SIZE / 2.0f);
            float fy = (y - SIZE / 2.0f) / (SIZE / 2.0f);
            float dist = sqrtf(fx * fx + fy * fy);

            float alpha = 1.0f - min(dist, 1.0f);
            alpha = alpha * alpha;

            uint8_t a = (uint8_t)(alpha * 255);
            textureData[y * SIZE + x] = (a << 24) | 0x00FFFFFF;
        }
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = SIZE;
    texDesc.Height = SIZE;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = textureData;
    initData.SysMemPitch = SIZE * sizeof(uint32_t);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device->CreateTexture2D(&texDesc, &initData, &texture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create fire texture");
    }

    hr = device->CreateShaderResourceView(texture.Get(), nullptr, &m_texture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create SRV");
    }

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, &m_samplerState);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create sampler state");
    }

    //OutputDebugStringA("[FIRE] Texture created\n");
}

// ========================================
// ブレンドステート作成
// ========================================

void FireParticleSystem::CreateBlendState(ID3D11Device* device)
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create blend state");
    }

    //OutputDebugStringA("[FIRE] Blend state created\n");
}

// ========================================
// 描画
// ========================================

void FireParticleSystem::Render(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection)
{
    if (m_particles.empty())
        return;

    struct ParticleVertex
    {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
        XMFLOAT4 color;
        float size;
    };

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr))
    {
        //OutputDebugStringA("[FIRE] Failed to map vertex buffer\n");
        return;
    }

    ParticleVertex* vertices = (ParticleVertex*)mappedResource.pData;
    int vertexIndex = 0;

    for (const auto& particle : m_particles)
    {
        if (!particle.active)
            continue;

        vertices[vertexIndex].position = particle.position;
        vertices[vertexIndex].texCoord = XMFLOAT2(0.0f, 1.0f);
        vertices[vertexIndex].color = particle.color;
        vertices[vertexIndex].size = particle.size;
        vertexIndex++;

        vertices[vertexIndex].position = particle.position;
        vertices[vertexIndex].texCoord = XMFLOAT2(1.0f, 1.0f);
        vertices[vertexIndex].color = particle.color;
        vertices[vertexIndex].size = particle.size;
        vertexIndex++;

        vertices[vertexIndex].position = particle.position;
        vertices[vertexIndex].texCoord = XMFLOAT2(0.0f, 0.0f);
        vertices[vertexIndex].color = particle.color;
        vertices[vertexIndex].size = particle.size;
        vertexIndex++;

        vertices[vertexIndex].position = particle.position;
        vertices[vertexIndex].texCoord = XMFLOAT2(1.0f, 0.0f);
        vertices[vertexIndex].color = particle.color;
        vertices[vertexIndex].size = particle.size;
        vertexIndex++;
    }

    context->Unmap(m_vertexBuffer.Get(), 0);

    if (vertexIndex == 0)
        return;

    struct MatrixBufferType
    {
        XMMATRIX view;
        XMMATRIX projection;
    };

    hr = context->Map(m_matrixBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        MatrixBufferType* matrixData = (MatrixBufferType*)mappedResource.pData;
        matrixData->view = XMMatrixTranspose(view);
        matrixData->projection = XMMatrixTranspose(projection);
        context->Unmap(m_matrixBuffer.Get(), 0);
    }

    context->IASetInputLayout(m_inputLayout.Get());

    UINT stride = sizeof(ParticleVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOf());

    context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);

    int particleCount = vertexIndex / 4;
    for (int i = 0; i < particleCount; i++)
    {
        context->Draw(4, i * 4);
    }

    context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
}