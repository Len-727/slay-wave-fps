//	WeaponSystem.cpp
#include "WeaponSpawn.h"
#include <Windows.h>
#include <cmath>

using namespace DirectX;

WeaponSpawnSystem::WeaponSpawnSystem()
{

}

//	===	�Ǖ���z�u	===
void WeaponSpawnSystem::CreateDefaultSpawns()
{
	m_spawns.clear();

	//	===	�Ǖ���z�u	===

	//	�����̕�
	m_spawns.push_back(WeaponSpawn(
		XMFLOAT3(-10.0f, 1.0f, -20.0f),	//	�ʒu
		WeaponType::RIFLE,				//	����RIFLE
		1000							//	���i
	));
	//	�E���̕�
	m_spawns.push_back(WeaponSpawn(
		XMFLOAT3(10.0f, 1.0f, -20.0f),
		WeaponType::SHOTGUN,
		1500
	));
	//	��O�̕�
	m_spawns.push_back(WeaponSpawn(
		XMFLOAT3(0.0f, 1.0f, 20.0f),
		WeaponType::RIFLE,
		1200
	));
	//	���̕�
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


//	===	�e�N�X�`���ǂݍ���	===
bool WeaponSpawnSystem::InitializeTextures(ID3D11Device* device, ID3D11DeviceContext* context)
{
	//	�����ނ��Ƃ̃e�N�X�`���t�@�C���p�X
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

	//	�e�Ǖ���Ƀe�N�X�`����ݒ�
	for (auto& spawn : m_spawns)
	{
		//	TexturedQuad���쐬
		spawn.wallTexture = std::make_unique<TexturedQuad>();

		if (!spawn.wallTexture->Initialize(device, context))
		{
			OutputDebugStringA("WeaponSpawnSystem::InitializeTextures - Failed to initialize quad\n");
			continue;
		}

		//	�Ή�����e�N�X�`���t�@�C����T��
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

//	===	�ǃe�N�X�`����`��	===
void WeaponSpawnSystem::DrawWallTextures(ID3D11DeviceContext* context,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection)
{
	for (const auto& spawn : m_spawns)
	{
		if (!spawn.wallTexture)
			continue;

		//	���[���h�s��(�ʒu�E�T�C�Y)
		XMMATRIX scale = XMMatrixScaling(2.0f, 2.0f, 1.0f);
		XMMATRIX translation = XMMatrixTranslation(
			spawn.position.x,
			spawn.position.y,
			spawn.position.z
			);

		XMMATRIX world = scale * translation;

		//	�`��
		spawn.wallTexture->Draw(context, world, view, projection);
	}
}


//	�v���C���[���Ǖ���̋߂��ɂ��邩�`�F�b�N
WeaponSpawn* WeaponSpawnSystem::CheckPlayerNearWeapon(DirectX::XMFLOAT3 playerPos)
{
	//	---�S�Ă̕Ǖ�����`�F�b�N	---
	for (auto& spawn : m_spawns)
	{
		if (!spawn.isActive)
			continue;

		//	---	�������v�Z	---
		float dx = playerPos.x - spawn.position.x;
		float dy = playerPos.y - spawn.position.y;
		float dz = playerPos.z - spawn.position.z;
		float distance = sqrtf(dx * dx + dy * dy + dz * dz);

		//	---	�w���\��������	===
		if (distance < PURCHASE_RANGE)
		{
			return &spawn;
		}
	}

	return nullptr;
}