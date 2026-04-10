#pragma once

#include <DirectXMath.h>	//	XMFLOAT3
#include "EnemySystem.h"

//	ウェーブ管理クラス
class WaveManager {
public:
	//	コンストラクタ
	//	【役割】初期値を設定（ウェーブ1、敵10体など）
	WaveManager();

	//	メイン処理

	//	Update - ウェーブの進行管理
	//	【役割】準備時間・スポーン・ウェーブクリア判定
	//	【引数】deltaTime: 経過時間
	//		   playerPos: プレイヤー位置
	//		   enemySystem: 敵システム（スポーン使用）
	void Update(float deltaTime, DirectX::XMFLOAT3 playerPos, EnemySystem* enemySystem);

	//	===	敵撃破通知	===

	//	OnEnemyKilled - 敵を倒したときに呼ぶ
	//	【役割】撃破数をカウント、ウェーブクリア判定
	//	【戻り値】ウェーブクリアボーナスポイント（0 or 100）
	int OnEnemyKilled();

	//	===	ゲッター	===

	//	GetCurrentWave	-	現在のウェーブ番号
	int GetCurrentWave() const { return m_currentWave; }

	//	GetEnemyCountForWave - 指定Waveの敵数を計算
	//	【役割】Wave番号から敵数を自動計算
	//	【引数】wave: Wave番号（1, 2, 3...）
	//	【戻り値】そのWaveの敵数
	//	【計算式】baseEnemyCount * (difficultyScale ^ (wave - 1))
	//	【例】Wave1=10体, Wave2=12体, Wave3=14体...
	int GetEnemyCountForWave(int wave) const;

	//	GetDifficultyMultiplier - 現在のWaveの難易度倍率
	//	【役割】敵のHP・ダメージ倍率を取得
	//	【戻り値】難易度倍率（1.0 ～ 3.0）
	//	【計算式】1.0 + (currentWave - 1) * 0.1
	//	【例】Wave1=1.0倍, Wave5=1.4倍, Wave10=1.9倍
	float GetDifficultyMultiplier() const;

	//	IsVictoryWave - 現在がボスWaveか判定
	//	【役割】10Waveごとにボス戦かどうか
	//	【戻り値】true=ボスWave, false=通常Wave
	//	【例】Wave10, Wave20, Wave30... → true
	bool IsVictoryWave() const;

	void Reset();


	void SetPaused(bool paused) { m_paused = paused; }
	bool IsPaused() const { return m_paused; }

	// game.cpp から確認＆リセット用
	bool DidMidBossSpawn() { bool r = m_midBossJustSpawned; m_midBossJustSpawned = false; return r; }
	bool DidBossSpawn() { bool r = m_bossJustSpawned;    m_bossJustSpawned = false;    return r; }

private:
	bool m_paused = false;

	//	===	ウェーブ状態	===
	int	m_currentWave;				//	現在のウェーブ番号
	int m_enemiesKilledThisWave;	//	このウェーブで倒した数
	int m_totalEnemiesThisWave;		//	ウェーブの総敵数
	bool m_betweenWaves;			//	ウェーブ間かどうか
	float m_waveStartTimer;			//	次ウェーブまでの時間
	float m_enemySpawnTimer;		//	敵スポーン用タイマー

	int m_baseEnemyCount;			//	基本敵数（Wave1の敵数）

	// ボススポーン通知フラグ
	bool m_midBossJustSpawned = false;
	bool m_bossJustSpawned = false;

	float m_difficultyScale;		//	難易度スケール（Wave増加率）
};