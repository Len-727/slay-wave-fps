// =============================================================
// FurRenderer.h - �t�@�[/�ۃ����_���[
//
// �y�����z�V�F���@�Ń��b�V����ɖ�/�ۂ�`�悷��V�X�e��
// �y�g�����z
//   1. Initialize() �ŏ�����
//   2. DrawGroundMoss() �Œn�ʂ̑ۂ�`��
//   (����) DrawFurOnModel() �Ń{�X�̖̑т�`��
// =============================================================
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;

// --- �t�@�[�V�F�[�_�[�ɓn���萔�o�b�t�@ ---
// �y�d�v�zHLSL����cbuffer�Ɗ��S�Ɉ�v������K�v������
// �y�d�v�z16�o�C�g�A���C�������g�K�{�iGPU�̐���j
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

// --- ���_�t�H�[�}�b�g�i�n�ʂ̃N�A�b�h�p�j---
struct FurVertex
{
    DirectX::XMFLOAT3 Position;  // �ʒu
    DirectX::XMFLOAT3 Normal;    // �@��
    DirectX::XMFLOAT2 TexCoord;  // UV���W
};

class FurRenderer
{
public:
    FurRenderer();
    ~FurRenderer() = default;

    // Initialize - �������i�V�F�[�_�[�R���p�C���{�n�ʃ��b�V���쐬�j
    // �y�����zdevice: D3D�f�o�C�X
    // �y�߂�l�ztrue=����, false=���s
    bool Initialize(ID3D11Device* device);

    // DrawGroundMoss - �n�ʂɑۂ�`��
    // �y�����zcontext: �f�o�C�X�R���e�L�X�g
    //        view: �r���[�s��
    //        projection: �ˉe�s��
    //        elapsedTime: �Q�[���J�n����̌o�ߎ��ԁi���̗h��p�j
    void DrawGroundMoss(
        ID3D11DeviceContext* context,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection,
        float elapsedTime
    );

private:
    // --- �V�F�[�_�[�R���p�C�� ---
    bool CompileShaders(ID3D11Device* device);

    // --- �n�ʃN�A�b�h�̍쐬 ---
    bool CreateGroundQuad(ID3D11Device* device);

    // --- GPU���\�[�X ---
    ComPtr<ID3D11VertexShader>  m_vertexShader;   // ���_�V�F�[�_�[
    ComPtr<ID3D11PixelShader>   m_pixelShader;    // �s�N�Z���V�F�[�_�[
    ComPtr<ID3D11InputLayout>   m_inputLayout;    // ���̓��C�A�E�g
    ComPtr<ID3D11Buffer>        m_constantBuffer;  // �萔�o�b�t�@
    ComPtr<ID3D11Buffer>        m_vertexBuffer;    // �n�ʂ̒��_�o�b�t�@
    ComPtr<ID3D11Buffer>        m_indexBuffer;     // �n�ʂ̃C���f�b�N�X�o�b�t�@

    // --- �u�����h�X�e�[�g�i�����`��p�j---
    ComPtr<ID3D11BlendState>        m_alphaBlendState;
    ComPtr<ID3D11RasterizerState>   m_noCullState;     // ���ʕ`��
    ComPtr<ID3D11DepthStencilState> m_depthWriteOff;   // �[�x��������OFF

    // --- �ݒ�l ---
    int m_shellCount;      // �w�̐��i16?32�j
    float m_furLength;     // �ۂ̒���
    float m_furDensity;    // �ۂ̖��x
    int m_indexCount;      // �C���f�b�N�X��
};
