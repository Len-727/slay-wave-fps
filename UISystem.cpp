// UISystem.cpp - UI描画システムの実装
#include "UISystem.h"
#include "Player.h"
#include "WeaponSystem.h"
#include "WaveManager.h"
#include <vector>
#include <string>

// コンストラクタ
UISystem::UISystem(int screenWidth, int screenHeight) :
    m_screenWidth(screenWidth),
    m_screenHeight(screenHeight)
{
}

// 画面サイズ変更
void UISystem::OnScreenSizeChanged(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;
}

// 全UI要素を描画
void UISystem::DrawAll(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    const Player* player,
    const WeaponSystem* weaponSystem,
    const WaveManager* waveManager)
{
    // NULLチェック（安全のため）
    if (!batch || !player || !weaponSystem || !waveManager)
        return;

    // 各UI要素を描画
    //DrawHealthBar(batch, player->GetHealth());
    DrawCrosshair(batch);
    DrawWaveNumber(batch, waveManager->GetCurrentWave());
    /*DrawPoints(batch, player->GetPoints());*/
    DrawAmmo(batch, weaponSystem->GetCurrentAmmo(),
        weaponSystem->GetReserveAmmo(),
        weaponSystem->IsReloading());
    DrawWeaponNumber(batch, (int)weaponSystem->GetCurrentWeapon() + 1);
}

// 体力バー描画（左下）
void UISystem::DrawHealthBar(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int health)
{
    float barWidth = 200.0f;
    float barHeight = 20.0f;
    float padding = 50.0f;
    float startX = padding;
    float startY = m_screenHeight - padding - barHeight;

    // 背景（暗い灰色）
    DirectX::XMFLOAT4 bgColor(0.2f, 0.2f, 0.2f, 0.8f);
    for (float i = 0; i < barHeight; ++i)
    {
        batch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + i, 1.0f), bgColor),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY + i, 1.0f), bgColor)
        );
    }

    // HP部分（色が変わる）
    float healthPercent = (float)health / 100.0f;
    float currentBarWidth = barWidth * healthPercent;

    if (currentBarWidth > 0)
    {
        DirectX::XMFLOAT4 healthColor;
        if (healthPercent > 0.6f)
            healthColor = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);  // 緑
        else if (healthPercent > 0.3f)
            healthColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);  // 黄
        else
            healthColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);  // 赤

        for (float i = 0; i < barHeight; ++i)
        {
            batch->DrawLine(
                DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + i, 1.0f), healthColor),
                DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + currentBarWidth, startY + i, 1.0f), healthColor)
            );
        }
    }

    // 枠線（白）
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

// クロスヘア描画（中央）
void UISystem::DrawCrosshair(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch)
{
    DirectX::XMFLOAT4 crosshairColor(1.0f, 1.0f, 1.0f, 1.0f);
    float centerX = m_screenWidth / 2.0f;
    float centerY = m_screenHeight / 2.0f;
    float size = 20.0f;

    // 縦線
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX, centerY - size, 1.0f), crosshairColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX, centerY + size, 1.0f), crosshairColor)
    );

    // 横線
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX - size, centerY, 1.0f), crosshairColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX + size, centerY, 1.0f), crosshairColor)
    );
}

// ウェーブ番号描画（上中央）
void UISystem::DrawWaveNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int wave)
{
    DirectX::XMFLOAT4 color(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色
    float digitWidth = 15.0f;
    float digitSpacing = 20.0f;

    if (wave == 0)
    {
        DrawSimpleNumber(batch, 0, (m_screenWidth - digitWidth) / 2.0f, 50.0f, color);
    }
    else
    {
        // 桁を分解
        std::vector<int> digits;
        int temp = wave;
        while (temp > 0)
        {
            digits.push_back(temp % 10);
            temp /= 10;
        }

        // 中央揃え
        int numDigits = digits.size();
        float totalWidth = numDigits * digitWidth + (numDigits - 1) * (digitSpacing - digitWidth);
        float startX = (m_screenWidth - totalWidth) / 2.0f;
        float startY = 50.0f;

        // 逆順で描画（桁を正しい順序に）
        for (int i = 0; i < numDigits; ++i)
        {
            DrawSimpleNumber(batch, digits[numDigits - 1 - i],
                startX + i * digitSpacing, startY, color);
        }
    }
}

// ポイント描画（右上）
void UISystem::DrawPoints(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int points)
{
    DirectX::XMFLOAT4 color(0.1f, 1.0f, 1.0f, 1.0f);  // シアン
    float digitWidth = 15.0f;
    float digitSpacing = 20.0f;
    float padding = 50.0f;

    if (points == 0)
    {
        DrawSimpleNumber(batch, 0, m_screenWidth - padding - digitWidth, padding, color);
    }
    else
    {
        // 桁を分解
        std::vector<int> digits;
        int temp = points;
        while (temp > 0)
        {
            digits.push_back(temp % 10);
            temp /= 10;
        }

        // 右揃え
        int numDigits = digits.size();
        float totalWidth = numDigits * digitWidth + (numDigits - 1) * (digitSpacing - digitWidth);
        float startX = m_screenWidth - padding - totalWidth;
        float startY = padding;

        // 逆順で描画
        for (int i = 0; i < numDigits; ++i)
        {
            DrawSimpleNumber(batch, digits[numDigits - 1 - i],
                startX + i * digitSpacing, startY, color);
        }
    }
}

// 弾薬描画（右下）
void UISystem::DrawAmmo(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    int currentAmmo, int reserveAmmo, bool isReloading)
{
    DirectX::XMFLOAT4 normalColor(1.0f, 1.0f, 1.0f, 1.0f);     // 白
    DirectX::XMFLOAT4 reloadColor(1.0f, 0.2f, 0.2f, 1.0f);     // 赤

    float digitHeight = 25.0f;
    float digitWidth = 15.0f;
    float digitSpacing = 20.0f;
    float separatorWidth = 20.0f;
    float padding = 50.0f;

    // リロード中なら赤色
    DirectX::XMFLOAT4 currentColor = isReloading ? reloadColor : normalColor;

    // 数字を文字列に変換
    std::string currentAmmoStr = std::to_string(currentAmmo);
    std::string reserveAmmoStr = std::to_string(reserveAmmo);

    // 位置計算
    float currentWidth = currentAmmoStr.length() * digitWidth +
        (currentAmmoStr.length() - 1) * (digitSpacing - digitWidth);
    float reserveWidth = reserveAmmoStr.length() * digitWidth +
        (reserveAmmoStr.length() - 1) * (digitSpacing - digitWidth);
    float totalWidth = currentWidth + separatorWidth + reserveWidth;
    float startX = m_screenWidth - padding - totalWidth;
    float startY = m_screenHeight - padding - digitHeight;

    // 現在弾数を描画（リロード中なら赤）
    float currentX = startX;
    for (char c : currentAmmoStr)
    {
        DrawSimpleNumber(batch, c - '0', currentX, startY, currentColor);
        currentX += digitSpacing;
    }

    // スラッシュ（白）
    currentX += (separatorWidth - digitSpacing) / 2;
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(currentX, startY + digitHeight, 1.0f), normalColor),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(currentX + 10.0f, startY, 1.0f), normalColor)
    );

    // 予備弾数を描画（常に白）
    currentX += separatorWidth - (separatorWidth - digitSpacing) / 2;
    for (char c : reserveAmmoStr)
    {
        DrawSimpleNumber(batch, c - '0', currentX, startY, normalColor);
        currentX += digitSpacing;
    }
}

// 武器番号描画（中央下）
void UISystem::DrawWeaponNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int weaponNum)
{
    DirectX::XMFLOAT4 weaponColor(1.0f, 1.0f, 1.0f, 1.0f);
    float centerX = m_screenWidth / 2.0f;
    float bottomY = m_screenHeight - 120.0f;

    DrawSimpleNumber(batch, weaponNum, centerX - 30, bottomY, weaponColor);
}

// 数字描画ヘルパー
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

    // === 画面中央下部に表示 ===
    float centerX = m_screenWidth * 0.5f;
    float promptY = m_screenHeight * 0.7f;

    // === 背景（半透明の黒）===
    float bgWidth = 300.0f;
    float bgHeight = 60.0f;
    DirectX::XMFLOAT4 bgColor(0.0f, 0.0f, 0.0f, 0.7f);

    DrawBox(batch, centerX - bgWidth / 2, promptY - bgHeight / 2, bgWidth, bgHeight, bgColor);

    // === 購入可能/不可の色 ===
    DirectX::XMFLOAT4 textColor;
    if (alreadyOwned)
    {
        // 弾薬補充
        int ammoCost = weaponSpawn->cost / 2;
        textColor = (playerPoints >= ammoCost) ?
            DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) :  // 緑（購入可）
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);   // 赤（お金不足）
    }
    else
    {
        // 武器購入
        textColor = (playerPoints >= weaponSpawn->cost) ?
            DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) :
            DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    // 枠線（色付き）
    DrawBoxOutline(batch, centerX - bgWidth / 2, promptY - bgHeight / 2, bgWidth, bgHeight, textColor);
}

void UISystem::DrawBox(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    float x, float y, float width, float height, DirectX::XMFLOAT4 color)
{
    // 【役割】四角形を塗りつぶす（横線を何本も引く）
    // 【引数】x, y: 左上座標, width, height: サイズ

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
    // 【役割】四角形の枠線だけを描画（4本の線）
    // 【引数】x, y: 左上座標, width, height: サイズ

    // 上の線
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y, 1.0f), color)
    );

    // 下の線
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y + height, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y + height, 1.0f), color)
    );

    // 左の線
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, y + height, 1.0f), color)
    );

    // 右の線
    batch->DrawLine(
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y, 1.0f), color),
        DirectX::VertexPositionColor(DirectX::XMFLOAT3(x + width, y + height, 1.0f), color)
    );
}