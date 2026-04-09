// StyleRankSystem.h
// �y�����z�X�^�C�������N�V�X�e��
// �y�@�\�z�L�����_���ɉ����ă����N���㉺�����A��V�{����Ԃ�
#pragma once

#include <string>
#include <DirectXMath.h>

// === �X�^�C�������N�̒i�K ===
// enum�́u���O�t���̐��l�v�B0���珇�ɔԍ����U����
// D=0, C=1, B=2 ... SSS=6
enum class StyleRank
{
    D = 0,      // �Œ჉���N
    C,          // ����
    B,          // �܂��܂�
    A,          // ��肢
    S,          // ������
    SS,         // ���o��
    SSS         // �_
};


// === �X�^�C�������N�V�X�e���{�� ===
class StyleRankSystem
{
public:
    // --- �R���X�g���N�^ ---
    StyleRankSystem();

    // --- ���t���[���Ăԁi���Ԍo�߂Ń|�C���g�����j ---
    // �y�����zdeltaTime: �O�t���[������̌o�ߕb���i��: 0.016 = 60FPS���j
    void Update(float deltaTime);

    // --- �L���������ɌĂ� ---
    // �y�����zisHeadshot: �w�b�h�V���b�g��������
    //         multiKillCount: �����L�����i1=�ʏ�, 2=�_�u��, 3=�g���v��...�j
    void NotifyKill(bool isHeadshot, int multiKillCount = 1);

    // --- �_���[�W���󂯂����ɌĂ� ---
    void NotifyDamage();

    // --- �p���B�������ɌĂ� --- 
    void NotifyParry();

    // --- ���݂̃����N���擾 ---
    StyleRank GetRank() const { return m_currentRank; }

    // --- �����N���𕶎���Ŏ擾�iHUD�\���p�j ---
    // ��: "SSS", "A", "D"
    const char* GetRankName() const;

    // --- �����N�ɉ������F���擾�iHUD�\���p�j ---
    // D=�D, C=��, B=��, A=��, S=��, SS=�I�����W, SSS=��
    DirectX::XMFLOAT4 GetRankColor() const;

    // --- ���݂̃X�^�C���|�C���g���擾 ---
    float GetStylePoints() const { return m_stylePoints; }

    // --- ���̃����N�܂ł̐i���i0.0?1.0�j�Q�[�W�\���p ---
    float GetProgress() const;

    // --- �����N�ɉ�����HP�񕜗ʂ��擾 ---
    int GetHealAmount() const;

    // --- �R���{�����擾 ---
    int GetComboCount() const { return m_comboCount; }

    // --- �R���{�^�C�}�[�c����擾�i0.0?1.0�j�Q�[�W�\���p ---
    float GetComboTimerRatio() const;

    // --- ���Z�b�g�i�Q�[���J�n���j ---
    void Reset();

private:
    float m_stylePoints;        // ���݂̃X�^�C���|�C���g
    StyleRank m_currentRank;    // ���݂̃����N

    int m_comboCount;           // ���݂̃R���{���i�A���L���j
    float m_comboTimer;         // �R���{���r�؂��܂ł̎c�莞�ԁi�b�j

    // --- �����֐��F�|�C���g���烉���N���Čv�Z ---
    void UpdateRank();

    // === �萔 ===
    static constexpr float COMBO_TIME_LIMIT = 3.0f;     // �R���{�������ԁi�b�j
    static constexpr float DECAY_PER_SECOND = 15.0f;    // ���b�̎��R����
    static constexpr float DAMAGE_PENALTY = 200.0f;     // ��_�����̃y�i���e�B

    // �e�����N�ɕK�v�ȃ|�C���g�i�z��j
    // D=0, C=100, B=250, A=500, S=800, SS=1200, SSS=1800
    static constexpr float RANK_THRESHOLDS[7] = {
        0.0f, 100.0f, 250.0f, 500.0f, 800.0f, 1200.0f, 1800.0f
    };
};