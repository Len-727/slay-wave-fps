//	Entities.h

#pragma once
#include <DirectXMath.h>

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
};

//	パーティクル
struct Particle {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	DirectX::XMFLOAT4 color;
	float lifetime;
	float maxLifetime;
};

////	アニメーションの種類
//enum class EnemyAnimationType
//{
//	IDLE,	//	待機
//	WALK,	//	歩行
//	RUN,	//	走行
//	ATTACK	//	攻撃
//};
////	アニメーション情報
//struct AnimationClip
//{
//	std::string name;
//	float duration;
//	float ticksPerSecond;
//	std::vector<>keyframes;
//};