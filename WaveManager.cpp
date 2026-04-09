//	WavaManager.cpp	-	�E�F�[�u�Ǘ��V�X�e���̎���
#define NOMINMAX
#include <windows.h>
#include "WaveManager.h"
#include <algorithm>	//	min, max
#include <cmath>		//	pow (�ݏ�v�Z�p)

//	�R���X�g���N�^
//	�y�����z�E�F�[�u1�A�G10�́A��������3�b�ŃX�^�[�g
WaveManager::WaveManager() :
	m_currentWave(3),
	m_enemiesKilledThisWave(0),
	m_totalEnemiesThisWave(10),		//	Wave1��10��
	m_betweenWaves(true),
	m_waveStartTimer(3.0f),			//	3�b�̏�������
	m_enemySpawnTimer(0.0f),
	m_baseEnemyCount(20),			//	��{�G��: 10��
	m_difficultyScale(1.3f)			//	��Փx�X�P�[��: 1.2�{������
{
}

//	Update	-	�E�F�[�u�i�s�Ǘ�
void WaveManager::Update(float deltaTime, DirectX::XMFLOAT3 playerPos, EnemySystem* enemySystem)
{
	// �ꎞ��~���̓X�|�[�����Ȃ�
	if (m_paused) return;

	if (m_betweenWaves)
	{
		//	===	�E�F�[�u�Ԃ̏�������	===
		m_waveStartTimer -= deltaTime;

		if (m_waveStartTimer <= 0)
		{
			m_betweenWaves = false;
			m_enemiesKilledThisWave = 0;

			//  �E�F�[�u��Փx�X�P�[�����O�K�p
			enemySystem->SetWaveScaling(m_currentWave);

			m_totalEnemiesThisWave = GetEnemyCountForWave(m_currentWave);

			m_totalEnemiesThisWave = GetEnemyCountForWave(m_currentWave);

			// MIDBOSS = 3�̔{���i������10�̔{���͏����j
			bool isMidBossWave = (m_currentWave % 2 == 0) && (m_currentWave % 10 != 0);
			// BOSS = 10�̔{��
			bool isBossWave = (m_currentWave % 3 == 0);

			if (isMidBossWave)
			{
				m_totalEnemiesThisWave += 1;
			}
			if (isBossWave)
			{
				m_totalEnemiesThisWave += 1;
			}

			char buffer[128];
			sprintf_s(buffer, "[WAVE] Wave %d started! Total enemies: %d %s%s\n",
				m_currentWave, m_totalEnemiesThisWave,
				isMidBossWave ? "(MIDBOSS!)" : "",
				isBossWave ? "(BOSS!!)" : "");
			OutputDebugStringA(buffer);

			float hpMultiplier = GetDifficultyMultiplier();

			if (isMidBossWave)
			{
				enemySystem->SpawnMidBoss(playerPos);
				m_midBossJustSpawned = true;
			}
			if (isBossWave)
			{
				enemySystem->SpawnBoss(playerPos);
				m_bossJustSpawned = true;
			}

			int initialSpawn = std::min(12, m_totalEnemiesThisWave);

			for (int i = 0; i < initialSpawn; i++)
			{
				enemySystem->SpawnEnemy(playerPos);
			}
		}
	}
	else
	{
		//	===	�E�F�[�u���̃X�|�[���Ǘ�	===

		//	���݂̐����Ă���G�̐����J�E���g
		int currentEnemyCount = 0;
		for (const auto& enemy : enemySystem->GetEnemies())
		{
			if (enemy.isAlive) currentEnemyCount++;
		}

		//	�X�|�[�����K�v������
		//	�y����1�z�܂��S���o�Ă��Ȃ�
		//	�y����2�z�G�̐�������ȉ�
		int totalSpawned = m_enemiesKilledThisWave + currentEnemyCount;
		if (totalSpawned < m_totalEnemiesThisWave && currentEnemyCount < enemySystem->GetMaxEnemies())
		{
			m_enemySpawnTimer += deltaTime;

			//	�E�F�[�u���i�ނɂ�ăX�|�[�����x�A�b�v
			//	�y�v�Z���z2.0�b �� 1.5�b �� 1.0�b �� 0.5�b�i�ő��j
			float spawnInterval = 2.0f - (m_currentWave * 0.15f);
			spawnInterval = std::max(0.5f, spawnInterval);  // �ő�0.5�b

			if (m_enemySpawnTimer >= spawnInterval)
			{
				int spawnCount = std::min(5, m_totalEnemiesThisWave - (m_enemiesKilledThisWave + currentEnemyCount));
				for (int i = 0; i < spawnCount; i++)
				{
					enemySystem->SpawnEnemy(playerPos);
				}
				m_enemySpawnTimer = 0.0f;
			}
		}
	}
}

//	OnEnemyKilled	-	�G��|�����Ƃ��ɌĂ�
//	�y�߂�l�z�E�F�[�u�N���A�{�[�i�X�i0 or 100�j
int WaveManager::OnEnemyKilled()
{
	m_enemiesKilledThisWave++;

	//	�E�F�[�u�N���A����
	if (m_enemiesKilledThisWave >= m_totalEnemiesThisWave)
	{
		m_currentWave++;

		//	�f�o�b�O���O
		char buffer[128];
		sprintf_s(buffer, "[WAVE] Wave %d completed!\n", m_currentWave - 1);
		OutputDebugStringA(buffer);

		//	�������Ԃɓ���
		m_betweenWaves = true;
		m_waveStartTimer = 3.0f;	// 3�b�̏�������

		return 100;	// �E�F�[�u�N���A�{�[�i�X
	}

	return 0;	// �܂��N���A���Ă��Ȃ�
}

//	=== �V�@�\: ���I�ȓG���v�Z	===

//	GetEnemyCountForWave - �w��Wave�̓G�����v�Z
//	�y�v�Z���zbaseEnemyCount * (difficultyScale ^ (wave - 1))
//	�y��zWave1=10��, Wave2=12��, Wave3=14��, Wave5=20��
int WaveManager::GetEnemyCountForWave(int wave) const
{
	//	�ݏ�v�Z: 10 * (1.2 ^ (wave - 1))
	float enemyCount = (float)m_baseEnemyCount * powf(m_difficultyScale, (float)(wave - 1));

	//	�����ɕϊ��i�l�̌ܓ��j
	return (int)(enemyCount + 0.5f);
}

//	GetDifficultyMultiplier - ��Փx�{�����擾
//	�y�v�Z���z1.0 + (currentWave - 1) * 0.1
//	�y��zWave1=1.0�{, Wave5=1.4�{, Wave10=1.9�{
float WaveManager::GetDifficultyMultiplier() const
{
	return 1.0f + ((m_currentWave - 1) * 0.1f);
}

//	IsVictoryWave - ���݂��{�XWave������
//	�y���[���z10Wave���ƂɃ{�X��
//	�y��zWave10, Wave20, Wave30... �� true
bool WaveManager::IsVictoryWave() const
{
	return (m_currentWave % 10 == 0);
}

void WaveManager::Reset()
{
	m_currentWave = 3;
	m_enemiesKilledThisWave = 0;
	m_totalEnemiesThisWave = 10;
	m_betweenWaves = true;
	m_waveStartTimer = 3.0f;
	m_enemySpawnTimer = 0.0f;
}