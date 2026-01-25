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
		config.bodyWidth = 0.50f;
		config.bodyHeight = 1.13f;
		config.headHeight = 1.41f;
		config.headRadius = 0.28f;
		config.health = 100;
		config.speedMultiplier = 1.0f;
		config.color = DirectX::XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f);  // 赤
		break;

	case EnemyType::RUNNER:
		config.bodyWidth = 0.40f;
		config.bodyHeight = 1.32f;
		config.headHeight = 1.47f;
		config.headRadius = 0.20f;
		config.health = 50;
		config.speedMultiplier = 2.0f;
		config.color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);  // 明るい赤
		break;

	case EnemyType::TANK:
		config.bodyWidth = 0.7f;
		config.bodyHeight = 2.0f;
		config.headHeight = 2.3f;
		config.headRadius = 0.7f;
		config.health = 300;
		config.speedMultiplier = 0.5f;
		config.color = DirectX::XMFLOAT4(0.2f, 0.2f, 0.8f, 1.0f);  // 青
		break;
	}

	return config;
}



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
	bool isDying = false;		//	死亡アニメーション再生中か?
	float corpseTimer = 0.0f;	//	死体が消えるまでのタイマー
	bool isRagdoll = false;

	//	ヘッドショット用
	bool headDestroyed = false;			//	頭が吹き飛んだか？
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


	Enemy()
		: position(0, 0, 0)
		, velocity(0, 0, 0)
		, color(1, 0, 0, 1)
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
		, isStaggered(false)
		, staggerFlashTimer(0.0f)
	{

	}
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