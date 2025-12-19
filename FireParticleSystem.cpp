#include "FireParticleSystem.h"
#include <d3dcompiler.h>
#include <random>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// ========================================
// ランダム生成器
// ========================================

static std::random_device rd;
static std::mt19937 gen(rd());

static float RandomFloat(float min, float max)
{
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

// ========================================
// コンストラクタ / デストラクタ
// ========================================

FireParticleSystem::FireParticleSystem()
    : m_maxParticles(1000)
    , m_isEmitting(false)
    , m_emissionRate(100.0f)  // 100パーティクル/秒
    , m_emissionAccumulator(0.0f)
{
}

FireParticleSystem::~FireParticleSystem()
{
}

// ========================================
// 初期化
// ========================================

void FireParticleSystem::Initialize(ID3D11Device* device, int maxParticles)
{
    m_maxParticles = maxParticles;
    m_particles.reserve(m_maxParticles);

    // デフォルトのベジェ曲線（左下から右上）
    SetBezierCurve(
        XMFLOAT3(-1.0f, -1.0f, 0.0f),  // 左下
        XMFLOAT3(-0.5f, 0.0f, 0.0f),  // 制御点1
        XMFLOAT3(0.5f, 0.5f, 0.0f),  // 制御点2
        XMFLOAT3(1.0f, 1.0f, 0.0f)   // 右上
    );

    CreateBuffers(device);
    CreateShaders(device);
    CreateTexture(device);
    CreateBlendState(device);
}

// ========================================
// ベジェ曲線設定
// ========================================

void FireParticleSystem::SetBezierCurve(
    XMFLOAT3 p0,
    XMFLOAT3 p1,
    XMFLOAT3 p2,
    XMFLOAT3 p3)
{
    m_bezierCurve.SetControlPoints(p0, p1, p2, p3);
}

// ========================================
// 放出制御
// ========================================

void FireParticleSystem::StartEmitting()
{
    m_isEmitting = true;
}

void FireParticleSystem::StopEmitting()
{
    m_isEmitting = false;
}

void FireParticleSystem::SetEmissionRate(float particlesPerSecond)
{
    m_emissionRate = particlesPerSecond;
}

// ========================================
// パーティクル生成
// ========================================

void FireParticleSystem::EmitParticle()
{
    // 最大数チェック
    if (m_particles.size() >= m_maxParticles)
        return;

    FireParticle particle;

    // === 初期位置（ベジェ曲線の開始点）===
    particle.curveT = 0.0f;
    particle.position = m_bezierCurve.GetPosition(0.0f);

    // === 初期速度（曲線の接線方向）===
    XMFLOAT3 tangent = m_bezierCurve.GetTangent(0.0f);
    float speed = RandomFloat(0.5f, 1.0f);
    particle.velocity = XMFLOAT3(
        tangent.x * speed,
        tangent.y * speed,
        tangent.z * speed
    );

    // === 色（オレンジ～黄色）===
    // XMFLOAT4は x=赤, y=緑, z=青, w=アルファ
    float r = RandomFloat(0.8f, 1.0f);   // 赤
    float g = RandomFloat(0.3f, 0.8f);   // 緑
    float b = RandomFloat(0.0f, 0.2f);   // 青（ほぼ0）
    float a = RandomFloat(0.7f, 1.0f);   // アルファ
    particle.color = XMFLOAT4(r, g, b, a);

    // === サイズ ===
    particle.size = RandomFloat(0.05f, 0.15f);

    // === 寿命 ===
    particle.lifetime = RandomFloat(2.0f, 3.0f);
    particle.age = 0.0f;

    // === アクティブ化 ===
    particle.active = true;

    m_particles.push_back(particle);
}

// ========================================
// 更新
// ========================================

void FireParticleSystem::Update(float deltaTime)
{
    // === パーティクル放出 ===
    if (m_isEmitting)
    {
        m_emissionAccumulator += deltaTime * m_emissionRate;

        while (m_emissionAccumulator >= 1.0f)
        {
            EmitParticle();
            m_emissionAccumulator -= 1.0f;
        }
    }

    // === 既存パーティクルの更新 ===
    for (auto it = m_particles.begin(); it != m_particles.end();)
    {
        if (it->active)
        {
            UpdateParticle(*it, deltaTime);

            // 寿命チェック
            if (it->age >= it->lifetime)
            {
                it->active = false;
            }
        }

        // 非アクティブなパーティクルを削除
        if (!it->active)
        {
            it = m_particles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void FireParticleSystem::UpdateParticle(FireParticle& particle, float deltaTime)
{
    // === 経過時間を増やす ===
    particle.age += deltaTime;

    // === ベジェ曲線上を移動 ===
    particle.curveT += deltaTime * 0.3f;  // 3秒で全行程
    if (particle.curveT > 1.0f)
        particle.curveT = 1.0f;

    // 新しい位置を取得
    particle.position = m_bezierCurve.GetPosition(particle.curveT);

    // === ランダムな揺らぎ（炎らしさ）===
    particle.position.x += RandomFloat(-0.02f, 0.02f);
    particle.position.y += RandomFloat(-0.01f, 0.03f);

    // === サイズを徐々に大きく ===
    particle.size += deltaTime * 0.05f;

    // === アルファをフェードアウト ===
    float lifeRatio = particle.age / particle.lifetime;
    particle.color.w = 1.0f - lifeRatio;  // w=アルファ: 1.0 → 0.0

    // === 色を黄色→赤→黒に変化 ===
    // 若い: 黄色（明るい）
    // 古い: 赤→黒（暗い）
    particle.color.y = (1.0f - lifeRatio) * 0.6f;  // y=緑を減らす
    particle.color.x = 1.0f - lifeRatio * 0.3f;    // x=赤を少し減らす
}

// ========================================
// バッファ作成（簡易版）
// ========================================

void FireParticleSystem::CreateBuffers(ID3D11Device* device)
{
    // TODO: 実装（次のステップで）
    OutputDebugStringA("[FIRE] CreateBuffers: TODO\n");
}

void FireParticleSystem::CreateShaders(ID3D11Device* device)
{
    // TODO: 実装（次のステップで）
    OutputDebugStringA("[FIRE] CreateShaders: TODO\n");
}

void FireParticleSystem::CreateTexture(ID3D11Device* device)
{
    // TODO: 実装（次のステップで）
    OutputDebugStringA("[FIRE] CreateTexture: TODO\n");
}

void FireParticleSystem::CreateBlendState(ID3D11Device* device)
{
    // TODO: 実装（次のステップで）
    OutputDebugStringA("[FIRE] CreateBlendState: TODO\n");
}

void FireParticleSystem::Render(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection)
{
  
}