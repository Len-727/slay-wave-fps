//	Model.cpp
#include "Model.h"
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <assimp/Importer.hpp>	//	Assimp�@�̃C���|�[�^�[	(FBX, OBJ)
#include <assimp/scene.h>		//	�ǂݍ��񂾃V�[��(�m�[�h�K�w�E���b�V���E�}�e���A���E�A�j���[�V����)�ɃA�N�Z�X����
#include <assimp/postprocess.h>	//	�O�p�`���E�@��/�ڐ퐶���E���_�����Ȃǂ̌㏈���t���O
#include <CommonStates.h>
#include <Effects.h>
#include <VertexTypes.h> // DirectXTK�̕W�����_�^��`
#include <functional>
#include <algorithm>
#include <WICTextureLoader.h>
#include <functional>  // std::function�p

#include "InstanceData.h"

#pragma comment(lib, "assimp-vc143-mt.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

using std::min;

//using namespace Colors;

Model::Model()
{
}


std::string Model::GetShortName(const std::string& fullName)
{
	std::string name = fullName;

	// �R�������O�̕������폜
	size_t colonPos = name.find(':');
	if (colonPos != std::string::npos)
	{
		name = name.substr(colonPos + 1);
	}


	// Assimp���t����]�v�ȃT�t�B�b�N�X���폜���鏈����//�i�ŏ����j
	const std::string assimpSuffix = "_$AssimpFbx$_";
	size_t assimpPos = name.find(assimpSuffix);
	if (assimpPos != std::string::npos)
	{
		name = name.substr(0, assimpPos);
	}

	return name;
}


//	�t�@�C������3D���f����ǂݍ���
bool Model::LoadFromFile(ID3D11Device* device, const std::string& filename)
{
	//	=== Assimp �t�@�C���ǂݍ��� ===
	Assimp::Importer importer;

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	//	�t�@�C����ǂݍ���
	//	�y�t���O�z�ǂ��������邩�w��
	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_Triangulate |			//	�S�ĎO�p�`�ɕϊ�
		aiProcess_ConvertToLeftHanded |	//	DirectX�p�ɍ�����W�n��
		aiProcess_GenNormals |			//	�@������������
		aiProcess_CalcTangentSpace |	    //	�^���W�F���g�v�Z
		aiProcess_FlipUVs               // ���W�𔽓] (�e�N�X�`�����㉺�t�ɂȂ�̂��C���B�f�B�t���[�Y�J���[�ɂ͖��֌W)
		//aiProcess_FlipNormals             // 
	);

	//	�ǂݍ��ݎ��s�`�F�b�N
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		//	�G���[���b�Z�[�W���o��
		/*OutputDebugStringA("Model::LoadFromFIle - Failed to load: ");
		OutputDebugStringA(filename.c_str());
		OutputDebugStringA("\n");
		OutputDebugStringA(importer.GetErrorString());
		OutputDebugStringA("\n");*/
		return false;
	}

	//	=== DirectXTK �̃G�b�t�F�N�g�쐬 ===
	//	BasicEffect��"���C�e�B���O�p�ɐݒ�"
	m_effect = std::make_unique<DirectX::SkinnedEffect>(device);	//	DirectXTK�̊�{�V�F�[�_(�`�����)���쐬
	m_effect->SetWeightsPerVertex(4);
	m_states = std::make_unique<DirectX::CommonStates>(device);

	//	---	�����{�[���s���ݒ�
	DirectX::XMMATRIX id = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX bones[DirectX::SkinnedEffect::MaxBones];

	for (int i = 0; i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		bones[i] = id;
	}

	m_effect->SetBoneTransforms(bones, DirectX::SkinnedEffect::MaxBones);

	m_effect->SetDiffuseColor(DirectX::Colors::White);

	m_effect->SetAlpha(1.0f);

	//	���̓��C�A�E�g���쐬
	//	BasicEffect�������Ŏg�����_�V�F�[�_�̃o�C�g�R�[�h�����o��
	void const* shaderByteCode;
	size_t byteCodeLength;
	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);


	//	---	���̓G�������g�L�q�z����`	---
	D3D11_INPUT_ELEMENT_DESC inputElements[] = {
		{ "SV_Position",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",         0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",       0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES",   0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};


	//	---	���̓��C�A�E�g���쐬	---
	HRESULT hr = device->CreateInputLayout(
		inputElements,	//	�G�������g�z��
		_countof(inputElements),	//	�G�������g��
		shaderByteCode,
		byteCodeLength,
		m_inputLayout.GetAddressOf()
	);

	if (FAILED(hr))
	{
		//	---	�G���[�̏ڍׂ��o��	---
		/*char errorMsg[512];
		sprintf_s(errorMsg, "Model::LoadFromFile - Failed to create input layout (HRESULT: 0x%08X)\n", hr);
		OutputDebugStringA(errorMsg);*/
		return false;
	}

	//OutputDebugStringA("Model::LoadFromFile - Input layout created successfully\n");

	//	===	�e���b�V��������	===
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* aiMesh = scene->mMeshes[i];

		Mesh mesh;

		//	===���_�f�[�^��ϊ�	===
		for (unsigned int v = 0; v < aiMesh->mNumVertices; v++)
		{
			ModelVertex vertex;

			//	�ʒu
			vertex.position.x = aiMesh->mVertices[v].x;
			vertex.position.y = aiMesh->mVertices[v].y;
			vertex.position.z = aiMesh->mVertices[v].z;

			//	�@��	(�ʂ̌�������\�����)
			if (aiMesh->HasNormals())
			{
				vertex.normal.x = aiMesh->mNormals[v].x;
				vertex.normal.y = aiMesh->mNormals[v].y;
				vertex.normal.z = aiMesh->mNormals[v].z;
			}
			else
			{
				vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0);	//	�f�t�H���g(�����)
			}

			//	�e�N�X�`�����W
			if (aiMesh->HasTextureCoords(0))
			{
				//	�e�N�X�`���[�͂Q����
				vertex.texCoord.x = aiMesh->mTextureCoords[0][v].x;
				vertex.texCoord.y = aiMesh->mTextureCoords[0][v].y;
			}
			else
			{
				vertex.texCoord = XMFLOAT2(0, 0);
			}
			//	���_���ЂƂÂ��������āA�`��p�̒��_�z��ɐςݏグ��B
			mesh.vertices.push_back(vertex);
		}


		//	===	�{�[�����̓ǂݍ���	===

		//	---	�q�̃��b�V���Ƀ{�[�������邩�m�F	---
		if (aiMesh->HasBones())
		{
			char debugBones[256];
			sprintf_s(debugBones, "Mesh %d has %d bones\n", i, aiMesh->mNumBones);
			OutputDebugStringA(debugBones);

			//	�e�{�[�������[�v
			//	�S�ẴE�F�C�g�����W
			for (unsigned int b = 0; b < aiMesh->mNumBones; b++)
			{
				aiBone* bone = aiMesh->mBones[b];

				//---	�{�[�������擾	---
				std::string boneName = bone->mName.C_Str();

				//	---�q�̃{�[����m_bones�ɑ��݂��邩�m�F	---
				//	�{�[���C���f�b�N�X���擾(���Ԗڂ̃{�[����)
				int boneIndex = -1;
				for (size_t j = 0; j < m_bones.size(); j++)
				{
					if (m_bones[j].name == boneName)
					{
						boneIndex = static_cast<int>(j);
						break;
					}
				}

				//	---	������Ȃ��ꍇ�͐V�K//	---
				if (boneIndex == -1)
				{
					//	�V�����{�[�����쐬
					Bone newBone;
					newBone.name = boneName;

					//	�I�t�Z�b�g�s���ݒ�
					//	�����p������{�[����Ԃւ̕ϊ�
					//	���_���{�[�����W�n�ɕϊ����邽�߂ɕK�v
					aiMatrix4x4 offsetMatrix = bone->mOffsetMatrix;

					//	aiMatrix4x4 �� DirectX::XMMATRIX �ɕϊ�
					newBone.offsetMatrix = DirectX::XMMATRIX(
						offsetMatrix.a1, offsetMatrix.b1, offsetMatrix.c1, offsetMatrix.d1,
						offsetMatrix.a2, offsetMatrix.b2, offsetMatrix.c2, offsetMatrix.d2,
						offsetMatrix.a3, offsetMatrix.b3, offsetMatrix.c3, offsetMatrix.d3,
						offsetMatrix.a4, offsetMatrix.b4, offsetMatrix.c4, offsetMatrix.d4
					);

					//	�e�{�[���͌��
					newBone.parentIndex = -1;

					//	�{�[���z���//
					m_bones.push_back(newBone);

					//	���̃{�[���̃C���f�b�N�X
					boneIndex = static_cast<int>(m_bones.size() - 1);

					char debugNewBone[256];
					sprintf_s(debugNewBone, "  Added bone[%d]: %s\n", boneIndex, boneName.c_str());
					OutputDebugStringA(debugNewBone);
				}

				//	---	���̃{�[�����e�����钸�_�����[�v	---
				//	�e���_�Ƀ{�[���C���f�b�N�X�ƃE�F�C�g��ݒ�
				for (unsigned int w = 0; w < bone->mNumWeights; w++)
				{
					aiVertexWeight weight = bone->mWeights[w];

					unsigned int vertexId = weight.mVertexId;
					float vertexWeight = weight.mWeight;

					//	---	���_�ԍ����͈͓����m�F	---
					if (vertexId >= mesh.vertices.size())
					{
						//	�͈͊O�i�G���[�j
						OutputDebugStringA("Warning: Vertex ID out of range\n");
						continue;
					}

					//	---���̒��_�̋󂢂Ă���X���b�g��T��	---
					ModelVertex& vertex = mesh.vertices[vertexId];

					bool added = false;
					for (int slot = 0; slot < 4; slot++)
					{
						//	�q�̃X���b�g���󂢂Ă��邩�m�F
						if (vertex.boneWeights[slot] == 0.0f)
						{
							//	�󂢂Ă��邱���ɐݒ�
							vertex.boneIndices[slot] = boneIndex;
							vertex.boneWeights[slot] = vertexWeight;
							added = true;
							break;
						}
					}

					//	---	�X���b�g�����܂��Ă�ꍇ	---
					if (!added)
					{
						//OutputDebugStringA("Warning: Vertex has more than 4 bone weights\n");
					}
				}
			}//nn

			//	---	�E�F�C�g���K��	---
			//	�e���_�̃E�F�C�g�����v1.0�ɂ���
			for (auto& vertex : mesh.vertices)
			{
				//	�E�F�C�g�̍��v���v�Z
				float totalWeight = 0.0f;
				for (int slot = 0; slot < 4; slot++)
				{
					totalWeight += vertex.boneWeights[slot];
				}

				//	���v��0�̏ꍇ
				if (totalWeight == 0.0f)
				{
					vertex.boneIndices[0] = 0;
					vertex.boneWeights[0] = 1.0f;
				}
				else if (totalWeight != 1.0f)
				{
					//	���K��(���v��1.0�ɂ���)
					for (int slot = 0; slot < 4; slot++)
					{
						vertex.boneWeights[slot] /= totalWeight;
					}
				}
			}
		}
		else
		{
			//OutputDebugStringA("Mesh has no bones - using default bone 0\n");

			for (auto& vertex : mesh.vertices)
			{
				vertex.boneIndices[0] = 0;
				vertex.boneWeights[0] = 1.0f;
			}
		}


		//	===	index�f�[�^��ϊ�		=== (�ǂ̒��_���Ȃ��ŎO�p�`����邩)
		//	���̃��b�V���Ɋ܂܂��S�Ă̖ʂ��P���������郋�[�v
		for (unsigned int f = 0; f < aiMesh->mNumFaces; f++)
		{
			//	[f]�Ԗڂ̃f�[�^�����[�J���ϐ��ɃR�s�[
			aiFace face = aiMesh->mFaces[f];

			//	���̖ʂ�����index(���_�ԍ�)��퓬���珇�ɓǂ�
			for (unsigned int idx = 0; idx < face.mNumIndices; idx++)
			{
				//	�ǂݎ�������_���W���A���O���b�V���̃C���f�b�N�X�z���//
				mesh.indices.push_back(face.mIndices[idx]);
			}
		}

		//	GPU�Ƀo�b�t�@���쐬
		if (!CreateBuffers(device, mesh))
		{
			//OutputDebugStringA("Model::LoadFromFile - Failed to create buffers\n");
			return false;
		}

		//	���b�V����ۑ�
		m_meshes.push_back(std::move(mesh));

		//// �f�o�b�O�F���b�V�����Ƃ̒��_�{�[�����
		//char meshDebug[256];
		//sprintf_s(meshDebug, "Mesh %d: %zu vertices, first vertex bone[0]=%d, weight=%.2f\n",
		//	i, mesh.vertices.size(),
		//	mesh.vertices[0].boneIndices[0],
		//	mesh.vertices[0].boneWeights[0]);
		//OutputDebugStringA(meshDebug);
	}


	// ���[�g�m�[�h�̕ϊ��s��́u�t�v�������ƂŁA���b�V�������_�ɖ߂��Čv�Z�ł��܂�
	aiMatrix4x4 globalInv = scene->mRootNode->mTransformation;
	globalInv.Inverse();
	m_globalInverseTransform = DirectX::XMMATRIX(
		globalInv.a1, globalInv.b1, globalInv.c1, globalInv.d1,
		globalInv.a2, globalInv.b2, globalInv.c2, globalInv.d2,
		globalInv.a3, globalInv.b3, globalInv.c3, globalInv.d3,
		globalInv.a4, globalInv.b4, globalInv.c4, globalInv.d4
	);

	// 2. �m�[�h�K�w���\�z
	m_nodes.clear();
	ReadNodeHierarchy(scene->mRootNode, -1);

	//DebugDumpSkeleton();

	//�f�o�b�O�o��
	char debug[256];
	sprintf_s(debug, "Model::LoadFromFIle = Success: %s (%zu meshes)\n",
		filename.c_str(), m_meshes.size());
	OutputDebugStringA(debug);

	//	===	���̃I�t�Z�b�g�s���ۑ�	===
	m_originalOffsetMatrices.clear();
	m_originalOffsetMatrices.reserve(m_bones.size());
	for (const auto& bone : m_bones)
	{
		m_originalOffsetMatrices.push_back(bone.offsetMatrix);
	}

	// =============================================================
	//  ���ߍ��݃e�N�X�`���iEmbedded Texture�j��ǂݍ���
	//  Mixamo��FBX Binary��PBR���[�N�t���[���g�����Ƃ������A
	//  DIFFUSE �ł͂Ȃ� BASE_COLOR ���Ƀe�N�X�`�����o�^����Ă���
	// =============================================================
	if (scene->mNumMaterials > 0)
	{
		// --- �����e�N�X�`���^�C�v�̈ꗗ---
		// Mixamo�� BASE_COLOR ���g�����Ƃ�����
		// �]���̃��f���� DIFFUSE ���g��
		aiTextureType textureTypes[] = {
			aiTextureType_DIFFUSE,
			aiTextureType_BASE_COLOR,
			aiTextureType_UNKNOWN,
		};

		bool textureLoaded = false;

		// --- �S�}�e���A���𑖍� ---
		for (unsigned int matIdx = 0; matIdx < scene->mNumMaterials && !textureLoaded; matIdx++)
		{
			aiMaterial* material = scene->mMaterials[matIdx];

			// �f�o�b�O�F�}�e���A�������o��
			aiString matName;
			material->Get(AI_MATKEY_NAME, matName);
			char debugMat[512];
			sprintf_s(debugMat, "[TEXTURE] Checking material[%d]: %s\n", matIdx, matName.C_Str());
			OutputDebugStringA(debugMat);

			// --- �e�e�N�X�`���^�C�v�����ԂɎ��� ---
			for (int t = 0; t < _countof(textureTypes) && !textureLoaded; t++)
			{
				aiString texturePath;

				if (material->GetTexture(textureTypes[t], 0, &texturePath) == AI_SUCCESS)
				{
					const char* path = texturePath.C_Str();

					char debugTex[512];
					sprintf_s(debugTex, "[TEXTURE] Found texture (type=%d): %s\n", textureTypes[t], path);
					OutputDebugStringA(debugTex);

					// --- ���ߍ��݃e�N�X�`�����擾 ---
					const aiTexture* embeddedTex = scene->GetEmbeddedTexture(path);

					if (embeddedTex)
					{
						OutputDebugStringA("[TEXTURE] Embedded texture found!\n");

						if (embeddedTex->mHeight == 0)
						{
							// ���k�`���iPNG/JPG�j�̃o�C�i���f�[�^
							HRESULT hr = DirectX::CreateWICTextureFromMemory(
								device,
								reinterpret_cast<const uint8_t*>(embeddedTex->pcData),
								embeddedTex->mWidth,
								nullptr,
								m_diffuseTexture.ReleaseAndGetAddressOf()
							);

							if (SUCCEEDED(hr))
							{
								m_effect->SetTexture(m_diffuseTexture.Get());
								OutputDebugStringA("[TEXTURE] Embedded texture loaded successfully!\n");
								textureLoaded = true;
							}
							else
							{
								char errMsg[256];
								sprintf_s(errMsg, "[TEXTURE] Failed (HR: 0x%08X)\n", hr);
								OutputDebugStringA(errMsg);
							}
						}
					}
					else
					{
						// �O���t�@�C���Ƃ��Ď���
						std::string texFile = path;
						std::wstring wTexFile(texFile.begin(), texFile.end());

						HRESULT hr = DirectX::CreateWICTextureFromFile(
							device, wTexFile.c_str(), nullptr,
							m_diffuseTexture.ReleaseAndGetAddressOf());

						if (FAILED(hr))
						{
							// ���f���Ɠ����t�H���_������
							std::string modelDir = filename.substr(0, filename.find_last_of("/\\") + 1);
							std::string fullPath = modelDir + texFile;
							std::wstring wFullPath(fullPath.begin(), fullPath.end());
							hr = DirectX::CreateWICTextureFromFile(
								device, wFullPath.c_str(), nullptr,
								m_diffuseTexture.ReleaseAndGetAddressOf());
						}

						if (SUCCEEDED(hr))
						{
							m_effect->SetTexture(m_diffuseTexture.Get());
							OutputDebugStringA("[TEXTURE] External texture loaded!\n");
							textureLoaded = true;
						}
					}
				}
			}
		}

		if (!textureLoaded)
		{
			OutputDebugStringA("[TEXTURE] No texture found in any material/type combination\n");

			// === �ŏI��i�Fscene->mTextures[] �𒼐ڃ`�F�b�N ===
			// �}�e���A���ɓo�^����ĂȂ��Ă��AFBX���ɖ��ߍ��܂�Ă�ꍇ������
			if (scene->mNumTextures > 0)
			{
				char debugEmb[256];
				sprintf_s(debugEmb, "[TEXTURE] Found %d embedded textures in scene, trying first one...\n",
					scene->mNumTextures);
				OutputDebugStringA(debugEmb);

				const aiTexture* embeddedTex = scene->mTextures[0];
				if (embeddedTex && embeddedTex->mHeight == 0)
				{
					HRESULT hr = DirectX::CreateWICTextureFromMemory(
						device,
						reinterpret_cast<const uint8_t*>(embeddedTex->pcData),
						embeddedTex->mWidth,
						nullptr,
						m_diffuseTexture.ReleaseAndGetAddressOf()
					);

					if (SUCCEEDED(hr))
					{
						m_effect->SetTexture(m_diffuseTexture.Get());
						OutputDebugStringA("[TEXTURE] Direct embedded texture loaded!\n");
					}
					else
					{
						char errMsg[256];
						sprintf_s(errMsg, "[TEXTURE] Direct embedded load failed (HR: 0x%08X)\n", hr);
						OutputDebugStringA(errMsg);
					}
				}
			}
			else
			{
				OutputDebugStringA("[TEXTURE] No embedded textures in scene at all\n");
			}
		}
	}

	return true;

}

//void Model::DebugDumpSkeleton()
//{
//	OutputDebugStringA("=== Skeleton Nodes ===\n");
//	for (size_t i = 0; i < m_nodes.size(); ++i)
//	{
//		const Node& n = m_nodes[i];
//		std::string shortName = GetShortName(n.name);
//
//		char buf[512];
//		sprintf_s(
//			buf,
//			"[%3zu] node='%s' short='%s' parent=%d boneIndex=%d\n",
//			i,
//			n.name.c_str(),
//			shortName.c_str(),
//			n.parentIndex,
//			n.boneIndex
//		);
//		OutputDebugStringA(buf);
//	}
//}



//	���_�o�b�t�@�E�C���f�b�N�X�o�b�t�@���쐬
//	�y�����zCPU �̃f�[�^�� GPU�ɓ]��
bool Model::CreateBuffers(ID3D11Device* device, Mesh& mesh)
{
	//	===	���_�o�b�t�@�쐬	===
	D3D11_BUFFER_DESC vbDesc = {};	//	�o�b�t�@�̃T�C�Y�E�p�r�E�A�N�Z�X�����܂Ƃ߂�\���̂�0�ŏ�����
	vbDesc.Usage = D3D11_USAGE_DEFAULT;	//	GPU ��p������	�g�p���@�f�t�H���g
	vbDesc.ByteWidth = static_cast<UINT>(mesh.vertices.size() * sizeof(ModelVertex));	//	�T�C�Y
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;	//	���_�o�b�t�@�Ƃ��Ďg��
	vbDesc.CPUAccessFlags = 0;	//	CPU����A�N�Z�X���Ȃ�


	//	�����f�[�^ (CPU����R�s�[���錳�f�[�^)
	D3D11_SUBRESOURCE_DATA vbData = {};	//	�����f�[�^�p�̓��ꕨ
	vbData.pSysMem = mesh.vertices.data();	//	CPU���̌��f�[�^�擪�A�h���X���w��


	//	GPU�Ƀo�b�t�@���쐬�@�{ �f�[�^�R�s�[
	//	&vbDesc �ǂ�ȃo�b�t�@���̎d�l��	&vbData �����f�[�^�̔�
	HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, mesh.vertexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	//	===	�C���f�b�N�X�o�b�t�@�̍쐬	===
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));	//	index�̌��@* �������̃T�C�Y = ���o�C�g��
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;	//	�C���f�b�N�X�o�b�t�@�Ƃ��Ĉ���
	ibDesc.CPUAccessFlags = 0;	//	CPU����G��Ȃ�

	D3D11_SUBRESOURCE_DATA ibData{};
	ibData.pSysMem = mesh.indices.data();

	hr = device->CreateBuffer(&ibDesc, &ibData, mesh.indexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	return true;

}

//	���f����`��
void Model::Draw(ID3D11DeviceContext* context,
	XMMATRIX world,
	XMMATRIX view,
	XMMATRIX projection,
	XMVECTOR color)
{
	//	===	�G�b�t�F�N�g�ɍs���ݒ�	===
	m_effect->SetWorld(world);				//	���f���ŗL�̕ϊ�(�ʒu�E��]�E�g�k)
	m_effect->SetView(view);				//	�J����(���_)�̕ϊ�(���[���h�r���[���r���[)
	m_effect->SetProjection(projection);	//	���e(�r���[���N���b�v:����/���ˉe)

	m_effect->SetDiffuseColor(color);

	//	===	���C�g�̐ݒ�	===
	m_effect->EnableDefaultLighting();

	//	���H�ݒ�
	m_effect->SetAmbientLightColor(XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));
	//	�f�B�t���[�Y�J���[�̓��f���S�̂̐F�ƌ��̔��ːF�����˂�
	m_effect->SetDiffuseColor(color);

	//	===	�G�t�F�N�g�K�p	===
	m_effect->Apply(context);
	// �f�o�b�O���O//
	//OutputDebugStringA("[MODEL] Draw called\n");

	// �[�x�������݂������L����
	if (m_states)
	{
		context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
		//OutputDebugStringA("[MODEL] Depth state set\n");
	}
	else
	{
		OutputDebugStringA("[MODEL] ERROR: m_states is null!\n");
	}
	//	===	���̓��C�A�E�g��ݒ�	===
	context->IASetInputLayout(m_inputLayout.Get());	//	���_�f�[�^�͂��̕���(InputLayout)�œǂ�
	//	���ꂩ�瑗�钸�_/index���O�p�`���X�g�Ƃ��ĉ���
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//	===	�e���b�V����`��	===
	for (const auto& mesh : m_meshes)
	{
		//	���_�o�b�t�@��ݒ�
		UINT stride = sizeof(ModelVertex);	//	1���_�̃T�C�Y
		UINT offset = 0;					//	�I�t�Z�b�g
		context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);

		//	�C���f�b�N�X�o�b�t�@��ݒ�
		context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//	�`�施��
		context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}

}


//	===	�A�j���[�V�����t�@�C����ǂݍ���	===
bool Model::LoadAnimation(const std::string& filename, const std::string& animationName)
{
	//	---	Assimp�Ńt�@�C����ǂݍ���	---
	Assimp::Importer importer;	//	�t�@�C����ǂݍ��ރN���X

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* scene = importer.ReadFile(
		filename,
		aiProcess_Triangulate | 
		aiProcess_ConvertToLeftHanded |
		aiProcess_FlipUVs);

	//	---	�G���[�`�F�b�N	---
	if (!scene || !scene->mRootNode)
	{
		//	�ǂݍ��ݎ��s
		/*OutputDebugStringA("Model::LoadAnimation - Failed to load file:");
		OutputDebugStringA(filename.c_str());
		OutputDebugStringA("\n");*/
		return false;
	}

	if (scene->mNumAnimations == 0)
	{
		//	�A�j���[�V�������܂܂�Ă��Ȃ�
		/*OutputDebugStringA("Model::LoadAnimation - No animations in file");
		OutputDebugStringA(filename.c_str());
		OutputDebugStringA("\n");*/
		return false;
	}

	//	---	�ŏ��̃A�j���[�V�������擾	---
	aiAnimation* anim = scene->mAnimations[0];

	//	AnimationClip �\���̂��쐬
	AnimationClip clip;
	
	//	���O��ݒ�
	clip.name = animationName;	//	�����œn���ꂽ���O

	//	�Đ����Ԃ�ݒ�
	clip.duration = static_cast<float>(anim->mDuration / anim->mTicksPerSecond);

	//	�Đ����x��ݒ�
	clip.ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);


	//	---	�e�{�[���̃A�j���[�V�����f�[�^(�`�����l��)�𒊏o
	for (unsigned int i = 0; i < anim->mNumChannels; i++)
	{
		aiNodeAnim* channel = anim->mChannels[i];

		//	AnimationChannel�@�\���̂��쐬
		AnimationChannel animChannel;

		//	�{�[������ݒ�
		animChannel.boneName = channel->mNodeName.C_Str();


		//	---	�L�[�t���[�����o	---

		//	�L�[�t���[�����̍ő�l���擾
		unsigned int numKeys = channel->mNumPositionKeys;	//	�Ƃ肠�����ʒu�̐�
		if (channel->mNumRotationKeys > numKeys)
			numKeys = channel->mNumRotationKeys;	//	��]�̂ق�������
		if (channel->mNumScalingKeys > numKeys)
			numKeys = channel->mNumScalingKeys;		//	�X�P�[���̂ق�������

		//	�L�[�t���[���z��̃T�C�Y���m��
		animChannel.keyframes.reserve(numKeys);

		//	�e�t���[��������
		for (unsigned int k = 0; k < numKeys; k++)
		{
			KeyFrame keyframe;

			//	---	������ݒ�	---
			if (k < channel->mNumPositionKeys)
			{
				keyframe.time = static_cast<float>(
					channel->mPositionKeys[k].mTime / anim->mTicksPerSecond);
			
				//	�y�v�Z�z�e�B�b�N�@���@�b�ɕϊ�
			}
			else
			{
				//	�ʒu�L�[�t���[����葽���ꍇ
				keyframe.time = 0.0f;
			}

			//	---	�ʒu��ݒ�	---
			if (k < channel->mNumPositionKeys)
			{
				aiVector3D pos = channel->mPositionKeys[k].mValue;
				keyframe.position = DirectX::XMFLOAT3(pos.x, pos.y, pos.z);
				//	�y�ϊ��zAssimp��aiVector3D�@���@DirectX �� XMFLOAT3
			}
			else
			{
				//	�ʒu�L�[�t���[�����Ȃ��ꍇ
				keyframe.position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			}

			//	---	��]��ݒ�(�N�H�[�^�j�I��)	---
			//	��Ԃ��X���[�Y
			//	����������������
			if (k < channel->mNumRotationKeys)
			{
				aiQuaternion rot = channel->mRotationKeys[k].mValue;
				keyframe.rotation = DirectX::XMFLOAT4(rot.x, rot.y, rot.z, rot.w);
				//	�y�ϊ��zAssimp �� aiQuaternion �� DirectX �� XMFLOAT4
			}
			else
			{
				//	��]�L�[�t���[�����Ȃ��ꍇ
				keyframe.rotation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
			}

			//	---	�X�P�[���ݒ�	---
			if (k < channel->mNumScalingKeys)
			{
				aiVector3D scale = channel->mScalingKeys[k].mValue;
				keyframe.scale = DirectX::XMFLOAT3(scale.x, scale.y, scale.z);

			}
			else
			{
				keyframe.scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
			}

			//	�L�[�t���[�����z���//
			animChannel.keyframes.push_back(keyframe);
		}
		//	�`�����l�����A�j���[�V�����N���b�v��//
		clip.channels.push_back(animChannel);
	}

	//---	�A�j���[�V�����N���b�v��ۑ�	---
	m_animations[animationName] = clip;

	// �f�o�b�O�o��
	/*char debugMsg[256];
	sprintf_s(debugMsg, "Model::LoadAnimation - Loaded: %s (%d channels, %.2f sec)\n",
		animationName.c_str(),
		static_cast<int>(clip.channels.size()),
		clip.duration);
	OutputDebugStringA(debugMsg);*/

	//for (const auto& channel : clip.channels)
	//{
	//	// ���̃��O
	//	OutputDebugStringA(("AnimChannel: " + channel.boneName + "\n").c_str());

	//	// //�̏ڍ׃��O
	//	std::string shortName = GetShortName(channel.boneName);
	//	char buf[256];
	//	sprintf_s(
	//		buf,
	//		"AnimChannel: full='%s' short='%s' keyframes=%u\n",
	//		channel.boneName.c_str(),
	//		shortName.c_str(),
	//		static_cast<unsigned>(channel.keyframes.size())
	//	);
	//	OutputDebugStringA(buf);
	//}





	return true;  // ����

}

//	===	�A�j���[�V�����̒������擾	===
float Model::GetAnimationDuration(const std::string& animationName) const
{
	//	---	�A�j���[�V����������	---
	auto it = m_animations.find(animationName);

	//	�����������m�F
	if (it != m_animations.end())
	{
		return it->second.duration;
	}
	return 0.0f;
}

//	===	�A�j���[�V�������ǂݍ��܂�Ă��邩�m�F	===
bool Model::HasAnimation(const std::string& animationName) const
{
	return m_animations.find(animationName) != m_animations.end();
}

//	===	�{�[���ϊ��s����v�Z	===
void Model::CalculateBoneTransforms(
	const std::string& animationName,
	float animationTime,
	std::vector<DirectX::XMMATRIX>& outTransforms)
{
	//	========================================================================
	//	�A�j���[�V����������
	//	========================================================================
	auto it = m_animations.find(animationName);
	if (it == m_animations.end())
	{
		/*OutputDebugStringA("Model::CalculateBoneTransforms - Animation not found: ");
		OutputDebugStringA(animationName.c_str());
		OutputDebugStringA("\n");*/
		return;
	}

	const AnimationClip& clip = it->second;

	//	�z��̃T�C�Y���m��
	outTransforms.resize(m_bones.size());

	//	�S�Ă̍s���P�ʍs��ŏ�����
	for (size_t i = 0; i < outTransforms.size(); i++)
	{
		outTransforms[i] = DirectX::XMMatrixIdentity();
	}

	//	========================================================================
	//	�X�e�b�v1: �e�{�[���̃��[�J���ϊ����v�Z
	//	========================================================================
	std::vector<DirectX::XMMATRIX> localTransforms(m_bones.size());

	for (size_t i = 0; i < m_bones.size(); i++)
	{
		localTransforms[i] = DirectX::XMMatrixIdentity();
	}

	//	�`�����l���i�{�[���j���ƂɃ��[�v
	for (const auto& channel : clip.channels)
	{
		//	���̃`�����l���̃{�[����T��
		int boneIndex = -1;
		for (size_t i = 0; i < m_bones.size(); i++)
		{
			if (m_bones[i].name == channel.boneName)
			{
				boneIndex = static_cast<int>(i);
				break;
			}
		}

		if (boneIndex == -1) continue;
		if (channel.keyframes.empty()) continue;

		//	====================================================================
		//	�L�[�t���[������
		//	====================================================================
		size_t frame0 = 0;
		size_t frame1 = 0;

		if (channel.keyframes.size() == 1)
		{
			frame0 = 0;
			frame1 = 0;
		}
		else
		{
			for (size_t i = 0; i < channel.keyframes.size() - 1; i++)
			{
				if (animationTime < channel.keyframes[i + 1].time)
				{
					frame0 = i;
					frame1 = i + 1;
					break;
				}
			}

			if (frame1 == 0)
			{
				frame0 = channel.keyframes.size() - 1;
				frame1 = channel.keyframes.size() - 1;
			}
		}

		//	====================================================================
		//	��ԌW�����v�Z
		//	====================================================================
		float t = 0.0f;

		if (frame0 != frame1)
		{
			float time0 = channel.keyframes[frame0].time;
			float time1 = channel.keyframes[frame1].time;
			float deltaTime = time1 - time0;

			if (deltaTime > 0.0f)
			{
				t = (animationTime - time0) / deltaTime;
				if (t < 0.0f) t = 0.0f;
				if (t > 1.0f) t = 1.0f;
			}
		}

		//	====================================================================
		//	�ʒu�̕�ԁi���`��ԁj
		//	====================================================================
		DirectX::XMFLOAT3 pos0 = channel.keyframes[frame0].position;
		DirectX::XMFLOAT3 pos1 = channel.keyframes[frame1].position;
		DirectX::XMVECTOR position0 = DirectX::XMLoadFloat3(&pos0);
		DirectX::XMVECTOR position1 = DirectX::XMLoadFloat3(&pos1);
		DirectX::XMVECTOR position = DirectX::XMVectorLerp(position0, position1, t);

		//	====================================================================
		//	��]�̕�ԁi���ʐ��`��ԁj
		//	====================================================================
		DirectX::XMFLOAT4 rot0 = channel.keyframes[frame0].rotation;
		DirectX::XMFLOAT4 rot1 = channel.keyframes[frame1].rotation;
		DirectX::XMVECTOR rotation0 = DirectX::XMLoadFloat4(&rot0);
		DirectX::XMVECTOR rotation1 = DirectX::XMLoadFloat4(&rot1);
		DirectX::XMVECTOR rotation = DirectX::XMQuaternionSlerp(rotation0, rotation1, t);

		//	====================================================================
		//	�X�P�[���̕�ԁi���`��ԁj
		//	====================================================================
		DirectX::XMFLOAT3 scl0 = channel.keyframes[frame0].scale;
		DirectX::XMFLOAT3 scl1 = channel.keyframes[frame1].scale;
		DirectX::XMVECTOR scale0 = DirectX::XMLoadFloat3(&scl0);
		DirectX::XMVECTOR scale1 = DirectX::XMLoadFloat3(&scl1);
		DirectX::XMVECTOR scale = DirectX::XMVectorLerp(scale0, scale1, t);

		//	====================================================================
		//	�ϊ��s����쐬
		//	====================================================================
		DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScalingFromVector(scale);
		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationQuaternion(rotation);
		DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(position);

		//	���[�J���ϊ���ۑ�
		localTransforms[boneIndex] = scaleMatrix * rotationMatrix * translationMatrix;
	}

	//	========================================================================
	//	�X�e�b�v2: �e�q�֌W���l�����ă��[���h�ϊ����v�Z
	//	========================================================================
	//	�y�d�v�z�e���珇�Ɍv�Z����K�v������

	for (size_t i = 0; i < m_bones.size(); i++)
	{
		if (m_bones[i].parentIndex == -1)
		{
			//	�e�����Ȃ��i���[�g�{�[���j
			outTransforms[i] = localTransforms[i];
		}
		else
		{
			//	�e������
			int parentIndex = m_bones[i].parentIndex;
			outTransforms[i] = localTransforms[i] * outTransforms[parentIndex];
		}
	}

	//	========================================================================
	//	�X�e�b�v3: �I�t�Z�b�g�s���K�p
	//	========================================================================
	for (size_t i = 0; i < outTransforms.size(); i++)
	{
		outTransforms[i] = m_bones[i].offsetMatrix * outTransforms[i];
	}

	//	========================================================================
	//	�f�o�b�O�o�́i�ŏ��̃t���[���̂݁j
	//	========================================================================
	static bool firstCall = true;
	if (firstCall)
	{
		firstCall = false;

		/*char debugMsg[512];
		sprintf_s(debugMsg, "=== CalculateBoneTransforms Debug ===\n");
		OutputDebugStringA(debugMsg);

		sprintf_s(debugMsg, "Animation: %s, Time: %.2f\n", animationName.c_str(), animationTime);
		OutputDebugStringA(debugMsg);

		sprintf_s(debugMsg, "Bones: %zu, Channels: %zu\n", m_bones.size(), clip.channels.size());
		OutputDebugStringA(debugMsg);*/

		// �ŏ���5�{�[���̐e�q�֌W��\��
		/*for (size_t i = 0; i < std::min((size_t)5, m_bones.size()); i++)
		{
			sprintf_s(debugMsg, "Bone[%zu]: %s (parent: %d)\n",
				i, m_bones[i].name.c_str(), m_bones[i].parentIndex);
			OutputDebugStringA(debugMsg);
		}*/
	}
}

void Model::BuildBoneHierarchy(const aiScene* scene)
{
	//	�e�{�[���̐e��T����parentIndex�ɐݒ�

	if (!scene->mRootNode)
		return;

	//OutputDebugStringA("===	Building Bone Hierarchy	===\n");

	//	---	�{�[�������C���f�b�N�X�}�b�v���쐻
	std::map<std::string, int> boneNameToIndex;
	for (size_t i = 0; i < m_bones.size(); i++)
	{
		boneNameToIndex[m_bones[i].name] = static_cast<int>(i);
	}

	//	---	�m�[�h������H���Đe�q�֌W��ݒ�	---
	std::function<void(aiNode*, int)> processNode = [&](aiNode* node, int parentIndex)
		{
			//	���̃m�[�h���{�[�����m�F
			std::string nodeName = node->mName.C_Str();
			auto it = boneNameToIndex.find(nodeName);

			int currentIndex = -1;
			if (it != boneNameToIndex.end())
			{
				//	�{�[������������
				currentIndex = it->second;
				m_bones[currentIndex].parentIndex = parentIndex;

				////	�f�o�b�O�o��
				//if (parentIndex == -1)
				//{
				//	char debugMsg[256];
				//	sprintf_s(debugMsg, "  Root bone[%d]: %s\n",
				//		currentIndex, nodeName.c_str());
				//	OutputDebugStringA(debugMsg);
				//}
				//else
				//{
				//	char debugMsg[256];
				//	sprintf_s(debugMsg, "  Bone[%d]: %s -> parent[%d]: %s\n",
				//		currentIndex, nodeName.c_str(),
				//		parentIndex, m_bones[parentIndex].name.c_str());
				//	OutputDebugStringA(debugMsg);
				//}
			}

			//	�q�m�[�h���ċA�I�ɏ���
			//	�e�̃C���f�b�N�X��n��
			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				processNode(node->mChildren[i], currentIndex != -1 ? currentIndex : parentIndex);
			}
		};

	//	���[�g�m�[�h����J�n
	processNode(scene->mRootNode, -1);

	//OutputDebugStringA("=== Bone Hierarchy Complete ===\n");
}

void Model::ReadNodeHierarchy(const aiNode* srcNode, int parentIndex)
{
	Node node;
	node.name = srcNode->mName.C_Str();
	node.parentIndex = parentIndex;

	// Assimp�s�� -> DirectX�s��
	aiMatrix4x4 t = srcNode->mTransformation;
	node.transformation = DirectX::XMMATRIX(
		t.a1, t.b1, t.c1, t.d1,
		t.a2, t.b2, t.c2, t.d2,
		t.a3, t.b3, t.c3, t.d3,
		t.a4, t.b4, t.c4, t.d4
	);

	// ���̃m�[�h���ɑΉ�����{�[�������邩�T��
	// (�������̂��ߖ{����map���g���ׂ��ł����A���͊�����m_bones���[�v�őΉ�)
	node.boneIndex = -1;
	std::string nodeName = node.name;

	for (size_t i = 0; i < m_bones.size(); ++i) 
	{
		if (m_bones[i].name == nodeName)
		{
			node.boneIndex = static_cast<int>(i);
			break;
		}
	}

	// �m�[�h���X�g��//
	int currentIndex = static_cast<int>(m_nodes.size());
	m_nodes.push_back(node);

	// �e�̎q���X�g�Ɏ�����//
	if (parentIndex != -1) {
		m_nodes[parentIndex].children.push_back(currentIndex);
	}
	else {
		m_rootNodeIndex = currentIndex;
	}

	// �q�m�[�h���ċA����
	for (unsigned int i = 0; i < srcNode->mNumChildren; ++i) {
		ReadNodeHierarchy(srcNode->mChildren[i], currentIndex);
	}
}

void Model::UpdateNodeTransforms(
	int nodeIndex,
	const AnimationClip& clip,
	float animationTime,
	DirectX::XMMATRIX parentTransform,
	std::vector<DirectX::XMMATRIX>& outTransforms)
{
	const Node& node = m_nodes[nodeIndex];
	std::string nodeName = node.name;

	//	��{�͂��̃m�[�h�̏����ϊ��s��
	DirectX::XMMATRIX nodeTransform = node.transformation;

	//	�A�j���[�V�����f�[�^��T��
	const AnimationChannel* channel = nullptr;
	std::string shortNodeName = GetShortName(node.name);
	for (const auto& c : clip.channels) {
		if (GetShortName(c.boneName) == shortNodeName) {
			channel = &c;
			break;
		}
	}

	// �f�o�b�O�F�Ή��֌W����x�����o��
	//static bool sPrinted = false;
	//if (!sPrinted && node.boneIndex != -1) {
	//	char buf[512];
	//	sprintf_s(
	//		buf,
	//		"[Match] node='%s' short='%s' boneIndex=%d  ->  channel='%s' (short='%s')\n",
	//		node.name.c_str(),
	//		shortNodeName.c_str(),
	//		node.boneIndex,
	//		channel ? channel->boneName.c_str() : "(none)",
	//		channel ? GetShortName(channel->boneName).c_str() : "(none)"
	//	);
	//	OutputDebugStringA(buf);
	//}
	//if (!sPrinted && nodeIndex == m_rootNodeIndex) {
	//	// 1�t���[�����o������~�߂����Ƃ��� true �ɂ��Ă�OK
	//	// sPrinted = true;
	//}


	//	�A�j���[�V����������΁A�s����v�Z���ď㏑��
	if (channel && !channel->keyframes.empty()) {
		//	---	�L�[�t���[������	---
		size_t frame0 = 0;
		size_t frame1 = 0;

		if (channel->keyframes.size() == 1) {
			frame0 = frame1 = 0;
		}
		else {
			for (size_t i = 0; i < channel->keyframes.size() - 1; i++) {
				if (animationTime < channel->keyframes[i + 1].time) {
					frame0 = i;
					frame1 = i + 1;
					break;
				}
			}
			//	�I�[�`�F�b�N
			if (frame1 == 0 && channel->keyframes.size() > 1) {
				frame0 = frame1 = channel->keyframes.size() - 1;
			}
		}

		//	---	��ԌW�� t	---
		float t = 0.0f;
		if (frame0 != frame1) {
			float time0 = channel->keyframes[frame0].time;
			float time1 = channel->keyframes[frame1].time;
			float deltaTime = time1 - time0;
			if (deltaTime > 0.0f) {
				t = (animationTime - time0) / deltaTime;
				if (t < 0.0f) t = 0.0f;
				if (t > 1.0f) t = 1.0f;
			}
		}

		//	---	��Ԍv�Z (Lerp / Slerp)	---
		//	�ʒu
		DirectX::XMFLOAT3 pos0 = channel->keyframes[frame0].position;
		DirectX::XMFLOAT3 pos1 = channel->keyframes[frame1].position;
		DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&pos0);
		DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&pos1);
		DirectX::XMVECTOR position = DirectX::XMVectorLerp(p0, p1, t);

		//	��]
		DirectX::XMFLOAT4 rot0 = channel->keyframes[frame0].rotation;
		DirectX::XMFLOAT4 rot1 = channel->keyframes[frame1].rotation;
		DirectX::XMVECTOR r0 = DirectX::XMLoadFloat4(&rot0);
		DirectX::XMVECTOR r1 = DirectX::XMLoadFloat4(&rot1);
		DirectX::XMVECTOR rotation = DirectX::XMQuaternionSlerp(r0, r1, t);

		//	�X�P�[��
		DirectX::XMFLOAT3 scl0 = channel->keyframes[frame0].scale;
		DirectX::XMFLOAT3 scl1 = channel->keyframes[frame1].scale;
		DirectX::XMVECTOR s0 = DirectX::XMLoadFloat3(&scl0);
		DirectX::XMVECTOR s1 = DirectX::XMLoadFloat3(&scl1);
		DirectX::XMVECTOR scale = DirectX::XMVectorLerp(s0, s1, t);

		//	--- �s�񍇐� ---
		DirectX::XMMATRIX scaleM = DirectX::XMMatrixScalingFromVector(scale);

		//	===============================
		//	===	�{�[���X�P�[���̋����K�p	===
		//	===============================
		std::string shortName = GetShortName(node.name);
		if (m_boneScales.find(shortName) != m_boneScales.end())
		{
			float s = m_boneScales[shortName];
			//	�A�j���[�V�����̃X�P�[���𖳎����āA�w�肵���X�P�[���ŏ㏑������
			scaleM = DirectX::XMMatrixScaling(s, s, s);
		}


		DirectX::XMMATRIX rotM = DirectX::XMMatrixRotationQuaternion(rotation);
		DirectX::XMMATRIX transM = DirectX::XMMatrixTranslationFromVector(position);

		//	�m�[�h�s����A�j���[�V�����p���ŏ㏑��
		nodeTransform = scaleM * rotM * transM;
	}

	//	�O���[�o���ϊ��s�� (�e * ����)
	DirectX::XMMATRIX globalTransform = nodeTransform * parentTransform;

	//	�{�[���Ȃ�ŏI�s����i�[
	if (node.boneIndex != -1) {
		// �I�t�Z�b�g * �O���[�o�� * �O���[�o���t�ϊ�(���_�␳)
		outTransforms[node.boneIndex] =
			m_bones[node.boneIndex].offsetMatrix * globalTransform * m_globalInverseTransform;
	}

	//	�q�m�[�h�֍ċA
	for (int childIndex : node.children) {
		UpdateNodeTransforms(childIndex, clip, animationTime, globalTransform, outTransforms);
	}
}

void Model::DrawAnimated_Legacy(
	ID3D11DeviceContext* context,
	DirectX::XMMATRIX world,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection,
	DirectX::XMVECTOR color,
	const std::string& animationName,
	float animationTime)
{
	
	//	�X�e�b�v1: �A�j���[�V���������邩�m�F
	auto it = m_animations.find(animationName);
	if (it == m_animations.end())
	{
		//	�A�j���[�V������������Ȃ��ꍇ�͒ʏ�`��
		/*OutputDebugStringA("Model::DrawAnimated - Animation not found: ");
		OutputDebugStringA(animationName.c_str());
		OutputDebugStringA("\n");
		Draw(context, world, view, projection, color);*/
		return;
	}

	const AnimationClip& clip = it->second;


	//  �����o�ϐ����ė��p�i���t���[��new���Ȃ��j
	if (m_cachedBoneTransforms.size() != m_bones.size())
		m_cachedBoneTransforms.resize(m_bones.size());
	auto& boneTransforms = m_cachedBoneTransforms;

	//	�S�Ă̍s���P�ʍs��ŏ�����
	for (size_t i = 0; i < boneTransforms.size(); i++)
	{
		boneTransforms[i] = DirectX::XMMatrixIdentity();
	}

	//	�m�[�h�K�w���g���ă{�[���ϊ����v�Z
	if (!m_nodes.empty())
	{
		//	�������̐S�IUpdateNodeTransforms ���Ă�
		UpdateNodeTransforms(
			m_rootNodeIndex,
			clip,
			animationTime,
			DirectX::XMMatrixIdentity(),
			boneTransforms
		);
	}


	//	�X�e�b�v3: �{�[���s����G�t�F�N�g�ɐݒ�
	//	�ő�72�̃{�[���iDirectXTK �� SkinnedEffect �̐����j
	DirectX::XMMATRIX finalBones[DirectX::SkinnedEffect::MaxBones];

	for (int i = 0; i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		if (i < static_cast<int>(boneTransforms.size()))
		{
			finalBones[i] = boneTransforms[i];
		}
		else
		{
			finalBones[i] = DirectX::XMMatrixIdentity();
		}
	}

	//	�G�t�F�N�g�Ƀ{�[���s���ݒ�
	m_effect->SetBoneTransforms(finalBones, DirectX::SkinnedEffect::MaxBones);


	//	�G�t�F�N�g�ɍs���ݒ�
	m_effect->SetWorld(world);
	m_effect->SetView(view);
	m_effect->SetProjection(projection);

	//	���C�g�̐ݒ�
	m_effect->EnableDefaultLighting();
	m_effect->SetAmbientLightColor(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));
	m_effect->SetDiffuseColor(color);

	//	�G�t�F�N�g�K�p
	m_effect->Apply(context);

	//	���̓��C�A�E�g��ݒ�
	context->IASetInputLayout(m_inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	�e���b�V����`��
	for (const auto& mesh : m_meshes)
	{
		//	���_�o�b�t�@��ݒ�
		UINT stride = sizeof(ModelVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);

		//	�C���f�b�N�X�o�b�t�@��ݒ�
		context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//	�`�施��
		context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

void Model::DrawInstanced(
ID3D11DeviceContext* context,
const std::vector<InstanceData>& instances,
DirectX::XMMATRIX view,
DirectX::XMMATRIX projection,
const std::string& animationName,
float animationTime)
{
	//	===	�A�j���[�V���������݂��Ă��邩�m�F	===
	auto it = m_animations.find(animationName);
	if (it == m_animations.end())
	{/*
		OutputDebugStringA("Model::DrawInstanced - Animation not found: ");
		OutputDebugStringA(animationName.c_str());
		OutputDebugStringA("\n");*/
		return;
	}

	const AnimationClip& clip = it->second;


	//	===	�{�[���ϊ��s��G���v�Z	===
	std::vector<DirectX::XMMATRIX> boneTransforms;
	boneTransforms.resize(m_bones.size());	//	�{�[���̐������z����m��

	for (size_t i = 0; i < boneTransforms.size(); i++)
	{
		boneTransforms[i] = DirectX::XMMatrixIdentity();
	}

	if (!m_nodes.empty())
	{
		UpdateNodeTransforms(
			m_rootNodeIndex,
			clip,
			animationTime,
			DirectX::XMMatrixIdentity(),
			boneTransforms
		);
	}


	//	===	�{�[���s����G�t�F�N�g�ɐݒ�	===
	DirectX::XMMATRIX finalBones[DirectX::SkinnedEffect::MaxBones];

	for (int i = 0; i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		if (i < static_cast<int>(boneTransforms.size()))
		{
			finalBones[i] = boneTransforms[i];
		}
		else
		{
			finalBones[i] = DirectX::XMMatrixIdentity();
		}
	}

	m_effect->SetBoneTransforms(finalBones, DirectX::SkinnedEffect::MaxBones);	//	�{�[���s��𑗂�


	//	===	�r���[�E�v���W�F�N�V�����s���ݒ�	===
	m_effect->SetView(view);	//	�J�����̐ݒ�𑗂�
	m_effect->SetProjection(projection);	//	���e��ݒ�

	//	===	���C�g�̐ݒ�	===
	m_effect->EnableDefaultLighting();
	m_effect->SetAmbientLightColor(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));


	//	===	���̓��C�A�E�g�ݒ�	===
	context->IASetInputLayout(m_inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//	===	�e�C���X�^���X��`��	===
	for (const auto& instance : instances)
	{
		//	---	���[���h�s���ݒ�	---
		DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&instance.world);
		m_effect->SetWorld(world);

		//	---	�F��ݒ�	---
		DirectX::XMVECTOR color = DirectX::XMLoadFloat4(&instance.color);
		m_effect->SetDiffuseColor(color);

		//	---	�G�t�F�N�g�K�p	---
		m_effect->Apply(context);
		
		// �f�o�b�O���O//
		/*char debugDraw[256];
		sprintf_s(debugDraw, "[MODEL] DrawInstanced called, instances=%zu\n", instances.size());
		OutputDebugStringA(debugDraw);*/

		// �[�x�������݂������L����
		if (m_states)
		{
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			//OutputDebugStringA("[MODEL] Depth state set\n");
		}
		else
		{
			//OutputDebugStringA("[MODEL] ERROR: m_states is null!\n");
		}
		//	---	�e���b�V����`��	---
		for (const auto& mesh : m_meshes)
		{
			UINT stride = sizeof(ModelVertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
		}
	}

	////	�f�o�b�O�o��
	//char debug[256];
	//sprintf_s(debug, "DrawInstanced: %zu instances, anim=%s, time=%.2f\n",
	//	instances.size(), animationName.c_str(), animationTime);
	//OutputDebugStringA(debug);

}


//	===	�}�b�v�ɒl��o�^���邾���̏���	===
void Model::SetBoneScale(const std::string& boneName, float scale)
{
	//	�w�肳�ꂽ�{�[�������L�[�ɂ��āA�X�P�[���l��ۑ�
	m_boneScales[boneName] = scale;
}

void Model::SetTexture(ID3D11ShaderResourceView* texture)
{
	m_diffuseTexture = texture;

	// SkinnedEffect�Ƀe�N�X�`����ݒ�
	if (m_effect && texture)
	{
		m_effect->SetTexture(texture);
	}
}

// �{�[�������o��
void Model::PrintBoneNames() const
{
	OutputDebugStringA("=== Bone Names ===\n");
	for (size_t i = 0; i < m_bones.size(); i++)
	{
		char buffer[256];
		sprintf_s(buffer, "  [%zu] %s\n", i, m_bones[i].name.c_str());
		OutputDebugStringA(buffer);
	}
	OutputDebugStringA("==================\n");
}

void Model::DrawWithBoneScale(ID3D11DeviceContext* context,
	XMMATRIX world,
	XMMATRIX view,
	XMMATRIX projection,
	XMVECTOR color)
{
	// �{�[���s��������i���ׂ�Identity = ����Draw�Ɠ����j
	DirectX::XMMATRIX finalBones[DirectX::SkinnedEffect::MaxBones];
	for (int i = 0; i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		finalBones[i] = DirectX::XMMatrixIdentity();
	}

	// �X�P�[��0�̃{�[���������ʏ����i��\���ɂ���j
	for (size_t i = 0; i < m_bones.size() && i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		std::string shortName = GetShortName(m_bones[i].name);
		if (m_boneScales.find(shortName) != m_boneScales.end())
		{
			float s = m_boneScales[shortName];
			if (s == 0.0f)
			{
				// �X�P�[��0 �� ���_�������Ȃ�����
				finalBones[i] = DirectX::XMMatrixScaling(0.0f, 0.0f, 0.0f);
			}
		}
	}

	m_effect->SetBoneTransforms(finalBones, DirectX::SkinnedEffect::MaxBones);

	// === �ȉ��͌���Draw�Ɠ��� ===
	m_effect->SetWorld(world);
	m_effect->SetView(view);
	m_effect->SetProjection(projection);
	m_effect->SetDiffuseColor(color);
	m_effect->EnableDefaultLighting();
	m_effect->SetAmbientLightColor(XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));

	m_effect->Apply(context);

	if (m_states)
	{
		context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	}

	context->IASetInputLayout(m_inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& mesh : m_meshes)
	{
		UINT stride = sizeof(ModelVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

void Model::SetBoneScaleByPrefix(const std::string& prefix, float scale)
{
	for (const auto& bone : m_bones)
	{
		std::string shortName = GetShortName(bone.name);
		// �v���t�B�b�N�X�Ŏn�܂�{�[����S�Đݒ�
		if (shortName.rfind(prefix, 0) == 0)  // starts_with
		{
			m_boneScales[shortName] = scale;
		}
	}
}

// === �J�X�^���V�F�[�_�[�ǂݍ��� ===
bool Model::LoadCustomShaders(ID3D11Device* device)
{
	// ========================================
	// �y�����z�G���f���p�J�X�^���V�F�[�_�[�i�X�L�j���O�Ή��j��ǂݍ���
	// �y�ύX�zfopen_s �� D3DReadFileToBlob �ɓ���
	//         �p�X�� Assets/Shaders/ �ɓ���
	// ========================================

	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3DBlob> blob;

	// --- ���_�V�F�[�_�[ .cso ��ǂݍ��� ---
	hr = D3DReadFileToBlob(L"Assets/Shaders/SkinnedVS.cso", &blob);
	if (FAILED(hr))
	{
		OutputDebugStringA("[SHADER] SkinnedVS.cso ��������Ȃ��I\n");
		return false;
	}

	hr = device->CreateVertexShader(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		nullptr,
		m_customVS.GetAddressOf()
	);
	if (FAILED(hr))
	{
		OutputDebugStringA("[SHADER] CreateVertexShader ���s�I\n");
		return false;
	}

	// --- ���̓��C�A�E�g�i���_�f�[�^�̍\���� GPU �ɋ�����j---
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		// �X���b�g0: ���_�f�[�^�i���f���̊e���_�j
		{ "POSITION",        0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "NORMAL",          0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "TEXCOORD",        0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "BLENDINDICES",    0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 32, D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "BLENDWEIGHT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA,   0 },

		// �X���b�g1: �C���X�^���X�f�[�^�i�G1�̂��ƂɈႤ�j
		{ "INST_WORLD",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_WORLD",      1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_WORLD",      2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_WORLD",      3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_BONEOFFSET", 0, DXGI_FORMAT_R32_UINT,           1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	hr = device->CreateInputLayout(
		layout,
		ARRAYSIZE(layout),          // �v�f���i= 11�j
		blob->GetBufferPointer(),   // ���_�V�F�[�_�[�̃o�C�i���i���ؗp�j
		blob->GetBufferSize(),
		m_customInputLayout.GetAddressOf()
	);
	if (FAILED(hr))
	{
		OutputDebugStringA("[SHADER] CreateInputLayout ���s�I\n");
		return false;
	}

	// --- �s�N�Z���V�F�[�_�[ .cso ��ǂݍ��� ---
	blob.Reset();
	hr = D3DReadFileToBlob(L"Assets/Shaders/SkinnedPS.cso", &blob);
	if (FAILED(hr))
	{
		OutputDebugStringA("[SHADER] SkinnedPS.cso ��������Ȃ��I\n");
		return false;
	}

	hr = device->CreatePixelShader(
		blob->GetBufferPointer(), blob->GetBufferSize(),
		nullptr, m_customPS.GetAddressOf()
	);
	if (FAILED(hr))
	{
		OutputDebugStringA("[SHADER] CreatePixelShader ���s�I\n");
		return false;
	}

	// --- �萔�o�b�t�@��3�쐬 ---
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;

	// Transform �o�b�t�@�ib0�j
	cbDesc.ByteWidth = sizeof(TransformCB);
	hr = device->CreateBuffer(&cbDesc, nullptr, m_transformCB.GetAddressOf());
	if (FAILED(hr)) return false;

	// Bone �o�b�t�@�ib1�j
	cbDesc.ByteWidth = sizeof(BoneCB);
	hr = device->CreateBuffer(&cbDesc, nullptr, m_boneCB.GetAddressOf());
	if (FAILED(hr)) return false;

	// Light �o�b�t�@�ib2�j
	cbDesc.ByteWidth = sizeof(LightCB);
	hr = device->CreateBuffer(&cbDesc, nullptr, m_lightCB.GetAddressOf());
	if (FAILED(hr)) return false;

	// --- �T���v���[�X�e�[�g ---
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hr = device->CreateSamplerState(&sampDesc, m_customSampler.GetAddressOf());
	if (FAILED(hr)) return false;

	m_useCustomShader = true;
	OutputDebugStringA("[SHADER] Custom shaders loaded from CSO!\n");
	return true;
}

void Model::DrawAnimated(
	ID3D11DeviceContext* context,
	DirectX::XMMATRIX world,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection,
	DirectX::XMVECTOR color,
	const std::string& animationName,
	float animationTime)
{
	// �J�X�^���V�F�[�_�[�{�C���X�^���X�o�b�t�@������΁A1�̂����̃C���X�^���V���O�ŕ`��
	if (m_useCustomShader && m_instanceBuffer)
	{
		std::vector<DirectX::XMMATRIX> worlds = { world };
		std::vector<DirectX::XMVECTOR> colors = { color };
		std::vector<std::string>       anims = { animationName };
		std::vector<float>             times = { animationTime };

		DrawInstanced_Custom(context, view, projection, worlds, colors, anims, times);
		return;
	}

	// �t�H�[���o�b�N�F���`��
	DrawAnimated_Legacy(context, world, view, projection, color, animationName, animationTime);
}

// === �C���X�^���V���O�p�o�b�t�@�쐬 ===
bool Model::CreateInstanceBuffers(ID3D11Device* device, int maxInstances)
{
	m_maxInstances = maxInstances;
	m_maxBoneEntries = maxInstances * 72;  // �e�C���X�^���X72�{�[��

	// --- �C���X�^���X�o�b�t�@�i���_�o�b�t�@�Ƃ��Ďg���j---
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = sizeof(InstanceGPU) * maxInstances;
	ibDesc.Usage = D3D11_USAGE_DYNAMIC;          // ���t���[���X�V���邽��
	ibDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = device->CreateBuffer(&ibDesc, nullptr, m_instanceBuffer.GetAddressOf());
	if (FAILED(hr)) { OutputDebugStringA("[INST] instanceBuffer �쐬���s\n"); return false; }

	// --- StructuredBuffer�i�{�[���s��̋���z��j---
	D3D11_BUFFER_DESC sbDesc = {};
	sbDesc.ByteWidth = sizeof(DirectX::XMMATRIX) * m_maxBoneEntries;
	sbDesc.Usage = D3D11_USAGE_DYNAMIC;
	sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;  // StructuredBuffer!
	sbDesc.StructureByteStride = sizeof(DirectX::XMMATRIX);   // 1�v�f = 64bytes
	hr = device->CreateBuffer(&sbDesc, nullptr, m_boneStructuredBuf.GetAddressOf());
	if (FAILED(hr)) { OutputDebugStringA("[INST] boneStructuredBuf �쐬���s\n"); return false; }

	// --- SRV�i�V�F�[�_�[����StructuredBuffer��ǂނ��߂̃r���[�j---
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;             // Structured = UNKNOWN
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_maxBoneEntries;
	hr = device->CreateShaderResourceView(m_boneStructuredBuf.Get(), &srvDesc, m_boneSRV.GetAddressOf());
	if (FAILED(hr)) { OutputDebugStringA("[INST] boneSRV �쐬���s\n"); return false; }

	// --- Frame�萔�o�b�t�@�iView/Projection�j---
	D3D11_BUFFER_DESC fcbDesc = {};
	fcbDesc.ByteWidth = sizeof(FrameCB);
	fcbDesc.Usage = D3D11_USAGE_DEFAULT;
	fcbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = device->CreateBuffer(&fcbDesc, nullptr, m_frameCB.GetAddressOf());
	if (FAILED(hr)) return false;

	char buf[128];
	sprintf_s(buf, "[INST] �o�b�t�@�쐬����: max %d instances, %d bones\n", maxInstances, m_maxBoneEntries);
	OutputDebugStringA(buf);
	return true;
}

// === �C���X�^���V���O�`�� ===
void Model::DrawInstanced_Custom(
	ID3D11DeviceContext* context,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection,
	const std::vector<DirectX::XMMATRIX>& worlds,
	const std::vector<DirectX::XMVECTOR>& colors,
	const std::vector<std::string>& animNames,
	const std::vector<float>& animTimes)
{
	int count = (int)worlds.size();
	if (count == 0 || !m_useCustomShader || !m_instanceBuffer) return;
	if (count > m_maxInstances) count = m_maxInstances;

	// ---  �S�C���X�^���X�̃{�[���s����v�Z����StructuredBuffer�ɏ������� ---
	D3D11_MAPPED_SUBRESOURCE mappedBones;
	context->Map(m_boneStructuredBuf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBones);
	DirectX::XMMATRIX* bonePtr = (DirectX::XMMATRIX*)mappedBones.pData;

	// �{�[���v�Z�p�i�����o�ϐ��̃L���b�V�����ė��p�j
	auto& tempBones = m_cachedBoneTransforms;

	for (int inst = 0; inst < count; inst++)
	{
		// ���̃C���X�^���X�̃A�j���[�V��������{�[���s����v�Z
		auto it = m_animations.find(animNames[inst]);

		if (it != m_animations.end() && !m_nodes.empty())
		{
			const AnimationClip& clip = it->second;
			tempBones.resize(m_bones.size());
			for (size_t b = 0; b < tempBones.size(); b++)
				tempBones[b] = DirectX::XMMatrixIdentity();

			UpdateNodeTransforms(m_rootNodeIndex, clip, animTimes[inst],
				DirectX::XMMatrixIdentity(), tempBones);

			// StructuredBuffer�ɓ]�u���ď�������
			for (int b = 0; b < 72; b++)
			{
				if (b < (int)tempBones.size())
					bonePtr[inst * 72 + b] = DirectX::XMMatrixTranspose(tempBones[b]);
				else
					bonePtr[inst * 72 + b] = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
			}
		}
		else
		{
			// �A�j���[�V�����Ȃ� �� �P�ʍs��Ŗ��߂�
			for (int b = 0; b < 72; b++)
				bonePtr[inst * 72 + b] = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
		}
	}
	context->Unmap(m_boneStructuredBuf.Get(), 0);

	// --- �C���X�^���X�o�b�t�@�ɏ������� ---
	D3D11_MAPPED_SUBRESOURCE mappedInst;
	context->Map(m_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedInst);
	InstanceGPU* instPtr = (InstanceGPU*)mappedInst.pData;

	for (int i = 0; i < count; i++)
	{
		instPtr[i].World = DirectX::XMMatrixTranspose(worlds[i]);
		DirectX::XMStoreFloat4(&instPtr[i].Color, colors[i]);
		instPtr[i].BoneOffset = i * 72;  // ���̃C���X�^���X�̃{�[���J�n�ʒu
		instPtr[i].pad[0] = instPtr[i].pad[1] = instPtr[i].pad[2] = 0;
	}
	context->Unmap(m_instanceBuffer.Get(), 0);

	// --- �萔�o�b�t�@�X�V ---
	FrameCB frameData;
	frameData.View = DirectX::XMMatrixTranspose(view);
	frameData.Projection = DirectX::XMMatrixTranspose(projection);
	context->UpdateSubresource(m_frameCB.Get(), 0, nullptr, &frameData, 0, 0);

	// --- Step 4: �p�C�v���C���Z�b�g ---
	context->VSSetShader(m_customVS.Get(), nullptr, 0);
	context->PSSetShader(m_customPS.Get(), nullptr, 0);
	context->IASetInputLayout(m_customInputLayout.Get());

	// �萔�o�b�t�@
	context->VSSetConstantBuffers(0, 1, m_frameCB.GetAddressOf());
	context->VSSetConstantBuffers(2, 1, m_lightCB.GetAddressOf());
	context->PSSetConstantBuffers(2, 1, m_lightCB.GetAddressOf());  // PS�ɂ�����LightCB���Z�b�g

	// StructuredBuffer �� t1 �ɃZ�b�g
	context->VSSetShaderResources(1, 1, m_boneSRV.GetAddressOf());

	// �e�N�X�`�� �� t0
	if (m_diffuseTexture)
		context->PSSetShaderResources(0, 1, m_diffuseTexture.GetAddressOf());
	context->PSSetSamplers(0, 1, m_customSampler.GetAddressOf());

	//  View�s�񂩂�J�����̃��[���h�ʒu�����o��
    // View�s��̋t�s�� = �J�����̃��[���h�ϊ��s��
    // ����4�s��(r[3]) = �J�����̈ʒu
	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, view);
	DirectX::XMFLOAT3 camPos;
	camPos.x = DirectX::XMVectorGetX(invView.r[3]);
	camPos.y = DirectX::XMVectorGetY(invView.r[3]);
	camPos.z = DirectX::XMVectorGetZ(invView.r[3]);

	LightCB lightData;
	lightData.AmbientColor = DirectX::XMFLOAT4(0.55f, 0.50f, 0.45f, 1.0f);
	lightData.DiffuseColor = DirectX::XMFLOAT4(1, 1, 1, 1);
	lightData.LightDirection = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f);
	lightData.Padding = 0.0f;
	lightData.CameraPos = camPos;
	lightData.Padding2 = 0.0f;
	context->UpdateSubresource(m_lightCB.Get(), 0, nullptr, &lightData, 0, 0);

	// ---  �`�� ---
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& mesh : m_meshes)
	{
		// �X���b�g0: ���f���̒��_�f�[�^�A�X���b�g1: �C���X�^���X�f�[�^
		ID3D11Buffer* buffers[] = { mesh.vertexBuffer.Get(), m_instanceBuffer.Get() };
		UINT strides[] = { sizeof(ModelVertex), sizeof(InstanceGPU) };
		UINT offsets[] = { 0, 0 };
		context->IASetVertexBuffers(0, 2, buffers, strides, offsets);
		context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//  DrawIndexedInstanced: 1��̃R�[���őS�C���X�^���X��`��I
		context->DrawIndexedInstanced((UINT)mesh.indices.size(), count, 0, 0, 0);
	}

	// --- �N���[���A�b�v: SRV��null�ɖ߂� ---
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->VSSetShaderResources(1, 1, &nullSRV);
}

// ============================================
//  �o�C���h�|�[�Y�̒��_���W���擾
//  �{�[���I�t�Z�b�g�s��̋t�s���K�p����
//  ���f����Ԃ̍��W�ɕ�������
// ============================================
std::vector<ModelVertex> Model::GetBindPoseVertices(int meshIndex)
{
	if (meshIndex < 0 || meshIndex >= (int)m_meshes.size())
		return {};

	const Mesh& mesh = m_meshes[meshIndex];
	std::vector<ModelVertex> result = mesh.vertices;

	if (m_bones.empty() || m_animations.empty()) return result;

	// Idle �̍ŏ��̃t���[���Ń{�[���ϊ����v�Z
	std::string animName = "Idle";
	if (m_animations.find(animName) == m_animations.end())
		animName = m_animations.begin()->first;

	std::vector<DirectX::XMMATRIX> boneTransforms;
	CalculateBoneTransforms(animName, 0.0f, boneTransforms);
	if (boneTransforms.empty()) return result;

	for (auto& vertex : result)
	{
		float totalWeight = 0.0f;
		for (int s = 0; s < 4; s++)
			totalWeight += vertex.boneWeights[s];
		if (totalWeight < 0.001f) continue;

		DirectX::XMVECTOR pos = DirectX::XMVectorSet(
			vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);
		DirectX::XMVECTOR norm = DirectX::XMVectorSet(
			vertex.normal.x, vertex.normal.y, vertex.normal.z, 0.0f);

		DirectX::XMVECTOR newPos = DirectX::XMVectorZero();
		DirectX::XMVECTOR newNorm = DirectX::XMVectorZero();

		// �S�{�[�����E�F�C�g�Ńu�����h�iGPU�̕`��Ɠ����v�Z�j
		for (int s = 0; s < 4; s++)
		{
			float w = vertex.boneWeights[s];
			if (w <= 0.0f) continue;

			uint32_t boneIdx = vertex.boneIndices[s];
			if (boneIdx >= boneTransforms.size()) continue;

			DirectX::XMVECTOR tPos =
				DirectX::XMVector3Transform(pos, boneTransforms[boneIdx]);
			DirectX::XMVECTOR tNorm =
				DirectX::XMVector3TransformNormal(norm, boneTransforms[boneIdx]);

			newPos = DirectX::XMVectorAdd(newPos,
				DirectX::XMVectorScale(tPos, w));
			newNorm = DirectX::XMVectorAdd(newNorm,
				DirectX::XMVectorScale(tNorm, w));
		}

		DirectX::XMStoreFloat3(&vertex.position, newPos);
		newNorm = DirectX::XMVector3Normalize(newNorm);
		DirectX::XMStoreFloat3(&vertex.normal, newNorm);
	}

	// �f�o�b�O
	float minX = 99999, maxX = -99999;
	float minY = 99999, maxY = -99999;
	float minZ = 99999, maxZ = -99999;
	for (const auto& v : result)
	{
		if (v.position.x < minX) minX = v.position.x;
		if (v.position.x > maxX) maxX = v.position.x;
		if (v.position.y < minY) minY = v.position.y;
		if (v.position.y > maxY) maxY = v.position.y;
		if (v.position.z < minZ) minZ = v.position.z;
		if (v.position.z > maxZ) maxZ = v.position.z;
	}
	char buf[512];
	sprintf_s(buf,
		"[GetBindPoseVertices] mesh=%d (full blend):\n"
		"  X: %.2f ~ %.2f (width=%.2f)\n"
		"  Y: %.2f ~ %.2f (height=%.2f)\n"
		"  Z: %.2f ~ %.2f (depth=%.2f)\n",
		meshIndex,
		minX, maxX, maxX - minX,
		minY, maxY, maxY - minY,
		minZ, maxZ, maxZ - minZ);
	OutputDebugStringA(buf);

	return result;
}

// ============================================
//  �A�j���[�V�����|�[�Y�K�p�ς݂̒��_���擾
//  �ؒf���Ɂu���̏u�Ԃ̃|�[�Y�v�Ő؂邽�߂Ɏg��
// ============================================
std::vector<ModelVertex> Model::GetAnimatedVertices(
	int meshIndex,
	const std::string& animName,
	float animTime)
{
	if (meshIndex < 0 || meshIndex >= (int)m_meshes.size())
		return {};

	const Mesh& mesh = m_meshes[meshIndex];
	std::vector<ModelVertex> result = mesh.vertices;

	if (m_bones.empty() || m_animations.empty())
		return result;

	// �w��A�j���[�V������T��
	std::string anim = animName;
	if (m_animations.find(anim) == m_animations.end())
	{
		if (!m_animations.empty())
			anim = m_animations.begin()->first;
		else
			return result;
	}

	const AnimationClip& clip = m_animations[anim];

	// --- GPU�`��Ɠ����p�X�Ń{�[���s����v�Z ---
	// UpdateNodeTransforms ���g���iglobalInverseTransform ���݁j
	std::vector<DirectX::XMMATRIX> boneTransforms(m_bones.size(),
		DirectX::XMMatrixIdentity());

	UpdateNodeTransforms(m_rootNodeIndex, clip, animTime,
		DirectX::XMMatrixIdentity(), boneTransforms);

	if (boneTransforms.empty()) return result;

	// �e���_�Ƀ{�[���ϊ���K�p�iGPU�̃X�L�j���O�Ɠ����v�Z�j
	for (auto& vertex : result)
	{
		float totalWeight = 0.0f;
		for (int s = 0; s < 4; s++)
			totalWeight += vertex.boneWeights[s];
		if (totalWeight < 0.001f) continue;

		DirectX::XMVECTOR pos = DirectX::XMVectorSet(
			vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);
		DirectX::XMVECTOR norm = DirectX::XMVectorSet(
			vertex.normal.x, vertex.normal.y, vertex.normal.z, 0.0f);

		DirectX::XMVECTOR newPos = DirectX::XMVectorZero();
		DirectX::XMVECTOR newNorm = DirectX::XMVectorZero();

		for (int s = 0; s < 4; s++)
		{
			float w = vertex.boneWeights[s];
			if (w <= 0.0f) continue;

			uint32_t boneIdx = vertex.boneIndices[s];
			if (boneIdx >= boneTransforms.size()) continue;

			DirectX::XMVECTOR tPos =
				DirectX::XMVector3Transform(pos, boneTransforms[boneIdx]);
			DirectX::XMVECTOR tNorm =
				DirectX::XMVector3TransformNormal(norm, boneTransforms[boneIdx]);

			newPos = DirectX::XMVectorAdd(newPos,
				DirectX::XMVectorScale(tPos, w));
			newNorm = DirectX::XMVectorAdd(newNorm,
				DirectX::XMVectorScale(tNorm, w));
		}

		DirectX::XMStoreFloat3(&vertex.position, newPos);
		newNorm = DirectX::XMVector3Normalize(newNorm);
		DirectX::XMStoreFloat3(&vertex.normal, newNorm);
	}

	return result;
}