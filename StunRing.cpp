// StunRing.cpp - スタンリングの初期化と描画
#include "StunRing.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

StunRing::StunRing() {}
StunRing::~StunRing() {}

// ============================================================
// 初期化（デバイス作成後に1回だけ呼ぶ）
// ============================================================
bool StunRing::Initialize(ID3D11Device* device)
{
    if (!CreateShaders(device))  return false;
    if (!CreateQuad(device))     return false;
    if (!CreateConstantBuffer(device)) return false;

    // === 加算ブレンドステート ===
    // リングの光を「足す」（元の色 + リングの色）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;       // ソースのアルファ
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;            // 加算！
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());

    // === 深度ステート（深度テストはするが書き込みはしない）===
    // 敵の後ろに隠れるけど、リング自体が深度を汚さない
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // 書き込まない
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    device->CreateDepthStencilState(&depthDesc, m_depthState.GetAddressOf());

    // === ラスタライザ（両面描画）===
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;  // 裏面も描画（ビルボードなので）
    device->CreateRasterizerState(&rasterDesc, m_rasterState.GetAddressOf());

    m_initialized = true;
    OutputDebugStringA("[StunRing] Initialize complete!\n");
    return true;
}

// ============================================================
// シェーダーのコンパイル
// ============================================================
bool StunRing::CreateShaders(ID3D11Device* device)
{
    // ========================================
    // 【役割】スタンリング用シェーダーを .cso から読み込む
    // 【理由】ランタイムコンパイル廃止 → 起動高速化＆配布時に .hlsl 不要
    // ========================================

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // --- 頂点シェーダー ---
    hr = D3DReadFileToBlob(L"Assets/Shaders/StunRingVS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[StunRing] StunRingVS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_vs.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    // --- 入力レイアウト ---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        m_inputLayout.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    // --- ピクセルシェーダー ---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/StunRingPS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[StunRing] StunRingPS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_ps.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    OutputDebugStringA("[StunRing] Shaders loaded from CSO\n");
    return true;
}
// ============================================================
// クワッド（四角形）作成
// ============================================================
bool StunRing::CreateQuad(ID3D11Device* device)
{
    // ビルボード用の四角形（2三角形 = 6頂点）
    // Position: -1?+1, TexCoord: 0?1
    RingVertex vertices[] = {
        // 三角形1
        { XMFLOAT3(-1,  1, 0), XMFLOAT2(0, 0) },  // 左上
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },  // 右上
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },  // 左下
        // 三角形2
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },  // 右上
        { XMFLOAT3(1, -1, 0), XMFLOAT2(1, 1) },  // 右下
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },  // 左下
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    HRESULT hr = device->CreateBuffer(&vbDesc, &initData, m_vertexBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

// ============================================================
// 定数バッファ作成
// ============================================================
bool StunRing::CreateConstantBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(RingCB);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

// ============================================================
// リングを1つ描画
// ============================================================
void StunRing::Render(
    ID3D11DeviceContext* context,
    XMFLOAT3 enemyPos,
    float ringSize,
    float time,
    XMMATRIX view,
    XMMATRIX projection)
{
    if (!m_initialized) return;

    // --- 定数バッファ更新 ---
    RingCB cb;
    cb.View = XMMatrixTranspose(view);
    cb.Projection = XMMatrixTranspose(projection);
    cb.EnemyPos = enemyPos;
    cb.RingSize = ringSize;
    cb.Time = time;
    cb.Padding = XMFLOAT3(0, 0, 0);
    context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // --- レンダリングステートを保存 ---
    // （他の描画に影響しないように、後で戻す）
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

    // 定数バッファをVSとPS両方にセット
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // 加算ブレンド + 深度書き込みOFF + 両面描画
    float blendFactor[] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthState.Get(), 0);
    context->RSSetState(m_rasterState.Get());

    // 頂点バッファ
    UINT stride = sizeof(RingVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- 描画！ ---
    context->Draw(6, 0);

    // --- レンダリングステートを戻す ---
    context->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
    context->OMSetDepthStencilState(prevDepth.Get(), prevStencilRef);
    context->RSSetState(prevRaster.Get());
}