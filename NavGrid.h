// NavGrid.h
// �i�r�Q�[�V�����O���b�h ? �}�b�v�̕��s�\�̈��2D�O���b�h�ŊǗ�
#pragma once
#include <vector>
#include <cstdint>
#include <DirectXMath.h>
#include <queue>
#include <unordered_set>
#include <algorithm>

class NavGrid
{
public:
    // === �������F�O���b�h�𐶐����� ===
    // minX, maxX, minZ, maxZ: �}�b�v�͈̔́i���[���h���W�j
    // cellSize: 1�}�X�̑傫���i���[�g���j
    // testRadius: ���s����Ɏg�����̔��a
    void Initialize(
        float minX, float maxX,
        float minZ, float maxZ,
        float cellSize = 1.0f,
        float testRadius = 0.4f);

    // === 1�}�X�������邩�ǂ�����ݒ� ===
    // gx, gz: �O���b�h���W�i0�n�܂�j
    // blocked: true=�ǁAfalse=������
    void SetBlocked(int gx, int gz, bool blocked);

    // === 1�}�X�������邩���� ===
    bool IsBlocked(int gx, int gz) const;

    // === ���[���h���W �� �O���b�h���W ===
    int WorldToGridX(float worldX) const;
    int WorldToGridZ(float worldZ) const;

    // === �O���b�h���W �� ���[���h���W�i�}�X�̒��S�j ===
    float GridToWorldX(int gx) const;
    float GridToWorldZ(int gz) const;

    // === �O���b�h�̃T�C�Y�擾 ===
    int GetWidth()  const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetCellSize() const { return m_cellSize; }

    // === �f�o�b�O�p�F�ǂ̐��ƍ��v�}�X��Ԃ� ===
    int CountBlocked() const;
    int CountTotal()   const { return m_width * m_height; }

    // === �O���b�h���������ς݂� ===
    bool IsReady() const { return m_ready; }

    // === �o�H�T�� ===
   // startX/Z, goalX/Z: ���[���h���W
   // �߂�l: �ʉ߂��郏�[���h���W�̔z��i��=�o�H�Ȃ��j
    std::vector<DirectX::XMFLOAT3> FindPath(
        float startX, float startZ,
        float goalX, float goalZ) const;

private:
    std::vector<uint8_t> m_grid;   // 0=������, 1=��
    int   m_width = 0;            // X�����̃}�X��
    int   m_height = 0;            // Z�����̃}�X��
    float m_cellSize = 1.0f;       // 1�}�X�̃T�C�Y�im�j
    float m_minX = 0, m_minZ = 0;  // ���[���h���W�̌��_
    float m_testRadius = 0.4f;     // ���s����p�̋��̔��a
    bool  m_ready = false;
};