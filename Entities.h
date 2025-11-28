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


//	敵
struct Enemy {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	DirectX::XMFLOAT4 color;
	bool isAlive;
	float moveTimer;
	float nextDirectionChange;
	int health;
	int maxHealth;
	bool touchingPlayer;
	//AnimationType currentAnimation;	//	現在再生中のアニメーション	切り替え
	
	//	---	アニメーション用	---
	std::string currentAnimation;
	float animationTime;	//	アニメーションの再生時刻
	float rotationY;	//	敵の向き

	Enemy()
		: position(0, 0, 0)
		, velocity(0, 0, 0)
		, color(1, 0, 0, 1)
		, isAlive(false)
		, moveTimer(0.0f)
		, nextDirectionChange(2.0f)
		, health(100)
		, maxHealth(100)
		, touchingPlayer(false)
		, currentAnimation("Walk")
		, animationTime(0.0f)
		, rotationY(0.0f)
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
};


