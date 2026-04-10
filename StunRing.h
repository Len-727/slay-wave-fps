// StunRing.h - スタン状態の敵に表示するビルボードリング
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

class StunRing
{
public:
    StunRing();
    ~StunRing();

    // 初期化（デバイス作成後に1回だけ呼ぶ）
    bool Initialize(ID3D11Device* device);

    // リングを1つ描画（スタガー中の敵ごとに呼ぶ）
    // enemyPos:  敵のワールド位置
    // ringSize:  リングの半径（敵のサイズに合わせて変える）
    // time:      ゲーム経過時間（アニメーション用）
    // view:      ビュー行列
    // proj:      プロジェクション行列
    void Render(
        ID3D11DeviceContext* context,
        DirectX::XMFLOAT3 enemyPos,
        float ringSize,
        float time,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection
    );

private:
    // シェーダーをコンパイル＆作成
    bool CreateShaders(ID3D11Device* device);

    // クワッド（四角形）の頂点バッファを作成
    bool CreateQuad(ID3D11Device* device);

    // 定数バッファを作成
    bool CreateConstantBuffer(ID3D11Device* device);

    // === 頂点データ ===
    struct RingVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 TexCoord;
    };

    // === 定数バッファ（HLSLと完全一致）===
    struct RingCB
    {
        DirectX::XMMATRIX View;         // 64 bytes
        DirectX::XMMATRIX Projection;   // 64 bytes
        DirectX::XMFLOAT3 EnemyPos;     // 12 bytes
        float              RingSize;     //  4 bytes
        float              Time;         //  4 bytes
        DirectX::XMFLOAT3 Padding;      // 12 bytes
    };

    // === GPUリソース ===
    Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_ps;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11BlendState>    m_blendState;      // 加算ブレンド
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;  // 深度書き込みOFF
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_rasterState; // 両面描画

    bool m_initialized = false;
};