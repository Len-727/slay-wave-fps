// UISystem.cpp - UI�`��V�X�e���̎���
#include "UISystem.h"
#include "Player.h"
#include "WeaponSystem.h"
#include "WaveManager.h"
#include <vector>
#include <string>

// �R���X�g���N�^
UISystem::UISystem(int screenWidth, int screenHeight) :
    m_screenWidth(screenWidth),
    m_screenHeight(screenHeight)
{
}

// ��ʃT�C�Y�ύX
void UISystem::OnScreenSizeChanged(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;
}

// �SUI�v�f��`��
void UISystem::DrawAll(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    const Player* player,
    const WeaponSystem* weaponSystem,
    const WaveManager* waveManager)
{
    // NULL�`�F�b�N�i���S�̂��߁j
    if (!batch || !player || !weaponSystem || !waveManager)
        return;

    // �eUI�v�f��`��
    //DrawHealthBar(batch, player->GetHealth());
    DrawCrosshair(batch);
    //DrawWaveNumber(batch, waveManager->GetCurrentWave());
    /*DrawPoints(batch, player->GetPoints());*/
    DrawAmmo(batch, weaponSystem->GetCurrentAmmo(),
        weaponSystem->GetReserveAmmo(),
        weaponSystem->IsReloading());
    DrawWeaponNumber(batch, (int)weaponSystem->GetCurrentWeapon() + 1);
}

// �̗̓o�[�`��i�����j
void UISystem::DrawHealthBar(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int health)
{
    float barWidth = 200.0f;
    float barHeight = 20.0f;
    float padding = 50.0f;
    float startX = padding;
    float startY = m_screenHeight - padding - barHeight;

    // �w�i�i�Â��D�F�j
    DirectX::XMFLOAT4 bgColor(0.2f, 0.2f, 0.2f, 0.8f);
    for (float i = 0; i < barHeight; ++i)
    {
        batch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + i, 1.0f), bgColor),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY + i, 1.0f), bgColor)
        );
    }

    // HP�����i�F���ς��j
    float healthPercent = (float)health / 100.0f;
    float currentBarWidth = barWidth * healthPercent;

    if (currentBarWidth > 0)
    {
        DirectX::XMFLOAT4 healthColor;
        if (healthPercent > 0.6f)
            healthColor = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);  // ��
        else if (healthPercent > 0.3f)
            healthColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);  // ��
        else
            healthColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);  // ��

        for (float i = 0; i < barHeight; ++i)
        {
            batch->DrawLine(
                DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + i, 1.0f), healthColor),
                DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + currentBarWidth, startY + i, 1.0f), healthColor)
            );
        }
    }

    // �g���i���j
    DirectX::XMFLOAT4 borderColor(1.0f, 1.0f, 1.0f, 1.0f);
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY, 1.0f), borderColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY, 1.0f), borderColor)
    );
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + barHeight, 1.0f), borderColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY + barHeight, 1.0f), borderColor)
    );
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY, 1.0f), borderColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + barHeight, 1.0f), borderColor)
    );
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY, 1.0f), borderColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY + barHeight, 1.0f), borderColor)
    );
}

// �N���X�w�A�`��i�����j
void UISystem::DrawCrosshair(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch)
{
    DirectX::XMFLOAT4 crosshairColor(1.0f, 1.0f, 1.0f, 1.0f);
    float centerX = m_screenWidth / 2.0f;
    float centerY = m_screenHeight / 2.0f;
    float size = 20.0f;

    // �c��
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX, centerY - size, 1.0f), crosshairColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX, centerY + size, 1.0f), crosshairColor)
    );

    // ����
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX - size, centerY, 1.0f), crosshairColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX + size, centerY, 1.0f), crosshairColor)
    );
}

// �E�F�[�u�ԍ��`��i�㒆���j
void UISystem::DrawWaveNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int wave)
{
    DirectX::XMFLOAT4 color(1.0f, 1.0f, 0.0f, 1.0f);  // ���F
    float digitWidth = 15.0f;
    float digitSpacing = 20.0f;

    if (wave == 0)
    {
        DrawSimpleNumber(batch, 0, (m_screenWidth - digitWidth) / 2.0f, 50.0f, color);
    }
    else
    {
        // ���𕪉�
        std::vector<int> digits;
        int temp = wave;
        while (temp > 0)
        {
            digits.push_back(temp % 10);
            temp /= 10;
        }

        // ��������
        int numDigits = digits.size();
        float totalWidth = numDigits * digitWidth + (numDigits - 1) * (digitSpacing - digitWidth);
        float startX = (m_screenWidth - totalWidth) / 2.0f;
        float startY = 50.0f;

        // �t���ŕ`��i���𐳂��������Ɂj
        for (int i = 0; i < numDigits; ++i)
        {
            DrawSimpleNumber(batch, digits[numDigits - 1 - i],
                startX + i * digitSpacing, startY, color);
        }
    }
}

// �|�C���g�`��i�E��j
void UISystem::DrawPoints(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int points)
{
    DirectX::XMFLOAT4 color(0.1f, 1.0f, 1.0f, 1.0f);  // �V�A��
    float digitWidth = 15.0f;
    float digitSpacing = 20.0f;
    float padding = 50.0f;

    if (points == 0)
    {
        DrawSimpleNumber(batch, 0, m_screenWidth - padding - digitWidth, padding, color);
    }
    else
    {
        // ���𕪉�
        std::vector<int> digits;
        int temp = points;
        while (temp > 0)
        {
            digits.push_back(temp % 10);
            temp /= 10;
        }

        // �E����
        int numDigits = digits.size();
        float totalWidth = numDigits * digitWidth + (numDigits - 1) * (digitSpacing - digitWidth);
        float startX = m_screenWidth - padding - totalWidth;
        float startY = padding;

        // �t���ŕ`��
        for (int i = 0; i < numDigits; ++i)
        {
            DrawSimpleNumber(batch, digits[numDigits - 1 - i],
                startX + i * digitSpacing, startY, color);
        }
    }
}

// �e��`��i�E���j
void UISystem::DrawAmmo(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    int currentAmmo, int reserveAmmo, bool isReloading)
{
    DirectX::XMFLOAT4 normalColor(1.0f, 1.0f, 1.0f, 1.0f);     // ��
    DirectX::XMFLOAT4 reloadColor(1.0f, 0.2f, 0.2f, 1.0f);     // ��

    float digitHeight = 25.0f;
    float digitWidth = 15.0f;
    float digitSpacing = 20.0f;
    float separatorWidth = 20.0f;
    float padding = 50.0f;

    // �����[�h���Ȃ�ԐF
    DirectX::XMFLOAT4 currentColor = isReloading ? reloadColor : normalColor;

    // �����𕶎���ɕϊ�
    std::string currentAmmoStr = std::to_string(currentAmmo);
    std::string reserveAmmoStr = std::to_string(reserveAmmo);

    // �ʒu�v�Z
    float currentWidth = currentAmmoStr.length() * digitWidth +
        (currentAmmoStr.length() - 1) * (digitSpacing - digitWidth);
    float reserveWidth = reserveAmmoStr.length() * digitWidth +
        (reserveAmmoStr.length() - 1) * (digitSpacing - digitWidth);
    float totalWidth = currentWidth + separatorWidth + reserveWidth;
    float startX = m_screenWidth - padding - totalWidth;
    float startY = m_screenHeight - padding - digitHeight;

    // ���ݒe����`��i�����[�h���Ȃ�ԁj
    float currentX = startX;
    for (char c : currentAmmoStr)
    {
        DrawSimpleNumber(batch, c - '0', currentX, startY, currentColor);
        currentX += digitSpacing;
    }

    // �X���b�V���i���j
    currentX += (separatorWidth - digitSpacing) / 2;
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(currentX, startY + digitHeight, 1.0f), normalColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(currentX + 10.0f, startY, 1.0f), normalColor)
    );

    // �\���e����`��i��ɔ��j
    currentX += separatorWidth - (separatorWidth - digitSpacing) / 2;
    for (char c : reserveAmmoStr)
    {
        DrawSimpleNumber(batch, c - '0', currentX, startY, normalColor);
        currentX += digitSpacing;
    }
}

// ����ԍ��`��i�������j
void UISystem::DrawWeaponNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int weaponNum)
{
    DirectX::XMFLOAT4 weaponColor(1.0f, 1.0f, 1.0f, 1.0f);
    float centerX = m_screenWidth / 2.0f;
    float bottomY = m_screenHeight - 120.0f;

    DrawSimpleNumber(batch, weaponNum, centerX - 30, bottomY, weaponColor);
}

// �����`��w���p�[
void UISystem::DrawSimpleNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    int digit, float x, float y, DirectX::XMFLOAT4 color)
{
    float w = 15.0f;
    float h = 25.0f;

    auto DrawThickLine = [&](float x1, float y1, float x2, float y2)
        {
            batch->DrawLine(
                DirectX::VertexPositionColor(DirectX::XMFLOAT3(x1, y1, 1.0f), color),
                DirectX::VertexPositionColor(DirectX::XMFLOAT3(x2, y2, 1.0f), color)
            );
        };

    switch (digit)
    {
    case 0:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x, y, x, y + h);
        DrawThickLine(x + w, y, x + w, y + h);
        DrawThickLine(x, y + h, x + w, y + h);
        break;
    case 1:
        DrawThickLine(x + w, y, x + w, y + h);
        break;
    case 2:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x + w, y, x + w, y + h / 2);
        DrawThickLine(x, y + h / 2, x + w, y + h / 2);
        DrawThickLine(x, y + h / 2, x, y + h);
        DrawThickLine(x, y + h, x + w, y + h);
        break;
    case 3:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x + w, y, x + w, y + h);
        DrawThickLine(x, y + h / 2, x + w, y + h / 2);
        DrawThickLine(x, y + h, x + w, y + h);
        break;
    case 4:
        DrawThickLine(x, y, x, y + h / 2);
        DrawThickLine(x, y + h / 2, x + w, y + h / 2);
        DrawThickLine(x + w, y, x + w, y + h);
        break;
    case 5:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x, y, x, y + h / 2);
        DrawThickLine(x, y + h / 2, x + w, y + h / 2);
        DrawThickLine(x + w, y + h / 2, x + w, y + h);
        DrawThickLine(x, y + h, x + w, y + h);
        break;
    case 6:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x, y, x, y + h);
        DrawThickLine(x, y + h / 2, x + w, y + h / 2);
        DrawThickLine(x + w, y + h / 2, x + w, y + h);
        DrawThickLine(x, y + h, x + w, y + h);
        break;
    case 7:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x + w, y, x + w, y + h);
        break;
    case 8:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x, y, x, y + h);
        DrawThickLine(x + w, y, x + w, y + h);
        DrawThickLine(x, y + h / 2, x + w, y + h / 2);
        DrawThickLine(x, y + h, x + w, y + h);
        break;
    case 9:
        DrawThickLine(x, y, x + w, y);
        DrawThickLine(x, y, x, y + h / 2);
        DrawThickLine(x + w, y, x + w, y + h);
        DrawThickLine(x, y + h / 2, x + w, y + h / 2);
        DrawThickLine(x, y + h, x + w, y + h);
        break;
    }
}

void UISystem::DrawWeaponPrompt(
    DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    WeaponSpawn* weaponSpawn,
    int playerPoints,
    bool alreadyOwned)
{
    if (!weaponSpawn)
        return;

    // === ��ʒ��������ɕ\�� ===
    float centerX = m_screenWidth * 0.5f;
    float promptY = m_screenHeight * 0.7f;

    // === �w�i�i�������̍��j===
    float bgWidth = 300.0f;
    float bgHeight = 60.0f;
    DirectX::XMFLOAT4 bgColor(0.0f, 0.0f, 0.0f, 0.7f);

    DrawBox(batch, centerX - bgWidth / 2, promptY - bgHeight / 2, bgWidth, bgHeight, bgColor);

    // === �w���\/�s�̐F ===
    DirectX::XMFLOAT4 textColor;
    if (alreadyOwned)
    {
        // �e���[
        int ammoCost = weaponSpawn->cost / 2;
        textColor = (playerPoints >= ammoCost) ?
            DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) :  // �΁i�w���j
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);   // �ԁi�����s���j
    }
    else
    {
        // ����w��
        textColor = (playerPoints >= weaponSpawn->cost) ?
            DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) :
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    // �g���i�F�t���j
    DrawBoxOutline(batch, centerX - bgWidth / 2, promptY - bgHeight / 2, bgWidth, bgHeight, textColor);
}

void UISystem::DrawBox(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    float x, float y, float width, float height, DirectX::XMFLOAT4 color)
{
    // �y�����z�l�p�`��h��Ԃ��i���������{�������j
    // �y�����zx, y: ������W, width, height: �T�C�Y

    for (float i = 0; i < height; ++i)
    {
        batch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y + i, 1.0f), color),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y + i, 1.0f), color)
        );
    }
}

void UISystem::DrawBoxOutline(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    float x, float y, float width, float height, DirectX::XMFLOAT4 color)
{
    // �y�����z�l�p�`�̘g��������`��i4�{�̐��j
    // �y�����zx, y: ������W, width, height: �T�C�Y

    // ��̐�
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y, 1.0f), color)
    );

    // ���̐�
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y + height, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y + height, 1.0f), color)
    );

    // ���̐�
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y + height, 1.0f), color)
    );

    // �E�̐�
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y + height, 1.0f), color)
    );
}