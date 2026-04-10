//	WeaponSystem.cpp
#include "WeaponSpawn.h"
#include <Windows.h>
#include <cmath>

using namespace DirectX;

WeaponSpawnSystem::WeaponSpawnSystem()
{

}

//	===	壁武器配置	===
void WeaponSpawnSystem::CreateDefaultSpawns()
{
	m_spawns.clear();

	//	===	壁武器配置	===

	//	左奥の壁
	m_spawns.push_back(WeaponSpawn(
		XMFLOAT3(-10.0f, 1.0f, -20.0f),	//	位置
		WeaponType::RIFLE,				//	仮でRIFLE
		1000							//	価格
	));
	//	右奥の壁
	m_spawns.push_back(WeaponSpawn(
		XMFLOAT3(10.0f, 1.0f, -20.0f),
		WeaponType::SHOTGUN,
		1500
	));
	//	手前の壁
	m_spawns.push_back(WeaponSpawn(
		XMFLOAT3(0.0f, 1.0f, 20.0f),
		WeaponType::RIFLE,
		1200
	));
	//	左の壁
	m_spawns.push_back(WeaponSpawn(
		XMFLOAT3(-20.0f, 1.0f, 0.0f),
		WeaponType::SNIPER,
		2000
	));

	char debug[256];
	sprintf_s(debug, "WeaponSpawnSystem::CreateDefaultSpawns - Created %zu weapon spawns\n",
		m_spawns.size());
	OutputDebugStringA(debug);

}


//	===	テクスチャ読み込み	===
bool WeaponSpawnSystem::InitializeTextures(ID3D11Device* device, ID3D11DeviceContext* context)
{
	//	武器種類ごとのテクスチャファイルパス
	struct WeaponTextureInfo
	{
		WeaponType type;
		const wchar_t* path;
	};

	WeaponTextureInfo textures[] =
	{
		{WeaponType::PISTOL, L"Assets/Texture/PISTOL.png"},
		{WeaponType::SHOTGUN, L"Assets/Texture/SHOTGUN.png"},
		{WeaponType::RIFLE, L"Assets/Texture/RIFLE.png"},
		{WeaponType::SNIPER, L"Assets/Texture/SNIPER.png"}
	};

	//	各壁武器にテクスチャを設定
	for (auto& spawn : m_spawns)
	{
		//	TexturedQuadを作成
		spawn.wallTexture = std::make_unique<TexturedQuad>();

		if (!spawn.wallTexture->Initialize(device, context))
		{
			OutputDebugStringA("WeaponSpawnSystem::InitializeTextures - Failed to initialize quad\n");
			continue;
		}

		//	対応するテクスチャファイルを探す
		const wchar_t* texturePath = nullptr;
		for (const auto& info : textures)
		{
			if (info.type == spawn.weaponType)
			{
				texturePath = info.path;
				break;
			}
		}

		if (texturePath)
		{
			if (!spawn.wallTexture->LoadTexture(device, texturePath))
			{
				OutputDebugStringA("WeaponSpawnSystem::InitializeTextures - Failed to load texture\n");
			}
		}
	}

	OutputDebugStringA("WeaponSpawnSystem::InitializeTextures - Success\n");
	return true;
}

//	===	壁テクスチャを描画	===
void WeaponSpawnSystem::DrawWallTextures(ID3D11DeviceContext* context,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection)
{
	for (const auto& spawn : m_spawns)
	{
		if (!spawn.wallTexture)
			continue;

		//	ワールド行列(位置・サイズ)
		XMMATRIX scale = XMMatrixScaling(2.0f, 2.0f, 1.0f);
		XMMATRIX translation = XMMatrixTranslation(
			spawn.position.x,
			spawn.position.y,
			spawn.position.z
			);

		XMMATRIX world = scale * translation;

		//	描画
		spawn.wallTexture->Draw(context, world, view, projection);
	}
}


//	プレイヤーが壁武器の近くにいるかチェック
WeaponSpawn* WeaponSpawnSystem::CheckPlayerNearWeapon(DirectX::XMFLOAT3 playerPos)
{
	//	---全ての壁武器をチェック	---
	for (auto& spawn : m_spawns)
	{
		if (!spawn.isActive)
			continue;

		//	---	距離を計算	---
		float dx = playerPos.x - spawn.position.x;
		float dy = playerPos.y - spawn.position.y;
		float dz = playerPos.z - spawn.position.z;
		float distance = sqrtf(dx * dx + dy * dy + dz * dz);

		//	---	購入可能距離内か	===
		if (distance < PURCHASE_RANGE)
		{
			return &spawn;
		}
	}

	return nullptr;
}