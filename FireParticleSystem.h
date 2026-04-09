#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include "BezierCurve.h"

// ========================================
// ���p�[�e�B�N���\����
// ========================================

struct FireParticle
{
    DirectX::XMFLOAT3 position;    // �ʒu
    DirectX::XMFLOAT3 velocity;    // ���x
    DirectX::XMFLOAT4 color;       // �F�iRGBA�j
    float size;                     // �T�C�Y
    float lifetime;                 // �����i�b�j
    float age;                      // �o�ߎ��ԁi�b�j
    float curveT;                   // �x�W�F�Ȑ���̈ʒu�i0.0�`1.0�j
    bool active;                    // �A�N�e�B�u��
};

// ========================================
// ���p�[�e�B�N���V�X�e���N���X
// ========================================

class FireParticleSystem
{
public:
    FireParticleSystem();
    ~FireParticleSystem();

    void Initialize(ID3D11Device* device, int maxParticles = 1000);
    void Update(float deltaTime);
    void Render(ID3D11DeviceContext* context, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

    // �x�W�F�Ȑ���ݒ�
    void SetBezierCurve(
        DirectX::XMFLOAT3 p0,
        DirectX::XMFLOAT3 p1,
        DirectX::XMFLOAT3 p2,
        DirectX::XMFLOAT3 p3
    );

    // �p�[�e�B�N���̕��o���J�n/��~
    void StartEmitting();
    void StopEmitting();

    // �p�[�e�B�N�����o���[�g�ݒ�
    void SetEmissionRate(float particlesPerSecond);

private:
    void EmitParticle();
    void UpdateParticle(FireParticle& particle, float deltaTime);
    void CreateBuffers(ID3D11Device* device);
    void CreateShaders(ID3D11Device* device);
    void CreateTexture(ID3D11Device* device);
    void CreateBlendState(ID3D11Device* device);

    std::vector<FireParticle> m_particles;
    int m_maxParticles;
    bool m_isEmitting;
    float m_emissionRate;        // �p�[�e�B�N��/�b
    float m_emissionAccumulator; // ���o�^�C�}�[

    BezierCurve m_bezierCurve;

    // DirectX ���\�[�X
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_matrixBuffer;
};