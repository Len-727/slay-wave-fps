// Player.cpp - �v���C���[��ԊǗ��̎���
#include "Player.h"
#include <cmath>  // sinf, cosf

// �R���X�g���N�^
// �y�����z�����ʒu�AHP100�A�|�C���g500�ŊJ�n
Player::Player() :
    m_position(0.0f, EYE_HEIGHT, -0.5f),   // �����ʒu
    m_rotation(0.0f, 0.0f, 0.0f),    // ���ʂ�����
    m_health(100),
    m_points(500),
    m_isDamaged(false),
    m_damageTimer(0.0f),
    m_mouseCaptured(true),
    m_firstMouse(true),
    m_lastMouseX(0),
    m_lastMouseY(0),
    m_meleeAttackCooldown(0.0f),
    m_velocityY(0.0f),          // �������x�[���i�������Ă��Ȃ��j
    m_isGrounded(true),         // �ŏ��͒n�ʂ̏�ɂ���
    m_coyoteTimer(0.0f),        // �P�\�Ȃ��i�n�ʂɂ��邩��s�v�j
    m_jumpBufferTimer(0.0f),     // �o�b�t�@�Ȃ�
    m_landingCameraOffset(0.0f),
    m_wasGroundedLastFrame(true),
    m_lastFallSpeed(0.0f)
{
}

// Update - ���C���X�V����
void Player::Update(HWND window)
{
    //  ����t���[���ŃJ�[�\��������
    static bool firstFrame = true;
    if (firstFrame && m_mouseCaptured)
    {
        ShowCursor(FALSE);
        firstFrame = false;
    }

    // �ړ�����
    UpdateMovement();

    // === ���n���o�̍X�V ===
    {
        const float deltaTime = 1.0f / 60.0f;  // TODO: �O������󂯎��

        // �󒆂ɂ���ԁA�������x���L�^��������
        // �iLand()��velocityY��0�Ƀ��Z�b�g�����̂ŁA���n��͎��Ȃ��j
        if (!m_isGrounded)
        {
            // velocityY�����i�������j�Ȃ瑬�x���L�^
            if (m_velocityY < 0.0f)
                m_lastFallSpeed = -m_velocityY;  // ���̒l�ɕϊ����ĕۑ�
        }

        // ���n�����u�Ԃ����o�i�O�t���[���� �� ���t���[���ڒn�j
        if (m_isGrounded && !m_wasGroundedLastFrame)
        {
            // �������x�ɉ����Ē��ݗʂ��v�Z
            // ���x0 �� DIP_MIN�A���xLAND_FALL_SPEED_REF�ȏ� �� DIP_MAX
            float ratio = m_lastFallSpeed / LAND_FALL_SPEED_REF;
            if (ratio > 1.0f) ratio = 1.0f;  // 1.0�œ��ł�

            float dipAmount = LAND_CAMERA_DIP_MIN
                + (LAND_CAMERA_DIP_MAX - LAND_CAMERA_DIP_MIN) * ratio;
            m_landingCameraOffset = -dipAmount;

            m_lastFallSpeed = 0.0f;  // ���Z�b�g
        }

        // ���ݍ��݂���[���Ɍ������Ċ��炩�ɖ߂�
        if (m_landingCameraOffset < 0.0f)
        {
            m_landingCameraOffset += LAND_CAMERA_RECOVER * deltaTime;
            if (m_landingCameraOffset > 0.0f)
                m_landingCameraOffset = 0.0f;
        }

        m_wasGroundedLastFrame = m_isGrounded;
    }
    // �}�E�X���_��]
    UpdateMouseLook(window);

    // �}�E�X�Œ�̐؂�ւ�
    UpdateMouseCapture(window);

    // �_���[�W�^�C�}�[�X�V
    if (m_damageTimer > 0.0f)
    {
        m_damageTimer -= 1.0f / 60.0f;
        if (m_damageTimer <= 0.0f)
        {
            m_isDamaged = false;
        }
    }

    //  �ߐڍU���̃N�[���_�E�����X�V
    UpdateMeleeAttackCooldown(1.0f / 60.0f);
}

//  AddCameraRecoil -   �J�������R�C��(����)
void Player::AddCameraRecoil(float pitchRecoil, float yawRecoil)
{
    //  Pitch(�㉺)�̔���
    m_rotation.x -= pitchRecoil;    //  ������ɒ��ˏオ��
    
    //  �㉺�̐���(�^��E�^�������������Ȃ��悤��)
    const float maxPitch = DirectX::XM_PIDIV2 - 0.1f;   //  90�x - 1
    if (m_rotation.x < -maxPitch) m_rotation.x = -maxPitch;
    if (m_rotation.x > maxPitch) m_rotation.x = maxPitch;

    //  Yaw(���E)�̔���(�����_���ɍ��E�ɗh���)
    float randomYaw = ((float)rand() / RAND_MAX - 0.5f) * yawRecoil;
    m_rotation.y += randomYaw;
}

// UpdateMovement - �ړ� + �d�� + �W�����v
void Player::UpdateMovement()
{
    // --- deltaTime�i1�t���[���̌o�ߕb���j---
    // TODO: �{���̓^�C�}�[����擾���ׂ��B���݂�60FPS�Œ�̎b��l�B
    const float deltaTime = 1.0f / 60.0f;

    // ========================================
    //  �����ړ��iWASD�j
    // ========================================
    float moveSpeed = MOVE_SPEED_WALK;

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        moveSpeed = MOVE_SPEED_RUN;
    }

    // W - �O�i
    if (GetAsyncKeyState('W') & 0x8000)
    {
        m_position.x += sinf(m_rotation.y) * moveSpeed;
        m_position.z += cosf(m_rotation.y) * moveSpeed;
    }
    // S - ���
    if (GetAsyncKeyState('S') & 0x8000)
    {
        m_position.x -= sinf(m_rotation.y) * moveSpeed;
        m_position.z -= cosf(m_rotation.y) * moveSpeed;
    }
    // A - ���ړ�
    if (GetAsyncKeyState('A') & 0x8000)
    {
        m_position.x += sinf(m_rotation.y - HALF_PI) * moveSpeed;
        m_position.z += cosf(m_rotation.y - HALF_PI) * moveSpeed;
    }
    // D - �E�ړ�
    if (GetAsyncKeyState('D') & 0x8000)
    {
        m_position.x += sinf(m_rotation.y + HALF_PI) * moveSpeed;
        m_position.z += cosf(m_rotation.y + HALF_PI) * moveSpeed;
    }

    // ========================================
    //  �W�����v����
    // ========================================
    bool jumpPressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    // �W�����v�o�b�t�@�F���n�O�̐�s���͂��o���Ă���
    if (jumpPressed)
    {
        m_jumpBufferTimer = JUMP_BUFFER;
    }
    else if (m_jumpBufferTimer > 0.0f)
    {
        m_jumpBufferTimer -= deltaTime;
    }

    // �R���[�e�^�C���F�n�ʂ𗣂ꂽ����̗P�\
    if (m_isGrounded)
    {
        m_coyoteTimer = COYOTE_TIME;
    }
    else
    {
        m_coyoteTimer -= deltaTime;
    }

    // �W�����v���s
    bool canJump = (m_coyoteTimer > 0.0f);
    bool wantsJump = (m_jumpBufferTimer > 0.0f);

    if (canJump && wantsJump)
    {
        m_velocityY = JUMP_FORCE;
        m_isGrounded = false;
        m_coyoteTimer = 0.0f;
        m_jumpBufferTimer = 0.0f;

        // === �O���u�[�X�g ===
        // WASD�̂ǂꂩ�������Ȃ���W�����v �� ���̕����ɏ������
        // �~�܂����܂܃W�����v �� �^��ɔ�ԁi�u�[�X�g�Ȃ��j
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
    //  �d�͂̓K�p
    // ========================================
    m_velocityY -= GRAVITY * deltaTime;
    m_position.y += m_velocityY * deltaTime;

    // ========================================
    //  ���S�l�b�g
    // ========================================
    // �{���̏������ GamePlay.cpp �� GetMeshFloorHeight ���s���B
    // �����́u���[���h�O�ɗ��������ꍇ�v�̕ی��B
    constexpr float WORLD_FLOOR = -200.0f;  // �}�b�v�Œ�n�_���\����
    if (m_position.y <= WORLD_FLOOR)
    {
        m_position.y = WORLD_FLOOR;
        m_velocityY = 0.0f;
        m_isGrounded = true;
    }
}

// UpdateMouseLook - �}�E�X���_��]
void Player::UpdateMouseLook(HWND window)
{
    if (!m_mouseCaptured)
        return;

    // �}�E�X�ʒu�擾
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(window, &mousePos);

    if (!m_firstMouse)
    {
        // �}�E�X�̈ړ��ʂ��v�Z
        int deltaX = mousePos.x - m_lastMouseX;
        int deltaY = mousePos.y - m_lastMouseY;

        // ��]���x���X�V
        float mouseSensitivity = 0.002f;
        m_rotation.y += deltaX * mouseSensitivity;  // ���E��]
        m_rotation.x += deltaY * mouseSensitivity;  // �㉺��]

        // �㉺�����̉�]�𐧌�����i�^��E�^���܂ōs���߂��Ȃ��悤�ɂ���j
        float maxPitch = 1.5f; // �������� 85�x ���炢�i���W�A���j
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

    // �}�E�X����ʒ����ɖ߂�
    RECT rect;
    GetClientRect(window, &rect);
    m_lastMouseX = rect.right / 2;
    m_lastMouseY = rect.bottom / 2;

    POINT center = { m_lastMouseX, m_lastMouseY };
    ClientToScreen(window, &center);
    SetCursorPos(center.x, center.y);
}

// UpdateMouseCapture - �}�E�X�Œ�̐؂�ւ�
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
                ShowCursor(FALSE);  // �J�[�\����\��

                // �E�B���h�E�����Ƀ}�E�X���ړ�
                RECT rect;
                GetClientRect(window, &rect);
                POINT center = { rect.right / 2, rect.bottom / 2 };
                ClientToScreen(window, &center);
                SetCursorPos(center.x, center.y);
            }
            else
            {
                ShowCursor(TRUE);  // �J�[�\���\��
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

// TakeDamage - �_���[�W���󂯂�
bool Player::TakeDamage(int damage)
{
    return false;

    // ���G���Ԓ��̓_���[�W�Ȃ�
    if (m_damageTimer > 0.0f)
        return false;

    m_health -= damage;
    m_damageTimer = 1.0f;  // 1�b�Ԃ̖��G����
    m_isDamaged = true;

    // ���S����
    if (m_health <= 0)
    {
        m_health = 0;
        return true;  // ����
    }

    return false;  // �܂������Ă���
}

// AddPoints - �|�C���g//
void Player::AddPoints(int points)
{
    m_points += points;
}