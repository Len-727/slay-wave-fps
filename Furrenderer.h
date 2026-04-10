// =============================================================
// FurRenderer.h - ファー/苔レンダラー
//
// 【役割】シェル法でメッシュ上に毛/苔を描画するシステム
// 【使い方】
//   1. Initialize() で初期化
//   2. DrawGroundMoss() で地面の苔を描画
//   (将来) DrawFurOnModel() でボスの体毛を描画
// =============================================================
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;

// --- ファーシェーダーに渡す定数バッファ ---
// 【重要】HLSL側のcbufferと完全に一致させる必要がある
// 【重要】16バイトアラインメント必須（GPUの制約）
struct FurCB
{
    DirectX::XMFLOAT4X4 World;          // 64 bytes
    DirectX::XMFLOAT4X4 View;           // 64 bytes
    DirectX::XMFLOAT4X4 Projection;     // 64 bytes

    float FurLength;                     // 4 bytes
    float CurrentLayer;                  // 4 bytes
    float TotalLayers;                   // 4 bytes
    float Time;                          // 4 bytes  = 16 bytes

    DirectX::XMFLOAT3 WindDirection;     // 12 bytes
    float WindStrength;                  // 4 bytes  = 16 bytes

    DirectX::XMFLOAT3 BaseColor;         // 12 bytes
    float FurDensity;                    // 4 bytes  = 16 bytes

    DirectX::XMFLOAT3 TipColor;          // 12 bytes
    float Padding2;                      // 4 bytes  = 16 bytes

    DirectX::XMFLOAT3 LightDir;          // 12 bytes
    float AmbientStrength;               // 4 bytes  = 16 bytes
};

// --- 頂点フォーマット（地面のクアッド用）---
struct FurVertex
{
    DirectX::XMFLOAT3 Position;  // 位置
    DirectX::XMFLOAT3 Normal;    // 法線
    DirectX::XMFLOAT2 TexCoord;  // UV座標
};

class FurRenderer
{
public:
    FurRenderer();
    ~FurRenderer() = default;

    // Initialize - 初期化（シェーダーコンパイル＋地面メッシュ作成）
    // 【引数】device: D3Dデバイス
    // 【戻り値】true=成功, false=失敗
    bool Initialize(ID3D11Device* device);

    // DrawGroundMoss - 地面に苔を描画
    // 【引数】context: デバイスコンテキスト
    //        view: ビュー行列
    //        projection: 射影行列
    //        elapsedTime: ゲーム開始からの経過時間（風の揺れ用）
    void DrawGroundMoss(
        ID3D11DeviceContext* context,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection,
        float elapsedTime
    );

private:
    // --- シェーダーコンパイル ---
    bool CompileShaders(ID3D11Device* device);

    // --- 地面クアッドの作成 ---
    bool CreateGroundQuad(ID3D11Device* device);

    // --- GPUリソース ---
    ComPtr<ID3D11VertexShader>  m_vertexShader;   // 頂点シェーダー
    ComPtr<ID3D11PixelShader>   m_pixelShader;    // ピクセルシェーダー
    ComPtr<ID3D11InputLayout>   m_inputLayout;    // 入力レイアウト
    ComPtr<ID3D11Buffer>        m_constantBuffer;  // 定数バッファ
    ComPtr<ID3D11Buffer>        m_vertexBuffer;    // 地面の頂点バッファ
    ComPtr<ID3D11Buffer>        m_indexBuffer;     // 地面のインデックスバッファ

    // --- ブレンドステート（透明描画用）---
    ComPtr<ID3D11BlendState>        m_alphaBlendState;
    ComPtr<ID3D11RasterizerState>   m_noCullState;     // 両面描画
    ComPtr<ID3D11DepthStencilState> m_depthWriteOff;   // 深度書き込みOFF

    // --- 設定値 ---
    int m_shellCount;      // 層の数（16?32）
    float m_furLength;     // 苔の長さ
    float m_furDensity;    // 苔の密度
    int m_indexCount;      // インデックス数
};
