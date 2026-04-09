//	Model.h	3D���f���ǂݍ��݁E�`��V�X�e��

#pragma once

#include <d3d11.h>			//	GPU�Ƀf�[�^�𑗂�		ID3D11Device, Id3D11DeviceContext, ID3D11Buffer ��
#include <DirectXMath.h>	//	3D���f���̈ʒu�E��]�E�X�P�[�����v�Z����
#include <wrl/client.h>		//	���������[�N��h��
#include <vector>			//	���_�f�[�^�⃁�b�V����ۑ����邽��
#include <string>			//	�t�@�C���p�X������
#include <memory>			//	BasicEffect�@���̃I�u�W�F�N�g�������Ǘ����邽��
#include <map>				//	�A�j���[�V���������f�[�^�̃}�b�v
#include <Effects.h>
#include <CommonStates.h>

#include "Entities.h"
#include "InstanceData.h"

struct aiScene;
struct aiNode;


//	===	���_�f�[�^�\����	===
struct ModelVertex {
	DirectX::XMFLOAT3 position;	//	�ʒu(x, y, z)
	DirectX::XMFLOAT3 normal;	//	�@��(���̌v�Z�p) - �ʂ̌���
	DirectX::XMFLOAT2 texCoord;	//	�e�N�X�`�����W(u, v) - �摜�̓\��t���ʒu

	//	---	�{�[���X�L�j���O	---
	uint32_t boneIndices[4];	//	�ځ[��C���f�b�N�X
	float boneWeights[4];	//	�{�[���E�F�C�g


	//	�R���X�g���N�^
	ModelVertex()
		: position(0, 0, 0)
		, normal(0, 1, 0)
		, texCoord(0, 0)
		, boneIndices{ 0, 0, 0, 0 }
		, boneWeights{ 0.0f, 0.0f, 0.0f, 0.0f }
	{

	}
};

//	===	�K�w�\����ێ����邽�߂̃m�[�h	===
struct Node
{
	std::string name;
	DirectX::XMMATRIX transformation;	//	�����ϊ��s��
	int parentIndex;	
	std::vector<int> children;

	int boneIndex = -1;
};


//	===	���b�V���\���� ===
//	�y�Ӗ��z3D���f���̈ꕔ��(�́A���A����Ȃ�)
struct Mesh {
	std::vector<ModelVertex> vertices;	//	���_�f�[�^(�O�p�`�̊p�̕���)
	std::vector<uint32_t> indices;		//	�C���f�b�N�X(���_�̐ڑ�����)

	//	DirectX��GPU�����[�X
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;	//	GPU�̒��_�f�[�^
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;	//	GPU���index�f�[�^
};


//	===	3D���f���N���X ===
class Model {
public:

	//	�R���X�g���N�^
	Model();

	//	FBX/OBJ�@���̃t�@�C����ǂݍ���
	//	�y�����zdevice: DirectX �f�o�C�X (GPU�@�𑀍삷��I�u�W�F�N�g)
	//			filename: �t�@�C���p�X
	//	�y�߂�l�z���� true, ���s false
	bool LoadFromFile(ID3D11Device* device, const std::string& filename);

	//	���f���`��
	//	�y�����zcontext: DirectX�@�f�o�C�X�R���e�L�X�g(�`�施�߂𑗂�I�u�W�F�N�g)
	//			world: ���[���h�ϊ��s��i�ʒu�E��]�E�X�P�[���j
	//			view: �r���[�s��i�J�����̈ʒu�E�����j
	//			projection: �v���W�F�N�V�����s��i�������e�j
	//			color: �F�iRGBA�j
	void Draw(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMVECTOR color);

	size_t GetMeshCount() const { return m_meshes.size(); }

	// ���b�V���f�[�^�ւ̃A�N�Z�X�i�ؒf�V�X�e���p�j
	const std::vector<Mesh>& GetMeshes() const { return m_meshes; }

	// �e�N�X�`���̎擾
	ID3D11ShaderResourceView* GetTexture() const { return m_diffuseTexture.Get(); }

	//	FBX�t�@�C������A�j���[�V������ǂݍ���
	bool LoadAnimation(const std::string& filename, const std::string& animationName);

	void DrawAnimated_Legacy(
		ID3D11DeviceContext* context,
		DirectX::XMMATRIX world, DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection, DirectX::XMVECTOR color,
		const std::string& animationName, float animationTime);

	//	�A�j���[�V�����t���ŕ`��
	void DrawAnimated(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMVECTOR color,
		const std::string& animationName,
		float animationTime);

	// �{�[���X�P�[���K�p�t���`��i�A�j���[�V�����Ȃ��j
	void DrawWithBoneScale(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMVECTOR color);

	//	�C���X�^���V���O�`��
	void DrawInstanced(
		ID3D11DeviceContext* context,
		const std::vector<InstanceData>& instances,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		const std::string& animationName,
		float animationTime
	);

	//	�A�j���[�V�����̒������擾
	//	�A�j���[�V�����̃��[�v����Ɏg��
	float GetAnimationDuration(const std::string& animationName) const;

	//	�A�j���[�V�������ǂݍ��܂�Ă��邩�m�F
	bool HasAnimation(const std::string& animationName) const;

	//	����̃{�[���̃X�P�[����ύX����
	void SetBoneScale(const std::string& boneName, float scale);

	void SetBoneScaleByPrefix(const std::string& prefix, float scale);

	void PrintBoneNames() const;

	//	�e�N�X�`����ݒ�
	void SetTexture(ID3D11ShaderResourceView* texture);

	// �J�X�^���V�F�[�_�[��ǂݍ���
	bool LoadCustomShaders(ID3D11Device* device);


	// �C���X�^���V���O�p�o�b�t�@���쐬
	bool CreateInstanceBuffers(ID3D11Device* device, int maxInstances);

	// �C���X�^���V���O�`��i�������f���̓G���܂Ƃ߂ĕ`��j
	void DrawInstanced_Custom(
		ID3D11DeviceContext* context,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		const std::vector<DirectX::XMMATRIX>& worlds,
		const std::vector<DirectX::XMVECTOR>& colors,
		const std::vector<std::string>& animNames,
		const std::vector<float>& animTimes
	);

	// �o�C���h�|�[�Y�̒��_���W���擾�i�ؒf�V�X�e���p�j
	// �A�j���[�V�����̍ŏ��̃t���[�����g���Ē��_��W�J����
	std::vector<ModelVertex> GetBindPoseVertices(int meshIndex);

	// �A�j���[�V�����|�[�Y�K�p�ς݂̒��_���擾�i�ؒf�p�j
	std::vector<ModelVertex> GetAnimatedVertices(
		int meshIndex,
		const std::string& animName,
		float animTime);

private:

	std::vector<DirectX::XMMATRIX> m_cachedBoneTransforms;  //  ���O�m�ۃL���b�V��

	// === �C���X�^���V���O�p ===
	struct InstanceGPU {
		DirectX::XMMATRIX World;       // 64 bytes
		DirectX::XMFLOAT4 Color;       // 16 bytes
		uint32_t BoneOffset;           // 4 bytes
		uint32_t pad[3];               // 12 bytes (16�o�C�g���E�ɑ�����)
	};  // ���v 96 bytes

	struct FrameCB {
		DirectX::XMMATRIX View;
		DirectX::XMMATRIX Projection;
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer>              m_instanceBuffer;     // �C���X�^���X�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D11Buffer>              m_boneStructuredBuf;  // StructuredBuffer
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_boneSRV;           // ����SRV
	Microsoft::WRL::ComPtr<ID3D11Buffer>              m_frameCB;           // View/Proj�萔�o�b�t�@
	int m_maxInstances = 0;
	int m_maxBoneEntries = 0;

	// === �J�X�^���V�F�[�_�[ ===
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_customVS;        // ���_�V�F�[�_�[
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_customPS;        // �s�N�Z���V�F�[�_�[
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_customInputLayout; // ���_�f�[�^�̍\����`
	Microsoft::WRL::ComPtr<ID3D11Buffer>        m_transformCB;     // �萔�o�b�t�@: Transform
	Microsoft::WRL::ComPtr<ID3D11Buffer>        m_boneCB;          // �萔�o�b�t�@: Bones
	Microsoft::WRL::ComPtr<ID3D11Buffer>        m_lightCB;         // �萔�o�b�t�@: Light
	Microsoft::WRL::ComPtr<ID3D11SamplerState>  m_customSampler;   // �e�N�X�`���T���v���[
	bool m_useCustomShader = false;  // �J�X�^���V�F�[�_�[���g����

	// �萔�o�b�t�@�̍\���́iHLSL�Ɗ��S�Ɉ�v������j
	struct TransformCB {
		DirectX::XMMATRIX World;
		DirectX::XMMATRIX View;
		DirectX::XMMATRIX Projection;
	};

	struct BoneCB {
		DirectX::XMMATRIX Bones[72];  // SkinnedEffect::MaxBones �Ɠ���
	};

	struct LightCB {
		DirectX::XMFLOAT4 AmbientColor;
		DirectX::XMFLOAT4 DiffuseColor;
		DirectX::XMFLOAT3 LightDirection;
		float Padding;
		DirectX::XMFLOAT3 CameraPos;
		float Padding2;
	};

	std::unique_ptr<DirectX::CommonStates> m_states;

	std::vector<Node> m_nodes;	//	�S�m�[�h���X�g
	int m_rootNodeIndex = 0;	//	���[�g�m�[�h�̃C���f�b�N�X

	//	�{�[�����ƃX�P�[���̃}�b�v
	std::map<std::string, float> m_boneScales;


	//	�O���[�o�����t�ϊ��s��
	DirectX::XMMATRIX m_globalInverseTransform;

	//	�ċA�I�Ƀm�[�h���X�V����w���p�[
	void ReadNodeHierarchy(const aiNode* srcNode, int parentIndex);

	//	�ċA�I�ɍs����X�V����w���p�[
	void UpdateNodeTransforms(
		int nodeIndex,
		const AnimationClip& clip,
		float animationTime,
		DirectX::XMMATRIX parentTransform,
		std::vector<DirectX::XMMATRIX>& outTransforms
	);

	//	���b�V���f�[�^(�����̃��b�V�����̂Ă�)
	std::vector<Mesh> m_meshes;

	//	DirectX�G�b�t�F�N�g(�V�F�[�_�[)
	std::unique_ptr<DirectX::SkinnedEffect> m_effect;				//	�`��G�b�t�F�N�g
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;	//	���_�f�[�^�̍\����`

	//	�{�[���\��
	std::vector<Bone> m_bones;

	//	���̃I�t�Z�b�g�s���ۑ�
	std::vector<DirectX::XMMATRIX> m_originalOffsetMatrices;

	//	�A�j���[�V�����N���b�v�̃}�b�v
	//	�����̃A�j���[�V�����𖼑O�ŊǗ�
	std::map<std::string, AnimationClip> m_animations;

	//	���_�o�b�t�@�E�C���f�b�N�X�o�b�t�@���쐬
	//	�y�����zGPU�̃f�[�^���@GPU �ɑ��M
	bool CreateBuffers(ID3D11Device* device, Mesh& mesh);

	//	�{�[���ϊ��s����v�Z
	//	�A�j���[�V���������ɑΉ�����{�[���s����v�Z
	void CalculateBoneTransforms(
		const std::string& animationName,
		float animationTime,
		std::vector<DirectX::XMMATRIX>& outTransforms
	);

	void BuildBoneHierarchy(const aiScene* scene);

	//void DebugDumpSkeleton();

	std::string GetShortName(const std::string& fullName);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_diffuseTexture;
};

