// ParticleSystem.cpp - �p�[�e�B�N���Ǘ��V�X�e���̎���
#include "ParticleSystem.h"
#include <cmath>      // cosf, sinf

// �R���X�g���N�^
// �y���Ă΂��zGame.cpp�� std::make_unique<ParticleSystem>() ������
ParticleSystem::ParticleSystem()
{
    m_particles.reserve(500);  //  �ő�500�p�[�e�B�N�������Ɋm��
    // m_particles �͋�̔z��Ƃ��Ď��������������
    // ��������K�v�Ȃ�
}

// Update - �p�[�e�B�N���̍X�V�ƍ폜
// �y�����z�S�p�[�e�B�N���𓮂����A�����؂���폜
void ParticleSystem::Update(float deltaTime)
{
    // �C�e���[�^�Ŕz��𑖍��i�폜���Ȃ��烋�[�v���邽�߁j
    // �y���R�zfor (auto& p : m_particles) �ł͍폜�ł��Ȃ�
    for (auto it = m_particles.begin(); it != m_particles.end();)
    {
        // === �ʒu�X�V ===
        // �y�����z�V�����ʒu = ���݈ʒu + ���x �~ ����
        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->position.z += it->velocity.z * deltaTime;

        // === �d�͓K�p ===
        // �y�����z���xY -= �d�͉����x �~ ����
        // �y�l�z9.8 m/s? = �n���̏d�͉����x
        // �y���ʁz�p�[�e�B�N�������ɗ�����
        it->velocity.y -= 9.8f * deltaTime;

        // ��C��R�i���x�����X�ɗ����� �� �����ӂ���Ǝ~�܂�j
        float drag = 0.98f;
        it->velocity.x *= drag;
        it->velocity.y *= drag;
        it->velocity.z *= drag;

        // === �����X�V ===
        it->lifetime -= deltaTime;

        // === �t�F�[�h�A�E�g ===
        // �y�v�Z�z�c����� �� �ő���� = �����x (0.0 ? 1.0)
        // �y��z���������� �� alpha = 0.5�i�������j
        float alpha = it->lifetime / it->maxLifetime;
        it->color.w = alpha;  // w = alpha�i�����x�j

        // === �����؂�̃p�[�e�B�N�����폜 ===
        if (it->lifetime <= 0.0f)
        {
            // erase() �͍폜��A���̗v�f���w���C�e���[�^��Ԃ�
            it = m_particles.erase(it);
        }
        else
        {
            // �폜���Ȃ��ꍇ�͎��̗v�f��
            ++it;
        }
    }
}

// CreateExplosion - �����G�t�F�N�g����
// �y�p�r�z�G��|�������ɌĂ�
void ParticleSystem::CreateExplosion(DirectX::XMFLOAT3 position)
{
    // 20�̃p�[�e�B�N���𐶐�
    for (int i = 0; i < 20; i++)
    {
        Particle particle;
        particle.position = position;  // �����ʒu

        // === �����_���ȕ����ɔ�юU�� ===

        // �p�x�������_�������i0 ? 360�x�j
        float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;

        // ���x�������_�������i5 ? 15 m/s�j
        float speed = 5.0f + (float)rand() / RAND_MAX * 10.0f;

        // �ɍ��W���������W�ɕϊ�
        particle.velocity.x = cosf(angle) * speed;  // X����
        particle.velocity.y = 5.0f + (float)rand() / RAND_MAX * 10.0f;  // Y�����i������j
        particle.velocity.z = sinf(angle) * speed;  // Z����

        // === �F�ݒ�i�ΐF�j ===
        particle.color = DirectX::XMFLOAT4(
            0.0f,  // R: �ԂȂ�
            1.0f,  // G: �΍ő�
            0.0f,  // B: �Ȃ�
            1.0f   // A: �s����
        );

        particle.size = 0.05f + (float)rand() / RAND_MAX * 0.1f;  // 0.05?0.15

        particle.lifetime = 1.0f + (float)rand() / RAND_MAX * 2.0f;

        // === �����ݒ�i1?3�b�j ===
        particle.lifetime = 1.0f + (float)rand() / RAND_MAX * 2.0f;
        particle.maxLifetime = particle.lifetime;

        // �z���//
        m_particles.push_back(particle);
    }
}

// CreateMuzzleFlash - �e���t���b�V������
// �y�p�r�z�ˌ����ɌĂ�
void ParticleSystem::CreateMuzzleFlash(DirectX::XMFLOAT3 muzzlePosition,
    DirectX::XMFLOAT3 cameraRotation,
    WeaponType weaponType)
{

    // �J�����̑O����
    float fwdX = sinf(cameraRotation.y) * cosf(cameraRotation.x);
    float fwdY = -sinf(cameraRotation.x);
    float fwdZ = cosf(cameraRotation.y) * cosf(cameraRotation.x);

    // �J�����̉E����
    float rgtX = cosf(cameraRotation.y);
    float rgtY = 0.0f;
    float rgtZ = -sinf(cameraRotation.y);

    // �J�����̏�����i�O�ρj
    float upX = rgtY * fwdZ - rgtZ * fwdY;
    float upY = rgtZ * fwdX - rgtX * fwdZ;
    float upZ = rgtX * fwdY - rgtY * fwdX;

    // === ���킲�Ƃ̃}�Y���t���b�V���ݒ� ===
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

    // === �Ήԃp�[�e�B�N���i�����M���j===
    for (int i = 0; i < sparkCount; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        // spread ���J�����̉E�����Ə�����ɂ�����
        float spreadH = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // ������
        float spreadV = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // �c����
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

    // === �R�ăK�X�i�I�����W�F�j===
    for (int i = 0; i < gasCount; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        // spread ���J�����̉E�����Ə�����ɂ�����
        float spreadH = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // ������
        float spreadV = ((float)rand() / RAND_MAX - 0.5f) * 0.3f; // �c����
        float speed = sparkSpeed + (float)rand() / RAND_MAX * 5.0f;

        particle.velocity.x = (fwdX + rgtX * spreadH + upX * spreadV) * speed;
        particle.velocity.y = (fwdY + rgtY * spreadH + upY * spreadV) * speed;
        particle.velocity.z = (fwdZ + rgtZ * spreadH + upZ * spreadV) * speed;

        particle.color = gasColor;

        particle.size = sparkSize * 1.5f;  // �K�X�͉ΉԂ�菭���傫��

        particle.lifetime = 0.3f + (float)rand() / RAND_MAX * 0.2f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }
}
// CreateMuzzleParticles - �e���p�[�e�B�N���̏ڍא���
void ParticleSystem::CreateMuzzleParticles(DirectX::XMFLOAT3 muzzlePosition,
    DirectX::XMFLOAT3 cameraRotation)
{
    // === ���M�����Ήԁi�����̋����Ёj6�� ===
    for (int i = 0; i < 6; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        // �g�U�p�x�i�ˌ��������班�������j
        float spreadAngle = ((float)rand() / RAND_MAX - 0.5f) * 0.8f;

        // ���x�i20 ? 35 m/s �̍����j
        float speed = 20.0f + (float)rand() / RAND_MAX * 15.0f;

        // �ˌ������ɔ��
        particle.velocity.x = sinf(cameraRotation.y + spreadAngle) * speed;
        particle.velocity.y = -sinf(cameraRotation.x) * speed +
            ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particle.velocity.z = cosf(cameraRotation.y + spreadAngle) * speed;

        // ��?�����F�i�����̋����j
        particle.color = DirectX::XMFLOAT4(
            1.0f,                              // R: ��
            1.0f,                              // G: ��
            0.9f + (float)rand() / RAND_MAX * 0.1f,  // B: �������F
            1.0f
        );

        // �Z�������i0.15 ? 0.25�b�j
        particle.lifetime = 0.15f + (float)rand() / RAND_MAX * 0.1f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }

    // === �R�ăK�X�i�I�����W�F�j4�� ===
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

        // �I�����W?�ԐF�i�R�ăK�X�j
        particle.color = DirectX::XMFLOAT4(
            1.0f,                                      // R: �ԍő�
            0.4f + (float)rand() / RAND_MAX * 0.4f,  // G: �I�����W
            0.1f,                                      // B: ����
            1.0f
        );

        // ��Ⓑ�������i0.3 ? 0.5�b�j
        particle.lifetime = 0.3f + (float)rand() / RAND_MAX * 0.2f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }
}

//  �����Ԃ�
void ParticleSystem::CreateBloodEffect(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count)
{
    // === Layer 1: �傫���򖗁i�S�̂�20%�j===
    // ���Ђ���Ԃ悤�ȑ傫����
    int splashCount = count / 5;
    for (int i = 0; i < splashCount; i++)
    {
        Particle p;
        p.position = pos;

        // �Â��ԁi�h�X�������j
        float r = 0.4f + ((rand() % 30) / 100.0f);  // 0.4?0.7
        p.color = DirectX::XMFLOAT4(r, 0.0f, 0.0f, 1.0f);

        // �����E�����ɉ����Ĕ��
        float speed = 5.0f + ((rand() % 300) / 100.0f);  // 5?8
        float rx = ((rand() % 100) - 50) / 100.0f;  // �����߂̊g�U
        float ry = ((rand() % 100) - 50) / 100.0f;
        float rz = ((rand() % 100) - 50) / 100.0f;
        p.velocity.x = dir.x * speed + rx;
        p.velocity.y = dir.y * speed + ry + 1.5f;
        p.velocity.z = dir.z * speed + rz;

        p.size = 0.08f + ((rand() % 100) / 1000.0f);  // 0.08?0.18�i�傫�߁j
        p.lifetime = 0.3f + ((rand() % 30) / 100.0f);  // 0.3?0.6�b�i�Z���j
        p.maxLifetime = p.lifetime;

        m_particles.push_back(p);
    }

    // === Layer 2: ���Ԃ̂��Ԃ��i�S�̂�50%�j===
    // ���C���̌����Ԃ�
    int midCount = count / 2;
    for (int i = 0; i < midCount; i++)
    {
        Particle p;
        p.position = pos;

        // �N�₩�Ȑԁi���C���̌��̐F�j
        float r = 0.6f + ((rand() % 40) / 100.0f);  // 0.6?1.0
        float g = ((rand() % 5) / 100.0f);           // 0?0.05�i�قڃ[���j
        p.color = DirectX::XMFLOAT4(r, g, 0.0f, 1.0f);

        float speed = 2.0f + ((rand() % 200) / 100.0f);  // 2?4
        float rx = ((rand() % 200) - 100) / 100.0f;
        float ry = ((rand() % 200) - 100) / 100.0f;
        float rz = ((rand() % 200) - 100) / 100.0f;
        p.velocity.x = dir.x * speed + rx;
        p.velocity.y = dir.y * speed + ry + 2.0f;
        p.velocity.z = dir.z * speed + rz;

        p.size = 0.04f + ((rand() % 80) / 1000.0f);  // 0.04?0.12
        p.lifetime = 0.5f + ((rand() % 50) / 100.0f);  // 0.5?1.0�b
        p.maxLifetime = p.lifetime;

        m_particles.push_back(p);
    }

    // === Layer 3: ����̃~�X�g�i�S�̂�30%�j===
    // �ӂ���ƕY�����̖�
    int mistCount = count - splashCount - midCount;
    for (int i = 0; i < mistCount; i++)
    {
        Particle p;

        // �������ꂽ�ʒu���甭���i�����ۂ��L����j
        p.position.x = pos.x + ((rand() % 40) - 20) / 100.0f;
        p.position.y = pos.y + ((rand() % 40) - 20) / 100.0f;
        p.position.z = pos.z + ((rand() % 40) - 20) / 100.0f;

        // �����ԁi�������̖��j
        float r = 0.3f + ((rand() % 20) / 100.0f);
        p.color = DirectX::XMFLOAT4(r, 0.0f, 0.0f, 0.4f);  //  �ŏ����甼����

        // �x���E�ӂ���ƕY��
        float speed = 0.5f + ((rand() % 100) / 100.0f);  // 0.5?1.5
        float rx = ((rand() % 200) - 100) / 100.0f;
        float ry = ((rand() % 100) / 100.0f);  // ���������
        float rz = ((rand() % 200) - 100) / 100.0f;
        p.velocity.x = dir.x * speed * 0.3f + rx * 0.5f;
        p.velocity.y = dir.y * speed * 0.3f + ry + 0.5f;
        p.velocity.z = dir.z * speed * 0.3f + rz * 0.5f;

        p.size = 0.15f + ((rand() % 150) / 1000.0f);  // 0.15?0.30�i�傫�����j
        p.lifetime = 1.0f + ((rand() % 80) / 100.0f);  // 1.0?1.8�b�i���߁j
        p.maxLifetime = p.lifetime;

        m_particles.push_back(p);
    }
}