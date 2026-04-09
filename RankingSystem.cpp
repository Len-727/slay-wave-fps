// RankingSystem.cpp
// �y�����z�����L���O�̕ۑ��E�ǂݍ��݁E�\�[�g�E//������
// �y�ۑ��`���z�o�C�i���t�@�C���i�������R���p�N�g�j
//   �w�b�_�[: MAGIC(4byte) + VERSION(4byte) + COUNT(4byte)
//   �f�[�^: RankingEntry �~ COUNT��

#define NOMINMAX            // Windows.h��min/max�}�N���𖳌���
#include <windows.h>        // OutputDebugStringA�i�f�o�b�O�o�́j
#include "RankingSystem.h"
#include <fstream>          // �t�@�C�����o�́iifstream, ofstream�j
#include <algorithm>        // std::sort�i�\�[�g�p�j
#include <ctime>            // time()�i���ݎ����擾�j

// =============================================
// �R���X�g���N�^
// �y�����z�t�@�C�������݂���Γǂݍ���
// =============================================
RankingSystem::RankingSystem()
{
    // �N�����Ƀt�@�C������ǂݍ���
    if (!Load())
    {
        // �ǂݍ��ݎ��s�i����N�� or �t�@�C���j���j
        OutputDebugStringA("[RANKING] No save file found, starting fresh\n");
        m_entries.clear();
    }
    else
    {
        char buf[128];
        sprintf_s(buf, "[RANKING] Loaded %d entries from file\n", (int)m_entries.size());
        OutputDebugStringA(buf);
    }
}

// =============================================
// AddEntry - �X�R�A��//���ăt�@�C���ɕۑ�
// =============================================
int RankingSystem::AddEntry(const RankingEntry& entry)
{
    // --- �^�C���X�^���v���L�^ ---
    RankingEntry newEntry = entry;
    newEntry.timestamp = time(nullptr);     // ���݂�UNIX����

    // --- �z���// ---
    m_entries.push_back(newEntry);

    // --- �X�R�A�~���Ń\�[�g ---
    SortEntries();

    // --- 10���𒴂�����؂�l�� ---
    if ((int)m_entries.size() > MAX_ENTRIES)
    {
        m_entries.resize(MAX_ENTRIES);
    }

    // --- //�����G���g���[�̏��ʂ�T�� ---
    int rank = -1;  // �����N�O
    for (int i = 0; i < (int)m_entries.size(); i++)
    {
        // �^�C���X�^���v�ƃX�R�A����v������̂�T��
        if (m_entries[i].score == newEntry.score &&
            m_entries[i].timestamp == newEntry.timestamp)
        {
            rank = i;   // 0=1��, 1=2��...
            break;
        }
    }

    // --- �t�@�C���ɕۑ� ---
    if (Save())
    {
        OutputDebugStringA("[RANKING] Saved successfully\n");
    }
    else
    {
        OutputDebugStringA("[RANKING] ERROR: Failed to save!\n");
    }

    return rank;
}

// =============================================
// IsNewRecord - Top10�ɓ��邩����
// =============================================
bool RankingSystem::IsNewRecord(int score) const
{
    // �܂�10�������Ȃ疳�����Ń����N�C��
    if ((int)m_entries.size() < MAX_ENTRIES)
    {
        return true;
    }

    // �ŉ��ʁi10�ʁj�̃X�R�A��荂����΃����N�C��
    return score > m_entries.back().score;
}

// =============================================
// Load - �o�C�i���t�@�C������ǂݍ���
// �y�t�@�C���\���z
//   [MAGIC: 4byte] [VERSION: 4byte] [COUNT: 4byte]
//   [RankingEntry �~ COUNT]
// =============================================
bool RankingSystem::Load()
{
    // --- �t�@�C�����J�� ---
    // ios::binary: �o�C�i�����[�h�i�e�L�X�g�ϊ����Ȃ��j
    std::ifstream file(SAVE_FILE, std::ios::binary);
    if (!file.is_open())
    {
        return false;   // �t�@�C�������݂��Ȃ��i����N���j
    }

    // --- �w�b�_�[�ǂݍ��� ---
    unsigned int magic = 0;
    unsigned int version = 0;
    int count = 0;

    // read(): �o�C�g������̂܂ܓǂݍ���
    // reinterpret_cast<char*>: �C�ӂ̌^�̃|�C���^��char*�ɕϊ�
    //   �iread()��char*��v�����邽�߁j
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    // --- �w�b�_�[���� ---
    if (magic != FILE_MAGIC)
    {
        OutputDebugStringA("[RANKING] ERROR: Invalid file magic\n");
        return false;   // �Ⴄ�t�@�C�� or �j��
    }

    if (version != FILE_VERSION)
    {
        OutputDebugStringA("[RANKING] ERROR: Version mismatch\n");
        return false;   // �o�[�W�����Ⴂ
    }

    // �ُ�Ȍ����`�F�b�N�i�j���h�~�j
    if (count < 0 || count > MAX_ENTRIES)
    {
        OutputDebugStringA("[RANKING] ERROR: Invalid entry count\n");
        return false;
    }

    // --- �f�[�^�ǂݍ��� ---
    m_entries.clear();
    m_entries.resize(count);

    // �S�G���g���[���ꊇ�ǂݍ���
    // sizeof(RankingEntry) �~ count �o�C�g��ǂ�
    file.read(reinterpret_cast<char*>(m_entries.data()),
        sizeof(RankingEntry) * count);

    // �ǂݍ��݂��r���Ŏ��s���Ă��Ȃ����m�F
    if (!file.good() && !file.eof())
    {
        OutputDebugStringA("[RANKING] ERROR: Read failed\n");
        m_entries.clear();
        return false;
    }

    // �O�̂��߃\�[�g
    SortEntries();

    return true;
}

// =============================================
// Save - �o�C�i���t�@�C���ɕۑ�
// =============================================
bool RankingSystem::Save() const
{
    // --- �t�@�C�����J���i�V�K�쐬 or �㏑���j ---
    std::ofstream file(SAVE_FILE, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        OutputDebugStringA("[RANKING] ERROR: Cannot open file for writing\n");
        return false;
    }

    // --- �w�b�_�[�������� ---
    unsigned int magic = FILE_MAGIC;
    unsigned int version = FILE_VERSION;
    int count = (int)m_entries.size();

    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // --- �f�[�^�������� ---
    if (count > 0)
    {
        file.write(reinterpret_cast<const char*>(m_entries.data()),
            sizeof(RankingEntry) * count);
    }

    return file.good();
}

// =============================================
// SortEntries - �X�R�A�~���Ń\�[�g
// =============================================
void RankingSystem::SortEntries()
{
    // std::sort + �����_���ŃX�R�A�~���\�[�g
    // a.score > b.score �Ȃ� a ����ɗ���i�~���j
    std::sort(m_entries.begin(), m_entries.end(),
        [](const RankingEntry& a, const RankingEntry& b)
        {
            // �X�R�A�������Ȃ瓞�B�E�F�[�u�Ŕ�r
            if (a.score == b.score)
                return a.wave > b.wave;
            return a.score > b.score;
        });
}

// =============================================
// Clear - �����L���O��S�����i�f�o�b�O�p�j
// =============================================
void RankingSystem::Clear()
{
    m_entries.clear();
    Save();
    OutputDebugStringA("[RANKING] Cleared all entries\n");
}