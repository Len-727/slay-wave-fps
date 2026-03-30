// StyleRankSystem.cpp
// 【役割】スタイルランクの全ロジックを実装
// 【機能】キル→ポイント加算→ランク更新、時間経過→減少、被ダメ→ペナルティ

#include "StyleRankSystem.h"
#include <algorithm>    // std::min, std::max を使う（値の範囲制限用）

// =============================================
// コンストラクタ（初期化）
// =============================================
StyleRankSystem::StyleRankSystem()
    : m_stylePoints(0.0f)       // ポイント0からスタート
    , m_currentRank(StyleRank::D)   // ランクDからスタート
    , m_comboCount(0)           // コンボなし
    , m_comboTimer(0.0f)        // タイマーなし
{
}

// =============================================
// 毎フレーム更新
// 【やること】
//   1. コンボタイマーを減らす → 0になったらコンボリセット
//   2. スタイルポイントを自然減少させる
//   3. ランクを再計算
// =============================================
void StyleRankSystem::Update(float deltaTime)
{
    // --- コンボタイマー処理 ---
    if (m_comboCount > 0)
    {
        m_comboTimer -= deltaTime;  // 毎フレーム経過時間ぶん減らす

        if (m_comboTimer <= 0.0f)
        {
            // コンボ途切れた！
            m_comboCount = 0;
            m_comboTimer = 0.0f;
        }
    }

    // --- スタイルポイント自然減少 ---
    // 常に攻め続けないとランクが下がる仕組み
    m_stylePoints -= DECAY_PER_SECOND * deltaTime;

    // 0未満にはならない
    if (m_stylePoints < 0.0f)
    {
        m_stylePoints = 0.0f;
    }

    // --- ランク再計算 ---
    UpdateRank();
}

// =============================================
// キル通知
// 【引数】
//   isHeadshot: ヘッドショットだったか（true/false）
//   multiKillCount: 同時キル数（1=通常キル, 2=ダブルキル...）
// =============================================
void StyleRankSystem::NotifyKill(bool isHeadshot, int multiKillCount)
{
    // --- 基本ポイント ---
    float points = 100.0f;  // 通常キル = 100pt

    // --- ヘッドショットボーナス ---
    if (isHeadshot)
    {
        points = 150.0f;    // ヘッドショット = 150pt
    }

    // --- マルチキルボーナス ---
    // 2体同時 = 200pt, 3体 = 350pt, 4体以上はさらに増加
    if (multiKillCount >= 2)
    {
        // 2体: 200, 3体: 350, 4体: 550...
        // 計算式: 100 × マルチキル数 × 1.0?1.5倍
        points = 100.0f * multiKillCount * (1.0f + (multiKillCount - 1) * 0.25f);
    }

    // --- コンボ更新 ---
    m_comboCount++;                     // コンボ数+1
    m_comboTimer = COMBO_TIME_LIMIT;    // タイマーリセット（3秒）

    // --- コンボボーナス ---
    // コンボが続くほどボーナスが加速する
    // 例: 5コンボ目 → +100pt//, 10コンボ目 → +200pt//
    float comboBonus = m_comboCount * 20.0f;
    points += comboBonus;

    // --- ポイント加算 ---
    m_stylePoints += points;

    // --- ランク再計算 ---
    UpdateRank();
}

// =============================================
// ダメージ通知
// 被弾するとポイントが大きく減る → ランク維持には上手く避けることが重要
// =============================================
void StyleRankSystem::NotifyDamage()
{
    m_stylePoints -= DAMAGE_PENALTY;    // -200pt

    if (m_stylePoints < 0.0f)
    {
        m_stylePoints = 0.0f;
    }

    // ランク再計算
    UpdateRank();
}

// =============================================
// パリィ通知
// パリィ成功 → スタイルポイント加算（コンボも更新）
// =============================================
void StyleRankSystem::NotifyParry()
{
    // --- パリィ基本ポイント ---
    float points = 120.0f;  // キル(100)より少し高い（技術を評価）

    // --- コンボ更新 ---
    m_comboCount++;
    m_comboTimer = COMBO_TIME_LIMIT;

    // --- コンボボーナス ---
    float comboBonus = m_comboCount * 20.0f;
    points += comboBonus;

    // --- ポイント加算 ---
    m_stylePoints += points;

    // --- ランク再計算 ---
    UpdateRank();
}

// =============================================
// ランク再計算（内部関数）
// ポイントが各閾値を超えているか上からチェック
// =============================================
void StyleRankSystem::UpdateRank()
{
    // SSS(6)から順にチェックして、最初に超えたランクを採用
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
// ランク名を文字列で返す（HUD表示用）
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
// ランクに応じた色を返す（HUD表示用）
// XMFLOAT4 = (R, G, B, A) で 0.0?1.0 の範囲
// =============================================
DirectX::XMFLOAT4 StyleRankSystem::GetRankColor() const
{
    switch (m_currentRank)
    {
    case StyleRank::D:   return { 0.5f, 0.5f, 0.5f, 1.0f };   // 灰色
    case StyleRank::C:   return { 1.0f, 1.0f, 1.0f, 1.0f };   // 白
    case StyleRank::B:   return { 0.3f, 0.5f, 1.0f, 1.0f };   // 青
    case StyleRank::A:   return { 0.2f, 1.0f, 0.3f, 1.0f };   // 緑
    case StyleRank::S:   return { 1.0f, 1.0f, 0.0f, 1.0f };   // 黄
    case StyleRank::SS:  return { 1.0f, 0.5f, 0.0f, 1.0f };   // オレンジ
    case StyleRank::SSS: return { 1.0f, 0.0f, 0.0f, 1.0f };   // 赤
    default:             return { 0.5f, 0.5f, 0.5f, 1.0f };
    }
}

// =============================================
// 次のランクまでの進捗（0.0?1.0）
// ゲージ表示に使う
// =============================================
float StyleRankSystem::GetProgress() const
{
    int rankIndex = static_cast<int>(m_currentRank);

    // SSSなら常に満タン
    if (rankIndex >= 6)
    {
        return 1.0f;
    }

    // 現在のランクの閾値と次のランクの閾値を取得
    float currentThreshold = RANK_THRESHOLDS[rankIndex];
    float nextThreshold = RANK_THRESHOLDS[rankIndex + 1];

    // 0除算防止
    float range = nextThreshold - currentThreshold;
    if (range <= 0.0f) return 0.0f;

    // 進捗 = (現在ポイント - 現在ランク閾値) / (次ランク閾値 - 現在ランク閾値)
    float progress = (m_stylePoints - currentThreshold) / range;

    // 0.0?1.0の範囲に収める
    return std::min(1.0f, std::max(0.0f, progress));
}

// =============================================
// ランクに応じたHP回復量
// 高ランクほど倒した時にたくさん回復する
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
// コンボタイマーの残り（0.0?1.0）
// コンボゲージ表示用
// =============================================
float StyleRankSystem::GetComboTimerRatio() const
{
    if (m_comboCount == 0) return 0.0f;

    // タイマー残り / 制限時間 = 0.0?1.0
    return std::min(1.0f, std::max(0.0f, m_comboTimer / COMBO_TIME_LIMIT));
}

// =============================================
// リセット（ゲーム開始時に呼ぶ）
// =============================================
void StyleRankSystem::Reset()
{
    m_stylePoints = 0.0f;
    m_currentRank = StyleRank::D;
    m_comboCount = 0;
    m_comboTimer = 0.0f;
}
