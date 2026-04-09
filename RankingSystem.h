// RankingSystem.h
// �y�����z�����L���O�f�[�^�̊Ǘ��i�ۑ��E�ǂݍ��݁E�\�[�g�j
// �y�i�����z�o�C�i���t�@�C�� "ranking.dat" �ɕۑ�
//          �Q�[������Ă��f�[�^���c��

#pragma once

#include <string>
#include <vector>
#include <ctime>        // time_t�i�����L�^�p�j

// =============================================
// �����L���O1�����̃f�[�^
// =============================================
struct RankingEntry
{
    int   score;            // ���v�X�R�A�im_goTotalScore�j
    int   wave;             // ���B�E�F�[�u
    int   kills;            // �L����
    int   headshots;        // �w�b�h�V���b�g��
    int   rank;             // �����N (0=C, 1=B, 2=A, 3=S)
    float survivalTime;     // �������ԁi�b�j
    time_t timestamp;       // �L�^���������iUNIX���ԁj

    // // �v���C���[���i�ő�15���� + null�I�[�j
    char  name[16];

    // �f�t�H���g�R���X�g���N�^: �S��0�ŏ�����
    RankingEntry()
        : score(0), wave(0), kills(0), headshots(0)
        , rank(0), survivalTime(0.0f), timestamp(0)
    {
        memset(name, 0, sizeof(name));
    }
};

// =============================================
// �����L���O�Ǘ��N���X
// =============================================
class RankingSystem
{
public:
    // --- �萔 ---
    static constexpr int MAX_ENTRIES = 10;  // Top10�܂ŕۑ�

    // --- �R���X�g���N�^ ---
    RankingSystem();

    // --- �X�R�A��// ---
    int AddEntry(const RankingEntry& entry);

    // --- �����L���O�ꗗ���擾 ---
    const std::vector<RankingEntry>& GetEntries() const { return m_entries; }

    // --- �V�L�^���ǂ������� ---
    bool IsNewRecord(int score) const;

    // --- �t�@�C������ ---
    bool Load();
    bool Save() const;
    void Clear();

    int GetEntryCount() const { return (int)m_entries.size(); }

private:
    std::vector<RankingEntry> m_entries;

    static constexpr const char* SAVE_FILE = "ranking.dat";
    static constexpr unsigned int FILE_MAGIC = 0x474F5448;  // "GOTH"
    static constexpr unsigned int FILE_VERSION = 1;

    void SortEntries();
};