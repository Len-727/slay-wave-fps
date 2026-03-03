// RankingSystem.cpp
// 【役割】ランキングの保存・読み込み・ソート・追加を実装
// 【保存形式】バイナリファイル（高速＆コンパクト）
//   ヘッダー: MAGIC(4byte) + VERSION(4byte) + COUNT(4byte)
//   データ: RankingEntry × COUNT件

#define NOMINMAX            // Windows.hのmin/maxマクロを無効化
#include <windows.h>        // OutputDebugStringA（デバッグ出力）
#include "RankingSystem.h"
#include <fstream>          // ファイル入出力（ifstream, ofstream）
#include <algorithm>        // std::sort（ソート用）
#include <ctime>            // time()（現在時刻取得）

// =============================================
// コンストラクタ
// 【処理】ファイルが存在すれば読み込む
// =============================================
RankingSystem::RankingSystem()
{
    // 起動時にファイルから読み込み
    if (!Load())
    {
        // 読み込み失敗（初回起動 or ファイル破損）
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
// AddEntry - スコアを追加してファイルに保存
// =============================================
int RankingSystem::AddEntry(const RankingEntry& entry)
{
    // --- タイムスタンプを記録 ---
    RankingEntry newEntry = entry;
    newEntry.timestamp = time(nullptr);     // 現在のUNIX時間

    // --- 配列に追加 ---
    m_entries.push_back(newEntry);

    // --- スコア降順でソート ---
    SortEntries();

    // --- 10件を超えたら切り詰め ---
    if ((int)m_entries.size() > MAX_ENTRIES)
    {
        m_entries.resize(MAX_ENTRIES);
    }

    // --- 追加したエントリーの順位を探す ---
    int rank = -1;  // ランク外
    for (int i = 0; i < (int)m_entries.size(); i++)
    {
        // タイムスタンプとスコアが一致するものを探す
        if (m_entries[i].score == newEntry.score &&
            m_entries[i].timestamp == newEntry.timestamp)
        {
            rank = i;   // 0=1位, 1=2位...
            break;
        }
    }

    // --- ファイルに保存 ---
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
// IsNewRecord - Top10に入るか判定
// =============================================
bool RankingSystem::IsNewRecord(int score) const
{
    // まだ10件未満なら無条件でランクイン
    if ((int)m_entries.size() < MAX_ENTRIES)
    {
        return true;
    }

    // 最下位（10位）のスコアより高ければランクイン
    return score > m_entries.back().score;
}

// =============================================
// Load - バイナリファイルから読み込み
// 【ファイル構造】
//   [MAGIC: 4byte] [VERSION: 4byte] [COUNT: 4byte]
//   [RankingEntry × COUNT]
// =============================================
bool RankingSystem::Load()
{
    // --- ファイルを開く ---
    // ios::binary: バイナリモード（テキスト変換しない）
    std::ifstream file(SAVE_FILE, std::ios::binary);
    if (!file.is_open())
    {
        return false;   // ファイルが存在しない（初回起動）
    }

    // --- ヘッダー読み込み ---
    unsigned int magic = 0;
    unsigned int version = 0;
    int count = 0;

    // read(): バイト列をそのまま読み込む
    // reinterpret_cast<char*>: 任意の型のポインタをchar*に変換
    //   （read()がchar*を要求するため）
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    // --- ヘッダー検証 ---
    if (magic != FILE_MAGIC)
    {
        OutputDebugStringA("[RANKING] ERROR: Invalid file magic\n");
        return false;   // 違うファイル or 破損
    }

    if (version != FILE_VERSION)
    {
        OutputDebugStringA("[RANKING] ERROR: Version mismatch\n");
        return false;   // バージョン違い
    }

    // 異常な件数チェック（破損防止）
    if (count < 0 || count > MAX_ENTRIES)
    {
        OutputDebugStringA("[RANKING] ERROR: Invalid entry count\n");
        return false;
    }

    // --- データ読み込み ---
    m_entries.clear();
    m_entries.resize(count);

    // 全エントリーを一括読み込み
    // sizeof(RankingEntry) × count バイトを読む
    file.read(reinterpret_cast<char*>(m_entries.data()),
        sizeof(RankingEntry) * count);

    // 読み込みが途中で失敗していないか確認
    if (!file.good() && !file.eof())
    {
        OutputDebugStringA("[RANKING] ERROR: Read failed\n");
        m_entries.clear();
        return false;
    }

    // 念のためソート
    SortEntries();

    return true;
}

// =============================================
// Save - バイナリファイルに保存
// =============================================
bool RankingSystem::Save() const
{
    // --- ファイルを開く（新規作成 or 上書き） ---
    std::ofstream file(SAVE_FILE, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        OutputDebugStringA("[RANKING] ERROR: Cannot open file for writing\n");
        return false;
    }

    // --- ヘッダー書き込み ---
    unsigned int magic = FILE_MAGIC;
    unsigned int version = FILE_VERSION;
    int count = (int)m_entries.size();

    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // --- データ書き込み ---
    if (count > 0)
    {
        file.write(reinterpret_cast<const char*>(m_entries.data()),
            sizeof(RankingEntry) * count);
    }

    return file.good();
}

// =============================================
// SortEntries - スコア降順でソート
// =============================================
void RankingSystem::SortEntries()
{
    // std::sort + ラムダ式でスコア降順ソート
    // a.score > b.score なら a が先に来る（降順）
    std::sort(m_entries.begin(), m_entries.end(),
        [](const RankingEntry& a, const RankingEntry& b)
        {
            // スコアが同じなら到達ウェーブで比較
            if (a.score == b.score)
                return a.wave > b.wave;
            return a.score > b.score;
        });
}

// =============================================
// Clear - ランキングを全消去（デバッグ用）
// =============================================
void RankingSystem::Clear()
{
    m_entries.clear();
    Save();
    OutputDebugStringA("[RANKING] Cleared all entries\n");
}