//	WeapomSystem.cpp
#include "WeaponSystem.h"

//	�R���X�g���N�^: ����V�X�e����������ԂɃZ�b�g
WeaponSystem::WeaponSystem()
	: m_currentWeapon(WeaponType::SHOTGUN)	//	���݂̕���F�s�X�g��
	, m_primaryWeapon(WeaponType::SHOTGUN)	//	�v���C�}���X���b�g�F�s�X�g��
	, m_secondaryWeapon(WeaponType::SHOTGUN)	//	�Z�J���_���X���b�g�F���g�p����������
	, m_currentWeaponSlot(0)				//	���݂̃X���b�g�F�O��
	, m_hasSecondaryWeapon(false)			//	�Q�ڂ̕���F�܂������Ă��Ȃ�
	, m_currentAmmo(20)						//	���U�e���F�W��
	, m_maxAmmo(20)							//	�ő呕�U���F�W��
	, m_reserveAmmo(120)						//	�\���e��F�W�O��
	, m_isReloading(false)					//	�����[�h���F������
	, m_reloadTimer(0.0f)					//	�����[�h�^�C�}�[0�b
	, m_fireRateTimer(0.0f)					//	�A�ː����^�C�}�[
{
	InitializeWeapons();
}

void WeaponSystem::InitializeWeapons()
{
	 //	�s�X�g��(��������E����)
	m_weaponStats[WeaponType::PISTOL] = {
		WeaponType::PISTOL, 30, 8, 80, 0.2f, 50.0f, 1, 1.5f, 0
	};

	//	�V���b�g�K���i�ߋ��������E500�|�C���g�j
	m_weaponStats[WeaponType::SHOTGUN] = {
		WeaponType::SHOTGUN, 200, 20, 120, 0.3f, 10.0f, 1, 1.0f, 500
	};

	//	���C�t���i�������o�����X�^�E1000�|�C���g�j
	m_weaponStats[WeaponType::RIFLE] = {
		WeaponType::RIFLE, 18, 30, 180, 1.0f, 80.0f, 1, 2.1f, 1000
	};

	//	�X�i�C�p�[�i�������ꌂ�E1500�|�C���g�j
	m_weaponStats[WeaponType::SNIPER] = {
		WeaponType::SNIPER, 500, 5, 50, 1.2f, 200.0f, 5, 3.0f, 1500
	};

	//	�e����̒e���Ԃ��������i�ŏ��͖��^���j
	m_weaponAmmoStatus[WeaponType::PISTOL] = { 8, 80 };
	m_weaponAmmoStatus[WeaponType::SHOTGUN] = { 20, 120 };
	m_weaponAmmoStatus[WeaponType::RIFLE] = { 30, 180 };
	m_weaponAmmoStatus[WeaponType::SNIPER] = { 5, 50 };
}

//	����؂�ւ��F���݂̒e���ێ����V����̒e��𕜌�
void WeaponSystem::SwitchWeapon(WeaponType newWeapon)
{
	//	�X�e�b�v�P�F���g���Ă��镐��̒e����L�^
	m_weaponAmmoStatus[m_currentWeapon] = { m_currentAmmo, m_reserveAmmo };

	//	�X�e�b�v2�F�V��������Ɏ����ւ�
	m_currentWeapon = newWeapon;
	auto& weapon = m_weaponStats[newWeapon];	//	&�́u�Q�Ɓv�@���@�R�s�[���Ȃ�

	//	�X�e�b�v�R�F�V����̕ۑ�����Ă���e���ǂݍ���
	m_currentAmmo = m_weaponAmmoStatus[newWeapon].currentAmmo;
	m_reserveAmmo = m_weaponAmmoStatus[newWeapon].reserveAmmo;
	m_maxAmmo = weapon.maxAmmo;

	//	�����[�h��Ԃ����Z�b�g
	m_reloadTimer = 0.0f;
	m_isReloading = false;
}

//	����w���F�|�C���g����ĐV�������ɓ����
void WeaponSystem::BuyWeapon(WeaponType weaponType, int& playerPoints)
{
	//	�P�[�X�P�F���łɑ������̕����I�����e�򂾂��w��
	if (m_currentWeapon == weaponType)
	{
		int cost = m_weaponStats[weaponType].cost / 2;	//	�e��͔��z
		if (playerPoints >= cost)
		{
			playerPoints -= cost;	//	�|�C���g�����炷
			m_reserveAmmo = m_weaponStats[weaponType].reserveAmmo;	//	���^����
			m_weaponAmmoStatus[m_currentWeapon].reserveAmmo = m_reserveAmmo;
		}
		return;
	}

	//	�P�[�X�Q�F���ɏ������Ă���ꍇ�������Ő؂�ւ�
	if (m_primaryWeapon == weaponType ||
		(m_hasSecondaryWeapon && m_secondaryWeapon == weaponType))
	{
		SwitchWeapon(weaponType);
		return;
	}

	//	�P�[�X�R�F�V�K�w��
	int cost = m_weaponStats[weaponType].cost;

	if (playerPoints < cost)
		return;	//	�|�C���g������Ȃ�

	playerPoints -= cost;	//	�x����

	//	���݂̃X���b�g�ɐV�����z�u
	if (m_currentWeaponSlot == 0)
		m_primaryWeapon = weaponType;
	else
		m_secondaryWeapon = weaponType;

	//	���߂ĂQ�ڂ̕���𔃂��Ƃ��̏���
	if (!m_hasSecondaryWeapon)
	{
		m_secondaryWeapon = WeaponType::PISTOL;	//	�s�X�g�����Z�J���_����
		m_hasSecondaryWeapon = true;
	}

	SwitchWeapon(weaponType);	//	�w����������Ɏ����ւ�
}
