//	WavaManager.cpp	-	ウェーブ管理システムの実装
#define NOMINMAX
#include <windows.h>
#include "WaveManager.h"
#include <algorithm>	//	min, max
#include <cmath>		//	pow (累乗計算用)

//	コンストラクタ
//	【役割】ウェーブ1、敵10体、準備時間3秒でスタート
WaveManager::WaveManager() :
	m_currentWave(3),
	m_enemiesKilledThisWave(0),
	m_totalEnemiesThisWave(10),		//	Wave1は10体
	m_betweenWaves(true),
	m_waveStartTimer(3.0f),			//	3秒の準備時間
	m_enemySpawnTimer(0.0f),
	m_baseEnemyCount(10),			//	基本敵数: 10体
	m_difficultyScale(1.2f)			//	難易度スケール: 1.2倍ずつ増加
{
}

//	Update	-	ウェーブ進行管理
void WaveManager::Update(float deltaTime, DirectX::XMFLOAT3 playerPos, EnemySystem* enemySystem)
{
	if (m_betweenWaves)
	{
		//	===	ウェーブ間の準備時間	===
		m_waveStartTimer -= deltaTime;

		if (m_waveStartTimer <= 0)
		{
			m_betweenWaves = false;
			m_enemiesKilledThisWave = 0;

			m_totalEnemiesThisWave = GetEnemyCountForWave(m_currentWave);

			// MIDBOSS = 3の倍数（ただし10の倍数は除く）
			bool isMidBossWave = (m_currentWave % 3 == 0) && (m_currentWave % 10 != 0);
			// BOSS = 10の倍数
			bool isBossWave = (m_currentWave % 5 == 0);

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
			}
			if (isBossWave)
			{
				enemySystem->SpawnBoss(playerPos);
			}

			int initialSpawn = std::min(5, m_totalEnemiesThisWave);

			for (int i = 0; i < initialSpawn; i++)
			{
				enemySystem->SpawnEnemy(playerPos);
			}
		}
	}
	else
	{
		//	===	ウェーブ中のスポーン管理	===

		//	現在の生きている敵の数をカウント
		int currentEnemyCount = 0;
		for (const auto& enemy : enemySystem->GetEnemies())
		{
			if (enemy.isAlive) currentEnemyCount++;
		}

		//	スポーンが必要か判定
		//	【条件1】まだ全部出ていない
		//	【条件2】敵の数が上限以下
		int totalSpawned = m_enemiesKilledThisWave + currentEnemyCount;
		if (totalSpawned < m_totalEnemiesThisWave && currentEnemyCount < enemySystem->GetMaxEnemies())
		{
			m_enemySpawnTimer += deltaTime;

			//	ウェーブが進むにつれてスポーン速度アップ
			//	【計算式】2.0秒 → 1.5秒 → 1.0秒 → 0.5秒（最速）
			float spawnInterval = 2.0f - (m_currentWave * 0.15f);
			spawnInterval = std::max(0.5f, spawnInterval);  // 最速0.5秒

			if (m_enemySpawnTimer >= spawnInterval)
			{
				enemySystem->SpawnEnemy(playerPos);
				m_enemySpawnTimer = 0.0f;
			}
		}
	}
}

//	OnEnemyKilled	-	敵を倒したときに呼ぶ
//	【戻り値】ウェーブクリアボーナス（0 or 100）
int WaveManager::OnEnemyKilled()
{
	m_enemiesKilledThisWave++;

	//	ウェーブクリア判定
	if (m_enemiesKilledThisWave >= m_totalEnemiesThisWave)
	{
		m_currentWave++;

		//	デバッグログ
		char buffer[128];
		sprintf_s(buffer, "[WAVE] Wave %d completed!\n", m_currentWave - 1);
		OutputDebugStringA(buffer);

		//	準備時間に入る
		m_betweenWaves = true;
		m_waveStartTimer = 3.0f;	// 3秒の準備時間

		return 100;	// ウェーブクリアボーナス
	}

	return 0;	// まだクリアしていない
}

//	=== 新機能: 動的な敵数計算	===

//	GetEnemyCountForWave - 指定Waveの敵数を計算
//	【計算式】baseEnemyCount * (difficultyScale ^ (wave - 1))
//	【例】Wave1=10体, Wave2=12体, Wave3=14体, Wave5=20体
int WaveManager::GetEnemyCountForWave(int wave) const
{
	//	累乗計算: 10 * (1.2 ^ (wave - 1))
	float enemyCount = (float)m_baseEnemyCount * powf(m_difficultyScale, (float)(wave - 1));

	//	整数に変換（四捨五入）
	return (int)(enemyCount + 0.5f);
}

//	GetDifficultyMultiplier - 難易度倍率を取得
//	【計算式】1.0 + (currentWave - 1) * 0.1
//	【例】Wave1=1.0倍, Wave5=1.4倍, Wave10=1.9倍
float WaveManager::GetDifficultyMultiplier() const
{
	return 1.0f + ((m_currentWave - 1) * 0.1f);
}

//	IsVictoryWave - 現在がボスWaveか判定
//	【ルール】10Waveごとにボス戦
//	【例】Wave10, Wave20, Wave30... → true
bool WaveManager::IsVictoryWave() const
{
	return (m_currentWave % 10 == 0);
}