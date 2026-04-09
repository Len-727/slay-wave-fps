//	EnemySystem.cpp	�G�Ǘ��V�X�e���̎���
#define NOMINMAX
#include "EnemySystem.h"
#include<windows.h>
#include <algorithm>	//	std::remmove_if(�z�񂩂�v�f���폜)���g������
#include <cmath>		//	sprtf(������), conf, sinf���g������

//	�R���X�g���N�^	EnemySystem�ō��ꂽ���̏�����
//	�y���Ă΂��zGame.cpp�� std::make_unique<EnemySystem>()�����Ƃ�
//	�y�����z�����o�ϐ��ɏ����l��ݒ�
EnemySystem::EnemySystem()
	: m_maxEnemies(100)			//	�ő�I���F�@�Q�S��
	, m_enemySpawnTimer(0.0f)	//	�X�|�[���^�C�}�[:	�O�b����X�^�[�g
	, m_nextEnemyID(0)
{
	m_enemies.reserve(60);  //  �ő�60�̕��̃��������Ɋm��
	//	m_enemies�@�͋�̔z��Ƃ��Ď��������������
}

//	�y�����z�S�Ă̓G�𓮂����āA�v���C���[�ɋ߂Â���
void EnemySystem::Update(float deltaTime, DirectX::XMFLOAT3 playerPos)
{
	// === �S�Ă̓G����̂��X�V ===
	for (auto& enemy : m_enemies)
	{
		// ����ł���G�̓X�L�b�v
		if (!enemy.isAlive)
			continue;

		// === ���O�h�[���i������сj ===
		if (enemy.isRagdoll)
		{
			// �ʒu���X�V
			enemy.position.x += enemy.velocity.x * deltaTime;
			enemy.position.y += enemy.velocity.y * deltaTime;
			enemy.position.z += enemy.velocity.z * deltaTime;

			// �d�͂�K�p�i�����������j
			enemy.velocity.y -= 9.8f * deltaTime;

			// ���x�������i��C��R�j
			enemy.velocity.x *= 0.98f;
			enemy.velocity.z *= 0.98f;

			// �n�ʂɂ��������
			if (enemy.position.y <= 0.0f)
			{
				enemy.isAlive = false;
			}

			continue;  // �ʏ�̈ړ��������X�L�b�v
		}

		// === ���߂���Ԕ��� ===
		float hpRatio = (float)enemy.health / (float)enemy.maxHealth;

		// HP30%�ȉ� �� �V�K�X�^�K�[����
		if (hpRatio <= 0.3f && enemy.health > 0 && !enemy.isStaggered && !enemy.hasBeenHPStaggered)
		{
			enemy.isStaggered = true;
			enemy.staggerTimer = 0.0f;
			enemy.staggerFlashTimer = 0.0f;
			enemy.hasBeenHPStaggered = true;

			// �X�^���A�j���[�V�����ɐ؂�ւ�
			enemy.currentAnimation = "Stun";
			enemy.animationTime = 0.0f;
		}

		// === �X�^�K�[���̏��� ===
		if (enemy.isStaggered)
		{
			// �^�C�}�[��i�߂�
			enemy.staggerTimer += deltaTime;
			enemy.staggerFlashTimer += deltaTime * 1.5f;  // �������C�g�̃p���X�p

			// �� �ړ����Ȃ��I�U�����Ȃ��I���S��ԁI
			// �A�j���[�V���������i�߂�
			enemy.velocity = { 0.0f, 0.0f, 0.0f };

			// �X�^���A�j�������[�v�Đ�
			if (enemy.currentAnimation != "Stun")
			{
				enemy.currentAnimation = "Stun";
				enemy.animationTime = 0.0f;
			}

			//  ���Ԍo�߂ŃX�^������
			if (enemy.staggerTimer >= enemy.staggerDuration)
			{
				enemy.isStaggered = false;
				enemy.staggerTimer = 0.0f;
				enemy.staggerFlashTimer = 0.0f;
				enemy.stunValue = -1.0f;   //  -1 �ōĒ~�σX�^�[�g��x�点��

				enemy.currentAnimation = "Idle";
				enemy.animationTime = 0.0f;
			}

			// �X�^�K�[���͈ړ��������X�L�b�v�I
		}
		else
		{
			// === �ʏ�̈ړ����� ===
			UpdateEnemyMovement(enemy, playerPos, deltaTime);
		}
	}

	// ========================================
	// === ��ԕ������g���������Փˉ��� ===
	// ========================================

	// Step 1: ��ԃO���b�h���\�z
	BuildSpatialGrid();

	// Step 2: �Փ˂������i�����o�������j
	ResolveCollisionsSpatial();
}

//	UpdateEnemyMovement	��т̓G�̈ړ�����
//	�y�����z�G���v���C���[�Ɍ������ē�����
//	�y�d�g�݁z��莞�Ԃ��Ƃɕ����]�����āA�v���C���[�ɋ߂Â�
void EnemySystem::UpdateEnemyMovement(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime)
{
	if (enemy.isDying)
		return;

	if (enemy.aiControlled)
		return;

		// == = MIDBOSS / BOSS �͐�pAI�ŏ��� == =
		if (enemy.type == EnemyType::MIDBOSS || enemy.type == EnemyType::BOSS)
		{
			UpdateBossAI(enemy, playerPos, deltaTime);
			return;  // �ʏ�AI�̓X�L�b�v
		}

	//	�ړ��^�C�}�[�𑝂₷
	//	�y�����z�u���b�����������v���L�^
	enemy.moveTimer += deltaTime;	//	deltaTime = 1/60�b = 0.0166...�b

	//	�����]���̃^�C�~���O���m�F
	//	�y�����zmoveTimer �� nextDirectionChange �𒴂���������]��
	//	�y��z�Q�b���Ƃɕ�����ς���
	if (enemy.moveTimer >= enemy.nextDirectionChange)
	{
		//	�v���C���[�̕����x�N�g�����v�Z
		//	�y�ړI�z�G���v���C���[�Ɍ������ē���������
		float dirX = playerPos.x - enemy.position.x;	//	X�����̋���
		float dirZ = playerPos.z - enemy.position.z;	//	Z�����̋���

		//	�������v�Z(�s�^�S���X�̒藝)
		//	�y�����zdistance = ��(dirX? + dirZ?)
		//	�y��zdirX = 3, dirZ = 4 distance = 5;
		float distance = sqrtf(dirX * dirX + dirZ * dirZ);

		//	�������O�łȂ����m�F(�[�����Z��h��)
		//	�y���R�zdistance = 0���ƁA���̐��I���Ŋ���Z�G���[�ɂȂ�
		if (distance > 0.1f)
		{
			//	�����x�N�g���𐳋K���i�������P�ɂ���j
			//	�y�ړI�z���x�����ɂ��邽��
			//	�y��zdirX = 3, dirZ = 4, distance = 5 �� dirX/distance = 0.6, dirZ/distance = 0.8
			float normalizedX = dirX / distance;
			float normalizedZ = dirZ / distance;

			enemy.velocity.x = normalizedX * 0.5f;
			enemy.velocity.z = normalizedZ * 0.5f;
		}

		//	�^�C�}�[�����Z�b�g
		//	�y���R�z���̕����]���܂ōĂуJ�E���g���邽��
		enemy.moveTimer = 0.0f;

		//	���̕����^�C�~���O�������_���ɐݒ�
		//	�y�͈́z1.0 - 3.0�b��
		//	�y�v�Z�z1.0 + (0.0 - 1.0�̃����_���l * 2.0) = 1.0 - 3.0
		enemy.nextDirectionChange = 1.0f + (float)rand() / RAND_MAX * 2.0f;
	}

	//	�ʒu���X�V(���x�ɉ����Ĉړ�)
	//	�y�����z�V�����ʒu�@���@���ݒn�@�{�@���x�@���@����
	//	�y��zposition.x = 5, velocity.x = 2 deltaTime = 0.5 �� �Vposition.x = 6
	enemy.position.x += enemy.velocity.x * deltaTime;
	enemy.position.z += enemy.velocity.z * deltaTime;

	//	�}�b�v�̋��E�`�F�b�N(�Ȉ�)
	//	�y�͈́zX���W�� -50 - 50�@�͈̔�
	//	�y�ړI�z�G�������ɉ����ɍs���Ȃ��悤��
	if (enemy.position.x < -50.0f || enemy.position.x > 50.0f ||
		enemy.position.z < -50.0f || enemy.position.z > 50.0f)
	{
		//	�͈͊O�ɏo����A���x�𔽓]���Ē������ɖ߂�
		//	�y���ʁz�G���ǂŒ��˕Ԃ�悤�ȓ���
		enemy.velocity.x *= -0.5f;	//�t�����ɔ����̑��x
		enemy.velocity.z *= -0.5f;
	}

	//	�v���C���[�ւ̕����x�N�g�����v�Z
	float dirToPlayerX = playerPos.x - enemy.position.x;
	float dirToPlayerZ = playerPos.z - enemy.position.z;

	//	===	�i�s�����Ɍ����悤�ɉ�]�p�x���X�V	===
	//	���x���ق�0����Ȃ��ꍇ�̂݌v�Z(�Ƃ܂��Ă���Ƃ��͌�����ς��Ȃ�)
	if (fabs(dirToPlayerX) > 0.001f || fabs(dirToPlayerZ) > 0.001f)
	{
		// atan2f(x, z) �ŁA(0, 1)�����iZ���v���X�j����̊p�x(���W�A��)�����߂���
		enemy.rotationY = atan2f(dirToPlayerX, dirToPlayerZ);

		enemy.rotationY += 3.14159f;
	}

	// �v���C���[�Ƃ̋������v�Z
   // �y�����zdistance = ��((�GX - �v���C���[X)? + (�GZ - �v���C���[Z)?)
   // �y���R�zY���W�͖����i�n�ʂ̍����͓����Ȃ̂Łj
	float playerDist = sqrtf(
		powf(enemy.position.x - playerPos.x, 2) +
		powf(enemy.position.z - playerPos.z, 2)
	);


	// �ڐG����i������1.5���[�g���ȓ��j
	if (playerDist < 1.5f)
	{
		enemy.touchingPlayer = true;
	}
	else
	{
		enemy.touchingPlayer = false;
	}

	// --- �^�C�v�ʃp�����[�^���Ɏ擾 ---
	float attackHitTime;
	float attackDuration;
	float attackAnimSpeed;
	switch (enemy.type)
	{
	case EnemyType::RUNNER:
		attackHitTime = m_runnerAttackHitTime;
		attackDuration = m_runnerAttackDuration;
		attackAnimSpeed = m_animSpeed_Attack[1];
		break;
	case EnemyType::TANK:
		attackHitTime = m_tankAttackHitTime;
		attackDuration = m_tankAttackDuration;
		attackAnimSpeed = m_animSpeed_Attack[2];
		break;
	default:
		attackHitTime = m_normalAttackHitTime;
		attackDuration = m_normalAttackDuration;
		attackAnimSpeed = m_animSpeed_Attack[0];
		break;
	}

	// --- ���݁u�U�����[�V�������v���ǂ��� ---
	bool isInAttackAnim = (enemy.currentAnimation == "Attack");

	// =============================================
	//  �U�����[�V�����Đ��� �� �Ō�܂ŐU��؂�
	// =============================================
	if (isInAttackAnim)
	{
		//  �q�b�g�O: �v���C���[�Ɍ������ă����W
		if (enemy.animationTime < attackHitTime)
		{
			float dx = playerPos.x - enemy.position.x;
			float dz = playerPos.z - enemy.position.z;
			float dist = sqrtf(dx * dx + dz * dz);

			if (dist > 0.5f)
			{
				float nx = dx / dist;
				float nz = dz / dist;
				float lungeSpeed = 2.0f * deltaTime;
				enemy.position.x += nx * lungeSpeed;
				enemy.position.z += nz * lungeSpeed;
			}
		}
		else
		{
			// �q�b�g��͑����~�߂�
			enemy.velocity.x = 0.0f;
			enemy.velocity.z = 0.0f;
		}

		//  �A�j���[�V�������Ԃ�i�߂�i���̃R�[�h�����j
		float prevTime = enemy.animationTime;
		enemy.animationTime += deltaTime * attackAnimSpeed;

		// �u����u�ԁv��ʉ߂�����_���[�W�t���OON
		if (prevTime < attackHitTime && enemy.animationTime >= attackHitTime)
		{
			if (enemy.touchingPlayer)
			{
				enemy.attackJustLanded = true;
			}
		}
		else
		{
			enemy.attackJustLanded = false;
		}

		// �A�j���[�V�������� �� ���̃A�N�V����������
		if (enemy.animationTime >= attackDuration)
		{
			enemy.animationTime = 0.0f;
			enemy.attackJustLanded = false;

			if (enemy.touchingPlayer)
			{
				// �܂��߂� �� �A���U��
			}
			else
			{
				// ���ꂽ �� �ǂ������ɐ؂�ւ�
				enemy.currentAnimation = "Walk";
			}
		}
	}

	// =============================================
	// �p�^�[��2: �U��������Ȃ� & �v���C���[���߂� �� �U���J�n
	// =============================================
	else if (enemy.touchingPlayer)
	{
		// �U�����[�V�����J�n
		enemy.currentAnimation = "Attack";

		// �G���m�̍U���������ɂȂ�Ȃ��悤�Ƀ����_���I�t�Z�b�g
		float maxOffset;
		switch (enemy.type)
		{
		case EnemyType::RUNNER:  maxOffset = m_runnerAttackHitTime * 0.5f; break;
		case EnemyType::TANK:   maxOffset = m_tankAttackHitTime * 0.5f;   break;
		default:                maxOffset = m_normalAttackHitTime * 0.5f;  break;
		}
		enemy.animationTime = ((float)rand() / RAND_MAX) * maxOffset;
		enemy.attackJustLanded = false;

		// �U���J�n�Ɠ����ɑ����~�߂�
		enemy.velocity.x = 0.0f;
		enemy.velocity.z = 0.0f;
	}
	// =============================================
	// �p�^�[��3: �U��������Ȃ� & �v���C���[������ �� �ړ�
	// =============================================
	else
	{
		enemy.attackJustLanded = false;

		// ���x�̒����i�X�s�[�h�v�Z�j
		float speed = sqrtf(enemy.velocity.x * enemy.velocity.x +
			enemy.velocity.z * enemy.velocity.z);

		// �^�C�v�ʂ̑��x�{��
		float speedMultiplier = 1.0f;
		switch (enemy.type)
		{
		case EnemyType::NORMAL: speedMultiplier = 2.5f; break;
		case EnemyType::RUNNER: speedMultiplier = 3.0f; break;
		case EnemyType::TANK:   speedMultiplier = 1.5f; break;
		}

		float finalSpeed = speed * speedMultiplier * m_waveSpeedMult;

		// �A�j���[�V�����I��
		std::string moveAnim = (finalSpeed > 4.0f) ? "Run" : "Walk";
		if (finalSpeed < 0.1f)
			moveAnim = "Idle";

		if (enemy.currentAnimation != moveAnim)
			enemy.currentAnimation = moveAnim;

		// �ʒu���X�V
		enemy.position.x += enemy.velocity.x * finalSpeed * deltaTime;
		enemy.position.z += enemy.velocity.z * finalSpeed * deltaTime;
	}
}

//	Spawn Enemy	�V�����G����̐���
//	�y���Ă΂��z�E�F�[�u�J�n���A�܂��͓G���������Ƃ�
//	�y�����z�����_���Ȉʒu�ɓG���o��������
void EnemySystem::SpawnEnemy(DirectX::XMFLOAT3 playerPos)
{
	if (m_enemies.size() >= m_maxEnemies)
		return;

	Enemy enemy;
	enemy.id = m_nextEnemyID++;

	// === �^�C�v�ݒ� ===
	int typeRoll = rand() % 100;

	if (typeRoll < 60)
	{
		enemy.type = EnemyType::NORMAL;
		enemy.health = (int)(100 * m_waveHpMult);     //  �E�F�[�u�{��
		enemy.maxHealth = enemy.health;
		enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if (typeRoll < 85)
	{
		enemy.type = EnemyType::RUNNER;
		enemy.health = (int)(50 * m_waveHpMult);      // 
		enemy.maxHealth = enemy.health;
		enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		enemy.type = EnemyType::TANK;
		enemy.health = (int)(300 * m_waveHpMult);     // 
		enemy.maxHealth = enemy.health;
		enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// === �d�Ȃ�Ȃ��ʒu�𐶐� ===
	const float MIN_SPAWN_DISTANCE = 2.5f;
	const int MAX_ATTEMPTS = 15;

	bool positionFound = false;
	int attempts = 0;

	while (!positionFound && attempts < MAX_ATTEMPTS)
	{
		attempts++;

		float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
		float distance = 10.0f + (float)rand() / RAND_MAX * 10.0f;

		DirectX::XMFLOAT3 candidatePos;
		candidatePos.x = playerPos.x + cosf(angle) * distance;
		candidatePos.y = 0.0f;
		candidatePos.z = playerPos.z + sinf(angle) * distance;

		//  FBX�_���W�����͈͓̔��ɃN�����v
		candidatePos.x = (std::max)(-31.0f, (std::min)(42.0f, candidatePos.x));
		candidatePos.z = (std::max)(-8.0f, (std::min)(22.0f, candidatePos.z));

		if (IsPositionValid(candidatePos, MIN_SPAWN_DISTANCE))
		{
			enemy.position = candidatePos;
			positionFound = true;
		}
	}

	if (!positionFound)
	{
		float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
		float distance = 10.0f + (float)rand() / RAND_MAX * 10.0f;

		enemy.position.x = playerPos.x + cosf(angle) * distance;
		enemy.position.z = playerPos.z + sinf(angle) * distance;

		//  �N�����v
		enemy.position.x = (std::max)(-31.0f, (std::min)(42.0f, enemy.position.x));
		enemy.position.z = (std::max)(-8.0f, (std::min)(22.0f, enemy.position.z));
	}

	// === �������x ===
	enemy.velocity.x = 0.0f;
	enemy.velocity.y = 0.0f;
	enemy.velocity.z = 0.0f;
	// �����ɕ����v�Z������i�_�����h�~�j
	enemy.moveTimer = 99.0f;

	// === �X�e�[�^�X ===
	enemy.isAlive = true;

	// === �A�j���[�V���� ===
	enemy.currentAnimation = "Idle";
	// �����A�j�����o������悤�Ƀ����_���I�t�Z�b�g
	enemy.animationTime = ((float)rand() / RAND_MAX) * 1.0f;

	// === ���̑� ===
	enemy.isDying = false;
	enemy.corpseTimer = 0.0f;  // �� ���������o�[
	enemy.rotationY = 0.0f;    // �� ���������o�[�irotation �ł͂Ȃ��I�j

	enemy.justSpawned = true;

	enemy.damageMultiplier = m_waveDamageMult;  //  �U���͔{����G�Ɏ�������
	m_enemies.push_back(enemy);
}

void EnemySystem::SpawnMidBoss(DirectX::XMFLOAT3 playerPos)
{
	Enemy enemy;
	enemy.id = m_nextEnemyID++;

	// MIDBOSS�Œ�X�e�[�^�X
	enemy.type = EnemyType::MIDBOSS;
	enemy.health = (int)(5000 * m_waveHpMult);     // 
	enemy.maxHealth = enemy.health;
	enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// �v���C���[�̐��ʂ�≓���ɃX�|�[��
	float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
	float distance = 15.0f + (float)rand() / RAND_MAX * 5.0f;

	enemy.position.x = playerPos.x + cosf(angle) * distance;
	enemy.position.z = playerPos.z + sinf(angle) * distance;

	//  �N�����v
	enemy.position.x = (std::max)(-31.0f, (std::min)(42.0f, enemy.position.x));
	enemy.position.z = (std::max)(-8.0f, (std::min)(22.0f, enemy.position.z));

	enemy.velocity = { 0.0f, 0.0f, 0.0f };
	enemy.isAlive = true;
	enemy.isDying = false;
	enemy.isRagdoll = false;
	enemy.isStaggered = false;
	enemy.headDestroyed = false;
	enemy.staggerFlashTimer = 0.0f;
	enemy.moveTimer = 99.0f;
	enemy.currentAnimation = "Idle";
	enemy.animationTime = 0.0f;
	enemy.corpseTimer = 0.0f;
	enemy.rotationY = 0.0f;


	enemy.justSpawned = true;

	enemy.damageMultiplier = m_waveDamageMult;
	m_enemies.push_back(enemy);


	char buf[128];
	sprintf_s(buf, "[MIDBOSS] Spawned! ID:%d, HP:%d\n", enemy.id, enemy.health);
	//OutputDebugStringA(buf);
}


void EnemySystem::SpawnBoss(DirectX::XMFLOAT3 playerPos)
{
	Enemy enemy;
	enemy.id = m_nextEnemyID++;

	enemy.type = EnemyType::BOSS;
	enemy.health = (int)(15000 * m_waveHpMult);    // 
	enemy.maxHealth = enemy.health;
	enemy.color = DirectX::XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f);  // �ԍ���

	float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
	float distance = 18.0f + (float)rand() / RAND_MAX * 5.0f;

	enemy.position.x = playerPos.x + cosf(angle) * distance;
	enemy.position.z = playerPos.z + sinf(angle) * distance;

	//  �N�����v
	enemy.position.x = (std::max)(-31.0f, (std::min)(42.0f, enemy.position.x));
	enemy.position.z = (std::max)(-8.0f, (std::min)(22.0f, enemy.position.z));

	enemy.velocity = { 0.0f, 0.0f, 0.0f };
	enemy.isAlive = true;
	enemy.isDying = false;
	enemy.isRagdoll = false;
	enemy.isStaggered = false;
	enemy.headDestroyed = false;
	enemy.staggerFlashTimer = 0.0f;
	enemy.currentAnimation = "Idle";
	enemy.animationTime = 0.0f;
	enemy.corpseTimer = 0.0f;
	enemy.rotationY = 0.0f;

	enemy.justSpawned = true;

	enemy.damageMultiplier = m_waveDamageMult;
	m_enemies.push_back(enemy);

	char buf[128];
	sprintf_s(buf, "[BOSS] Spawned! ID:%d, HP:%d\n", enemy.id, enemy.health);
	//OutputDebugStringA(buf);
}

//	\���񂾓G��z�񂩂�폜
//	�y���Ă΂��zUpdate()�̍Ō�A�܂��̓E�F�[�u�I����
//	�y�����zisAlive == false �̓G������������폜
void EnemySystem::ClearDeadEnemies()
{
	//	std::remove_if + erase �C�f�B�I��
	//	�y�d�g�݁z
	//	1, remove_if;	�����ɍ����v�f��z��̌��Ɉړ�
	//	2, erase;	���̕s�v�ȗv�f���폜 

	m_enemies.erase(
		//	remove_if;	�u����ł���G�v��������
		// �y�����P�z�z��̊J�n�ʒu
		// �y�����Q�z�z��̏I���ʒu
		// �y�����R�z�����_��(���������������֐�)
		std::remove_if(
			m_enemies.begin(),	//	�z��̍ŏ�
			m_enemies.end(),//	�z��̍Ō�
			//	�����_���F[](����) {����}
			//	�y�����z�e�G�ɂ��āu�폜����ׂ����v�𔻒�
			//	�y�����zconst Enemy& e = �e�G(�Q�ƂŎ󂯎��)
			//	�y�߂�l�zbool = true �Ȃ�폜
			[](const Enemy& e) {
				// isDying���i���S�A�j���Đ����j�͏����Ȃ�
				return !e.isAlive && !e.isDying;
			}
		),
		m_enemies.end()	//	remove_if���Ԃ����u��������폜�v�ʒu
	);
} 

// === �֐�1: ���W����O���b�h�L�[���擾 ===
EnemySystem::GridKey EnemySystem::GetGridKey(const DirectX::XMFLOAT3& position) const
{
	GridKey key;
	// �ʒu���Z���T�C�Y�Ŋ����āA�Z���̍��W���擾
	// ��: position.x = 12.5, CELL_SIZE = 5.0 �� key.x = 2
	key.x = (int)floorf(position.x / CELL_SIZE);
	key.z = (int)floorf(position.z / CELL_SIZE);
	return key;
}

// === �֐�2: ��ԃO���b�h���\�z ===
void EnemySystem::BuildSpatialGrid()
{
	// �O�t���[���̃O���b�h���N���A
	m_spatialGrid.clear();

	// �S�Ă̓G���O���b�h�ɓo�^
	for (size_t i = 0; i < m_enemies.size(); i++)
	{
		const Enemy& enemy = m_enemies[i];

		// ����ł�G�⎀�S���̓G�͖���
		if (!enemy.isAlive || enemy.isDying)
			continue;

		// ���̓G��������Z�����v�Z
		GridKey key = GetGridKey(enemy.position);

		// �Z���ɓG�̃C���f�b�N�X��//
		m_spatialGrid[key].push_back(i);
	}
}

// === �֐�3: ��ԕ������g�����Փˉ��� ===
void EnemySystem::ResolveCollisionsSpatial()
{
	const float COLLISION_RADIUS = 0.5f;   // �G�̔��a
	const float PUSH_STRENGTH = 0.05f;     // �����o������
	const float MIN_DISTANCE = COLLISION_RADIUS * 2.0f;  // 1.0m

	// === �e�Z�����ŏՓ˃`�F�b�N ===
	for (auto& cellPair : m_spatialGrid)
	{
		const GridKey& currentKey = cellPair.first;
		std::vector<size_t>& enemiesInCell = cellPair.second;

		// === 1. �����Z�����̓G���m���`�F�b�N ===
		for (size_t i = 0; i < enemiesInCell.size(); i++)
		{
			Enemy& enemyA = m_enemies[enemiesInCell[i]];

			// �U�����͉����o���Ȃ��i�Q��ŏP���j
			if (enemyA.currentAnimation == "Attack")
				continue;

			for (size_t j = i + 1; j < enemiesInCell.size(); j++)
			{
				Enemy& enemyB = m_enemies[enemiesInCell[j]];

				if (enemyB.currentAnimation == "Attack")
					continue;

				// �������v�Z
				float dx = enemyB.position.x - enemyA.position.x;
				float dz = enemyB.position.z - enemyA.position.z;
				float distance = sqrtf(dx * dx + dz * dz);

				// �Փ˔���
				if (distance < MIN_DISTANCE && distance > 0.001f)
				{
					// �����o������
					float nx = dx / distance;
					float nz = dz / distance;

					// �d�Ȃ��
					float overlap = MIN_DISTANCE - distance;
					float pushDistance = overlap * PUSH_STRENGTH;

					// �݂��ɉ����o��
					enemyA.position.x -= nx * pushDistance;
					enemyA.position.z -= nz * pushDistance;

					enemyB.position.x += nx * pushDistance;
					enemyB.position.z += nz * pushDistance;
				}
			}
		}

		// === 2. �אڃZ���̓G�Ƃ��`�F�b�N ===
		// �Z���̋��E�t�߂̓G�̂��߁A�א�8�Z�����`�F�b�N
		for (int dx = -1; dx <= 1; dx++)
		{
			for (int dz = -1; dz <= 1; dz++)
			{
				// �����Z���̓X�L�b�v�i���Ƀ`�F�b�N�ς݁j
				if (dx == 0 && dz == 0)
					continue;

				// �אڃZ���̃L�[
				GridKey neighborKey;
				neighborKey.x = currentKey.x + dx;
				neighborKey.z = currentKey.z + dz;

				// �אڃZ�������݂��邩�m�F
				auto it = m_spatialGrid.find(neighborKey);
				if (it == m_spatialGrid.end())
					continue;  // �Z������

				std::vector<size_t>& neighborsInCell = it->second;

				// ���݂̃Z���̓G vs �אڃZ���̓G
				for (size_t i : enemiesInCell)
				{
					Enemy& enemyA = m_enemies[i];

					if (enemyA.currentAnimation == "Attack")
						continue;

					for (size_t j : neighborsInCell)
					{
						Enemy& enemyB = m_enemies[j];

						if (enemyB.currentAnimation == "Attack")
							continue;

						float dx = enemyB.position.x - enemyA.position.x;
						float dz = enemyB.position.z - enemyA.position.z;
						float distance = sqrtf(dx * dx + dz * dz);

						if (distance < MIN_DISTANCE && distance > 0.001f)
						{
							float nx = dx / distance;
							float nz = dz / distance;
							float overlap = MIN_DISTANCE - distance;
							float pushDistance = overlap * PUSH_STRENGTH;

							enemyA.position.x -= nx * pushDistance;
							enemyA.position.z -= nz * pushDistance;

							enemyB.position.x += nx * pushDistance;
							enemyB.position.z += nz * pushDistance;
						}
					}
				}
			}
		}
	}
}


// === �֐�4: �X�|�[���ʒu���L�����`�F�b�N ===
bool EnemySystem::IsPositionValid(const DirectX::XMFLOAT3& position, float minDistance) const
{
	// �S�Ă̊����̓G�Ƃ̋������`�F�b�N
	for (const auto& existingEnemy : m_enemies)
	{
		// ����ł�G�⎀�S���̓G�͖���
		if (!existingEnemy.isAlive || existingEnemy.isDying)
			continue;

		// �������v�Z
		float dx = position.x - existingEnemy.position.x;
		float dz = position.z - existingEnemy.position.z;
		float distance = sqrtf(dx * dx + dz * dz);

		// �߂�����ꍇ��false
		if (distance < minDistance)
			return false;
	}

	// �S�Ă̓G����\������Ă�����true
	return true;
}

// =============================================================
// �{�XAI��ԃ}�V���iMIDBOSS / BOSS ���ʁj
// =============================================================
void EnemySystem::UpdateBossAI(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime)
{
	// --- �v���C���[�ւ̕����Ƌ������v�Z ---
	float dx = playerPos.x - enemy.position.x;
	float dz = playerPos.z - enemy.position.z;
	float dist = sqrtf(dx * dx + dz * dz);

	// --- ��Ƀv���C���[�̕������� ---
	if (dist > 0.1f)
	{
		enemy.rotationY = atan2f(dx, dz) + 3.14159f;
	}
	
	// --- �t�F�[�Y�^�C�}�[�i�s ---
	enemy.bossPhaseTimer += deltaTime;

	// --- ��ԃ}�V�� ---
	switch (enemy.bossPhase)
	{
		// ============================================
		// IDLE�F�v���C���[�ɋ߂Â� �� ����or���ԂōU���J�n
		// ============================================
	case BossAttackPhase::IDLE:
	{
		// �N�[���_�E�����͋߂Â�����
		enemy.bossAttackCooldown -= deltaTime;

		// �v���C���[�Ɍ������ĕ���
		if (dist > 3.0f && enemy.currentAnimation != "Attack" && enemy.currentAnimation != "AttackSlash")
		{
			float speed = 1.5f;  // �{�X�̕��s���x
			if (enemy.type == EnemyType::MIDBOSS) speed = 2.5f;

			float nx = dx / dist;  // ���K��
			float nz = dz / dist;
			enemy.velocity.x = nx * speed;
			enemy.velocity.z = nz * speed;

			float finalSpeed = ((enemy.type == EnemyType::BOSS) ? 1.0f : 0.4f) * m_waveSpeedMult;
			enemy.position.x += enemy.velocity.x * finalSpeed * deltaTime;
			enemy.position.z += enemy.velocity.z * finalSpeed * deltaTime;

			if (enemy.currentAnimation != "Walk")
			{
				enemy.currentAnimation = "Walk";
				enemy.animationTime = 0.0f;
			}
		}
		else
		{
			enemy.velocity = { 0, 0, 0 };
			//  �ߐڍU����("Attack")��Idle�ɖ߂��Ȃ�
			if (enemy.currentAnimation != "Idle" && enemy.currentAnimation != "Attack" && enemy.currentAnimation != "AttackSlash")
			{
				enemy.currentAnimation = "Idle";
				enemy.animationTime = 0.0f;
			}
		}

		// --- �U���I�� ---
		// �N�[���_�E�����I����Ă��āA������x�߂�
		if (enemy.bossAttackCooldown <= 0.0f && dist > 5.0f && dist < 30.0f) 
		{
			enemy.bossTargetPos = playerPos;  // �U�����̃^�[�Q�b�g�ʒu���L�^
			enemy.bossAttackCount++;

			if (enemy.type == EnemyType::BOSS)
			{
				// BOSS: �W�����v�@���� �� �a�� �������_��
				int pattern = rand() % 2;  // 0=�W�����v, 1=�a��
				if (pattern == 0)
				{
					enemy.bossPhase = BossAttackPhase::JUMP_WINDUP;
					enemy.bossPhaseTimer = 0.0f;
					enemy.currentAnimation = "AttackJump";
					enemy.animationTime = 0.0f;

					//OutputDebugStringA("[BOSS AI] -> JUMP_WINDUP (random)\n");
				}
				else
				{
					enemy.bossPhase = BossAttackPhase::SLASH_WINDUP;
					enemy.bossPhaseTimer = 0.0f;
					enemy.currentAnimation = "AttackSlash";
					enemy.animationTime = 0.0f;

					//OutputDebugStringA("[BOSS AI] -> SLASH_WINDUP (random)\n");
				}
			}
			else // MIDBOSS
			{
				// MIDBOSS: ���K�r�[�� �� �W�����v�@���� ������
				if (enemy.bossAttackCount % 2 == 1)
				{
					enemy.bossPhase = BossAttackPhase::ROAR_WINDUP;
					enemy.bossPhaseTimer = 0.0f;
					enemy.currentAnimation = "Attack";
					enemy.animationTime = 0.0f;
					enemy.bossBeamParriable = (rand() % 2 == 0);

					char buf[64];
					sprintf_s(buf, "[MIDBOSS AI] -> ROAR_WINDUP (%s)\n",
						enemy.bossBeamParriable ? "GREEN" : "RED");
					//OutputDebugStringA(buf);
				}
				else
				{
					enemy.bossPhase = BossAttackPhase::JUMP_WINDUP;
					enemy.bossPhaseTimer = 0.0f;
					enemy.currentAnimation = "Attack";
					enemy.animationTime = 0.0f;

					//OutputDebugStringA("[MIDBOSS AI] -> JUMP_WINDUP\n");
				}
			}
		}

		// �ߐڍU���i�ʏ퉣��j- �v���C���[���߂��ꍇ
		// �ߐڍU�� - �v���C���[���߂��ꍇ
		// �ߐڍU�� - �v���C���[���߂��ꍇ
		{
			const char* meleeAnim = (enemy.type == EnemyType::BOSS) ? "AttackSlash" : "Attack";
			bool isInMelee = (enemy.currentAnimation == meleeAnim);

			// �U���J�n����: �߂��A�܂��͊��ɍU�����i�r���Œ��f�����Ȃ��j
			if ((dist < 3.0f || isInMelee) && enemy.bossPhase == BossAttackPhase::IDLE)
			{
				enemy.touchingPlayer = (dist < 3.0f);  // �_���[�W����p�͎�����

				// ����̂݃A�j���؂�ւ��{���Z�b�g
				if (!isInMelee)
				{
					enemy.currentAnimation = meleeAnim;
					enemy.animationTime = 0.0f;
					enemy.attackJustLanded = false;
				}

				// �A�j���^�C�}�[�i�s
				float attackHitTime = m_bossMeleeHitTime;
				float attackDuration = m_bossMeleeDuration;
				float attackAnimSpeed = m_animSpeed_Attack[(enemy.type == EnemyType::BOSS) ? 4 : 3];

				float prevTime = enemy.animationTime;
				enemy.animationTime += deltaTime * attackAnimSpeed;

				if (prevTime < attackHitTime && enemy.animationTime >= attackHitTime)
					enemy.attackJustLanded = true;
				else
					enemy.attackJustLanded = false;

				// �A�j������ �� ����Ă���Walk�ɖ߂�
				if (enemy.animationTime >= attackDuration)
				{
					enemy.animationTime = 0.0f;
					enemy.attackJustLanded = false;

					if (dist > 3.0f)
					{
						enemy.currentAnimation = "Walk";
						enemy.touchingPlayer = false;
					}
					// �߂���΂��̂܂܃��[�v
				}

				enemy.velocity = { 0, 0, 0 };
			}
			else if (enemy.bossPhase == BossAttackPhase::IDLE && !isInMelee)
			{
				enemy.touchingPlayer = false;
				enemy.attackJustLanded = false;
			}
		}

		break;
	}


	// ============================================
	// �W�����v�@�����iBOSS & MIDBOSS���ʁj
	// ============================================
	case BossAttackPhase::JUMP_WINDUP:
	{
		// 0.5�b�̗��߃��[�V�����i�n�ʂɗ͂����߂�j
		enemy.velocity = { 0, 0, 0 };

		if (enemy.bossPhaseTimer >= m_bossJumpWindupTime)
		{
			enemy.bossPhase = BossAttackPhase::JUMP_AIR;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossTargetPos = playerPos;  // �W�����v����X�V enemy.bossJumpStartPos = enemy.position;
			enemy.bossJumpStartPos = enemy.position;

			//OutputDebugStringA("[BOSS AI] -> JUMP_AIR\n");
		}
		break;
	}

	case BossAttackPhase::JUMP_AIR:
	{
		float jumpDuration = m_bossJumpAirTime;
		float progress = enemy.bossPhaseTimer / jumpDuration;
		if (progress > 1.0f) progress = 1.0f;

		//  �������isin �ŎR�Ȃ�j
		enemy.bossJumpHeight = sinf(progress * 3.14159f) * m_bossJumpHeight;
		enemy.position.y = enemy.bossJumpStartPos.y + enemy.bossJumpHeight;

		//  �����ړ�: �J�n�ʒu���^�[�Q�b�g�𒼐����
		//    progress��0��1�ŁA���傤�ǃ^�[�Q�b�g�ɓ��B����
		enemy.position.x = enemy.bossJumpStartPos.x +
			(enemy.bossTargetPos.x - enemy.bossJumpStartPos.x) * progress;
		enemy.position.z = enemy.bossJumpStartPos.z +
			(enemy.bossTargetPos.z - enemy.bossJumpStartPos.z) * progress;

		//  �v���C���[�̕�������
		float dx = enemy.bossTargetPos.x - enemy.position.x;
		float dz = enemy.bossTargetPos.z - enemy.position.z;
		if (dx * dx + dz * dz > 0.01f)
			enemy.rotationY = atan2f(dx, dz) + 3.14159f;

		if (enemy.bossPhaseTimer >= jumpDuration)
		{
			enemy.bossPhase = BossAttackPhase::JUMP_SLAM;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossJumpHeight = 0.0f;
			enemy.position.y = enemy.bossJumpStartPos.y;
			enemy.attackJustLanded = true;
		}
		break;
	}

	case BossAttackPhase::JUMP_SLAM:
	{

		if (enemy.bossPhaseTimer >= 0.1f)
		{
			enemy.attackJustLanded = false;
			enemy.bossPhase = BossAttackPhase::SLAM_RECOVERY;
			enemy.bossPhaseTimer = 0.0f;

			//OutputDebugStringA("[BOSS AI] -> SLAM_RECOVERY\n");
		}
		break;
	}

	case BossAttackPhase::SLAM_RECOVERY:
	{
		// 1.5�b�̍d���i�v���C���[�̍U���`�����X�j
		enemy.velocity = { 0, 0, 0 };

		if (enemy.currentAnimation != "Idle")
		{
			enemy.currentAnimation = "Idle";
			enemy.animationTime = 0.0f;
		}

		if (enemy.bossPhaseTimer >= m_bossSlamRecoveryTime)
		{
			enemy.bossPhase = BossAttackPhase::IDLE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossAttackCooldown = m_bossAttackCooldown;  // ���̍U���܂�3�b

			//OutputDebugStringA("[BOSS AI] -> IDLE (cooldown)\n");
		}
		break;
	}

	// ============================================
	// �a��3�{��΂��iBOSS�j
	// ============================================
	case BossAttackPhase::SLASH_WINDUP:
	{
		// 0.8�b�̉���グ���[�V����
		enemy.velocity = { 0, 0, 0 };

		if (enemy.bossPhaseTimer >= m_bossSlashWindupTime)
		{
			enemy.bossPhase = BossAttackPhase::SLASH_FIRE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossTargetPos = playerPos;  // ���˕������m��

			// // game.cpp��3�{�̃v���W�F�N�^�C���𐶐�����
			//   (�t���O�Œʒm)
			enemy.attackJustLanded = true;

			//OutputDebugStringA("[BOSS AI] -> SLASH_FIRE!\n");
		}
		break;
	}

	case BossAttackPhase::SLASH_FIRE:
	{
		// ���˂̏u�ԁi0.2�b�j
		if (enemy.bossPhaseTimer >= m_bossSlashFireTime)
		{
			enemy.attackJustLanded = false;
			enemy.bossPhase = BossAttackPhase::SLASH_RECOVERY;
			enemy.bossPhaseTimer = 0.0f;

			//OutputDebugStringA("[BOSS AI] -> SLASH_RECOVERY\n");
		}
		break;
	}

	case BossAttackPhase::SLASH_RECOVERY:
	{
		// 1.0�b�̍d��
		enemy.velocity = { 0, 0, 0 };

		if (enemy.currentAnimation != "Idle")
		{
			enemy.currentAnimation = "Idle";
			enemy.animationTime = 0.0f;
		}

		if (enemy.bossPhaseTimer >= m_bossSlashRecoveryTime)
		{
			enemy.bossPhase = BossAttackPhase::IDLE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossAttackCooldown = m_bossAttackCooldown;

			//OutputDebugStringA("[BOSS AI] -> IDLE (cooldown)\n");
		}
		break;
	}

	// ============================================
	// ���K�r�[���iMIDBOSS�j
	// ============================================
	case BossAttackPhase::ROAR_WINDUP:
	{
		//  �`���[�W����Attack�A�j�����ێ�
		if (enemy.currentAnimation != "Attack")
		{
			enemy.currentAnimation = "Attack";
		}

		// 1.0�b�̙��K���߁i�����J���鉉�o�j
		enemy.velocity = { 0, 0, 0 };

		if (enemy.bossPhaseTimer >= 1.0f)
		{
			enemy.bossPhase = BossAttackPhase::ROAR_FIRE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossTargetPos = playerPos;

			enemy.attackJustLanded = true;

			//OutputDebugStringA("[MIDBOSS AI] -> ROAR_FIRE!\n");
		}
		break;
	}

	case BossAttackPhase::ROAR_FIRE:
	{
		if (enemy.currentAnimation != "Attack")
			enemy.currentAnimation = "Attack";

		// �������v���C���[��Ǐ]
		float trackSpeed = 0.5f * deltaTime;
		enemy.bossTargetPos.x += (playerPos.x - enemy.bossTargetPos.x) * trackSpeed;
		enemy.bossTargetPos.z += (playerPos.z - enemy.bossTargetPos.z) * trackSpeed;

		float bdx = enemy.bossTargetPos.x - enemy.position.x;
		float bdz = enemy.bossTargetPos.z - enemy.position.z;
		if (fabs(bdx) > 0.01f || fabs(bdz) > 0.01f)
		{
			enemy.rotationY = atan2f(bdx, bdz) + 3.14159f;
		}

		// 5.0�b�ԁi�`���[�W2.5s + ����5.0s = �S��7.5s�œ͂��j
		if (enemy.bossPhaseTimer >= m_bossRoarFireTime)
		{
			enemy.attackJustLanded = false;
			enemy.bossPhase = BossAttackPhase::ROAR_RECOVERY;
			enemy.bossPhaseTimer = 0.0f;
			//OutputDebugStringA("[MIDBOSS AI] -> ROAR_RECOVERY\n");
		}
		break;
	}

	case BossAttackPhase::ROAR_RECOVERY:
	{
		// 2.0�b�̍d���i�傫�ȍU���`�����X�j
		enemy.velocity = { 0, 0, 0 };

		if (enemy.currentAnimation != "Idle")
		{
			enemy.currentAnimation = "Idle";
			enemy.animationTime = 0.0f;
		}

		if (enemy.bossPhaseTimer >= m_bossRoarRecoveryTime)
		{
			enemy.bossPhase = BossAttackPhase::IDLE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossAttackCooldown = m_bossAttackCooldown;

			//OutputDebugStringA("[MIDBOSS AI] -> IDLE (cooldown)\n");
		}
		break;
	}

	} // switch end

	// --- �}�b�v���E�`�F�b�N ---
	if (enemy.position.x < -50.0f) enemy.position.x = -50.0f;
	if (enemy.position.x > 50.0f) enemy.position.x = 50.0f;
	if (enemy.position.z < -50.0f) enemy.position.z = -50.0f;
	if (enemy.position.z > 50.0f) enemy.position.z = 50.0f;
}


// �f�o�b�O�p�F�w��^�C�v�̓G��1�̃X�|�[��
void EnemySystem::SpawnEnemyOfType(EnemyType type, DirectX::XMFLOAT3 playerPos)
{
	Enemy enemy;
	enemy.id = m_nextEnemyID++;
	enemy.type = type;

	// �^�C�v�ʃX�e�[�^�X
	switch (type)
	{
	case EnemyType::RUNNER:
		enemy.health = 50;
		enemy.maxHealth = 50;
		break;
	case EnemyType::TANK:
		enemy.health = 300;
		enemy.maxHealth = 300;
		break;
	default: // NORMAL
		enemy.health = 100;
		enemy.maxHealth = 100;
		break;
	}

	enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// �v���C���[�̐���5m�ɃX�|�[��
	float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
	float distance = 5.0f;
	enemy.position.x = playerPos.x + cosf(angle) * distance;
	enemy.position.z = playerPos.z + sinf(angle) * distance;

	enemy.velocity = { 0.0f, 0.0f, 0.0f };
	enemy.moveTimer = 99.0f;
	enemy.isAlive = true;

	enemy.currentAnimation = "Idle";
	enemy.animationTime = ((float)rand() / RAND_MAX) * 1.0f;

	enemy.justSpawned = true;

	m_enemies.push_back(enemy);

	char buf[128];
	sprintf_s(buf, "[DEBUG SPAWN] Type:%d ID:%d\n", (int)type, enemy.id);
	//OutputDebugStringA(buf);
}