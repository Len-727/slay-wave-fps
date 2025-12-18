//	EnemySystem.h

#pragma once
#include "Entities.h"
#include <vector>
#include <DirectXMath.h>

class EnemySystem {
public:
	//	敵の配列を空にして、スポーンタイマーなどを初期化
	//	コンストラクタ
	EnemySystem();

	// 【役割】全ての敵の移動、プレイヤーへの接近などを処理
	// 【引数】deltaTime: 前フレームからの経過時間（秒）
	//        playerPos: プレイヤーの現在位置（敵が追いかけるため）
	void Update(float deltaTime, DirectX::XMFLOAT3 playerPos);

	// SpawnEnemy - 新しい敵を1体生成
	// 【役割】ランダムな位置に敵を出現させる
	// 【引数】playerPos: プレイヤー位置（プレイヤーから離れた場所にスポーン）
	// 【なぜpublicか】Gameクラスから「今敵を出せ！」と命令するため
	void SpawnEnemy(DirectX::XMFLOAT3 playerPos);

	// ClearDeadEnemies - 死んだ敵を配列から削除
	// 【役割】isAlive == falseの敵をm_enemiesから消す
	// 【理由】死んだ敵を残すとメモリの無駄＆処理が重くなる
	void ClearDeadEnemies();

	//	ゲッター (情報を読み取る関数）

	// GetEnemies (const版) - 敵の配列を読み取り専用で取得
	// 【用途】描画時など「見るだけ」の処理で使う
	// 【戻り値】const std::vector<Enemy>&
	//   - const: 変更不可（安全）
	//   - &: 参照（コピーしないので高速）
	const std::vector<Enemy>& GetEnemies() const { return m_enemies; }

	// GetEnemies (非const版) - 敵の配列を変更可能な形で取得
    // 【用途】射撃時にダメージを与えるなど「変更する」処理で使う
    // 【戻り値】std::vector<Enemy>&
    //   - &: 参照（本物を触る）
    //   - constなし: 変更可能
    // 【なぜ2つ必要か】用途に応じて使い分けることで、バグを防ぐ
	std::vector<Enemy>& GetEnemies() { return m_enemies; }	//	非const版（ダメージ処理用）

	//	最大的数を取得　「これ以上敵をださない」という上限をチェック
	int GetMaxEnemies() const { return m_maxEnemies; }

	//	ウェーブ管理用	ウェーブが進むと敵の上限を増やすなどで使う
	void SeMaxEnemies(int max) { m_maxEnemies = max; }

private:
	// UpdateEnemyMovement - 1体の敵の移動を更新
	// 【役割】敵をプレイヤーに向かって動かす
	// 【引数】enemy: 動かす敵（参照で渡すので本物を変更）
	//        playerPos: プレイヤー位置
	//        deltaTime: 経過時間
	// 【なぜprivateか】外部から直接呼ぶ必要がない内部処理
	void UpdateEnemyMovement(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime);

	std::vector<Enemy> m_enemies;	//	マップにいるすべての敵を格納
	int m_maxEnemies;				//	同時に存在できる敵の最大数
	float m_enemySpawnTimer;		//	敵の自動スポーン用タイマー
	int m_nextEnemyID;				//	次に割り当てるID
};