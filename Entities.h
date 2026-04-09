//	Entities.h

#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <map>

//	�Q�[���̏��
enum class GameState {
	TITLE,
	LOADING,
	PLAYING,
	GAMEOVER,
	RANKING
};

//	����̎��
enum class WeaponType {
	PISTOL,
	SHOTGUN,
	RIFLE,
	SNIPER
};

//	===	�G�̎�ނ�//	===
enum class EnemyType{
	NORMAL,	//	�ʏ�̓G
	RUNNER,	//	���������G
	TANK,	//	�^�t�ȓG
	MIDBOSS,
	BOSS
};

//	����f�[�^
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

//	����̒e����
struct WeaponAmmo {
	int currentAmmo;
	int reserveAmmo;
};

//	===	�A�j���[�V�����֘A�̍\����	===

//	---	�{�[��(��)	---
//	3D���f���̒��̍�
struct Bone
{
	std::string name;	//	���̖��O(LeftArm, RightLeg)
	DirectX::XMMATRIX offsetMatrix;	//	�I�t�Z�b�g�s��(�����p������̕ϊ�)	���f�������p������{�[����Ԃւ̕ϊ��A���_���W���{�[���̍��W�n�ɕϊ����邽��
	int parentIndex;	//	�e�{�[���̃C���f�b�N�X(-1�Ȃ�e�Ȃ�)�@�{�[���̊K�w�\����\���B�I�̃{�[�����e�͌��̃{�[��
};

//	---	�L�[�t���[��(�A�j���[�V�����̋�)	---
struct KeyFrame
{
	float time;					//	����
	DirectX::XMFLOAT3 position;	//	���̎����ł̈ʒu
	DirectX::XMFLOAT4 rotation;	//	���̎����ł̉�]
	DirectX::XMFLOAT3 scale;	//	���̎����ł̃X�P�[��(����܂���Ȃ�)
};

//	---	�A�j���[�V�����`�����l��	---
struct AnimationChannel
{
	std::string boneName;				//	���̃`�����l�������䂷��{�[���̖��O
	std::vector<KeyFrame> keyframes;	//	�L�[�t���[���̔z��
};

//	---	�A�j���[�V�����N���b�v	---
//	����S�̂�ۑ�
struct AnimationClip
{
	std::string name;		//	�A�j�}�[�V�����̖��O
	float duration;			//	�A�j���[�V�����̒���(�b)
	float ticksPerSecond;	//	�Đ����x�@�A�j���[�V�������Ԃ������Ԃɕϊ�(30 = 30fps�ŃA�j���[�V�����쐬)
	std::vector<AnimationChannel> channels;	//	�S�{�[���̃A�j���[�V�����f�[�^
};

//	---	�A�j���[�V�����̎��	---
//	�ǂ̃A�j���[�V�������Đ������Ǘ�
//enum class AnimationType
//{
//	IDLE,	//	�ҋ@
//	WALK,	//	���s
//	RUN,	//	���s
//	ATTACK	//	�U��
//};

//	�G�̃^�C�v�ʐݒ�i��ӏ��ŊǗ��j
struct EnemyTypeConfig
{
	// �̂̓����蔻��
	float bodyWidth;
	float bodyHeight;

	// ���̓����蔻��
	float headHeight;
	float headRadius;

	// �X�e�[�^�X
	int health;
	float speedMultiplier;

	// �F
	DirectX::XMFLOAT4 color;

	float staggerDuration;	//	�X�^����������

};


// === �G�^�C�v�̐ݒ���擾����֐� ===
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
		config.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // ��
		config.staggerDuration = 2.5f;
		break;

	case EnemyType::RUNNER:
		config.bodyWidth = 0.45f;
		config.bodyHeight = 1.70f;
		config.headHeight = 1.47f;
		config.headRadius = 0.25f;
		config.health = 50;
		config.speedMultiplier = 2.0f;
		config.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // ���邢��
		config.staggerDuration = 2.0f;
		break;

	case EnemyType::TANK:
		config.bodyWidth = 0.70f;
		config.bodyHeight = 1.84f;
		config.headHeight = 2.30f;
		config.headRadius = 0.70f;
		config.health = 300;
		config.speedMultiplier = 0.5f;
		config.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // ��
		config.staggerDuration = 4.0f;
		break;

	case EnemyType::MIDBOSS:
		config.bodyWidth = 1.00f;
		config.bodyHeight = 1.94f;
		config.headHeight = 1.68f;
		config.headRadius = 0.44f;
		config.health = 5000;
		config.speedMultiplier = 0.4f; // �������
		config.color = DirectX::XMFLOAT4(1.0f, 0.3f, 0.3f, 1.0f);
		config.staggerDuration = 5.0f;
		break;

	case EnemyType::BOSS:
		config.bodyWidth = 2.45f;
		config.bodyHeight = 2.87f;
		config.headHeight = 2.48f;
		config.headRadius = 1.11f;
		config.health = 15000;
		config.speedMultiplier = 0.3f;  // �x�����Ǎd��
		config.color = DirectX::XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f);
		config.staggerDuration = 6.0f;
		break;
	}

	return config;
}


// === �{�X�U����� ===
enum class BossAttackPhase {
	IDLE,           // �ʏ�s���i�v���C���[�ɐڋ߁j
	JUMP_WINDUP,    // �W�����v���߁i�n�ʂ����яオ��O�j
	JUMP_AIR,       // �󒆁i�㏸�������j
	JUMP_SLAM,      // ���n�@�����i�͈̓_���[�W�����j
	SLAM_RECOVERY,  // �@������̍d��
	SLASH_WINDUP,   // ����グ����
	SLASH_FIRE,     // �a��3�{����
	SLASH_RECOVERY, // �a����̍d��
	ROAR_WINDUP,    // ���K���߁iMIDBOSS�r�[���j
	ROAR_FIRE,      // �r�[�����˒�
	ROAR_RECOVERY,  // ���K��̍d��
};


//	�G
struct Enemy {
	int id;	//	�G�̈��ID
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
	bool wasTouchingPlayer = false;	//	�O�t���[���̐ڐG���
	bool attackJustLanded = false;	//	�U�������������u�Ԃ���true
	bool isDying = false;		//	���S�A�j���[�V�����Đ�����?
	float corpseTimer = 0.0f;	//	���̂�������܂ł̃^�C�}�[
	bool isRagdoll = false;
	bool isExploded = false;    // ���U�ς݁i���f����\���AGib�̂݁j

	//	�w�b�h�V���b�g�p
	bool headDestroyed = false;			//	����������񂾂��H
	bool justSpawned = false;
	DirectX::XMFLOAT3 headPosition;		//	���̈ʒu(�q�b�g����p)
	DirectX::XMFLOAT3 bloodDirection;	//	������ђ���	

	//AnimationType currentAnimation;	//	���ݍĐ����̃A�j���[�V����	�؂�ւ�
	
	//	---	�A�j���[�V�����p	---
	std::string currentAnimation;
	float animationTime;	//	�A�j���[�V�����̍Đ�����
	float rotationY;		//	�G�̌���

	//	---	�O���[���[�L���p	---
	bool isStaggered;	//	���߂����(HP30%�ȉ�)
	float staggerFlashTimer;	//	�_�ŗp�^�C�}�[

	//	�p���B�E�X�^���V�X�e��
	float damageMultiplier = 1.0f;  //  �E�F�[�u�U���͔{��
	float stunValue = 0.0f;	//	���݂̃X�^���~�ϒl
	float maxStunValue = 100.0f;	//	�X�^���l������ɒB����ƃX�^���`

	float staggerTimer = 0.0f;	//	�X�^���o�ߎ���
	float staggerDuration = 3.0f;	//	�X�^���p������

	bool hasBeenHPStaggered = false;

	// === �{�XAI�p ===
	BossAttackPhase bossPhase = BossAttackPhase::IDLE;
	float bossPhaseTimer = 0.0f;        // ���݃t�F�[�Y�̌o�ߎ���
	float bossAttackCooldown = 0.0f;    // ���̍U���܂ł̃N�[���_�E��
	int   bossAttackCount = 0;          // �U���񐔁i�p�^�[���ؑ֗p�j
	float bossJumpHeight = 0.0f;        // �W�����v����Y�ʒu
	DirectX::XMFLOAT3 bossTargetPos = { 0,0,0 }; // �U�����̃^�[�Q�b�g�ʒu
	DirectX::XMFLOAT3 bossJumpStartPos = { 0,0,0 };	//	�W�����v�J�n�ʒu
	bool bossBeamParriable = false;
	// === �p�X�Ǐ]�p ===
	std::vector<DirectX::XMFLOAT3> aiPath;  // �o�H�̃E�F�C�|�C���g�z��
	int aiPathIndex = 0;                      // ���ǂ̃E�F�C�|�C���g�Ɍ������Ă邩
	float aiPathTimer = 0.0f;                 // �o�H�Čv�Z�^�C�}�[

	bool aiControlled = false;



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
		, staggerTimer(0.0f)
		, staggerDuration(3.0f)
		, hasBeenHPStaggered(false)
	{

	}
};

// === �{�X�a���v���W�F�N�^�C�� ===
struct BossProjectile {
	DirectX::XMFLOAT3 position;     // ���݈ʒu
	DirectX::XMFLOAT3 direction;    // ��ԕ����i���K���ς݁j
	float speed = 15.0f;            // �ړ����x
	float lifetime = 3.0f;          // �c�����
	float damage = 30.0f;           // �_���[�W
	bool  isParriable = false;      // true=�΁i�p���B�j, false=�ԁi�K�[�h�̂݁j
	bool  isActive = true;          // �����Ă邩
	int   ownerID = -1;             // ���˂����G��ID
	int   effectHandle = -1;
};

// === �񕜃A�C�e�� ===
struct HealthPickup {
	DirectX::XMFLOAT3 position;     // ���[���h�ʒu
	int   healAmount = 25;           // �񕜗�
	float lifetime = 30.0f;        // �c�莞�ԁi�b�j
	float maxLifetime = 30.0f;       // �ő�����i�t�F�[�h�p�j
	float bobTimer = 0.0f;         // �㉺�{�u���o�p
	float spinAngle = 0.0f;         // ��]���o�p
	bool  isActive = true;         // �����Ă邩
};

// === �X�R�A�|�b�v�A�b�v�̎�� ===
enum class ScorePopupType {
	KILL,          // �ʏ�L���i���A���O���E�j
	HEADSHOT,      // �w�b�h�V���b�g�i���A�o�[�X�g�j
	GLORY_KILL,    // �O���[���[�L���i�ԁA�h�N���j
	WAVE_BONUS,    // �E�F�[�u�{�[�i�X�i���A�O���E�j
	MELEE_KILL,    // �ߐڃL��
	SHIELD_KILL,    // �������L��
	PARRY
};

// === �X�R�A�|�b�v�A�b�v ===
struct ScorePopup {
	float screenX = 0.0f;          // �X�N���[��X�i���S��j
	float screenY = 0.0f;          // �X�N���[��Y
	int   points = 0;             // �\���|�C���g
	ScorePopupType type = ScorePopupType::KILL;
	float timer = 0.0f;          // �o�ߎ���
	float maxTime = 1.4f;          // ���\������
	float scale = 1.0f;          // ���݂̃X�P�[��
	float alpha = 1.0f;          // ���݂̓����x
	float offsetY = 0.0f;          // ������̃I�t�Z�b�g�i�����オ��j
	float burstAngle = 0.0f;       // �o�[�X�g��]�p
	bool  isActive = true;
};

//	�p�[�e�B�N��
struct Particle {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	DirectX::XMFLOAT4 color;
	float lifetime;
	float maxLifetime;
	float size;
};

// �X�N���[���u���b�h�i��ʂɔ�юU�錌�j
struct ScreenBlood {
	float x, y;        // �X�N���[����̈ʒu (0?1)
	float size;         // �T�C�Y
	float alpha;        // �����x
	float lifetime;     // �c�莞��
	float maxLifetime;  // �ő����
	float rotation;     // ��]�p�x�i�����_���j
};

// ���f�J�[���i���Ɏc�錌���j
struct BloodDecal {
	DirectX::XMFLOAT3 position;
	float size;
	float rotation;
	float alpha;
	float lifetime;
	float maxLifetime;
	DirectX::XMFLOAT4 color;
};

// �ǂ̌����f�J�[��(�d�͂Ő����)
struct WallBloodDecal {
	DirectX::XMFLOAT3 position;    // �Ǐ�̏Փˈʒu
	DirectX::XMFLOAT3 normal;      // �ǂ̖@��
	float size;
	float rotation;
	float alpha;
	float lifetime;
	float maxLifetime;
	float dripLength;              // ���݂̐���̒���(���ԂŐL�т�)
	float maxDripLength;           // �ő吂�꒷��
	float dripSpeed;               // ����鑬�x
	DirectX::XMFLOAT4 color;
};