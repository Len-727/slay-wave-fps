//	WeaponSystem.h
#pragma once
#include <DirectXMath.h>
#include <vector>
#include "WeaponSystem.h"
#include "TexturedQuad.h"

struct WeaponSpawn
{
	DirectX::XMFLOAT3 position;	//	壁武器の配置場所
	WeaponType weaponType;		//	購入できる武器
	int cost;					//	購入に必要なポイント
	bool isActive;				//	購入可能か

	//	===	テクスチャ付き板ポリゴン	===
	std::unique_ptr<TexturedQuad> wallTexture;


	//	コンストラクタ
	WeaponSpawn(DirectX::XMFLOAT3 pos, WeaponType weapon, int price)
		: position(pos), weaponType(weapon), cost(price), isActive(true),
		wallTexture(nullptr)
	{
	}
};

//	===	壁武器管理システム	===
class WeaponSpawnSystem
{
public:
	//	コンストラクタ
	WeaponSpawnSystem();

	//	各武器のテクスチャを読み込む
	bool InitializeTextures(ID3D11Device* device, ID3D11DeviceContext* context);

	//	デフォルトの壁武器配置
	//	マップに壁武器を配置
	void CreateDefaultSpawns();

	//	プレイヤーが壁武器近くにいるかチェック
	WeaponSpawn* CheckPlayerNearWeapon(DirectX::XMFLOAT3 playerPos);


	//	全ての壁武器を取得
	const std::vector<WeaponSpawn>& GetSpawns() const { return m_spawns; }


	//	壁テクスチャ描画
	void DrawWallTextures(ID3D11DeviceContext* context,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection);

private:
	std::vector<WeaponSpawn> m_spawns;	//	壁武器リスト

	const float PURCHASE_RANGE = 2.0f;	//	購入可能距離(2メートル)

};


