// MapSystem.cpp
// �v���~�e�B�u�i�����蔻��j+ FBX���f���iAssimp�`��j
#define NOMINMAX
#include "MapSystem.h"

// Assimp�iFBX�ǂݍ��ݗp�j
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ============================================
//  �R���X�g���N�^
// ============================================
MapSystem::MapSystem() {}

// ============================================
//  ������
// ============================================
bool MapSystem::Initialize(ID3D11DeviceContext* context, ID3D11Device* device)
{
	m_device = device;

	// �����v���~�e�B�u
	m_box = GeometricPrimitive::CreateBox(context, XMFLOAT3(1, 1, 1), true, false);
	m_cylinder = GeometricPrimitive::CreateCylinder(context, 1.0f, 1.0f, 32, true);
	m_states = std::make_unique<DirectX::CommonStates>(device);

	// BasicEffect�iFBX�}�b�v�`��p�j
	m_mapEffect = std::make_unique<BasicEffect>(device);
	m_mapEffect->SetTextureEnabled(true);		// �e�N�X�`���L��
	m_mapEffect->SetLightingEnabled(true);		// ���C�e�B���O�L��
	//  �Â����͋C�̃��C�e�B���O
	m_mapEffect->SetLightEnabled(0, true);
	m_mapEffect->SetLightEnabled(1, true);
	m_mapEffect->SetLightEnabled(2, false);   // 3�ڂ̃��C�gOFF

	// ���C�����C�g�F�ォ��̔��Â��g�F
	m_mapEffect->SetLightDirection(0, XMVectorSet(0.0f, -1.0f, 0.3f, 0.0f));
	m_mapEffect->SetLightDiffuseColor(0, XMVectorSet(0.6f, 0.45f, 0.3f, 1.0f));

	// �⏕���C�g�F������̔����i�e�ɐF��t����j
	m_mapEffect->SetLightDirection(1, XMVectorSet(-0.5f, -0.3f, -0.7f, 0.0f));
	m_mapEffect->SetLightDiffuseColor(1, XMVectorSet(0.15f, 0.15f, 0.25f, 1.0f));

	// �������Â�
	m_mapEffect->SetAmbientLightColor(XMVectorSet(0.08f, 0.06f, 0.05f, 1.0f));

	// ���̓��C�A�E�g�쐬
	void const* shaderByteCode;
	size_t byteCodeLength;
	m_mapEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	HRESULT hr = device->CreateInputLayout(
		VertexPositionNormalTexture::InputElements,
		VertexPositionNormalTexture::InputElementCount,
		shaderByteCode, byteCodeLength,
		m_mapInputLayout.ReleaseAndGetAddressOf());

	if (FAILED(hr))
	{
		OutputDebugStringA("MapSystem: ���̓��C�A�E�g�쐬���s\n");
		return false;
	}

	OutputDebugStringA("MapSystem::Initialize - Success\n");

	hr = CreateWICTextureFromFile(
		device, L"Assets/Models/Map/textures/MALL_TIL23.jpg",   // �� ���e�N�X�`���̃p�X
		nullptr, m_floorTexture.ReleaseAndGetAddressOf());
	if (SUCCEEDED(hr))
		OutputDebugStringA("[MapSystem] Floor tile texture loaded!\n");
	else
		OutputDebugStringA("[MapSystem] WARNING: floor tile texture not found\n");

	hr = CreateWICTextureFromFile(
		device, L"Assets/Models/Map/textures/OffsetCobblestoneDC19.jpg",    // �� �ǃe�N�X�`���̃p�X
		nullptr, m_wallTexture.ReleaseAndGetAddressOf());
	if (SUCCEEDED(hr))
		OutputDebugStringA("[MapSystem] Wall tile texture loaded!\n");
	else
		OutputDebugStringA("[MapSystem] WARNING: wall tile texture not found\n");


	m_box->CreateInputLayout(m_mapEffect.get(),
		m_boxInputLayout.ReleaseAndGetAddressOf());

	if (m_boxInputLayout)
		OutputDebugStringA("[MapSystem] Box InputLayout created from m_mapEffect!\n");
	else
		OutputDebugStringA("[MapSystem] ERROR: Box InputLayout creation failed!\n");


	return true;
}

void MapSystem::ApplyLightSettings()
{
	if (!m_mapEffect) return;

	m_mapEffect->SetLightDirection(0,
		XMVectorSet(m_lightDir0.x, m_lightDir0.y, m_lightDir0.z, 0.0f));
	m_mapEffect->SetLightDiffuseColor(0,
		XMLoadFloat4(&m_lightColor0));

	m_mapEffect->SetLightDirection(1,
		XMVectorSet(m_lightDir1.x, m_lightDir1.y, m_lightDir1.z, 0.0f));
	m_mapEffect->SetLightDiffuseColor(1,
		XMLoadFloat4(&m_lightColor1));

	m_mapEffect->SetAmbientLightColor(XMLoadFloat4(&m_ambientColor));
}

// ============================================
//  FBX�}�b�v�ǂݍ��݁iAssimp�o�R�j
// ============================================
bool MapSystem::LoadMapFBX(const std::string& fbxPath,
	const std::wstring& textureDir, float scale)
{
	if (!m_device) return false;

	m_mapScale = scale;

	// --- Assimp�Ńt�@�C���ǂݍ��� ---
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fbxPath,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |		// DirectX�͍�����W�n
		aiProcess_GenNormals |
		aiProcess_GenUVCoords |
		aiProcess_CalcTangentSpace);

	if (!scene || !scene->mRootNode)
	{
		char buf[512];
		sprintf_s(buf, "MapSystem: FBX�ǂݍ��ݎ��s: %s\n", importer.GetErrorString());
		OutputDebugStringA(buf);
		return false;
	}

	char debugBuf[256];
	sprintf_s(debugBuf, "MapSystem: FBX�ǂݍ��݊��� - ���b�V��:%d �}�e���A��:%d\n",
		scene->mNumMeshes, scene->mNumMaterials);
	OutputDebugStringA(debugBuf);

	// === �}�e���A���ǂݍ��� ===
	m_materials.resize(scene->mNumMaterials);
	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* mat = scene->mMaterials[i];

		// �}�e���A�����擾
		aiString matName;
		mat->Get(AI_MATKEY_NAME, matName);
		m_materials[i].name = matName.C_Str();

		// �e�N�X�`���T���iDIFFUSE��BASE_COLOR�̏��j
		bool texLoaded = false;
		aiTextureType tryTypes[] = {
			aiTextureType_DIFFUSE,
			aiTextureType_BASE_COLOR
		};

		for (auto texType : tryTypes)
		{
			if (texLoaded) break;
			if (mat->GetTextureCount(texType) > 0)
			{
				aiString texPath;
				mat->GetTexture(texType, 0, &texPath);

				// �t�@�C�����������o�i�p�X����j
				std::string texStr = texPath.C_Str();
				size_t lastSlash = texStr.find_last_of("/\\");
				std::string texFileName = (lastSlash != std::string::npos)
					? texStr.substr(lastSlash + 1) : texStr;

				// wstring�ɕϊ����ăt���p�X�쐬
				std::wstring wTexFile(texFileName.begin(), texFileName.end());
				std::wstring fullPath = textureDir + L"/" + wTexFile;

				// �e�N�X�`���ǂݍ���
				HRESULT hr = CreateWICTextureFromFile(
					m_device, fullPath.c_str(), nullptr,
					m_materials[i].diffuseTexture.ReleaseAndGetAddressOf());

				if (SUCCEEDED(hr))
				{
					texLoaded = true;
					char db[512];
					sprintf_s(db, "  Material[%d] '%s': �e�N�X�`��OK\n", i, texFileName.c_str());
					OutputDebugStringA(db);
				}
			}
		}

		// �t�H�[���o�b�N�F�}�e���A��������e�N�X�`���𐄑�
		if (!texLoaded)
		{
			std::string mname = m_materials[i].name;
			std::string tryNames[] = {
				mname + "color.png",
				mname + "Color.png",
				mname + ".png",
				mname + "_diffuse.png"
			};

			for (const auto& tryName : tryNames)
			{
				if (texLoaded) break;
				std::wstring wTry(tryName.begin(), tryName.end());
				std::wstring fullPath = textureDir + L"/" + wTry;

				HRESULT hr = CreateWICTextureFromFile(
					m_device, fullPath.c_str(), nullptr,
					m_materials[i].diffuseTexture.ReleaseAndGetAddressOf());

				if (SUCCEEDED(hr))
				{
					texLoaded = true;
					char db[512];
					sprintf_s(db, "  Material[%d] �t�H�[���o�b�N '%s': OK\n", i, tryName.c_str());
					OutputDebugStringA(db);
				}
			}
		}

		// �e�N�X�`�����Ȃ��ꍇ�̓f�B�t���[�Y�F���g��
		if (!texLoaded)
		{
			aiColor4D color(0.5f, 0.5f, 0.5f, 1.0f);
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			m_materials[i].diffuseColor = { color.r, color.g, color.b, color.a };

			// �}�b�v�e�N�X�`�����蓮�œǂݍ���
			std::wstring fallbackFiles[] = {
				textureDir + L"/Map1_1.png",
				textureDir + L"/Map1_2.png",
				textureDir + L"/Map1_3.png",
				textureDir + L"/Map1_4.png"
			};
			for (const auto& fb : fallbackFiles)
			{
				HRESULT hr2 = CreateWICTextureFromFile(
					m_device, fb.c_str(), nullptr,
					m_materials[i].diffuseTexture.ReleaseAndGetAddressOf());
				if (SUCCEEDED(hr2))
				{
					texLoaded = true;
					OutputDebugStringA("  [MapSystem] Fallback texture loaded!\n");
					break;  // �ŏ��Ɍ����������̂��g��
				}
			}

			char db[256];
			sprintf_s(db, "  Material[%d] '%s': �e�N�X�`���Ȃ� �F=(%.2f,%.2f,%.2f)\n",
				i, m_materials[i].name.c_str(), color.r, color.g, color.b);
			OutputDebugStringA(db);
		}
	}

	// === ���b�V���ǂݍ��� ===
	m_subMeshes.resize(scene->mNumMeshes);
	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
	{
		aiMesh* mesh = scene->mMeshes[m];

		// ���_�f�[�^�쐬
		std::vector<VertexPositionNormalTexture> vertices(mesh->mNumVertices);
		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{
			vertices[v].position = XMFLOAT3(
				mesh->mVertices[v].x,
				mesh->mVertices[v].y,
				mesh->mVertices[v].z);

			if (mesh->mNormals)
			{
				vertices[v].normal = XMFLOAT3(
					mesh->mNormals[v].x,
					mesh->mNormals[v].y,
					mesh->mNormals[v].z);
			}
			else
			{
				vertices[v].normal = XMFLOAT3(0, 1, 0);
			}

			if (mesh->mTextureCoords[0])
			{
				vertices[v].textureCoordinate = XMFLOAT2(
					mesh->mTextureCoords[0][v].x,
					mesh->mTextureCoords[0][v].y);
			}
			else
			{
				vertices[v].textureCoordinate = XMFLOAT2(0, 0);
			}
		}

		// �C���f�b�N�X�f�[�^�쐬
		std::vector<uint32_t> indices;
		indices.reserve(mesh->mNumFaces * 3);
		for (unsigned int f = 0; f < mesh->mNumFaces; f++)
		{
			aiFace& face = mesh->mFaces[f];
			for (unsigned int idx = 0; idx < face.mNumIndices; idx++)
			{
				indices.push_back(face.mIndices[idx]);
			}
		}

		// ���_�o�b�t�@�쐬
		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = (UINT)(vertices.size() * sizeof(VertexPositionNormalTexture));
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		D3D11_SUBRESOURCE_DATA vbData = {};
		vbData.pSysMem = vertices.data();

		m_device->CreateBuffer(&vbDesc, &vbData,
			m_subMeshes[m].vertexBuffer.ReleaseAndGetAddressOf());

		// �C���f�b�N�X�o�b�t�@�쐬
		D3D11_BUFFER_DESC ibDesc = {};
		ibDesc.ByteWidth = (UINT)(indices.size() * sizeof(uint32_t));
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibDesc.Usage = D3D11_USAGE_DEFAULT;
		D3D11_SUBRESOURCE_DATA ibData = {};
		ibData.pSysMem = indices.data();

		m_device->CreateBuffer(&ibDesc, &ibData,
			m_subMeshes[m].indexBuffer.ReleaseAndGetAddressOf());

		m_subMeshes[m].vertexCount = (UINT)vertices.size();
		m_subMeshes[m].indexCount = (UINT)indices.size();
		m_subMeshes[m].materialIndex = mesh->mMaterialIndex;
		m_subMeshes[m].meshName = mesh->mName.C_Str();

		sprintf_s(debugBuf, "  Mesh[%d] '%s': %d���_, %d�O�p�`, mat=%d\n",
			m, mesh->mName.C_Str(), mesh->mNumVertices,
			mesh->mNumFaces, mesh->mMaterialIndex);
		OutputDebugStringA(debugBuf);
	}

	// ===  �S���b�V����nodeTransform��P�ʍs��ŏ����� ===
   // �i�m�[�h�ɎQ�Ƃ���Ȃ����b�V�����S�~�l�ɂȂ�Ȃ��悤�Ɂj
	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
	{
		XMStoreFloat4x4(&m_subMeshes[m].nodeTransform, XMMatrixIdentity());
	}

	// ===  �m�[�h�K�w���烁�b�V�����Ƃ̕ϊ��s����擾 ===
	std::function<void(aiNode*, XMMATRIX)> processNode;
	processNode = [&](aiNode* node, XMMATRIX parentTransform)
		{
			// Assimp�̍s����擾
			aiMatrix4x4 m = node->mTransformation;

			//  Assimp������Transpose()�ŕϊ�
			// Assimp = ��x�N�g�������itranslation = a4,b4,c4�j
			// DirectX = �s�x�N�g�������itranslation = _41,_42,_43�j
			// Transpose()�ŗ񁨍s�ɕϊ�
			m.Transpose();

			XMMATRIX localTransform(
				m.a1, m.a2, m.a3, m.a4,
				m.b1, m.b2, m.b3, m.b4,
				m.c1, m.c2, m.c3, m.c4,
				m.d1, m.d2, m.d3, m.d4
			);

			// �e�̕ϊ��ƍ����iDirectX�̍s�x�N�g������: local * parent�j
			XMMATRIX worldTransform = localTransform * parentTransform;

			// �f�o�b�O�F�m�[�h���ƕϊ��s��̈ꕔ���o��
			if (node->mNumMeshes > 0)
			{
				// translation�����i_41,_42,_43�j��\��
				XMFLOAT4X4 dbgMat;
				XMStoreFloat4x4(&dbgMat, worldTransform);
				char db[256];
				sprintf_s(db, "  Node '%s': meshes=%d, pos=(%.2f, %.2f, %.2f)\n",
					node->mName.C_Str(), node->mNumMeshes,
					dbgMat._41, dbgMat._42, dbgMat._43);
				OutputDebugStringA(db);
			}

			// ���̃m�[�h�������b�V���ɕϊ��s���ݒ�
			for (unsigned int i = 0; i < node->mNumMeshes; i++)
			{
				unsigned int meshIndex = node->mMeshes[i];
				if (meshIndex < m_subMeshes.size())
				{
					XMStoreFloat4x4(&m_subMeshes[meshIndex].nodeTransform, worldTransform);
				}
			}

			// �q�m�[�h���ċA����
			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				processNode(node->mChildren[i], worldTransform);
			}
		};

	//  ���[�g�m�[�h�̍s����f�o�b�O�o��
	{
		aiMatrix4x4 rootM = scene->mRootNode->mTransformation;
		char db[512];
		sprintf_s(db, "ROOT NODE '%s' raw matrix:\n"
			"  [%.4f %.4f %.4f %.4f]\n"
			"  [%.4f %.4f %.4f %.4f]\n"
			"  [%.4f %.4f %.4f %.4f]\n"
			"  [%.4f %.4f %.4f %.4f]\n",
			scene->mRootNode->mName.C_Str(),
			rootM.a1, rootM.a2, rootM.a3, rootM.a4,
			rootM.b1, rootM.b2, rootM.b3, rootM.b4,
			rootM.c1, rootM.c2, rootM.c3, rootM.c4,
			rootM.d1, rootM.d2, rootM.d3, rootM.d4);
		OutputDebugStringA(db);
	}

	processNode(scene->mRootNode, XMMatrixIdentity());

	// === ���b�V���R���C�_�[�p�F�S�O�p�`��CPU���ɕۑ� ===
	m_collisionTriangles.clear();

	// �}�b�v�S�̂̃��[���h�ϊ��i�ʒu�E��]�E�X�P�[���j
	XMMATRIX mapWorld =
		XMMatrixScaling(m_mapScale, m_mapScale, m_mapScale) *
		XMMatrixRotationY(m_mapRotationY) *
		XMMatrixTranslation(m_mapPosition.x, m_mapPosition.y, m_mapPosition.z);

	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
	{
		aiMesh* mesh = scene->mMeshes[m];

		// ���̃T�u���b�V���̃m�[�h�ϊ����擾
		XMMATRIX nodeT = XMLoadFloat4x4(&m_subMeshes[m].nodeTransform);

		// �ŏI�ϊ� = �m�[�h�ϊ� �~ �}�b�v���[���h�ϊ�
		XMMATRIX finalT = nodeT * mapWorld;

		for (unsigned int f = 0; f < mesh->mNumFaces; f++)
		{
			aiFace& face = mesh->mFaces[f];
			if (face.mNumIndices != 3) continue; // �O�p�`�ȊO�̓X�L�b�v

			CollisionTriangle tri;

			// �e���_�����[���h���W�ɕϊ�
			for (int i = 0; i < 3; i++)
			{
				unsigned int idx = face.mIndices[i];
				XMVECTOR pos = XMVectorSet(
					mesh->mVertices[idx].x,
					mesh->mVertices[idx].y,
					mesh->mVertices[idx].z,
					1.0f);

				// �m�[�h�ϊ� �~ �}�b�v�ϊ� ��K�p
				pos = XMVector3Transform(pos, finalT);

				XMFLOAT3* dest = (i == 0) ? &tri.v0 : (i == 1) ? &tri.v1 : &tri.v2;
				XMStoreFloat3(dest, pos);
			}

			m_collisionTriangles.push_back(tri);
		}
	}

	{
		char db[256];
		sprintf_s(db, "[MapSystem] Collision triangles collected: %zu\n",
			m_collisionTriangles.size());
		OutputDebugStringA(db);
	}

	m_mapLoaded = true;
	OutputDebugStringA("MapSystem: FBX�}�b�v�ǂݍ��ݐ����I\n");
	return true;
}

// ============================================
//  FBX���f���`��
// ============================================
void MapSystem::DrawMapModel(ID3D11DeviceContext* context,
	XMMATRIX view, XMMATRIX projection)
{

	//  ImGui�̒l�𔽉f
	ApplyLightSettings();

	if (!m_mapLoaded || !m_mapEffect) return;

	// �`���Ԃ����S���Z�b�g
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->RSSetState(m_states->CullNone());

	//  �S�̂̃g�����X�t�H�[��
	XMMATRIX globalTransform = XMMatrixScaling(m_mapScale, m_mapScale, m_mapScale)
		* XMMatrixRotationY(m_mapRotationY)
		* XMMatrixTranslation(m_mapPosition.x, m_mapPosition.y, m_mapPosition.z);

	m_mapEffect->SetView(view);
	m_mapEffect->SetProjection(projection);

	context->IASetInputLayout(m_mapInputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// �e�T�u���b�V����`��
	for (const auto& sub : m_subMeshes)
	{
		if (!sub.vertexBuffer || !sub.indexBuffer || sub.indexCount == 0)
			continue;

		//  �R���W�����p���b�V���͕`��X�L�b�v
		// �R���W�����p���b�V���͕`��X�L�b�v
		if (sub.meshName.find("collision") != std::string::npos ||
			sub.meshName.find("collsion") != std::string::npos ||
			//sub.meshName.find("Plane") != std::string::npos ||
			sub.meshName.find("Ferrari") != std::string::npos ||
			sub.meshName.find("Lamborghini") != std::string::npos ||
			sub.meshName.find("polySurface735") != std::string::npos ||
			sub.meshName.find("Object_43") != std::string::npos ||
			sub.meshName.find("Object_45") != std::string::npos ||
			sub.meshName.find("model_13") != std::string::npos ||
			sub.meshName.find("Combined3D") != std::string::npos)
			continue;

		//  �m�[�h�ϊ� �~ �S�̃g�����X�t�H�[�� = �e�s�[�X�̐������ʒu
		XMMATRIX nodeWorld = XMLoadFloat4x4(&sub.nodeTransform) * globalTransform;
		m_mapEffect->SetWorld(nodeWorld);

		// �}�e���A���ݒ�
		if (sub.materialIndex >= 0 && sub.materialIndex < (int)m_materials.size())
		{
			const auto& mat = m_materials[sub.materialIndex];
			if (mat.diffuseTexture)
			{
				m_mapEffect->SetTextureEnabled(true);
				m_mapEffect->SetTexture(mat.diffuseTexture.Get());
				m_mapEffect->SetDiffuseColor(XMVectorSet(1, 1, 1, 1));
			}
			else
			{
				m_mapEffect->SetTextureEnabled(false);
				m_mapEffect->SetDiffuseColor(XMLoadFloat4(&mat.diffuseColor));
			}
		}

		m_mapEffect->Apply(context);

		// �o�b�t�@�ݒ�
		UINT stride = sizeof(VertexPositionNormalTexture);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, sub.vertexBuffer.GetAddressOf(),
			&stride, &offset);
		context->IASetIndexBuffer(sub.indexBuffer.Get(),
			DXGI_FORMAT_R32_UINT, 0);

		// �`����s
		context->DrawIndexed(sub.indexCount, 0, 0);
	}

	// DepthStencil���f�t�H���g�ɖ߂�
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);

	context->RSSetState(m_states->CullClockwise());

	//  �p�C�v���C����Ԃ����Z�b�g�i����Draw�֐��ɉe�������Ȃ��j
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// ============================================
//  �g�����X�t�H�[���ݒ�
// ============================================
void MapSystem::SetMapTransform(XMFLOAT3 pos, float rotY, float scale)
{
	m_mapPosition = pos;
	m_mapRotationY = rotY;
	m_mapScale = scale;
}

// ============================================
//  �`��i���C���j
// ============================================
void MapSystem::Draw(ID3D11DeviceContext* context,
	XMMATRIX view, XMMATRIX projection)
{
	// FBX�}�b�v�`��
	DrawMapModel(context, view, projection);

	// �����v���~�e�B�u�`��i�f�o�b�O�E�����蔻������p�j
	if (m_drawPrimitives)
	{
		DrawPrimitives(context, view, projection);
	}
}

// ============================================
//  �����v���~�e�B�u�`��
// ============================================
void MapSystem::DrawPrimitives(ID3D11DeviceContext* context,
	XMMATRIX view, XMMATRIX projection)
{
	if (!m_mapEffect || !m_boxInputLayout) return;

	//  m_mapEffect ���g���ăv���~�e�B�u��`��
	//    FBX�Ɠ����G�t�F�N�g �� ���C�g�E�e�N�X�`���m���ɓ���
	m_mapEffect->SetView(view);
	m_mapEffect->SetProjection(projection);

	for (const auto& obj : m_objects)
	{
		XMMATRIX s = XMMatrixScaling(obj.scale.x, obj.scale.y, obj.scale.z);
		XMMATRIX r = XMMatrixRotationRollPitchYaw(
			obj.rotation.x, obj.rotation.y, obj.rotation.z);
		XMMATRIX t = XMMatrixTranslation(
			obj.position.x, obj.position.y, obj.position.z);
		XMMATRIX w = s * r * t;

		m_mapEffect->SetWorld(w);

		switch (obj.type)
		{
		case MapObjectType::FLOOR:
			if (m_floorTexture)
			{
				m_mapEffect->SetTextureEnabled(true);
				m_mapEffect->SetTexture(m_floorTexture.Get());
				m_mapEffect->SetDiffuseColor(XMVectorSet(1, 1, 1, 1));
			}
			else
			{
				m_mapEffect->SetTextureEnabled(false);
				m_mapEffect->SetDiffuseColor(XMLoadFloat4(&obj.color));
			}
			m_box->Draw(m_mapEffect.get(), m_boxInputLayout.Get());
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			break;

		case MapObjectType::WALL:
			if (m_wallTexture)
			{
				m_mapEffect->SetTextureEnabled(true);
				m_mapEffect->SetTexture(m_wallTexture.Get());
				m_mapEffect->SetDiffuseColor(XMVectorSet(1, 1, 1, 1));
			}
			else
			{
				m_mapEffect->SetTextureEnabled(false);
				m_mapEffect->SetDiffuseColor(XMLoadFloat4(&obj.color));
			}
			m_box->Draw(m_mapEffect.get(), m_boxInputLayout.Get());
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			break;

		case MapObjectType::BOX:
			m_mapEffect->SetTextureEnabled(false);
			m_mapEffect->SetDiffuseColor(XMLoadFloat4(&obj.color));
			m_box->Draw(m_mapEffect.get(), m_boxInputLayout.Get());
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			break;

		case MapObjectType::CYLINDER:
			m_mapEffect->SetTextureEnabled(false);
			m_mapEffect->SetDiffuseColor(XMLoadFloat4(&obj.color));
			m_cylinder->Draw(w, view, projection, XMLoadFloat4(&obj.color));
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			break;
		}
	}
}

// ============================================
//  �ȉ������R�[�h�i�ύX�Ȃ��j
// ============================================
void MapSystem::AddFloor(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color)
{
	MapObject obj;
	obj.type = MapObjectType::FLOOR;
	obj.position = position;
	obj.scale = scale;
	obj.rotation = XMFLOAT3(0, 0, 0);
	obj.color = color;
	obj.hasCollision = false;
	m_objects.push_back(obj);
}

void MapSystem::AddWall(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color)
{
	MapObject obj;
	obj.type = MapObjectType::WALL;
	obj.position = position;
	obj.scale = scale;
	obj.rotation = XMFLOAT3(0, 0, 0);
	obj.color = color;
	obj.hasCollision = true;
	m_objects.push_back(obj);
}

void MapSystem::AddBox(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color)
{
	MapObject obj;
	obj.type = MapObjectType::BOX;
	obj.position = position;
	obj.scale = scale;
	obj.rotation = XMFLOAT3(0, 0, 0);
	obj.color = color;
	obj.hasCollision = true;
	m_objects.push_back(obj);
}

bool MapSystem::CheckCollision(XMFLOAT3 position, float radius)
{
	// �v���C���[�̑���Y�i�ڂ̍���1.8�������j
	float footY = position.y - 1.8f;

	for (const auto& obj : m_objects)
	{
		if (!obj.hasCollision) continue;

		//  Y�͈̓`�F�b�N�i2�K�̕ǂ�1�K�Ō����Ȃ��悤�Ɂj
		float wallBottom = obj.position.y - obj.scale.y / 2.0f;
		float wallTop = obj.position.y + obj.scale.y / 2.0f;
		if (footY >= wallTop || footY + 1.5f <= wallBottom)
			continue;  // �ʂ̊K �� �X�L�b�v

		float dx = abs(position.x - obj.position.x);
		float dz = abs(position.z - obj.position.z);
		float halfWidth = obj.scale.x / 2.0f;
		float halfDepth = obj.scale.z / 2.0f;

		if (dx < (halfWidth + radius) && dz < (halfDepth + radius))
			return true;
	}
	return false;
}

//void MapSystem::CreateDefaultMap()
//{
//	m_objects.clear();
//	m_floorZones.clear();
//
//	// ============================================
//	//  Market AL_DANUBE �R���W���� (scale=0.005)
//	//  �}�b�v���͈�: X[-14, 13]  Z[-25, 30]
//	//  ���s�G���A: X[-8, 9]  Z[-10, 23]
//	// ============================================
//
//	XMFLOAT4 cOuter = { 0.0f, 0.8f, 0.0f, 0.5f };  // �O��=��
//	XMFLOAT4 cBuild = { 0.8f, 0.0f, 0.0f, 0.5f };  // ����=��
//	XMFLOAT4 cFloor = { 0.3f, 0.3f, 0.3f, 0.3f };
//
//	// === �� ===
//	AddFloor(XMFLOAT3(0.0f, -0.05f, 0.0f),
//		XMFLOAT3(30.0f, 0.1f, 60.0f), cFloor);
//
//	// ============================================
//	//  �y�O�ǁz�}�b�v���E�i�����̗����j
//	// ============================================
//	AddWall(XMFLOAT3(-14.0f, 3.0f, 0.0f), XMFLOAT3(1.0f, 8.0f, 60.0f), cOuter);  // ��
//	AddWall(XMFLOAT3(14.0f, 3.0f, 0.0f), XMFLOAT3(1.0f, 8.0f, 60.0f), cOuter);  // ��
//	AddWall(XMFLOAT3(0.0f, 3.0f, -25.0f), XMFLOAT3(30.0f, 8.0f, 1.0f), cOuter);  // ��
//
//	// ============================================
//	//  �y���i���j�����Q�zX=-14 ? -7
//	//  �m�[�h: Grid(-0.7,2.2,-0.7) �� �����[
//	//  Tree(-12.4) �� �����̊O
//	//  �����ʂ� X?-7 ������
//	// ============================================
//	// ������ ��u���b�N Z[-24, -3]
//	AddWall(XMFLOAT3(-10.5f, 3.0f, -13.5f),
//		XMFLOAT3(7.0f, 8.0f, 21.0f), cBuild);
//
//	// ������ �����u���b�N Z[-1, 10]
//	AddWall(XMFLOAT3(-10.5f, 3.0f, 4.5f),
//		XMFLOAT3(7.0f, 8.0f, 11.0f), cBuild);
//
//	// ������ �k�u���b�N Z[12, 22]
//	AddWall(XMFLOAT3(-10.5f, 3.0f, 17.0f),
//		XMFLOAT3(7.0f, 8.0f, 10.0f), cBuild);
//
//	// ============================================
//	//  �y�E�i���j�����Q�zX=7 ? 14
//	//  Circle�Q: X=7?8.5 �ɓX�܂�����
//	// ============================================
//	// �E���� ��u���b�N Z[-24, -4]
//	AddWall(XMFLOAT3(10.5f, 3.0f, -14.0f),
//		XMFLOAT3(7.0f, 8.0f, 20.0f), cBuild);
//
//	// �E���� �����u���b�N Z[-2, 10]
//	AddWall(XMFLOAT3(10.5f, 3.0f, 4.0f),
//		XMFLOAT3(7.0f, 8.0f, 12.0f), cBuild);
//
//	// �E���� �k�u���b�N Z[12, 22]
//	AddWall(XMFLOAT3(10.5f, 3.0f, 17.0f),
//		XMFLOAT3(7.0f, 8.0f, 10.0f), cBuild);
//
//	// ============================================
//	//  �y�k�̑�^�����zCube Z=29.5
//	// ============================================
//	AddWall(XMFLOAT3(0.0f, 3.0f, 26.0f),
//		XMFLOAT3(30.0f, 8.0f, 10.0f), cBuild);
//
//	// ============================================
//	//  FloorZone�i���R�j
//	// ============================================
//	m_floorZones.push_back({ -14.0f, 14.0f, -25.0f, 30.0f, 0.0f, 0.0f, false });
//
//	char debug[256];
//	sprintf_s(debug, "MapSystem::CreateDefaultMap - %zu objects, %zu floor zones\n",
//		m_objects.size(), m_floorZones.size());
//	OutputDebugStringA(debug);
//}

void MapSystem::CreateDefaultMap()
{
	m_objects.clear();
	m_floorZones.clear();

	XMFLOAT4 cOuter = { 0.0f, 0.8f, 0.0f, 0.5f };
	XMFLOAT4 cBuild = { 0.8f, 0.0f, 0.0f, 0.5f };
	XMFLOAT4 cFloor = { 0.3f, 0.3f, 0.3f, 0.3f };

	//// ��
	//AddFloor(XMFLOAT3(0.0f, 0.1f, 5.0f),
	//	XMFLOAT3(60.0f, 0.1f, 120.0f), cFloor);

	// �O��
	AddWall(XMFLOAT3(-33.0f, 3.0f, 5.0f), XMFLOAT3(1.0f, 8.0f, 112.0f), cOuter); // ��
	AddWall(XMFLOAT3(44.0f, 3.0f, 5.0f), XMFLOAT3(1.0f, 8.0f, 112.0f), cOuter); // ��
	AddWall(XMFLOAT3(0.0f, 3.0f, -10.0f), XMFLOAT3(90.0f, 8.0f, 1.0f), cOuter);  // ��

	// �k����
	AddWall(XMFLOAT3(0.0f, 3.0f, 37.0f),
		XMFLOAT3(90.0f, 8.0f, 28.0f), cBuild);

	// FloorZone
	m_floorZones.push_back({ -32.0f, 43.0f, -9.0f, 60.0f, 0.0f, 0.0f, false });

	char debug[256];
	sprintf_s(debug, "MapSystem::CreateDefaultMap - %zu objects, %zu floor zones\n",
		m_objects.size(), m_floorZones.size());
	OutputDebugStringA(debug);
}


// ============================================
//  ���̍����擾�i�v���C���[�E�G��Y���W�p�j
// ============================================
float MapSystem::GetFloorHeight(float x, float z) const
{
	float bestHeight = 0.0f;  // �f�t�H���g = �n��K

	for (const auto& zone : m_floorZones)
	{
		// XZ�͈͓����`�F�b�N
		if (x >= zone.minX && x <= zone.maxX &&
			z >= zone.minZ && z <= zone.maxZ)
		{
			float h;
			if (zone.isSlopeZ)
			{
				// Z�����̃X���[�v�i�K�i�j
				// Z�͈͓��ł̐i�s�x(0.0�`1.0)���v�Z
				float t = (z - zone.minZ) / (zone.maxZ - zone.minZ);
				t = std::max(0.0f, std::min(1.0f, t));  // �N�����v
				h = zone.heightStart + (zone.heightEnd - zone.heightStart) * t;
			}
			else
			{
				// �t���b�g�ȏ�
				h = zone.heightStart;
			}

			// ��ԍ��������̗p�i�d�Ȃ����ꍇ�j
			if (h > bestHeight)
				bestHeight = h;
		}
	}

	return bestHeight;
}