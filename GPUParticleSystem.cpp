// ============================================================
//  GPUParticleSystem.cpp
//  GPU Compute Shaderによる血パーティクルシステム
// ============================================================

#include "GPUParticleSystem.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <WICTextureLoader.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// ============================================================
//  コンストラクタ
// ============================================================
GPUParticleSystem::GPUParticleSystem()
{
    m_emitQueue.reserve(512);  // 発射キューの初期容量
}

// ============================================================
//  初期化
// ============================================================
bool GPUParticleSystem::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
    int maxParticles, int screenWidth, int screenHeight)
{
    m_device = device;
    m_context = context;
    m_maxParticles = maxParticles;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // シェーダーコンパイル
    if (!CompileShaders())
    {
        OutputDebugStringA("[GPUParticle] Shader compile FAILED!\n");
        return false;
    }

    
    // GPUバッファ作成
    if (!CreateBuffers())
    {
        OutputDebugStringA("[GPUParticle] Buffer creation FAILED!\n");
        return false;
    }

    // ブレンド/深度ステート作成
    if (!CreateStates())
    {
        OutputDebugStringA("[GPUParticle] State creation FAILED!\n");
        return false;
    }

    char msg[128];
    sprintf_s(msg, "[GPUParticle] Initialized OK! Max particles: %d\n", m_maxParticles);
    OutputDebugStringA(msg);

    //  霧テクスチャ読み込み
    {
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_device,
            L"Assets/Texture/blood_mist_v2.png",
            nullptr,
            m_bloodMistSRV.GetAddressOf()
        );
        if (SUCCEEDED(hr))
            OutputDebugStringA("[GPUParticle] Blood MIST texture loaded!\n");
        else
            OutputDebugStringA("[GPUParticle] WARNING: mist texture NOT found!\n");
    }

    //  液体スプラッシュテクスチャ読み込み
    {
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_device,
            L"Assets/Texture/blood_splash_v2.png",
            nullptr,
            m_bloodSplashSRV.GetAddressOf()
        );
        if (SUCCEEDED(hr))
            OutputDebugStringA("[GPUParticle] Blood SPLASH texture loaded!\n");
        else
            OutputDebugStringA("[GPUParticle] WARNING: splash texture NOT found!\n");
    }

    //  リニアサンプラー作成
   // 【何をしている？】
   // テクスチャからピクセルを読むときの「補間ルール」を定義する。
   //
   // Filter = LINEAR → 隣接ピクセルを滑らかに補間（ぼかし効果）
   //          POINT にすると補間なし（ドット絵風、カクカク）
   //
   // AddressU/V = CLAMP → テクスチャの端をはみ出したら端の色を使う
   //              WRAP にすると繰り返し（タイル状）
    {
        D3D11_SAMPLER_DESC sd = {};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // 滑らか補間
        sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;      // はみ出し防止
        sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;

        HRESULT hr = m_device->CreateSamplerState(&sd, m_linearSampler.GetAddressOf());
        if (SUCCEEDED(hr))
            OutputDebugStringA("[GPUParticle] Linear sampler created!\n");
    }



    // // 流体レンダリング用リソース(失敗しても通常描画にフォールバック)
    if (CompileFluidShaders())
    {
        CreateFluidResources();
        OutputDebugStringA("[GPUParticle] Fluid rendering READY!\n");
    }
    else
    {
        OutputDebugStringA("[GPUParticle] Fluid shaders failed - billboard fallback\n");
    }

    return true;
}

// ============================================================
//  シェーダーコンパイル(CS, VS, PS の3つ)
// ============================================================
bool GPUParticleSystem::CompileShaders()
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> errBlob;

    // === Compute Shader(物理演算) ===
    hr = D3DCompileFromFile(
        L"Assets/Shaders/BloodParticleCS.hlsl",
        nullptr, nullptr, "main", "cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[GPUParticle] CS compile failed\n");
        return false;
    }
    hr = m_device->CreateComputeShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_computeShader.GetAddressOf());
    if (FAILED(hr)) return false;
    OutputDebugStringA("[GPUParticle] CS compiled OK\n");

    // === Vertex Shader(ビルボード展開) ===
    blob.Reset(); errBlob.Reset();
    hr = D3DCompileFromFile(
        L"Assets/Shaders/BloodParticleVS.hlsl",
        nullptr, nullptr, "main", "vs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[GPUParticle] VS compile failed\n");
        return false;
    }
    hr = m_device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_vertexShader.GetAddressOf());
    if (FAILED(hr)) return false;
    OutputDebugStringA("[GPUParticle] VS compiled OK\n");

    // === Pixel Shader(血の描画) ===
    blob.Reset(); errBlob.Reset();
    hr = D3DCompileFromFile(
        L"Assets/Shaders/BloodParticlePS.hlsl",
        nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[GPUParticle] PS compile failed\n");
        return false;
    }
    hr = m_device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_pixelShader.GetAddressOf());
    if (FAILED(hr)) return false;
    OutputDebugStringA("[GPUParticle] PS compiled OK\n");

    return true;
}

// ============================================================
//  GPUバッファ作成
// ============================================================
bool GPUParticleSystem::CreateBuffers()
{
    HRESULT hr;

    // === パーティクル StructuredBuffer(GPU読み書き) ===
    {
        // 全パーティクルを死亡状態(life=0)で初期化
        std::vector<Particle> initData(m_maxParticles);
        memset(initData.data(), 0, sizeof(Particle) * m_maxParticles);

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Particle) * m_maxParticles;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(Particle);  // 1要素のサイズ

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = initData.data();

        hr = m_device->CreateBuffer(&desc, &init, m_particleBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // UAV(Compute Shader用: 読み書き可能)
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = m_maxParticles;

        hr = m_device->CreateUnorderedAccessView(
            m_particleBuffer.Get(), &uavDesc, m_particleUAV.GetAddressOf());
        if (FAILED(hr)) return false;

        // SRV(Vertex Shader用: 読み取り専用)
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements = m_maxParticles;

        hr = m_device->CreateShaderResourceView(
            m_particleBuffer.Get(), &srvDesc, m_particleSRV.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === ステージングバッファ(CPU->GPUアップロード用) ===
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Particle) * m_maxParticles;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

        hr = m_device->CreateBuffer(&desc, nullptr, m_stagingBuffer.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === インデックスバッファ(1クワッド = 6インデックス = 三角形2つ) ===
    {
        std::vector<uint32_t> indices(m_maxParticles * 6);
        for (int i = 0; i < m_maxParticles; i++)
        {
            uint32_t base = i * 4;  // 4頂点ごと
            indices[i * 6 + 0] = base + 0;  // 三角形1: 左上-右上-左下
            indices[i * 6 + 1] = base + 1;
            indices[i * 6 + 2] = base + 2;
            indices[i * 6 + 3] = base + 1;  // 三角形2: 右上-右下-左下
            indices[i * 6 + 4] = base + 3;
            indices[i * 6 + 5] = base + 2;
        }

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(sizeof(uint32_t) * indices.size());
        desc.Usage = D3D11_USAGE_IMMUTABLE;  // 一度作ったら変更しない
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = indices.data();

        hr = m_device->CreateBuffer(&desc, &init, m_indexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === Compute Shader用の定数バッファ ===
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(UpdateCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;      // 毎フレーム書き換え
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&desc, nullptr, m_updateCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === カメラ用の定数バッファ ===
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(CameraCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&desc, nullptr, m_cameraCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    return true;
}

// ============================================================
//  ブレンド/深度ステート作成
// ============================================================
bool GPUParticleSystem::CreateStates()
{
    HRESULT hr;

    // === アルファブレンド(半透明の血) ===
    {
        D3D11_BLEND_DESC desc = {};
        desc.AlphaToCoverageEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;       // ソースのアルファ
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   // 1-ソースのアルファ
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;          // 加算
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = m_device->CreateBlendState(&desc, m_blendState.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === 深度: 読み取り専用(パーティクルは半透明なので深度を書かない) ===
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // 書き込み無効
        desc.DepthFunc = D3D11_COMPARISON_LESS;

        hr = m_device->CreateDepthStencilState(&desc, m_depthState.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    return true;
}

// ============================================================
//  パーティクル発射(CPU側 - 次のUpdateでGPUにアップロード)
// ============================================================
void GPUParticleSystem::Emit(XMFLOAT3 position, int count, float power, float sizeMin, float sizeMax)
{
    for (int i = 0; i < count; i++)
    {
        Particle p = {};
        p.position = position;

        // ランダムな放射方向(上半球)
        float theta = ((float)rand() / RAND_MAX) * 6.28318f;          // 水平角(0~360度)
        float phi = ((float)rand() / RAND_MAX) * 3.14159f * 0.5f;   // 仰角(0~90度)
        float speed = power * (0.5f + ((float)rand() / RAND_MAX) * 1.0f);

        p.velocity.x = cosf(theta) * sinf(phi) * speed * 1.5f;
        p.velocity.y = cosf(phi) * speed * 1.2f + 3.0f;   // 上に強く飛ばす
        p.velocity.z = sinf(theta) * sinf(phi) * speed * 1.5f;

        p.life = 1.5f + ((float)rand() / RAND_MAX) * 2.5f;   // 1.5~4秒
        p.maxLife = p.life;
        p.size = sizeMin + ((float)rand() / RAND_MAX) * (sizeMax - sizeMin);

        m_emitQueue.push_back(p);
    }
}

// ============================================================
//  EmitSplash: 血の飛沫（細かい飛び散り）
//
//  【イメージ】
//  水風船が割れた瞬間のパシャッ！
//  細かい飛沫が弾の方向にバーッと弾ける。
//
//  【パラメータ設計】
//  サイズ: 0.03~0.12（とても小さい → 細かい飛沫感）
//  数:     30~60個（中量 → 密度感）
//  速度:   速い（power × 1.5~3.0 → パッと弾ける）
//  寿命:   0.3~1.0秒（短い → パッと出てサッと消える）
//  方向:   弾の進行方向に偏る + ランダム散らし
// ============================================================
void GPUParticleSystem::EmitSplash(XMFLOAT3 position, XMFLOAT3 direction, int count, float power)
{
    // 方向ベクトルを正規化
    float len = sqrtf(direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z);
    if (len < 0.001f)
    {
        direction = XMFLOAT3(0.0f, 1.0f, 0.0f);
        len = 1.0f;
    }
    float inv = 1.0f / len;
    direction.x *= inv;
    direction.y *= inv;
    direction.z *= inv;

    for (int i = 0; i < count; i++)
    {
        Particle p = {};
        p.position = position;

        // ランダム散らし（-1~+1）
        float sx = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float sy = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float sz = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        // 速度: 速めでバラつきあり（power × 1.5~3.0）
        float speed = power * (1.5f + ((float)rand() / RAND_MAX) * 1.5f);

        // メイン方向 + 大きめの散らし → 弾ける感じ
        p.velocity.x = direction.x * speed + sx * power * 1.5f;
        p.velocity.y = direction.y * speed + sy * power * 1.0f + 2.0f;
        p.velocity.z = direction.z * speed + sz * power * 1.5f;

        // 寿命: 短い（0.3~1.0秒）→ パッと消える
        p.life = 0.5f + ((float)rand() / RAND_MAX) * 1.0f;
        p.maxLife = p.life;

        // サイズ: 小さい（0.03~0.12）→ 細かい飛沫
        p.size = 0.001f + ((float)rand() / RAND_MAX) * 0.17f;

        m_emitQueue.push_back(p);
    }
}

// ============================================================
//  EmitMist: 血の霧（ふわっと広がる赤い靄）
//
//  【イメージ】
//  赤いスプレーをシュッと吹いた感じ。
//  細かい粒子が全方向にふわ?っと広がって漂う。
//
//  【パラメータ設計】
//  サイズ: 0.02~0.08（極小 → 霧っぽい）
//  数:     80~200個（大量 → 密度で霧感を出す）
//  速度:   遅い（power × 0.3~1.0 → ゆっくり広がる）
//  寿命:   1.0~3.0秒（長め → じわ?っと消える）
//  方向:   全方向均等（球面ランダム）
// ============================================================
void GPUParticleSystem::EmitMist(XMFLOAT3 position, int count, float power)
{
    for (int i = 0; i < count; i++)
    {
        Particle p = {};
        p.position = position;

        // --- 球面上の均等ランダム方向 ---
        // theta: 0~2π（水平角）
        // phi:   acos(2r-1)（仰角、均等分布にするためのテクニック）
        float theta = ((float)rand() / RAND_MAX) * 6.2832f;
        float u = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float phi = acosf(u);

        // 速度: 遅め（power × 0.3~1.0）→ ふわっと
        float speed = power * (0.5f + ((float)rand() / RAND_MAX) * 1.0f);

        p.velocity.x = cosf(theta) * sinf(phi) * speed;
        p.velocity.y = cosf(phi) * speed + 0.5f;   // 微妙に上昇（煙は上に行く）
        p.velocity.z = sinf(theta) * sinf(phi) * speed;

        // 寿命: 長め（1.0~3.0秒）→ じわ?っと
        p.life = 1.0f + ((float)rand() / RAND_MAX) * 2.0f;
        p.maxLife = p.life;

        // サイズ: 極小（0.02~0.08）→ 霧の粒
        p.size = 0.04f + ((float)rand() / RAND_MAX) * 0.08f;

        m_emitQueue.push_back(p);
    }
}

// ============================================================
//  発射キューをGPUにアップロード(リングバッファ方式)
// ============================================================
void GPUParticleSystem::UploadEmitQueue()
{
    if (m_emitQueue.empty()) return;

    int count = (int)m_emitQueue.size();
    if (count > m_maxParticles) count = m_maxParticles;

    // === リングバッファの連続領域ごとにUpdateSubresource ===
    // GPU→CPU往復コピー不要！直接GPUバッファに書き込む
    int emitIdx = 0;

    // チャンク1: m_nextSlot ～ バッファ末尾
    int firstChunk = (std::min)(count, m_maxParticles - m_nextSlot);
    if (firstChunk > 0)
    {
        D3D11_BOX box = {};
        box.left = m_nextSlot * sizeof(Particle);   // バイトオフセット開始
        box.right = box.left + firstChunk * sizeof(Particle); // バイトオフセット終了
        box.top = 0; box.bottom = 1;
        box.front = 0; box.back = 1;

        m_context->UpdateSubresource(
            m_particleBuffer.Get(), 0, &box,
            &m_emitQueue[emitIdx], sizeof(Particle), 0);
        emitIdx += firstChunk;
    }

    // チャンク2: バッファ先頭に折り返し（リングバッファ）
    int secondChunk = count - firstChunk;
    if (secondChunk > 0)
    {
        D3D11_BOX box = {};
        box.left = 0;
        box.right = secondChunk * sizeof(Particle);
        box.top = 0; box.bottom = 1;
        box.front = 0; box.back = 1;

        m_context->UpdateSubresource(
            m_particleBuffer.Get(), 0, &box,
            &m_emitQueue[emitIdx], sizeof(Particle), 0);
    }

    m_nextSlot = (m_nextSlot + count) % m_maxParticles;
    m_emitQueue.clear();
    m_activeCount = (std::min)(m_activeCount + count, m_maxParticles);
}

// ============================================================
//  更新(GPU Compute Shaderを実行)
// ============================================================
void GPUParticleSystem::Update(float deltaTime)
{
    m_totalTime += deltaTime;

    // 溜まっているパーティクルをGPUにアップロード
    UploadEmitQueue();

    // === 定数バッファを更新 ===
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_context->Map(m_updateCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            UpdateCBData* cb = (UpdateCBData*)mapped.pData;
            cb->DeltaTime = deltaTime;
            cb->Gravity = -6.0f;     // 強めの重力(血は重い)
            cb->Drag = 0.975f;     // 軽い空気抵抗
            cb->FloorY = 0.02f;      // 床のちょっと上
            cb->BounceFactor = 0.35f;       // 血はあまりバウンスしない
            cb->Time = m_totalTime;
            cb->Padding[0] = 0.0f;
            cb->Padding[1] = 0.0f;
            m_context->Unmap(m_updateCB.Get(), 0);
        }
    }

    // === Compute Shaderを実行 ===
    m_context->CSSetShader(m_computeShader.Get(), nullptr, 0);
    m_context->CSSetConstantBuffers(0, 1, m_updateCB.GetAddressOf());

    // UAVをバインド(Compute Shaderが読み書きするバッファ)
    ID3D11UnorderedAccessView* uavs[] = { m_particleUAV.Get() };
    UINT initCounts[] = { (UINT)-1 };
    m_context->CSSetUnorderedAccessViews(0, 1, uavs, initCounts);

    // ディスパッチ: ceil(最大パーティクル数 / 256) グループ
    UINT groups = (m_maxParticles + 255) / 256;
    m_context->Dispatch(groups, 1, 1);

    // === UAVをアンバインド(SRVとして使う前に必須) ===
    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    m_context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    m_context->CSSetShader(nullptr, nullptr, 0);
}

// ============================================================

// ============================================================
//  ステート保存(描画前のパイプライン状態を記憶)
// ============================================================
void GPUParticleSystem::SaveState(SavedState& s)
{
    m_context->IAGetInputLayout(s.IL.GetAddressOf());
    m_context->IAGetPrimitiveTopology(&s.Topology);
    s.IBFormat = DXGI_FORMAT_UNKNOWN; s.IBOffset = 0;
    m_context->IAGetIndexBuffer(s.IB.GetAddressOf(), &s.IBFormat, &s.IBOffset);
    s.VBStride = 0; s.VBOffset = 0;
    m_context->IAGetVertexBuffers(0, 1, s.VB.GetAddressOf(), &s.VBStride, &s.VBOffset);

    m_context->VSGetShader(s.VS.GetAddressOf(), nullptr, nullptr);
    m_context->VSGetShaderResources(0, 1, s.VS_SRV.GetAddressOf());
    m_context->VSGetConstantBuffers(0, 1, s.VS_CB.GetAddressOf());

    m_context->PSGetShader(s.PS.GetAddressOf(), nullptr, nullptr);
    m_context->PSGetShaderResources(0, 1, s.PS_SRV0.GetAddressOf());
    m_context->PSGetShaderResources(1, 1, s.PS_SRV1.GetAddressOf());
    m_context->PSGetConstantBuffers(0, 1, s.PS_CB.GetAddressOf());
    m_context->PSGetSamplers(0, 1, s.PS_Sampler.GetAddressOf());

    m_context->OMGetBlendState(s.Blend.GetAddressOf(), s.BlendFactor, &s.SampleMask);
    m_context->OMGetDepthStencilState(s.Depth.GetAddressOf(), &s.StencilRef);
    m_context->OMGetRenderTargets(1, s.RTV.GetAddressOf(), s.DSV.GetAddressOf());

    m_context->RSGetState(s.RS.GetAddressOf());
}

// ============================================================
//  ステート復元(描画前の状態に完全に戻す)
// ============================================================
void GPUParticleSystem::RestoreState(const SavedState& s)
{
    // まずSRVをクリア(競合防止)
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    m_context->VSSetShaderResources(0, 1, nullSRV);
    m_context->PSSetShaderResources(0, 1, nullSRV);
    m_context->PSSetShaderResources(1, 1, nullSRV);

    m_context->IASetInputLayout(s.IL.Get());
    m_context->IASetPrimitiveTopology(s.Topology);
    m_context->IASetIndexBuffer(s.IB.Get(), s.IBFormat, s.IBOffset);
    UINT stride = s.VBStride, offset = s.VBOffset;
    ID3D11Buffer* vb = s.VB.Get();
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    m_context->VSSetShader(s.VS.Get(), nullptr, 0);
    m_context->VSSetShaderResources(0, 1, s.VS_SRV.GetAddressOf());
    m_context->VSSetConstantBuffers(0, 1, s.VS_CB.GetAddressOf());

    m_context->PSSetShader(s.PS.Get(), nullptr, 0);
    ID3D11ShaderResourceView* srvs[2] = { s.PS_SRV0.Get(), s.PS_SRV1.Get() };
    m_context->PSSetShaderResources(0, 2, srvs);
    m_context->PSSetConstantBuffers(0, 1, s.PS_CB.GetAddressOf());
    m_context->PSSetSamplers(0, 1, s.PS_Sampler.GetAddressOf());

    m_context->OMSetBlendState(s.Blend.Get(), s.BlendFactor, s.SampleMask);
    m_context->OMSetDepthStencilState(s.Depth.Get(), s.StencilRef);
    ID3D11RenderTargetView* rtv = s.RTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, s.DSV.Get());

    m_context->RSSetState(s.RS.Get());
}

// ============================================================
//  通常描画(ビルボード方式 - ステート保存/復元付き)
// ============================================================
void GPUParticleSystem::Draw(XMMATRIX view, XMMATRIX proj, XMFLOAT3 cameraPos)
{
    // // 流体モードがONなら通常描画はスキップ(DrawFluidを使う)
    // ここでは従来のビルボード描画(フォールバック用)

    SavedState saved;
    SaveState(saved);

    // ビュー逆行列からカメラの右/上ベクトル取得
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMFLOAT4X4 ivF;
    XMStoreFloat4x4(&ivF, invView);
    XMFLOAT3 camRight(ivF._11, ivF._12, ivF._13);
    XMFLOAT3 camUp(ivF._21, ivF._22, ivF._23);

    // カメラ定数バッファ更新
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_cameraCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CameraCBData* cb = (CameraCBData*)mapped.pData;
            XMStoreFloat4x4(&cb->View, XMMatrixTranspose(view));
            XMStoreFloat4x4(&cb->Projection, XMMatrixTranspose(proj));
            cb->CameraRight = camRight;
            cb->Time = m_totalTime;
            cb->CameraUp = camUp;
            cb->SizeScale = 1.0f;
            m_context->Unmap(m_cameraCB.Get(), 0);
        }
    }

    // パイプライン設定
    m_context->IASetInputLayout(nullptr);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    // // 2つのテクスチャをバインド
    ID3D11ShaderResourceView* mistSRV = m_bloodMistSRV.Get();
    m_context->PSSetShaderResources(1, 1, &mistSRV);      // t1 = 霧

    ID3D11ShaderResourceView* splashSRV = m_bloodSplashSRV.Get();
    m_context->PSSetShaderResources(2, 1, &splashSRV);     // t2 = 液体

    m_context->VSSetShaderResources(0, 1, m_particleSRV.GetAddressOf());

    // サンプラー
    ID3D11SamplerState* sampler = m_linearSampler.Get();
    m_context->PSSetSamplers(0, 1, &sampler);

    m_context->PSSetSamplers(0, 1, &sampler);
    m_context->VSSetConstantBuffers(0, 1, m_cameraCB.GetAddressOf());
    float bf[4] = { 0, 0, 0, 0 };
    m_context->OMSetBlendState(m_blendState.Get(), bf, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_depthState.Get(), 0);

    m_context->DrawIndexed(m_maxParticles * 6, 0, 0);

    RestoreState(saved);

    //  t1スロットをクリア
   // // t1, t2スロットをクリア
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_context->PSSetShaderResources(1, 1, &nullSRV);
    m_context->PSSetShaderResources(2, 1, &nullSRV);


}

// ============================================================
//  流体シェーダーコンパイル
// ============================================================
bool GPUParticleSystem::CompileFluidShaders()
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob, errBlob;

    // --- フルスクリーンVS ---
    hr = D3DCompileFromFile(L"Assets/Shaders/FullscreenQuadVS.hlsl",
        nullptr, nullptr, "main", "vs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[GPUParticle] FullscreenVS compile FAILED\n");
        return false;
    }
    m_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_fullscreenVS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FullscreenVS compiled OK\n");

    // --- 深度PS ---
    blob.Reset(); errBlob.Reset();
    hr = D3DCompileFromFile(L"Assets/Shaders/BloodFluidDepthPS.hlsl",
        nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[GPUParticle] FluidDepthPS compile FAILED\n");
        return false;
    }
    m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_fluidDepthPS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FluidDepthPS compiled OK\n");

    // --- ブラーPS ---
    blob.Reset(); errBlob.Reset();
    hr = D3DCompileFromFile(L"Assets/Shaders/BloodFluidBlurPS.hlsl",
        nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[GPUParticle] FluidBlurPS compile FAILED\n");
        return false;
    }
    m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_fluidBlurPS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FluidBlurPS compiled OK\n");

    // --- 合成PS ---
    blob.Reset(); errBlob.Reset();
    hr = D3DCompileFromFile(L"Assets/Shaders/BloodFluidCompositePS.hlsl",
        nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        blob.GetAddressOf(), errBlob.GetAddressOf());
    if (FAILED(hr)) {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[GPUParticle] FluidCompositePS compile FAILED\n");
        return false;
    }
    m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_fluidCompositePS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FluidCompositePS compiled OK\n");

    m_fluidShadersReady = true;
    return true;
}

// ============================================================
//  流体テクスチャ/バッファ作成
// ============================================================
bool GPUParticleSystem::CreateFluidResources()
{
    HRESULT hr;

    // 古いリソースを解放
    m_fluidDepthTex.Reset(); m_fluidDepthRTV.Reset(); m_fluidDepthSRV.Reset();
    m_blurTempTex.Reset();   m_blurTempRTV.Reset();   m_blurTempSRV.Reset();

    // --- 深度テクスチャ(R32_FLOAT) ---
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = m_screenWidth;
        desc.Height = m_screenHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R32_FLOAT;  // 32bit浮動小数点(深度保存用)
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = m_device->CreateTexture2D(&desc, nullptr, m_fluidDepthTex.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(m_fluidDepthTex.Get(), nullptr, m_fluidDepthRTV.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateShaderResourceView(m_fluidDepthTex.Get(), nullptr, m_fluidDepthSRV.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- ブラー中間テクスチャ(R32_FLOAT) ---
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = m_screenWidth;
        desc.Height = m_screenHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R32_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = m_device->CreateTexture2D(&desc, nullptr, m_blurTempTex.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(m_blurTempTex.Get(), nullptr, m_blurTempRTV.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateShaderResourceView(m_blurTempTex.Get(), nullptr, m_blurTempSRV.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- ブラー定数バッファ ---
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(BlurCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = m_device->CreateBuffer(&desc, nullptr, m_blurCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- 合成定数バッファ ---
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(CompositeCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = m_device->CreateBuffer(&desc, nullptr, m_compositeCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- ポイントサンプラー(深度テクスチャは補間しない) ---
    if (!m_pointSampler)
    {
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        hr = m_device->CreateSamplerState(&desc, m_pointSampler.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    char msg[256];
    sprintf_s(msg, "[GPUParticle] Fluid resources created (%dx%d)\n", m_screenWidth, m_screenHeight);
    OutputDebugStringA(msg);
    return true;
}

// ============================================================
//  画面サイズ変更時
// ============================================================
void GPUParticleSystem::OnResize(int width, int height)
{
    if (width == m_screenWidth && height == m_screenHeight) return;
    m_screenWidth = width;
    m_screenHeight = height;
    if (m_fluidShadersReady)
        CreateFluidResources();
}

// ============================================================
//  Pass 1: 深度パス - パーティクルの球面深度をテクスチャに描く
// ============================================================
void GPUParticleSystem::FluidDepthPass(XMMATRIX view, XMMATRIX proj, XMFLOAT3 cameraPos)
{
    // 深度テクスチャをクリア(0 = 何もない)
    float clearColor[4] = { 0, 0, 0, 0 };
    m_context->ClearRenderTargetView(m_fluidDepthRTV.Get(), clearColor);

    // レンダーターゲットを深度テクスチャに変更(深度テストなし)
    ID3D11RenderTargetView* rtv = m_fluidDepthRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, nullptr);

    // カメラ定数バッファ更新
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMFLOAT4X4 ivF;
    XMStoreFloat4x4(&ivF, invView);
    XMFLOAT3 camRight(ivF._11, ivF._12, ivF._13);
    XMFLOAT3 camUp(ivF._21, ivF._22, ivF._23);
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_cameraCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CameraCBData* cb = (CameraCBData*)mapped.pData;
            XMStoreFloat4x4(&cb->View, XMMatrixTranspose(view));
            XMStoreFloat4x4(&cb->Projection, XMMatrixTranspose(proj));
            cb->CameraRight = camRight;
            cb->Time = m_totalTime;
            cb->CameraUp = camUp;
            cb->SizeScale = 3.0f;
            m_context->Unmap(m_cameraCB.Get(), 0);
        }
    }

    // パイプライン設定(既存VSを再利用、PSだけ深度用に変更)
    m_context->IASetInputLayout(nullptr);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);       // 既存ビルボードVS
    m_context->PSSetShader(m_fluidDepthPS.Get(), nullptr, 0);       // //深度PS
    m_context->VSSetShaderResources(0, 1, m_particleSRV.GetAddressOf());
    m_context->VSSetConstantBuffers(0, 1, m_cameraCB.GetAddressOf());

    // ブレンド: 最小値書き込み(手前の深度を優先)
    // // 加算ブレンドではなく不透明で上書き(深度は混ぜない)
    m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(nullptr, 0);

    // 描画
    m_context->DrawIndexed(m_maxParticles * 6, 0, 0);

    // SRVアンバインド
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    m_context->VSSetShaderResources(0, 1, nullSRV);
}

// ============================================================
//  Pass 2: ブラーパス - バイラテラルブラーで深度を滑らかに
// ============================================================
void GPUParticleSystem::FluidBlurPass()
{
    float texelW = 1.0f / (float)m_screenWidth;
    float texelH = 1.0f / (float)m_screenHeight;

    // --- 水平ブラー: fluidDepth → blurTemp ---
    {
        float clearColor[4] = { 0, 0, 0, 0 };
        m_context->ClearRenderTargetView(m_blurTempRTV.Get(), clearColor);

        ID3D11RenderTargetView* rtv = m_blurTempRTV.Get();
        m_context->OMSetRenderTargets(1, &rtv, nullptr);

        // 定数バッファ: 水平方向
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_blurCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            BlurCBData* cb = (BlurCBData*)mapped.pData;
            cb->TexelSize = XMFLOAT2(texelW, texelH);
            cb->BlurRadius = 15.0f;        // 7ピクセル半径
            cb->DepthThreshold = 0.05f;    // 深度差1%以上は混ぜない
            cb->BlurDirection = XMFLOAT2(1.0f, 0.0f);  // 水平
            cb->Padding[0] = cb->Padding[1] = 0.0f;
            m_context->Unmap(m_blurCB.Get(), 0);
        }

        // フルスクリーンクワッドで描画
        m_context->IASetInputLayout(nullptr);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
        m_context->PSSetShader(m_fluidBlurPS.Get(), nullptr, 0);
        m_context->PSSetShaderResources(0, 1, m_fluidDepthSRV.GetAddressOf());  // 入力: 深度テクスチャ
        m_context->PSSetConstantBuffers(0, 1, m_blurCB.GetAddressOf());
        m_context->PSSetSamplers(0, 1, m_pointSampler.GetAddressOf());
        m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(nullptr, 0);

        m_context->Draw(3, 0);  // フルスクリーン三角形

        // SRVアンバインド
        ID3D11ShaderResourceView* nullSRV[] = { nullptr };
        m_context->PSSetShaderResources(0, 1, nullSRV);
    }

    // --- 垂直ブラー: blurTemp → fluidDepth(上書き) ---
    {
        float clearColor[4] = { 0, 0, 0, 0 };
        m_context->ClearRenderTargetView(m_fluidDepthRTV.Get(), clearColor);

        ID3D11RenderTargetView* rtv = m_fluidDepthRTV.Get();
        m_context->OMSetRenderTargets(1, &rtv, nullptr);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_blurCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            BlurCBData* cb = (BlurCBData*)mapped.pData;
            cb->TexelSize = XMFLOAT2(texelW, texelH);
            cb->BlurRadius = 15.0f;
            cb->DepthThreshold = 0.05f;
            cb->BlurDirection = XMFLOAT2(0.0f, 1.0f);  // 垂直
            cb->Padding[0] = cb->Padding[1] = 0.0f;
            m_context->Unmap(m_blurCB.Get(), 0);
        }

        m_context->PSSetShaderResources(0, 1, m_blurTempSRV.GetAddressOf());  // 入力: 水平ブラー結果
        m_context->Draw(3, 0);

        ID3D11ShaderResourceView* nullSRV[] = { nullptr };
        m_context->PSSetShaderResources(0, 1, nullSRV);
    }
}

// ============================================================
//  Pass 3: 合成パス - 法線復元 + PBRライティング
// ============================================================
void GPUParticleSystem::FluidCompositePass(XMMATRIX proj, XMFLOAT3 cameraPos,
    ID3D11ShaderResourceView* sceneColorSRV, ID3D11RenderTargetView* finalRTV)
{
    // 最終出力先をセット
    m_context->OMSetRenderTargets(1, &finalRTV, nullptr);

    // 合成定数バッファ更新
    {
        XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_compositeCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CompositeCBData* cb = (CompositeCBData*)mapped.pData;
            XMStoreFloat4x4(&cb->InvProjection, XMMatrixTranspose(invProj));
            cb->TexelSize = XMFLOAT2(1.0f / m_screenWidth, 1.0f / m_screenHeight);
            cb->Time = m_totalTime;
            cb->Thickness = 0.3f;           // 血の厚み
            cb->LightDir = XMFLOAT3(0.3f, -0.8f, 0.5f);  // 斜め上からの光
            cb->SpecularPower = 128.0f;     // 鋭いスペキュラ
            cb->CameraPos = cameraPos;
            cb->FluidAlpha = 0.85f;         // 流体の不透明度
            m_context->Unmap(m_compositeCB.Get(), 0);
        }
    }

    // シェーダー設定
    m_context->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
    m_context->PSSetShader(m_fluidCompositePS.Get(), nullptr, 0);

    // テクスチャバインド
    ID3D11ShaderResourceView* srvs[2] = {
        m_fluidDepthSRV.Get(),  // t0: ブラー済み深度
        sceneColorSRV            // t1: シーンカラー
    };
    m_context->PSSetShaderResources(0, 2, srvs);
    m_context->PSSetConstantBuffers(0, 1, m_compositeCB.GetAddressOf());
    m_context->PSSetSamplers(0, 1, m_pointSampler.GetAddressOf());

    m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(nullptr, 0);

    // フルスクリーン描画
    m_context->Draw(3, 0);

    // クリーンアップ
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    m_context->PSSetShaderResources(0, 2, nullSRVs);
}

// ============================================================
//  流体描画(マルチパス: 深度→ブラー→合成)
// ============================================================
void GPUParticleSystem::DrawFluid(XMMATRIX view, XMMATRIX proj, XMFLOAT3 cameraPos,
    ID3D11ShaderResourceView* sceneColorSRV, ID3D11RenderTargetView* finalRTV)
{
    if (!m_fluidShadersReady || !m_fluidEnabled)
    {
        Draw(view, proj, cameraPos);
        return;
    }

    // ステート保存
    SavedState saved;
    SaveState(saved);

    // ビューポート設定
    D3D11_VIEWPORT vp = {};
    vp.Width = (float)m_screenWidth;
    vp.Height = (float)m_screenHeight;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    // // ラスタライザをデフォルトに(カリング問題を防止)
    m_context->RSSetState(nullptr);

    // Pass 1: パーティクル深度を描画
    FluidDepthPass(view, proj, cameraPos);

    // Pass 2: バイラテラルブラーで深度を滑らかに
    FluidBlurPass();

    // Pass 3: 法線復元 + PBRライティング + シーン合成
    FluidCompositePass(proj, cameraPos, sceneColorSRV, finalRTV);

    // ステート復元
    RestoreState(saved);
}