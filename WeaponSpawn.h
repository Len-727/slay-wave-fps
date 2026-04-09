//	WeaponSystem.h
#pragma once
#include <DirectXMath.h>
#include <vector>
#include "WeaponSystem.h"
#include "TexturedQuad.h"

struct WeaponSpawn
{
	DirectX::XMFLOAT3 position;	//	�Ǖ���̔z�u�ꏊ
	WeaponType weaponType;		//	�w���ł��镐��
	int cost;					//	�w���ɕK�v�ȃ|�C���g
	bool isActive;				//	�w���\��

	//	===	�e�N�X�`���t���|���S��	===
	std::unique_ptr<TexturedQuad> wallTexture;


	//	�R���X�g���N�^
	WeaponSpawn(DirectX::XMFLOAT3 pos, WeaponType weapon, int price)
		: position(pos), weaponType(weapon), cost(price), isActive(true),
		wallTexture(nullptr)
	{
	}
};

//	===	�Ǖ���Ǘ��V�X�e��	===
class WeaponSpawnSystem
{
public:
	//	�R���X�g���N�^
	WeaponSpawnSystem();

	//	�e����̃e�N�X�`����ǂݍ���
	bool InitializeTextures(ID3D11Device* device, ID3D11DeviceContext* context);

	//	�f�t�H���g�̕Ǖ���z�u
	//	�}�b�v�ɕǕ����z�u
	void CreateDefaultSpawns();

	//	�v���C���[���Ǖ���߂��ɂ��邩�`�F�b�N
	WeaponSpawn* CheckPlayerNearWeapon(DirectX::XMFLOAT3 playerPos);


	//	�S�Ă̕Ǖ�����擾
	const std::vector<WeaponSpawn>& GetSpawns() const { return m_spawns; }


	//	�ǃe�N�X�`���`��
	void DrawWallTextures(ID3D11DeviceContext* context,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection);

private:
	std::vector<WeaponSpawn> m_spawns;	//	�Ǖ��탊�X�g

	const float PURCHASE_RANGE = 2.0f;	//	�w���\����(2���[�g��)

};


