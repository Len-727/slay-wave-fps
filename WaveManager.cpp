//	WavaManager.cpp	-	ウェーブ管理システムの実装
#include "WaveManager.h"
#include <algorithm>	//	min, max

//	コンストラクタ
//	【役割】ウェーブ1, 敵６体,準備時間5秒スタート
WaveManager::WaveManager() :
	m_currentWave(1),
	m_enemiesPerWave(6),
	m_enemiesKilledThisWave(0),
	m_totalEnemiesThisWave(6),
	m_betweenWaves(true),
	m_waveStartTimer(5.0f),
	m_enemySpawnTimer(0.0f)
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
			//	新ウェーブ開始
			m_betweenWaves = false;
			m_enemiesKilledThisWave = 0;
			m_totalEnemiesThisWave = m_enemiesPerWave;

			//	初期スポーン(2 - 4体)
			//	【計算】Wave1:2体, Wave2:3体, Wave3以降:4体
			int initialSpawn = std::min(2 + (m_currentWave - 1), 4);
			//	【制限】総数を超えないように
			initialSpawn = std::min(initialSpawn, m_totalEnemiesThisWave);

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
			//	【計算】Wave1:2秒、Wave10:1秒、Wave16以降:0.5秒
			float spawnInterval = 2.0f - (m_currentWave * 0.1f);
			spawnInterval = std::max(0.5f, spawnInterval);

			if (m_enemySpawnTimer >= spawnInterval)
			{
				enemySystem->SpawnEnemy(playerPos);
				m_enemySpawnTimer = 0.0f;
			}
		}
	}
}

//	OnEnemyKilled	-	敵を倒したときに呼ぶ
//	【戻り値】ウェーブクリアボーナス(0 or 100)
int WaveManager::OnEnemyKilled()
{
	m_enemiesKilledThisWave++;

	//	ウェーブクリア判定
	if (m_enemiesKilledThisWave >= m_totalEnemiesThisWave)	//	このウェーブで倒した敵が総敵数以上になったら
	{
		m_currentWave++;

		//	敵の数を増やす(毎ウェーブ３体)
		//	【計算】Wave1:6体、Wave2:9体、Wave3:12体
		m_enemiesPerWave = 6 + (m_currentWave - 1) * 3;

		//	準備時間に入る
		m_betweenWaves = true;
		m_waveStartTimer = 10.0f;	//	10秒の準備時間

		return 100;
	}

	return 0;	//	まだクリアしていない
}