// TargetMarker.h - 盾構え中のロックオンマーカー
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

class TargetMarker
{
public:
    TargetMarker();
    ~TargetMarker();

    bool Initialize(ID3D11Device* device);

    // マーカーを1つ描画
    void Render(
        ID3D11DeviceContext* context,
        DirectX::XMFLOAT3 targetPos,
        float markerSize,
        float time,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection
    );

private:
    bool CreateShaders(ID3D11Device* device);
    bool CreateQuad(ID3D11Device* device);
    bool CreateConstantBuffer(ID3D11Device* device);

    struct MarkerVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 TexCoord;
    };

    // HLSLと完全一致（StunRingVS.hlslと同じ構造）
    struct MarkerCB
    {
        DirectX::XMMATRIX View;
        DirectX::XMMATRIX Projection;
        DirectX::XMFLOAT3 EnemyPos;    // ターゲット位置
        float              RingSize;     // マーカーサイズ
        float              Time;
        DirectX::XMFLOAT3 Padding;
    };

    Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_ps;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11BlendState>        m_blendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_rasterState;

    bool m_initialized = false;
};
