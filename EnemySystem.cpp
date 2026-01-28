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

		//	HPが30%以下ならよろめき状態にする
		float hpRatio = (float)enemy.health / (float)enemy.maxHealth;

		if (hpRatio <= 0.3f && enemy.health > 0)
		{
			enemy.isStaggered = true;

			//	点滅タイマー(5倍速)
			enemy.staggerFlashTimer += deltaTime * 0.5f;

			// デバッグログ（最初の1回だけ）
			static int lastStaggeredID = -1;
			if (lastStaggeredID != enemy.id)
			{
				char buffer[128];
				sprintf_s(buffer, "[GLORY] Enemy ID:%d is STAGGERED! HP:%.1f/%.1f\n",
					enemy.id, (float)enemy.health, (float)enemy.maxHealth);
				OutputDebugStringA(buffer);
				lastStaggeredID = enemy.id;
			}
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
	// 【理由】敵のサイズ(2.0) + プレイヤーのサイズを考慮
	if (playerDist < 1.5f)
	{
		enemy.touchingPlayer = true;  // フラグを立てる
	}
	else
	{
		enemy.touchingPlayer = false;
	}

	//	===	プレイヤーに接触しているなら攻撃	===
	if (enemy.touchingPlayer)
	{
		if (enemy.currentAnimation != "Attack")
		{
			enemy.currentAnimation = "Attack";
			enemy.animationTime = 0.0f;	//	最初から再生
		}

		//	---	攻撃中は足を止める	---
		enemy.velocity.x = 0.0f;
		enemy.velocity.z = 0.0f;
	}
	//	動いているなら移動
	else
	{
		//	速度の長さ(スピード計算)
		float speed = sqrtf(enemy.velocity.x * enemy.velocity.x + enemy.velocity.z * enemy.velocity.z);

		//	===	タイプ別の速度倍率	===
		float speedMultiplier = 1.0f;

		switch (enemy.type)
		{
		case EnemyType::NORMAL:
			speedMultiplier = 1.0f;	//	通常速度
			break;

		case EnemyType::RUNNER:
			speedMultiplier = 2.0f;	//	2倍速い
			break;

		case EnemyType::TANK:
			speedMultiplier = 0.5f;	//	半分遅い
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
		enemy.color = DirectX::XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f);
	}
	else if (typeRoll < 85)
	{
		enemy.type = EnemyType::RUNNER;
		enemy.health = 50;
		enemy.maxHealth = 50;
		enemy.color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (typeRoll < 99)
	{
		enemy.type = EnemyType::TANK;
		enemy.health = 300;
		enemy.maxHealth = 300;
		enemy.color = DirectX::XMFLOAT4(0.2f, 0.2f, 0.8f, 1.0f);
	}
	else
	{
		enemy.type = EnemyType::MIDBOSS;
		enemy.health = 5000;
		enemy.maxHealth = 5000;
		enemy.color = DirectX::XMFLOAT4(1.0f, 0.8f, 0.8f, 1.0f);
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
	enemy.velocity.x = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;
	enemy.velocity.y = 0.0f;
	enemy.velocity.z = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;

	// === ステータス ===
	enemy.isAlive = true;

	// === アニメーション ===
	enemy.currentAnimation = "Idle";
	enemy.animationTime = 0.0f;

	// === その他 ===
	enemy.isDying = false;
	enemy.corpseTimer = 0.0f;  // ← 既存メンバー
	enemy.rotationY = 0.0f;    // ← 既存メンバー（rotation ではない！）

	m_enemies.push_back(enemy);
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
				return !e.isAlive;	//	死んでいたら true(削除)
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