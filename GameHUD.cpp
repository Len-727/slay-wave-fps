// ============================================================
//  GameHUD.cpp
//  HUD描画（スコア/HP/弾薬/シールド/ウェーブバナー/エフェクト）
//
//  【責務】
//  - ゲームプレイ中の2D UI要素の全描画
//  - ダメージフラッシュ/低体力ビネット等の画面エフェクト
//  - スタイルランクHUD（テクスチャ版）
//  - ウェーブバナー表示
//  - スコアHUD（血しぶき背景付き）
//  - リロード警告/爪ダメージ/グローリーキルフラッシュ
//
//  【設計意図】
//  「プレイ中に画面に表示される2D要素」を全て集約。
//  3D描画（GameRender.cpp）とは明確に分離。
//  SpriteBatch + PrimitiveBatch の2Dレンダリング系を担当。
//
//  【含まれるメソッド】
//  DrawUI                - メインHUD（HP/弾薬/シールド/スタイルランク）
//  RenderDamageFlash     - 被弾時の赤フラッシュ
//  RenderLowHealthVignette - 低HP時の赤黒ビネット
//  RenderSpeedLines      - ダッシュ時のスピードライン
//  RenderDashOverlay     - ダッシュ時の画面効果
//  RenderReloadWarning   - リロード警告 + 進捗バー
//  RenderWaveBanner      - ウェーブ開始バナー
//  RenderScoreHUD        - スコア表示（血しぶき背景）
//  RenderClawDamage      - 爪攻撃のスクラッチ痕
//  RenderGloryKillFlash  - グローリーキル時の白フラッシュ
// ============================================================

#include "Game.h"

using namespace DirectX;

// ============================================================
//  HUD共通定数
// ============================================================
namespace HUDConstants
{
    // 画面エフェクトの基本パラメータ
    constexpr float DAMAGE_FLASH_NEAR_PLANE = 0.1f;
    constexpr float DAMAGE_FLASH_FAR_PLANE = 10.0f;

    // スピードラインのパラメータ
    constexpr int   SPEED_LINE_COUNT = 40;
    constexpr float SPEED_LINE_MIN_LENGTH = 60.0f;
    constexpr float SPEED_LINE_MAX_LENGTH = 200.0f;

    // グローリーキルフラッシュ
    constexpr float GLORY_FLASH_DURATION = 0.3f;

    // ウェーブバナーのフェードタイミング
    constexpr float BANNER_FADE_IN_THRESHOLD = 0.85f;   // t > 0.85 で登場
    constexpr float BANNER_FADE_OUT_THRESHOLD = 0.2f;    // t < 0.2 で消滅
}


// ============================================================
//  DrawUI - メインHUD描画
//
//  【描画内容】
//  1. UISystem::DrawAll() でHP/弾薬/クロスヘア等を描画
//  2. スタイルランクHUD（テクスチャ版、ランク名+ゲージ）
//  3. シールドHP/状態バー
//  4. チャージエネルギーゲージ
//  5. 近接チャージゲージ（ストック表示）
//  6. パリィタイミングリング
// ============================================================
void Game::DrawUI()
{
    // --- UI描画のための共通設定 ---
    auto context = m_d3dContext.Get();
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f, (float)m_outputWidth, (float)m_outputHeight, 0.0f, 0.1f, 1.0f);

    m_effect->SetProjection(projection);
    m_effect->SetView(DirectX::XMMatrixIdentity());
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // UISystemに全ての描画を委譲
    m_uiSystem->DrawAll(primitiveBatch.get(),
        m_player.get(),
        m_weaponSystem.get(),
        m_waveManager.get());

    // === 壁武器の購入UI ===
    //if (m_nearbyWeaponSpawn != nullptr)
    //{
    //    // すでに持っている武器か？
    //    bool alreadyOwned = false;
    //    if (m_weaponSystem->GetPrimaryWeapon() == m_nearbyWeaponSpawn->weaponType ||
    //        (m_weaponSystem->HasSecondaryWeapon() &&
    //            m_weaponSystem->GetSecondaryWeapon() == m_nearbyWeaponSpawn->weaponType))
    //    {
    //        alreadyOwned = true;
    //    }

    //    m_uiSystem->DrawWeaponPrompt(
    //        primitiveBatch.get(),
    //        m_nearbyWeaponSpawn,
    //        m_player->GetPoints(),
    //        alreadyOwned
    //    );
    //}


    primitiveBatch->End();



    // =============================================
    // スタイルランク HUD表示（テクスチャ版）
    // =============================================
    if (m_styleRank && m_spriteBatch && m_states)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // --- ランクテクスチャ描画 ---
        int rankIndex = static_cast<int>(m_styleRank->GetRank());

        if (m_rankTexturesLoaded && rankIndex >= 0 && rankIndex < 7
            && m_rankTextures[rankIndex]
            && m_styleRank->GetStylePoints() > 0.0f)
        {
            float rankX = m_outputWidth - 160.0f;
            float rankY = 10.0f;

            RECT destRect;
            destRect.left = (LONG)rankX;
            destRect.top = (LONG)rankY;
            destRect.right = (LONG)(rankX + 128.0f);
            destRect.bottom = (LONG)(rankY + 128.0f);

            m_spriteBatch->Draw(
                m_rankTextures[rankIndex].Get(),
                destRect,
                DirectX::Colors::White
            );

            // --- コンボ数（ランク画像の下）---
            int combo = m_styleRank->GetComboCount();
            if (combo > 1 && m_comboTexturesLoaded)
            {
                // コンボ数を文字列に変換（例: 12 → "12"）
                char comboStr[16];
                sprintf_s(comboStr, "%d", combo);
                int numDigits = (int)strlen(comboStr);

                // 各桁の表示サイズ
                float digitW = 40.0f;
                float digitH = 40.0f;
                float startX = rankX;
                float startY = rankY + 130.0f;

                // 数字を1桁ずつテクスチャで描画
                for (int i = 0; i < numDigits; i++)
                {
                    int digit = comboStr[i] - '0';  // 文字→数値変換
                    if (digit >= 0 && digit <= 9 && m_comboDigitTex[digit])
                    {
                        RECT dRect;
                        dRect.left = (LONG)(startX + i * digitW);
                        dRect.top = (LONG)startY;
                        dRect.right = (LONG)(startX + i * digitW + digitW);
                        dRect.bottom = (LONG)(startY + digitH);

                        m_spriteBatch->Draw(
                            m_comboDigitTex[digit].Get(),
                            dRect,
                            DirectX::Colors::White
                        );
                    }
                }

                // "COMBO" ラベル（数字の右に表示）
                if (m_comboLabelTex)
                {
                    float labelX = startX + numDigits * digitW + 4.0f;
                    RECT lRect;
                    lRect.left = (LONG)labelX;
                    lRect.top = (LONG)(startY + 2.0f);
                    lRect.right = (LONG)(labelX + 80.0f);
                    lRect.bottom = (LONG)(startY + digitH - 2.0f);

                    m_spriteBatch->Draw(
                        m_comboLabelTex.Get(),
                        lRect,
                        DirectX::Colors::White
                    );
                }
            }
        }

        // ==============================================
        // HP & シールドバー テクスチャHUD描画
        // ==============================================
        if (m_shieldHudLoaded)
        {
            // --- バーの位置・サイズ（ピクセル座標）---
            float barW = 260.0f;    // バーの幅
            float barH = 6.0f;     // バーの高さ
            float barX = 30.0f;     // 左端
            float hpBarY = (float)m_outputHeight - 50.0f;   // HPバーのY位置
            float shieldBarY = hpBarY + barH + 14.0f;        // シールドバー（HPの下）

            // アイコンサイズ
            float iconSize = 18.0f;

            // ─────────────────────────
            // HPバー
            // ─────────────────────────
            {
                float hpPercent = (float)m_player->GetHealth() / (float)m_player->GetMaxHealth();
                hpPercent = max(0.0f, min(1.0f, hpPercent));

                // ゲージ中身（HP割合でsourceRectを制限）
                auto* fillTex = (hpPercent < 0.25f && m_hpHudFillCritical)
                    ? m_hpHudFillCritical.Get()
                    : m_hpHudFillGreen.Get();

                if (fillTex)
                {
                    // sourceRect: テクスチャの左端から割合分だけ切り取る
                    RECT srcRect = { 0, 0, (LONG)(512 * hpPercent), 16 };
                    // destRect: 画面上の表示位置（割合分の幅）
                    RECT destRect = {
                        (LONG)barX,
                        (LONG)hpBarY,
                        (LONG)(barX + barW * hpPercent),
                        (LONG)(hpBarY + barH)
                    };
                    m_spriteBatch->Draw(fillTex, destRect, &srcRect, DirectX::Colors::White);
                }

                // フレーム（常に全幅で表示）
                if (m_hpHudFrame)
                {
                    RECT frameRect = {
                        (LONG)barX, (LONG)hpBarY,
                        (LONG)(barX + barW), (LONG)(hpBarY + barH)
                    };
                    m_spriteBatch->Draw(m_hpHudFrame.Get(), frameRect, DirectX::Colors::White);
                }
            }

            // ─────────────────────────
            // シールドバー
            // ─────────────────────────
            {
                // 表示用HPを滑らかに追従
                float shieldPercent = m_shieldDisplayHP / m_shieldMaxHP;
                shieldPercent = max(0.0f, min(1.0f, shieldPercent));

                // グロウ（ガード/パリィ中のみ）
                if (m_shieldGlowIntensity > 0.01f && m_shieldHudGlow)
                {
                    float glowExpand = 6.0f;
                    RECT glowRect = {
                        (LONG)(barX - glowExpand),
                        (LONG)(shieldBarY - glowExpand),
                        (LONG)(barX + barW + glowExpand),
                        (LONG)(shieldBarY + barH + glowExpand)
                    };
                    // 透明度をグロウ強度で制御
                    float a = m_shieldGlowIntensity;
                    DirectX::XMVECTORF32 glowColor = { a, a, a, a };
                    m_spriteBatch->Draw(m_shieldHudGlow.Get(), glowRect, glowColor);
                }

                // ゲージ中身（状態で切替）
                ID3D11ShaderResourceView* shieldFillTex = nullptr;
                if (m_shieldState == ShieldState::Broken)
                {
                    shieldFillTex = nullptr;  // 壊れてる → 空
                }
                else if (shieldPercent < 0.25f)
                {
                    shieldFillTex = m_shieldHudFillDanger.Get();  // 危険 → 赤
                }
                else if (m_shieldState == ShieldState::Guarding || m_shieldState == ShieldState::Parrying)
                {
                    shieldFillTex = m_shieldHudFillGuard.Get();   // ガード中 → 水色
                }
                else
                {
                    shieldFillTex = m_shieldHudFillBlue.Get();    // 通常 → 青
                }

                if (shieldFillTex && shieldPercent > 0.0f)
                {
                    RECT srcRect = { 0, 0, (LONG)(512 * shieldPercent), 16 };
                    RECT destRect = {
                        (LONG)barX,
                        (LONG)shieldBarY,
                        (LONG)(barX + barW * shieldPercent),
                        (LONG)(shieldBarY + barH)
                    };
                    m_spriteBatch->Draw(shieldFillTex, destRect, &srcRect, DirectX::Colors::White);
                }

                // フレーム
                if (m_shieldHudFrame)
                {
                    RECT frameRect = {
                        (LONG)barX, (LONG)shieldBarY,
                        (LONG)(barX + barW), (LONG)(shieldBarY + barH)
                    };
                    m_spriteBatch->Draw(m_shieldHudFrame.Get(), frameRect, DirectX::Colors::White);
                }

                // ヒビ割れオーバーレイ（壊れた時のみ）
                if (m_shieldState == ShieldState::Broken && m_shieldHudCrack)
                {
                    RECT crackRect = {
                        (LONG)barX, (LONG)shieldBarY,
                        (LONG)(barX + barW), (LONG)(shieldBarY + barH)
                    };
                    // 点滅させる
                    float blink = (sinf(m_shieldBrokenTimer * 10.0f) + 1.0f) * 0.5f;
                    DirectX::XMVECTORF32 crackColor = { 1.0f, 1.0f, 1.0f, 0.5f + blink * 0.5f };
                    m_spriteBatch->Draw(m_shieldHudCrack.Get(), crackRect, crackColor);
                }

                // パリィ成功フラッシュ
                if (m_parryFlashTimer > 0.0f && m_shieldHudParryFlash)
                {
                    float flashAlpha = m_parryFlashTimer / 0.3f;  // 0.3秒かけてフェードアウト
                    RECT flashRect = {
                        (LONG)(barX - 8),
                        (LONG)(shieldBarY - 6),
                        (LONG)(barX + barW + 8),
                        (LONG)(shieldBarY + barH + 6)
                    };
                    DirectX::XMVECTORF32 flashColor = { flashAlpha, flashAlpha, flashAlpha, flashAlpha };
                    m_spriteBatch->Draw(m_shieldHudParryFlash.Get(), flashRect, flashColor);
                }

                // シールドアイコン（バー左横）
                if (m_shieldHudIcon)
                {
                    RECT iconRect = {
                        (LONG)(barX + barW + 6),
                        (LONG)(shieldBarY + (barH - iconSize) * 0.5f),
                        (LONG)(barX + barW + 6 + iconSize),
                        (LONG)(shieldBarY + (barH + iconSize) * 0.5f)
                    };
                    m_spriteBatch->Draw(m_shieldHudIcon.Get(), iconRect, DirectX::Colors::White);
                }
            }

            //  チャージエネルギーゲージ
            if (m_chargeEnergy > 0.0f && m_shieldHudFillGuard
                && (m_shieldState == ShieldState::Guarding || m_shieldState == ShieldState::Charging))
            {
                float centerX = m_outputWidth * 0.5f;
                float centerY = m_outputHeight * 0.5f;
                float eBarW = 60.0f;
                float eBarH = 3.0f;
                float eBarY = centerY + 25.0f;

                float filledW = eBarW * m_chargeEnergy;
                float left = centerX - eBarW * 0.5f;
                float right = centerX + eBarW * 0.5f;

                // 外枠（白、1px大きく）
                RECT frameRect = {
                    (LONG)(left - 1), (LONG)(eBarY - 1),
                    (LONG)(right + 1), (LONG)(eBarY + eBarH + 1)
                };
                DirectX::XMVECTORF32 frameColor = { 0.8f, 0.8f, 0.8f, 0.7f };
                m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), frameRect, frameColor);

                // 背景（暗い）
                RECT bgRect = {
                    (LONG)left, (LONG)eBarY,
                    (LONG)right, (LONG)(eBarY + eBarH)
                };
                DirectX::XMVECTORF32 bgColor = { 0.05f, 0.05f, 0.1f, 0.8f };
                m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), bgRect, bgColor);

                // エネルギー（明るいシアン、満タンで白く輝く）
                RECT fillRect = {
                    (LONG)left, (LONG)eBarY,
                    (LONG)(left + filledW), (LONG)(eBarY + eBarH)
                };
                if (m_chargeReady)
                {
                    float pulse = 0.8f + sinf(m_chargeEnergy * 20.0f) * 0.2f;
                    DirectX::XMVECTORF32 readyColor = { pulse, pulse, 1.0f, 1.0f };
                    m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), fillRect, readyColor);
                }
                else
                {
                    DirectX::XMVECTORF32 fillColor = { 0.3f, 0.7f, 1.0f, 0.9f };
                    m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), fillRect, fillColor);
                }
            }

            // =============================================
            // 近接チャージゲージ（シールドバー横 + 縦ストック）
            // =============================================
            if (m_whitePixel)
            {
                float meleeW = 28.0f;
                float meleeFullH = 44.0f;
                float meleeX = barX + barW + 12.0f;
                float meleeY = shieldBarY + barH - meleeFullH;

                // === 背景（真っ黒） ===
                RECT meleeBgRect = {
                    (LONG)meleeX, (LONG)meleeY,
                    (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH)
                };
                DirectX::XMVECTORF32 bgColor2 = { 0.06f, 0.06f, 0.06f, 0.85f };
                m_spriteBatch->Draw(m_whitePixel.Get(), meleeBgRect, bgColor2);

                // === ゲージ中身 ===
                float gaugePercent = 1.0f;
                if (m_meleeCharges < m_meleeMaxCharges && m_meleeRechargeTime > 0.0f)
                {
                    gaugePercent = m_meleeRechargeTimer / m_meleeRechargeTime;
                }

                if (m_meleeCharges > 0)
                {
                    RECT meleeFillRect = {
                        (LONG)(meleeX + 2), (LONG)meleeY,
                        (LONG)(meleeX + meleeW - 2), (LONG)(meleeY + meleeFullH)
                    };
                    float pulse = 0.8f + sinf(m_gameTime * 4.0f) * 0.2f;
                    DirectX::XMVECTORF32 fullColor = { pulse, 0.6f * pulse, 0.1f, 1.0f };
                    m_spriteBatch->Draw(m_whitePixel.Get(), meleeFillRect, fullColor);

                    // リチャージ中なら白っぽい進捗を重ねる
                    if (m_meleeCharges < m_meleeMaxCharges && m_meleeRechargeTime > 0.0f)
                    {
                        float progress = m_meleeRechargeTimer / m_meleeRechargeTime;
                        float fillH = meleeFullH * progress;
                        RECT progRect = {
                            (LONG)(meleeX + 2), (LONG)(meleeY + meleeFullH - fillH),
                            (LONG)(meleeX + meleeW - 2), (LONG)(meleeY + meleeFullH)
                        };
                        DirectX::XMVECTORF32 progColor = { 1.0f, 0.9f, 0.6f, 0.3f };
                        m_spriteBatch->Draw(m_whitePixel.Get(), progRect, progColor);
                    }
                }
                else
                {
                    if (gaugePercent > 0.0f)
                    {
                        float fillH = meleeFullH * gaugePercent;
                        RECT meleeFillRect = {
                            (LONG)(meleeX + 2), (LONG)(meleeY + meleeFullH - fillH),
                            (LONG)(meleeX + meleeW - 2), (LONG)(meleeY + meleeFullH)
                        };
                        DirectX::XMVECTORF32 fillColor2 = { 1.0f, 0.6f, 0.1f, 0.4f };
                        m_spriteBatch->Draw(m_whitePixel.Get(), meleeFillRect, fillColor2);
                    }
                }

                // === スキルアイコン ===
                if (m_meleeIconTexture)
                {
                    float mIconSize = 32.0f;
                    float iconX = meleeX + (meleeW - mIconSize) * 0.5f;
                    float iconY = meleeY + (meleeFullH - mIconSize) * 0.5f;
                    RECT iconRect = {
                        (LONG)iconX, (LONG)iconY,
                        (LONG)(iconX + mIconSize), (LONG)(iconY + mIconSize)
                    };
                    float iconAlpha = (m_meleeCharges > 0) ? 1.0f : 0.3f;
                    DirectX::XMVECTORF32 iconColor = { iconAlpha, iconAlpha, iconAlpha, iconAlpha };
                    m_spriteBatch->Draw(m_meleeIconTexture.Get(), iconRect, iconColor);
                }

                // === 外枠（白ピクセルで薄いグレー枠） ===
                {
                    DirectX::XMVECTORF32 frameColor = { 0.4f, 0.4f, 0.4f, 0.6f };
                    // 上辺
                    RECT top = { (LONG)meleeX, (LONG)meleeY, (LONG)(meleeX + meleeW), (LONG)(meleeY + 1) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), top, frameColor);
                    // 下辺
                    RECT bot = { (LONG)meleeX, (LONG)(meleeY + meleeFullH - 1), (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), bot, frameColor);
                    // 左辺
                    RECT lft = { (LONG)meleeX, (LONG)meleeY, (LONG)(meleeX + 1), (LONG)(meleeY + meleeFullH) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), lft, frameColor);
                    // 右辺
                    RECT rgt = { (LONG)(meleeX + meleeW - 1), (LONG)meleeY, (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), rgt, frameColor);
                }

                // === ストックバー ===
                {
                    float stockX = meleeX + meleeW + 4.0f;
                    float stockW = 6.0f;
                    float stockH = meleeFullH;
                    float divider = 1.0f;

                    RECT stockBgRect = {
                        (LONG)stockX, (LONG)meleeY,
                        (LONG)(stockX + stockW), (LONG)(meleeY + stockH)
                    };
                    DirectX::XMVECTORF32 bgColor3 = { 0.06f, 0.06f, 0.06f, 0.7f };
                    m_spriteBatch->Draw(m_whitePixel.Get(), stockBgRect, bgColor3);

                    float sectionH = (stockH - divider * (m_meleeMaxCharges - 1)) / (float)m_meleeMaxCharges;

                    for (int i = 0; i < m_meleeMaxCharges; i++)
                    {
                        float sy = meleeY + stockH - (i + 1) * sectionH - i * divider;
                        RECT secRect = {
                            (LONG)(stockX + 1), (LONG)sy,
                            (LONG)(stockX + stockW - 1), (LONG)(sy + sectionH)
                        };

                        if (i < m_meleeCharges)
                        {
                            DirectX::XMVECTORF32 stockColor = { 1.0f, 0.6f, 0.1f, 1.0f };
                            m_spriteBatch->Draw(m_whitePixel.Get(), secRect, stockColor);
                        }
                        else
                        {
                            DirectX::XMVECTORF32 emptyColor = { 0.15f, 0.15f, 0.15f, 0.4f };
                            m_spriteBatch->Draw(m_whitePixel.Get(), emptyColor, emptyColor);
                        }
                    }
                }
            }

        }

        // =============================================
        // チュートリアル操作説明（常時表示）
        // =============================================
        if (m_showTutorial && m_font)
        {
            float screenW = (float)m_outputWidth;
            float screenH = (float)m_outputHeight;

            float panelW = 320.0f;
            float panelH = 260.0f;
            float panelX = screenW - panelW - 20.0f;
            float panelY = screenH - panelH - 80.0f;

            // 半透明の黒背景
            RECT bgRect = {
                (LONG)panelX, (LONG)panelY,
                (LONG)(panelX + panelW), (LONG)(panelY + panelH)
            };
            DirectX::XMVECTORF32 bgColor = { 0.0f, 0.0f, 0.0f, 0.6f };
            m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

            // 枠線（上）
            RECT borderTop = { (LONG)panelX, (LONG)panelY, (LONG)(panelX + panelW), (LONG)(panelY + 2) };
            DirectX::XMVECTORF32 borderColor = { 0.8f, 0.15f, 0.0f, 0.8f };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderTop, borderColor);
            // 枠線（下）
            RECT borderBot = { (LONG)panelX, (LONG)(panelY + panelH - 2), (LONG)(panelX + panelW), (LONG)(panelY + panelH) };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderBot, borderColor);
            // 枠線（左）
            RECT borderLeft = { (LONG)panelX, (LONG)panelY, (LONG)(panelX + 2), (LONG)(panelY + panelH) };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderLeft, borderColor);
            // 枠線（右）
            RECT borderRight = { (LONG)(panelX + panelW - 2), (LONG)panelY, (LONG)(panelX + panelW), (LONG)(panelY + panelH) };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderRight, borderColor);

            // テキスト色
            DirectX::XMVECTORF32 titleColor = { 0.9f, 0.3f, 0.0f, 1.0f };
            DirectX::XMVECTORF32 keyColor = { 1.0f, 0.85f, 0.2f, 1.0f };
            DirectX::XMVECTORF32 descColor = { 0.85f, 0.85f, 0.85f, 1.0f };

            float x = panelX + 15.0f;
            float y = panelY + 12.0f;
            float lineH = 26.0f;

            // タイトル
            m_font->DrawString(m_spriteBatch.get(), L"- CONTROLS -",
                DirectX::XMFLOAT2(panelX + panelW * 0.5f - 50.0f, y), titleColor);
            y += lineH + 6.0f;

            // 操作一覧
            struct { const wchar_t* key; const wchar_t* desc; } controls[] = {
                { L"W A S D",       L"Move" },
                { L"Mouse",         L"Look" },
                { L"Left Click",    L"Shoot" },
                { L"Right Click",   L"Shield / Parry" },
                { L"Side Button",   L"Melee" },
                { L"E",             L"Shield Throw" },
                { L"Space",         L"Jump" },
                { L"F1",            L"Debug" },
            };

            for (const auto& ctrl : controls)
            {
                m_font->DrawString(m_spriteBatch.get(), ctrl.key,
                    DirectX::XMFLOAT2(x, y), keyColor);
                m_font->DrawString(m_spriteBatch.get(), ctrl.desc,
                    DirectX::XMFLOAT2(x + 150.0f, y), descColor);
                y += lineH;
            }
        }

        // =============================================
        // ボスHPバー（画面上部中央）
        // =============================================
        {
            // 生きてるBOSS/MIDBOSSを探す
            const Enemy* bossEnemy = nullptr;
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS)
                    && enemy.isAlive && !enemy.isDying)
                {
                    // BOSSを優先（両方いる場合）
                    if (!bossEnemy || enemy.type == EnemyType::BOSS)
                        bossEnemy = &enemy;
                }
            }

            if (bossEnemy && m_whitePixel)
            {
                float screenW = (float)m_outputWidth;

                // バーのサイズと位置
                float barW = 500.0f;
                float barH = 14.0f;
                float barX = (screenW - barW) * 0.5f;  // 画面中央
                float barY = 40.0f;                      // 上端から40px

                // HP割合
                float hpPercent = (float)bossEnemy->health / (float)bossEnemy->maxHealth;
                if (hpPercent < 0.0f) hpPercent = 0.0f;
                if (hpPercent > 1.0f) hpPercent = 1.0f;

                // 背景（暗い）
                RECT bgRect = { (LONG)barX, (LONG)barY, (LONG)(barX + barW), (LONG)(barY + barH) };
                DirectX::XMVECTORF32 bgColor = { 0.1f, 0.0f, 0.0f, 0.7f };
                m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

                //  残像バー（白・ゆっくり減る）
                float trailW = barW * m_bossHpTrail;
                RECT trailRect = { (LONG)barX, (LONG)barY, (LONG)(barX + trailW), (LONG)(barY + barH) };
                DirectX::XMVECTORF32 trailColor = { 1.0f, 1.0f, 1.0f, 0.4f };
                m_spriteBatch->Draw(m_whitePixel.Get(), trailRect, trailColor);

                //  表示HPバー（ぬるっと減る）
                float fillW = barW * m_bossHpDisplay;
                RECT fillRect = { (LONG)barX, (LONG)barY, (LONG)(barX + fillW), (LONG)(barY + barH) };

                DirectX::XMVECTORF32 fillColor;
                if (m_bossHpDisplay > 0.5f)
                    fillColor = { 0.8f, 0.1f, 0.0f, 0.9f };
                else if (m_bossHpDisplay > 0.25f)
                    fillColor = { 0.9f, 0.4f, 0.0f, 0.9f };
                else
                    fillColor = { 1.0f, 0.8f, 0.0f, 0.9f };

                m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, fillColor);

                // 枠線（4辺）
                DirectX::XMVECTORF32 frameColor = { 0.6f, 0.6f, 0.6f, 0.8f };
                RECT top = { (LONG)barX, (LONG)barY, (LONG)(barX + barW), (LONG)(barY + 1) };
                RECT bot = { (LONG)barX, (LONG)(barY + barH - 1), (LONG)(barX + barW), (LONG)(barY + barH) };
                RECT lft = { (LONG)barX, (LONG)barY, (LONG)(barX + 1), (LONG)(barY + barH) };
                RECT rgt = { (LONG)(barX + barW - 1), (LONG)barY, (LONG)(barX + barW), (LONG)(barY + barH) };
                m_spriteBatch->Draw(m_whitePixel.Get(), top, frameColor);
                m_spriteBatch->Draw(m_whitePixel.Get(), bot, frameColor);
                m_spriteBatch->Draw(m_whitePixel.Get(), lft, frameColor);
                m_spriteBatch->Draw(m_whitePixel.Get(), rgt, frameColor);

                // ボス名テキスト
                if (m_font)
                {
                    const wchar_t* bossName = (bossEnemy->type == EnemyType::BOSS) ? L"BOSS" : L"MIDBOSS";
                    m_font->DrawString(m_spriteBatch.get(), bossName,
                        DirectX::XMFLOAT2(barX, barY - 22.0f),
                        DirectX::XMVECTORF32{ 0.9f, 0.2f, 0.0f, 1.0f });
                }
            }
        }


        m_spriteBatch->End();
    }



}



// ============================================================
//  RenderDamageFlash - 被弾時の赤フラッシュ
//
//  【表示条件】m_damageFlashAlpha > 0（被弾後に減衰）
//  【描画方式】PrimitiveBatchで画面全体を赤く覆う
// ============================================================
void Game::RenderDamageFlash()
{
    auto context = m_d3dContext.Get();

    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    float alpha = m_player->GetDamageTimer();  // タイマーに応じてフェードアウト
    DirectX::XMFLOAT4 bloodColor(0.8f, 0.0f, 0.0f, alpha * 0.3f);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面端を赤く
    float borderSize = 100.0f;

    // 上
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight - borderSize, 1.0f), bloodColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(-halfWidth, halfHeight - borderSize, 1.0f), bloodColor);

    primitiveBatch->DrawQuad(v1, v2, v3, v4);


    primitiveBatch->End();
}


// ============================================================
//  RenderLowHealthVignette - 低HP時の赤黒ビネット
//
//  【表示条件】HP < 30（低体力時に常時表示）
//  【描画方式】中央→端に向かって赤黒のグラデーション
//  パルス演出で「ドクドク」と脈打つような表現
// ============================================================
void Game::RenderLowHealthVignette()
{
    // テクスチャが読み込めていなければスキップ
    if (!m_lowHealthVignetteSRV || !m_player) return;

    // === HP比率を計算 ===
    int   hp = m_player->GetHealth();
    int   maxHp = 100;  // 最大HPに合わせて変更してね
    float ratio = (float)hp / (float)maxHp;  // 1.0=満タン, 0.0=瀕死

    // HP70%以上なら何も表示しない
    if (ratio > 0.8f) return;

    // === 強度を計算（0.0?1.0） ===
    // HP70%で0.0、HP0%で1.0
    float intensity = 1.0f - (ratio / 0.7f);
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    // === 心拍パルス（HP30%以下で発動） ===
    float pulse = 1.0f;
    if (ratio < 0.3f)
    {
        // 心拍リズム: HP低いほど速い（1.5?3.5Hz）
        float heartRate = 1.5f + (1.0f - ratio / 0.3f) * 2.0f;

        // sin^8 で鋭い「ドクッ」パルスを作る
        float t = sinf(m_accumulatedAnimTime * heartRate * 3.14159f);
        t = t * t;  // sin^2
        t = t * t;  // sin^4
        t = t * t;  // sin^8（鋭いスパイク）

        // パルスの振れ幅（HP低いほど大きい）
        float pulseStrength = 0.15f + (1.0f - ratio / 0.3f) * 0.4f;
        pulse = 1.0f + t * pulseStrength;
    }

    // === 最終アルファ ===
    // intensity（0?1）× pulse（1?1.5くらい）
    float alpha = intensity * pulse;
    if (alpha > 1.0f) alpha = 1.0f;

    // === SpriteBatchで画面全体にテクスチャを描画 ===
    auto context = m_d3dContext.Get();

    // ブレンドステート: アルファブレンド
    // （テクスチャのアルファ × 頂点カラーのアルファ で合成される）
    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,   // ソート方式
        m_states->NonPremultiplied()         // // PNG用（プリマルチプライドじゃない）
    );

    // 画面全体に描画
    RECT destRect = { 0, 0, (LONG)m_outputWidth, (LONG)m_outputHeight };

    // 色の調整:
    // - RGBで赤の濃さを制御
    // - Aでアルファ（不透明度）を制御
    DirectX::XMVECTORF32 tintColor = { {
        1.0f,          // R: そのまま
        0.0f,          // G: 緑を消す（より赤く）
        0.0f,          // B: 青を消す
        alpha * 1.7f  // A: HPに連動（最大85%）
    } };

    m_spriteBatch->Draw(
        m_lowHealthVignetteSRV.Get(),  // テクスチャ
        destRect,                       // 描画先（画面全体）
        tintColor                       // 色とアルファ
    );

    m_spriteBatch->End();
}

//  スピードライン描画 

// ============================================================
//  RenderSpeedLines - ダッシュ時のスピードライン
//
//  【表示条件】m_speedLineAlpha > 0（ダッシュ中に増加）
//  【描画方式】画面中央から放射状にランダムな線を描画
//  疑似乱数（sin/cos）でフレーム間の揺らぎを表現
// ============================================================
void Game::RenderSpeedLines()
{
    if (m_speedLineAlpha <= 0.01f) return;

    auto context = m_d3dContext.Get();

    // 2D描画セットアップ（RenderDamageFlashと同じパターン）
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    batch->Begin();

    float halfW = m_outputWidth * 0.5f;
    float halfH = m_outputHeight * 0.5f;
    float alpha = m_speedLineAlpha;

    int lineCount = 28;  // 放射状の線の数

    for (int i = 0; i < lineCount; i++)
    {
        // 放射角度
        float angle = (float)i / lineCount * 6.283f;

        // ランダムなゆらぎ（毎フレーム変わって動いてる感じ）
        float jitter = ((float)(rand() % 1000) / 1000.0f) * 0.15f;
        angle += jitter;

        float cosA = cosf(angle);
        float sinA = sinf(angle);

        // 外側の点（画面端の外まで伸ばす）
        float outerDist = halfW * 1.3f;
        float outerX = cosA * outerDist;
        float outerY = sinA * outerDist;

        // 内側の点（画面中央に向かって）
        // ランダムな長さで「流れてる」感じ
        float innerRatio = 0.45f + ((float)(rand() % 1000) / 1000.0f) * 0.25f;
        float innerX = cosA * outerDist * innerRatio;
        float innerY = sinA * outerDist * innerRatio;

        // 線の太さ（外側が太く、内側が細い）
        float thickness = 2.5f + ((float)(rand() % 1000) / 1000.0f) * 3.0f;
        float perpX = -sinA * thickness;
        float perpY = cosA * thickness;

        // 色（白、外側は不透明、内側は透明）
        float lineAlpha = alpha * (0.3f + ((float)(rand() % 1000) / 1000.0f) * 0.5f);
        DirectX::XMFLOAT4 outerColor(1.0f, 1.0f, 1.0f, lineAlpha);
        DirectX::XMFLOAT4 innerColor(1.0f, 1.0f, 1.0f, 0.0f);

        // 三角形2枚で1本のスピードラインを描画
        DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(outerX + perpX, outerY + perpY, 1.0f), outerColor);
        DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(outerX - perpX, outerY - perpY, 1.0f), outerColor);
        DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(innerX, innerY, 1.0f), innerColor);

        batch->DrawTriangle(v1, v2, v3);
    }

    batch->End();
}


// ============================================================
//  RenderDashOverlay - ダッシュ時の画面効果（色調変化）
//
//  【表示条件】m_dashOverlayAlpha > 0
//  【描画方式】画面全体を青みがかった半透明でオーバーレイ
// ============================================================
void Game::RenderDashOverlay()
{
    // テクスチャがないorアルファ0なら描画しない
    if (!m_dashSpeedlineSRV || m_dashOverlayAlpha <= 0.01f) return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    RECT destRect = { 0, 0, (LONG)m_outputWidth, (LONG)m_outputHeight };

    // 回転させてバリエーションを出す（毎フレーム微妙に回転）
    DirectX::XMVECTORF32 tintColor = { {
        0.9f, 0.9f, 1.0f,
        m_dashOverlayAlpha
    } };

    m_spriteBatch->Draw(
        m_dashSpeedlineSRV.Get(),
        destRect,
        tintColor
    );

    m_spriteBatch->End();
}


// ============================================================
//  RenderReloadWarning - リロード警告 + 進捗バー
//
//  【表示条件】弾切れ時 or リロード中
//  【描画内容】
//  - "RELOADING" テキスト（点滅）
//  - 進捗バー（ゲージが左から右に伸びる）
//  - 弾切れ時は赤い "NO AMMO" 警告
// ============================================================
void Game::RenderReloadWarning()
{
    // === リロード中の進捗バーも表示 ===
    bool isReloading = m_weaponSystem->IsReloading();
    bool showWarning = (m_reloadWarningAlpha > 0.01f) || isReloading;

    if (!showWarning) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // =============================================
    // "RELOAD" 警告テキスト（弾切れ時）
    // =============================================
    if (m_reloadWarningAlpha > 0.01f && !isReloading)
    {
        // 脈動（パルス）: sinで明滅
        float pulse = 0.5f + sinf(m_reloadWarningTimer * 6.0f) * 0.5f;
        float alpha = m_reloadWarningAlpha * (0.5f + pulse * 0.5f);

        // スケール弾み（登場時にバウンス）
        float scaleT = m_reloadWarningTimer;
        float scale = 1.0f;
        if (scaleT < 0.3f)
        {
            float t = scaleT / 0.3f;
            scale = 1.0f + sinf(t * 3.14159f) * 0.3f;  // 登場時に1.3倍→1.0倍
        }

        // --- メインテキスト ---
        const wchar_t* text = L"[  R E L O A D  ]";

        DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(text);
        float textW = DirectX::XMVectorGetX(textSize) * scale;
        float textH = DirectX::XMVectorGetY(textSize) * scale;
        float textX = (W - textW) * 0.5f;
        float textY = H * 0.58f;

        // グロー（影として2回描画）
        DirectX::XMVECTORF32 glowColor = { 0.3f, 0.0f, 0.0f, alpha * 0.6f };
        for (int dx = -2; dx <= 2; dx += 4)
        {
            for (int dy = -2; dy <= 2; dy += 4)
            {
                m_fontLarge->DrawString(m_spriteBatch.get(), text,
                    DirectX::XMFLOAT2(textX + dx, textY + dy),
                    glowColor, 0.0f,
                    DirectX::XMFLOAT2(0, 0),
                    scale);
            }
        }

        // メインテキスト（赤）
        DirectX::XMVECTORF32 mainColor = { 0.95f, 0.15f, 0.1f, alpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), text,
            DirectX::XMFLOAT2(textX, textY),
            mainColor, 0.0f,
            DirectX::XMFLOAT2(0, 0),
            scale);

        // --- サブテキスト ---
        const wchar_t* subText = L"Press  R";

        DirectX::XMVECTOR subSize = m_font->MeasureString(subText);
        float subW = DirectX::XMVectorGetX(subSize);
        float subX = (W - subW) * 0.5f;
        float subY = textY + textH + 8.0f;

        float subPulse = 0.3f + pulse * 0.7f;
        DirectX::XMVECTORF32 subColor = { 0.7f, 0.7f, 0.7f, alpha * subPulse };
        m_font->DrawString(m_spriteBatch.get(), subText,
            DirectX::XMFLOAT2(subX, subY), subColor);
    }

    // =============================================
    //  リロード進捗バー（リロード中）
    // =============================================
    if (isReloading && m_whitePixel)
    {
        float progress = m_reloadAnimProgress;  // 0?1

        float barW = W * 0.25f;
        float barH = 4.0f;
        float barX = (W - barW) * 0.5f;
        float barY = H * 0.62f;

        // 背景（暗い）
        RECT bgRect = {
            (LONG)barX, (LONG)barY,
            (LONG)(barX + barW), (LONG)(barY + barH)
        };
        DirectX::XMVECTORF32 bgColor = { 0.15f, 0.15f, 0.15f, 0.8f };
        m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

        // 進捗（白→赤のグラデーション）
        float fillW = barW * progress;
        RECT fillRect = {
            (LONG)barX, (LONG)barY,
            (LONG)(barX + fillW), (LONG)(barY + barH)
        };
        // 完了に近づくほど白くなる
        float r = 1.0f;
        float g = 0.3f + progress * 0.7f;
        float b = 0.3f + progress * 0.7f;
        DirectX::XMVECTORF32 fillColor = { r, g, b, 0.9f };
        m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, fillColor);

        // テキスト "RELOADING..."
        const wchar_t* reloadText = L"RELOADING...";
        DirectX::XMVECTOR rSize = m_font->MeasureString(reloadText);
        float rW = DirectX::XMVectorGetX(rSize);
        float rX = (W - rW) * 0.5f;
        float rY = barY - 24.0f;

        DirectX::XMVECTORF32 reloadColor = { 0.8f, 0.8f, 0.8f, 0.9f };
        m_font->DrawString(m_spriteBatch.get(), reloadText,
            DirectX::XMFLOAT2(rX, rY), reloadColor);
    }

    m_spriteBatch->End();
}


// ============================================================
//  RenderWaveBanner - ウェーブ開始バナー
//
//  【表示タイミング】新しいウェーブが始まった時
//  【演出】右から中央にスライドイン → 一定時間表示 → フェードアウト
//  ノーマル/ミッドボス/ボスで色とテキストが変わる
// ============================================================
void Game::RenderWaveBanner()
{
    if (m_waveBannerTimer <= 0.0f) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;
    float t = m_waveBannerTimer / m_waveBannerDuration;  // 1→0

    // フェードイン/アウト
    float alpha = 1.0f;
    if (t > 0.85f) alpha = (1.0f - t) / 0.15f;      // 登場
    else if (t < 0.2f) alpha = t / 0.2f;              // 消滅

    // スライドイン（右から中央へ）
    float slideOffset = 0.0f;
    float slideT = 1.0f - t;
    if (slideT < 0.12f)
        slideOffset = (1.0f - slideT / 0.12f) * 300.0f;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // === バナー画像 ===
    auto& bannerSRV = m_waveBannerIsBoss ? m_waveBannerBossSRV : m_waveBannerNormalSRV;
    if (bannerSRV)
    {
        float bannerH = H * 0.12f;
        float bannerY = (H - bannerH) * 0.5f;
        RECT destRect = {
            (LONG)(0 + slideOffset * 0.3f),
            (LONG)bannerY,
            (LONG)(W + slideOffset * 0.3f),
            (LONG)(bannerY + bannerH)
        };
        DirectX::XMVECTORF32 bannerColor = { 1.0f, 1.0f, 1.0f, alpha };
        m_spriteBatch->Draw(bannerSRV.Get(), destRect, bannerColor);
    }

    // === ウェーブ番号（大きくスライドイン） ===
    if (m_fontLarge)
    {
        wchar_t numText[16];
        swprintf_s(numText, L"%d", m_waveBannerNumber);

        DirectX::XMVECTOR numSize = m_fontLarge->MeasureString(numText);
        float numW = DirectX::XMVectorGetX(numSize);
        float numX = (W - numW) * 0.5f + slideOffset + W * 0.16f;
        float numY = H * 0.5f - DirectX::XMVectorGetY(numSize) * 0.49f;

        // グロー
        DirectX::XMVECTORF32 glowColor = m_waveBannerIsBoss
            ? DirectX::XMVECTORF32{ 0.4f, 0.02f, 0.0f, alpha * 0.6f }
        : DirectX::XMVECTORF32{ 0.4f, 0.35f, 0.0f, alpha * 0.6f };
        for (int dx = -3; dx <= 3; dx += 3)
            for (int dy = -3; dy <= 3; dy += 3)
                m_fontLarge->DrawString(m_spriteBatch.get(), numText,
                    DirectX::XMFLOAT2(numX + dx, numY + dy), glowColor);

        // メイン
        DirectX::XMVECTORF32 numColor = m_waveBannerIsBoss
            ? DirectX::XMVECTORF32{ 1.0f, 0.2f, 0.1f, alpha }
        : DirectX::XMVECTORF32{ 1.0f, 0.85f, 0.2f, alpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), numText,
            DirectX::XMFLOAT2(numX, numY), numColor);
    }

    m_spriteBatch->End();
}



// ============================================================
//  RenderScoreHUD - スコア表示（血しぶき背景付き）
//
//  【描画位置】画面左上
//  【描画内容】スコア数値 + 血しぶきテクスチャの背景
//  背景テクスチャが無い場合は数値のみ表示
// ============================================================
void Game::RenderScoreHUD()
{
    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // === 血しぶき背景 ===
    if (m_scoreBackdropSRV)
    {
        float bgW = W * 0.22f;
        float bgH = H * 0.07f;
        float bgX = 10.0f;
        float bgY = 10.0f;

        // 加算時にちょっと揺れる
        float shakeX = 0.0f, shakeY = 0.0f;
        if (m_scoreShakeTimer > 0.0f)
        {
            float intensity = m_scoreShakeTimer / 0.2f;
            shakeX = sinf(m_scoreShakeTimer * 80.0f) * 3.0f * intensity;
            shakeY = cosf(m_scoreShakeTimer * 60.0f) * 2.0f * intensity;
        }

        RECT bgRect = {
            (LONG)(bgX + shakeX),
            (LONG)(bgY + shakeY),
            (LONG)(bgX + bgW + shakeX),
            (LONG)(bgY + bgH + shakeY)
        };

        // 加算時に少し明るくなる
        float flashBright = 1.0f;
        if (m_scoreFlashTimer > 0.0f)
            flashBright = 1.0f + (m_scoreFlashTimer / 0.4f) * 0.8f;

        DirectX::XMVECTORF32 bgColor = {
            flashBright, flashBright, flashBright, 0.9f
        };
        m_spriteBatch->Draw(m_scoreBackdropSRV.Get(), bgRect, bgColor);

        // === スコア数字 ===
        if (m_fontLarge)
        {
            wchar_t scoreText[32];
            swprintf_s(scoreText, L"%d", (int)m_scoreDisplayValue);

            DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(scoreText);
            float textH = DirectX::XMVectorGetY(textSize);
            float textX = bgX + 15.0f + shakeX;
            float textY = bgY + (bgH - textH) * 0.5f + shakeY;

            // グロー（赤い影）
            DirectX::XMVECTORF32 glowColor = { 0.5f, 0.02f, 0.0f, 0.6f };
            for (int dx = -2; dx <= 2; dx += 4)
                for (int dy = -2; dy <= 2; dy += 4)
                    m_fontLarge->DrawString(m_spriteBatch.get(), scoreText,
                        DirectX::XMFLOAT2(textX + dx, textY + dy), glowColor);

            // メイン（加算時は赤→白に光る）
            float flash = (m_scoreFlashTimer > 0.0f) ? m_scoreFlashTimer / 0.4f : 0.0f;
            DirectX::XMVECTORF32 mainColor = {
                0.95f,
                0.85f + flash * 0.15f,
                0.8f + flash * 0.2f,
                1.0f
            };
            m_fontLarge->DrawString(m_spriteBatch.get(), scoreText,
                DirectX::XMFLOAT2(textX, textY), mainColor);
        }

        // === "PTS" ラベル（小さく右に） ===
        if (m_font)
        {
            wchar_t scoreText[32];
            swprintf_s(scoreText, L"%d", (int)m_scoreDisplayValue);
            DirectX::XMVECTOR scoreSize = m_fontLarge->MeasureString(scoreText);
            float scoreW = DirectX::XMVectorGetX(scoreSize);

            float ptsX = bgX + 15.0f + scoreW + 8.0f + shakeX;
            float ptsY = bgY + bgH * 0.5f - 5.0f + shakeY;

            DirectX::XMVECTORF32 ptsColor = { 0.6f, 0.15f, 0.1f, 0.8f };
            m_font->DrawString(m_spriteBatch.get(), L"PTS",
                DirectX::XMFLOAT2(ptsX, ptsY), ptsColor);
        }
    }

    m_spriteBatch->End();
}


// ============================================================
//  RenderClawDamage - 爪攻撃のスクラッチ痕
//
//  【表示条件】敵の近接攻撃が命中した時
//  【描画方式】スクラッチテクスチャを画面にオーバーレイ
//  時間経過でフェードアウト
// ============================================================
void Game::RenderClawDamage()
{
    if (m_clawTimer <= 0.0f || !m_clawDamageSRV) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;
    float t = m_clawTimer / m_clawDuration;  // 1→0

    // スラッシュアニメーション: 右上から右下へスライド
    // t=1(開始): 画面外の上 → t=0.5: 画面中央 → t=0: 画面下でフェードアウト
    float slideProgress = 1.0f - t;  // 0→1

    // テクスチャサイズ
    float clawW = W * 0.5f;
    float clawH = H * 1.2f;

    // X位置: 画面右寄り
    float clawX = (W - clawW) * 0.5f;

    // Y位置: 上から下へスライド（高速で引っかく感じ）
    float startY = -clawH * 0.8f;   // 画面上の外
    float endY = H * 0.1f;          // 画面中央付近
    float clawY;

    if (slideProgress < 0.15f)
    {
        // 最初の15%: 高速スラッシュ（上→中央）
        float p = slideProgress / 0.15f;
        p = p * p * (3.0f - 2.0f * p);  // スムーズステップ
        clawY = startY + (endY - startY) * p;
    }
    else
    {
        // 残り: 中央に留まってフェードアウト
        clawY = endY;
    }

    // アルファ: 一気に出て、じわっと消える
    float alpha;
    if (slideProgress < 0.15f)
        alpha = slideProgress / 0.15f;          // フェードイン
    else if (slideProgress < 0.4f)
        alpha = 1.0f;                            // フル表示
    else
        alpha = 1.0f - (slideProgress - 0.4f) / 0.6f;  // フェードアウト

    alpha = max(0.0f, min(1.0f, alpha));

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    RECT destRect = {
        (LONG)clawX,
        (LONG)clawY,
        (LONG)(clawX + clawW),
        (LONG)(clawY + clawH)
    };

    DirectX::XMVECTORF32 color = { 1.0f, 1.0f, 1.0f, alpha * 0.9f };
    m_spriteBatch->Draw(m_clawDamageSRV.Get(), destRect, color);

    m_spriteBatch->End();
}



// ============================================================
//  RenderGloryKillFlash - グローリーキル時の白フラッシュ
//
//  【表示条件】グローリーキル成功時（0.3秒間）
//  【描画方式】画面全体を白くフラッシュ → 急速にフェードアウト
//  「止めを刺した」インパクトを視覚的に強調
// ============================================================
void Game::RenderGloryKillFlash()
{
    return;

    // グローリーキルフラッシュが無効なら何もしない
    if (m_gloryKillFlashTimer <= 0.0f || m_gloryKillFlashAlpha <= 0.0f)
        return;

    auto context = m_d3dContext.Get();

    // 2D描画用の設定
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // PrimitiveBatchを作成
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // 白いフラッシュの色（透明度はm_gloryKillFlashAlpha）
    DirectX::XMFLOAT4 flashColor(1.0f, 1.0f, 1.0f, m_gloryKillFlashAlpha * 0.01f);

    // 画面のサイズ（中心からの距離）
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面全体を覆う四角形
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), flashColor);   // 左上
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), flashColor);    // 右上
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, -halfHeight, 1.0f), flashColor);   // 右下
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(-halfWidth, -halfHeight, 1.0f), flashColor);  // 左下

    // 四角形を描画
    primitiveBatch->DrawQuad(v1, v2, v3, v4);

    primitiveBatch->End();
}