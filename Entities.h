//	Entities.h

#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <map>

//	ゲームの状態
enum class GameState {
	TITLE,
	PLAYING,
	GAMEOVER
};

//	武器の種類
enum class WeaponType {
	PISTOL,
	SHOTGUN,
	RIFLE,
	SNIPER
};

//	===	敵の種類を追加	===
enum class EnemyType{
	NORMAL,	//	通常の敵
	RUNNER,	//	足が速い敵
	TANK,	//	タフな敵
	MIDBOSS,
	BOSS
};

//	武器データ
struct WeaponData {
	WeaponType type;
	int damage;
	int maxAmmo;
	int reserveAmmo;
	float fireRate;
	float range;
	int penetration;
	float reloadTime;
	int cost;
};

//	武器の弾薬状態
struct WeaponAmmo {
	int currentAmmo;
	int reserveAmmo;
};

//	===	アニメーション関連の構造体	===

//	---	ボーン(骨)	---
//	3Dモデルの中の骨
struct Bone
{
	std::string name;	//	骨の名前(LeftArm, RightLeg)
	DirectX::XMMATRIX offsetMatrix;	//	オフセット行列(初期姿勢からの変換)	モデル初期姿勢からボーン空間への変換、頂点座標をボーンの座標系に変換するため
	int parentIndex;	//	親ボーンのインデックス(-1なら親なし)　ボーンの階層構造を表現。肘のボーン→親は肩のボーン
};

//	---	キーフレーム(アニメーションの駒)	---
struct KeyFrame
{
	float time;					//	時刻
	DirectX::XMFLOAT3 position;	//	その時刻での位置
	DirectX::XMFLOAT4 rotation;	//	その時刻での回転
	DirectX::XMFLOAT3 scale;	//	その時刻でのスケール(あんまつかわない)
};

//	---	アニメーションチャンネル	---
struct AnimationChannel
{
	std::string boneName;				//	このチャンネルが制御するボーンの名前
	std::vector<KeyFrame> keyframes;	//	キーフレームの配列
};

//	---	アニメーションクリップ	---
//	動作全体を保存
struct AnimationClip
{
	std::string name;		//	アニマーションの名前
	float duration;			//	アニメーションの長さ(秒)
	float ticksPerSecond;	//	再生速度　アニメーション時間を実時間に変換(30 = 30fpsでアニメーション作成)
	std::vector<AnimationChannel> channels;	//	全ボーンのアニメーションデータ
};

//	---	アニメーションの種類	---
//	どのアニメーションを再生中か管理
//enum class AnimationType
//{
//	IDLE,	//	待機
//	WALK,	//	歩行
//	RUN,	//	走行
//	ATTACK	//	攻撃
//};

//	敵のタイプ別設定（一箇所で管理）
struct EnemyTypeConfig
{
	// 体の当たり判定
	float bodyWidth;
	float bodyHeight;

	// 頭の当たり判定
	float headHeight;
	float headRadius;

	// ステータス
	int health;
	float speedMultiplier;

	// 色
	DirectX::XMFLOAT4 color;
};


// === 敵タイプの設定を取得する関数 ===
inline EnemyTypeConfig GetEnemyConfig(EnemyType type)
{
	EnemyTypeConfig config;

	switch (type)
	{
	case EnemyType::NORMAL:
		config.bodyWidth = 0.74f;
		config.bodyHeight = 1.67f;
		config.headHeight = 1.59f;
		config.headRadius = 0.37f;
		config.health = 100;
		config.speedMultiplier = 1.0f;
		config.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // 赤
		break;

	case EnemyType::RUNNER:
		config.bodyWidth = 0.45f;
		config.bodyHeight = 1.70f;
		config.headHeight = 1.47f;
		config.headRadius = 0.25f;
		config.health = 50;
		config.speedMultiplier = 2.0f;
		config.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // 明るい赤
		break;

	case EnemyType::TANK:
		config.bodyWidth = 0.70f;
		config.bodyHeight = 1.84f;
		config.headHeight = 2.30f;
		config.headRadius = 0.70f;
		config.health = 300;
		config.speedMultiplier = 0.5f;
		config.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // 青
		break;

	case EnemyType::MIDBOSS:
		config.bodyWidth = 1.00f;
		config.bodyHeight = 1.94f;
		config.headHeight = 1.68f;
		config.headRadius = 0.44f;
		config.health = 5000;
		config.speedMultiplier = 0.4f; // ゆっくり
		config.color = DirectX::XMFLOAT4(1.0f, 0.3f, 0.3f, 1.0f);
		break;

	case EnemyType::BOSS:
		config.bodyWidth = 2.45f;
		config.bodyHeight = 2.87f;
		config.headHeight = 2.48f;
		config.headRadius = 1.11f;
		config.health = 15000;
		config.speedMultiplier = 0.3f;  // 遅いけど硬い
		config.color = DirectX::XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f);
		break;
	}

	return config;
}


// === ボス攻撃状態 ===
enum class BossAttackPhase {
	IDLE,           // 通常行動（プレイヤーに接近）
	JUMP_WINDUP,    // ジャンプ溜め（地面から飛び上がる前）
	JUMP_AIR,       // 空中（上昇→落下）
	JUMP_SLAM,      // 着地叩きつけ（範囲ダメージ発生）
	SLAM_RECOVERY,  // 叩きつけ後の硬直
	SLASH_WINDUP,   // 殴り上げ溜め
	SLASH_FIRE,     // 斬撃3本発射
	SLASH_RECOVERY, // 斬撃後の硬直
	ROAR_WINDUP,    // 咆哮溜め（MIDBOSSビーム）
	ROAR_FIRE,      // ビーム発射中
	ROAR_RECOVERY,  // 咆哮後の硬直
};


//	敵
struct Enemy {
	int id;	//	敵の一意ID
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	DirectX::XMFLOAT4 color;
	EnemyType type;
	bool isAlive;
	float moveTimer;
	float nextDirectionChange;
	int health;
	int maxHealth;
	bool touchingPlayer;
	bool wasTouchingPlayer = false;	//	前フレームの接触状態
	bool attackJustLanded = false;	//	攻撃が当たった瞬間だけtrue
	bool isDying = false;		//	死亡アニメーション再生中か?
	float corpseTimer = 0.0f;	//	死体が消えるまでのタイマー
	bool isRagdoll = false;
	bool isExploded = false;    // 爆散済み（モデル非表示、Gibのみ）

	//	ヘッドショット用
	bool headDestroyed = false;			//	頭が吹き飛んだか？
	bool justSpawned = false;
	DirectX::XMFLOAT3 headPosition;		//	頭の位置(ヒット判定用)
	DirectX::XMFLOAT3 bloodDirection;	//	吹っ飛び中か	

	//AnimationType currentAnimation;	//	現在再生中のアニメーション	切り替え
	
	//	---	アニメーション用	---
	std::string currentAnimation;
	float animationTime;	//	アニメーションの再生時刻
	float rotationY;		//	敵の向き

	//	---	グローリーキル用	---
	bool isStaggered;	//	よろめき状態(HP30%以下)
	float staggerFlashTimer;	//	点滅用タイマー

	//	パリィ・スタンシステム
	float stunValue = 0.0f;	//	現在のスタン蓄積値
	float maxStunValue = 100.0f;	//	スタン値がこれに達するとスタが～

	// === ボスAI用 ===
	BossAttackPhase bossPhase = BossAttackPhase::IDLE;
	float bossPhaseTimer = 0.0f;        // 現在フェーズの経過時間
	float bossAttackCooldown = 0.0f;    // 次の攻撃までのクールダウン
	int   bossAttackCount = 0;          // 攻撃回数（パターン切替用）
	float bossJumpHeight = 0.0f;        // ジャンプ中のY位置
	DirectX::XMFLOAT3 bossTargetPos = { 0,0,0 }; // 攻撃時のターゲット位置
	bool bossBeamParriable = false;

	Enemy()
		: position(0, 0, 0)
		, velocity(0, 0, 0)
		, color(1, 1, 1, 1)
		, type(EnemyType::NORMAL)
		, isAlive(false)
		, moveTimer(0.0f)
		, nextDirectionChange(2.0f)
		, health(100)
		, maxHealth(100)
		, touchingPlayer(false)
		, currentAnimation("Walk")
		, animationTime(0.0f)
		, rotationY(0.0f)
		, headDestroyed(false)
		, headPosition(0, 0, 0)
		, bloodDirection(0, 1, 0)
		, isRagdoll(false)
		, isExploded(false)
		, isStaggered(false)
		, staggerFlashTimer(0.0f)
		, stunValue(0.0f)
		, maxStunValue(100.0f)
		, bossPhase(BossAttackPhase::IDLE)
		, bossPhaseTimer(0.0f)
		, bossAttackCooldown(3.0f)
		, bossAttackCount(0)
		, bossJumpHeight(0.0f)
		, bossTargetPos{ 0, 0, 0 }
	{

	}
};

// === ボス斬撃プロジェクタイル ===
struct BossProjectile {
	DirectX::XMFLOAT3 position;     // 現在位置
	DirectX::XMFLOAT3 direction;    // 飛ぶ方向（正規化済み）
	float speed = 15.0f;            // 移動速度
	float lifetime = 3.0f;          // 残り寿命
	float damage = 30.0f;           // ダメージ
	bool  isParriable = false;      // true=緑（パリィ可）, false=赤（ガードのみ）
	bool  isActive = true;          // 生きてるか
	int   ownerID = -1;             // 発射した敵のID
	int   effectHandle = -1;
};

//	パーティクル
struct Particle {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	DirectX::XMFLOAT4 color;
	float lifetime;
	float maxLifetime;
	float size;
};

// スクリーンブラッド（画面に飛び散る血）
struct ScreenBlood {
	float x, y;        // スクリーン上の位置 (0?1)
	float size;         // サイズ
	float alpha;        // 透明度
	float lifetime;     // 残り時間
	float maxLifetime;  // 最大寿命
	float rotation;     // 回転角度（ランダム）
};

// 血デカール（床に残る血痕）
struct BloodDecal {
	DirectX::XMFLOAT3 position;
	float size;
	float rotation;
	float alpha;
	float lifetime;
	float maxLifetime;
	DirectX::XMFLOAT4 color;
};

// 壁の血痕デカール(重力で垂れる)
struct WallBloodDecal {
	DirectX::XMFLOAT3 position;    // 壁上の衝突位置
	DirectX::XMFLOAT3 normal;      // 壁の法線
	float size;
	float rotation;
	float alpha;
	float lifetime;
	float maxLifetime;
	float dripLength;              // 現在の垂れの長さ(時間で伸びる)
	float maxDripLength;           // 最大垂れ長さ
	float dripSpeed;               // 垂れる速度
	DirectX::XMFLOAT4 color;
};