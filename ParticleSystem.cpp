// ParticleSystem.cpp - パーティクル管理システムの実装
#include "ParticleSystem.h"
#include <cmath>      // cosf, sinf

// コンストラクタ
// 【いつ呼ばれる】Game.cppで std::make_unique<ParticleSystem>() した時
ParticleSystem::ParticleSystem()
{
    m_particles.reserve(500);  //  最大500パーティクル分を先に確保
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

        // 空気抵抗（速度が徐々に落ちる → 霧がふわっと止まる）
        float drag = 0.98f;
        it->velocity.x *= drag;
        it->velocity.y *= drag;
        it->velocity.z *= drag;

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

        particle.size = 0.05f + (float)rand() / RAND_MAX * 0.1f;  // 0.05?0.15

        particle.lifetime = 1.0f + (float)rand() / RAND_MAX * 2.0f;

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
    DirectX::XMFLOAT3 cameraRotation,
    WeaponType weaponType)
{

    // カメラの前方向
    float fwdX = sinf(cameraRotation.y) * cosf(cameraRotation.x);
    float fwdY = -sinf(cameraRotation.x);
    float fwdZ = cosf(cameraRotation.y) * cosf(cameraRotation.x);

    // カメラの右方向
    float rgtX = cosf(cameraRotation.y);
    float rgtY = 0.0f;
    float rgtZ = -sinf(cameraRotation.y);

    // カメラの上方向（外積）
    float upX = rgtY * fwdZ - rgtZ * fwdY;
    float upY = rgtZ * fwdX - rgtX * fwdZ;
    float upZ = rgtX * fwdY - rgtY * fwdX;

    // === 武器ごとのマズルフラッシュ設定 ===
    int sparkCount = 8;
    int gasCount = 4;
    float sparkSpeed = 10.0f;
    float gasSpeed = 8.0f;
    float sparkSize = 0.1f;
    DirectX::XMFLOAT4 sparkColor(1.0f, 1.0f, 0.9f, 1.0f);
    DirectX::XMFLOAT4 gasColor(1.0f, 0.4f, 0.1f, 1.0f);

    switch (weaponType)
    {
    case WeaponType::PISTOL:
        sparkCount = 6;
        gasCount = 2;
        sparkSpeed = 8.0f;
        gasSpeed = 6.0f;
        sparkSize = 0.08f;
        sparkColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.8f, 1.0f);
        gasColor = DirectX::XMFLOAT4(1.0f, 0.6f, 0.2f, 1.0f);
        break;

    case WeaponType::SHOTGUN:
        sparkCount = 20;
        gasCount = 10;
        sparkSpeed = 15.0f;
        gasSpeed = 12.0f;
        sparkSize = 0.15f;
        sparkColor = DirectX::XMFLOAT4(1.0f, 0.9f, 0.7f, 1.0f);
        gasColor = DirectX::XMFLOAT4(1.0f, 0.3f, 0.1f, 1.0f);
        break;

    case WeaponType::RIFLE:
        sparkCount = 10;
        gasCount = 5;
        sparkSpeed = 12.0f;
        gasSpeed = 9.0f;
        sparkSize = 0.1f;
        sparkColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.85f, 1.0f);
        gasColor = DirectX::XMFLOAT4(1.0f, 0.5f, 0.15f, 1.0f);
        break;

    case WeaponType::SNIPER:
        sparkCount = 15;
        gasCount = 8;
        sparkSpeed = 18.0f;
        gasSpeed = 14.0f;
        sparkSize = 0.2f;
        sparkColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        gasColor = DirectX::XMFLOAT4(1.0f, 0.7f, 0.3f, 1.0f);
        break;
    }

    // === 火花パーティクル（白い閃光）===
    for (int i = 0; i < sparkCount; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        // spread をカメラの右方向と上方向にかける
        float spreadH = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // 横方向
        float spreadV = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // 縦方向
        float speed = sparkSpeed + (float)rand() / RAND_MAX * 5.0f;

        particle.velocity.x = (fwdX + rgtX * spreadH + upX * spreadV) * speed;
        particle.velocity.y = (fwdY + rgtY * spreadH + upY * spreadV) * speed;
        particle.velocity.z = (fwdZ + rgtZ * spreadH + upZ * spreadV) * speed;

        particle.size = sparkSize;
        particle.color = sparkColor;

        particle.lifetime = 0.15f + (float)rand() / RAND_MAX * 0.1f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }

    // === 燃焼ガス（オレンジ色）===
    for (int i = 0; i < gasCount; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        // spread をカメラの右方向と上方向にかける
        float spreadH = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // 横方向
        float spreadV = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // 縦方向
        float speed = sparkSpeed + (float)rand() / RAND_MAX * 5.0f;

        particle.velocity.x = (fwdX + rgtX * spreadH + upX * spreadV) * speed;
        particle.velocity.y = (fwdY + rgtY * spreadH + upY * spreadV) * speed;
        particle.velocity.z = (fwdZ + rgtZ * spreadH + upZ * spreadV) * speed;

        particle.color = gasColor;

        particle.size = sparkSize * 1.5f;  // ガスは火花より少し大きめ

        particle.lifetime = 0.3f + (float)rand() / RAND_MAX * 0.2f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }
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

//  血しぶき
void ParticleSystem::CreateBloodEffect(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count)
{
    // === Layer 1: 大きい飛沫（全体の20%）===
    // 肉片が飛ぶような大きい塊
    int splashCount = count / 5;
    for (int i = 0; i < splashCount; i++)
    {
        Particle p;
        p.position = pos;

        // 暗い赤（ドス黒い血）
        float r = 0.4f + ((rand() % 30) / 100.0f);  // 0.4?0.7
        p.color = DirectX::XMFLOAT4(r, 0.0f, 0.0f, 1.0f);

        // 速い・方向に沿って飛ぶ
        float speed = 5.0f + ((rand() % 300) / 100.0f);  // 5?8
        float rx = ((rand() % 100) - 50) / 100.0f;  // 小さめの拡散
        float ry = ((rand() % 100) - 50) / 100.0f;
        float rz = ((rand() % 100) - 50) / 100.0f;
        p.velocity.x = dir.x * speed + rx;
        p.velocity.y = dir.y * speed + ry + 1.5f;
        p.velocity.z = dir.z * speed + rz;

        p.size = 0.08f + ((rand() % 100) / 1000.0f);  // 0.08?0.18（大きめ）
        p.lifetime = 0.3f + ((rand() % 30) / 100.0f);  // 0.3?0.6秒（短い）
        p.maxLifetime = p.lifetime;

        m_particles.push_back(p);
    }

    // === Layer 2: 中間のしぶき（全体の50%）===
    // メインの血しぶき
    int midCount = count / 2;
    for (int i = 0; i < midCount; i++)
    {
        Particle p;
        p.position = pos;

        // 鮮やかな赤（メインの血の色）
        float r = 0.6f + ((rand() % 40) / 100.0f);  // 0.6?1.0
        float g = ((rand() % 5) / 100.0f);           // 0?0.05（ほぼゼロ）
        p.color = DirectX::XMFLOAT4(r, g, 0.0f, 1.0f);

        float speed = 2.0f + ((rand() % 200) / 100.0f);  // 2?4
        float rx = ((rand() % 200) - 100) / 100.0f;
        float ry = ((rand() % 200) - 100) / 100.0f;
        float rz = ((rand() % 200) - 100) / 100.0f;
        p.velocity.x = dir.x * speed + rx;
        p.velocity.y = dir.y * speed + ry + 2.0f;
        p.velocity.z = dir.z * speed + rz;

        p.size = 0.04f + ((rand() % 80) / 1000.0f);  // 0.04?0.12
        p.lifetime = 0.5f + ((rand() % 50) / 100.0f);  // 0.5?1.0秒
        p.maxLifetime = p.lifetime;

        m_particles.push_back(p);
    }

    // === Layer 3: 霧状のミスト（全体の30%）===
    // ふわっと漂う血の霧
    int mistCount = count - splashCount - midCount;
    for (int i = 0; i < mistCount; i++)
    {
        Particle p;

        // 少しずれた位置から発生（霧っぽく広がる）
        p.position.x = pos.x + ((rand() % 40) - 20) / 100.0f;
        p.position.y = pos.y + ((rand() % 40) - 20) / 100.0f;
        p.position.z = pos.z + ((rand() % 40) - 20) / 100.0f;

        // 薄い赤（半透明の霧）
        float r = 0.3f + ((rand() % 20) / 100.0f);
        p.color = DirectX::XMFLOAT4(r, 0.0f, 0.0f, 0.4f);  //  最初から半透明

        // 遅い・ふわっと漂う
        float speed = 0.5f + ((rand() % 100) / 100.0f);  // 0.5?1.5
        float rx = ((rand() % 200) - 100) / 100.0f;
        float ry = ((rand() % 100) / 100.0f);  // 上向きだけ
        float rz = ((rand() % 200) - 100) / 100.0f;
        p.velocity.x = dir.x * speed * 0.3f + rx * 0.5f;
        p.velocity.y = dir.y * speed * 0.3f + ry + 0.5f;
        p.velocity.z = dir.z * speed * 0.3f + rz * 0.5f;

        p.size = 0.15f + ((rand() % 150) / 1000.0f);  // 0.15?0.30（大きい霧）
        p.lifetime = 1.0f + ((rand() % 80) / 100.0f);  // 1.0?1.8秒（長め）
        p.maxLifetime = p.lifetime;

        m_particles.push_back(p);
    }
}