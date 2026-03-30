// NavGrid.cpp
// ナビゲーショングリッドの実装
#include "NavGrid.h"
#include <cmath>
#include <cstdio>

// Windows APIのOutputDebugStringA用
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// ============================================
//  初期化：グリッドのメモリを確保
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

    // マップの幅と高さからマス数を計算
    // 例: minX=-15, maxX=15, cellSize=1.0 → width=30
    m_width = (int)ceilf((maxX - minX) / cellSize);
    m_height = (int)ceilf((maxZ - minZ) / cellSize);

    // グリッド配列を確保（初期値：0=すべて歩ける）
    m_grid.assign(m_width * m_height, 0);

    m_ready = true;

    char buf[256];
    sprintf_s(buf, "[NavGrid] Initialized: %d x %d cells (%.1f m each)\n",
        m_width, m_height, m_cellSize);
    OutputDebugStringA(buf);
}

// ============================================
//  壁フラグの設定
// ============================================
void NavGrid::SetBlocked(int gx, int gz, bool blocked)
{
    // 範囲外チェック
    if (gx < 0 || gx >= m_width || gz < 0 || gz >= m_height) return;

    // 1D配列のインデックス = gz * 幅 + gx
    m_grid[gz * m_width + gx] = blocked ? 1 : 0;
}

// ============================================
//  歩けるか判定
// ============================================
bool NavGrid::IsBlocked(int gx, int gz) const
{
    // 範囲外は壁扱い（マップ外に出ない）
    if (gx < 0 || gx >= m_width || gz < 0 || gz >= m_height) return true;

    return m_grid[gz * m_width + gx] != 0;
}

// ============================================
//  座標変換：ワールド → グリッド
// ============================================
int NavGrid::WorldToGridX(float worldX) const
{
    // 例: worldX=5.3, minX=-15, cellSize=1.0 → (5.3-(-15))/1.0 = 20.3 → 20
    return (int)floorf((worldX - m_minX) / m_cellSize);
}

int NavGrid::WorldToGridZ(float worldZ) const
{
    return (int)floorf((worldZ - m_minZ) / m_cellSize);
}

// ============================================
//  座標変換：グリッド → ワールド（マスの中心）
// ============================================
float NavGrid::GridToWorldX(int gx) const
{
    // 例: gx=20, minX=-15, cellSize=1.0 → -15 + 20*1.0 + 0.5 = 5.5（マスの中心）
    return m_minX + gx * m_cellSize + m_cellSize * 0.5f;
}

float NavGrid::GridToWorldZ(int gz) const
{
    return m_minZ + gz * m_cellSize + m_cellSize * 0.5f;
}

// ============================================
//  デバッグ用：壁マスの数を数える
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
    std::vector<DirectX::XMFLOAT3> result;  // 戻り値（経路）

    if (!m_ready) return result;

    // --- ワールド座標 → グリッド座標に変換 ---
    int sx = WorldToGridX(startX);
    int sz = WorldToGridZ(startZ);
    int gx = WorldToGridX(goalX);
    int gz = WorldToGridZ(goalZ);

    // スタートかゴールが範囲外 or 壁ならすぐ終了
    if (sx < 0 || sx >= m_width || sz < 0 || sz >= m_height) return result;
    if (gx < 0 || gx >= m_width || gz < 0 || gz >= m_height) return result;

    // ゴールが壁の場合、一番近い歩けるマスを探す
    if (IsBlocked(gx, gz))
    {
        // 周囲5マス以内で歩けるマスを探す
        bool found = false;
        for (int r = 1; r <= 5 && !found; r++)
        {
            for (int dx = -r; dx <= r && !found; dx++)
            {
                for (int dz = -r; dz <= r && !found; dz++)
                {
                    if (abs(dx) == r || abs(dz) == r)  // 外周だけチェック
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
        if (!found) return result;  // 近くに歩けるマスがない
    }

    // スタートが壁の中にいる場合も同様
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

    // スタートとゴールが同じマス
    if (sx == gx && sz == gz)
    {
        result.push_back({ goalX, 0.0f, goalZ });
        return result;
    }

    // --- A* 用のデータ構造 ---

    // 1マスの情報を格納するノード
    struct Node
    {
        int x, z;       // グリッド座標
        float f, g;     // f=総合評価, g=スタートからの実コスト
        int parentIdx;  // 親ノードのインデックス（経路復元用）
    };

    // fが小さい順に取り出す比較関数
    struct CompareF
    {
        bool operator()(const std::pair<float, int>& a,
            const std::pair<float, int>& b) const
        {
            return a.first > b.first;  // 小さいfを優先
        }
    };

    // ヒューリスティック関数（チェビシェフ距離）
    // 8方向移動なので、斜め1歩=1.41、直線1歩=1.0
    auto heuristic = [](int x1, int z1, int x2, int z2) -> float
        {
            int dx = abs(x1 - x2);
            int dz = abs(z1 - z2);
            // 斜め移動のコストを考慮した距離
            return (float)(dx + dz) + (1.414f - 2.0f) * (float)(dx < dz ? dx : dz);
        };

    // 全ノードを格納する配列
    std::vector<Node> allNodes;
    allNodes.reserve(1024);

    // 各マスの最良gコストを記録（-1=未訪問）
    std::vector<float> bestG(m_width * m_height, -1.0f);

    // オープンリスト（priority_queue: fが小さい順）
    std::priority_queue <
        std::pair<float, int>,             // (f値, ノードインデックス)
        std::vector<std::pair<float, int>>,
        CompareF
            > openList;

            // --- スタートノードを// ---
            float h0 = heuristic(sx, sz, gx, gz);
            allNodes.push_back({ sx, sz, h0, 0.0f, -1 });
            bestG[sz * m_width + sx] = 0.0f;
            openList.push({ h0, 0 });

            // 8方向の隣接マス（上下左右＋斜め）
            const int dx8[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
            const int dz8[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
            const float cost8[] = {
                1.414f, 1.0f, 1.414f,   // 左上, 上, 右上
                1.0f,         1.0f,     // 左,      右
                1.414f, 1.0f, 1.414f    // 左下, 下, 右下
            };

            int goalNodeIdx = -1;

            // 探索回数の上限（無限ループ防止）
            int maxIterations = (m_width * m_height) / 4;
            int iterations = 0;

            // --- A* メインループ ---
            while (!openList.empty() && iterations < maxIterations)
            {
                iterations++;

                // fが最小のノードを取り出す
                float f = openList.top().first;
                int nodeIdx = openList.top().second;
                openList.pop();

                Node& current = allNodes[nodeIdx];

                // ゴールに到達！
                if (current.x == gx && current.z == gz)
                {
                    goalNodeIdx = nodeIdx;
                    break;
                }

                // すでにより良い経路で訪問済みならスキップ
                float recordedG = bestG[current.z * m_width + current.x];
                if (recordedG >= 0 && current.g > recordedG + 0.01f)
                    continue;

                // 8方向の隣接マスを調べる
                for (int d = 0; d < 8; d++)
                {
                    int nx = current.x + dx8[d];
                    int nz = current.z + dz8[d];

                    // 範囲外・壁はスキップ
                    if (IsBlocked(nx, nz)) continue;

                    // 斜め移動の場合、隣接する直線マスも通れるか確認
                    // （壁の角をすり抜けないように）
                    if (dx8[d] != 0 && dz8[d] != 0)
                    {
                        if (IsBlocked(current.x + dx8[d], current.z) ||
                            IsBlocked(current.x, current.z + dz8[d]))
                            continue;  // 壁の角はすり抜け禁止
                    }

                    float newG = current.g + cost8[d];
                    int cellIdx = nz * m_width + nx;

                    // このマスに初めて来た or より短い経路を見つけた
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

            // --- 経路が見つからなかった ---
            if (goalNodeIdx < 0) return result;

            // --- 経路を復元（ゴール→スタートの順にたどる） ---
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

            // 逆順にして スタート→ゴール の順にする
            result.reserve(reversePath.size());
            for (int i = (int)reversePath.size() - 1; i >= 0; i--)
            {
                result.push_back(reversePath[i]);
            }

            return result;
}