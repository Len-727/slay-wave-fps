// ParticleSystem.cpp - パーティクル管理システムの実装
#include "ParticleSystem.h"
#include <cmath>      // cosf, sinf

// コンストラクタ
// 【いつ呼ばれる】Game.cppで std::make_unique<ParticleSystem>() した時
ParticleSystem::ParticleSystem()
{
    // m_particles は空の配列として自動初期化される
    // 何もする必要なし
}

// Update - パーティクルの更新と削除
// 【役割】全パーティクルを動かし、寿命切れを削除
void ParticleSystem::Update(float deltaTime)
{
    // イテレータで配列を走査（削除しながらループするため）
    // 【理由】for (auto& p : m_particles) では削除できない
    for (auto it = m_particles.begin(); it != m_particles.end();)
    {
        // === 位置更新 ===
        // 【数式】新しい位置 = 現在位置 + 速度 × 時間
        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->position.z += it->velocity.z * deltaTime;

        // === 重力適用 ===
        // 【数式】速度Y -= 重力加速度 × 時間
        // 【値】9.8 m/s? = 地球の重力加速度
        // 【効果】パーティクルが下に落ちる
        it->velocity.y -= 9.8f * deltaTime;

        // === 寿命更新 ===
        it->lifetime -= deltaTime;

        // === フェードアウト ===
        // 【計算】残り寿命 ÷ 最大寿命 = 透明度 (0.0 ? 1.0)
        // 【例】寿命が半分 → alpha = 0.5（半透明）
        float alpha = it->lifetime / it->maxLifetime;
        it->color.w = alpha;  // w = alpha（透明度）

        // === 寿命切れのパーティクルを削除 ===
        if (it->lifetime <= 0.0f)
        {
            // erase() は削除後、次の要素を指すイテレータを返す
            it = m_particles.erase(it);
        }
        else
        {
            // 削除しない場合は次の要素へ
            ++it;
        }
    }
}

// CreateExplosion - 爆発エフェクト生成
// 【用途】敵を倒した時に呼ぶ
void ParticleSystem::CreateExplosion(DirectX::XMFLOAT3 position)
{
    // 20個のパーティクルを生成
    for (int i = 0; i < 20; i++)
    {
        Particle particle;
        particle.position = position;  // 爆発位置

        // === ランダムな方向に飛び散る ===

        // 角度をランダム生成（0 ? 360度）
        float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;

        // 速度をランダム生成（5 ? 15 m/s）
        float speed = 5.0f + (float)rand() / RAND_MAX * 10.0f;

        // 極座標→直交座標に変換
        particle.velocity.x = cosf(angle) * speed;  // X方向
        particle.velocity.y = 5.0f + (float)rand() / RAND_MAX * 10.0f;  // Y方向（上向き）
        particle.velocity.z = sinf(angle) * speed;  // Z方向

        // === 色設定（緑色） ===
        particle.color = DirectX::XMFLOAT4(
            0.0f,  // R: 赤なし
            1.0f,  // G: 緑最大
            0.0f,  // B: 青なし
            1.0f   // A: 不透明
        );

        // === 寿命設定（1?3秒） ===
        particle.lifetime = 1.0f + (float)rand() / RAND_MAX * 2.0f;
        particle.maxLifetime = particle.lifetime;

        // 配列に追加
        m_particles.push_back(particle);
    }
}

// CreateMuzzleFlash - 銃口フラッシュ生成
// 【用途】射撃時に呼ぶ
void ParticleSystem::CreateMuzzleFlash(DirectX::XMFLOAT3 muzzlePosition,
    DirectX::XMFLOAT3 cameraRotation)
{
    // 詳細生成処理を呼ぶ
    CreateMuzzleParticles(muzzlePosition, cameraRotation);
}

// CreateMuzzleParticles - 銃口パーティクルの詳細生成
void ParticleSystem::CreateMuzzleParticles(DirectX::XMFLOAT3 muzzlePosition,
    DirectX::XMFLOAT3 cameraRotation)
{
    // === 白熱した火花（高温の金属片）6個 ===
    for (int i = 0; i < 6; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        // 拡散角度（射撃方向から少しずれる）
        float spreadAngle = ((float)rand() / RAND_MAX - 0.5f) * 0.8f;

        // 速度（20 ? 35 m/s の高速）
        float speed = 20.0f + (float)rand() / RAND_MAX * 15.0f;

        // 射撃方向に飛ぶ
        particle.velocity.x = sinf(cameraRotation.y + spreadAngle) * speed;
        particle.velocity.y = -sinf(cameraRotation.x) * speed +
            ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particle.velocity.z = cosf(cameraRotation.y + spreadAngle) * speed;

        // 白?薄黄色（高温の金属）
        particle.color = DirectX::XMFLOAT4(
            1.0f,                              // R: 白
            1.0f,                              // G: 白
            0.9f + (float)rand() / RAND_MAX * 0.1f,  // B: 少し黄色
            1.0f
        );

        // 短い寿命（0.15 ? 0.25秒）
        particle.lifetime = 0.15f + (float)rand() / RAND_MAX * 0.1f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }

    // === 燃焼ガス（オレンジ色）4個 ===
    for (int i = 0; i < 4; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        float spreadAngle = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
        float speed = 8.0f + (float)rand() / RAND_MAX * 8.0f;

        particle.velocity.x = sinf(cameraRotation.y + spreadAngle) * speed;
        particle.velocity.y = -sinf(cameraRotation.x) * speed +
            ((float)rand() / RAND_MAX) * 5.0f;
        particle.velocity.z = cosf(cameraRotation.y + spreadAngle) * speed;

        // オレンジ?赤色（燃焼ガス）
        particle.color = DirectX::XMFLOAT4(
            1.0f,                                      // R: 赤最大
            0.4f + (float)rand() / RAND_MAX * 0.4f,  // G: オレンジ
            0.1f,                                      // B: 少し
            1.0f
        );

        // やや長い寿命（0.3 ? 0.5秒）
        particle.lifetime = 0.3f + (float)rand() / RAND_MAX * 0.2f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }
}