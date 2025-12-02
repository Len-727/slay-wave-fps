//	EnemySystem.cpp	敵管理システムの実装
#include "EnemySystem.h"
#include <Algorithm>	//	std::remmove_if(配列から要素を削除)を使うため
#include <cmath>		//	sprtf(平方根), conf, sinfを使うため

//	コンストラクタ	EnemySystemで作られた時の初期化
//	【いつ呼ばれる】Game.cppで std::make_unique<EnemySystem>()したとき
//	【役割】メンバ変数に初期値を設定
EnemySystem::EnemySystem()
	: m_maxEnemies(24)			//	最大的数：　２４体
	, m_enemySpawnTimer(0.0f)	//	スポーンタイマー:	０秒からスタート
{
	//	m_enemies　は空の配列として自動初期化される
}

//	【役割】全ての敵を動かして、プレイヤーに近づける
void EnemySystem::Update(float deltaTime, DirectX::XMFLOAT3 playerPos)
{
	//	【全ての敵を一体ずつ更新】
	//	Enemy& = 参照(コピーせず本物を触る)なので、enemy.position を変更できる
	for (auto& enemy : m_enemies)
	{
		//	死んでいる敵はスキップ
		if (!enemy.isAlive)
			continue;

		//	子の敵を動かす(内部関数を呼ぶ) 
		UpdateEnemyMovement(enemy, playerPos, deltaTime);
	}
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

			enemy.velocity.x = normalizedX * 1.0f;
			enemy.velocity.z = normalizedZ * 1.0f;
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

		//	スピードが速ければ「走り」、遅ければ「歩き」
		std::string moveAnim = (speed > 4.0f) ? "Run" : "Walk";

		//	とまっている(速度がほぼ０)なら待機
		if (speed < 0.1f)
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
	}
}

//	Spawn Enemy	新しい敵を一体生成
//	【いつ呼ばれる】ウェーブ開始時、または敵が減ったとき
//	【役割】ランダムな位置に敵を出現させる
void EnemySystem::SpawnEnemy(DirectX::XMFLOAT3 playerPos)
{
	//	敵の数が上限に達していないか確認
	//	【理由】敵が増えすぎるとゲームが重くなる
	if (m_enemies.size() >= m_maxEnemies)
		return;	//	上限に達していたら何もせず終了

	//	【ステップ２】新しい敵を作成
	Enemy enemy;	//	空のオブジェクト


	//	位置の設定(プレイヤーから離れた場所)
	
	//	ランダムな角度を生成（０　～　360度）
	//	【計算】rand() / RAND_MAX = 0.0 ～ 1.0 のランダムな値
	//	【計算】* 2π = 0 ～ 6.28(ラジアン) = 0　～ 360度
	float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;

	//	ランダムな距離を生成(10 - 20m)
	//	【計算】10.0 + (0.0-1.0 * 10.0) = 10.0 - 20
	float distance = 10.0f + (float)rand() / RAND_MAX * 10.0f;

	//	極座標→直交座標に変換
	//	【数式】x = プレイヤーX + cos(角度) * 距離
	//		   z = プレイヤーZ + sin(角度) * 距離
	//	【効果】プレイヤー周辺いランダムに配置
	enemy.position.x = playerPos.x + cosf(angle) * distance;
	enemy.position.y = 1.0f;	//	地面の高さ
	enemy.position.z = playerPos.z + sinf(angle) * distance;


	//	初期速度の設定(ランダムな方向)
	//	【範囲】-2.0 - 2.0 m/s
	//	【計算】(0.0-1.0 - 0.5) * 4.0 = -2.0 - 2.0
	enemy.velocity.x = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;
	enemy.velocity.y = 0.0f;	//	y軸には動かない
	enemy.velocity.z = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;

	// == = 見た目の設定（暗めの色） == =

		// ランダムな色を生成（暗い緑っぽい色）
		// 【R】0.3 ? 0.5（暗い赤）
		// 【G】0.4 ? 0.6（暗い緑）
		// 【B】0.2（低い青）→ 緑っぽくなる
		enemy.color = DirectX::XMFLOAT4(
			0.3f + (float)rand() / RAND_MAX * 0.2f,  // R
			0.4f + (float)rand() / RAND_MAX * 0.2f,  // G
			0.2f,                                      // B
			1.0f                                       // A（透明度：不透明）
		);


		//	ステータス設定

		//	HP(後でウェーブ数に応じて増やす予定)
		enemy.maxHealth = 100;	//	最大HP
		enemy.health = 100;		//	現在HP(最初は満タン)

		//	状態フラグ
		enemy.isAlive = true;	//	生存状態

		//	移動タイマー
		enemy.moveTimer = 0.0f;	//	0秒からスタート

		//	次の方向転換までの時間(2-5秒後)
		enemy.nextDirectionChange = 2.0f + (float)rand() / RAND_MAX * 3.0f;

		enemy.touchingPlayer = false;

		//	【ステップ3】配列に追加
		//	【効果】m_enemiesの末尾に新しい敵を追加
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