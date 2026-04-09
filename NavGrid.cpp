// NavGrid.cpp
// �i�r�Q�[�V�����O���b�h�̎���
#include "NavGrid.h"
#include <cmath>
#include <cstdio>

// Windows API��OutputDebugStringA�p
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// ============================================
//  �������F�O���b�h�̃��������m��
// ============================================
void NavGrid::Initialize(
    float minX, float maxX,
    float minZ, float maxZ,
    float cellSize, float testRadius)
{
    m_minX = minX;
    m_minZ = minZ;
    m_cellSize = cellSize;
    m_testRadius = testRadius;

    // �}�b�v�̕��ƍ�������}�X�����v�Z
    // ��: minX=-15, maxX=15, cellSize=1.0 �� width=30
    m_width = (int)ceilf((maxX - minX) / cellSize);
    m_height = (int)ceilf((maxZ - minZ) / cellSize);

    // �O���b�h�z����m�ہi�����l�F0=���ׂĕ�����j
    m_grid.assign(m_width * m_height, 0);

    m_ready = true;

    char buf[256];
    sprintf_s(buf, "[NavGrid] Initialized: %d x %d cells (%.1f m each)\n",
        m_width, m_height, m_cellSize);
    OutputDebugStringA(buf);
}

// ============================================
//  �ǃt���O�̐ݒ�
// ============================================
void NavGrid::SetBlocked(int gx, int gz, bool blocked)
{
    // �͈͊O�`�F�b�N
    if (gx < 0 || gx >= m_width || gz < 0 || gz >= m_height) return;

    // 1D�z��̃C���f�b�N�X = gz * �� + gx
    m_grid[gz * m_width + gx] = blocked ? 1 : 0;
}

// ============================================
//  �����邩����
// ============================================
bool NavGrid::IsBlocked(int gx, int gz) const
{
    // �͈͊O�͕ǈ����i�}�b�v�O�ɏo�Ȃ��j
    if (gx < 0 || gx >= m_width || gz < 0 || gz >= m_height) return true;

    return m_grid[gz * m_width + gx] != 0;
}

// ============================================
//  ���W�ϊ��F���[���h �� �O���b�h
// ============================================
int NavGrid::WorldToGridX(float worldX) const
{
    // ��: worldX=5.3, minX=-15, cellSize=1.0 �� (5.3-(-15))/1.0 = 20.3 �� 20
    return (int)floorf((worldX - m_minX) / m_cellSize);
}

int NavGrid::WorldToGridZ(float worldZ) const
{
    return (int)floorf((worldZ - m_minZ) / m_cellSize);
}

// ============================================
//  ���W�ϊ��F�O���b�h �� ���[���h�i�}�X�̒��S�j
// ============================================
float NavGrid::GridToWorldX(int gx) const
{
    // ��: gx=20, minX=-15, cellSize=1.0 �� -15 + 20*1.0 + 0.5 = 5.5�i�}�X�̒��S�j
    return m_minX + gx * m_cellSize + m_cellSize * 0.5f;
}

float NavGrid::GridToWorldZ(int gz) const
{
    return m_minZ + gz * m_cellSize + m_cellSize * 0.5f;
}

// ============================================
//  �f�o�b�O�p�F�ǃ}�X�̐��𐔂���
// ============================================
int NavGrid::CountBlocked() const
{
    int count = 0;
    for (auto cell : m_grid)
    {
        if (cell != 0) count++;
    }
    return count;
}

std::vector<DirectX::XMFLOAT3> NavGrid::FindPath(
    float startX, float startZ,
    float goalX, float goalZ) const
{
    std::vector<DirectX::XMFLOAT3> result;  // �߂�l�i�o�H�j

    if (!m_ready) return result;

    // --- ���[���h���W �� �O���b�h���W�ɕϊ� ---
    int sx = WorldToGridX(startX);
    int sz = WorldToGridZ(startZ);
    int gx = WorldToGridX(goalX);
    int gz = WorldToGridZ(goalZ);

    // �X�^�[�g���S�[�����͈͊O or �ǂȂ炷���I��
    if (sx < 0 || sx >= m_width || sz < 0 || sz >= m_height) return result;
    if (gx < 0 || gx >= m_width || gz < 0 || gz >= m_height) return result;

    // �S�[�����ǂ̏ꍇ�A��ԋ߂�������}�X��T��
    if (IsBlocked(gx, gz))
    {
        // ����5�}�X�ȓ��ŕ�����}�X��T��
        bool found = false;
        for (int r = 1; r <= 5 && !found; r++)
        {
            for (int dx = -r; dx <= r && !found; dx++)
            {
                for (int dz = -r; dz <= r && !found; dz++)
                {
                    if (abs(dx) == r || abs(dz) == r)  // �O�������`�F�b�N
                    {
                        int nx = gx + dx;
                        int nz = gz + dz;
                        if (!IsBlocked(nx, nz))
                        {
                            gx = nx;
                            gz = nz;
                            found = true;
                        }
                    }
                }
            }
        }
        if (!found) return result;  // �߂��ɕ�����}�X���Ȃ�
    }

    // �X�^�[�g���ǂ̒��ɂ���ꍇ�����l
    if (IsBlocked(sx, sz))
    {
        bool found = false;
        for (int r = 1; r <= 3 && !found; r++)
        {
            for (int dx = -r; dx <= r && !found; dx++)
            {
                for (int dz = -r; dz <= r && !found; dz++)
                {
                    int nx = sx + dx;
                    int nz = sz + dz;
                    if (!IsBlocked(nx, nz))
                    {
                        sx = nx; sz = nz;
                        found = true;
                    }
                }
            }
        }
        if (!found) return result;
    }

    // �X�^�[�g�ƃS�[���������}�X
    if (sx == gx && sz == gz)
    {
        result.push_back({ goalX, 0.0f, goalZ });
        return result;
    }

    // --- A* �p�̃f�[�^�\�� ---

    // 1�}�X�̏����i�[����m�[�h
    struct Node
    {
        int x, z;       // �O���b�h���W
        float f, g;     // f=�����]��, g=�X�^�[�g����̎��R�X�g
        int parentIdx;  // �e�m�[�h�̃C���f�b�N�X�i�o�H�����p�j
    };

    // f�����������Ɏ��o����r�֐�
    struct CompareF
    {
        bool operator()(const std::pair<float, int>& a,
            const std::pair<float, int>& b) const
        {
            return a.first > b.first;  // ������f��D��
        }
    };

    // �q���[���X�e�B�b�N�֐��i�`�F�r�V�F�t�����j
    // 8�����ړ��Ȃ̂ŁA�΂�1��=1.41�A����1��=1.0
    auto heuristic = [](int x1, int z1, int x2, int z2) -> float
        {
            int dx = abs(x1 - x2);
            int dz = abs(z1 - z2);
            // �΂߈ړ��̃R�X�g���l����������
            return (float)(dx + dz) + (1.414f - 2.0f) * (float)(dx < dz ? dx : dz);
        };

    // �S�m�[�h���i�[����z��
    std::vector<Node> allNodes;
    allNodes.reserve(1024);

    // �e�}�X�̍ŗ�g�R�X�g���L�^�i-1=���K��j
    std::vector<float> bestG(m_width * m_height, -1.0f);

    // �I�[�v�����X�g�ipriority_queue: f�����������j
    std::priority_queue <
        std::pair<float, int>,             // (f�l, �m�[�h�C���f�b�N�X)
        std::vector<std::pair<float, int>>,
        CompareF
            > openList;

            // --- �X�^�[�g�m�[�h��// ---
            float h0 = heuristic(sx, sz, gx, gz);
            allNodes.push_back({ sx, sz, h0, 0.0f, -1 });
            bestG[sz * m_width + sx] = 0.0f;
            openList.push({ h0, 0 });

            // 8�����̗אڃ}�X�i�㉺���E�{�΂߁j
            const int dx8[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
            const int dz8[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
            const float cost8[] = {
                1.414f, 1.0f, 1.414f,   // ����, ��, �E��
                1.0f,         1.0f,     // ��,      �E
                1.414f, 1.0f, 1.414f    // ����, ��, �E��
            };

            int goalNodeIdx = -1;

            // �T���񐔂̏���i�������[�v�h�~�j
            int maxIterations = (m_width * m_height) / 4;
            int iterations = 0;

            // --- A* ���C�����[�v ---
            while (!openList.empty() && iterations < maxIterations)
            {
                iterations++;

                // f���ŏ��̃m�[�h�����o��
                float f = openList.top().first;
                int nodeIdx = openList.top().second;
                openList.pop();

                Node& current = allNodes[nodeIdx];

                // �S�[���ɓ��B�I
                if (current.x == gx && current.z == gz)
                {
                    goalNodeIdx = nodeIdx;
                    break;
                }

                // ���łɂ��ǂ��o�H�ŖK��ς݂Ȃ�X�L�b�v
                float recordedG = bestG[current.z * m_width + current.x];
                if (recordedG >= 0 && current.g > recordedG + 0.01f)
                    continue;

                // 8�����̗אڃ}�X�𒲂ׂ�
                for (int d = 0; d < 8; d++)
                {
                    int nx = current.x + dx8[d];
                    int nz = current.z + dz8[d];

                    // �͈͊O�E�ǂ̓X�L�b�v
                    if (IsBlocked(nx, nz)) continue;

                    // �΂߈ړ��̏ꍇ�A�אڂ��钼���}�X���ʂ�邩�m�F
                    // �i�ǂ̊p�����蔲���Ȃ��悤�Ɂj
                    if (dx8[d] != 0 && dz8[d] != 0)
                    {
                        if (IsBlocked(current.x + dx8[d], current.z) ||
                            IsBlocked(current.x, current.z + dz8[d]))
                            continue;  // �ǂ̊p�͂��蔲���֎~
                    }

                    float newG = current.g + cost8[d];
                    int cellIdx = nz * m_width + nx;

                    // ���̃}�X�ɏ��߂ė��� or ���Z���o�H��������
                    if (bestG[cellIdx] < 0 || newG < bestG[cellIdx] - 0.01f)
                    {
                        bestG[cellIdx] = newG;
                        float h = heuristic(nx, nz, gx, gz);
                        int newIdx = (int)allNodes.size();
                        allNodes.push_back({ nx, nz, newG + h, newG, nodeIdx });
                        openList.push({ newG + h, newIdx });
                    }
                }
            }

            // --- �o�H��������Ȃ����� ---
            if (goalNodeIdx < 0) return result;

            // --- �o�H�𕜌��i�S�[�����X�^�[�g�̏��ɂ��ǂ�j ---
            std::vector<DirectX::XMFLOAT3> reversePath;
            int idx = goalNodeIdx;
            while (idx >= 0)
            {
                Node& n = allNodes[idx];
                float wx = GridToWorldX(n.x);
                float wz = GridToWorldZ(n.z);
                reversePath.push_back({ wx, 0.0f, wz });
                idx = n.parentIdx;
            }

            // �t���ɂ��� �X�^�[�g���S�[�� �̏��ɂ���
            result.reserve(reversePath.size());
            for (int i = (int)reversePath.size() - 1; i >= 0; i--)
            {
                result.push_back(reversePath[i]);
            }

            return result;
}