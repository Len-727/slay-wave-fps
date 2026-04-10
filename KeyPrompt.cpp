// ============================================================
//  KeyPrompt.cpp - キープロンプト ビルボードの実装
//
//  【設計判断】
//  StunRingVS.cso を再利用し、テクスチャ用PSだけ新規。
//  StunRing とは別クラスにして責務を分離。
//  「形をシェーダーで描く（StunRing）」と
//  「テクスチャを貼る（KeyPrompt）」は別の仕事。
// ============================================================
#include "KeyPrompt.h"
#include <WICTextureLoader.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

KeyPrompt::KeyPrompt() {}
KeyPrompt::~KeyPrompt() {}

// ============================================================
//  Initialize - 全リソースの初期化
// ============================================================
bool KeyPrompt::Initialize(ID3D11Device* device, const wchar_t* texturePath)
{
    if (!CreateShaders(device))        return false;
    if (!CreateQuad(device))           return false;
    if (!CreateConstantBuffer(device)) return false;
    if (!LoadTexture(device, texturePath)) return false;

    // === アルファブレンドステート ===
    // 通常のアルファブレンド（SrcAlpha * Src + InvSrcAlpha * Dest）
    // StunRingの加算ブレンドとは異なる。テクスチャの半透明を正しく描くため。
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());

    // === 深度ステート（テストも書き込みもOFF）===
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    device->CreateDepthStencilState(&depthDesc, m_depthState.GetAddressOf());

    // === ラスタライザ（両面描画）===
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    device->CreateRasterizerState(&rasterDesc, m_rasterState.GetAddressOf());

    // === テクスチャサンプラー ===
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // バイリニアフィルタ
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf());

    m_initialized = true;
    OutputDebugStringA("[KeyPrompt] Initialize complete!\n");
    return true;
}

// ============================================================
//  シェーダーの読み込み
// ============================================================
bool KeyPrompt::CreateShaders(ID3D11Device* device)
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // --- 頂点シェーダー: StunRingVS.cso を再利用 ---
    hr = D3DReadFileToBlob(L"Assets/Shaders/StunRingVS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[KeyPrompt] StunRingVS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_vs.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- 入力レイアウト（StunRingと同じ）---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        m_inputLayout.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- ピクセルシェーダー: KeyPromptPS.cso ---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/KeyPromptPS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[KeyPrompt] KeyPromptPS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_ps.GetAddressOf());
    if (FAILED(hr)) return false;

    OutputDebugStringA("[KeyPrompt] Shaders loaded OK\n");
    return true;
}

// ============================================================
//  クワッド作成（StunRingと同じ6頂点）
// ============================================================
bool KeyPrompt::CreateQuad(ID3D11Device* device)
{
    BillboardVertex vertices[] = {
        { XMFLOAT3(-1,  1, 0), XMFLOAT2(0, 0) },
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },
        { XMFLOAT3(1, -1, 0), XMFLOAT2(1, 1) },
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    return SUCCEEDED(device->CreateBuffer(&vbDesc, &initData, m_vertexBuffer.GetAddressOf()));
}

// ============================================================
//  定数バッファ作成
// ============================================================
bool KeyPrompt::CreateConstantBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(PromptCB);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    return SUCCEEDED(device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf()));
}

// ============================================================
//  テクスチャ読み込み
// ============================================================
bool KeyPrompt::LoadTexture(ID3D11Device* device, const wchar_t* path)
{
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        device, path, nullptr, m_texture.GetAddressOf());

    if (FAILED(hr))
    {
        OutputDebugStringA("[KeyPrompt] Texture load FAILED!\n");
        return false;
    }

    OutputDebugStringA("[KeyPrompt] Texture loaded OK\n");
    return true;
}

// ============================================================
//  ビルボード描画
// ============================================================
void KeyPrompt::Render(
    ID3D11DeviceContext* context,
    XMFLOAT3 worldPos,
    float size,
    float time,
    XMMATRIX view,
    XMMATRIX projection)
{
    if (!m_initialized) return;

    // --- 定数バッファ更新 ---
    PromptCB cb;
    cb.View = XMMatrixTranspose(view);
    cb.Projection = XMMatrixTranspose(projection);
    cb.EnemyPos = worldPos;
    cb.Size = size;
    cb.Time = time;
    cb.Padding = XMFLOAT3(0, 0, 0);
    context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // --- レンダリングステートを保存 ---
    Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlend;
    FLOAT prevBlendFactor[4];
    UINT prevSampleMask;
    context->OMGetBlendState(prevBlend.GetAddressOf(), prevBlendFactor, &prevSampleMask);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> prevDepth;
    UINT prevStencilRef;
    context->OMGetDepthStencilState(prevDepth.GetAddressOf(), &prevStencilRef);

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> prevRaster;
    context->RSGetState(prevRaster.GetAddressOf());

    // --- パイプラインセット ---
    context->VSSetShader(m_vs.Get(), nullptr, 0);
    context->PSSetShader(m_ps.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());

    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // テクスチャとサンプラーをセット
    context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

    // ブレンド + 深度 + ラスタライザ
    float blendFactor[] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthState.Get(), 0);
    context->RSSetState(m_rasterState.Get());

    // 頂点バッファ
    UINT stride = sizeof(BillboardVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- 描画 ---
    context->Draw(6, 0);

    // --- テクスチャをクリア（他の描画に影響しないように）---
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    // --- レンダリングステートを戻す ---
    context->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
    context->OMSetDepthStencilState(prevDepth.Get(), prevStencilRef);
    context->RSSetState(prevRaster.Get());
}
