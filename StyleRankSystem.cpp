// StyleRankSystem.cpp
// �y�����z�X�^�C�������N�̑S���W�b�N������
// �y�@�\�z�L�����|�C���g���Z�������N�X�V�A���Ԍo�߁������A��_�����y�i���e�B

#include "StyleRankSystem.h"
#include <algorithm>    // std::min, std::max ���g���i�l�͈̔͐����p�j

// =============================================
// �R���X�g���N�^�i�������j
// =============================================
StyleRankSystem::StyleRankSystem()
    : m_stylePoints(0.0f)       // �|�C���g0����X�^�[�g
    , m_currentRank(StyleRank::D)   // �����ND����X�^�[�g
    , m_comboCount(0)           // �R���{�Ȃ�
    , m_comboTimer(0.0f)        // �^�C�}�[�Ȃ�
{
}

// =============================================
// ���t���[���X�V
// �y��邱�Ɓz
//   1. �R���{�^�C�}�[�����炷 �� 0�ɂȂ�����R���{���Z�b�g
//   2. �X�^�C���|�C���g�����R����������
//   3. �����N���Čv�Z
// =============================================
void StyleRankSystem::Update(float deltaTime)
{
    // --- �R���{�^�C�}�[���� ---
    if (m_comboCount > 0)
    {
        m_comboTimer -= deltaTime;  // ���t���[���o�ߎ��ԂԂ񌸂炷

        if (m_comboTimer <= 0.0f)
        {
            // �R���{�r�؂ꂽ�I
            m_comboCount = 0;
            m_comboTimer = 0.0f;
        }
    }

    // --- �X�^�C���|�C���g���R���� ---
    // ��ɍU�ߑ����Ȃ��ƃ����N��������d�g��
    m_stylePoints -= DECAY_PER_SECOND * deltaTime;

    // 0�����ɂ͂Ȃ�Ȃ�
    if (m_stylePoints < 0.0f)
    {
        m_stylePoints = 0.0f;
    }

    // --- �����N�Čv�Z ---
    UpdateRank();
}

// =============================================
// �L���ʒm
// �y�����z
//   isHeadshot: �w�b�h�V���b�g���������itrue/false�j
//   multiKillCount: �����L�����i1=�ʏ�L��, 2=�_�u���L��...�j
// =============================================
void StyleRankSystem::NotifyKill(bool isHeadshot, int multiKillCount)
{
    // --- ��{�|�C���g ---
    float points = 100.0f;  // �ʏ�L�� = 100pt

    // --- �w�b�h�V���b�g�{�[�i�X ---
    if (isHeadshot)
    {
        points = 150.0f;    // �w�b�h�V���b�g = 150pt
    }

    // --- �}���`�L���{�[�i�X ---
    // 2�̓��� = 200pt, 3�� = 350pt, 4�̈ȏ�͂���ɑ���
    if (multiKillCount >= 2)
    {
        // 2��: 200, 3��: 350, 4��: 550...
        // �v�Z��: 100 �~ �}���`�L���� �~ 1.0?1.5�{
        points = 100.0f * multiKillCount * (1.0f + (multiKillCount - 1) * 0.25f);
    }

    // --- �R���{�X�V ---
    m_comboCount++;                     // �R���{��+1
    m_comboTimer = COMBO_TIME_LIMIT;    // �^�C�}�[���Z�b�g�i3�b�j

    // --- �R���{�{�[�i�X ---
    // �R���{�������قǃ{�[�i�X����������
    // ��: 5�R���{�� �� +100pt//, 10�R���{�� �� +200pt//
    float comboBonus = m_comboCount * 20.0f;
    points += comboBonus;

    // --- �|�C���g���Z ---
    m_stylePoints += points;

    // --- �����N�Čv�Z ---
    UpdateRank();
}

// =============================================
// �_���[�W�ʒm
// ��e����ƃ|�C���g���傫������ �� �����N�ێ��ɂ͏�肭�����邱�Ƃ��d�v
// =============================================
void StyleRankSystem::NotifyDamage()
{
    m_stylePoints -= DAMAGE_PENALTY;    // -200pt

    if (m_stylePoints < 0.0f)
    {
        m_stylePoints = 0.0f;
    }

    // �����N�Čv�Z
    UpdateRank();
}

// =============================================
// �p���B�ʒm
// �p���B���� �� �X�^�C���|�C���g���Z�i�R���{���X�V�j
// =============================================
void StyleRankSystem::NotifyParry()
{
    // --- �p���B��{�|�C���g ---
    float points = 120.0f;  // �L��(100)��菭�������i�Z�p��]���j

    // --- �R���{�X�V ---
    m_comboCount++;
    m_comboTimer = COMBO_TIME_LIMIT;

    // --- �R���{�{�[�i�X ---
    float comboBonus = m_comboCount * 20.0f;
    points += comboBonus;

    // --- �|�C���g���Z ---
    m_stylePoints += points;

    // --- �����N�Čv�Z ---
    UpdateRank();
}

// =============================================
// �����N�Čv�Z�i�����֐��j
// �|�C���g���e臒l�𒴂��Ă��邩�ォ��`�F�b�N
// =============================================
void StyleRankSystem::UpdateRank()
{
    // SSS(6)���珇�Ƀ`�F�b�N���āA�ŏ��ɒ����������N���̗p
    // RANK_THRESHOLDS[6]=1800, [5]=1200, [4]=800 ...
    for (int i = 6; i >= 0; i--)
    {
        if (m_stylePoints >= RANK_THRESHOLDS[i])
        {
            m_currentRank = static_cast<StyleRank>(i);
            return;
        }
    }

    m_currentRank = StyleRank::D;
}

// =============================================
// �����N���𕶎���ŕԂ��iHUD�\���p�j
// =============================================
const char* StyleRankSystem::GetRankName() const
{
    switch (m_currentRank)
    {
    case StyleRank::D:   return "D";
    case StyleRank::C:   return "C";
    case StyleRank::B:   return "B";
    case StyleRank::A:   return "A";
    case StyleRank::S:   return "S";
    case StyleRank::SS:  return "SS";
    case StyleRank::SSS: return "SSS";
    default:             return "D";
    }
}

// =============================================
// �����N�ɉ������F��Ԃ��iHUD�\���p�j
// XMFLOAT4 = (R, G, B, A) �� 0.0?1.0 �͈̔�
// =============================================
DirectX::XMFLOAT4 StyleRankSystem::GetRankColor() const
{
    switch (m_currentRank)
    {
    case StyleRank::D:   return { 0.5f, 0.5f, 0.5f, 1.0f };   // �D�F
    case StyleRank::C:   return { 1.0f, 1.0f, 1.0f, 1.0f };   // ��
    case StyleRank::B:   return { 0.3f, 0.5f, 1.0f, 1.0f };   // ��
    case StyleRank::A:   return { 0.2f, 1.0f, 0.3f, 1.0f };   // ��
    case StyleRank::S:   return { 1.0f, 1.0f, 0.0f, 1.0f };   // ��
    case StyleRank::SS:  return { 1.0f, 0.5f, 0.0f, 1.0f };   // �I�����W
    case StyleRank::SSS: return { 1.0f, 0.0f, 0.0f, 1.0f };   // ��
    default:             return { 0.5f, 0.5f, 0.5f, 1.0f };
    }
}

// =============================================
// ���̃����N�܂ł̐i���i0.0?1.0�j
// �Q�[�W�\���Ɏg��
// =============================================
float StyleRankSystem::GetProgress() const
{
    int rankIndex = static_cast<int>(m_currentRank);

    // SSS�Ȃ��ɖ��^��
    if (rankIndex >= 6)
    {
        return 1.0f;
    }

    // ���݂̃����N��臒l�Ǝ��̃����N��臒l���擾
    float currentThreshold = RANK_THRESHOLDS[rankIndex];
    float nextThreshold = RANK_THRESHOLDS[rankIndex + 1];

    // 0���Z�h�~
    float range = nextThreshold - currentThreshold;
    if (range <= 0.0f) return 0.0f;

    // �i�� = (���݃|�C���g - ���݃����N臒l) / (�������N臒l - ���݃����N臒l)
    float progress = (m_stylePoints - currentThreshold) / range;

    // 0.0?1.0�͈̔͂Ɏ��߂�
    return std::min(1.0f, std::max(0.0f, progress));
}

// =============================================
// �����N�ɉ�����HP�񕜗�
// �������N�قǓ|�������ɂ�������񕜂���
// =============================================
int StyleRankSystem::GetHealAmount() const
{
    switch (m_currentRank)
    {
    case StyleRank::D:   return 0;
    case StyleRank::C:   return 2;
    case StyleRank::B:   return 5;
    case StyleRank::A:   return 8;
    case StyleRank::S:   return 12;
    case StyleRank::SS:  return 18;
    case StyleRank::SSS: return 25;
    default:             return 0;
    }
}

// =============================================
// �R���{�^�C�}�[�̎c��i0.0?1.0�j
// �R���{�Q�[�W�\���p
// =============================================
float StyleRankSystem::GetComboTimerRatio() const
{
    if (m_comboCount == 0) return 0.0f;

    // �^�C�}�[�c�� / �������� = 0.0?1.0
    return std::min(1.0f, std::max(0.0f, m_comboTimer / COMBO_TIME_LIMIT));
}

// =============================================
// ���Z�b�g�i�Q�[���J�n���ɌĂԁj
// =============================================
void StyleRankSystem::Reset()
{
    m_stylePoints = 0.0f;
    m_currentRank = StyleRank::D;
    m_comboCount = 0;
    m_comboTimer = 0.0f;
}
