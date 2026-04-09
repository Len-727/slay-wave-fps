// ============================================================
//  KeyPrompt.h - キープロンプト ビルボード
//
//  【目的】スタガー中の敵に「F」キーアイコンを3D空間で表示
//  【仕組み】StunRingVS.cso を再利用したテクスチャビルボード
//    - 深度テストあり → 壁に隠れる
//    - アルファブレンド → 半透明の縁が自然に溶け込む
//    - ビルボード → 常にカメラを向く
// ============================================================
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>

class KeyPrompt
{
public:
    KeyPrompt();
    ~KeyPrompt();

    // 初期化（デバイス作成後に1回だけ呼ぶ）
    // texturePath: テクスチャファイルのパス（例: L"Assets/Texture/HUD/f_prompt.png"）
    bool Initialize(ID3D11Device* device, const wchar_t* texturePath);

    // ビルボードを1つ描画
    // worldPos:  表示するワールド座標
    // size:      ビルボードの半径
    // time:      アニメーション用の経過時間
    // view:      ビュー行列
    // proj:      プロジェクション行列
    void Render(
        ID3D11DeviceContext* context,
        DirectX::XMFLOAT3 worldPos,
        float size,
        float time,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection);

private:
    bool CreateShaders(ID3D11Device* device);
    bool CreateQuad(ID3D11Device* device);
    bool CreateConstantBuffer(ID3D11Device* device);
    bool LoadTexture(ID3D11Device* device, const wchar_t* path);

    // === 頂点データ（StunRingと同じ構造）===
    struct BillboardVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 TexCoord;
    };

    // === 定数バッファ（StunRingVS と完全一致）===
    struct PromptCB
    {
        DirectX::XMMATRIX View;         // 64 bytes
        DirectX::XMMATRIX Projection;   // 64 bytes
        DirectX::XMFLOAT3 EnemyPos;     // 12 bytes
        float              Size;         //  4 bytes
        float              Time;         //  4 bytes
        DirectX::XMFLOAT3 Padding;      // 12 bytes
    };

    // === GPUリソース ===
    Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_ps;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>      m_sampler;
    Microsoft::WRL::ComPtr<ID3D11BlendState>        m_blendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_rasterState;

    bool m_initialized = false;
};
