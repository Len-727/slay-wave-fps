// ============================================================
//  GPUParticleSystem.h
//  GPU Compute Shaderによる血パーティクルシステム
//  + スクリーンスペース流体レンダリング(マルチパス)
//
//  【処理の流れ(流体モード)】
//  Pass 0: Compute Shader - 物理演算(重力, 空気抵抗, 床衝突)
//  Pass 1: 深度パス     - パーティクルを球面深度でR32テクスチャに描画
//  Pass 2: ブラーパス   - バイラテラルブラーで深度を滑らかに(粒が融合)
//  Pass 3: 合成パス     - 法線復元 → PBRライティング → 液体!
// ============================================================
#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

class GPUParticleSystem
{
public:
    // --- パーティクルデータ(HLSLの構造体と完全一致) ---
    struct Particle
    {
        DirectX::XMFLOAT3 position;    // ワールド座標        12バイト
        float              life;        // 残り寿命(秒)         4バイト
        DirectX::XMFLOAT3 velocity;    // 移動方向+速度       12バイト
        float              maxLife;     // 初期寿命(フェード用)  4バイト
        float              size;        // ビルボードの大きさ    4バイト
        DirectX::XMFLOAT3 padding;     // アラインメント用     12バイト
    };  // 合計: 48バイト

    GPUParticleSystem();
    ~GPUParticleSystem() = default;

    // 初期化(screenWidth/Heightは流体テクスチャ用)
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
        int maxParticles = 4096, int screenWidth = 1920, int screenHeight = 1080);

    // パーティクル発射
    void Emit(DirectX::XMFLOAT3 position, int count, float power = 5.0f,
        float sizeMin = 0.08f, float sizeMax = 0.25f);

    void EmitSplash(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 direction,
        int count = 8, float power = 4.0f);

    void EmitMist(DirectX::XMFLOAT3 position, int count = 120, float power = 2.0f);

    // 毎フレーム更新(CS実行)
    void Update(float deltaTime);

    // 通常描画(従来のビルボード方式)
    void Draw(DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::XMFLOAT3 cameraPos);

    // 流体描画(マルチパス)
    void DrawFluid(DirectX::XMMATRIX view, DirectX::XMMATRIX proj,
        DirectX::XMFLOAT3 cameraPos,
        ID3D11ShaderResourceView* sceneColorSRV,
        ID3D11RenderTargetView* finalRTV);

    // 流体モードON/OFF
    void SetFluidEnabled(bool enabled) { m_fluidEnabled = enabled; }
    // 床の高さを設定（メッシュの地面に合わせる）
    void SetFloorY(float y) { m_floorY = y; }
    bool IsFluidEnabled() const { return m_fluidEnabled; }

    int GetActiveCount() const { return m_activeCount; }
    void OnResize(int width, int height);

private:
    // --- 既存: パーティクルGPUリソース ---
    Microsoft::WRL::ComPtr<ID3D11Buffer>              m_particleBuffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>  m_particleUAV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   m_particleSRV;
    Microsoft::WRL::ComPtr<ID3D11Buffer>              m_stagingBuffer;

    // --- 既存シェーダー ---
    Microsoft::WRL::ComPtr<ID3D11ComputeShader>  m_computeShader;   // 物理演算CS
    Microsoft::WRL::ComPtr<ID3D11VertexShader>   m_vertexShader;    // ビルボードVS
    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_pixelShader;     // 通常描画PS

    // ---  流体シェーダー ---

    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_fluidDepthPS;    // 深度PS
    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_fluidBlurPS;     // ブラーPS
    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_fluidCompositePS;// 合成PS
    Microsoft::WRL::ComPtr<ID3D11VertexShader>   m_fullscreenVS;    // フルスクリーンVS
    bool m_fluidShadersReady = false;  // 流体シェーダーのコンパイル成功フラグ


    // ---  流体テクスチャ ---
    // 深度テクスチャ(R32_FLOAT: パーティクル深度を保存)
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_fluidDepthTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_fluidDepthRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fluidDepthSRV;


    // ブラー中間テクスチャ(水平ブラー結果を一時保存)
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_blurTempTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_blurTempRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_blurTempSRV;

    // ---  流体定数バッファ ---
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurCB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_compositeCB;

    // ---  流体ステート ---
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pointSampler;

    // --- 既存バッファ/ステート ---
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_updateCB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cameraCB;
    Microsoft::WRL::ComPtr<ID3D11BlendState>        m_blendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bloodFlipbookSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bloodMistSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bloodSplashSRV;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_linearSampler;

    // --- CPU側 ---
    std::vector<Particle> m_emitQueue;

    // --- 設定 ---
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    int   m_maxParticles = 4096;
    int   m_activeCount = 0;
    int   m_nextSlot = 0;
    float m_totalTime = 0.0f;
    int   m_screenWidth = 1280;
    int   m_screenHeight = 960;
    bool  m_fluidEnabled = false;
    float m_floorY = 0.02f;

    // --- 定数バッファ構造体(HLSL一致) ---
    struct alignas(16) UpdateCBData
    {
        float DeltaTime, Gravity, Drag, FloorY;
        float BounceFactor, Time;
        float Padding[2];
    };

    struct alignas(16) CameraCBData
    {
        DirectX::XMFLOAT4X4 View;
        DirectX::XMFLOAT4X4 Projection;
        DirectX::XMFLOAT3   CameraRight;
        float                Time;
        DirectX::XMFLOAT3   CameraUp;
        float                SizeScale;
    };

    struct alignas(16) BlurCBData
    {
        DirectX::XMFLOAT2 TexelSize;       // 1ピクセルのUVサイズ
        float             BlurRadius;       // ブラー半径
        float             DepthThreshold;   // エッジ保護閾値
        DirectX::XMFLOAT2 BlurDirection;   // (1,0)or(0,1)
        float             Padding[2];
    };

    struct alignas(16) CompositeCBData
    {
        DirectX::XMFLOAT4X4 InvProjection;
        DirectX::XMFLOAT2   TexelSize;
        float                Time;
        float                Thickness;
        DirectX::XMFLOAT3   LightDir;
        float                SpecularPower;
        DirectX::XMFLOAT3   CameraPos;
        float                FluidAlpha;
    };

    // --- ステート保存/復元 ---
    struct SavedState
    {
        Microsoft::WRL::ComPtr<ID3D11InputLayout>       IL;
        D3D11_PRIMITIVE_TOPOLOGY                        Topology;
        Microsoft::WRL::ComPtr<ID3D11Buffer>            IB;
        DXGI_FORMAT IBFormat; UINT IBOffset;
        Microsoft::WRL::ComPtr<ID3D11Buffer>            VB;
        UINT VBStride, VBOffset;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>      VS;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> VS_SRV;
        Microsoft::WRL::ComPtr<ID3D11Buffer>            VS_CB;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>       PS;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PS_SRV0, PS_SRV1;
        Microsoft::WRL::ComPtr<ID3D11Buffer>            PS_CB;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>      PS_Sampler;
        Microsoft::WRL::ComPtr<ID3D11BlendState>        Blend;
        float BlendFactor[4]; UINT SampleMask;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> Depth;
        UINT StencilRef;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState>   RS;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  RTV;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  DSV;
    };
    void SaveState(SavedState& s);
    void RestoreState(const SavedState& s);

    // --- 内部ヘルパー ---
    bool CompileShaders();
    bool CompileFluidShaders();
    bool CreateBuffers();
    bool CreateStates();
    bool CreateFluidResources();
    void UploadEmitQueue();
    void FluidDepthPass(DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::XMFLOAT3 cameraPos);
    void FluidBlurPass();
    void FluidCompositePass(DirectX::XMMATRIX proj, DirectX::XMFLOAT3 cameraPos,
        ID3D11ShaderResourceView* sceneColorSRV, ID3D11RenderTargetView* finalRTV);
};
