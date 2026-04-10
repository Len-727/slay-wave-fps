//  // WeaponSystem.h - 武器管理システム

#pragma once
#pragma once
#include "Entities.h"
#include <map>

class WeaponSystem {
public:
    WeaponSystem();

    void InitializeWeapons();
    void SwitchWeapon(WeaponType newWeapon);
    void BuyWeapon(WeaponType weaponType, int& playerPoints);

    // ゲッター
    WeaponType GetCurrentWeapon() const { return m_currentWeapon; }
    WeaponType GetPrimaryWeapon() const { return m_primaryWeapon; }
    WeaponType GetSecondaryWeapon() const { return m_secondaryWeapon; }
    int GetCurrentWeaponSlot() const { return m_currentWeaponSlot; }
    bool HasSecondaryWeapon() const { return m_hasSecondaryWeapon; }

    int GetCurrentAmmo() const { return m_currentAmmo; }
    int GetMaxAmmo() const { return m_maxAmmo; }
    int GetReserveAmmo() const { return m_reserveAmmo; }

    bool IsReloading() const { return m_isReloading; }
    float GetReloadTimer() const { return m_reloadTimer; }
    float GetFireRateTimer() const { return m_fireRateTimer; }

    const WeaponData& GetWeaponData(WeaponType type) const { return m_weaponStats.at(type); }

    // セッター（弾薬更新など）
    void SetCurrentAmmo(int ammo) { m_currentAmmo = ammo; }
    void SetReserveAmmo(int ammo) { m_reserveAmmo = ammo; }
    void SetReloading(bool reloading) { m_isReloading = reloading; }
    void SetReloadTimer(float timer) { m_reloadTimer = timer; }
    void SetFireRateTimer(float timer) { m_fireRateTimer = timer; }
    void SetCurrentWeaponSlot(int slot) { m_currentWeaponSlot = slot; }

    void SaveAmmoStatus() {
        m_weaponAmmoStatus[m_currentWeapon] = { m_currentAmmo, m_reserveAmmo };
    }

private:
    std::map<WeaponType, WeaponData> m_weaponStats;
    std::map<WeaponType, WeaponAmmo> m_weaponAmmoStatus;

    WeaponType m_currentWeapon;
    WeaponType m_primaryWeapon;
    WeaponType m_secondaryWeapon;
    int m_currentWeaponSlot;
    bool m_hasSecondaryWeapon;

    int m_currentAmmo;
    int m_maxAmmo;
    int m_reserveAmmo;

    bool m_isReloading;
    float m_reloadTimer;
    float m_fireRateTimer;
};