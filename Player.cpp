// Player.cpp - プレイヤー状態管理の実装
#include "Player.h"
#include <cmath>  // sinf, cosf

// コンストラクタ
// 【役割】初期位置、HP100、ポイント500で開始
Player::Player() :
    m_position(0.0f, EYE_HEIGHT, -0.5f),   // 初期位置
    m_rotation(0.0f, 0.0f, 0.0f),    // 正面を向く
    m_health(100),
    m_points(500),
    m_isDamaged(false),
    m_damageTimer(0.0f),
    m_mouseCaptured(true),
    m_firstMouse(true),
    m_lastMouseX(0),
    m_lastMouseY(0),
    m_meleeAttackCooldown(0.0f),
    m_velocityY(0.0f),          // 初期速度ゼロ（落下していない）
    m_isGrounded(true),         // 最初は地面の上にいる
    m_coyoteTimer(0.0f),        // 猶予なし（地面にいるから不要）
    m_jumpBufferTimer(0.0f),     // バッファなし
    m_landingCameraOffset(0.0f),
    m_wasGroundedLastFrame(true),
    m_lastFallSpeed(0.0f)
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

    // === 着地演出の更新 ===
    {
        const float deltaTime = 1.0f / 60.0f;  // TODO: 外部から受け取る

        // 空中にいる間、落下速度を記録し続ける
        // （Land()でvelocityYが0にリセットされるので、着地後は取れない）
        if (!m_isGrounded)
        {
            // velocityYが負（落下中）なら速度を記録
            if (m_velocityY < 0.0f)
                m_lastFallSpeed = -m_velocityY;  // 正の値に変換して保存
        }

        // 着地した瞬間を検出（前フレーム空中 → 今フレーム接地）
        if (m_isGrounded && !m_wasGroundedLastFrame)
        {
            // 落下速度に応じて沈み量を計算
            // 速度0 → DIP_MIN、速度LAND_FALL_SPEED_REF以上 → DIP_MAX
            float ratio = m_lastFallSpeed / LAND_FALL_SPEED_REF;
            if (ratio > 1.0f) ratio = 1.0f;  // 1.0で頭打ち

            float dipAmount = LAND_CAMERA_DIP_MIN
                + (LAND_CAMERA_DIP_MAX - LAND_CAMERA_DIP_MIN) * ratio;
            m_landingCameraOffset = -dipAmount;

            m_lastFallSpeed = 0.0f;  // リセット
        }

        // 沈み込みからゼロに向かって滑らかに戻る
        if (m_landingCameraOffset < 0.0f)
        {
            m_landingCameraOffset += LAND_CAMERA_RECOVER * deltaTime;
            if (m_landingCameraOffset > 0.0f)
                m_landingCameraOffset = 0.0f;
        }

        m_wasGroundedLastFrame = m_isGrounded;
    }
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

// UpdateMovement - 移動 + 重力 + ジャンプ
void Player::UpdateMovement()
{
    // --- deltaTime（1フレームの経過秒数）---
    // TODO: 本来はタイマーから取得すべき。現在は60FPS固定の暫定値。
    const float deltaTime = 1.0f / 60.0f;

    // ========================================
    //  水平移動（WASD）
    // ========================================
    float moveSpeed = MOVE_SPEED_WALK;

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        moveSpeed = MOVE_SPEED_RUN;
    }

    // W - 前進
    if (GetAsyncKeyState('W') & 0x8000)
    {
        m_position.x += sinf(m_rotation.y) * moveSpeed;
        m_position.z += cosf(m_rotation.y) * moveSpeed;
    }
    // S - 後退
    if (GetAsyncKeyState('S') & 0x8000)
    {
        m_position.x -= sinf(m_rotation.y) * moveSpeed;
        m_position.z -= cosf(m_rotation.y) * moveSpeed;
    }
    // A - 左移動
    if (GetAsyncKeyState('A') & 0x8000)
    {
        m_position.x += sinf(m_rotation.y - HALF_PI) * moveSpeed;
        m_position.z += cosf(m_rotation.y - HALF_PI) * moveSpeed;
    }
    // D - 右移動
    if (GetAsyncKeyState('D') & 0x8000)
    {
        m_position.x += sinf(m_rotation.y + HALF_PI) * moveSpeed;
        m_position.z += cosf(m_rotation.y + HALF_PI) * moveSpeed;
    }

    // ========================================
    //  ジャンプ入力
    // ========================================
    bool jumpPressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    // ジャンプバッファ：着地前の先行入力を覚えておく
    if (jumpPressed)
    {
        m_jumpBufferTimer = JUMP_BUFFER;
    }
    else if (m_jumpBufferTimer > 0.0f)
    {
        m_jumpBufferTimer -= deltaTime;
    }

    // コヨーテタイム：地面を離れた直後の猶予
    if (m_isGrounded)
    {
        m_coyoteTimer = COYOTE_TIME;
    }
    else
    {
        m_coyoteTimer -= deltaTime;
    }

    // ジャンプ実行
    bool canJump = (m_coyoteTimer > 0.0f);
    bool wantsJump = (m_jumpBufferTimer > 0.0f);

    if (canJump && wantsJump)
    {
        m_velocityY = JUMP_FORCE;
        m_isGrounded = false;
        m_coyoteTimer = 0.0f;
        m_jumpBufferTimer = 0.0f;

        // === 前方ブースト ===
        // WASDのどれかを押しながらジャンプ → その方向に少し飛ぶ
        // 止まったままジャンプ → 真上に飛ぶ（ブーストなし）
        bool anyMoveKey =
            (GetAsyncKeyState('W') & 0x8000) ||
            (GetAsyncKeyState('S') & 0x8000) ||
            (GetAsyncKeyState('A') & 0x8000) ||
            (GetAsyncKeyState('D') & 0x8000);

        if (anyMoveKey)
        {
            m_position.x += sinf(m_rotation.y) * JUMP_FORWARD_BOOST;
            m_position.z += cosf(m_rotation.y) * JUMP_FORWARD_BOOST;
        }
    }

    // ========================================
    //  重力の適用
    // ========================================
    m_velocityY -= GRAVITY * deltaTime;
    m_position.y += m_velocityY * deltaTime;

    // ========================================
    //  安全ネット
    // ========================================
    // 本当の床判定は GamePlay.cpp の GetMeshFloorHeight が行う。
    // ここは「ワールド外に落下した場合」の保険。
    constexpr float WORLD_FLOOR = -200.0f;  // マップ最低地点より十分下
    if (m_position.y <= WORLD_FLOOR)
    {
        m_position.y = WORLD_FLOOR;
        m_velocityY = 0.0f;
        m_isGrounded = true;
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
    return false;

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

// AddPoints - ポイント//
void Player::AddPoints(int points)
{
    m_points += points;
}