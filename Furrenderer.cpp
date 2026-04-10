// =============================================================
// FurRenderer.cpp - ファー/苔レンダラーの実装
// =============================================================

#include "FurRenderer.h"
#include <d3dcompiler.h>    // D3DCompileFromFile
#include <vector>

// d3dcompiler.lib をリンク
#pragma comment(lib, "d3dcompiler.lib")

// =============================================================
// コンストラクタ
// =============================================================
FurRenderer::FurRenderer()
    : m_shellCount(12)
    , m_furLength(0.08f)     // 苔の高さ（メートル単位）
    , m_furDensity(0.80f)     // 密度（0.0?1.0）
    , m_indexCount(0)
{
}

// =============================================================
// Initialize - 初期化
// =============================================================
bool FurRenderer::Initialize(ID3D11Device* device)
{
    if (!CompileShaders(device))
    {
        //OutputDebugStringA("[FUR] Shader compilation FAILED\n");
        return false;
    }

    if (!CreateGroundQuad(device))
    {
        //OutputDebugStringA("[FUR] Ground quad creation FAILED\n");
        return false;
    }

    // --- 定数バッファ作成 ---
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(FurCB);           // バッファサイズ
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;          // CPU書き換え可能
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
    if (FAILED(hr))
    {
       // OutputDebugStringA("[FUR] Constant buffer creation FAILED\n");
        return false;
    }

    // --- アルファブレンドステート ---
    // 毛の先端が半透明で下の層が透けて見える必要がある
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;        // ソースのアルファ
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   // 1-ソースアルファ
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&blendDesc, m_alphaBlendState.GetAddressOf());
    if (FAILED(hr))
    {
        //OutputDebugStringA("[FUR] Blend state creation FAILED\n");
        return false;
    }

    // --- 両面描画（カリングなし）---
    // 苔は薄い層なので裏面も見える必要がある
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;    // 両面描画
    rsDesc.DepthClipEnable = TRUE;

    hr = device->CreateRasterizerState(&rsDesc, m_noCullState.GetAddressOf());
    if (FAILED(hr))
    {
        //OutputDebugStringA("[FUR] Rasterizer state creation FAILED\n");
        return false;
    }

    // --- 深度テストあり・書き込みなし ---
    // 他のオブジェクトの前後関係は保つが、ファー層同士は深度を書かない
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;                          // 深度テストON
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 深度書き込みOFF
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    hr = device->CreateDepthStencilState(&dsDesc, m_depthWriteOff.GetAddressOf());
    if (FAILED(hr))
    {
        ////OutputDebugStringA("[FUR] Depth stencil state creation FAILED\n");
        return false;
    }

    OutputDebugStringA("[FUR] Initialized successfully!\n");
    return true;
}

// =============================================================
// CompileShaders - HLSLファイルからシェーダーをコンパイル
// =============================================================
bool FurRenderer::CompileShaders(ID3D11Device* device)
{
    HRESULT hr;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;

    // --- 頂点シェーダーのコンパイル ---
    hr = D3DCompileFromFile(
        L"FurVS.hlsl",  // ファイルパス
        nullptr,                        // マクロ定義
        nullptr,                        // インクルード
        "main",                         // エントリーポイント
        "vs_5_0",                       // シェーダーモデル
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  // デバッグ用フラグ
        0,
        vsBlob.GetAddressOf(),
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            char buf[1024];
            sprintf_s(buf, "[FUR] VS compile error: %s\n",
                (char*)errorBlob->GetBufferPointer());
            //OutputDebugStringA(buf);
        }
        return false;
    }

    // --- ピクセルシェーダーのコンパイル ---
    hr = D3DCompileFromFile(
        L"FurPS.hlsl",
        nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        psBlob.GetAddressOf(),
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            char buf[1024];
            sprintf_s(buf, "[FUR] PS compile error: %s\n",
                (char*)errorBlob->GetBufferPointer());
            //OutputDebugStringA(buf);
        }
        return false;
    }

    // --- シェーダーオブジェクト作成 ---
    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        m_vertexShader.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        m_pixelShader.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    // --- 入力レイアウト ---
    // 頂点シェーダーが「どんなデータが来るか」を知るための定義
    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(
        inputDesc,
        3,                                // 要素数
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        m_inputLayout.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    //OutputDebugStringA("[FUR] Shaders compiled OK\n");
    return true;
}

// =============================================================
// CreateGroundQuad - 地面の四角形メッシュを作成
// 
// 既存の地面（50x50）より少し大きめのクアッドを作る
// 法線は上向き(0,1,0)、UVは0?1
//
// =============================================================
bool FurRenderer::CreateGroundQuad(ID3D11Device* device)
{
    // 地面のサイズ（MapSystemのFloorに合わせる：50x50）
    float halfSize = 25.0f;
    float groundY = 0.0f;  // 地面の高さ（地面のBoxの上面に合わせる）

    // --- 頂点データ ---
    FurVertex vertices[] = {
        // Position                        Normal           TexCoord
        { {-halfSize, groundY, -halfSize}, {0, 1, 0},      {0, 0} },  // 左奥
        { { halfSize, groundY, -halfSize}, {0, 1, 0},      {1, 0} },  // 右奥
        { {-halfSize, groundY,  halfSize}, {0, 1, 0},      {0, 1} },  // 左手前
        { { halfSize, groundY,  halfSize}, {0, 1, 0},      {1, 1} },  // 右手前
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- インデックスデータ（三角形2つで四角形）---
    UINT indices[] = {
        0, 1, 2,    // 三角形1
        1, 3, 2     // 三角形2
    };
    m_indexCount = 6;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;

    hr = device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

   // OutputDebugStringA("[FUR] Ground quad created OK\n");
    return true;
}

// =============================================================
// DrawGroundMoss - 地面に苔を描画
//
// 【仕組み】同じクアッドをm_shellCount回描画する
// 各回で CurrentLayer の値を変えることで、
// 法線方向への押し出し量が変わる → 層が積み重なる
// =============================================================
void FurRenderer::DrawGroundMoss(
    ID3D11DeviceContext* context,
    DirectX::XMMATRIX view,
    DirectX::XMMATRIX projection,
    float elapsedTime)
{
    // --- 現在の描画ステートを保存 ---
    ComPtr<ID3D11BlendState> prevBlend;
    FLOAT prevBlendFactor[4];
    UINT prevSampleMask;
    context->OMGetBlendState(prevBlend.GetAddressOf(), prevBlendFactor, &prevSampleMask);

    ComPtr<ID3D11RasterizerState> prevRS;
    context->RSGetState(prevRS.GetAddressOf());

    ComPtr<ID3D11DepthStencilState> prevDS;
    UINT prevStencilRef;
    context->OMGetDepthStencilState(prevDS.GetAddressOf(), &prevStencilRef);

    // --- 描画ステートを設定 ---
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
    context->RSSetState(m_noCullState.Get());
    context->OMSetDepthStencilState(m_depthWriteOff.Get(), 0);

    // --- シェーダーをセット ---
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());

    // --- 頂点バッファをセット ---
    UINT stride = sizeof(FurVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- 定数バッファをセット ---
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // --- ワールド行列（地面は原点に配置）---
    DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

    // ==============================================
    // 各層を描画
    // ==============================================
    for (int i = 0; i < m_shellCount; i++)
    {
        // --- 定数バッファを更新 ---
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) continue;

        FurCB* cb = (FurCB*)mapped.pData;

        // 行列を格納（XMMATRIXからXMFLOAT4X4に変換）
        DirectX::XMStoreFloat4x4(&cb->World, DirectX::XMMatrixTranspose(world));
        DirectX::XMStoreFloat4x4(&cb->View, DirectX::XMMatrixTranspose(view));
        DirectX::XMStoreFloat4x4(&cb->Projection, DirectX::XMMatrixTranspose(projection));

        // ファーパラメータ
        cb->FurLength = m_furLength;
        cb->CurrentLayer = (float)i;
        cb->TotalLayers = (float)m_shellCount;
        cb->Time = elapsedTime;

        // 風（緩やかに揺れる程度）
        cb->WindDirection = DirectX::XMFLOAT3(1.0f, 0.0f, 0.3f);
        cb->WindStrength = 0.015f;

        // 苔の色（ゴシックな暗い緑?明るい緑）
        cb->BaseColor = DirectX::XMFLOAT3(0.05f, 0.12f, 0.03f);  // 根元：暗い緑
        cb->FurDensity = m_furDensity;
        cb->TipColor = DirectX::XMFLOAT3(0.15f, 0.35f, 0.08f);   // 先端：明るい緑
        cb->Padding2 = 0.0f;

        // ライティング
        cb->LightDir = DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f);
        cb->AmbientStrength = 0.4f;

        context->Unmap(m_constantBuffer.Get(), 0);

        // --- この層を描画 ---
        context->DrawIndexed(m_indexCount, 0, 0);
    }

    // --- 描画ステートを復元 ---
    context->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
    context->RSSetState(prevRS.Get());
    context->OMSetDepthStencilState(prevDS.Get(), prevStencilRef);
}
