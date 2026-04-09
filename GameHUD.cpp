// ============================================================
//  GameHUD.cpp
//  HUD�`��i�X�R�A/HP/�e��/�V�[���h/�E�F�[�u�o�i�[/�G�t�F�N�g�j
//
//  �y�Ӗ��z
//  - �Q�[���v���C����2D UI�v�f�̑S�`��
//  - �_���[�W�t���b�V��/��̗̓r�l�b�g���̉�ʃG�t�F�N�g
//  - �X�^�C�������NHUD�i�e�N�X�`���Łj
//  - �E�F�[�u�o�i�[�\��
//  - �X�R�AHUD�i�����Ԃ��w�i�t���j
//  - �����[�h�x��/�܃_���[�W/�O���[���[�L���t���b�V��
//
//  �y�݌v�Ӑ}�z
//  �u�v���C���ɉ�ʂɕ\�������2D�v�f�v��S�ďW��B
//  3D�`��iGameRender.cpp�j�Ƃ͖��m�ɕ����B
//  SpriteBatch + PrimitiveBatch ��2D�����_�����O�n��S���B
//
//  �y�܂܂�郁�\�b�h�z
//  DrawUI                - ���C��HUD�iHP/�e��/�V�[���h/�X�^�C�������N�j
//  RenderDamageFlash     - ��e���̐ԃt���b�V��
//  RenderLowHealthVignette - ��HP���̐ԍ��r�l�b�g
//  RenderSpeedLines      - �_�b�V�����̃X�s�[�h���C��
//  RenderDashOverlay     - �_�b�V�����̉�ʌ���
//  RenderReloadWarning   - �����[�h�x�� + �i���o�[
//  RenderWaveBanner      - �E�F�[�u�J�n�o�i�[
//  RenderScoreHUD        - �X�R�A�\���i�����Ԃ��w�i�j
//  RenderClawDamage      - �܍U���̃X�N���b�`��
//  RenderGloryKillFlash  - �O���[���[�L�����̔��t���b�V��
// ============================================================

#include "Game.h"

using namespace DirectX;

// ============================================================
//  HUD���ʒ萔
// ============================================================
namespace HUDConstants
{
    // ��ʃG�t�F�N�g�̊�{�p�����[�^
    constexpr float DAMAGE_FLASH_NEAR_PLANE = 0.1f;
    constexpr float DAMAGE_FLASH_FAR_PLANE = 10.0f;

    // �X�s�[�h���C���̃p�����[�^
    constexpr int   SPEED_LINE_COUNT = 40;
    constexpr float SPEED_LINE_MIN_LENGTH = 60.0f;
    constexpr float SPEED_LINE_MAX_LENGTH = 200.0f;

    // �O���[���[�L���t���b�V��
    constexpr float GLORY_FLASH_DURATION = 0.3f;

    // �E�F�[�u�o�i�[�̃t�F�[�h�^�C�~���O
    constexpr float BANNER_FADE_IN_THRESHOLD = 0.85f;   // t > 0.85 �œo��
    constexpr float BANNER_FADE_OUT_THRESHOLD = 0.2f;    // t < 0.2 �ŏ���
}


// ============================================================
//  DrawUI - ���C��HUD�`��
//
//  �y�`����e�z
//  1. UISystem::DrawAll() ��HP/�e��/�N���X�w�A����`��
//  2. �X�^�C�������NHUD�i�e�N�X�`���ŁA�����N��+�Q�[�W�j
//  3. �V�[���hHP/��ԃo�[
//  4. �`���[�W�G�l���M�[�Q�[�W
//  5. �ߐڃ`���[�W�Q�[�W�i�X�g�b�N�\���j
//  6. �p���B�^�C�~���O�����O
// ============================================================
void Game::DrawUI()
{
    // --- UI�`��̂��߂̋��ʐݒ� ---
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

    // UISystem�ɑS�Ă̕`����Ϗ�
    m_uiSystem->DrawAll(primitiveBatch.get(),
        m_player.get(),
        m_weaponSystem.get(),
        m_waveManager.get());

    // === �Ǖ���̍w��UI ===
    //if (m_nearbyWeaponSpawn != nullptr)
    //{
    //    // ���łɎ����Ă��镐�킩�H
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
    // �X�^�C�������N HUD�\���i�e�N�X�`���Łj
    // =============================================
    if (m_styleRank && m_spriteBatch && m_states)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // --- �����N�e�N�X�`���`�� ---
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

            // --- �R���{���i�����N�摜�̉��j---
            int combo = m_styleRank->GetComboCount();
            if (combo > 1 && m_comboTexturesLoaded)
            {
                // �R���{���𕶎���ɕϊ��i��: 12 �� "12"�j
                char comboStr[16];
                sprintf_s(comboStr, "%d", combo);
                int numDigits = (int)strlen(comboStr);

                // �e���̕\���T�C�Y
                float digitW = 40.0f;
                float digitH = 40.0f;
                float startX = rankX;
                float startY = rankY + 130.0f;

                // ������1�����e�N�X�`���ŕ`��
                for (int i = 0; i < numDigits; i++)
                {
                    int digit = comboStr[i] - '0';  // ���������l�ϊ�
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

                // "COMBO" ���x���i�����̉E�ɕ\���j
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
        // HP & �V�[���h�o�[ �e�N�X�`��HUD�`��
        // ==============================================
        if (m_shieldHudLoaded)
        {
            // --- �o�[�̈ʒu�E�T�C�Y�i�s�N�Z�����W�j---
            float barW = 260.0f;    // �o�[�̕�
            float barH = 6.0f;     // �o�[�̍���
            float barX = 30.0f;     // ���[
            float hpBarY = (float)m_outputHeight - 50.0f;   // HP�o�[��Y�ʒu
            float shieldBarY = hpBarY + barH + 14.0f;        // �V�[���h�o�[�iHP�̉��j

            // �A�C�R���T�C�Y
            float iconSize = 18.0f;

            // ��������������������������������������������������
            // HP�o�[
            // ��������������������������������������������������
            {
                float hpPercent = (float)m_player->GetHealth() / (float)m_player->GetMaxHealth();
                hpPercent = max(0.0f, min(1.0f, hpPercent));

                // �Q�[�W���g�iHP������sourceRect�𐧌��j
                auto* fillTex = (hpPercent < 0.25f && m_hpHudFillCritical)
                    ? m_hpHudFillCritical.Get()
                    : m_hpHudFillGreen.Get();

                if (fillTex)
                {
                    // sourceRect: �e�N�X�`���̍��[���犄���������؂���
                    RECT srcRect = { 0, 0, (LONG)(512 * hpPercent), 16 };
                    // destRect: ��ʏ�̕\���ʒu�i�������̕��j
                    RECT destRect = {
                        (LONG)barX,
                        (LONG)hpBarY,
                        (LONG)(barX + barW * hpPercent),
                        (LONG)(hpBarY + barH)
                    };
                    m_spriteBatch->Draw(fillTex, destRect, &srcRect, DirectX::Colors::White);
                }

                // �t���[���i��ɑS���ŕ\���j
                if (m_hpHudFrame)
                {
                    RECT frameRect = {
                        (LONG)barX, (LONG)hpBarY,
                        (LONG)(barX + barW), (LONG)(hpBarY + barH)
                    };
                    m_spriteBatch->Draw(m_hpHudFrame.Get(), frameRect, DirectX::Colors::White);
                }
            }

            // ��������������������������������������������������
            // �V�[���h�o�[
            // ��������������������������������������������������
            {
                // �\���pHP�����炩�ɒǏ]
                float shieldPercent = m_shieldDisplayHP / m_shieldMaxHP;
                shieldPercent = max(0.0f, min(1.0f, shieldPercent));

                // �O���E�i�K�[�h/�p���B���̂݁j
                if (m_shieldGlowIntensity > 0.01f && m_shieldHudGlow)
                {
                    float glowExpand = 6.0f;
                    RECT glowRect = {
                        (LONG)(barX - glowExpand),
                        (LONG)(shieldBarY - glowExpand),
                        (LONG)(barX + barW + glowExpand),
                        (LONG)(shieldBarY + barH + glowExpand)
                    };
                    // �����x���O���E���x�Ő���
                    float a = m_shieldGlowIntensity;
                    DirectX::XMVECTORF32 glowColor = { a, a, a, a };
                    m_spriteBatch->Draw(m_shieldHudGlow.Get(), glowRect, glowColor);
                }

                // �Q�[�W���g�i��ԂŐؑցj
                ID3D11ShaderResourceView* shieldFillTex = nullptr;
                if (m_shieldState == ShieldState::Broken)
                {
                    shieldFillTex = nullptr;  // ���Ă� �� ��
                }
                else if (shieldPercent < 0.25f)
                {
                    shieldFillTex = m_shieldHudFillDanger.Get();  // �댯 �� ��
                }
                else if (m_shieldState == ShieldState::Guarding || m_shieldState == ShieldState::Parrying)
                {
                    shieldFillTex = m_shieldHudFillGuard.Get();   // �K�[�h�� �� ���F
                }
                else
                {
                    shieldFillTex = m_shieldHudFillBlue.Get();    // �ʏ� �� ��
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

                // �t���[��
                if (m_shieldHudFrame)
                {
                    RECT frameRect = {
                        (LONG)barX, (LONG)shieldBarY,
                        (LONG)(barX + barW), (LONG)(shieldBarY + barH)
                    };
                    m_spriteBatch->Draw(m_shieldHudFrame.Get(), frameRect, DirectX::Colors::White);
                }

                // �q�r����I�[�o�[���C�i��ꂽ���̂݁j
                if (m_shieldState == ShieldState::Broken && m_shieldHudCrack)
                {
                    RECT crackRect = {
                        (LONG)barX, (LONG)shieldBarY,
                        (LONG)(barX + barW), (LONG)(shieldBarY + barH)
                    };
                    // �_�ł�����
                    float blink = (sinf(m_shieldBrokenTimer * 10.0f) + 1.0f) * 0.5f;
                    DirectX::XMVECTORF32 crackColor = { 1.0f, 1.0f, 1.0f, 0.5f + blink * 0.5f };
                    m_spriteBatch->Draw(m_shieldHudCrack.Get(), crackRect, crackColor);
                }

                // �p���B�����t���b�V��
                if (m_parryFlashTimer > 0.0f && m_shieldHudParryFlash)
                {
                    float flashAlpha = m_parryFlashTimer / 0.3f;  // 0.3�b�����ăt�F�[�h�A�E�g
                    RECT flashRect = {
                        (LONG)(barX - 8),
                        (LONG)(shieldBarY - 6),
                        (LONG)(barX + barW + 8),
                        (LONG)(shieldBarY + barH + 6)
                    };
                    DirectX::XMVECTORF32 flashColor = { flashAlpha, flashAlpha, flashAlpha, flashAlpha };
                    m_spriteBatch->Draw(m_shieldHudParryFlash.Get(), flashRect, flashColor);
                }

                // �V�[���h�A�C�R���i�o�[�����j
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

            //  �`���[�W�G�l���M�[�Q�[�W
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

                // �O�g�i���A1px�傫���j
                RECT frameRect = {
                    (LONG)(left - 1), (LONG)(eBarY - 1),
                    (LONG)(right + 1), (LONG)(eBarY + eBarH + 1)
                };
                DirectX::XMVECTORF32 frameColor = { 0.8f, 0.8f, 0.8f, 0.7f };
                m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), frameRect, frameColor);

                // �w�i�i�Â��j
                RECT bgRect = {
                    (LONG)left, (LONG)eBarY,
                    (LONG)right, (LONG)(eBarY + eBarH)
                };
                DirectX::XMVECTORF32 bgColor = { 0.05f, 0.05f, 0.1f, 0.8f };
                m_spriteBatch->Draw(m_shieldHudFillGuard.Get(), bgRect, bgColor);

                // �G�l���M�[�i���邢�V�A���A���^���Ŕ����P���j
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
            // �ߐڃ`���[�W�Q�[�W�i�V�[���h�o�[�� + �c�X�g�b�N�j
            // =============================================
            if (m_whitePixel)
            {
                float meleeW = 28.0f;
                float meleeFullH = 44.0f;
                float meleeX = barX + barW + 12.0f;
                float meleeY = shieldBarY + barH - meleeFullH;

                // === �w�i�i�^�����j ===
                RECT meleeBgRect = {
                    (LONG)meleeX, (LONG)meleeY,
                    (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH)
                };
                DirectX::XMVECTORF32 bgColor2 = { 0.06f, 0.06f, 0.06f, 0.85f };
                m_spriteBatch->Draw(m_whitePixel.Get(), meleeBgRect, bgColor2);

                // === �Q�[�W���g ===
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

                    // ���`���[�W���Ȃ甒���ۂ��i�����d�˂�
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

                // === �X�L���A�C�R�� ===
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

                // === �O�g�i���s�N�Z���Ŕ����O���[�g�j ===
                {
                    DirectX::XMVECTORF32 frameColor = { 0.4f, 0.4f, 0.4f, 0.6f };
                    // ���
                    RECT top = { (LONG)meleeX, (LONG)meleeY, (LONG)(meleeX + meleeW), (LONG)(meleeY + 1) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), top, frameColor);
                    // ����
                    RECT bot = { (LONG)meleeX, (LONG)(meleeY + meleeFullH - 1), (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), bot, frameColor);
                    // ����
                    RECT lft = { (LONG)meleeX, (LONG)meleeY, (LONG)(meleeX + 1), (LONG)(meleeY + meleeFullH) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), lft, frameColor);
                    // �E��
                    RECT rgt = { (LONG)(meleeX + meleeW - 1), (LONG)meleeY, (LONG)(meleeX + meleeW), (LONG)(meleeY + meleeFullH) };
                    m_spriteBatch->Draw(m_whitePixel.Get(), rgt, frameColor);
                }

                // === �X�g�b�N�o�[ ===
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
        // �`���[�g���A����������i�펞�\���j
        // =============================================
        if (m_showTutorial && m_font)
        {
            float screenW = (float)m_outputWidth;
            float screenH = (float)m_outputHeight;

            float panelW = 320.0f;
            float panelH = 260.0f;
            float panelX = screenW - panelW - 20.0f;
            float panelY = screenH - panelH - 80.0f;

            // �������̍��w�i
            RECT bgRect = {
                (LONG)panelX, (LONG)panelY,
                (LONG)(panelX + panelW), (LONG)(panelY + panelH)
            };
            DirectX::XMVECTORF32 bgColor = { 0.0f, 0.0f, 0.0f, 0.6f };
            m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

            // �g���i��j
            RECT borderTop = { (LONG)panelX, (LONG)panelY, (LONG)(panelX + panelW), (LONG)(panelY + 2) };
            DirectX::XMVECTORF32 borderColor = { 0.8f, 0.15f, 0.0f, 0.8f };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderTop, borderColor);
            // �g���i���j
            RECT borderBot = { (LONG)panelX, (LONG)(panelY + panelH - 2), (LONG)(panelX + panelW), (LONG)(panelY + panelH) };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderBot, borderColor);
            // �g���i���j
            RECT borderLeft = { (LONG)panelX, (LONG)panelY, (LONG)(panelX + 2), (LONG)(panelY + panelH) };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderLeft, borderColor);
            // �g���i�E�j
            RECT borderRight = { (LONG)(panelX + panelW - 2), (LONG)panelY, (LONG)(panelX + panelW), (LONG)(panelY + panelH) };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderRight, borderColor);

            // �e�L�X�g�F
            DirectX::XMVECTORF32 titleColor = { 0.9f, 0.3f, 0.0f, 1.0f };
            DirectX::XMVECTORF32 keyColor = { 1.0f, 0.85f, 0.2f, 1.0f };
            DirectX::XMVECTORF32 descColor = { 0.85f, 0.85f, 0.85f, 1.0f };

            float x = panelX + 15.0f;
            float y = panelY + 12.0f;
            float lineH = 26.0f;

            // �^�C�g��
            m_font->DrawString(m_spriteBatch.get(), L"- CONTROLS -",
                DirectX::XMFLOAT2(panelX + panelW * 0.5f - 50.0f, y), titleColor);
            y += lineH + 6.0f;

            // ����ꗗ
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
        // �{�XHP�o�[�i��ʏ㕔�����j
        // =============================================
        {
            // �����Ă�BOSS/MIDBOSS��T��
            const Enemy* bossEnemy = nullptr;
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS)
                    && enemy.isAlive && !enemy.isDying)
                {
                    // BOSS��D��i��������ꍇ�j
                    if (!bossEnemy || enemy.type == EnemyType::BOSS)
                        bossEnemy = &enemy;
                }
            }

            if (bossEnemy && m_whitePixel)
            {
                float screenW = (float)m_outputWidth;

                // �o�[�̃T�C�Y�ƈʒu
                float barW = 500.0f;
                float barH = 14.0f;
                float barX = (screenW - barW) * 0.5f;  // ��ʒ���
                float barY = 40.0f;                      // ��[����40px

                // HP����
                float hpPercent = (float)bossEnemy->health / (float)bossEnemy->maxHealth;
                if (hpPercent < 0.0f) hpPercent = 0.0f;
                if (hpPercent > 1.0f) hpPercent = 1.0f;

                // �w�i�i�Â��j
                RECT bgRect = { (LONG)barX, (LONG)barY, (LONG)(barX + barW), (LONG)(barY + barH) };
                DirectX::XMVECTORF32 bgColor = { 0.1f, 0.0f, 0.0f, 0.7f };
                m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

                //  �c���o�[�i���E������茸��j
                float trailW = barW * m_bossHpTrail;
                RECT trailRect = { (LONG)barX, (LONG)barY, (LONG)(barX + trailW), (LONG)(barY + barH) };
                DirectX::XMVECTORF32 trailColor = { 1.0f, 1.0f, 1.0f, 0.4f };
                m_spriteBatch->Draw(m_whitePixel.Get(), trailRect, trailColor);

                //  �\��HP�o�[�i�ʂ���ƌ���j
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

                // �g���i4�Ӂj
                DirectX::XMVECTORF32 frameColor = { 0.6f, 0.6f, 0.6f, 0.8f };
                RECT top = { (LONG)barX, (LONG)barY, (LONG)(barX + barW), (LONG)(barY + 1) };
                RECT bot = { (LONG)barX, (LONG)(barY + barH - 1), (LONG)(barX + barW), (LONG)(barY + barH) };
                RECT lft = { (LONG)barX, (LONG)barY, (LONG)(barX + 1), (LONG)(barY + barH) };
                RECT rgt = { (LONG)(barX + barW - 1), (LONG)barY, (LONG)(barX + barW), (LONG)(barY + barH) };
                m_spriteBatch->Draw(m_whitePixel.Get(), top, frameColor);
                m_spriteBatch->Draw(m_whitePixel.Get(), bot, frameColor);
                m_spriteBatch->Draw(m_whitePixel.Get(), lft, frameColor);
                m_spriteBatch->Draw(m_whitePixel.Get(), rgt, frameColor);

                // �{�X���e�L�X�g
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
//  RenderDamageFlash - ��e���̐ԃt���b�V��
//
//  �y�\�������zm_damageFlashAlpha > 0�i��e��Ɍ����j
//  �y�`������zPrimitiveBatch�ŉ�ʑS�̂�Ԃ�����
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

    float alpha = m_player->GetDamageTimer();  // �^�C�}�[�ɉ����ăt�F�[�h�A�E�g
    DirectX::XMFLOAT4 bloodColor(0.8f, 0.0f, 0.0f, alpha * 0.3f);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // ��ʒ[��Ԃ�
    float borderSize = 100.0f;

    // ��
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight - borderSize, 1.0f), bloodColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(-halfWidth, halfHeight - borderSize, 1.0f), bloodColor);

    primitiveBatch->DrawQuad(v1, v2, v3, v4);


    primitiveBatch->End();
}


// ============================================================
//  RenderLowHealthVignette - ��HP���̐ԍ��r�l�b�g
//
//  �y�\�������zHP < 30�i��̗͎��ɏ펞�\���j
//  �y�`������z�������[�Ɍ������Đԍ��̃O���f�[�V����
//  �p���X���o�Łu�h�N�h�N�v�Ɩ��ł悤�ȕ\��
// ============================================================
void Game::RenderLowHealthVignette()
{
    // �e�N�X�`�����ǂݍ��߂Ă��Ȃ���΃X�L�b�v
    if (!m_lowHealthVignetteSRV || !m_player) return;

    // === HP�䗦���v�Z ===
    int   hp = m_player->GetHealth();
    int   maxHp = 100;  // �ő�HP�ɍ��킹�ĕύX���Ă�
    float ratio = (float)hp / (float)maxHp;  // 1.0=���^��, 0.0=�m��

    // HP70%�ȏ�Ȃ牽���\�����Ȃ�
    if (ratio > 0.8f) return;

    // === ���x���v�Z�i0.0?1.0�j ===
    // HP70%��0.0�AHP0%��1.0
    float intensity = 1.0f - (ratio / 0.7f);
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    // === �S���p���X�iHP30%�ȉ��Ŕ����j ===
    float pulse = 1.0f;
    if (ratio < 0.3f)
    {
        // �S�����Y��: HP�Ⴂ�قǑ����i1.5?3.5Hz�j
        float heartRate = 1.5f + (1.0f - ratio / 0.3f) * 2.0f;

        // sin^8 �ŉs���u�h�N�b�v�p���X�����
        float t = sinf(m_accumulatedAnimTime * heartRate * 3.14159f);
        t = t * t;  // sin^2
        t = t * t;  // sin^4
        t = t * t;  // sin^8�i�s���X�p�C�N�j

        // �p���X�̐U�ꕝ�iHP�Ⴂ�قǑ傫���j
        float pulseStrength = 0.15f + (1.0f - ratio / 0.3f) * 0.4f;
        pulse = 1.0f + t * pulseStrength;
    }

    // === �ŏI�A���t�@ ===
    // intensity�i0?1�j�~ pulse�i1?1.5���炢�j
    float alpha = intensity * pulse;
    if (alpha > 1.0f) alpha = 1.0f;

    // === SpriteBatch�ŉ�ʑS�̂Ƀe�N�X�`����`�� ===
    auto context = m_d3dContext.Get();

    // �u�����h�X�e�[�g: �A���t�@�u�����h
    // �i�e�N�X�`���̃A���t�@ �~ ���_�J���[�̃A���t�@ �ō��������j
    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,   // �\�[�g����
        m_states->NonPremultiplied()         // // PNG�p�i�v���}���`�v���C�h����Ȃ��j
    );

    // ��ʑS�̂ɕ`��
    RECT destRect = { 0, 0, (LONG)m_outputWidth, (LONG)m_outputHeight };

    // �F�̒���:
    // - RGB�ŐԂ̔Z���𐧌�
    // - A�ŃA���t�@�i�s�����x�j�𐧌�
    DirectX::XMVECTORF32 tintColor = { {
        1.0f,          // R: ���̂܂�
        0.0f,          // G: �΂������i���Ԃ��j
        0.0f,          // B: ������
        alpha * 1.7f  // A: HP�ɘA���i�ő�85%�j
    } };

    m_spriteBatch->Draw(
        m_lowHealthVignetteSRV.Get(),  // �e�N�X�`��
        destRect,                       // �`���i��ʑS�́j
        tintColor                       // �F�ƃA���t�@
    );

    m_spriteBatch->End();
}

//  �X�s�[�h���C���`�� 

// ============================================================
//  RenderSpeedLines - �_�b�V�����̃X�s�[�h���C��
//
//  �y�\�������zm_speedLineAlpha > 0�i�_�b�V�����ɑ����j
//  �y�`������z��ʒ���������ˏ�Ƀ����_���Ȑ���`��
//  �^�������isin/cos�j�Ńt���[���Ԃ̗h�炬��\��
// ============================================================
void Game::RenderSpeedLines()
{
    if (m_speedLineAlpha <= 0.01f) return;

    auto context = m_d3dContext.Get();

    // 2D�`��Z�b�g�A�b�v�iRenderDamageFlash�Ɠ����p�^�[���j
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

    int lineCount = 28;  // ���ˏ�̐��̐�

    for (int i = 0; i < lineCount; i++)
    {
        // ���ˊp�x
        float angle = (float)i / lineCount * 6.283f;

        // �����_���Ȃ�炬�i���t���[���ς���ē����Ă銴���j
        float jitter = ((float)(rand() % 1000) / 1000.0f) * 0.15f;
        angle += jitter;

        float cosA = cosf(angle);
        float sinA = sinf(angle);

        // �O���̓_�i��ʒ[�̊O�܂ŐL�΂��j
        float outerDist = halfW * 1.3f;
        float outerX = cosA * outerDist;
        float outerY = sinA * outerDist;

        // �����̓_�i��ʒ����Ɍ������āj
        // �����_���Ȓ����Łu����Ă�v����
        float innerRatio = 0.45f + ((float)(rand() % 1000) / 1000.0f) * 0.25f;
        float innerX = cosA * outerDist * innerRatio;
        float innerY = sinA * outerDist * innerRatio;

        // ���̑����i�O���������A�������ׂ��j
        float thickness = 2.5f + ((float)(rand() % 1000) / 1000.0f) * 3.0f;
        float perpX = -sinA * thickness;
        float perpY = cosA * thickness;

        // �F�i���A�O���͕s�����A�����͓����j
        float lineAlpha = alpha * (0.3f + ((float)(rand() % 1000) / 1000.0f) * 0.5f);
        DirectX::XMFLOAT4 outerColor(1.0f, 1.0f, 1.0f, lineAlpha);
        DirectX::XMFLOAT4 innerColor(1.0f, 1.0f, 1.0f, 0.0f);

        // �O�p�`2����1�{�̃X�s�[�h���C����`��
        DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(outerX + perpX, outerY + perpY, 1.0f), outerColor);
        DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(outerX - perpX, outerY - perpY, 1.0f), outerColor);
        DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(innerX, innerY, 1.0f), innerColor);

        batch->DrawTriangle(v1, v2, v3);
    }

    batch->End();
}


// ============================================================
//  RenderDashOverlay - �_�b�V�����̉�ʌ��ʁi�F���ω��j
//
//  �y�\�������zm_dashOverlayAlpha > 0
//  �y�`������z��ʑS�̂�݂��������������ŃI�[�o�[���C
// ============================================================
void Game::RenderDashOverlay()
{
    // �e�N�X�`�����Ȃ�or�A���t�@0�Ȃ�`�悵�Ȃ�
    if (!m_dashSpeedlineSRV || m_dashOverlayAlpha <= 0.01f) return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    RECT destRect = { 0, 0, (LONG)m_outputWidth, (LONG)m_outputHeight };

    // ��]�����ăo���G�[�V�������o���i���t���[�������ɉ�]�j
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
//  RenderReloadWarning - �����[�h�x�� + �i���o�[
//
//  �y�\�������z�e�؂ꎞ or �����[�h��
//  �y�`����e�z
//  - "RELOADING" �e�L�X�g�i�_�Łj
//  - �i���o�[�i�Q�[�W��������E�ɐL�т�j
//  - �e�؂ꎞ�͐Ԃ� "NO AMMO" �x��
// ============================================================
void Game::RenderReloadWarning()
{
    // === �����[�h���̐i���o�[���\�� ===
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
    // "RELOAD" �x���e�L�X�g�i�e�؂ꎞ�j
    // =============================================
    if (m_reloadWarningAlpha > 0.01f && !isReloading)
    {
        // �����i�p���X�j: sin�Ŗ���
        float pulse = 0.5f + sinf(m_reloadWarningTimer * 6.0f) * 0.5f;
        float alpha = m_reloadWarningAlpha * (0.5f + pulse * 0.5f);

        // �X�P�[���e�݁i�o�ꎞ�Ƀo�E���X�j
        float scaleT = m_reloadWarningTimer;
        float scale = 1.0f;
        if (scaleT < 0.3f)
        {
            float t = scaleT / 0.3f;
            scale = 1.0f + sinf(t * 3.14159f) * 0.3f;  // �o�ꎞ��1.3�{��1.0�{
        }

        // --- ���C���e�L�X�g ---
        const wchar_t* text = L"[  R E L O A D  ]";

        DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(text);
        float textW = DirectX::XMVectorGetX(textSize) * scale;
        float textH = DirectX::XMVectorGetY(textSize) * scale;
        float textX = (W - textW) * 0.5f;
        float textY = H * 0.58f;

        // �O���[�i�e�Ƃ���2��`��j
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

        // ���C���e�L�X�g�i�ԁj
        DirectX::XMVECTORF32 mainColor = { 0.95f, 0.15f, 0.1f, alpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), text,
            DirectX::XMFLOAT2(textX, textY),
            mainColor, 0.0f,
            DirectX::XMFLOAT2(0, 0),
            scale);

        // --- �T�u�e�L�X�g ---
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
    //  �����[�h�i���o�[�i�����[�h���j
    // =============================================
    if (isReloading && m_whitePixel)
    {
        float progress = m_reloadAnimProgress;  // 0?1

        float barW = W * 0.25f;
        float barH = 4.0f;
        float barX = (W - barW) * 0.5f;
        float barY = H * 0.62f;

        // �w�i�i�Â��j
        RECT bgRect = {
            (LONG)barX, (LONG)barY,
            (LONG)(barX + barW), (LONG)(barY + barH)
        };
        DirectX::XMVECTORF32 bgColor = { 0.15f, 0.15f, 0.15f, 0.8f };
        m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

        // �i���i�����Ԃ̃O���f�[�V�����j
        float fillW = barW * progress;
        RECT fillRect = {
            (LONG)barX, (LONG)barY,
            (LONG)(barX + fillW), (LONG)(barY + barH)
        };
        // �����ɋ߂Â��قǔ����Ȃ�
        float r = 1.0f;
        float g = 0.3f + progress * 0.7f;
        float b = 0.3f + progress * 0.7f;
        DirectX::XMVECTORF32 fillColor = { r, g, b, 0.9f };
        m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, fillColor);

        // �e�L�X�g "RELOADING..."
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
//  RenderWaveBanner - �E�F�[�u�J�n�o�i�[
//
//  �y�\���^�C�~���O�z�V�����E�F�[�u���n�܂�����
//  �y���o�z�E���璆���ɃX���C�h�C�� �� ��莞�ԕ\�� �� �t�F�[�h�A�E�g
//  �m�[�}��/�~�b�h�{�X/�{�X�ŐF�ƃe�L�X�g���ς��
// ============================================================
void Game::RenderWaveBanner()
{
    if (m_waveBannerTimer <= 0.0f) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;
    float t = m_waveBannerTimer / m_waveBannerDuration;  // 1��0

    // �t�F�[�h�C��/�A�E�g
    float alpha = 1.0f;
    if (t > 0.85f) alpha = (1.0f - t) / 0.15f;      // �o��
    else if (t < 0.2f) alpha = t / 0.2f;              // ����

    // �X���C�h�C���i�E���璆���ցj
    float slideOffset = 0.0f;
    float slideT = 1.0f - t;
    if (slideT < 0.12f)
        slideOffset = (1.0f - slideT / 0.12f) * 300.0f;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // === �o�i�[�摜 ===
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

    // === �E�F�[�u�ԍ��i�傫���X���C�h�C���j ===
    if (m_fontLarge)
    {
        wchar_t numText[16];
        swprintf_s(numText, L"%d", m_waveBannerNumber);

        DirectX::XMVECTOR numSize = m_fontLarge->MeasureString(numText);
        float numW = DirectX::XMVectorGetX(numSize);
        float numX = (W - numW) * 0.5f + slideOffset + W * 0.16f;
        float numY = H * 0.5f - DirectX::XMVectorGetY(numSize) * 0.49f;

        // �O���[
        DirectX::XMVECTORF32 glowColor = m_waveBannerIsBoss
            ? DirectX::XMVECTORF32{ 0.4f, 0.02f, 0.0f, alpha * 0.6f }
        : DirectX::XMVECTORF32{ 0.4f, 0.35f, 0.0f, alpha * 0.6f };
        for (int dx = -3; dx <= 3; dx += 3)
            for (int dy = -3; dy <= 3; dy += 3)
                m_fontLarge->DrawString(m_spriteBatch.get(), numText,
                    DirectX::XMFLOAT2(numX + dx, numY + dy), glowColor);

        // ���C��
        DirectX::XMVECTORF32 numColor = m_waveBannerIsBoss
            ? DirectX::XMVECTORF32{ 1.0f, 0.2f, 0.1f, alpha }
        : DirectX::XMVECTORF32{ 1.0f, 0.85f, 0.2f, alpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), numText,
            DirectX::XMFLOAT2(numX, numY), numColor);
    }

    m_spriteBatch->End();
}



// ============================================================
//  RenderScoreHUD - �X�R�A�\���i�����Ԃ��w�i�t���j
//
//  �y�`��ʒu�z��ʍ���
//  �y�`����e�z�X�R�A���l + �����Ԃ��e�N�X�`���̔w�i
//  �w�i�e�N�X�`���������ꍇ�͐��l�̂ݕ\��
// ============================================================
void Game::RenderScoreHUD()
{
    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // === �����Ԃ��w�i ===
    if (m_scoreBackdropSRV)
    {
        float bgW = W * 0.22f;
        float bgH = H * 0.07f;
        float bgX = 10.0f;
        float bgY = 10.0f;

        // ���Z���ɂ�����Ɨh���
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

        // ���Z���ɏ������邭�Ȃ�
        float flashBright = 1.0f;
        if (m_scoreFlashTimer > 0.0f)
            flashBright = 1.0f + (m_scoreFlashTimer / 0.4f) * 0.8f;

        DirectX::XMVECTORF32 bgColor = {
            flashBright, flashBright, flashBright, 0.9f
        };
        m_spriteBatch->Draw(m_scoreBackdropSRV.Get(), bgRect, bgColor);

        // === �X�R�A���� ===
        if (m_fontLarge)
        {
            wchar_t scoreText[32];
            swprintf_s(scoreText, L"%d", (int)m_scoreDisplayValue);

            DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(scoreText);
            float textH = DirectX::XMVectorGetY(textSize);
            float textX = bgX + 15.0f + shakeX;
            float textY = bgY + (bgH - textH) * 0.5f + shakeY;

            // �O���[�i�Ԃ��e�j
            DirectX::XMVECTORF32 glowColor = { 0.5f, 0.02f, 0.0f, 0.6f };
            for (int dx = -2; dx <= 2; dx += 4)
                for (int dy = -2; dy <= 2; dy += 4)
                    m_fontLarge->DrawString(m_spriteBatch.get(), scoreText,
                        DirectX::XMFLOAT2(textX + dx, textY + dy), glowColor);

            // ���C���i���Z���͐ԁ����Ɍ���j
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

        // === "PTS" ���x���i�������E�Ɂj ===
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
//  RenderClawDamage - �܍U���̃X�N���b�`��
//
//  �y�\�������z�G�̋ߐڍU��������������
//  �y�`������z�X�N���b�`�e�N�X�`������ʂɃI�[�o�[���C
//  ���Ԍo�߂Ńt�F�[�h�A�E�g
// ============================================================
void Game::RenderClawDamage()
{
    if (m_clawTimer <= 0.0f || !m_clawDamageSRV) return;

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;
    float t = m_clawTimer / m_clawDuration;  // 1��0

    // �X���b�V���A�j���[�V����: �E�ォ��E���փX���C�h
    // t=1(�J�n): ��ʊO�̏� �� t=0.5: ��ʒ��� �� t=0: ��ʉ��Ńt�F�[�h�A�E�g
    float slideProgress = 1.0f - t;  // 0��1

    // �e�N�X�`���T�C�Y
    float clawW = W * 0.5f;
    float clawH = H * 1.2f;

    // X�ʒu: ��ʉE���
    float clawX = (W - clawW) * 0.5f;

    // Y�ʒu: �ォ�牺�փX���C�h�i�����ň������������j
    float startY = -clawH * 0.8f;   // ��ʏ�̊O
    float endY = H * 0.1f;          // ��ʒ����t��
    float clawY;

    if (slideProgress < 0.15f)
    {
        // �ŏ���15%: �����X���b�V���i�と�����j
        float p = slideProgress / 0.15f;
        p = p * p * (3.0f - 2.0f * p);  // �X���[�Y�X�e�b�v
        clawY = startY + (endY - startY) * p;
    }
    else
    {
        // �c��: �����ɗ��܂��ăt�F�[�h�A�E�g
        clawY = endY;
    }

    // �A���t�@: ��C�ɏo�āA������Ə�����
    float alpha;
    if (slideProgress < 0.15f)
        alpha = slideProgress / 0.15f;          // �t�F�[�h�C��
    else if (slideProgress < 0.4f)
        alpha = 1.0f;                            // �t���\��
    else
        alpha = 1.0f - (slideProgress - 0.4f) / 0.6f;  // �t�F�[�h�A�E�g

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
//  RenderGloryKillFlash - �O���[���[�L�����̔��t���b�V��
//
//  �y�\�������z�O���[���[�L���������i0.3�b�ԁj
//  �y�`������z��ʑS�̂𔒂��t���b�V�� �� �}���Ƀt�F�[�h�A�E�g
//  �u�~�߂��h�����v�C���p�N�g�����o�I�ɋ���
// ============================================================
void Game::RenderGloryKillFlash()
{
    return;

    // �O���[���[�L���t���b�V���������Ȃ牽�����Ȃ�
    if (m_gloryKillFlashTimer <= 0.0f || m_gloryKillFlashAlpha <= 0.0f)
        return;

    auto context = m_d3dContext.Get();

    // 2D�`��p�̐ݒ�
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // PrimitiveBatch���쐬
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // �����t���b�V���̐F�i�����x��m_gloryKillFlashAlpha�j
    DirectX::XMFLOAT4 flashColor(1.0f, 1.0f, 1.0f, m_gloryKillFlashAlpha * 0.01f);

    // ��ʂ̃T�C�Y�i���S����̋����j
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // ��ʑS�̂𕢂��l�p�`
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), flashColor);   // ����
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), flashColor);    // �E��
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, -halfHeight, 1.0f), flashColor);   // �E��
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(-halfWidth, -halfHeight, 1.0f), flashColor);  // ����

    // �l�p�`��`��
    primitiveBatch->DrawQuad(v1, v2, v3, v4);

    primitiveBatch->End();
}