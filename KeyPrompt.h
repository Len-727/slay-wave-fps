// ============================================================
//  KeyPrompt.h - �L�[�v�����v�g �r���{�[�h
//
//  �y�ړI�z�X�^�K�[���̓G�ɁuF�v�L�[�A�C�R����3D��Ԃŕ\��
//  �y�d�g�݁zStunRingVS.cso ���ė��p�����e�N�X�`���r���{�[�h
//    - �[�x�e�X�g���� �� �ǂɉB���
//    - �A���t�@�u�����h �� �������̉������R�ɗn������
//    - �r���{�[�h �� ��ɃJ����������
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

    // �������i�f�o�C�X�쐬���1�񂾂��Ăԁj
    // texturePath: �e�N�X�`���t�@�C���̃p�X�i��: L"Assets/Texture/HUD/f_prompt.png"�j
    bool Initialize(ID3D11Device* device, const wchar_t* texturePath);

    // �r���{�[�h��1�`��
    // worldPos:  �\�����郏�[���h���W
    // size:      �r���{�[�h�̔��a
    // time:      �A�j���[�V�����p�̌o�ߎ���
    // view:      �r���[�s��
    // proj:      �v���W�F�N�V�����s��
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

    // === ���_�f�[�^�iStunRing�Ɠ����\���j===
    struct BillboardVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 TexCoord;
    };

    // === �萔�o�b�t�@�iStunRingVS �Ɗ��S��v�j===
    struct PromptCB
    {
        DirectX::XMMATRIX View;         // 64 bytes
        DirectX::XMMATRIX Projection;   // 64 bytes
        DirectX::XMFLOAT3 EnemyPos;     // 12 bytes
        float              Size;         //  4 bytes
        float              Time;         //  4 bytes
        DirectX::XMFLOAT3 Padding;      // 12 bytes
    };

    // === GPU���\�[�X ===
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
