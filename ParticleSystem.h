// ParticleSystem.h - パーティクル管理システム
// 【目的】爆発、銃口フラッシュなどのエフェクトを管理
#pragma once

#include "Entities.h"      // Particle構造体
#include <vector>          // std::vector
#include <DirectXMath.h>   // XMFLOAT3

enum class WeaponType;

// パーティクル管理クラス
class ParticleSystem {
public:
    // コンストラクタ
    // 【役割】パーティクル配列を初期化
    ParticleSystem();

    // === メイン処理 ===

    // Update - 毎フレーム呼ばれる更新処理
    // 【役割】全てのパーティクルを動かし、寿命切れを削除
    // 【引数】deltaTime: 経過時間（秒）
    void Update(float deltaTime);

    // === エフェクト生成 ===

    // CreateExplosion - 爆発エフェクトを生成
    // 【役割】指定位置に20個の緑色パーティクルをランダムに飛び散らせる
    // 【引数】position: 爆発位置
    // 【用途】敵を倒した時
    void CreateExplosion(DirectX::XMFLOAT3 position);

    // CreateMuzzleFlash - 銃口フラッシュを生成
    // 【役割】銃口に火花と燃焼ガスのパーティクルを生成
    // 【引数】muzzlePosition: 銃口の位置
    //        cameraRotation: カメラの回転（射撃方向）
    // 【用途】射撃時
    void CreateMuzzleFlash(
        DirectX::XMFLOAT3 muzzlePosition,
        DirectX::XMFLOAT3 cameraRotation,
        WeaponType weaponType);

    // === ゲッター ===

    // GetParticles - パーティクル配列を取得（読み取り専用）
    // 【用途】描画時に全パーティクルを見る
    // 【戻り値】const参照（変更不可・コピーしない）
    const std::vector<Particle>& GetParticles() const { return m_particles; }

    // IsEmpty - パーティクルが空か確認
    // 【用途】描画前の最適化（パーティクルがなければ描画スキップ）
    bool IsEmpty() const { return m_particles.empty(); }

    //  位置(pos)から方向(dir)に向かって血を噴出させる
    void CreateBloodEffect(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 dir, int count = 10);

private:
    // === プライベート関数 ===

    // CreateMuzzleParticles - 銃口パーティクルの詳細生成
    // 【役割】火花と燃焼ガスを別々に生成
    // 【理由】CreateMuzzleFlashから分離して見やすく
    void CreateMuzzleParticles(DirectX::XMFLOAT3 muzzlePosition,
        DirectX::XMFLOAT3 cameraRotation);

    // === メンバ変数 ===

    // m_particles - 現在存在する全てのパーティクル
    // 【型】std::vector<Particle>
    //   - 可変長配列（動的に増減）
    //   - Particle: 位置、速度、色、寿命を持つ構造体
    std::vector<Particle> m_particles;
};