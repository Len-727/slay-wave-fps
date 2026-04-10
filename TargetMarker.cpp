// TargetMarker.cpp - ロックオンマーカーの初期化と描画
#include "TargetMarker.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

TargetMarker::TargetMarker() {}
TargetMarker::~TargetMarker() {}

bool TargetMarker::Initialize(ID3D11Device* device)
{
    if (!CreateShaders(device))  return false;
    if (!CreateQuad(device))     return false;
    if (!CreateConstantBuffer(device)) return false;

    // 加算ブレンド
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());

    // 深度テストあり、書き込みなし
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    device->CreateDepthStencilState(&depthDesc, m_depthState.GetAddressOf());

    // 両面描画
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    device->CreateRasterizerState(&rasterDesc, m_rasterState.GetAddressOf());

    m_initialized = true;
    OutputDebugStringA("[TargetMarker] Initialize complete!\n");
    return true;
}

bool TargetMarker::CreateShaders(ID3D11Device* device)
{
    // ========================================
    // 【役割】ロックオンマーカー用シェーダーを .cso から読み込む
    // ========================================

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // --- 頂点シェーダー（StunRingVS を共用）---
    hr = D3DReadFileToBlob(L"Assets/Shaders/StunRingVS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[TargetMarker] StunRingVS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_vs.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- 入力レイアウト ---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        m_inputLayout.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- ピクセルシェーダー（TargetMarkerPS）---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/TargetMarkerPS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[TargetMarker] TargetMarkerPS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_ps.GetAddressOf());
    if (FAILED(hr)) return false;

    OutputDebugStringA("[TargetMarker] Shaders loaded from CSO\n");
    return true;
}

bool TargetMarker::CreateQuad(ID3D11Device* device)
{
    MarkerVertex vertices[] = {
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

bool TargetMarker::CreateConstantBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(MarkerCB);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    return SUCCEEDED(device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf()));
}

void TargetMarker::Render(
    ID3D11DeviceContext* context,
    XMFLOAT3 targetPos,
    float markerSize,
    float time,
    XMMATRIX view,
    XMMATRIX projection)
{
    if (!m_initialized) return;

    MarkerCB cb;
    cb.View = XMMatrixTranspose(view);
    cb.Projection = XMMatrixTranspose(projection);
    cb.EnemyPos = targetPos;
    cb.RingSize = markerSize;
    cb.Time = time;
    cb.Padding = XMFLOAT3(0, 0, 0);
    context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // ステート保存
    Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlend;
    FLOAT prevBlendFactor[4]; UINT prevSampleMask;
    context->OMGetBlendState(prevBlend.GetAddressOf(), prevBlendFactor, &prevSampleMask);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> prevDepth; UINT prevStencilRef;
    context->OMGetDepthStencilState(prevDepth.GetAddressOf(), &prevStencilRef);
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> prevRaster;
    context->RSGetState(prevRaster.GetAddressOf());

    // パイプラインセット
    context->VSSetShader(m_vs.Get(), nullptr, 0);
    context->PSSetShader(m_ps.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    float blendFactor[] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthState.Get(), 0);
    context->RSSetState(m_rasterState.Get());

    UINT stride = sizeof(MarkerVertex), offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->Draw(6, 0);

    // ステート復元
    context->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
    context->OMSetDepthStencilState(prevDepth.Get(), prevStencilRef);
    context->RSSetState(prevRaster.Get());
}
