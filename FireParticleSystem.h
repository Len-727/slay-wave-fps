#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include "BezierCurve.h"

// ========================================
// 炎パーティクル構造体
// ========================================

struct FireParticle
{
    DirectX::XMFLOAT3 position;    // 位置
    DirectX::XMFLOAT3 velocity;    // 速度
    DirectX::XMFLOAT4 color;       // 色（RGBA）
    float size;                     // サイズ
    float lifetime;                 // 寿命（秒）
    float age;                      // 経過時間（秒）
    float curveT;                   // ベジェ曲線上の位置（0.0～1.0）
    bool active;                    // アクティブか
};

// ========================================
// 炎パーティクルシステムクラス
// ========================================

class FireParticleSystem
{
public:
    FireParticleSystem();
    ~FireParticleSystem();

    void Initialize(ID3D11Device* device, int maxParticles = 1000);
    void Update(float deltaTime);
    void Render(ID3D11DeviceContext* context, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

    // ベジェ曲線を設定
    void SetBezierCurve(
        DirectX::XMFLOAT3 p0,
        DirectX::XMFLOAT3 p1,
        DirectX::XMFLOAT3 p2,
        DirectX::XMFLOAT3 p3
    );

    // パーティクルの放出を開始/停止
    void StartEmitting();
    void StopEmitting();

    // パーティクル放出レート設定
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
    float m_emissionRate;        // パーティクル/秒
    float m_emissionAccumulator; // 放出タイマー

    BezierCurve m_bezierCurve;

    // DirectX リソース
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