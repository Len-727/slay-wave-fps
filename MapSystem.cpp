// MapSystem.cpp
// プリミティブ（当たり判定）+ FBXモデル（Assimp描画）
#define NOMINMAX
#include "MapSystem.h"

// Assimp（FBX読み込み用）
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ============================================
//  コンストラクタ
// ============================================
MapSystem::MapSystem() {}

// ============================================
//  初期化
// ============================================
bool MapSystem::Initialize(ID3D11DeviceContext* context, ID3D11Device* device)
{
	m_device = device;

	// 既存プリミティブ
	m_box = GeometricPrimitive::CreateBox(context, XMFLOAT3(1, 1, 1), true, false);
	m_cylinder = GeometricPrimitive::CreateCylinder(context, 1.0f, 1.0f, 32, true);
	m_states = std::make_unique<DirectX::CommonStates>(device);

	// BasicEffect（FBXマップ描画用）
	m_mapEffect = std::make_unique<BasicEffect>(device);
	m_mapEffect->SetTextureEnabled(true);		// テクスチャ有効
	m_mapEffect->SetLightingEnabled(true);		// ライティング有効
	//  暗い雰囲気のライティング
	m_mapEffect->SetLightEnabled(0, true);
	m_mapEffect->SetLightEnabled(1, true);
	m_mapEffect->SetLightEnabled(2, false);   // 3つ目のライトOFF

	// メインライト：上からの薄暗い暖色
	m_mapEffect->SetLightDirection(0, XMVectorSet(0.0f, -1.0f, 0.3f, 0.0f));
	m_mapEffect->SetLightDiffuseColor(0, XMVectorSet(0.6f, 0.45f, 0.3f, 1.0f));

	// 補助ライト：横からの薄い青（影に色を付ける）
	m_mapEffect->SetLightDirection(1, XMVectorSet(-0.5f, -0.3f, -0.7f, 0.0f));
	m_mapEffect->SetLightDiffuseColor(1, XMVectorSet(0.15f, 0.15f, 0.25f, 1.0f));

	// 環境光を暗く
	m_mapEffect->SetAmbientLightColor(XMVectorSet(0.08f, 0.06f, 0.05f, 1.0f));

	// 入力レイアウト作成
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
		OutputDebugStringA("MapSystem: 入力レイアウト作成失敗\n");
		return false;
	}

	OutputDebugStringA("MapSystem::Initialize - Success\n");

	hr = CreateWICTextureFromFile(
		device, L"Assets/Models/Map/textures/MALL_TIL23.jpg",   // ← 床テクスチャのパス
		nullptr, m_floorTexture.ReleaseAndGetAddressOf());
	if (SUCCEEDED(hr))
		OutputDebugStringA("[MapSystem] Floor tile texture loaded!\n");
	else
		OutputDebugStringA("[MapSystem] WARNING: floor tile texture not found\n");

	hr = CreateWICTextureFromFile(
		device, L"Assets/Models/Map/textures/OffsetCobblestoneDC19.jpg",    // ← 壁テクスチャのパス
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
//  FBXマップ読み込み（Assimp経由）
// ============================================
bool MapSystem::LoadMapFBX(const std::string& fbxPath,
	const std::wstring& textureDir, float scale)
{
	if (!m_device) return false;

	m_mapScale = scale;

	// --- Assimpでファイル読み込み ---
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fbxPath,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |		// DirectXは左手座標系
		aiProcess_GenNormals |
		aiProcess_GenUVCoords |
		aiProcess_CalcTangentSpace);

	if (!scene || !scene->mRootNode)
	{
		char buf[512];
		sprintf_s(buf, "MapSystem: FBX読み込み失敗: %s\n", importer.GetErrorString());
		OutputDebugStringA(buf);
		return false;
	}

	char debugBuf[256];
	sprintf_s(debugBuf, "MapSystem: FBX読み込み完了 - メッシュ:%d マテリアル:%d\n",
		scene->mNumMeshes, scene->mNumMaterials);
	OutputDebugStringA(debugBuf);

	// === マテリアル読み込み ===
	m_materials.resize(scene->mNumMaterials);
	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* mat = scene->mMaterials[i];

		// マテリアル名取得
		aiString matName;
		mat->Get(AI_MATKEY_NAME, matName);
		m_materials[i].name = matName.C_Str();

		// テクスチャ探索（DIFFUSE→BASE_COLORの順）
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

				// ファイル名だけ抽出（パスから）
				std::string texStr = texPath.C_Str();
				size_t lastSlash = texStr.find_last_of("/\\");
				std::string texFileName = (lastSlash != std::string::npos)
					? texStr.substr(lastSlash + 1) : texStr;

				// wstringに変換してフルパス作成
				std::wstring wTexFile(texFileName.begin(), texFileName.end());
				std::wstring fullPath = textureDir + L"/" + wTexFile;

				// テクスチャ読み込み
				HRESULT hr = CreateWICTextureFromFile(
					m_device, fullPath.c_str(), nullptr,
					m_materials[i].diffuseTexture.ReleaseAndGetAddressOf());

				if (SUCCEEDED(hr))
				{
					texLoaded = true;
					char db[512];
					sprintf_s(db, "  Material[%d] '%s': テクスチャOK\n", i, texFileName.c_str());
					OutputDebugStringA(db);
				}
			}
		}

		// フォールバック：マテリアル名からテクスチャを推測
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
					sprintf_s(db, "  Material[%d] フォールバック '%s': OK\n", i, tryName.c_str());
					OutputDebugStringA(db);
				}
			}
		}

		// テクスチャがない場合はディフューズ色を使う
		if (!texLoaded)
		{
			aiColor4D color(0.5f, 0.5f, 0.5f, 1.0f);
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			m_materials[i].diffuseColor = { color.r, color.g, color.b, color.a };

			char db[256];
			sprintf_s(db, "  Material[%d] '%s': テクスチャなし 色=(%.2f,%.2f,%.2f)\n",
				i, m_materials[i].name.c_str(), color.r, color.g, color.b);
			OutputDebugStringA(db);
		}
	}

	// === メッシュ読み込み ===
	m_subMeshes.resize(scene->mNumMeshes);
	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
	{
		aiMesh* mesh = scene->mMeshes[m];

		// 頂点データ作成
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

		// インデックスデータ作成
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

		// 頂点バッファ作成
		D3D11_BUFFER_DESC vbDesc = {};
		vbDesc.ByteWidth = (UINT)(vertices.size() * sizeof(VertexPositionNormalTexture));
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.Usage = D3D11_USAGE_DEFAULT;
		D3D11_SUBRESOURCE_DATA vbData = {};
		vbData.pSysMem = vertices.data();

		m_device->CreateBuffer(&vbDesc, &vbData,
			m_subMeshes[m].vertexBuffer.ReleaseAndGetAddressOf());

		// インデックスバッファ作成
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

		sprintf_s(debugBuf, "  Mesh[%d] '%s': %d頂点, %d三角形, mat=%d\n",
			m, mesh->mName.C_Str(), mesh->mNumVertices,
			mesh->mNumFaces, mesh->mMaterialIndex);
		OutputDebugStringA(debugBuf);
	}

	// ===  全メッシュのnodeTransformを単位行列で初期化 ===
   // （ノードに参照されないメッシュがゴミ値にならないように）
	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
	{
		XMStoreFloat4x4(&m_subMeshes[m].nodeTransform, XMMatrixIdentity());
	}

	// ===  ノード階層からメッシュごとの変換行列を取得 ===
	std::function<void(aiNode*, XMMATRIX)> processNode;
	processNode = [&](aiNode* node, XMMATRIX parentTransform)
		{
			// Assimpの行列を取得
			aiMatrix4x4 m = node->mTransformation;

			//  Assimp内蔵のTranspose()で変換
			// Assimp = 列ベクトル方式（translation = a4,b4,c4）
			// DirectX = 行ベクトル方式（translation = _41,_42,_43）
			// Transpose()で列→行に変換
			m.Transpose();

			XMMATRIX localTransform(
				m.a1, m.a2, m.a3, m.a4,
				m.b1, m.b2, m.b3, m.b4,
				m.c1, m.c2, m.c3, m.c4,
				m.d1, m.d2, m.d3, m.d4
			);

			// 親の変換と合成（DirectXの行ベクトル方式: local * parent）
			XMMATRIX worldTransform = localTransform * parentTransform;

			// デバッグ：ノード名と変換行列の一部を出力
			if (node->mNumMeshes > 0)
			{
				// translation成分（_41,_42,_43）を表示
				XMFLOAT4X4 dbgMat;
				XMStoreFloat4x4(&dbgMat, worldTransform);
				char db[256];
				sprintf_s(db, "  Node '%s': meshes=%d, pos=(%.2f, %.2f, %.2f)\n",
					node->mName.C_Str(), node->mNumMeshes,
					dbgMat._41, dbgMat._42, dbgMat._43);
				OutputDebugStringA(db);
			}

			// このノードが持つメッシュに変換行列を設定
			for (unsigned int i = 0; i < node->mNumMeshes; i++)
			{
				unsigned int meshIndex = node->mMeshes[i];
				if (meshIndex < m_subMeshes.size())
				{
					XMStoreFloat4x4(&m_subMeshes[meshIndex].nodeTransform, worldTransform);
				}
			}

			// 子ノードを再帰処理
			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				processNode(node->mChildren[i], worldTransform);
			}
		};

	//  ルートノードの行列もデバッグ出力
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

	m_mapLoaded = true;
	OutputDebugStringA("MapSystem: FBXマップ読み込み成功！\n");
	return true;
}

// ============================================
//  FBXモデル描画
// ============================================
void MapSystem::DrawMapModel(ID3D11DeviceContext* context,
	XMMATRIX view, XMMATRIX projection)
{

	//  ImGuiの値を反映
	ApplyLightSettings();

	if (!m_mapLoaded || !m_mapEffect) return;

	// 描画状態を完全リセット
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->RSSetState(m_states->CullNone());

	//  全体のトランスフォーム
	XMMATRIX globalTransform = XMMatrixScaling(m_mapScale, m_mapScale, m_mapScale)
		* XMMatrixRotationY(m_mapRotationY)
		* XMMatrixTranslation(m_mapPosition.x, m_mapPosition.y, m_mapPosition.z);

	m_mapEffect->SetView(view);
	m_mapEffect->SetProjection(projection);

	context->IASetInputLayout(m_mapInputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// 各サブメッシュを描画
	for (const auto& sub : m_subMeshes)
	{
		if (!sub.vertexBuffer || !sub.indexBuffer || sub.indexCount == 0)
			continue;

		//  コリジョン用メッシュは描画スキップ
		// コリジョン用メッシュは描画スキップ
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

		//  ノード変換 × 全体トランスフォーム = 各ピースの正しい位置
		XMMATRIX nodeWorld = XMLoadFloat4x4(&sub.nodeTransform) * globalTransform;
		m_mapEffect->SetWorld(nodeWorld);

		// マテリアル設定
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

		// バッファ設定
		UINT stride = sizeof(VertexPositionNormalTexture);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, sub.vertexBuffer.GetAddressOf(),
			&stride, &offset);
		context->IASetIndexBuffer(sub.indexBuffer.Get(),
			DXGI_FORMAT_R32_UINT, 0);

		// 描画実行
		context->DrawIndexed(sub.indexCount, 0, 0);
	}

	// DepthStencilをデフォルトに戻す
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);

	context->RSSetState(m_states->CullClockwise());

	//  パイプライン状態をリセット（他のDraw関数に影響させない）
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// ============================================
//  トランスフォーム設定
// ============================================
void MapSystem::SetMapTransform(XMFLOAT3 pos, float rotY, float scale)
{
	m_mapPosition = pos;
	m_mapRotationY = rotY;
	m_mapScale = scale;
}

// ============================================
//  描画（メイン）
// ============================================
void MapSystem::Draw(ID3D11DeviceContext* context,
	XMMATRIX view, XMMATRIX projection)
{
	// FBXマップ描画
	DrawMapModel(context, view, projection);

	// 既存プリミティブ描画（デバッグ・当たり判定可視化用）
	if (m_drawPrimitives)
	{
		DrawPrimitives(context, view, projection);
	}
}

// ============================================
//  既存プリミティブ描画
// ============================================
void MapSystem::DrawPrimitives(ID3D11DeviceContext* context,
	XMMATRIX view, XMMATRIX projection)
{
	if (!m_mapEffect || !m_boxInputLayout) return;

	//  m_mapEffect を使ってプリミティブを描画
	//    FBXと同じエフェクト → ライト・テクスチャ確実に動作
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
//  以下既存コード（変更なし）
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
	// プレイヤーの足元Y（目の高さ1.8を引く）
	float footY = position.y - 1.8f;

	for (const auto& obj : m_objects)
	{
		if (!obj.hasCollision) continue;

		//  Y範囲チェック（2階の壁が1階で効かないように）
		float wallBottom = obj.position.y - obj.scale.y / 2.0f;
		float wallTop = obj.position.y + obj.scale.y / 2.0f;
		if (footY >= wallTop || footY + 1.5f <= wallBottom)
			continue;  // 別の階 → スキップ

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
//	//  Market AL_DANUBE コリジョン (scale=0.005)
//	//  マップ実範囲: X[-14, 13]  Z[-25, 30]
//	//  歩行エリア: X[-8, 9]  Z[-10, 23]
//	// ============================================
//
//	XMFLOAT4 cOuter = { 0.0f, 0.8f, 0.0f, 0.5f };  // 外壁=緑
//	XMFLOAT4 cBuild = { 0.8f, 0.0f, 0.0f, 0.5f };  // 建物=赤
//	XMFLOAT4 cFloor = { 0.3f, 0.3f, 0.3f, 0.3f };
//
//	// === 床 ===
//	AddFloor(XMFLOAT3(0.0f, -0.05f, 0.0f),
//		XMFLOAT3(30.0f, 0.1f, 60.0f), cFloor);
//
//	// ============================================
//	//  【外壁】マップ境界（建物の裏側）
//	// ============================================
//	AddWall(XMFLOAT3(-14.0f, 3.0f, 0.0f), XMFLOAT3(1.0f, 8.0f, 60.0f), cOuter);  // 西
//	AddWall(XMFLOAT3(14.0f, 3.0f, 0.0f), XMFLOAT3(1.0f, 8.0f, 60.0f), cOuter);  // 東
//	AddWall(XMFLOAT3(0.0f, 3.0f, -25.0f), XMFLOAT3(30.0f, 8.0f, 1.0f), cOuter);  // 南
//
//	// ============================================
//	//  【左（西）建物群】X=-14 ? -7
//	//  ノード: Grid(-0.7,2.2,-0.7) → 建物端
//	//  Tree(-12.4) → 建物の外
//	//  建物面は X?-7 あたり
//	// ============================================
//	// 左建物 南ブロック Z[-24, -3]
//	AddWall(XMFLOAT3(-10.5f, 3.0f, -13.5f),
//		XMFLOAT3(7.0f, 8.0f, 21.0f), cBuild);
//
//	// 左建物 中央ブロック Z[-1, 10]
//	AddWall(XMFLOAT3(-10.5f, 3.0f, 4.5f),
//		XMFLOAT3(7.0f, 8.0f, 11.0f), cBuild);
//
//	// 左建物 北ブロック Z[12, 22]
//	AddWall(XMFLOAT3(-10.5f, 3.0f, 17.0f),
//		XMFLOAT3(7.0f, 8.0f, 10.0f), cBuild);
//
//	// ============================================
//	//  【右（東）建物群】X=7 ? 14
//	//  Circle群: X=7?8.5 に店舗が並ぶ
//	// ============================================
//	// 右建物 南ブロック Z[-24, -4]
//	AddWall(XMFLOAT3(10.5f, 3.0f, -14.0f),
//		XMFLOAT3(7.0f, 8.0f, 20.0f), cBuild);
//
//	// 右建物 中央ブロック Z[-2, 10]
//	AddWall(XMFLOAT3(10.5f, 3.0f, 4.0f),
//		XMFLOAT3(7.0f, 8.0f, 12.0f), cBuild);
//
//	// 右建物 北ブロック Z[12, 22]
//	AddWall(XMFLOAT3(10.5f, 3.0f, 17.0f),
//		XMFLOAT3(7.0f, 8.0f, 10.0f), cBuild);
//
//	// ============================================
//	//  【北の大型建物】Cube Z=29.5
//	// ============================================
//	AddWall(XMFLOAT3(0.0f, 3.0f, 26.0f),
//		XMFLOAT3(30.0f, 8.0f, 10.0f), cBuild);
//
//	// ============================================
//	//  FloorZone（平坦）
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

	//// 床
	//AddFloor(XMFLOAT3(0.0f, 0.1f, 5.0f),
	//	XMFLOAT3(60.0f, 0.1f, 120.0f), cFloor);

	// 外壁
	AddWall(XMFLOAT3(-33.0f, 3.0f, 5.0f), XMFLOAT3(1.0f, 8.0f, 112.0f), cOuter); // 西
	AddWall(XMFLOAT3(44.0f, 3.0f, 5.0f), XMFLOAT3(1.0f, 8.0f, 112.0f), cOuter); // 東
	AddWall(XMFLOAT3(0.0f, 3.0f, -10.0f), XMFLOAT3(90.0f, 8.0f, 1.0f), cOuter);  // 南

	// 北建物
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
//  床の高さ取得（プレイヤー・敵のY座標用）
// ============================================
float MapSystem::GetFloorHeight(float x, float z) const
{
	float bestHeight = 0.0f;  // デフォルト = 地上階

	for (const auto& zone : m_floorZones)
	{
		// XZ範囲内かチェック
		if (x >= zone.minX && x <= zone.maxX &&
			z >= zone.minZ && z <= zone.maxZ)
		{
			float h;
			if (zone.isSlopeZ)
			{
				// Z方向のスロープ（階段）
				// Z範囲内での進行度(0.0～1.0)を計算
				float t = (z - zone.minZ) / (zone.maxZ - zone.minZ);
				t = std::max(0.0f, std::min(1.0f, t));  // クランプ
				h = zone.heightStart + (zone.heightEnd - zone.heightStart) * t;
			}
			else
			{
				// フラットな床
				h = zone.heightStart;
			}

			// 一番高い床を採用（重なった場合）
			if (h > bestHeight)
				bestHeight = h;
		}
	}

	return bestHeight;
}