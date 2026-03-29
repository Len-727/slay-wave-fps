// NavGrid.h
// ナビゲーショングリッド ? マップの歩行可能領域を2Dグリッドで管理
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
    // === 初期化：グリッドを生成する ===
    // minX, maxX, minZ, maxZ: マップの範囲（ワールド座標）
    // cellSize: 1マスの大きさ（メートル）
    // testRadius: 歩行判定に使う球の半径
    void Initialize(
        float minX, float maxX,
        float minZ, float maxZ,
        float cellSize = 1.0f,
        float testRadius = 0.4f);

    // === 1マスが歩けるかどうかを設定 ===
    // gx, gz: グリッド座標（0始まり）
    // blocked: true=壁、false=歩ける
    void SetBlocked(int gx, int gz, bool blocked);

    // === 1マスが歩けるか判定 ===
    bool IsBlocked(int gx, int gz) const;

    // === ワールド座標 → グリッド座標 ===
    int WorldToGridX(float worldX) const;
    int WorldToGridZ(float worldZ) const;

    // === グリッド座標 → ワールド座標（マスの中心） ===
    float GridToWorldX(int gx) const;
    float GridToWorldZ(int gz) const;

    // === グリッドのサイズ取得 ===
    int GetWidth()  const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetCellSize() const { return m_cellSize; }

    // === デバッグ用：壁の数と合計マスを返す ===
    int CountBlocked() const;
    int CountTotal()   const { return m_width * m_height; }

    // === グリッドが初期化済みか ===
    bool IsReady() const { return m_ready; }

    // === 経路探索 ===
   // startX/Z, goalX/Z: ワールド座標
   // 戻り値: 通過するワールド座標の配列（空=経路なし）
    std::vector<DirectX::XMFLOAT3> FindPath(
        float startX, float startZ,
        float goalX, float goalZ) const;

private:
    std::vector<uint8_t> m_grid;   // 0=歩ける, 1=壁
    int   m_width = 0;            // X方向のマス数
    int   m_height = 0;            // Z方向のマス数
    float m_cellSize = 1.0f;       // 1マスのサイズ（m）
    float m_minX = 0, m_minZ = 0;  // ワールド座標の原点
    float m_testRadius = 0.4f;     // 歩行判定用の球の半径
    bool  m_ready = false;
};