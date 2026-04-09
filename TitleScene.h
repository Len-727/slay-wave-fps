// ========================================
// TitleScene.h - �^�C�g���V�[���N���X�i���ăG�t�F�N�g�Łj
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

    // ������
    void Initialize(
        ID3D11Device* device,
        int screenWidth,
        int screenHeight
    );

    // �X�V
    void Update(float deltaTime);

    // �`��
    void Render(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBufferRTV,
        ID3D11DepthStencilView* depthStencilView
    );

    // === ���Đ��� ===
    void StartBurning();           // �R�ĊJ�n
    void StopBurning();            // �R�Ē�~
    void ResetBurn();              // �R�ă��Z�b�g
    float GetBurnProgress() const { return m_burnProgress; }

    //  ���j���[�֘A
    enum class MenuResult
    {
        None,
        Play,
        Settings,
        Exit
    };

    MenuResult GetMenuResult() const { return m_menuResult; }

    void ResetMenuResult() { m_menuResult = MenuResult::None; }

    // === ��ʃ��T�C�Y�Ή� ===
    void OnResize(ID3D11Device* device, int newWidth, int newHeight);

    // === ���̓H��G�t�F�N�g ===
    struct BloodDrip
    {
        float x;          // X�ʒu
        float y;          // Y�ʒu�i��[�j
        float speed;      // �������x
        float length;     // ���̒���
        float width;      // ���̕�
        float alpha;      // �����x
        float delay;      // �J�n�x��
        bool active;      // �A�N�e�B�u��
    };

    static const int MAX_BLOOD_DRIPS = 30;
    BloodDrip m_bloodDrips[MAX_BLOOD_DRIPS];

    // 1x1���e�N�X�`���i�F�t����`�`��p�j
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whiteTexture;

    void CreateWhiteTexture(ID3D11Device* device);
    void InitBloodDrips();
    void UpdateBloodDrips(float deltaTime);
    void RenderBloodDrips();

private:
    // �����b�V��
    std::unique_ptr<FlagMesh> m_flagMesh;

    // ���p�[�e�B�N���V�X�e��
    std::unique_ptr<FireParticleSystem> m_fireParticleSystem;

    // �V�F�[�_�[
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_burnPixelShader;  // ���ėp�V�F�[�_�[
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    // �萔�o�b�t�@
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_matrixBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_timeBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_burnBuffer;  // ���ăp�����[�^�p

    // �e�N�X�`��
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_flagTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_noiseTexture;  // �m�C�Y�e�N�X�`��
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

    // === �|�X�g�v���Z�X�p ===
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_renderTexture;
    // === FX�p��2�����_�[�e�N�X�`�� ===
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_fxSourceTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_fxSourceRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fxSourceSRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_renderTextureSRV;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_postProcessVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_blurPS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_menuFxPS;
    // === �^�C�g���w�i�V�F�[�_�[ ===
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_titleBgPS;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_titleBgBuffer;

    struct TitleBGBufferType
    {
        float time;
        float screenWidth;
        float screenHeight;
        float intensity;
    };

    // �^�C�g���e�L�X�g�\������
    float m_titleAlpha;          // �^�C�g�������̓����x
    bool m_titleVisible;         // �^�C�g���\������
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_menuFxBuffer;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_postProcessLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_fullscreenQuadVB;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_postProcessSampler;


    // �u���[���x
    float m_blurStrength;

    // ��ʃT�C�Y
    int m_screenWidth;
    int m_screenHeight;

    // ����
    float m_time;

    // �V�F�[�_�[�p�����[�^
    float m_waveSpeed;
    float m_waveAmplitude;

    // === ���ăp�����[�^ ===
    bool m_isBurning;           // �R�Ē��t���O
    float m_burnProgress;       // �R�Đi�s�x�i0.0?1.0�j
    float m_burnSpeed;          // �R�đ��x
    float m_emberWidth;         // �R���ۂ̕�

    //  === �ځ`�j���[�֘A ===
    enum class MenuState
    {
        Burning,    //  �R�Ē�
        FadeToMenu,   //  ���j���[�փt�F�[�h
        Menu        //  ���j���[�\����
    };

    MenuState m_menuState;
    int m_selectedMenuItem; //  �I�𒆂̃��j���[����(0 = PLAY, 1 = SETTHING, 2 = EXIT)
    float m_menuAlpha;  //  ���j���[�̓����x
    float m_menuGlowTime;


    // �X�v���C�g�o�b�`�i�e�L�X�g�`��p�j
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> m_menuFont;

    MenuResult m_menuResult;


    // �萔�o�b�t�@�\����
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

    // ���ăp�����[�^�p�萔�o�b�t�@
    struct BurnBufferType
    {
        float burnProgress;     // �R�Đi�s�x
        float time;             // ����
        float emberWidth;       // �R���ۂ̕�
        float burnDirection;    // �R�������
    };

    // �w���p�[�֐�
    void CreateShaders(ID3D11Device* device);
    void CreateBuffers(ID3D11Device* device);
    void CreateTexture(ID3D11Device* device);
    void CreateNoiseTexture(ID3D11Device* device);  // �m�C�Y�e�N�X�`������
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

    // ���j���[�`��
    void RenderMenu(
        ID3D11DeviceContext* context,
        ID3D11RenderTargetView* backBufferRTV,
        ID3D11DepthStencilView* depthStencilView
    );

    // ���j���[FX�K�p�i��ȁE�r�l�b�g���j
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