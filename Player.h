// Player.h - プレイヤー状態管理
// 【目的】プレイヤーの位置、HP、ポイントを管理
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>   // XMFLOAT3
#include <Windows.h>       // HWND



// プレイヤークラス
class Player {
public:
    // コンストラクタ
    // 【役割】初期位置、HP、ポイントを設定
    Player();

    // === メイン処理 ===

    // Update - プレイヤーの移動とカメラ制御
    // 【役割】キー入力・マウス入力を処理
    // 【引数】window: ウィンドウハンドル（マウス制御用）
    void Update(HWND window);

    void AddCameraRecoil(float pitchRecoil, float yawRecoil);

    void Draw(
        ID3D11DeviceContext* context,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX projection);


    // TakeDamage - ダメージを受ける
    // 【役割】HPを減らし、ダメージエフェクトを開始
    // 【引数】damage: ダメージ量
    // 【戻り値】死んだらtrue
    bool TakeDamage(int damage);

    // AddPoints - ポイントを追加
    // 【役割】敵を倒した時などに呼ぶ
    void AddPoints(int points);

    // === ゲッター ===

    // GetPosition - プレイヤー位置
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }

    // GetRotation - カメラ回転
    DirectX::XMFLOAT3 GetRotation() const { return m_rotation; }

    // GetHealth - 現在のHP
    int GetHealth() const { return m_health; }

    int GetMaxHealth() const { return 100; }

    void SetHealth(int health)
    {
        m_health = health;
        if (m_health > 100) m_health = 100;  // 最大HP100
        if (m_health < 0) m_health = 0;      // 最小HP0
    }

    // GetPoints - 現在のポイント
    int GetPoints() const { return m_points; }

    // GetPointsRef - ポイントの参照（武器購入で減らすため）
    int& GetPointsRef() { return m_points; }

    // IsDamaged - ダメージエフェクト表示中か
    bool IsDamaged() const { return m_isDamaged; }

    // GetDamageTimer - ダメージタイマー（エフェクト用）
    float GetDamageTimer() const { return m_damageTimer; }

    // IsMouseCaptured - マウスが固定されているか
    bool IsMouseCaptured() const { return m_mouseCaptured; }

    void SetMouseCaptured(bool captured) { m_mouseCaptured = captured; m_firstMouse = true; }

    //  プレイヤー位置設定
    void SetPosition(DirectX::XMFLOAT3 pos) { m_position = pos; }

    void SetRotation(DirectX::XMFLOAT3 rot) { m_rotation = rot; }

    DirectX::XMMATRIX GetWorldMatrix() const
    {
        return DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) *
            DirectX::XMMatrixRotationY(m_rotation.y) *
            DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    }

    //  === 近接攻撃用   ===

    //  カメラのY軸回転を取得
    float GetCameraRotationY() const { return m_rotation.y; }

    //  近接攻撃可能か
    bool CanMeleeAttack() const {return m_meleeAttackCooldown <= 0.0f;}

    //  近接武器用のクールダウンを開始
    void StartMeleeAttackCooldown() { m_meleeAttackCooldown = 1.0f; }

    //  クールダウンタイマーを取得
    float GetMeleeAttackCooldown() const { return m_meleeAttackCooldown; }

    //  クールダウンを更新(毎フレーム)
    void UpdateMeleeAttackCooldown(float deltaTime)
    {
        if (m_meleeAttackCooldown > 0.0f)
        {
            m_meleeAttackCooldown -= deltaTime;
            if (m_meleeAttackCooldown < 0.0f)
                m_meleeAttackCooldown = 0.0f;
        }
    }

private:
    // UpdateMovement - 移動処理
    void UpdateMovement();

    // UpdateMouseLook - マウスによる視点回転
    void UpdateMouseLook(HWND window);

    // UpdateMouseCapture - マウス固定の切り替え
    void UpdateMouseCapture(HWND window);

    // === メンバ変数 ===

    // 位置・回転
    DirectX::XMFLOAT3 m_position;     // プレイヤー位置
    DirectX::XMFLOAT3 m_rotation;     // カメラ回転（X:上下, Y:左右）

    // ステータス
    int m_health;                      // 体力（0?100）
    int m_points;                      // ポイント（お金）

    // ダメージ関連
    bool m_isDamaged;                  // ダメージエフェクト表示中
    float m_damageTimer;               // ダメージ無敵時間

    // マウス制御
    bool m_mouseCaptured;              // マウスが固定されているか
    bool m_firstMouse;                 // 初回マウス移動か
    int m_lastMouseX;                  // 前フレームのマウスX
    int m_lastMouseY;                  // 前フレームのマウスY

    //  近接攻撃用メンバ変数
    float m_meleeAttackCooldown;    //  クールダウンタイマー
};