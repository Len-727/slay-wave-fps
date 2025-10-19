// UISystem.h - UI描画システム
// 【目的】ゲーム画面のUI（HP、ポイント、弾薬など）を描画
#pragma once

#include <DirectXMath.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <memory>

// 前方宣言
class Player;
class WeaponSystem;
class WaveManager;

// UIシステムクラス
class UISystem {
public:
    // コンストラクタ
    // 【役割】画面サイズを設定
    UISystem(int screenWidth, int screenHeight);

    // === メイン描画 ===

    // DrawAll - 全てのUI要素を描画
    // 【役割】HP、ポイント、弾薬など全UI要素を一括描画
    // 【引数】batch: DirectXのプリミティブバッチ（描画ツール）
    //        player: プレイヤー（HP・ポイント取得用）
    //        weaponSystem: 武器システム（弾薬取得用）
    //        waveManager: ウェーブ管理（ウェーブ番号取得用）
    void DrawAll(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        const Player* player,
        const WeaponSystem* weaponSystem,
        const WaveManager* waveManager);

    // OnScreenSizeChanged - 画面サイズ変更時に呼ぶ
    void OnScreenSizeChanged(int width, int height);

private:
    // === プライベート描画関数 ===

    // 各UI要素の描画
    void DrawHealthBar(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int health);
    void DrawCrosshair(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch);
    void DrawWaveNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int wave);
    void DrawPoints(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int points);
    void DrawAmmo(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        int currentAmmo, int reserveAmmo, bool isReloading);
    void DrawWeaponNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int weaponNum);

    // 数字描画ヘルパー
    // 【役割】0-9の数字を線で描画
    void DrawSimpleNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        int digit, float x, float y, DirectX::XMFLOAT4 color);

    // === メンバ変数 ===

    int m_screenWidth;   // 画面の幅
    int m_screenHeight;  // 画面の高さ
};