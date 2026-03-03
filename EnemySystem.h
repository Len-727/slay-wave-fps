//	EnemySystem.h

#pragma once
#include "Entities.h"
#include <vector>
#include <DirectXMath.h>
#include <unordered_map>
#include <algorithm>

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

    void SpawnMidBoss(DirectX::XMFLOAT3 playerPos);
    void SpawnBoss(DirectX::XMFLOAT3 playerPos);

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

    std::vector<Enemy>& GetEnemiesMutable() { return m_enemies; }

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

    // アタックタイミング　IMGui
    float m_normalAttackHitTime = 1.22f;
    float m_normalAttackDuration = 3.83f;
    float m_runnerAttackHitTime = 1.2f;
    float m_runnerAttackDuration = 2.20f;
    float m_tankAttackHitTime = 1.2f;
    float m_tankAttackDuration = 3.17f;

    // デバッグ用：指定タイプの敵を1体スポーン
    void SpawnEnemyOfType(EnemyType type, DirectX::XMFLOAT3 playerPos);

    // アニメーション速度（タイプ×アニメ別）
    // [0]=NORMAL [1]=RUNNER [2]=TANK [3]=MIDBOSS [4]=BOSS
    float m_animSpeed_Idle[5] = { 1.0f, 1.20f, 1.0f, 1.0f, 1.0f };
    float m_animSpeed_Walk[5] = { 1.0f, 1.20f, 1.0f, 1.0f, 1.0f };
    float m_animSpeed_Run[5] = { 1.0f, 1.20f, 1.0f, 1.0f, 1.0f };
    float m_animSpeed_Attack[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.00f };
    float m_animSpeed_Death[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };


    //  ウェーブ難易度スケーリング
    float m_waveHpMult = 1.0f;  // HP倍率
    float m_waveSpeedMult = 1.0f;  // 移動速度倍率
    float m_waveDamageMult = 1.0f; // 攻撃力倍率

    void SetWaveScaling(int wave)
    {
        float w = (float)(wave - 1);
        m_waveHpMult = (std::min)(1.0f + w * 0.08f, 3.0f);  // 最大3倍（Wave26で到達）
        m_waveSpeedMult = (std::min)(1.0f + w * 0.03f, 1.8f);   // 最大1.8倍（Wave27で到達）
        m_waveDamageMult = (std::min)(1.0f + w * 0.06f, 2.5f);  // 最大2.5倍（Wave26で到達）
    }
private:
    // グリッドセルのキー（X, Z座標のペア）
    struct GridKey
    {
        int x;  // セルのX座標
        int z;  // セルのZ座標

        // 比較演算子（unordered_mapで必要）
        bool operator==(const GridKey& other) const
        {
            return x == other.x && z == other.z;
        }
    };

    // ハッシュ関数（GridKeyを数値に変換）
    struct GridKeyHash
    {
        size_t operator()(const GridKey& key) const
        {
            // 2つの整数を1つのハッシュ値に変換
            // 大きな素数を使って衝突を減らす
            return ((size_t)key.x * 73856093) ^ ((size_t)key.z * 19349663);
        }
    };

    // === 空間分割用の関数宣言 ===

    // 座標からグリッドキーを取得
    GridKey GetGridKey(const DirectX::XMFLOAT3& position) const;

    // 空間グリッドを構築（毎フレーム呼ぶ）
    void BuildSpatialGrid();

    // 空間分割を使った衝突解決
    void ResolveCollisionsSpatial();

    // スポーン位置が他の敵と重ならないかチェック
    bool IsPositionValid(const DirectX::XMFLOAT3& position, float minDistance) const;

    // === 空間分割用のメンバ変数 ===

    // セルサイズ（5m × 5m のグリッド）
    const float CELL_SIZE = 5.0f;

    // 空間ハッシュテーブル
    // キー: GridKey（セルの座標）
    // 値: vector<size_t>（このセル内の敵のインデックス）
    std::unordered_map<GridKey, std::vector<size_t>, GridKeyHash> m_spatialGrid;


	// UpdateEnemyMovement - 1体の敵の移動を更新
	// 【役割】敵をプレイヤーに向かって動かす
	// 【引数】enemy: 動かす敵（参照で渡すので本物を変更）
	//        playerPos: プレイヤー位置
	//        deltaTime: 経過時間
	// 【なぜprivateか】外部から直接呼ぶ必要がない内部処理
	void UpdateEnemyMovement(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime);

    void UpdateBossAI(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime);

	std::vector<Enemy> m_enemies;	//	マップにいるすべての敵を格納
	int m_maxEnemies;				//	同時に存在できる敵の最大数
	float m_enemySpawnTimer;		//	敵の自動スポーン用タイマー
	int m_nextEnemyID;				//	次に割り当てるID
};