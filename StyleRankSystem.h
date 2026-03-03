// StyleRankSystem.h
// 【役割】スタイルランクシステム
// 【機能】キルや被ダメに応じてランクを上下させ、報酬倍率を返す
#pragma once

#include <string>
#include <DirectXMath.h>

// === スタイルランクの段階 ===
// enumは「名前付きの数値」。0から順に番号が振られる
// D=0, C=1, B=2 ... SSS=6
enum class StyleRank
{
    D = 0,      // 最低ランク
    C,          // 普通
    B,          // まあまあ
    A,          // 上手い
    S,          // すごい
    SS,         // ヤバい
    SSS         // 神
};


// === スタイルランクシステム本体 ===
class StyleRankSystem
{
public:
    // --- コンストラクタ ---
    StyleRankSystem();

    // --- 毎フレーム呼ぶ（時間経過でポイント減少） ---
    // 【引数】deltaTime: 前フレームからの経過秒数（例: 0.016 = 60FPS時）
    void Update(float deltaTime);

    // --- キルした時に呼ぶ ---
    // 【引数】isHeadshot: ヘッドショットだったか
    //         multiKillCount: 同時キル数（1=通常, 2=ダブル, 3=トリプル...）
    void NotifyKill(bool isHeadshot, int multiKillCount = 1);

    // --- ダメージを受けた時に呼ぶ ---
    void NotifyDamage();

    // --- パリィ成功時に呼ぶ --- 
    void NotifyParry();

    // --- 現在のランクを取得 ---
    StyleRank GetRank() const { return m_currentRank; }

    // --- ランク名を文字列で取得（HUD表示用） ---
    // 例: "SSS", "A", "D"
    const char* GetRankName() const;

    // --- ランクに応じた色を取得（HUD表示用） ---
    // D=灰, C=白, B=青, A=緑, S=黄, SS=オレンジ, SSS=赤
    DirectX::XMFLOAT4 GetRankColor() const;

    // --- 現在のスタイルポイントを取得 ---
    float GetStylePoints() const { return m_stylePoints; }

    // --- 次のランクまでの進捗（0.0?1.0）ゲージ表示用 ---
    float GetProgress() const;

    // --- ランクに応じたHP回復量を取得 ---
    int GetHealAmount() const;

    // --- コンボ数を取得 ---
    int GetComboCount() const { return m_comboCount; }

    // --- コンボタイマー残りを取得（0.0?1.0）ゲージ表示用 ---
    float GetComboTimerRatio() const;

    // --- リセット（ゲーム開始時） ---
    void Reset();

private:
    float m_stylePoints;        // 現在のスタイルポイント
    StyleRank m_currentRank;    // 現在のランク

    int m_comboCount;           // 現在のコンボ数（連続キル）
    float m_comboTimer;         // コンボが途切れるまでの残り時間（秒）

    // --- 内部関数：ポイントからランクを再計算 ---
    void UpdateRank();

    // === 定数 ===
    static constexpr float COMBO_TIME_LIMIT = 3.0f;     // コンボ制限時間（秒）
    static constexpr float DECAY_PER_SECOND = 15.0f;    // 毎秒の自然減少
    static constexpr float DAMAGE_PENALTY = 200.0f;     // 被ダメ時のペナルティ

    // 各ランクに必要なポイント（配列）
    // D=0, C=100, B=250, A=500, S=800, SS=1200, SSS=1800
    static constexpr float RANK_THRESHOLDS[7] = {
        0.0f, 100.0f, 250.0f, 500.0f, 800.0f, 1200.0f, 1800.0f
    };
};