// ========================================
// TitleScene.h - タイトルシーンクラス（簡易版）
// ========================================

#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include "FlagMesh.h"
#include "FireParticleSystem.h"

class TitleScene
{
public:
    TitleScene();
    ~TitleScene();

    // 初期化
    void Initialize(
        ID3D11Device* device,
        int screenWidth,
        int screenHeight
    );

    // 更新
    void Update(float deltaTime);

    // 描画
    void Render(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBufferRTV,      // 追加
        ID3D11DepthStencilView* depthStencilView    // 追加
    );

private:
    // 旗メッシュ
    std::unique_ptr<FlagMesh> m_flagMesh;

    //  炎パーティクルシステム
    std::unique_ptr<FireParticleSystem> m_fireParticleSystem;

    // シェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_matrixBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_timeBuffer;

    // テクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_flagTexture;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;


    //  === ポストプロセス用    ===
    
    // レンダーテクスチャ
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_renderTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_renderTextureSRV;

    // ポストプロセス用シェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_postProcessVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_blurPS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_postProcessLayout;

    // フルスクリーンクアッド用頂点バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_fullscreenQuadVB;

    // ポストプロセス用サンプラー
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_postProcessSampler;

    // ブラー強度（0.0?1.0）
    float m_blurStrength;

    // 画面サイズ
    int m_screenWidth;
    int m_screenHeight;

    // 時間
    float m_time;

    // シェーダーパラメータ
    float m_waveSpeed;
    float m_waveAmplitude;

    // 定数バッファ構造体
    struct MatrixBufferType
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    };

    struct TimeBufferType
    {
        float time;
        float waveSpeed;
        float waveAmplitude;
        float padding;
    };


    void CreatePostProcessResources(ID3D11Device* device);
    void CreatePostProcessShaders(ID3D11Device* device);
    void RenderToTexture(ID3D11DeviceContext* context);
    void ApplyBlur(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBufferRTV,      // 追加
        ID3D11DepthStencilView* depthStencilView    // 追加
    );


    // ヘルパー関数
    void CreateShaders(ID3D11Device* device);
    void CreateBuffers(ID3D11Device* device);
    void CreateTexture(ID3D11Device* device);
};