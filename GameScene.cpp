// ============================================================
//  GameScene.cpp
//  シーン管理（タイトル/ローディング/ゲームオーバー/ランキング/フェード）
//
//  【責務】
//  - 各シーンのUpdate（入力受付・状態遷移・タイマー進行）
//  - 各シーンのRender（SpriteBatch/PrimitiveBatchでUI描画）
//  - フェードイン/フェードアウトの制御
//
//  【設計意図】
//  ゲームプレイ以外の「画面遷移」を集約。
//  将来的にStateパターンに移行する場合も、
//  このファイルを分割するだけで済む。
//
//  【含まれるシーン】
//  1. Title    - TitleSceneクラスの更新/描画 + SPACE→LOADING
//  2. Loading  - フェイクローディングバー + Enter→PLAYING
//  3. GameOver - YOU DIED演出 + スコア表示 + 名前入力
//  4. Ranking  - リーダーボード表示
//  5. Fade     - シーン遷移時の黒フェード
// ============================================================

#include "Game.h"
#include <algorithm>

using namespace DirectX;

// ============================================================
//  シーン共通定数
// ============================================================
namespace SceneConstants
{
    // deltaTime（固定フレームレート想定）
    constexpr float FIXED_DT = 1.0f / 60.0f;

    // フェード速度（秒あたりのアルファ変化量）
    constexpr float FADE_SPEED = 2.0f;

    // ゲームオーバー演出タイミング
    constexpr float GO_COUNTUP_START = 1.5f;     // スコアカウントアップ開始時刻
    constexpr float GO_COUNTUP_DURATION = 2.0f;   // カウントアップにかかる時間
    constexpr float GO_STAT_DELAY = 0.15f;         // スタッツの段階的表示の間隔
    constexpr float GO_STAT_DURATION = 0.6f;       // 各スタッツのカウントアップ時間
    constexpr float GO_NOISE_DURATION = 0.5f;      // ノイズトランジションの長さ
    constexpr float GO_NAME_INPUT_START = 4.5f;    // 名前入力開始タイミング
    constexpr float GO_INPUT_DELAY = 0.5f;         // 入力受付開始の遅延

    // ローディング画面タイミング
    constexpr float LOAD_PHASE_0_END = 0.5f;
    constexpr float LOAD_PHASE_1_END = 1.2f;
    constexpr float LOAD_PHASE_2_END = 1.8f;
    constexpr float LOAD_PHASE_3_END = 2.5f;
    constexpr float LOAD_PHASE_4_END = 3.0f;
    constexpr float LOAD_BAR_LERP_SPEED = 3.0f;

    // スコア計算の倍率
    constexpr int KILL_POINTS = 10;
    constexpr int HEADSHOT_POINTS = 25;
    constexpr int MELEE_KILL_POINTS = 30;
    constexpr int COMBO_POINTS = 20;
    constexpr int PARRY_POINTS = 50;
    constexpr int SURVIVAL_POINTS_PER_SEC = 2;
    constexpr int DAMAGE_DIVISOR = 10;
    constexpr int NO_DAMAGE_BONUS = 1000;
    constexpr int STYLE_RANK_POINTS = 200;

    // ランク閾値
    constexpr int RANK_S_THRESHOLD = 8000;
    constexpr int RANK_A_THRESHOLD = 5000;
    constexpr int RANK_B_THRESHOLD = 2500;

    // 名前入力の最大文字数
    constexpr int MAX_NAME_LENGTH = 15;
}

// ============================================================
//  イージング関数（RenderGameOver / RenderRanking 共通）
// ============================================================
namespace Easing
{
    // じわっと減速（オーバーレイ向き）
    float OutQuad(float t)
    {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    // もう少し強い減速（スライドイン向き）
    float OutCubic(float t)
    {
        float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    // 目的地を行き過ぎてバネで戻る（装飾線向き）
    float OutBack(float t)
    {
        constexpr float OVERSHOOT = 1.70158f;
        float u = t - 1.0f;
        return 1.0f + (OVERSHOOT + 1.0f) * u * u * u + OVERSHOOT * u * u;
    }

    // バウンド（YOU DIED向き）
    float OutBounce(float t)
    {
        constexpr float N1 = 7.5625f;
        constexpr float D1 = 2.75f;
        if (t < 1.0f / D1)
            return N1 * t * t;
        else if (t < 2.0f / D1) {
            t -= 1.5f / D1;
            return N1 * t * t + 0.75f;
        }
        else if (t < 2.5f / D1) {
            t -= 2.25f / D1;
            return N1 * t * t + 0.9375f;
        }
        else {
            t -= 2.625f / D1;
            return N1 * t * t + 0.984375f;
        }
    }
}


// ============================================================
//  UpdateTitle - タイトル画面の更新
//
//  【入力】SPACE → ローディングへ、L → ランキングへ
//  【処理】TitleSceneの更新 + フェード更新
// ============================================================
void Game::UpdateTitle()
{
    // TitleScene の更新
    if (m_titleScene)
    {
        m_titleScene->Update(SceneConstants::FIXED_DT);
    }

    // フェード更新
    UpdateFade();

    // SPACEキーでゲーム開始
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        ResetGame();

        // ローディング画面の初期化
        m_loadingTimer = 0.0f;
        m_loadingPhase = 0;
        m_loadingBarTarget = 0.0f;
        m_loadingBarCurrent = 0.0f;

        m_gameState = GameState::LOADING;
    }

    // Lキーでランキング画面
    if (GetAsyncKeyState('L') & 0x8000)
    {
        m_rankingTimer = 0.0f;
        m_newRecordRank = -1;
        m_gameState = GameState::RANKING;
    }
}

// ============================================================
//  UpdateLoading - ローディング画面の更新
//
//  【フェイクローディング】実際のリソース読み込みは
//  Initialize()で完了済み。演出としてのローディング画面。
//  5フェーズ（INIT→ARSENAL→ENTITIES→CALIBRATE→READY）
// ============================================================
void Game::UpdateLoading()
{
    m_loadingTimer += SceneConstants::FIXED_DT;

    // フェーズ進行（時間でメッセージとバーの目標値を切り替え）
    if (m_loadingTimer < SceneConstants::LOAD_PHASE_0_END)
    {
        m_loadingPhase = 0;       // "INITIALIZING SYSTEMS..."
        m_loadingBarTarget = 0.1f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_1_END)
    {
        m_loadingPhase = 1;       // "LOADING ARSENAL..."
        m_loadingBarTarget = 0.3f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_2_END)
    {
        m_loadingPhase = 2;       // "SPAWNING ENTITIES..."
        m_loadingBarTarget = 0.55f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_3_END)
    {
        m_loadingPhase = 3;       // "CALIBRATING GOTHIC FREQUENCIES..."
        m_loadingBarTarget = 0.78f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_4_END)
    {
        m_loadingPhase = 4;       // "READY"
        m_loadingBarTarget = 1.0f;
    }

    // バーのスムーズ補間（Lerp）
    float lerpSpeed = SceneConstants::LOAD_BAR_LERP_SPEED * SceneConstants::FIXED_DT;
    m_loadingBarCurrent += (m_loadingBarTarget - m_loadingBarCurrent) * lerpSpeed;

    // ぴったり目標に近づいたらスナップ
    constexpr float SNAP_THRESHOLD = 0.001f;
    if (fabsf(m_loadingBarCurrent - m_loadingBarTarget) < SNAP_THRESHOLD)
        m_loadingBarCurrent = m_loadingBarTarget;

    // READY後、Enterキーで遷移
    constexpr int READY_PHASE = 4;
    if (m_loadingPhase == READY_PHASE)
    {
        m_loadingBarCurrent = 1.0f;

        static bool enterPressed = false;
        if (GetAsyncKeyState(VK_RETURN) & 0x8000)
        {
            if (!enterPressed)
            {
                m_fadeAlpha = 1.0f;
                m_fadingIn = true;
                m_fadeActive = true;
                m_gameState = GameState::PLAYING;
                enterPressed = true;
            }
        }
        else
        {
            enterPressed = false;
        }
    }
}

// ============================================================
//  UpdateFade - フェードイン/フェードアウトの更新
//
//  【仕組み】m_fadingIn=true → アルファ減少（明るくなる）
//           m_fadingIn=false → アルファ増加（暗くなる）
// ============================================================
void Game::UpdateFade()
{
    if (!m_fadeActive) return;

    float fadeSpeed = SceneConstants::FADE_SPEED * SceneConstants::FIXED_DT;

    if (m_fadingIn)
    {
        m_fadeAlpha -= fadeSpeed;
        if (m_fadeAlpha <= 0.0f)
        {
            m_fadeAlpha = 0.0f;
            m_fadeActive = false;
        }
    }
    else
    {
        m_fadeAlpha += fadeSpeed;
        if (m_fadeAlpha >= 1.0f)
        {
            m_fadeAlpha = 1.0f;
            m_fadeActive = false;
        }
    }
}

// ============================================================
//  UpdateRanking - ランキング画面の更新
//
//  【入力】ESC → タイトルに戻る、R → リスタート
// ============================================================
void Game::UpdateRanking()
{
    m_rankingTimer += SceneConstants::FIXED_DT;

    // ESCキーでタイトルに戻る（誤操作防止で0.5秒後から受付）
    if (m_rankingTimer > SceneConstants::GO_INPUT_DELAY &&
        (GetAsyncKeyState(VK_ESCAPE) & 0x8000))
    {
        m_gameState = GameState::TITLE;
        m_rankingTimer = 0.0f;
    }

    // Rキーでリスタート
    if (m_rankingTimer > SceneConstants::GO_INPUT_DELAY &&
        (GetAsyncKeyState('R') & 0x8000))
    {
        ResetGame();
        m_gameOverTimer = 0.0f;
        m_gameOverScore = 0;
        m_gameOverWave = 0;
        m_gameOverCountUp = 0.0f;
        m_gameOverRank = 0;
        m_gameOverNoiseT = 0.0f;
        m_gameState = GameState::TITLE;
    }
}


// ============================================================
//  UpdateGameOver - ゲームオーバー画面の更新
//
//  【処理フロー】
//  1. 初回: スコア・スタッツ計算 + ランク判定
//  2. 演出: カウントアップアニメーション
//  3. 名前入力: A-Z, 0-9, Backspace, Enter
//  4. 保存後: R→リスタート、L→ランキング
//
//  【スコア計算式】
//  合計 = キル×10 + HS×25 + 近接×30 + コンボ×20
//       + パリィ×50 + 生存秒×2 + 与ダメ/10
//       + (被ダメ0なら1000, 否なら5000/(被ダメ+1))
//       + スタイルランク×200
// ============================================================
void Game::UpdateGameOver()
{
    // 経過時間を加算（全演出のタイミング制御に使用）
    m_gameOverTimer += SceneConstants::FIXED_DT;

    // --- 初回のみ：スコアとスタッツを計算 ---
    if (m_gameOverWave == 0)
    {
        m_gameOverScore = m_player->GetPoints();
        m_gameOverWave = m_waveManager->GetCurrentWave();

        // スタッツをリザルト用にコピー
        m_goKills = m_statKills;
        m_goHeadshots = m_statHeadshots;
        m_goMeleeKills = m_statMeleeKills;
        m_goMaxCombo = m_statMaxCombo;
        m_goParryCount = m_parrySuccessCount;
        m_goDamageDealt = m_statDamageDealt;
        m_goDamageTaken = m_statDamageTaken;
        m_goMaxStyleRank = m_statMaxStyleRank;
        m_goSurvivalTime = m_statSurvivalTime;

        // 各スタッツにボーナスポイントを計算
        m_goStatScores[0] = m_goKills * SceneConstants::KILL_POINTS;
        m_goStatScores[1] = m_goHeadshots * SceneConstants::HEADSHOT_POINTS;
        m_goStatScores[2] = m_goMeleeKills * SceneConstants::MELEE_KILL_POINTS;
        m_goStatScores[3] = m_goMaxCombo * SceneConstants::COMBO_POINTS;
        m_goStatScores[4] = m_goParryCount * SceneConstants::PARRY_POINTS;
        m_goStatScores[5] = (int)(m_goSurvivalTime)*SceneConstants::SURVIVAL_POINTS_PER_SEC;
        m_goStatScores[6] = m_goDamageDealt / SceneConstants::DAMAGE_DIVISOR;
        m_goStatScores[7] = (m_goDamageTaken == 0) ? SceneConstants::NO_DAMAGE_BONUS :
            (int)(5000.0f / (float)(m_goDamageTaken + 1));
        m_goStatScores[8] = m_goMaxStyleRank * SceneConstants::STYLE_RANK_POINTS;

        // 合計スコア
        m_goTotalScore = 0;
        for (int i = 0; i < 9; i++)
            m_goTotalScore += m_goStatScores[i];

        // ランク判定
        if (m_goTotalScore >= SceneConstants::RANK_S_THRESHOLD)
            m_gameOverRank = 3; // S
        else if (m_goTotalScore >= SceneConstants::RANK_A_THRESHOLD)
            m_gameOverRank = 2; // A
        else if (m_goTotalScore >= SceneConstants::RANK_B_THRESHOLD)
            m_gameOverRank = 1; // B
        else
            m_gameOverRank = 0; // C

        // カウントアップ初期化
        for (int i = 0; i < 9; i++)
            m_goStatCountUp[i] = 0.0f;

        // 名前入力の準備
        if (!m_rankingSaved && !m_nameInputActive)
        {
            memset(m_nameBuffer, 0, sizeof(m_nameBuffer));
            m_nameLength = 0;
            m_nameKeyTimer = 0.0f;
        }
    }

    // --- スコアカウントアップ演出（EaseOutExpo） ---
    if (m_gameOverTimer > SceneConstants::GO_COUNTUP_START)
    {
        float countProgress = (m_gameOverTimer - SceneConstants::GO_COUNTUP_START)
            / SceneConstants::GO_COUNTUP_DURATION;
        if (countProgress > 1.0f) countProgress = 1.0f;
        m_gameOverCountUp = 1.0f - powf(2.0f, -10.0f * countProgress);
    }

    // --- スタッツの段階的カウントアップ（0.15秒ずつずらして9行） ---
    for (int i = 0; i < 9; i++)
    {
        float startTime = SceneConstants::GO_COUNTUP_START + i * SceneConstants::GO_STAT_DELAY;
        if (m_gameOverTimer > startTime)
        {
            float progress = (m_gameOverTimer - startTime) / SceneConstants::GO_STAT_DURATION;
            if (progress > 1.0f) progress = 1.0f;
            m_goStatCountUp[i] = 1.0f - powf(2.0f, -10.0f * progress);
        }
    }

    // --- ノイズトランジション（死亡直後に画面がザザッとノイズで切り替わる） ---
    if (m_gameOverTimer < SceneConstants::GO_NOISE_DURATION)
    {
        m_gameOverNoiseT = m_gameOverTimer / SceneConstants::GO_NOISE_DURATION;
    }
    else
    {
        m_gameOverNoiseT = 1.0f;
    }

    // =============================================
    // 名前入力の処理（4.5秒後に開始）
    // =============================================
    if (m_gameOverTimer > SceneConstants::GO_NAME_INPUT_START && !m_rankingSaved)
    {
        // 名前入力モード開始
        if (!m_nameInputActive)
        {
            m_nameInputActive = true;
            memset(m_nameBuffer, 0, sizeof(m_nameBuffer));
            m_nameLength = 0;
            m_nameKeyWasDown = false;
            OutputDebugStringA("[RANKING] Name input started\n");
        }

        // 今フレームで何かキーが押されているか調べる
        bool anyKeyDown = false;
        int pressedKey = 0;
        bool isBackspace = false;
        bool isEnter = false;

        if (GetAsyncKeyState(VK_RETURN) & 0x8000) { anyKeyDown = true; isEnter = true; }
        if (GetAsyncKeyState(VK_BACK) & 0x8000) { anyKeyDown = true; isBackspace = true; }

        // A-Z チェック
        if (!anyKeyDown)
        {
            for (int key = 'A'; key <= 'Z'; key++)
            {
                if (GetAsyncKeyState(key) & 0x8000)
                {
                    anyKeyDown = true;
                    pressedKey = key;
                    break;
                }
            }
        }

        // 0-9 チェック
        if (!anyKeyDown)
        {
            for (int key = '0'; key <= '9'; key++)
            {
                if (GetAsyncKeyState(key) & 0x8000)
                {
                    anyKeyDown = true;
                    pressedKey = key;
                    break;
                }
            }
        }

        // 「押した瞬間」だけ反応する（前フレームで押されてなくて、今押された）
        if (anyKeyDown && !m_nameKeyWasDown)
        {
            if (isEnter)
            {
                if (m_nameLength == 0)
                {
                    strcpy_s(m_nameBuffer, "NONAME");
                    m_nameLength = 6;
                }

                // ランキングに保存
                RankingEntry entry;
                entry.score = m_goTotalScore;
                entry.wave = m_gameOverWave;
                entry.kills = m_goKills;
                entry.headshots = m_goHeadshots;
                entry.rank = m_gameOverRank;
                entry.survivalTime = m_goSurvivalTime;
                strcpy_s(entry.name, m_nameBuffer);

                m_newRecordRank = m_rankingSystem->AddEntry(entry);
                m_rankingSaved = true;
                m_nameInputActive = false;
            }
            else if (isBackspace)
            {
                if (m_nameLength > 0)
                {
                    m_nameLength--;
                    m_nameBuffer[m_nameLength] = '\0';
                }
            }
            else if (pressedKey != 0 && m_nameLength < SceneConstants::MAX_NAME_LENGTH)
            {
                m_nameBuffer[m_nameLength] = (char)pressedKey;
                m_nameLength++;
                m_nameBuffer[m_nameLength] = '\0';
            }
        }

        m_nameKeyWasDown = anyKeyDown;
        return;  // 名前入力中は R/L キーをブロック
    }

    // --- 保存後：R→リスタート、L→ランキング ---
    if (m_gameOverTimer > SceneConstants::GO_NAME_INPUT_START &&
        (GetAsyncKeyState('R') & 0x8000))
    {
        ResetGame();
        m_gameOverTimer = 0.0f;
        m_gameOverScore = 0;
        m_gameOverWave = 0;
        m_gameOverCountUp = 0.0f;
        m_gameOverRank = 0;
        m_gameOverNoiseT = 0.0f;
        m_gameState = GameState::TITLE;
    }

    if (m_gameOverTimer > SceneConstants::GO_NAME_INPUT_START &&
        (GetAsyncKeyState('L') & 0x8000))
    {
        m_rankingTimer = 0.0f;
        m_gameState = GameState::RANKING;
    }
}


// ============================================================
//  RenderTitle - タイトル画面の描画
//
//  【描画内容】TitleSceneの3D描画 + フェードオーバーレイ
//  TitleSceneが存在しない場合は何も描画しない（安全）
// ============================================================
void Game::RenderTitle()
{
    // === 画面クリア ===
    Clear();

    // === タイトルシーンを描画 ===
    if (m_titleScene)
    {
        try
        {
            m_titleScene->Render(
                m_d3dContext.Get(),
                m_renderTargetView.Get(),    // バックバッファ
                m_depthStencilView.Get()     // 深度バッファ
            );
        }
        catch (const std::exception& e)
        {
            //OutputDebugStringA("[RENDER ERROR] TitleScene render failed: ");
            //OutputDebugStringA(e.what());
            //OutputDebugStringA("\n");
        }
    }
    else
    {
        // TitleScene がない場合はエラーメッセージを表示
        //OutputDebugStringA("[RENDER WARNING] TitleScene is null\n");
    }

    // === UI描画（オプション）===
    // タイトルテキストなどを//する場合
    /*
    m_spriteBatch->Begin();

    DirectX::XMFLOAT2 titlePos(
        m_outputWidth / 2.0f,
        m_outputHeight / 2.0f
    );

    m_fontLarge->DrawString(
        m_spriteBatch.get(),
        L"GOTHIC SWARM",
        titlePos,
        DirectX::Colors::White,
        0.0f,
        DirectX::XMFLOAT2(0.5f, 0.5f)
    );

    DirectX::XMFLOAT2 startPos(
        m_outputWidth / 2.0f,
        m_outputHeight / 2.0f + 100.0f
    );

    m_font->DrawString(
        m_spriteBatch.get(),
        L"Press ENTER to Start",
        startPos,
        DirectX::Colors::White,
        0.0f,
        DirectX::XMFLOAT2(0.5f, 0.5f)
    );

    m_spriteBatch->End();
    */

    // === フェード描画 ===
    RenderFade();
}

// ============================================================
//  RenderLoading - ローディング画面の描画
//
//  【描画構成】
//  1. "GOTHIC SWARM" タイトルロゴ（フェードイン）
//  2. フェーズメッセージ（5段階）
//  3. プログレスバー（パルス光る赤）
//  4. パーセント表示
//  5. "Press Enter" 点滅テキスト（READY後）
// ============================================================
void Game::RenderLoading()
{
    // === 画面クリア（真っ黒） ===
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), black);

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // =========================================
    // ロードメッセージ
    // =========================================
    const wchar_t* messages[] = {
        L"INITIALIZING SYSTEMS...",
        L"LOADING ARSENAL...",
        L"SPAWNING ENTITIES...",
        L"CALIBRATING GOTHIC FREQUENCIES...",
        L"READY"
    };

    const wchar_t* msg = messages[m_loadingPhase];

    // メッセージの色（最終フェーズだけ赤く）
    DirectX::XMVECTORF32 msgColor;
    if (m_loadingPhase == 4)
        msgColor = { 0.9f, 0.15f, 0.1f, 1.0f };   // 赤（READY）
    else
        msgColor = { 0.6f, 0.6f, 0.6f, 1.0f };     // グレー

    // 中央下あたりに表示
    DirectX::XMVECTOR msgSize = m_font->MeasureString(msg);
    float msgW = DirectX::XMVectorGetX(msgSize);
    float msgX = (W - msgW) * 0.5f;
    float msgY = H * 0.58f;

    m_font->DrawString(m_spriteBatch.get(), msg,
        DirectX::XMFLOAT2(msgX, msgY), msgColor);

    // =========================================
    // プログレスバー
    // =========================================
    if (m_whitePixel)
    {
        float barW = W * 0.5f;       // バーの最大幅（画面幅の50%）
        float barH = 6.0f;            // バーの高さ
        float barX = (W - barW) * 0.5f;
        float barY = H * 0.65f;

        // --- 背景（暗いグレー） ---
        RECT bgRect = {
            (LONG)barX,
            (LONG)barY,
            (LONG)(barX + barW),
            (LONG)(barY + barH)
        };
        DirectX::XMVECTORF32 bgColor = { 0.15f, 0.15f, 0.15f, 1.0f };
        m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

        // --- 進捗バー（赤） ---
        float fillW = barW * m_loadingBarCurrent;
        RECT fillRect = {
            (LONG)barX,
            (LONG)barY,
            (LONG)(barX + fillW),
            (LONG)(barY + barH)
        };

        // パルス効果（微妙に明滅）
        float pulse = 0.7f + sinf(m_loadingTimer * 4.0f) * 0.3f;
        DirectX::XMVECTORF32 barColor = { 0.8f * pulse, 0.1f * pulse, 0.05f * pulse, 1.0f };
        m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, barColor);

        // --- パーセント表示 ---
        int percent = (int)(m_loadingBarCurrent * 100.0f);
        wchar_t percentText[16];
        swprintf_s(percentText, L"%d%%", percent);

        DirectX::XMVECTOR pSize = m_font->MeasureString(percentText);
        float pW = DirectX::XMVectorGetX(pSize);
        float pX = (W - pW) * 0.5f;
        float pY = barY + barH + 10.0f;

        DirectX::XMVECTORF32 percentColor = { 0.5f, 0.5f, 0.5f, 0.8f };
        m_font->DrawString(m_spriteBatch.get(), percentText,
            DirectX::XMFLOAT2(pX, pY), percentColor);
    }

    // =========================================
    // タイトルロゴ（画面上部）
    // =========================================
    if (m_fontLarge)
    {
        const wchar_t* title = L"GOTHIC SWARM";
        DirectX::XMVECTOR titleSize = m_fontLarge->MeasureString(title);
        float titleW = DirectX::XMVectorGetX(titleSize);
        float titleX = (W - titleW) * 0.5f;
        float titleY = H * 0.3f;

        // じわっとフェードイン
        float titleAlpha = (m_loadingTimer < 0.8f) ? (m_loadingTimer / 0.8f) : 1.0f;
        DirectX::XMVECTORF32 titleColor = { 0.9f, 0.1f, 0.05f, titleAlpha };

        m_fontLarge->DrawString(m_spriteBatch.get(), title,
            DirectX::XMFLOAT2(titleX, titleY), titleColor);
    }

    // =========================================
    //  点滅するドット演出（...のアニメーション）
    // =========================================
    if (m_loadingPhase < 4)
    {
        // 0.5秒ごとにドットが増える（1?3個）
        int dots = ((int)(m_loadingTimer * 2.0f) % 3) + 1;
        wchar_t dotText[8] = L"";
        for (int i = 0; i < dots; i++)
            wcscat_s(dotText, L".");

        float dotX = msgX + msgW + 2.0f;
        DirectX::XMVECTORF32 dotColor = { 0.4f, 0.4f, 0.4f, 0.6f };
        m_font->DrawString(m_spriteBatch.get(), dotText,
            DirectX::XMFLOAT2(dotX, msgY), dotColor);
    }

    // =========================================
    // "Press Enter" 表示（READY後）
    // =========================================
    if (m_loadingPhase == 4)
    {
        const wchar_t* pressText = L"- - Press Enter - -";

        // 点滅
        float blink = (sinf(m_loadingTimer * 4.0f) + 1.0f) * 0.5f;
        float alpha = 0.4f + blink * 0.6f;

        DirectX::XMVECTOR peSize = m_font->MeasureString(pressText);
        float peW = DirectX::XMVectorGetX(peSize);
        float peX = (W - peW) * 0.5f;
        float peY = H * 0.75f;

        DirectX::XMVECTORF32 peColor = { 0.8f, 0.8f, 0.8f, alpha };
        m_font->DrawString(m_spriteBatch.get(), pressText,
            DirectX::XMFLOAT2(peX, peY), peColor);
    }

    m_spriteBatch->End();
}

// ============================================================
//  RenderGameOver - ゲームオーバー画面の描画（約800行）
//
//  【描画レイヤー】
//  Layer 0: ノイズトランジション（走査線ノイズ + 赤フラッシュ）
//  Layer 1: 赤黒ビネット（画面全体を覆う闇）
//  Layer 2: SpriteBatchでテキスト+装飾
//    - "YOU DIED"（バウンスで落ちてくる + グロー）
//    - 左パネル: スタッツ9行（カウントアップ + ドットリーダー）
//    - 右パネル: ランク文字（フラッシュ放射） + トータルスコア
//    - 名前入力UI + "Press R" / "L: RANKING"
// ============================================================
void Game::RenderGameOver()
{
    // =============================================
    // イージング関数（UI演出の心臓部）
    // 全部 t=0.0（開始）→ t=1.0（完了）で値を返す
    // =============================================

    // EaseOutQuad: じわっと減速（オーバーレイ向き）

    // EaseOutCubic: もう少し強い減速（スライドイン向き）

    // EaseOutBack: 目的地を行き過ぎてバネで戻る（装飾線向き）

    // EaseOutBounce: バウンド（YOU DIED向き）

    // 経過時間のショートカット
    float timer = m_gameOverTimer;
    float screenW = (float)m_outputWidth;
    float screenH = (float)m_outputHeight;
    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    // =============================================
    // Layer 0: ノイズトランジション（死亡直後に走査線ノイズ）
    // =============================================
    if (m_gameOverNoiseT < 1.0f && m_whitePixel && m_spriteBatch && m_states)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // ランダムな水平ラインを描画（走査線ノイズ風）
        float noiseAlpha = 1.0f - m_gameOverNoiseT;  // だんだん消える
        int numLines = (int)(60 * (1.0f - m_gameOverNoiseT));  // ライン数も減る

        for (int i = 0; i < numLines; i++)
        {
            // 疑似ランダム（フレームごとに変わる）
            float seed = sinf((float)i * 127.1f + timer * 311.7f) * 43758.5453f;
            seed = seed - floorf(seed);  // 0?1に正規化

            float lineY = seed * screenH;
            float lineH = 1.0f + seed * 3.0f;  // 1?4pxの太さ

            // ランダムなX方向オフセット（画面がズレる感じ）
            float offsetX = sinf(seed * 100.0f + timer * 50.0f) * 20.0f * noiseAlpha;

            RECT noiseRect = {
                (LONG)(offsetX), (LONG)lineY,
                (LONG)(screenW + offsetX), (LONG)(lineY + lineH)
            };

            // 白い走査線
            DirectX::XMVECTORF32 noiseColor = { 1.0f, 1.0f, 1.0f, noiseAlpha * 0.3f * seed };
            m_spriteBatch->Draw(m_whitePixel.Get(), noiseRect, noiseColor);
        }

        // 画面全体に赤いフラッシュ（一瞬だけ）
        if (m_gameOverNoiseT < 0.15f)
        {
            float flashAlpha = (0.15f - m_gameOverNoiseT) / 0.15f;
            RECT fullScreen = { 0, 0, (LONG)screenW, (LONG)screenH };
            DirectX::XMVECTORF32 flashColor = { 0.8f, 0.0f, 0.0f, flashAlpha * 0.5f };
            m_spriteBatch->Draw(m_whitePixel.Get(), fullScreen, flashColor);
        }

        m_spriteBatch->End();
    }

    auto context = m_d3dContext.Get();

    // =============================================
    // Layer 1: 赤黒ビネット（画面全体を覆う暗幕）
    // =============================================
    {
        // じわっと暗くなる（EaseOutQuadで自然な減速）
        float overlayT = min(timer / 1.5f, 1.0f);
        float overlayAlpha = Easing::OutQuad(overlayT) * 0.88f;

        DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
            screenW, screenH, 0.1f, 10.0f);

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(DirectX::XMMatrixIdentity());
        m_effect->SetVertexColorEnabled(true);
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());

        auto primBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
        primBatch->Begin();

        float hw = screenW * 0.5f;
        float hh = screenH * 0.5f;

        // 中央は赤黒、端はもっと暗い（ビネット効果）
        // 中央の色（やや赤い）
        DirectX::XMFLOAT4 centerColor(0.12f, 0.0f, 0.0f, overlayAlpha * 0.7f);
        // 端の色（ほぼ真っ黒）
        DirectX::XMFLOAT4 edgeColor(0.03f, 0.0f, 0.0f, overlayAlpha);

        // 中心点
        DirectX::VertexPositionColor vc(DirectX::XMFLOAT3(0, 0, 1.0f), centerColor);

        // 四隅
        DirectX::VertexPositionColor vTL(DirectX::XMFLOAT3(-hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vTR(DirectX::XMFLOAT3(hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBR(DirectX::XMFLOAT3(hw, -hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBL(DirectX::XMFLOAT3(-hw, -hh, 1.0f), edgeColor);

        // 中心から四隅への三角形4枚（グラデーション＝ビネット）
        primBatch->DrawTriangle(vc, vTL, vTR);  // 上
        primBatch->DrawTriangle(vc, vTR, vBR);  // 右
        primBatch->DrawTriangle(vc, vBR, vBL);  // 下
        primBatch->DrawTriangle(vc, vBL, vTL);  // 左

        primBatch->End();
    }

    // =============================================
    // Layer 2: SpriteBatchでテキスト＋装飾
    // =============================================
    if (m_spriteBatch && m_states && m_font && m_whitePixel)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // --- 装飾線（上）: 0.3秒後に左右から中央へ伸びる ---
        if (timer > 0.3f)
        {
            float lineT = min((timer - 0.3f) / 0.5f, 1.0f);
            // EaseOutBackで行き過ぎてバネで戻る
            float lineProgress = Easing::OutBack(lineT);
            float lineHalfW = 220.0f * lineProgress;  // 最大片側220px
            float lineY = centerY - 340.0f;             // YOU DIEDの上

            // 左側の線（中央から左へ伸びる）
            RECT leftLine = {
                (LONG)(centerX - lineHalfW), (LONG)lineY,
                (LONG)centerX, (LONG)(lineY + 2)
            };
            // 右側の線（中央から右へ伸びる）
            RECT rightLine = {
                (LONG)centerX, (LONG)lineY,
                (LONG)(centerX + lineHalfW), (LONG)(lineY + 2)
            };

            DirectX::XMVECTORF32 lineColor = { 0.7f, 0.15f, 0.0f, 0.9f * lineT };
            m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
            m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

            // 中央の小さなダイヤ装飾（ゴシック感）
            RECT diamond = {
                (LONG)(centerX - 4), (LONG)(lineY - 3),
                (LONG)(centerX + 4), (LONG)(lineY + 5)
            };
            m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
        }

        // --- 「YOU DIED」: 0.6秒後にバウンドしながら上から落ちる ---
        if (timer > 0.6f)
        {
            float diedT = min((timer - 0.6f) / 0.8f, 1.0f);
            // EaseOutBounceでドスンと落ちてバウンド
            float bounceT = Easing::OutBounce(diedT);
            float textAlpha = min((timer - 0.6f) * 3.0f, 1.0f);

            // 上からスライド（-80px → 0px）
            float slideY = (1.0f - bounceT) * -80.0f;
            float diedY = centerY - 320.0f + slideY;

            //  グロー（発光）：同じテキストを少しずらして半透明で複数回描く
            if (m_fontLarge)
            {
                // テキスト幅を測って手動で中央揃え
                DirectX::XMVECTOR diedSize = m_fontLarge->MeasureString(L"YOU DIED");
                float diedW = DirectX::XMVectorGetX(diedSize);
                float diedX = centerX - diedW * 0.5f;

                // グロー（4方向にずらして半透明で描く→発光して見える）
                float glowAlpha = textAlpha * 0.25f;
                DirectX::XMVECTORF32 glowColor = { 1.0f, 0.2f, 0.0f, glowAlpha };
                float glowSize = 3.0f;
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX - glowSize, diedY), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX + glowSize, diedY), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY - glowSize), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY + glowSize), glowColor);

                // 本体テキスト
                DirectX::XMVECTORF32 diedColor = { 0.95f, 0.15f, 0.05f, textAlpha };
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY), diedColor);
            }
            else
            {
                // fontLargeが無い場合は通常フォント×2倍スケール
                DirectX::XMVECTORF32 diedColor = { 0.95f, 0.15f, 0.05f, textAlpha };
                m_font->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(centerX, diedY), diedColor, 0.0f,
                    DirectX::XMFLOAT2(0.5f, 0.5f), 2.0f);
            }
        }

        // ===================================================================
        //  2カラムレイアウト
        //  左パネル: スタッツ9行   右パネル: ランク + トータルスコア
        // ===================================================================

        // --- パネル座標 ---
        const float leftL = 55.0f;
        const float leftR = 605.0f;
        const float rightL = 645.0f;
        const float rightR = 1225.0f;
        const float rightCX = (rightL + rightR) * 0.5f;
        const float panelTop = 125.0f;
        const float panelBot = 555.0f;

        // --- パネル背景（半透明ダークボックス）---
        if (timer > 0.8f)
        {
            float bgT = min((timer - 0.8f) / 0.4f, 1.0f);
            float bgA = Easing::OutCubic(bgT) * 0.2f;

            RECT leftBg = { (LONG)leftL,  (LONG)panelTop, (LONG)leftR,  (LONG)panelBot };
            RECT rightBg = { (LONG)rightL, (LONG)panelTop, (LONG)rightR, (LONG)panelBot };
            DirectX::XMVECTORF32 bgCol = { 0.05f, 0.01f, 0.0f, bgA };
            m_spriteBatch->Draw(m_whitePixel.Get(), leftBg, bgCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), rightBg, bgCol);

            // パネル枠線（暗い赤）
            float borderA = bgT * 0.35f;
            DirectX::XMVECTORF32 borderCol = { 0.5f, 0.1f, 0.0f, borderA };
            RECT bTop1 = { (LONG)leftL, (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + 1) };
            RECT bBot1 = { (LONG)leftL, (LONG)(panelBot - 1), (LONG)leftR, (LONG)panelBot };
            RECT bLft1 = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + 1), (LONG)panelBot };
            RECT bRgt1 = { (LONG)(leftR - 1), (LONG)panelTop, (LONG)leftR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), bTop1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bBot1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bLft1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bRgt1, borderCol);

            RECT bTop2 = { (LONG)rightL, (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + 1) };
            RECT bBot2 = { (LONG)rightL, (LONG)(panelBot - 1), (LONG)rightR, (LONG)panelBot };
            RECT bLft2 = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + 1), (LONG)panelBot };
            RECT bRgt2 = { (LONG)(rightR - 1), (LONG)panelTop, (LONG)rightR, (LONG)panelBot };

            //  コーナー装飾（L字型、各パネルの四隅）
            float cornerLen = 20.0f;
            float cornerThick = 2.0f;
            DirectX::XMVECTORF32 cornerCol = { 0.8f, 0.2f, 0.0f, borderA * 1.5f };

            // 左パネル四隅
            // 左上
            RECT cLT_H = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + cornerLen), (LONG)(panelTop + cornerThick) };
            RECT cLT_V = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + cornerThick), (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), cLT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cLT_V, cornerCol);
            // 右上
            RECT cRT_H = { (LONG)(leftR - cornerLen), (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + cornerThick) };
            RECT cRT_V = { (LONG)(leftR - cornerThick), (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), cRT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cRT_V, cornerCol);
            // 左下
            RECT cLB_H = { (LONG)leftL, (LONG)(panelBot - cornerThick), (LONG)(leftL + cornerLen), (LONG)panelBot };
            RECT cLB_V = { (LONG)leftL, (LONG)(panelBot - cornerLen), (LONG)(leftL + cornerThick), (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), cLB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cLB_V, cornerCol);
            // 右下
            RECT cRB_H = { (LONG)(leftR - cornerLen), (LONG)(panelBot - cornerThick), (LONG)leftR, (LONG)panelBot };
            RECT cRB_V = { (LONG)(leftR - cornerThick), (LONG)(panelBot - cornerLen), (LONG)leftR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), cRB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cRB_V, cornerCol);

            // 右パネル四隅
            RECT c2LT_H = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + cornerLen), (LONG)(panelTop + cornerThick) };
            RECT c2LT_V = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + cornerThick), (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LT_V, cornerCol);
            RECT c2RT_H = { (LONG)(rightR - cornerLen), (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + cornerThick) };
            RECT c2RT_V = { (LONG)(rightR - cornerThick), (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RT_V, cornerCol);
            RECT c2LB_H = { (LONG)rightL, (LONG)(panelBot - cornerThick), (LONG)(rightL + cornerLen), (LONG)panelBot };
            RECT c2LB_V = { (LONG)rightL, (LONG)(panelBot - cornerLen), (LONG)(rightL + cornerThick), (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LB_V, cornerCol);
            RECT c2RB_H = { (LONG)(rightR - cornerLen), (LONG)(panelBot - cornerThick), (LONG)rightR, (LONG)panelBot };
            RECT c2RB_V = { (LONG)(rightR - cornerThick), (LONG)(panelBot - cornerLen), (LONG)rightR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RB_V, cornerCol);

            m_spriteBatch->Draw(m_whitePixel.Get(), bTop2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bBot2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bLft2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bRgt2, borderCol);
        }

        // ======================
        //  左パネル: スタッツ9行
        // ======================
        if (timer > 1.2f)
        {
            const wchar_t* styleRankNames[] = { L"D", L"C", L"B", L"A", L"S", L"SS", L"SSS" };

            const float tableL = leftL + 20.0f;
            const float colVal = leftR - 140.0f;
            const float colBonus = leftR - 20.0f;
            const float statStartY = panelTop + 20.0f;
            const float rowH = 25.0f;

            const wchar_t* labels[] = {
                L"KILLS", L"HEADSHOTS", L"MELEE KILLS", L"MAX COMBO",
                L"PARRIES", L"SURVIVAL", L"DMG DEALT", L"DMG TAKEN", L"BEST STYLE"
            };
            int rawValues[] = {
                m_goKills, m_goHeadshots, m_goMeleeKills, m_goMaxCombo,
                m_goParryCount, 0, m_goDamageDealt, m_goDamageTaken, m_goMaxStyleRank
            };

            for (int i = 0; i < 9; i++)
            {
                float rowStart = 1.2f + i * 0.12f;
                if (timer <= rowStart) continue;

                float t = min((timer - rowStart) / 0.3f, 1.0f);
                float ease = Easing::OutCubic(t);
                float alpha = ease;
                float slideUp = (1.0f - ease) * 8.0f;

                float y = statStartY + i * rowH + slideUp;



                // ラベル（左揃え）
                DirectX::XMVECTORF32 labelColor = { 0.6f, 0.6f, 0.6f, alpha };
                m_font->DrawString(m_spriteBatch.get(), labels[i],
                    DirectX::XMFLOAT2(tableL, y), labelColor);

                // ドットリーダー
                DirectX::XMVECTOR labelSize = m_font->MeasureString(labels[i]);
                float labelEnd = tableL + DirectX::XMVectorGetX(labelSize);
                float dotS = labelEnd + 6.0f;
                float dotE = colVal - 70.0f;
                if (dotE > dotS && m_whitePixel)
                {
                    for (float dx = dotS; dx < dotE; dx += 6.0f)
                    {
                        RECT dot = { (LONG)dx, (LONG)(y + 8), (LONG)(dx + 2), (LONG)(y + 10) };
                        DirectX::XMVECTORF32 dotColor = { 0.35f, 0.35f, 0.35f, alpha * 0.5f };
                        m_spriteBatch->Draw(m_whitePixel.Get(), dot, dotColor);
                    }
                }

                // 値（右揃え）
                wchar_t valBuf[64];
                if (i == 5)
                {
                    int sec = (int)(m_goSurvivalTime * m_goStatCountUp[i]);
                    swprintf_s(valBuf, L"%d:%02d", sec / 60, sec % 60);
                }
                else if (i == 8)
                {
                    int dr = (int)(m_goMaxStyleRank * m_goStatCountUp[i]);
                    if (dr > 6) dr = 6;
                    swprintf_s(valBuf, L"%s", styleRankNames[dr]);
                }
                else
                {
                    swprintf_s(valBuf, L"%d", (int)(rawValues[i] * m_goStatCountUp[i]));
                }

                DirectX::XMVECTORF32 valColor = { 1.0f, 1.0f, 1.0f, alpha };
                DirectX::XMVECTOR valSize = m_font->MeasureString(valBuf);
                float valW = DirectX::XMVectorGetX(valSize);
                m_font->DrawString(m_spriteBatch.get(), valBuf,
                    DirectX::XMFLOAT2(colVal - valW, y), valColor);

                // ボーナスポイント（右揃え、金色）
                int dispScore = (int)(m_goStatScores[i] * m_goStatCountUp[i]);
                wchar_t sBuf[64];
                swprintf_s(sBuf, L"+%d", dispScore);
                DirectX::XMVECTORF32 bonusColor = { 1.0f, 0.85f, 0.2f, alpha };
                DirectX::XMVECTOR sSize = m_font->MeasureString(sBuf);
                float sW = DirectX::XMVectorGetX(sSize);
                m_font->DrawString(m_spriteBatch.get(), sBuf,
                    DirectX::XMFLOAT2(colBonus - sW, y), bonusColor);
            }

            // --- 左パネル下部: WAVE表示 ---
            float waveStart = 1.2f + 9 * 0.12f + 0.2f;
            if (timer > waveStart)
            {
                float wt = min((timer - waveStart) / 0.4f, 1.0f);
                float wa = Easing::OutCubic(wt);

                float waveY = statStartY + 9 * rowH + 20.0f;
                wchar_t waveBuf[64];
                swprintf_s(waveBuf, L"WAVE  %d", m_gameOverWave);
                DirectX::XMVECTORF32 waveCol = { 0.8f, 0.3f, 0.1f, wa };
                m_font->DrawString(m_spriteBatch.get(), waveBuf,
                    DirectX::XMFLOAT2(tableL, waveY), waveCol);
            }
        }

        // =====================================
        //  右パネル: ランク + トータルスコア
        // =====================================

        // --- 「RANK」ラベル ---
        if (timer > 2.0f)
        {
            float rlT = min((timer - 2.0f) / 0.3f, 1.0f);
            float rlA = Easing::OutCubic(rlT);
            DirectX::XMVECTORF32 rlCol = { 0.5f, 0.5f, 0.5f, rlA * 0.7f };
            DirectX::XMVECTOR rlSize = m_font->MeasureString(L"RANK");
            float rlW = DirectX::XMVectorGetX(rlSize);
            m_font->DrawString(m_spriteBatch.get(), L"RANK",
                DirectX::XMFLOAT2(rightCX - rlW * 0.5f, panelTop + 25.0f), rlCol);
        }

        // // ランク登場フラッシュ（文字の形に沿って放射状に光る）
        if (timer > 2.15f && timer < 4.0f && m_fontLarge)
        {
            float flashT = (timer - 2.15f) / 1.85f;  // 0→1（1.85秒かけてゆっくり減衰）
            float flashA = (1.0f - flashT) * (1.0f - flashT);
            float intensity = (m_gameOverRank >= 2) ? 1.0f : 0.5f;

            const wchar_t* rankNames[] = { L"C", L"B", L"A", L"S" };
            const wchar_t* rn = rankNames[m_gameOverRank];
            float rankY = panelTop + 70.0f;

            DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rn);
            float rW = DirectX::XMVectorGetX(rSize);
            float rH = DirectX::XMVectorGetY(rSize);
            float rX = rightCX - rW * 0.5f;

            // フラッシュ色
            DirectX::XMFLOAT3 flashRGB;
            if (m_gameOverRank == 3)      flashRGB = { 1.0f, 0.4f, 0.1f };
            else if (m_gameOverRank == 2) flashRGB = { 1.0f, 0.9f, 0.3f };
            else if (m_gameOverRank == 1) flashRGB = { 0.5f, 0.7f, 1.0f };
            else                          flashRGB = { 0.8f, 0.8f, 0.8f };

            // === 外側レイヤー: 16方向に大きく放射 ===
            float outerDist = 12.0f + (1.0f - flashT) * 60.0f;
            float outerDirs[][2] = {
                {1,0},{-1,0},{0,1},{0,-1},
                {0.707f,0.707f},{-0.707f,0.707f},{0.707f,-0.707f},{-0.707f,-0.707f},
                {0.92f,0.38f},{-0.92f,0.38f},{0.92f,-0.38f},{-0.92f,-0.38f},
                {0.38f,0.92f},{-0.38f,0.92f},{0.38f,-0.92f},{-0.38f,-0.92f}
            };
            for (int d = 0; d < 16; d++)
            {
                float dx = outerDirs[d][0] * outerDist;
                float dy = outerDirs[d][1] * outerDist;
                DirectX::XMVECTORF32 gc = { flashRGB.x, flashRGB.y, flashRGB.z, flashA * intensity * 0.2f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === 中間レイヤー: 8方向 ===
            float midDist = 5.0f + (1.0f - flashT) * 30.0f;
            for (int d = 0; d < 8; d++)
            {
                float dx = outerDirs[d][0] * midDist;
                float dy = outerDirs[d][1] * midDist;
                DirectX::XMVECTORF32 gc = { flashRGB.x, flashRGB.y, flashRGB.z, flashA * intensity * 0.35f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === 内側レイヤー: 8方向（明るい）===
            float innerDist = 2.0f + (1.0f - flashT) * 12.0f;
            for (int d = 0; d < 8; d++)
            {
                float dx = outerDirs[d][0] * innerDist;
                float dy = outerDirs[d][1] * innerDist;
                DirectX::XMVECTORF32 gc = {
                    min(flashRGB.x + 0.3f, 1.0f),
                    min(flashRGB.y + 0.3f, 1.0f),
                    min(flashRGB.z + 0.2f, 1.0f),
                    flashA * intensity * 0.5f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === S/Aランク: 背景にぼんやり光の円 ===
            if (m_gameOverRank >= 2 && m_whitePixel)
            {
                float glowRadius = 80.0f + (1.0f - flashT) * 120.0f;
                float cx = rightCX;
                float cy = rankY + rH * 0.5f;
                // 同心円的に3層の光
                for (int ring = 0; ring < 3; ring++)
                {
                    float r = glowRadius * (0.4f + ring * 0.3f);
                    float ringA = flashA * intensity * 0.08f / (ring + 1);
                    RECT glowRect = {
                        (LONG)(cx - r), (LONG)(cy - r),
                        (LONG)(cx + r), (LONG)(cy + r)
                    };
                    DirectX::XMVECTORF32 glowCol = { flashRGB.x, flashRGB.y, flashRGB.z, ringA };
                    m_spriteBatch->Draw(m_whitePixel.Get(), glowRect, glowCol);
                }
            }
        }


        // --- ランク文字（大きく中央に）---
        if (timer > 2.2f)
        {
            float rankT = min((timer - 2.2f) / 0.5f, 1.0f);
            float rankEase = Easing::OutBack(rankT);
            float rankAlpha = min((timer - 2.2f) * 3.0f, 1.0f);

            const wchar_t* rankNames[] = { L"C", L"B", L"A", L"S" };
            const wchar_t* rankName = rankNames[m_gameOverRank];

            DirectX::XMVECTORF32 rankColors[] = {
                { 0.5f, 0.5f, 0.5f, rankAlpha },
                { 0.3f, 0.5f, 1.0f, rankAlpha },
                { 1.0f, 0.85f, 0.0f, rankAlpha },
                { 1.0f, 0.2f, 0.0f, rankAlpha },
            };
            DirectX::XMVECTORF32 rankColor = rankColors[m_gameOverRank];

            float rankY = panelTop + 70.0f;

            if (m_fontLarge)
            {
                // グロー（A/Sランク）
                if (m_gameOverRank >= 2)
                {
                    float glowPulse = (sinf(timer * 4.0f) + 1.0f) * 0.5f;
                    DirectX::XMVECTORF32 glowColor = { rankColor.f[0], rankColor.f[1], rankColor.f[2],
                        rankAlpha * 0.2f * (0.5f + glowPulse * 0.5f) };
                    float glow = 5.0f;
                    DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rankName);
                    float rW = DirectX::XMVectorGetX(rSize);
                    float rH = DirectX::XMVectorGetY(rSize);
                    float rX = rightCX - rW * 0.5f;
                    DirectX::XMFLOAT2 glowOrigin(rW * 0.5f, rH * 0.5f);
                    DirectX::XMFLOAT2 glowCenter(rightCX, rankY + rH * 0.5f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x - glow, glowCenter.y), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x + glow, glowCenter.y), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x, glowCenter.y - glow), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x, glowCenter.y + glow), glowColor, 0.0f, glowOrigin, 2.0f);
                }

                DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rankName);
                float rW = DirectX::XMVectorGetX(rSize);
                float rH = DirectX::XMVectorGetY(rSize);
                // 2倍スケールで描画（originを中央にして拡大）
                DirectX::XMFLOAT2 origin(rW * 0.5f, rH * 0.5f);
                DirectX::XMFLOAT2 pos(rightCX, rankY + rH * 0.5f);
                m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                    pos, rankColor, 0.0f, origin, 2.0f);
            }
            else
            {
                DirectX::XMVECTOR rSize = m_font->MeasureString(rankName);
                float rW = DirectX::XMVectorGetX(rSize);
                m_font->DrawString(m_spriteBatch.get(), rankName,
                    DirectX::XMFLOAT2(rightCX - rW * 0.5f, rankY), rankColor);
            }
        }

        // --- 右パネル: 区切り線 ---
        if (timer > 2.5f)
        {
            float sepT = min((timer - 2.5f) / 0.4f, 1.0f);
            float sepE = Easing::OutCubic(sepT);
            float sepY = panelTop + 220.0f;
            float sepHW = 200.0f * sepE;
            RECT sepLine = {
                (LONG)(rightCX - sepHW), (LONG)sepY,
                (LONG)(rightCX + sepHW), (LONG)(sepY + 1)
            };
            DirectX::XMVECTORF32 sepCol = { 0.5f, 0.12f, 0.0f, sepE * 0.6f };
            m_spriteBatch->Draw(m_whitePixel.Get(), sepLine, sepCol);
        }

        // --- 右パネル: TOTAL SCORE ---
        if (timer > 2.7f)
        {
            float tsT = min((timer - 2.7f) / 0.4f, 1.0f);
            float tsA = Easing::OutCubic(tsT);

            float scoreY = panelTop + 240.0f;
            DirectX::XMVECTORF32 tsLabelCol = { 0.6f, 0.6f, 0.6f, tsA * 0.8f };
            DirectX::XMVECTOR tsLabelSize = m_font->MeasureString(L"TOTAL SCORE");
            float tsLabelW = DirectX::XMVectorGetX(tsLabelSize);
            m_font->DrawString(m_spriteBatch.get(), L"TOTAL SCORE",
                DirectX::XMFLOAT2(rightCX - tsLabelW * 0.5f, scoreY), tsLabelCol);

            int dispTotal = (int)(m_goTotalScore * m_gameOverCountUp);
            wchar_t tBuf[64];
            swprintf_s(tBuf, L"%d", dispTotal);
            DirectX::XMVECTORF32 tCol = { 1.0f, 0.85f, 0.0f, tsA };
            float numY = scoreY + 25.0f;

            if (m_fontLarge)
            {
                DirectX::XMVECTOR ts = m_fontLarge->MeasureString(tBuf);
                float tw = DirectX::XMVectorGetX(ts);
                m_fontLarge->DrawString(m_spriteBatch.get(), tBuf,
                    DirectX::XMFLOAT2(rightCX - tw * 0.5f, numY), tCol);
            }
            else
            {
                DirectX::XMVECTOR ts = m_font->MeasureString(tBuf);
                float tw = DirectX::XMVectorGetX(ts);
                m_font->DrawString(m_spriteBatch.get(), tBuf,
                    DirectX::XMFLOAT2(rightCX - tw * 0.5f, numY), tCol);
            }
        }

        // --- パーティクル（A/Sランクで右パネル周辺に火の粉）---
        if (timer > 3.5f && m_gameOverRank >= 2 && m_whitePixel)
        {
            float particleAlpha = min((timer - 3.5f) * 2.0f, 1.0f);
            int numParticles = (m_gameOverRank == 3) ? 30 : 15;

            for (int i = 0; i < numParticles; i++)
            {
                float seed1 = sinf((float)i * 127.1f + timer * 0.7f) * 0.5f + 0.5f;
                float seed2 = cosf((float)i * 311.7f + timer * 0.5f) * 0.5f + 0.5f;
                float seed3 = sinf((float)i * 269.5f + timer * 1.3f) * 0.5f + 0.5f;

                float px = rightCX + (seed1 - 0.5f) * 500.0f;
                float py = panelTop + seed2 * (panelBot - panelTop);
                float pSize = 2.0f + seed3 * 4.0f;

                float flickerAlpha = particleAlpha * (0.3f + seed3 * 0.7f);
                DirectX::XMVECTORF32 pColor = {
                    0.9f + seed1 * 0.1f,
                    0.3f + seed2 * 0.4f,
                    0.0f,
                    flickerAlpha
                };

                RECT pRect = {
                    (LONG)(px - pSize * 0.5f), (LONG)(py - pSize * 0.5f),
                    (LONG)(px + pSize * 0.5f), (LONG)(py + pSize * 0.5f)
                };
                m_spriteBatch->Draw(m_whitePixel.Get(), pRect, pColor);
            }
        }

        // --- Press R to Restart（右パネル下）---
        if (timer > 4.0f && m_rankingSaved)
        {
            float restartT = min((timer - 4.0f) / 0.3f, 1.0f);
            float restartAlpha = Easing::OutCubic(restartT);
            float blink = (sinf(timer * 3.0f) + 1.0f) * 0.5f;
            float finalAlpha = restartAlpha * (0.4f + blink * 0.6f);

            DirectX::XMVECTORF32 restartColor = { 0.6f, 0.6f, 0.6f, finalAlpha };
            DirectX::XMVECTOR restartSize = m_font->MeasureString(L"Press R to Restart");
            float restartW = DirectX::XMVectorGetX(restartSize);
            m_font->DrawString(m_spriteBatch.get(), L"Press R to Restart",
                DirectX::XMFLOAT2(rightCX - restartW * 0.5f, panelBot + 15.0f), restartColor);
        }

        // --- L: RANKING 案内テキスト ---
        if (timer > 4.0f && m_rankingSaved)
        {
            float lT = min((timer - 4.0f) / 0.3f, 1.0f);
            float lAlpha = Easing::OutCubic(lT);
            float lBlink = (sinf(timer * 2.5f) + 1.0f) * 0.5f;
            float lFinal = lAlpha * (0.3f + lBlink * 0.5f);

            DirectX::XMVECTORF32 lColor = { 0.8f, 0.6f, 0.2f, lFinal };
            DirectX::XMVECTOR lSize = m_font->MeasureString(L"L: RANKING");
            float lW = DirectX::XMVectorGetX(lSize);
            m_font->DrawString(m_spriteBatch.get(), L"L: RANKING",
                DirectX::XMFLOAT2(rightCX - lW * 0.5f, panelBot + 40.0f), lColor);
        }

        // =============================================
        // 名前入力 UI
        // =============================================
        if (m_nameInputActive)
        {
            float inputY = panelBot + 75.0f;
            float inputCenterX = rightCX;

            // --- 入力ボックス背景 ---
            RECT inputBg = {
                (LONG)(inputCenterX - 140), (LONG)(inputY - 5),
                (LONG)(inputCenterX + 140), (LONG)(inputY + 35)
            };
            DirectX::XMVECTORF32 bgColor = { 0.0f, 0.0f, 0.0f, 0.7f };
            m_spriteBatch->Draw(m_whitePixel.Get(), inputBg, bgColor);

            // --- 入力ボックス枠 ---
            float borderPulse = sinf(timer * 3.0f) * 0.3f + 0.7f;
            DirectX::XMVECTORF32 borderColor = { 0.9f, 0.6f, 0.1f, borderPulse };

            // 上枠
            RECT borderTop = { inputBg.left, inputBg.top, inputBg.right, inputBg.top + 2 };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderTop, borderColor);
            // 下枠
            RECT borderBot = { inputBg.left, inputBg.bottom - 2, inputBg.right, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderBot, borderColor);
            // 左枠
            RECT borderL = { inputBg.left, inputBg.top, inputBg.left + 2, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderL, borderColor);
            // 右枠
            RECT borderR = { inputBg.right - 2, inputBg.top, inputBg.right, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderR, borderColor);

            // --- "ENTER YOUR NAME" ラベル ---
            DirectX::XMVECTORF32 labelColor = { 1.0f, 0.8f, 0.3f, 1.0f };
            DirectX::XMVECTOR labelSize = m_font->MeasureString(L"ENTER YOUR NAME");
            float labelW = DirectX::XMVectorGetX(labelSize);
            m_font->DrawString(m_spriteBatch.get(), L"ENTER YOUR NAME",
                DirectX::XMFLOAT2(inputCenterX - labelW * 0.5f, inputY - 28.0f), labelColor);

            // --- 入力テキスト表示 ---
            wchar_t wideNameBuf[32] = {};
            for (int c = 0; c < m_nameLength && c < 15; c++)
                wideNameBuf[c] = (wchar_t)m_nameBuffer[c];
            wideNameBuf[m_nameLength] = L'\0';

            // 点滅カーソルを//
            float cursorBlink = sinf(timer * 6.0f);
            if (cursorBlink > 0.0f && m_nameLength < 15)
            {
                wideNameBuf[m_nameLength] = L'_';
                wideNameBuf[m_nameLength + 1] = L'\0';
            }

            DirectX::XMVECTORF32 nameColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            DirectX::XMVECTOR nameSize = m_font->MeasureString(wideNameBuf);
            float nameW = DirectX::XMVectorGetX(nameSize);
            m_font->DrawString(m_spriteBatch.get(), wideNameBuf,
                DirectX::XMFLOAT2(inputCenterX - nameW * 0.5f, inputY + 3.0f), nameColor);

            // --- "PRESS ENTER TO CONFIRM" ---
            DirectX::XMVECTORF32 hintColor = { 0.6f, 0.6f, 0.5f, 0.6f + sinf(timer * 2.0f) * 0.2f };
            DirectX::XMVECTOR hintSize = m_font->MeasureString(L"ENTER: OK    BACKSPACE: DELETE");
            float hintW = DirectX::XMVectorGetX(hintSize);
            m_font->DrawString(m_spriteBatch.get(), L"ENTER: OK    BACKSPACE: DELETE",
                DirectX::XMFLOAT2(inputCenterX - hintW * 0.5f, inputY + 42.0f), hintColor);
        }

        m_spriteBatch->End();
    }
}

// ============================================================
//  RenderFade - 黒フェードオーバーレイの描画
//
//  【仕組み】画面全体を覆う黒い四角形をm_fadeAlphaで透過。
//  PrimitiveBatchで三角形2枚 = 四角形1枚。
//  描画後にVertexColorEnabledをtrueに戻すのがポイント。
// ============================================================
void Game::RenderFade()
{
    if (m_fadeAlpha <= 0.0f)
        return;

    // フェード用の2D描画設定
    auto context = m_d3dContext.Get();

    // 2D用の単位行列
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());

    // 頂点カラーを無効化して単色描画モードに
    m_effect->SetVertexColorEnabled(false);
    DirectX::XMVECTOR diffuseColor = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    m_effect->SetDiffuseColor(diffuseColor);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // フルスクリーンの四角形を描画
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    DirectX::XMFLOAT4 fadeColor(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面全体を覆う四角形
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, -halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(halfWidth, -halfHeight, 1.0f), fadeColor);

    // 三角形2つで四角形を構成
    primitiveBatch->DrawTriangle(v1, v2, v3);
    primitiveBatch->DrawTriangle(v1, v3, v4);

    primitiveBatch->End();

    // 頂点カラーを再度有効化
    m_effect->SetVertexColorEnabled(true);
}

// ============================================================
//  RenderRanking - ランキング画面の描画
//
//  【描画構成】
//  Layer 0: 赤黒ビネット背景
//  Layer 1: SpriteBatch UI
//    - 装飾線（上下） + "LEADERBOARD"タイトル（バウンス+グロー）
//    - ヘッダー行（#, NAME, SCORE, WAVE, KILLS）
//    - エントリー最大10行（スライドイン + カウントアップ）
//    - NEW!マーク（新記録エントリーに金色パルス）
//    - フッター操作案内
//
//  【改善】イージング関数をEasing名前空間に統一、
//  DrawTextWithShadowラムダは残存（この関数内でのみ使用）
// ============================================================
void Game::RenderRanking()
{
    // =============================================
    // イージング関数
    // =============================================




    // テキストに影を描くヘルパー
    // 本体の2px右下に黒い影を描いてから本体を描く
    auto DrawTextWithShadow = [&](DirectX::SpriteFont* font,
        const wchar_t* text, DirectX::XMFLOAT2 pos,
        DirectX::XMVECTORF32 color)
        {
            // 影（黒・半透明）
            DirectX::XMVECTORF32 shadowColor = { 0.0f, 0.0f, 0.0f, color.f[3] * 0.8f };
            font->DrawString(m_spriteBatch.get(), text,
                DirectX::XMFLOAT2(pos.x + 2.0f, pos.y + 2.0f), shadowColor);
            // 本体
            font->DrawString(m_spriteBatch.get(), text, pos, color);
        };

    float timer = m_rankingTimer;
    float screenW = (float)m_outputWidth;
    float screenH = (float)m_outputHeight;
    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    auto context = m_d3dContext.Get();

    // =============================================
    // Layer 0: 背景（より濃いビネット）
    // =============================================
    {
        float overlayT = (std::min)(timer / 0.8f, 1.0f);
        float overlayAlpha = Easing::OutQuad(overlayT) * 0.95f;  // // 0.92→0.95 より暗く

        DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
            screenW, screenH, 0.1f, 10.0f);

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(DirectX::XMMatrixIdentity());
        m_effect->SetVertexColorEnabled(true);
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());

        auto primBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
        primBatch->Begin();

        float hw = screenW * 0.5f;
        float hh = screenH * 0.5f;

        // // 中央もかなり暗くして文字との対比を強くする
        DirectX::XMFLOAT4 centerColor(0.06f, 0.0f, 0.02f, overlayAlpha * 0.85f);
        DirectX::XMFLOAT4 edgeColor(0.01f, 0.0f, 0.0f, overlayAlpha);

        DirectX::VertexPositionColor vc(DirectX::XMFLOAT3(0, 0, 1.0f), centerColor);
        DirectX::VertexPositionColor vTL(DirectX::XMFLOAT3(-hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vTR(DirectX::XMFLOAT3(hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBR(DirectX::XMFLOAT3(hw, -hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBL(DirectX::XMFLOAT3(-hw, -hh, 1.0f), edgeColor);

        primBatch->DrawTriangle(vc, vTL, vTR);
        primBatch->DrawTriangle(vc, vTR, vBR);
        primBatch->DrawTriangle(vc, vBR, vBL);
        primBatch->DrawTriangle(vc, vBL, vTL);

        primBatch->End();
    }

    // =============================================
    // Layer 1: SpriteBatchで全UI描画
    // =============================================
    if (!m_spriteBatch || !m_states || !m_font || !m_whitePixel)
        return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->AlphaBlend(),
        nullptr,
        m_states->DepthNone()
    );

    const auto& entries = m_rankingSystem->GetEntries();
    int entryCount = (int)entries.size();

    // =============================================
    // 装飾線（上）
    // =============================================
    if (timer > 0.2f)
    {
        float lineT = (std::min)((timer - 0.2f) / 0.5f, 1.0f);
        float lineProgress = Easing::OutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;  // // 280→320 少し長く
        float lineY = 55.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 2)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 2)
        };

        // // 色をより明るい赤金に
        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.95f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

        RECT diamond = {
            (LONG)(centerX - 5), (LONG)(lineY - 4),
            (LONG)(centerX + 5), (LONG)(lineY + 6)
        };
        m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
    }

    // =============================================
    // タイトル "LEADERBOARD" ? バウンスで落ちてくる
    // =============================================
    if (timer > 0.3f && m_fontLarge)
    {
        float titleT = (std::min)((timer - 0.3f) / 0.6f, 1.0f);
        float bounceT = Easing::OutBounce(titleT);
        float titleAlpha = (std::min)((timer - 0.3f) * 4.0f, 1.0f);

        float slideY = (1.0f - bounceT) * -60.0f;
        float titleY = 65.0f + slideY;

        DirectX::XMVECTOR titleSize = m_fontLarge->MeasureString(L"LEADERBOARD");
        float titleW = DirectX::XMVectorGetX(titleSize);
        float titleX = centerX - titleW * 0.5f;

        // // グロー色をより明るく
        float glowAlpha = titleAlpha * 0.3f;
        DirectX::XMVECTORF32 glowColor = { 1.0f, 0.3f, 0.0f, glowAlpha };
        for (int dx = -2; dx <= 2; dx += 4)
        {
            for (int dy = -2; dy <= 2; dy += 4)
            {
                m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
                    DirectX::XMFLOAT2(titleX + dx, titleY + dy), glowColor);
            }
        }

        // // 影を//
        DirectX::XMVECTORF32 shadowColor = { 0.0f, 0.0f, 0.0f, titleAlpha * 0.7f };
        m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
            DirectX::XMFLOAT2(titleX + 3, titleY + 3), shadowColor);

        // // 本体色を明るいゴールドに
        DirectX::XMVECTORF32 titleColor = { 1.0f, 0.9f, 0.75f, titleAlpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
            DirectX::XMFLOAT2(titleX, titleY), titleColor);
    }

    // =============================================
    // 装飾線（タイトルの下）
    // =============================================
    if (timer > 0.4f)
    {
        float lineT = (std::min)((timer - 0.4f) / 0.5f, 1.0f);
        float lineProgress = Easing::OutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;
        float lineY = 115.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 1)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 1)
        };

        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.8f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);
    }

    // =============================================
    // ヘッダー行: #  NAME  SCORE  WAVE  KILLS
    // =============================================
    if (timer > 0.6f)
    {
        float headerAlpha = (std::min)((timer - 0.6f) * 3.0f, 1.0f);
        // // ヘッダー色をより明るい金色に（白だと内容と紛らわしい）
        DirectX::XMVECTORF32 headerColor = { 1.0f, 0.75f, 0.35f, headerAlpha * 0.9f };

        float headerY = 130.0f;

        // // 列の配置を変更（NAME列を//）
        float colNum = centerX - 310.0f;   // #
        float colName = centerX - 250.0f;   // NAME  //新規
        float colScore = centerX - 60.0f;    // SCORE
        float colWave = centerX + 100.0f;   // WAVE
        float colKills = centerX + 210.0f;   // KILLS

        DrawTextWithShadow(m_font.get(), L"#",
            DirectX::XMFLOAT2(colNum, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"NAME",
            DirectX::XMFLOAT2(colName, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"SCORE",
            DirectX::XMFLOAT2(colScore, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"WAVE",
            DirectX::XMFLOAT2(colWave, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"KILLS",
            DirectX::XMFLOAT2(colKills, headerY), headerColor);
    }

    // =============================================
    // 各エントリー
    // =============================================
    float entryStartTime = 0.8f;
    float entryDelay = 0.12f;
    float entryBaseY = 160.0f;
    float entryHeight = 42.0f;

    // 列のX座標（ヘッダーと同じ）
    float colNum = centerX - 310.0f;
    float colName = centerX - 250.0f;
    float colScore = centerX - 60.0f;
    float colWave = centerX + 100.0f;
    float colKills = centerX + 210.0f;

    for (int i = 0; i < entryCount && i < 10; i++)
    {
        float startT = entryStartTime + i * entryDelay;

        if (timer < startT)
            continue;

        // --- スライドインアニメーション ---
        float slideT = (std::min)((timer - startT) / 0.5f, 1.0f);
        float slideProgress = Easing::OutCubic(slideT);

        float slideX = (1.0f - slideProgress) * -300.0f;
        float alpha = slideProgress;

        float rowY = entryBaseY + i * entryHeight;

        bool isNewRecord = (i == m_newRecordRank);

        // --- 背景バー ---
        {
            float barAlpha = alpha * 0.25f;  // // 0.15→0.25 バーをより目立たせる

            if (isNewRecord)
            {
                float pulse = sinf(timer * 4.0f) * 0.5f + 0.5f;
                barAlpha = alpha * (0.3f + pulse * 0.2f);
            }

            if (i == 0)
                barAlpha = alpha * 0.3f;

            RECT bar = {
                (LONG)(centerX - 330.0f + slideX), (LONG)(rowY - 2),
                (LONG)(centerX + 330.0f + slideX), (LONG)(rowY + entryHeight - 8)
            };

            DirectX::XMVECTORF32 barColor;
            if (isNewRecord)
                barColor = { 1.0f, 0.8f, 0.1f, barAlpha };
            else if (i == 0)
                barColor = { 1.0f, 0.75f, 0.2f, barAlpha };
            else if (i == 1)
                barColor = { 0.75f, 0.75f, 0.85f, barAlpha };
            else if (i == 2)
                barColor = { 0.75f, 0.5f, 0.2f, barAlpha };
            else
                barColor = { 0.4f, 0.2f, 0.15f, barAlpha };

            m_spriteBatch->Draw(m_whitePixel.Get(), bar, barColor);
        }

        // --- 順位の色（ 全体的に明るく） ---
        DirectX::XMVECTORF32 rankNumColor;
        if (i == 0)
            rankNumColor = { 1.0f, 0.9f, 0.3f, alpha };      // 1位: 明るい金
        else if (i == 1)
            rankNumColor = { 0.9f, 0.9f, 1.0f, alpha };       // 2位: 明るい銀
        else if (i == 2)
            rankNumColor = { 1.0f, 0.65f, 0.3f, alpha };      // 3位: 明るい銅
        else
            rankNumColor = { 0.85f, 0.8f, 0.7f, alpha };      //  他: 明るいベージュ

        if (isNewRecord)
        {
            float pulse = sinf(timer * 5.0f) * 0.5f + 0.5f;
            rankNumColor = { 1.0f, 0.8f + pulse * 0.2f, 0.2f + pulse * 0.3f, alpha };
        }

        // --- テキスト色（ 明るく） ---
        DirectX::XMVECTORF32 textColor;
        if (isNewRecord)
        {
            float pulse = sinf(timer * 5.0f) * 0.5f + 0.5f;
            textColor = { 1.0f, 0.9f + pulse * 0.1f, 0.6f + pulse * 0.3f, alpha };
        }
        else
        {
            textColor = { 1.0f, 0.95f, 0.85f, alpha };  // // 明るいクリーム色
        }

        const RankingEntry& e = entries[i];

        // --- 順位番号 ---
        wchar_t rankBuf[16];
        swprintf_s(rankBuf, L"%d.", i + 1);
        DrawTextWithShadow(m_font.get(), rankBuf,
            DirectX::XMFLOAT2(colNum + slideX, rowY), rankNumColor);

        // ---  名前（新規列） ---
        wchar_t nameBuf[32] = {};
        if (e.name[0] != '\0')
        {
            // char[16] → wchar_t に変換
            for (int c = 0; c < 15 && e.name[c] != '\0'; c++)
                nameBuf[c] = (wchar_t)e.name[c];
        }
        else
        {
            wcscpy_s(nameBuf, L"---");  // 名前なし（旧データ互換）
        }

        // 名前は順位と同じ色系統（1位金, 2位銀...）
        DrawTextWithShadow(m_font.get(), nameBuf,
            DirectX::XMFLOAT2(colName + slideX, rowY), rankNumColor);

        // --- スコア（カウントアップ演出） ---
        float countUpT = (std::min)((timer - startT) / 1.0f, 1.0f);
        float easedCount = Easing::OutQuad(countUpT);
        int displayScore = (int)(e.score * easedCount);

        wchar_t scoreBuf[32];
        swprintf_s(scoreBuf, L"%d", displayScore);
        DrawTextWithShadow(m_font.get(), scoreBuf,
            DirectX::XMFLOAT2(colScore + slideX, rowY), textColor);

        // --- ウェーブ ---
        wchar_t waveBuf[16];
        swprintf_s(waveBuf, L"W%d", e.wave);
        DrawTextWithShadow(m_font.get(), waveBuf,
            DirectX::XMFLOAT2(colWave + slideX, rowY), textColor);

        // --- キル数 ---
        wchar_t killBuf[16];
        swprintf_s(killBuf, L"%d", e.kills);
        DrawTextWithShadow(m_font.get(), killBuf,
            DirectX::XMFLOAT2(colKills + slideX, rowY), textColor);

        // --- 新記録マーク ---
        if (isNewRecord && slideT > 0.5f)
        {
            float newAlpha = (std::min)((slideT - 0.5f) * 4.0f, 1.0f);
            float pulse = sinf(timer * 6.0f) * 0.5f + 0.5f;

            DirectX::XMVECTORF32 newColor = {
                1.0f, 0.85f + pulse * 0.15f, 0.0f, newAlpha
            };

            DrawTextWithShadow(m_font.get(), L"NEW!",
                DirectX::XMFLOAT2(colKills + 70.0f + slideX, rowY), newColor);
        }
    }

    // =============================================
    // エントリー0件
    // =============================================
    if (entryCount == 0 && timer > 1.0f)
    {
        float emptyAlpha = (std::min)((timer - 1.0f) * 2.0f, 1.0f);
        DirectX::XMVECTORF32 emptyColor = { 0.8f, 0.7f, 0.5f, emptyAlpha };

        const wchar_t* emptyText = L"NO RECORDS YET";
        DirectX::XMVECTOR emptySize = m_font->MeasureString(emptyText);
        float emptyW = DirectX::XMVectorGetX(emptySize);

        DrawTextWithShadow(m_font.get(), emptyText,
            DirectX::XMFLOAT2(centerX - emptyW * 0.5f, centerY - 20.0f), emptyColor);
    }

    // =============================================
    // 装飾線（下部）
    // =============================================
    if (timer > 1.5f)
    {
        float lineT = (std::min)((timer - 1.5f) / 0.5f, 1.0f);
        float lineProgress = Easing::OutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;
        float lineY = entryBaseY + 10 * entryHeight + 10.0f;
        if (lineY > screenH - 80.0f) lineY = screenH - 80.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 1)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 1)
        };

        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.6f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

        RECT diamond = {
            (LONG)(centerX - 3), (LONG)(lineY - 2),
            (LONG)(centerX + 3), (LONG)(lineY + 3)
        };
        m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
    }

    // =============================================
    // フッター: 操作案内
    // =============================================
    if (timer > 2.0f)
    {
        float footerAlpha = (std::min)((timer - 2.0f) * 2.0f, 1.0f);
        float pulse = sinf(timer * 2.0f) * 0.15f + 0.85f;

        //  フッター色をもっと明るく
        DirectX::XMVECTORF32 footerColor = { 0.85f, 0.75f, 0.55f, footerAlpha * pulse };

        float footerY = screenH - 50.0f;

        const wchar_t* backText = L"ESC: BACK     R: RESTART";
        DirectX::XMVECTOR backSize = m_font->MeasureString(backText);
        float backW = DirectX::XMVectorGetX(backSize);

        DrawTextWithShadow(m_font.get(), backText,
            DirectX::XMFLOAT2(centerX - backW * 0.5f, footerY), footerColor);
    }

    m_spriteBatch->End();
}