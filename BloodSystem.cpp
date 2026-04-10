// ============================================================
//  BloodSystem.cpp
//  血しぶき＆血デカール管理 (カスタムHLSLシェーダー対応版)
// ============================================================

#include "BloodSystem.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstdlib>

#pragma comment(lib, "d3dcompiler.lib")

// ============================================================
//  コンストラクタ
// ============================================================
BloodSystem::BloodSystem()
{
    m_screenBloods.reserve(200);
    m_bloodDecals.reserve(MAX_BLOOD_DECALS);
}

// ============================================================
//  シェーダーコンパイル
// ============================================================
bool BloodSystem::CompileShaders(ID3D11Device* device)
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errBlob;

    // === 頂点シェーダーコンパイル ===
    hr = D3DCompileFromFile(
        L"Assets/Shaders/BloodVS.hlsl",
        nullptr, nullptr,
        "main", "vs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        vsBlob.GetAddressOf(),
        errBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errBlob)
            OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[BloodSystem] VS compile FAILED - falling back to BasicEffect\n");
        return false;
    }

    // === 頂点シェーダー作成 ===
    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, m_bloodVS.GetAddressOf());
    if (FAILED(hr)) return false;

    // === 入力レイアウト作成 ===
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(
        layoutDesc, 3,
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        m_bloodIL.GetAddressOf());
    if (FAILED(hr)) return false;

    // === ピクセルシェーダーコンパイル ===
    errBlob.Reset();
    hr = D3DCompileFromFile(
        L"Assets/Shaders/BloodPS.hlsl",
        nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        psBlob.GetAddressOf(),
        errBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errBlob)
            OutputDebugStringA((char*)errBlob->GetBufferPointer());
        OutputDebugStringA("[BloodSystem] PS compile FAILED - falling back to BasicEffect\n");
        return false;
    }

    // === ピクセルシェーダー作成 ===
    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
        nullptr, m_bloodPS.GetAddressOf());
    if (FAILED(hr)) return false;

    // === 定数バッファ作成 ===
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(BloodCBData);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(&cbDesc, nullptr, m_bloodCB.GetAddressOf());
    if (FAILED(hr)) return false;

    OutputDebugStringA("[BloodSystem] Custom blood shader compiled OK!\n");
    return true;
}

// ============================================================
//  初期化
// ============================================================
void BloodSystem::Initialize(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    DirectX::CommonStates* states,
    ID3D11ShaderResourceView* bloodParticleSRV,
    DirectX::BasicEffect* particleEffect,
    ID3D11InputLayout* particleInputLayout)
{
    m_context = context;
    m_states = states;
    m_bloodSRV = bloodParticleSRV;
    m_particleEffect = particleEffect;
    m_particleInputLayout = particleInputLayout;

    m_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColorTexture>>(context);

    m_hasCustomShader = CompileShaders(device);
}

// ============================================================
//  定数バッファ更新
// ============================================================
void BloodSystem::SetupBloodCB(
    DirectX::XMMATRIX wvp,
    DirectX::XMFLOAT3 camPos,
    DirectX::XMFLOAT3 lightDir,
    float screenMode)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_context->Map(m_bloodCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        BloodCBData* cb = (BloodCBData*)mapped.pData;
        DirectX::XMStoreFloat4x4(&cb->WorldViewProj, DirectX::XMMatrixTranspose(wvp));
        cb->CameraPos = camPos;
        cb->Time = m_totalTime;
        cb->LightDir = lightDir;
        cb->ScreenMode = screenMode;
        m_context->Unmap(m_bloodCB.Get(), 0);
    }
}

// ============================================================
//  更新
// ============================================================
void BloodSystem::Update(float deltaTime)
{
    m_totalTime += deltaTime;
    UpdateScreenBlood(deltaTime);
    UpdateBloodDecals(deltaTime);
}

// ============================================================
//  トリガー
// ============================================================
void BloodSystem::OnEnemyKilled(
    DirectX::XMFLOAT3 enemyPos,
    DirectX::XMFLOAT3 playerPos,
    float maxRange)
{
    float dx = enemyPos.x - playerPos.x;
    float dz = enemyPos.z - playerPos.z;
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < maxRange)
    {
        float intensity = 1.0f - (dist / maxRange);
        SpawnScreenBlood((int)(6 + intensity * 12), intensity);
        SpawnBloodDecal(enemyPos, 0.5f + intensity * 1.5f);
    }
}

void BloodSystem::OnGloryKill(DirectX::XMFLOAT3 enemyPos)
{
    SpawnScreenBlood(20, 1.0f);

    for (int i = 0; i < 5; i++)
    {
        DirectX::XMFLOAT3 pos = enemyPos;
        pos.x += ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        pos.z += ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        SpawnBloodDecal(pos, 1.0f + ((float)rand() / RAND_MAX) * 1.5f);
    }
}

void BloodSystem::OnExplosionKill(DirectX::XMFLOAT3 enemyPos, DirectX::XMFLOAT3 playerPos)
{
    float dx = enemyPos.x - playerPos.x;
    float dz = enemyPos.z - playerPos.z;
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < 10.0f)
    {
        float intensity = 1.0f - (dist / 10.0f);
        SpawnScreenBlood((int)(8 + intensity * 15), intensity);
    }

    for (int i = 0; i < 3; i++)
    {
        DirectX::XMFLOAT3 pos = enemyPos;
        pos.x += ((float)rand() / RAND_MAX - 0.5f) * 4.0f;
        pos.z += ((float)rand() / RAND_MAX - 0.5f) * 4.0f;
        SpawnBloodDecal(pos, 0.8f + ((float)rand() / RAND_MAX) * 2.0f);
    }
}

void BloodSystem::OnMeleeKill(DirectX::XMFLOAT3 enemyPos)
{
    SpawnScreenBlood(15, 1.0f);
    SpawnBloodDecal(enemyPos, 1.0f + ((float)rand() / RAND_MAX) * 1.0f);
}

// ============================================================
//  スクリーンブラッド - 生成
// ============================================================
void BloodSystem::SpawnScreenBlood(int count, float intensity)
{
    int impactCount = 2 + count / 2;
    for (int imp = 0; imp < impactCount; imp++)
    {
        float angle = ((float)rand() / RAND_MAX) * 6.28318f;
        float dist = 0.2f + ((float)rand() / RAND_MAX) * 0.5f;
        float cx = 0.5f + cosf(angle) * dist;
        float cy = 0.5f + sinf(angle) * dist;

        ScreenBlood main = {};
        main.x = cx;
        main.y = cy;
        main.size = (0.12f + ((float)rand() / RAND_MAX) * 0.15f) * intensity;
        main.alpha = 0.8f + ((float)rand() / RAND_MAX) * 0.2f;
        main.lifetime = 2.0f + ((float)rand() / RAND_MAX) * 1.5f;
        main.maxLifetime = main.lifetime;
        main.rotation = ((float)rand() / RAND_MAX) * 6.28318f;
        m_screenBloods.push_back(main);

        int splatterCount = 5 + rand() % 4;
        for (int s = 0; s < splatterCount; s++)
        {
            ScreenBlood splat = {};
            float sAngle = ((float)rand() / RAND_MAX) * 6.28318f;
            float sDist = 0.02f + ((float)rand() / RAND_MAX) * 0.08f;
            splat.x = cx + cosf(sAngle) * sDist;
            splat.y = cy + sinf(sAngle) * sDist;
            splat.size = (0.03f + ((float)rand() / RAND_MAX) * 0.06f) * intensity;
            splat.alpha = 0.6f + ((float)rand() / RAND_MAX) * 0.4f;
            splat.lifetime = 1.5f + ((float)rand() / RAND_MAX) * 1.0f;
            splat.maxLifetime = splat.lifetime;
            splat.rotation = ((float)rand() / RAND_MAX) * 6.28318f;
            m_screenBloods.push_back(splat);
        }

        int dripCount = 2 + rand() % 2;
        for (int d = 0; d < dripCount; d++)
        {
            float dripLen = 0.08f + ((float)rand() / RAND_MAX) * 0.15f;
            int segments = 5 + rand() % 4;
            for (int seg = 0; seg < segments; seg++)
            {
                ScreenBlood drip = {};
                float t = (float)seg / segments;
                drip.x = cx + ((float)rand() / RAND_MAX - 0.5f) * 0.01f;
                drip.y = cy + t * dripLen;
                drip.size = (0.02f + ((float)rand() / RAND_MAX) * 0.03f) * intensity * (1.0f - t * 0.3f);
                drip.alpha = (0.6f + ((float)rand() / RAND_MAX) * 0.3f) * (1.0f - t * 0.2f);
                drip.lifetime = 2.5f + ((float)rand() / RAND_MAX) * 1.5f + t * 1.0f;
                drip.maxLifetime = drip.lifetime;
                drip.rotation = 0.0f;
                m_screenBloods.push_back(drip);
            }
        }
    }
}

// ============================================================
//  スクリーンブラッド - 更新 (swap-and-pop)
// ============================================================
void BloodSystem::UpdateScreenBlood(float deltaTime)
{
    int count = (int)m_screenBloods.size();
    for (int i = 0; i < count;)
    {
        ScreenBlood& b = m_screenBloods[i];
        b.lifetime -= deltaTime;
        float t = b.lifetime / b.maxLifetime;
        b.alpha = t * t;
        b.y += deltaTime * (0.02f + (1.0f - b.size * 5.0f) * 0.04f);

        if (b.lifetime <= 0.0f)
        {
            m_screenBloods[i] = m_screenBloods[count - 1];
            count--;
            m_screenBloods.resize(count);
        }
        else
        {
            i++;
        }
    }
}

// ============================================================
//  スクリーンブラッド - 描画
// ============================================================
void BloodSystem::DrawScreenBlood(int screenWidth, int screenHeight)
{
    if (m_screenBloods.empty()) return;

    DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f, (float)screenWidth, (float)screenHeight, 0.0f, 0.0f, 1.0f);

    if (m_hasCustomShader)
    {
        SetupBloodCB(proj, { 0, 0, 0 }, { 0, -1, 0 }, 1.0f);
        m_context->VSSetShader(m_bloodVS.Get(), nullptr, 0);
        m_context->PSSetShader(m_bloodPS.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_bloodCB.GetAddressOf());
        m_context->PSSetConstantBuffers(0, 1, m_bloodCB.GetAddressOf());
        m_context->PSSetShaderResources(0, 1, &m_bloodSRV);
        m_context->IASetInputLayout(m_bloodIL.Get());
    }
    else
    {
        m_particleEffect->SetView(DirectX::XMMatrixIdentity());
        m_particleEffect->SetProjection(proj);
        m_particleEffect->SetWorld(DirectX::XMMatrixIdentity());
        m_particleEffect->Apply(m_context);
        m_context->IASetInputLayout(m_particleInputLayout);
    }

    float blendFactor[4] = { 0, 0, 0, 0 };
    m_context->OMSetBlendState(m_states->AlphaBlend(), blendFactor, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_states->DepthNone(), 0);

    auto sampler = m_states->LinearClamp();
    m_context->PSSetSamplers(0, 1, &sampler);

    m_batch->Begin();

    for (const auto& blood : m_screenBloods)
    {
        float px = blood.x * screenWidth;
        float py = blood.y * screenHeight;
        float s = blood.size * screenWidth * 0.5f;
        float c = cosf(blood.rotation);
        float sn = sinf(blood.rotation);

        DirectX::XMFLOAT4 col(0.4f, 0.0f, 0.0f, blood.alpha);

        auto rotPt = [&](float lx, float ly) -> DirectX::XMFLOAT3 {
            return DirectX::XMFLOAT3(
                px + (lx * c - ly * sn) * s,
                py + (lx * sn + ly * c) * s,
                0.0f);
            };

        DirectX::XMFLOAT3 tl = rotPt(-1, -1);
        DirectX::XMFLOAT3 tr = rotPt(1, -1);
        DirectX::XMFLOAT3 bl = rotPt(-1, 1);
        DirectX::XMFLOAT3 br = rotPt(1, 1);

        DirectX::VertexPositionColorTexture vTL(tl, col, DirectX::XMFLOAT2(0, 0));
        DirectX::VertexPositionColorTexture vTR(tr, col, DirectX::XMFLOAT2(1, 0));
        DirectX::VertexPositionColorTexture vBL(bl, col, DirectX::XMFLOAT2(0, 1));
        DirectX::VertexPositionColorTexture vBR(br, col, DirectX::XMFLOAT2(1, 1));

        m_batch->DrawTriangle(vTL, vTR, vBL);
        m_batch->DrawTriangle(vTR, vBR, vBL);
    }

    m_batch->End();

    m_context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
}

// ============================================================
//  床デカール - 生成
// ============================================================
void BloodSystem::SpawnBloodDecal(DirectX::XMFLOAT3 position, float size)
{
    position.y = 0.01f;

    BloodDecal decal = {};
    decal.position = position;
    decal.size = size * (0.8f + ((float)rand() / RAND_MAX) * 0.4f);
    decal.rotation = ((float)rand() / RAND_MAX) * 6.28318f;
    decal.maxLifetime = 30.0f + ((float)rand() / RAND_MAX) * 30.0f;
    decal.lifetime = decal.maxLifetime;
    decal.alpha = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;

    float r = 0.2f + ((float)rand() / RAND_MAX) * 0.3f;
    float g = ((float)rand() / RAND_MAX) * 0.02f;
    decal.color = DirectX::XMFLOAT4(r, g, 0.0f, decal.alpha);

    if ((int)m_bloodDecals.size() >= MAX_BLOOD_DECALS)
        m_bloodDecals.erase(m_bloodDecals.begin());

    m_bloodDecals.push_back(decal);
}

// ============================================================
//  床デカール - 更新 (swap-and-pop)
// ============================================================
void BloodSystem::UpdateBloodDecals(float deltaTime)
{
    int count = (int)m_bloodDecals.size();
    for (int i = 0; i < count;)
    {
        BloodDecal& d = m_bloodDecals[i];
        d.lifetime -= deltaTime;

        if (d.lifetime <= 0.0f)
        {
            m_bloodDecals[i] = m_bloodDecals[count - 1];
            count--;
            m_bloodDecals.resize(count);
        }
        else
        {
            if (d.lifetime < 5.0f)
                d.color.w = d.alpha * (d.lifetime / 5.0f);
            i++;
        }
    }
}

// ============================================================
//  床デカール - 描画
// ============================================================
void BloodSystem::DrawBloodDecals(
    DirectX::XMMATRIX view,
    DirectX::XMMATRIX proj,
    DirectX::XMFLOAT3 cameraPos)
{
    if (m_bloodDecals.empty()) return;

    DirectX::XMMATRIX vp = view * proj;

    if (m_hasCustomShader)
    {
        DirectX::XMFLOAT3 lightDir(0.3f, -0.8f, 0.5f);
        float len = sqrtf(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
        lightDir.x /= len; lightDir.y /= len; lightDir.z /= len;

        SetupBloodCB(vp, cameraPos, lightDir, 0.0f);
        m_context->VSSetShader(m_bloodVS.Get(), nullptr, 0);
        m_context->PSSetShader(m_bloodPS.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_bloodCB.GetAddressOf());
        m_context->PSSetConstantBuffers(0, 1, m_bloodCB.GetAddressOf());
        m_context->PSSetShaderResources(0, 1, &m_bloodSRV);
        m_context->IASetInputLayout(m_bloodIL.Get());
    }
    else
    {
        m_particleEffect->SetView(view);
        m_particleEffect->SetProjection(proj);
        m_particleEffect->SetWorld(DirectX::XMMatrixIdentity());
        m_particleEffect->Apply(m_context);
        m_context->IASetInputLayout(m_particleInputLayout);
    }

    float blendFactor[4] = { 0, 0, 0, 0 };
    m_context->OMSetBlendState(m_states->AlphaBlend(), blendFactor, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    auto sampler = m_states->LinearClamp();
    m_context->PSSetSamplers(0, 1, &sampler);

    m_batch->Begin();

    for (const auto& decal : m_bloodDecals)
    {
        float s = decal.size;
        float c = cosf(decal.rotation);
        float sn = sinf(decal.rotation);
        DirectX::XMFLOAT3 p = decal.position;

        auto rotPt = [&](float lx, float lz) -> DirectX::XMFLOAT3 {
            return DirectX::XMFLOAT3(
                p.x + (lx * c - lz * sn) * s,
                p.y,
                p.z + (lx * sn + lz * c) * s);
            };

        DirectX::XMFLOAT3 tl = rotPt(-1, -1);
        DirectX::XMFLOAT3 tr = rotPt(1, -1);
        DirectX::XMFLOAT3 bl = rotPt(-1, 1);
        DirectX::XMFLOAT3 br = rotPt(1, 1);

        DirectX::VertexPositionColorTexture vTL(tl, decal.color, DirectX::XMFLOAT2(0, 0));
        DirectX::VertexPositionColorTexture vTR(tr, decal.color, DirectX::XMFLOAT2(1, 0));
        DirectX::VertexPositionColorTexture vBL(bl, decal.color, DirectX::XMFLOAT2(0, 1));
        DirectX::VertexPositionColorTexture vBR(br, decal.color, DirectX::XMFLOAT2(1, 1));

        m_batch->DrawTriangle(vTL, vTR, vBL);
        m_batch->DrawTriangle(vTR, vBR, vBL);
    }

    m_batch->End();

    m_context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
}