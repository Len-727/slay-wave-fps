// ============================================================
//  GPUParticleSystem.h
//  GPU Compute Shader�ɂ�錌�p�[�e�B�N���V�X�e��
//  + �X�N���[���X�y�[�X���̃����_�����O(�}���`�p�X)
//
//  �y�����̗���(���̃��[�h)�z
//  Pass 0: Compute Shader - �������Z(�d��, ��C��R, ���Փ�)
//  Pass 1: �[�x�p�X     - �p�[�e�B�N�������ʐ[�x��R32�e�N�X�`���ɕ`��
//  Pass 2: �u���[�p�X   - �o�C���e�����u���[�Ő[�x�����炩��(�����Z��)
//  Pass 3: �����p�X     - �@������ �� PBR���C�e�B���O �� �t��!
// ============================================================
#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

class GPUParticleSystem
{
public:
    // --- �p�[�e�B�N���f�[�^(HLSL�̍\���̂Ɗ��S��v) ---
    struct Particle
    {
        DirectX::XMFLOAT3 position;    // ���[���h���W        12�o�C�g
        float              life;        // �c�����(�b)         4�o�C�g
        DirectX::XMFLOAT3 velocity;    // �ړ�����+���x       12�o�C�g
        float              maxLife;     // ��������(�t�F�[�h�p)  4�o�C�g
        float              size;        // �r���{�[�h�̑傫��    4�o�C�g
        DirectX::XMFLOAT3 padding;     // �A���C�������g�p     12�o�C�g
    };  // ���v: 48�o�C�g

    GPUParticleSystem();
    ~GPUParticleSystem() = default;

    // ������(screenWidth/Height�͗��̃e�N�X�`���p)
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
        int maxParticles = 4096, int screenWidth = 1920, int screenHeight = 1080);

    // �p�[�e�B�N������
    void Emit(DirectX::XMFLOAT3 position, int count, float power = 5.0f,
        float sizeMin = 0.08f, float sizeMax = 0.25f);

    void EmitSplash(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 direction,
        int count = 8, float power = 4.0f);

    void EmitMist(DirectX::XMFLOAT3 position, int count = 120, float power = 2.0f);

    // ���t���[���X�V(CS���s)
    void Update(float deltaTime);

    // �ʏ�`��(�]���̃r���{�[�h����)
    void Draw(DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::XMFLOAT3 cameraPos);

    // ���̕`��(�}���`�p�X)
    void DrawFluid(DirectX::XMMATRIX view, DirectX::XMMATRIX proj,
        DirectX::XMFLOAT3 cameraPos,
        ID3D11ShaderResourceView* sceneColorSRV,
        ID3D11RenderTargetView* finalRTV);

    // ���̃��[�hON/OFF
    void SetFluidEnabled(bool enabled) { m_fluidEnabled = enabled; }
    // ���̍�����ݒ�i���b�V���̒n�ʂɍ��킹��j
    void SetFloorY(float y) { m_floorY = y; }
    bool IsFluidEnabled() const { return m_fluidEnabled; }

    int GetActiveCount() const { return m_activeCount; }
    void OnResize(int width, int height);

private:
    // --- ����: �p�[�e�B�N��GPU���\�[�X ---
    Microsoft::WRL::ComPtr<ID3D11Buffer>              m_particleBuffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>  m_particleUAV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   m_particleSRV;
    Microsoft::WRL::ComPtr<ID3D11Buffer>              m_stagingBuffer;

    // --- �����V�F�[�_�[ ---
    Microsoft::WRL::ComPtr<ID3D11ComputeShader>  m_computeShader;   // �������ZCS
    Microsoft::WRL::ComPtr<ID3D11VertexShader>   m_vertexShader;    // �r���{�[�hVS
    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_pixelShader;     // �ʏ�`��PS

    // ---  ���̃V�F�[�_�[ ---

    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_fluidDepthPS;    // �[�xPS
    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_fluidBlurPS;     // �u���[PS
    Microsoft::WRL::ComPtr<ID3D11PixelShader>    m_fluidCompositePS;// ����PS
    Microsoft::WRL::ComPtr<ID3D11VertexShader>   m_fullscreenVS;    // �t���X�N���[��VS
    bool m_fluidShadersReady = false;  // ���̃V�F�[�_�[�̃R���p�C�������t���O


    // ---  ���̃e�N�X�`�� ---
    // �[�x�e�N�X�`��(R32_FLOAT: �p�[�e�B�N���[�x��ۑ�)
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_fluidDepthTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_fluidDepthRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fluidDepthSRV;


    // �u���[���ԃe�N�X�`��(�����u���[���ʂ��ꎞ�ۑ�)
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_blurTempTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_blurTempRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_blurTempSRV;

    // ---  ���̒萔�o�b�t�@ ---
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurCB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_compositeCB;

    // ---  ���̃X�e�[�g ---
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pointSampler;

    // --- �����o�b�t�@/�X�e�[�g ---
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_updateCB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cameraCB;
    Microsoft::WRL::ComPtr<ID3D11BlendState>        m_blendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bloodFlipbookSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bloodMistSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bloodSplashSRV;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_linearSampler;

    // --- CPU�� ---
    std::vector<Particle> m_emitQueue;

    // --- �ݒ� ---
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

    // --- �萔�o�b�t�@�\����(HLSL��v) ---
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
        DirectX::XMFLOAT2 TexelSize;       // 1�s�N�Z����UV�T�C�Y
        float             BlurRadius;       // �u���[���a
        float             DepthThreshold;   // �G�b�W�ی�臒l
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

    // --- �X�e�[�g�ۑ�/���� ---
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

    // --- �����w���p�[ ---
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
