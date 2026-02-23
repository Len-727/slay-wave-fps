//	EnemySystem.cpp	敵管理システムの実装
#include "EnemySystem.h"
#include<windows.h>
#include <Algorithm>	//	std::remmove_if(配列から要素を削除)を使うため
#include <cmath>		//	sprtf(平方根), conf, sinfを使うため

//	コンストラクタ	EnemySystemで作られた時の初期化
//	【いつ呼ばれる】Game.cppで std::make_unique<EnemySystem>()したとき
//	【役割】メンバ変数に初期値を設定
EnemySystem::EnemySystem()
	: m_maxEnemies(100)			//	最大的数：　２４体
	, m_enemySpawnTimer(0.0f)	//	スポーンタイマー:	０秒からスタート
	, m_nextEnemyID(0)
{
	//	m_enemies　は空の配列として自動初期化される
}

//	【役割】全ての敵を動かして、プレイヤーに近づける
void EnemySystem::Update(float deltaTime, DirectX::XMFLOAT3 playerPos)
{
	// === 全ての敵を一体ずつ更新 ===
	for (auto& enemy : m_enemies)
	{
		// 死んでいる敵はスキップ
		if (!enemy.isAlive)
			continue;

		// === ラグドール（吹っ飛び） ===
		if (enemy.isRagdoll)
		{
			// 位置を更新
			enemy.position.x += enemy.velocity.x * deltaTime;
			enemy.position.y += enemy.velocity.y * deltaTime;
			enemy.position.z += enemy.velocity.z * deltaTime;

			// 重力を適用（下向き加速）
			enemy.velocity.y -= 9.8f * deltaTime;

			// 速度を減衰（空気抵抗）
			enemy.velocity.x *= 0.98f;
			enemy.velocity.z *= 0.98f;

			// 地面についたら消す
			if (enemy.position.y <= 0.0f)
			{
				enemy.position.y = 0.0f;
				enemy.isAlive = false;
			}

			continue;  // 通常の移動処理をスキップ
		}

		//	===	よろめき状態判定	===
		float hpRatio = (float)enemy.health / (float)enemy.maxHealth;

		// HP30%以下 OR パリィによるスタン蓄積MAX → スタガー
		if ((hpRatio <= 0.3f && enemy.health > 0) || enemy.isStaggered)
		{
			// isStaggeredが既にtrueの場合はそのまま維持
			if (!enemy.isStaggered)
				enemy.isStaggered = true;

			enemy.staggerFlashTimer += deltaTime * 0.5f;
		}
		else
		{
			enemy.isStaggered = false;
			enemy.staggerFlashTimer = 0.0f;
		}

		// === 通常の移動処理 ===
		UpdateEnemyMovement(enemy, playerPos, deltaTime);
	}

	// ========================================
	// === 空間分割を使った高速衝突解決 ===
	// ========================================

	// Step 1: 空間グリッドを構築
	BuildSpatialGrid();

	// Step 2: 衝突を解決（押し出し処理）
	ResolveCollisionsSpatial();
}

//	UpdateEnemyMovement	一帯の敵の移動処理
//	【役割】敵をプレイヤーに向かって動かす
//	【仕組み】一定時間ごとに方向転換して、プレイヤーに近づく
void EnemySystem::UpdateEnemyMovement(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime)
{
	if (enemy.isDying)
		return;

		// == = MIDBOSS / BOSS は専用AIで処理 == =
		if (enemy.type == EnemyType::MIDBOSS || enemy.type == EnemyType::BOSS)
		{
			UpdateBossAI(enemy, playerPos, deltaTime);
			return;  // 通常AIはスキップ
		}

	//	移動タイマーを増やす
	//	【役割】「何秒うごいたか」を記録
	enemy.moveTimer += deltaTime;	//	deltaTime = 1/60秒 = 0.0166...秒

	//	方向転換のタイミングか確認
	//	【条件】moveTimer が nextDirectionChange を超えたら方向転換
	//	【例】２秒ごとに方向を変える
	if (enemy.moveTimer >= enemy.nextDirectionChange)
	{
		//	プレイヤーの方向ベクトルを計算
		//	【目的】敵をプレイヤーに向かって動かすため
		float dirX = playerPos.x - enemy.position.x;	//	X方向の距離
		float dirZ = playerPos.z - enemy.position.z;	//	Z方向の距離

		//	距離を計算(ピタゴラスの定理)
		//	【数式】distance = √(dirX? + dirZ?)
		//	【例】dirX = 3, dirZ = 4 distance = 5;
		float distance = sqrtf(dirX * dirX + dirZ * dirZ);

		//	距離が０でないか確認(ゼロ除算を防ぐ)
		//	【理由】distance = 0だと、次の世紀化で割り算エラーになる
		if (distance > 0.1f)
		{
			//	方向ベクトルを正規化（長さを１にする）
			//	【目的】速度を一定にするため
			//	【例】dirX = 3, dirZ = 4, distance = 5 → dirX/distance = 0.6, dirZ/distance = 0.8
			float normalizedX = dirX / distance;
			float normalizedZ = dirZ / distance;

			enemy.velocity.x = normalizedX * 0.5f;
			enemy.velocity.z = normalizedZ * 0.5f;
		}

		//	タイマーをリセット
		//	【理由】次の方向転換まで再びカウントするため
		enemy.moveTimer = 0.0f;

		//	次の方向タイミングをランダムに設定
		//	【範囲】1.0 - 3.0秒後
		//	【計算】1.0 + (0.0 - 1.0のランダム値 * 2.0) = 1.0 - 3.0
		enemy.nextDirectionChange = 1.0f + (float)rand() / RAND_MAX * 2.0f;
	}

	//	位置を更新(速度に応じて移動)
	//	【数式】新しい位置　＝　現在地　＋　速度　＊　時間
	//	【例】position.x = 5, velocity.x = 2 deltaTime = 0.5 → 新position.x = 6
	enemy.position.x += enemy.velocity.x * deltaTime;
	enemy.position.z += enemy.velocity.z * deltaTime;

	//	地面の高さを維持
	//	【理由】敵が宙に浮いたり、地面に埋まったりしないように
	enemy.position.y = 0.0f;	//	常に1.0の高さ

	//	マップの境界チェック(簡易)
	//	【範囲】X座標が -50 - 50　の範囲
	//	【目的】敵が無限に遠くに行かないように
	if (enemy.position.x < -50.0f || enemy.position.x > 50.0f ||
		enemy.position.z < -50.0f || enemy.position.z > 50.0f)
	{
		//	範囲外に出たら、速度を反転して中央寄りに戻す
		//	【効果】敵が壁で跳ね返るような動き
		enemy.velocity.x *= -0.5f;	//逆方向に半分の速度
		enemy.velocity.z *= -0.5f;
	}

	//	プレイヤーへの方向ベクトルを計算
	float dirToPlayerX = playerPos.x - enemy.position.x;
	float dirToPlayerZ = playerPos.z - enemy.position.z;

	//	===	進行方向に向くように回転角度を更新	===
	//	速度がほぼ0じゃない場合のみ計算(とまっているときは向きを変えない)
	if (fabs(dirToPlayerX) > 0.001f || fabs(dirToPlayerZ) > 0.001f)
	{
		// atan2f(x, z) で、(0, 1)方向（Z軸プラス）からの角度(ラジアン)を求められる
		enemy.rotationY = atan2f(dirToPlayerX, dirToPlayerZ);

		enemy.rotationY += 3.14159f;
	}

	// プレイヤーとの距離を計算
   // 【数式】distance = √((敵X - プレイヤーX)? + (敵Z - プレイヤーZ)?)
   // 【理由】Y座標は無視（地面の高さは同じなので）
	float playerDist = sqrtf(
		powf(enemy.position.x - playerPos.x, 2) +
		powf(enemy.position.z - playerPos.z, 2)
	);


	// 接触判定（距離が1.5メートル以内）
	if (playerDist < 1.5f)
	{
		enemy.touchingPlayer = true;
	}
	else
	{
		enemy.touchingPlayer = false;
	}

	// === プレイヤーに接触しているなら攻撃 ===
	if (enemy.touchingPlayer)
	{
		if (enemy.currentAnimation != "Attack")
		{
			//	モーションが同時にならないように
			enemy.currentAnimation = "Attack";
			float maxOffset;
			switch (enemy.type)
			{
			case EnemyType::RUNNER:  maxOffset = m_runnerAttackHitTime * 0.5f; break;
			case EnemyType::TANK:    maxOffset = m_tankAttackHitTime * 0.5f;   break;
			default:                 maxOffset = m_normalAttackHitTime * 0.5f;  break;
			}
			enemy.animationTime = ((float)rand() / RAND_MAX) * maxOffset;

			enemy.attackJustLanded = false; // まだ殴ってない
		}

		// --- 攻撃中は止まる ---
		enemy.velocity.x = 0.0f;
		enemy.velocity.z = 0.0f;

		// アニメが「殴る瞬間」に達したらダメージ発生
		// === タイプ別あったっくタイミング==
		float attackHitTime;
		switch (enemy.type)
		{
		case EnemyType::RUNNER:
			attackHitTime = m_runnerAttackHitTime;
			break;
		case EnemyType::TANK:
			attackHitTime = m_tankAttackHitTime;
			break;
		default:
			attackHitTime = m_normalAttackHitTime;
			break;
		}
		float attackAnimSpeed;
		switch (enemy.type)
		{
		case EnemyType::RUNNER:  attackAnimSpeed = m_animSpeed_Attack[1]; break;
		case EnemyType::TANK:    attackAnimSpeed = m_animSpeed_Attack[2]; break;
		default:                 attackAnimSpeed = m_animSpeed_Attack[0]; break;
		}
		float prevTime = enemy.animationTime;
		enemy.animationTime += deltaTime * attackAnimSpeed;

		// prevTime < hitTime <= currentTime → 殴る瞬間を通過した！
		if (prevTime < attackHitTime && enemy.animationTime >= attackHitTime)
		{
			enemy.attackJustLanded = true;

			char buf[128];
			sprintf_s(buf, "[ATTACK] Enemy ID:%d STRIKE! (animTime=%.2f)\n",
				enemy.id, enemy.animationTime);
			OutputDebugStringA(buf);
		}
		else
		{
			enemy.attackJustLanded = false;
		}

		// アタックアニメのループ（終了→最初に戻る）
		float attackDuration;
		switch (enemy.type)
		{
		case EnemyType::RUNNER:
			attackDuration = m_runnerAttackDuration;
			break;
		case EnemyType::TANK:
			attackDuration = m_tankAttackDuration;
			break;
		default:
			attackDuration = m_normalAttackDuration;
			break;
		}
		if (enemy.animationTime >= attackDuration)
		{
			enemy.animationTime = 0.0f;
		}
	}
	//	動いているなら移動
	else
	{
		enemy.attackJustLanded = false;

		//	速度の長さ(スピード計算)
		float speed = sqrtf(enemy.velocity.x * enemy.velocity.x + enemy.velocity.z * enemy.velocity.z);

		//	===	タイプ別の速度倍率	===
		float speedMultiplier = 1.0f;

		switch (enemy.type)
		{
		case EnemyType::NORMAL:
			speedMultiplier = 2.5f;	//	通常速度
			break;

		case EnemyType::RUNNER:
			speedMultiplier = 3.0f;	//	2倍速い
			break;

		case EnemyType::TANK:
			speedMultiplier = 1.5f;	//	半分遅い
			break;
		}

		//	最終的な速度を計算（倍率をかける）
		float finalSpeed = speed * speedMultiplier;

		//	スピードが速ければ「走り」、遅ければ「歩き」
		std::string moveAnim = (finalSpeed > 4.0f) ? "Run" : "Walk";

		//	とまっている(速度がほぼ0)なら待機
		if (finalSpeed < 0.1f)
		{
			moveAnim = "Idle";
		}

		//	アニメーション変更
		if (enemy.currentAnimation != moveAnim && enemy.currentAnimation != "Attack")
		{
			enemy.currentAnimation = moveAnim;
		}

		//	攻撃モーションが終わったら移動に戻す処理
		if (enemy.currentAnimation == "Attack")
		{
			if (!enemy.touchingPlayer)
			{
				enemy.currentAnimation = moveAnim;
			}
		}

		//	位置を更新（ここで倍率を使う）
		enemy.position.x += enemy.velocity.x * finalSpeed * deltaTime;
		enemy.position.z += enemy.velocity.z * finalSpeed * deltaTime;
	}
}

//	Spawn Enemy	新しい敵を一体生成
//	【いつ呼ばれる】ウェーブ開始時、または敵が減ったとき
//	【役割】ランダムな位置に敵を出現させる
void EnemySystem::SpawnEnemy(DirectX::XMFLOAT3 playerPos)
{
	if (m_enemies.size() >= m_maxEnemies)
		return;

	Enemy enemy;
	enemy.id = m_nextEnemyID++;

	// === タイプ設定 ===
	int typeRoll = rand() % 100;

	if (typeRoll < 60)
	{
		enemy.type = EnemyType::NORMAL;
		enemy.health = 100;
		enemy.maxHealth = 100;
		enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if (typeRoll < 85)
	{
		enemy.type = EnemyType::RUNNER;
		enemy.health = 50;
		enemy.maxHealth = 50;
		enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		enemy.type = EnemyType::TANK;
		enemy.health = 300;
		enemy.maxHealth = 300;
		enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// === 重ならない位置を生成 ===
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
		enemy.position.y = 0.0f;
		enemy.position.z = playerPos.z + sinf(angle) * distance;
	}

	// === 初期速度 ===
	enemy.velocity.x = 0.0f;
	enemy.velocity.y = 0.0f;
	enemy.velocity.z = 0.0f;

	// === ステータス ===
	enemy.isAlive = true;

	// === アニメーション ===
	enemy.currentAnimation = "Idle";
	// 歩きアニメがバラけるようにランダムオフセット
	enemy.animationTime = ((float)rand() / RAND_MAX) * 1.0f;

	// === その他 ===
	enemy.isDying = false;
	enemy.corpseTimer = 0.0f;  // ← 既存メンバー
	enemy.rotationY = 0.0f;    // ← 既存メンバー（rotation ではない！）

	enemy.justSpawned = true;

	m_enemies.push_back(enemy);
}

void EnemySystem::SpawnMidBoss(DirectX::XMFLOAT3 playerPos)
{
	Enemy enemy;
	enemy.id = m_nextEnemyID++;

	// MIDBOSS固定ステータス
	enemy.type = EnemyType::MIDBOSS;
	enemy.health = 5000;
	enemy.maxHealth = 5000;
	enemy.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// プレイヤーの正面やや遠くにスポーン
	float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
	float distance = 15.0f + (float)rand() / RAND_MAX * 5.0f;

	enemy.position.x = playerPos.x + cosf(angle) * distance;
	enemy.position.y = 0.0f;
	enemy.position.z = playerPos.z + sinf(angle) * distance;

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

	m_enemies.push_back(enemy);


	char buf[128];
	sprintf_s(buf, "[MIDBOSS] Spawned! ID:%d, HP:%d\n", enemy.id, enemy.health);
	OutputDebugStringA(buf);
}


void EnemySystem::SpawnBoss(DirectX::XMFLOAT3 playerPos)
{
	Enemy enemy;
	enemy.id = m_nextEnemyID++;

	enemy.type = EnemyType::BOSS;
	enemy.health = 15000;        // MIDBOSSの3倍！
	enemy.maxHealth = 15000;
	enemy.color = DirectX::XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f);  // 赤黒い

	float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
	float distance = 18.0f + (float)rand() / RAND_MAX * 5.0f;

	enemy.position.x = playerPos.x + cosf(angle) * distance;
	enemy.position.y = 0.0f;
	enemy.position.z = playerPos.z + sinf(angle) * distance;

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

	m_enemies.push_back(enemy);

	char buf[128];
	sprintf_s(buf, "[BOSS] Spawned! ID:%d, HP:%d\n", enemy.id, enemy.health);
	OutputDebugStringA(buf);
}

//	\死んだ敵を配列から削除
//	【いつ呼ばれる】Update()の最後、またはウェーブ終了時
//	【役割】isAlive == false の敵をメモリから削除
void EnemySystem::ClearDeadEnemies()
{
	//	std::remove_if + erase イディオム
	//	【仕組み】
	//	1, remove_if;	条件に合う要素を配列の後ろに移動
	//	2, erase;	後ろの不要な要素を削除 

	m_enemies.erase(
		//	remove_if;	「死んでいる敵」を見つける
		// 【引数１】配列の開始位置
		// 【引数２】配列の終了位置
		// 【引数３】ラムダ式(条件を書く無名関数)
		std::remove_if(
			m_enemies.begin(),	//	配列の最初
			m_enemies.end(),//	配列の最後
			//	ラムダ式：[](引数) {処理}
			//	【役割】各敵について「削除するべきか」を判定
			//	【引数】const Enemy& e = 各敵(参照で受け取る)
			//	【戻り値】bool = true なら削除
			[](const Enemy& e) {
				// isDying中（死亡アニメ再生中）は消さない
				return !e.isAlive && !e.isDying;
			}
		),
		m_enemies.end()	//	remove_ifが返した「ここから削除」位置
	);
} 

// === 関数1: 座標からグリッドキーを取得 ===
EnemySystem::GridKey EnemySystem::GetGridKey(const DirectX::XMFLOAT3& position) const
{
	GridKey key;
	// 位置をセルサイズで割って、セルの座標を取得
	// 例: position.x = 12.5, CELL_SIZE = 5.0 → key.x = 2
	key.x = (int)floorf(position.x / CELL_SIZE);
	key.z = (int)floorf(position.z / CELL_SIZE);
	return key;
}

// === 関数2: 空間グリッドを構築 ===
void EnemySystem::BuildSpatialGrid()
{
	// 前フレームのグリッドをクリア
	m_spatialGrid.clear();

	// 全ての敵をグリッドに登録
	for (size_t i = 0; i < m_enemies.size(); i++)
	{
		const Enemy& enemy = m_enemies[i];

		// 死んでる敵や死亡中の敵は無視
		if (!enemy.isAlive || enemy.isDying)
			continue;

		// この敵が属するセルを計算
		GridKey key = GetGridKey(enemy.position);

		// セルに敵のインデックスを追加
		m_spatialGrid[key].push_back(i);
	}
}

// === 関数3: 空間分割を使った衝突解決 ===
void EnemySystem::ResolveCollisionsSpatial()
{
	const float COLLISION_RADIUS = 0.5f;   // 敵の半径
	const float PUSH_STRENGTH = 0.05f;     // 押し出す強さ
	const float MIN_DISTANCE = COLLISION_RADIUS * 2.0f;  // 1.0m

	// === 各セル内で衝突チェック ===
	for (auto& cellPair : m_spatialGrid)
	{
		const GridKey& currentKey = cellPair.first;
		std::vector<size_t>& enemiesInCell = cellPair.second;

		// === 1. 同じセル内の敵同士をチェック ===
		for (size_t i = 0; i < enemiesInCell.size(); i++)
		{
			Enemy& enemyA = m_enemies[enemiesInCell[i]];

			// 攻撃中は押し出さない（群れで襲う）
			if (enemyA.currentAnimation == "Attack")
				continue;

			for (size_t j = i + 1; j < enemiesInCell.size(); j++)
			{
				Enemy& enemyB = m_enemies[enemiesInCell[j]];

				if (enemyB.currentAnimation == "Attack")
					continue;

				// 距離を計算
				float dx = enemyB.position.x - enemyA.position.x;
				float dz = enemyB.position.z - enemyA.position.z;
				float distance = sqrtf(dx * dx + dz * dz);

				// 衝突判定
				if (distance < MIN_DISTANCE && distance > 0.001f)
				{
					// 押し出す方向
					float nx = dx / distance;
					float nz = dz / distance;

					// 重なり量
					float overlap = MIN_DISTANCE - distance;
					float pushDistance = overlap * PUSH_STRENGTH;

					// 互いに押し出す
					enemyA.position.x -= nx * pushDistance;
					enemyA.position.z -= nz * pushDistance;

					enemyB.position.x += nx * pushDistance;
					enemyB.position.z += nz * pushDistance;
				}
			}
		}

		// === 2. 隣接セルの敵ともチェック ===
		// セルの境界付近の敵のため、隣接8セルもチェック
		for (int dx = -1; dx <= 1; dx++)
		{
			for (int dz = -1; dz <= 1; dz++)
			{
				// 同じセルはスキップ（既にチェック済み）
				if (dx == 0 && dz == 0)
					continue;

				// 隣接セルのキー
				GridKey neighborKey;
				neighborKey.x = currentKey.x + dx;
				neighborKey.z = currentKey.z + dz;

				// 隣接セルが存在するか確認
				auto it = m_spatialGrid.find(neighborKey);
				if (it == m_spatialGrid.end())
					continue;  // セルが空

				std::vector<size_t>& neighborsInCell = it->second;

				// 現在のセルの敵 vs 隣接セルの敵
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


// === 関数4: スポーン位置が有効かチェック ===
bool EnemySystem::IsPositionValid(const DirectX::XMFLOAT3& position, float minDistance) const
{
	// 全ての既存の敵との距離をチェック
	for (const auto& existingEnemy : m_enemies)
	{
		// 死んでる敵や死亡中の敵は無視
		if (!existingEnemy.isAlive || existingEnemy.isDying)
			continue;

		// 距離を計算
		float dx = position.x - existingEnemy.position.x;
		float dz = position.z - existingEnemy.position.z;
		float distance = sqrtf(dx * dx + dz * dz);

		// 近すぎる場合はfalse
		if (distance < minDistance)
			return false;
	}

	// 全ての敵から十分離れていたらtrue
	return true;
}

// =============================================================
// ボスAI状態マシン（MIDBOSS / BOSS 共通）
// =============================================================
void EnemySystem::UpdateBossAI(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime)
{
	// --- プレイヤーへの方向と距離を計算 ---
	float dx = playerPos.x - enemy.position.x;
	float dz = playerPos.z - enemy.position.z;
	float dist = sqrtf(dx * dx + dz * dz);

	// --- 常にプレイヤーの方を向く ---
	if (dist > 0.1f)
	{
		enemy.rotationY = atan2f(dx, dz) + 3.14159f;
	}
	
	// --- フェーズタイマー進行 ---
	enemy.bossPhaseTimer += deltaTime;

	// --- 状態マシン ---
	switch (enemy.bossPhase)
	{
		// ============================================
		// IDLE：プレイヤーに近づく → 距離or時間で攻撃開始
		// ============================================
	case BossAttackPhase::IDLE:
	{
		// クールダウン中は近づくだけ
		enemy.bossAttackCooldown -= deltaTime;

		// プレイヤーに向かって歩く
		if (dist > 3.0f)
		{
			float speed = 1.5f;  // ボスの歩行速度
			if (enemy.type == EnemyType::MIDBOSS) speed = 2.5f;

			float nx = dx / dist;  // 正規化
			float nz = dz / dist;
			enemy.velocity.x = nx * speed;
			enemy.velocity.z = nz * speed;

			float finalSpeed = (enemy.type == EnemyType::BOSS) ? 0.3f : 0.4f;
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
			if (enemy.currentAnimation != "Idle")
			{
				enemy.currentAnimation = "Idle";
				enemy.animationTime = 0.0f;
			}
		}

		// --- 攻撃選択 ---
		// クールダウンが終わっていて、ある程度近い
		if (enemy.bossAttackCooldown <= 0.0f && dist < 30.0f)
		{
			enemy.bossTargetPos = playerPos;  // 攻撃時のターゲット位置を記録
			enemy.bossAttackCount++;

			if (enemy.type == EnemyType::BOSS)
			{
				// BOSS: ジャンプ叩きつけ と 斬撃 をランダム
				int pattern = rand() % 2;  // 0=ジャンプ, 1=斬撃
				if (pattern == 0)
				{
					enemy.bossPhase = BossAttackPhase::JUMP_WINDUP;
					enemy.bossPhaseTimer = 0.0f;
					enemy.currentAnimation = "AttackJump";
					enemy.animationTime = 0.0f;

					OutputDebugStringA("[BOSS AI] -> JUMP_WINDUP (random)\n");
				}
				else
				{
					enemy.bossPhase = BossAttackPhase::SLASH_WINDUP;
					enemy.bossPhaseTimer = 0.0f;
					enemy.currentAnimation = "AttackSlash";
					enemy.animationTime = 0.0f;

					OutputDebugStringA("[BOSS AI] -> SLASH_WINDUP (random)\n");
				}
			}
			else // MIDBOSS
			{
				// MIDBOSS: 咆哮ビーム と ジャンプ叩きつけ を交互
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
					OutputDebugStringA(buf);
				}
				else
				{
					enemy.bossPhase = BossAttackPhase::JUMP_WINDUP;
					enemy.bossPhaseTimer = 0.0f;
					enemy.currentAnimation = "Attack";
					enemy.animationTime = 0.0f;

					OutputDebugStringA("[MIDBOSS AI] -> JUMP_WINDUP\n");
				}
			}
		}

		// 近接攻撃（通常殴り）- プレイヤーが超近い場合
		if (dist < 1.5f)
		{
			enemy.touchingPlayer = true;
			if (enemy.type == EnemyType::BOSS)
			{
				if (enemy.currentAnimation != "AttackJump")
				{
					enemy.currentAnimation = "AttackJump";
					enemy.animationTime = 0.0f;
				}
			}
			else
			{
				if (enemy.currentAnimation != "Attack")
				{
					enemy.currentAnimation = "Attack";
					enemy.animationTime = 0.0f;
				}
			}
		}
		else
		{
			enemy.touchingPlayer = false;
		}

		break;
	}

	// ============================================
	// ジャンプ叩きつけ（BOSS & MIDBOSS共通）
	// ============================================
	case BossAttackPhase::JUMP_WINDUP:
	{
		// 0.5秒の溜めモーション（地面に力を込める）
		enemy.velocity = { 0, 0, 0 };

		if (enemy.bossPhaseTimer >= 0.5f)
		{
			enemy.bossPhase = BossAttackPhase::JUMP_AIR;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossTargetPos = playerPos;  // ジャンプ先を更新

			OutputDebugStringA("[BOSS AI] -> JUMP_AIR\n");
		}
		break;
	}

	case BossAttackPhase::JUMP_AIR:
	{
		// 0.6秒で空中 → ターゲット位置に向かって移動
		float jumpDuration = 0.6f;
		float progress = enemy.bossPhaseTimer / jumpDuration;

		// 放物線（sin で山なり）
		enemy.bossJumpHeight = sinf(progress * 3.14159f) * 8.0f;
		enemy.position.y = enemy.bossJumpHeight;

		// 水平移動（ターゲットに向かって補間）
		float lerpSpeed = 5.0f * deltaTime;
		enemy.position.x += (enemy.bossTargetPos.x - enemy.position.x) * lerpSpeed;
		enemy.position.z += (enemy.bossTargetPos.z - enemy.position.z) * lerpSpeed;

		if (enemy.bossPhaseTimer >= jumpDuration)
		{
			enemy.bossPhase = BossAttackPhase::JUMP_SLAM;
			enemy.bossPhaseTimer = 0.0f;
			enemy.position.y = 0.0f;
			enemy.bossJumpHeight = 0.0f;

			// ★ ここでダメージ判定とエフェクトを発生させる
			//   (game.cppで処理 - attackJustLandedを流用)
			enemy.attackJustLanded = true;

			OutputDebugStringA("[BOSS AI] -> JUMP_SLAM!\n");
		}
		break;
	}

	case BossAttackPhase::JUMP_SLAM:
	{
		// 着地した瞬間 → 0.1秒だけSLAM状態
		enemy.position.y = 0.0f;

		if (enemy.bossPhaseTimer >= 0.1f)
		{
			enemy.attackJustLanded = false;
			enemy.bossPhase = BossAttackPhase::SLAM_RECOVERY;
			enemy.bossPhaseTimer = 0.0f;

			OutputDebugStringA("[BOSS AI] -> SLAM_RECOVERY\n");
		}
		break;
	}

	case BossAttackPhase::SLAM_RECOVERY:
	{
		// 1.5秒の硬直（プレイヤーの攻撃チャンス）
		enemy.velocity = { 0, 0, 0 };

		if (enemy.currentAnimation != "Idle")
		{
			enemy.currentAnimation = "Idle";
			enemy.animationTime = 0.0f;
		}

		if (enemy.bossPhaseTimer >= 1.5f)
		{
			enemy.bossPhase = BossAttackPhase::IDLE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossAttackCooldown = 3.0f;  // 次の攻撃まで3秒

			OutputDebugStringA("[BOSS AI] -> IDLE (cooldown)\n");
		}
		break;
	}

	// ============================================
	// 斬撃3本飛ばし（BOSS）
	// ============================================
	case BossAttackPhase::SLASH_WINDUP:
	{
		// 0.8秒の殴り上げモーション
		enemy.velocity = { 0, 0, 0 };

		if (enemy.bossPhaseTimer >= 0.8f)
		{
			enemy.bossPhase = BossAttackPhase::SLASH_FIRE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossTargetPos = playerPos;  // 発射方向を確定

			// ★ game.cppで3本のプロジェクタイルを生成する
			//   (フラグで通知)
			enemy.attackJustLanded = true;

			OutputDebugStringA("[BOSS AI] -> SLASH_FIRE!\n");
		}
		break;
	}

	case BossAttackPhase::SLASH_FIRE:
	{
		// 発射の瞬間（0.2秒）
		if (enemy.bossPhaseTimer >= 0.2f)
		{
			enemy.attackJustLanded = false;
			enemy.bossPhase = BossAttackPhase::SLASH_RECOVERY;
			enemy.bossPhaseTimer = 0.0f;

			OutputDebugStringA("[BOSS AI] -> SLASH_RECOVERY\n");
		}
		break;
	}

	case BossAttackPhase::SLASH_RECOVERY:
	{
		// 1.0秒の硬直
		enemy.velocity = { 0, 0, 0 };

		if (enemy.currentAnimation != "Idle")
		{
			enemy.currentAnimation = "Idle";
			enemy.animationTime = 0.0f;
		}

		if (enemy.bossPhaseTimer >= 1.0f)
		{
			enemy.bossPhase = BossAttackPhase::IDLE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossAttackCooldown = 2.5f;

			OutputDebugStringA("[BOSS AI] -> IDLE (cooldown)\n");
		}
		break;
	}

	// ============================================
	// 咆哮ビーム（MIDBOSS）
	// ============================================
	case BossAttackPhase::ROAR_WINDUP:
	{
		//  チャージ中もAttackアニメを維持
		if (enemy.currentAnimation != "Attack")
		{
			enemy.currentAnimation = "Attack";
		}

		// 1.0秒の咆哮溜め（口を開ける演出）
		enemy.velocity = { 0, 0, 0 };

		if (enemy.bossPhaseTimer >= 1.0f)
		{
			enemy.bossPhase = BossAttackPhase::ROAR_FIRE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossTargetPos = playerPos;

			enemy.attackJustLanded = true;

			OutputDebugStringA("[MIDBOSS AI] -> ROAR_FIRE!\n");
		}
		break;
	}

	case BossAttackPhase::ROAR_FIRE:
	{
		if (enemy.currentAnimation != "Attack")
			enemy.currentAnimation = "Attack";

		// ゆっくりプレイヤーを追従
		float trackSpeed = 0.5f * deltaTime;
		enemy.bossTargetPos.x += (playerPos.x - enemy.bossTargetPos.x) * trackSpeed;
		enemy.bossTargetPos.z += (playerPos.z - enemy.bossTargetPos.z) * trackSpeed;

		float bdx = enemy.bossTargetPos.x - enemy.position.x;
		float bdz = enemy.bossTargetPos.z - enemy.position.z;
		if (fabs(bdx) > 0.01f || fabs(bdz) > 0.01f)
		{
			enemy.rotationY = atan2f(bdx, bdz) + 3.14159f;
		}

		// 5.0秒間（チャージ2.5s + 発射5.0s = 全体7.5sで届く）
		if (enemy.bossPhaseTimer >= 5.0f)
		{
			enemy.attackJustLanded = false;
			enemy.bossPhase = BossAttackPhase::ROAR_RECOVERY;
			enemy.bossPhaseTimer = 0.0f;
			OutputDebugStringA("[MIDBOSS AI] -> ROAR_RECOVERY\n");
		}
		break;
	}

	case BossAttackPhase::ROAR_RECOVERY:
	{
		// 2.0秒の硬直（大きな攻撃チャンス）
		enemy.velocity = { 0, 0, 0 };

		if (enemy.currentAnimation != "Idle")
		{
			enemy.currentAnimation = "Idle";
			enemy.animationTime = 0.0f;
		}

		if (enemy.bossPhaseTimer >= 2.0f)
		{
			enemy.bossPhase = BossAttackPhase::IDLE;
			enemy.bossPhaseTimer = 0.0f;
			enemy.bossAttackCooldown = 3.0f;

			OutputDebugStringA("[MIDBOSS AI] -> IDLE (cooldown)\n");
		}
		break;
	}

	} // switch end

	// --- マップ境界チェック ---
	if (enemy.position.x < -50.0f) enemy.position.x = -50.0f;
	if (enemy.position.x > 50.0f) enemy.position.x = 50.0f;
	if (enemy.position.z < -50.0f) enemy.position.z = -50.0f;
	if (enemy.position.z > 50.0f) enemy.position.z = 50.0f;
}


// デバッグ用：指定タイプの敵を1体スポーン
void EnemySystem::SpawnEnemyOfType(EnemyType type, DirectX::XMFLOAT3 playerPos)
{
	Enemy enemy;
	enemy.id = m_nextEnemyID++;
	enemy.type = type;

	// タイプ別ステータス
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

	// プレイヤーの正面5mにスポーン
	float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
	float distance = 5.0f;
	enemy.position.x = playerPos.x + cosf(angle) * distance;
	enemy.position.y = 0.0f;
	enemy.position.z = playerPos.z + sinf(angle) * distance;

	enemy.velocity = { 0.0f, 0.0f, 0.0f };
	enemy.isAlive = true;

	enemy.currentAnimation = "Idle";
	enemy.animationTime = ((float)rand() / RAND_MAX) * 1.0f;

	enemy.justSpawned = true;

	m_enemies.push_back(enemy);

	char buf[128];
	sprintf_s(buf, "[DEBUG SPAWN] Type:%d ID:%d\n", (int)type, enemy.id);
	OutputDebugStringA(buf);
}