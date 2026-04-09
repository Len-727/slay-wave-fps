// ParticleSystem.h - �p�[�e�B�N���Ǘ��V�X�e��
// �y�ړI�z�����A�e���t���b�V���Ȃǂ̃G�t�F�N�g���Ǘ�
#pragma once

#include "Entities.h"      // Particle�\����
#include <vector>          // std::vector
#include <DirectXMath.h>   // XMFLOAT3

enum class WeaponType;

// �p�[�e�B�N���Ǘ��N���X
class ParticleSystem {
public:
    // �R���X�g���N�^
    // �y�����z�p�[�e�B�N���z���������
    ParticleSystem();

    // === ���C������ ===

    // Update - ���t���[���Ă΂��X�V����
    // �y�����z�S�Ẵp�[�e�B�N���𓮂����A�����؂���폜
    // �y�����zdeltaTime: �o�ߎ��ԁi�b�j
    void Update(float deltaTime);

    // === �G�t�F�N�g���� ===

    // CreateExplosion - �����G�t�F�N�g�𐶐�
    // �y�����z�w��ʒu��20�̗ΐF�p�[�e�B�N���������_���ɔ�юU�点��
    // �y�����zposition: �����ʒu
    // �y�p�r�z�G��|������
    void CreateExplosion(DirectX::XMFLOAT3 position);

    // CreateMuzzleFlash - �e���t���b�V���𐶐�
    // �y�����z�e���ɉΉԂƔR�ăK�X�̃p�[�e�B�N���𐶐�
    // �y�����zmuzzlePosition: �e���̈ʒu
    //        cameraRotation: �J�����̉�]�i�ˌ������j
    // �y�p�r�z�ˌ���
    void CreateMuzzleFlash(
        DirectX::XMFLOAT3 muzzlePosition,
        DirectX::XMFLOAT3 cameraRotation,
        WeaponType weaponType);

    // === �Q�b�^�[ ===

    // GetParticles - �p�[�e�B�N���z����擾�i�ǂݎ���p�j
    // �y�p�r�z�`�掞�ɑS�p�[�e�B�N��������
    // �y�߂�l�zconst�Q�Ɓi�ύX�s�E�R�s�[���Ȃ��j
    const std::vector<Particle>& GetParticles() const { return m_particles; }

    // IsEmpty - �p�[�e�B�N�����󂩊m�F
    // �y�p�r�z�`��O�̍œK���i�p�[�e�B�N�����Ȃ���Ε`��X�L�b�v�j
    bool IsEmpty() const { return m_particles.empty(); }

    //  �ʒu(pos)�������(dir)�Ɍ������Č��𕬏o������
    void CreateBloodEffect(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count = 10);

private:
    // === �v���C�x�[�g�֐� ===

    // CreateMuzzleParticles - �e���p�[�e�B�N���̏ڍא���
    // �y�����z�ΉԂƔR�ăK�X��ʁX�ɐ���
    // �y���R�zCreateMuzzleFlash���番�����Č��₷��
    void CreateMuzzleParticles(DirectX::XMFLOAT3 muzzlePosition,
        DirectX::XMFLOAT3 cameraRotation);

    // === �����o�ϐ� ===

    // m_particles - ���ݑ��݂���S�Ẵp�[�e�B�N��
    // �y�^�zstd::vector<Particle>
    //   - �ϒ��z��i���I�ɑ����j
    //   - Particle: �ʒu�A���x�A�F�A���������\����
    std::vector<Particle> m_particles;
};