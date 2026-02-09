// ========================================
// TitleScene.h - タイトルシーンクラス（燻焼エフェクト版）
// ========================================

#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include <SpriteBatch.h>
#include <SpriteFont.h>
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
        ID3D11RenderTargetView* backBufferRTV,
        ID3D11DepthStencilView* depthStencilView
    );

    // === 燻焼制御 ===
    void StartBurning();           // 燃焼開始
    void StopBurning();            // 燃焼停止
    void ResetBurn();              // 燃焼リセット
    float GetBurnProgress() const { return m_burnProgress; }

    //  メニュー関連
    enum class MenuResult
    {
        None,
        Play,
        Settings,
        Exit
    };

    MenuResult GetMenuResult() const { return m_menuResult; }

    void ResetMenuResult() { m_menuResult = MenuResult::None; }

    // === 画面リサイズ対応 ===
    void OnResize(ID3D11Device* device, int newWidth, int newHeight);

    // === 血の滴りエフェクト ===
    struct BloodDrip
    {
        float x;          // X位置
        float y;          // Y位置（先端）
        float speed;      // 落下速度
        float length;     // 血の長さ
        float width;      // 血の幅
        float alpha;      // 透明度
        float delay;      // 開始遅延
        bool active;      // アクティブか
    };

    static const int MAX_BLOOD_DRIPS = 30;
    BloodDrip m_bloodDrips[MAX_BLOOD_DRIPS];

    // 1x1白テクスチャ（色付き矩形描画用）
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whiteTexture;

    void CreateWhiteTexture(ID3D11Device* device);
    void InitBloodDrips();
    void UpdateBloodDrips(float deltaTime);
    void RenderBloodDrips();

private:
    // 旗メッシュ
    std::unique_ptr<FlagMesh> m_flagMesh;

    // 炎パーティクルシステム
    std::unique_ptr<FireParticleSystem> m_fireParticleSystem;

    // シェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_burnPixelShader;  // 燻焼用シェーダー
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_matrixBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_timeBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_burnBuffer;  // 燻焼パラメータ用

    // テクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_flagTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_noiseTexture;  // ノイズテクスチャ
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

    // === ポストプロセス用 ===
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_renderTexture;
    // === FX用第2レンダーテクスチャ ===
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_fxSourceTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_fxSourceRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fxSourceSRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_renderTextureSRV;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_postProcessVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_blurPS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_menuFxPS;
    // === タイトル背景シェーダー ===
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_titleBgPS;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_titleBgBuffer;

    struct TitleBGBufferType
    {
        float time;
        float screenWidth;
        float screenHeight;
        float intensity;
    };

    // タイトルテキスト表示制御
    float m_titleAlpha;          // タイトル文字の透明度
    bool m_titleVisible;         // タイトル表示中か
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_menuFxBuffer;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_postProcessLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_fullscreenQuadVB;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_postProcessSampler;


    // ブラー強度
    float m_blurStrength;

    // 画面サイズ
    int m_screenWidth;
    int m_screenHeight;

    // 時間
    float m_time;

    // シェーダーパラメータ
    float m_waveSpeed;
    float m_waveAmplitude;

    // === 燻焼パラメータ ===
    bool m_isBurning;           // 燃焼中フラグ
    float m_burnProgress;       // 燃焼進行度（0.0?1.0）
    float m_burnSpeed;          // 燃焼速度
    float m_emberWidth;         // 燃え際の幅

    //  === 目～ニュー関連 ===
    enum class MenuState
    {
        Burning,    //  燃焼中
        FadeToMenu,   //  メニューへフェード
        Menu        //  メニュー表示中
    };

    MenuState m_menuState;
    int m_selectedMenuItem; //  選択中のメニュー項目(0 = PLAY, 1 = SETTHING, 2 = EXIT)
    float m_menuAlpha;  //  メニューの透明度
    float m_menuGlowTime;


    // スプライトバッチ（テキスト描画用）
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> m_menuFont;

    MenuResult m_menuResult;


    // 定数バッファ構造体
    struct MatrixBufferType
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    };

    struct MenuFXBufferType
    {
        float time;
        float shakeIntensity;
        float vignetteIntensity;
        float lightningIntensity;

        float chromaticAmount;
        float screenWidth;
        float screenHeight;
        float heartbeat;
    };

    struct TimeBufferType
    {
        float time;
        float waveSpeed;
        float waveAmplitude;
        float padding;
    };

    // 燻焼パラメータ用定数バッファ
    struct BurnBufferType
    {
        float burnProgress;     // 燃焼進行度
        float time;             // 時間
        float emberWidth;       // 燃え際の幅
        float burnDirection;    // 燃える方向
    };

    // ヘルパー関数
    void CreateShaders(ID3D11Device* device);
    void CreateBuffers(ID3D11Device* device);
    void CreateTexture(ID3D11Device* device);
    void CreateNoiseTexture(ID3D11Device* device);  // ノイズテクスチャ生成
    void CreatePostProcessResources(ID3D11Device* device);
    void CreatePostProcessShaders(ID3D11Device* device);
    void RenderToTexture(ID3D11DeviceContext* context);
    void ApplyBlur(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBufferRTV,
        ID3D11DepthStencilView* depthStencilView
    );

    void ApplyBlurTo(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* targetRTV,
        ID3D11DepthStencilView* depthStencilView
    );

    // メニュー描画
    void RenderMenu(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBufferRTV,
        ID3D11DepthStencilView* depthStencilView
    );

    // メニューFX適用（稲妻・ビネット等）
    void ApplyMenuFX(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBufferRTV,
        ID3D11DepthStencilView* depthStencilView
    );

    void RenderTitleBackground(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* targetRTV,
        ID3D11DepthStencilView* depthStencilView
    );
};