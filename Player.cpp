// Player.cpp - プレイヤー状態管理の実装
#include "Player.h"
#include <cmath>  // sinf, cosf

// コンストラクタ
// 【役割】初期位置、HP100、ポイント500で開始
Player::Player() :
    m_position(0.0f, 1.8f, -0.5f),   // 初期位置
    m_rotation(0.0f, 0.0f, 0.0f),    // 正面を向く
    m_health(100),
    m_points(500),
    m_isDamaged(false),
    m_damageTimer(0.0f),
    m_mouseCaptured(true),
    m_firstMouse(true),
    m_lastMouseX(0),
    m_lastMouseY(0),
    m_meleeAttackCooldown(0.0f)
{
}

// Update - メイン更新処理
void Player::Update(HWND window)
{
    //  初回フレームでカーソルを消す
    static bool firstFrame = true;
    if (firstFrame && m_mouseCaptured)
    {
        ShowCursor(FALSE);
        firstFrame = false;
    }

    // 移動処理
    UpdateMovement();

    // マウス視点回転
    UpdateMouseLook(window);

    // マウス固定の切り替え
    UpdateMouseCapture(window);

    // ダメージタイマー更新
    if (m_damageTimer > 0.0f)
    {
        m_damageTimer -= 1.0f / 60.0f;
        if (m_damageTimer <= 0.0f)
        {
            m_isDamaged = false;
        }
    }

    //  近接攻撃のクールダウンを更新
    UpdateMeleeAttackCooldown(1.0f / 60.0f);
}

//  AddCameraRecoil -   カメラリコイル(反動)
void Player::AddCameraRecoil(float pitchRecoil, float yawRecoil)
{
    //  Pitch(上下)の反動
    m_rotation.x -= pitchRecoil;    //  上向きに跳ね上がる
    
    //  上下の制限(真上・真下を向きすぎないように)
    const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;   //  90度 - 1
    if (m_rotation.x < -maxPitch) m_rotation.x = -maxPitch;
    if (m_rotation.x > maxPitch) m_rotation.x = maxPitch;

    //  Yaw(左右)の反動(ランダムに左右に揺れる)
    float randomYaw = ((float)rand() / RAND_MAX - 0.5f) * yawRecoil;
    m_rotation.y += randomYaw;
}

// UpdateMovement - 移動処理
void Player::UpdateMovement()
{
    float moveSpeed = 0.1f;

    //  シフトでダッシュ（1.8倍速）
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        moveSpeed = 0.13f;

    // W - 前進
    if (GetAsyncKeyState('W') & 0x8000)
    {
        float forwardX = sinf(m_rotation.y);
        float forwardZ = cosf(m_rotation.y);
        m_position.x += forwardX * moveSpeed;
        m_position.z += forwardZ * moveSpeed;
    }

    // S - 後退
    if (GetAsyncKeyState('S') & 0x8000)
    {
        float forwardX = sinf(m_rotation.y);
        float forwardZ = cosf(m_rotation.y);
        m_position.x -= forwardX * moveSpeed;
        m_position.z -= forwardZ * moveSpeed;
    }

    // A - 左移動
    if (GetAsyncKeyState('A') & 0x8000)
    {
        float leftX = sinf(m_rotation.y - 1.57f);  // 1.57 ? π/2
        float leftZ = cosf(m_rotation.y - 1.57f);
        m_position.x += leftX * moveSpeed;
        m_position.z += leftZ * moveSpeed;
    }

    // D - 右移動
    if (GetAsyncKeyState('D') & 0x8000)
    {
        float rightX = sinf(m_rotation.y + 1.57f);
        float rightZ = cosf(m_rotation.y + 1.57f);
        m_position.x += rightX * moveSpeed;
        m_position.z += rightZ * moveSpeed;
    }

    //  目線の高さ
    float eyeHeight = 1.8f;

    // もし高さが目線より下がっていたら、強制的に持ち上げる（床判定）
    // (重力処理を入れたときに、これがないと無限に落ちていく)
    if (m_position.y < eyeHeight)
    {
        m_position.y = eyeHeight;
    }

}

// UpdateMouseLook - マウス視点回転
void Player::UpdateMouseLook(HWND window)
{
    if (!m_mouseCaptured)
        return;

    // マウス位置取得
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(window, &mousePos);

    if (!m_firstMouse)
    {
        // マウスの移動量を計算
        int deltaX = mousePos.x - m_lastMouseX;
        int deltaY = mousePos.y - m_lastMouseY;

        // 回転速度を更新
        float mouseSensitivity = 0.002f;
        m_rotation.y += deltaX * mouseSensitivity;  // 左右回転
        m_rotation.x += deltaY * mouseSensitivity;  // 上下回転

        // 上下方向の回転を制限する（真上・真下まで行き過ぎないようにする）
        float maxPitch = 1.5f; // だいたい 85度 くらい（ラジアン）
        if (m_rotation.x > maxPitch)
        {
            m_rotation.x = maxPitch;
        }
        if (m_rotation.x < -maxPitch)
        {
            m_rotation.x = -maxPitch;
        }
    }
    else
    {
        m_firstMouse = false;
    }

    // マウスを画面中央に戻す
    RECT rect;
    GetClientRect(window, &rect);
    m_lastMouseX = rect.right / 2;
    m_lastMouseY = rect.bottom / 2;

    POINT center = { m_lastMouseX, m_lastMouseY };
    ClientToScreen(window, &center);
    SetCursorPos(center.x, center.y);
}

// UpdateMouseCapture - マウス固定の切り替え
void Player::UpdateMouseCapture(HWND window)
{
    static bool tabPressed = false;

    if (GetAsyncKeyState(VK_F2) & 0x8000)
    {
        if (!tabPressed)
        {
            m_mouseCaptured = !m_mouseCaptured;

            if (m_mouseCaptured)
            {
                ShowCursor(FALSE);  // カーソル非表示

                // ウィンドウ中央にマウスを移動
                RECT rect;
                GetClientRect(window, &rect);
                POINT center = { rect.right / 2, rect.bottom / 2 };
                ClientToScreen(window, &center);
                SetCursorPos(center.x, center.y);
            }
            else
            {
                ShowCursor(TRUE);  // カーソル表示
            }

            tabPressed = true;
        }
    }
    else
    {
        tabPressed = false;
    }
}

void Player::Draw(
    ID3D11DeviceContext* context,
    DirectX::XMMATRIX view,
    DirectX::XMMATRIX projection)
{
   
}

// TakeDamage - ダメージを受ける
bool Player::TakeDamage(int damage)
{
    //return false;


    // 無敵時間中はダメージなし
    if (m_damageTimer > 0.0f)
        return false;

    m_health -= damage;
    m_damageTimer = 1.0f;  // 1秒間の無敵時間
    m_isDamaged = true;

    // 死亡判定
    if (m_health <= 0)
    {
        m_health = 0;
        return true;  // 死んだ
    }

    return false;  // まだ生きている
}

// AddPoints - ポイント追加
void Player::AddPoints(int points)
{
    m_points += points;
}