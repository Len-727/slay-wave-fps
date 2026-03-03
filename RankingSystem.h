// RankingSystem.h
// 【役割】ランキングデータの管理（保存・読み込み・ソート）
// 【永続化】バイナリファイル "ranking.dat" に保存
//          ゲームを閉じてもデータが残る

#pragma once

#include <string>
#include <vector>
#include <ctime>        // time_t（日時記録用）

// =============================================
// ランキング1件分のデータ
// =============================================
struct RankingEntry
{
    int   score;            // 合計スコア（m_goTotalScore）
    int   wave;             // 到達ウェーブ
    int   kills;            // キル数
    int   headshots;        // ヘッドショット数
    int   rank;             // ランク (0=C, 1=B, 2=A, 3=S)
    float survivalTime;     // 生存時間（秒）
    time_t timestamp;       // 記録した日時（UNIX時間）

    // // プレイヤー名（最大15文字 + null終端）
    char  name[16];

    // デフォルトコンストラクタ: 全部0で初期化
    RankingEntry()
        : score(0), wave(0), kills(0), headshots(0)
        , rank(0), survivalTime(0.0f), timestamp(0)
    {
        memset(name, 0, sizeof(name));
    }
};

// =============================================
// ランキング管理クラス
// =============================================
class RankingSystem
{
public:
    // --- 定数 ---
    static constexpr int MAX_ENTRIES = 10;  // Top10まで保存

    // --- コンストラクタ ---
    RankingSystem();

    // --- スコアを追加 ---
    int AddEntry(const RankingEntry& entry);

    // --- ランキング一覧を取得 ---
    const std::vector<RankingEntry>& GetEntries() const { return m_entries; }

    // --- 新記録かどうか判定 ---
    bool IsNewRecord(int score) const;

    // --- ファイル操作 ---
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