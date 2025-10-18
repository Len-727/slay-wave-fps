//	WeveManager.h	-	ウェーブ管理システム
//	【目的】	敵のスポーン・ウェーブ進行を管理

#pragma once

#include <DirectXMath.h>	//	XMFLOAT3
#include "EnemySystem.h"

//	ウェーブ管理クラス
class WaveManager {
public:
	//	コンストラクタ
	//	【役割】初期値を設定(ウェーブ1、敵６隊など)
	WaveManager();

	//	メイン処理

	//	Update - ウェーブの進行管理
	//	【役割】準備時間・スポーン・ウェーブクリア判定
	//	【引数】deltaTime: 経過時間
	//		   playeerPos: プレイヤー位置
	//		   enemySystem: 敵システム(スポーン使用)
	void Update(float deltaTime, DirectX::XMFLOAT3 playerPos, EnemySystem* enemySystem);

	//	===	敵撃破通知	===

	//	OnEnemyKilled - 敵を倒したときに呼ぶ
	//	【役割】撃破数をカウント、ウェーブクリア判定
	//	【戻り値】ウェーブクリアボーナスポイント(0 - 100)
	int OnEnemyKilled();

	//	===	ゲッター	===

	//	GetCurrentWave	-	現在のウェーブ番号
	int GetCurrentWave() const { return m_currentWave; }

private:
	//	===	ウェーブ状態	===
	int	m_currentWave;				//	現在のウェーブ番号
	int m_enemiesPerWave;			//	このウェーブの総敵数
	int m_enemiesKilledThisWave;	//	このウェーブで倒した数
	int m_totalEnemiesThisWave;		//	ウェーブの敵総数
	bool m_betweenWaves;			//	ウェーブ間かどうか
	float m_waveStartTimer;			//	ちぎウェーブまでの時間
	float m_enemySpawnTimer;		//	敵スポーン用タイマー
};