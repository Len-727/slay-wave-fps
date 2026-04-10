//	WeapomSystem.cpp
#include "WeaponSystem.h"

//	コンストラクタ: 武器システムを初期状態にセット
WeaponSystem::WeaponSystem()
	: m_currentWeapon(WeaponType::SHOTGUN)	//	現在の武器：ピストル
	, m_primaryWeapon(WeaponType::SHOTGUN)	//	プライマリスロット：ピストル
	, m_secondaryWeapon(WeaponType::SHOTGUN)	//	セカンダリスロット：未使用だが初期化
	, m_currentWeaponSlot(0)				//	現在のスロット：０番
	, m_hasSecondaryWeapon(false)			//	２つ目の武器：まだ持っていない
	, m_currentAmmo(20)						//	装填弾数：８発
	, m_maxAmmo(20)							//	最大装填数：８発
	, m_reserveAmmo(120)						//	予備弾薬：８０発
	, m_isReloading(false)					//	リロード中：いいえ
	, m_reloadTimer(0.0f)					//	リロードタイマー0秒
	, m_fireRateTimer(0.0f)					//	連射制限タイマー
{
	InitializeWeapons();
}

void WeaponSystem::InitializeWeapons()
{
	 //	ピストル(初期武器・無料)
	m_weaponStats[WeaponType::PISTOL] = {
		WeaponType::PISTOL, 30, 8, 80, 0.2f, 50.0f, 1, 1.5f, 0
	};

	//	ショットガン（近距離特化・500ポイント）
	m_weaponStats[WeaponType::SHOTGUN] = {
		WeaponType::SHOTGUN, 200, 20, 120, 0.3f, 10.0f, 1, 1.0f, 500
	};

	//	ライフル（中距離バランス型・1000ポイント）
	m_weaponStats[WeaponType::RIFLE] = {
		WeaponType::RIFLE, 18, 30, 180, 1.0f, 80.0f, 1, 2.1f, 1000
	};

	//	スナイパー（遠距離一撃・1500ポイント）
	m_weaponStats[WeaponType::SNIPER] = {
		WeaponType::SNIPER, 500, 5, 50, 1.2f, 200.0f, 5, 3.0f, 1500
	};

	//	各武器の弾薬状態を初期化（最初は満タン）
	m_weaponAmmoStatus[WeaponType::PISTOL] = { 8, 80 };
	m_weaponAmmoStatus[WeaponType::SHOTGUN] = { 20, 120 };
	m_weaponAmmoStatus[WeaponType::RIFLE] = { 30, 180 };
	m_weaponAmmoStatus[WeaponType::SNIPER] = { 5, 50 };
}

//	武器切り替え：現在の弾薬を保持→新武器の弾薬を復元
void WeaponSystem::SwitchWeapon(WeaponType newWeapon)
{
	//	ステップ１：今使っている武器の弾薬を記録
	m_weaponAmmoStatus[m_currentWeapon] = { m_currentAmmo, m_reserveAmmo };

	//	ステップ2：新しい武器に持ち替え
	m_currentWeapon = newWeapon;
	auto& weapon = m_weaponStats[newWeapon];	//	&は「参照」　＝　コピーしない

	//	ステップ３：新武器の保存されている弾薬を読み込む
	m_currentAmmo = m_weaponAmmoStatus[newWeapon].currentAmmo;
	m_reserveAmmo = m_weaponAmmoStatus[newWeapon].reserveAmmo;
	m_maxAmmo = weapon.maxAmmo;

	//	リロード状態をリセット
	m_reloadTimer = 0.0f;
	m_isReloading = false;
}

//	武器購入：ポイント消費して新武器を手に入れる
void WeaponSystem::BuyWeapon(WeaponType weaponType, int& playerPoints)
{
	//	ケース１：すでに装備中の武器を選択→弾薬だけ購入
	if (m_currentWeapon == weaponType)
	{
		int cost = m_weaponStats[weaponType].cost / 2;	//	弾薬は半額
		if (playerPoints >= cost)
		{
			playerPoints -= cost;	//	ポイントを減らす
			m_reserveAmmo = m_weaponStats[weaponType].reserveAmmo;	//	満タンに
			m_weaponAmmoStatus[m_currentWeapon].reserveAmmo = m_reserveAmmo;
		}
		return;
	}

	//	ケース２：既に所持している場合→無料で切り替え
	if (m_primaryWeapon == weaponType ||
		(m_hasSecondaryWeapon && m_secondaryWeapon == weaponType))
	{
		SwitchWeapon(weaponType);
		return;
	}

	//	ケース３：新規購入
	int cost = m_weaponStats[weaponType].cost;

	if (playerPoints < cost)
		return;	//	ポイントが足りない

	playerPoints -= cost;	//	支払い

	//	現在のスロットに新武器を配置
	if (m_currentWeaponSlot == 0)
		m_primaryWeapon = weaponType;
	else
		m_secondaryWeapon = weaponType;

	//	初めて２つ目の武器を買うときの処理
	if (!m_hasSecondaryWeapon)
	{
		m_secondaryWeapon = WeaponType::PISTOL;	//	ピストルをセカンダリに
		m_hasSecondaryWeapon = true;
	}

	SwitchWeapon(weaponType);	//	購入した武器に持ち替え
}
