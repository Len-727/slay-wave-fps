// StunRing.h - �X�^����Ԃ̓G�ɕ\������r���{�[�h�����O
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

    // �������i�f�o�C�X�쐬���1�񂾂��Ăԁj
    bool Initialize(ID3D11Device* device);

    // �����O��1�`��i�X�^�K�[���̓G���ƂɌĂԁj
    // enemyPos:  �G�̃��[���h�ʒu
    // ringSize:  �����O�̔��a�i�G�̃T�C�Y�ɍ��킹�ĕς���j
    // time:      �Q�[���o�ߎ��ԁi�A�j���[�V�����p�j
    // view:      �r���[�s��
    // proj:      �v���W�F�N�V�����s��
    void Render(
        ID3D11DeviceContext* context,
        DirectX::XMFLOAT3 enemyPos,
        float ringSize,
        float time,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection
    );

private:
    // �V�F�[�_�[���R���p�C�����쐬
    bool CreateShaders(ID3D11Device* device);

    // �N���b�h�i�l�p�`�j�̒��_�o�b�t�@���쐬
    bool CreateQuad(ID3D11Device* device);

    // �萔�o�b�t�@���쐬
    bool CreateConstantBuffer(ID3D11Device* device);

    // === ���_�f�[�^ ===
    struct RingVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT2 TexCoord;
    };

    // === �萔�o�b�t�@�iHLSL�Ɗ��S��v�j===
    struct RingCB
    {
        DirectX::XMMATRIX View;         // 64 bytes
        DirectX::XMMATRIX Projection;   // 64 bytes
        DirectX::XMFLOAT3 EnemyPos;     // 12 bytes
        float              RingSize;     //  4 bytes
        float              Time;         //  4 bytes
        DirectX::XMFLOAT3 Padding;      // 12 bytes
    };

    // === GPU���\�[�X ===
    Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_ps;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11BlendState>    m_blendState;      // ���Z�u�����h
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState;  // �[�x��������OFF
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_rasterState; // ���ʕ`��

    bool m_initialized = false;
};