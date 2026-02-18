//	Model.cpp
#include "Model.h"
#include <DirectXColors.h>
#include <assimp/Importer.hpp>	//	Assimp　のインポーター	(FBX, OBJ)
#include <assimp/scene.h>		//	読み込んだシーン(ノード階層・メッシュ・マテリアル・アニメーション)にアクセスする
#include <assimp/postprocess.h>	//	三角形化・法線/接戦生成・頂点結合などの後処理フラグ
#include <CommonStates.h>
#include <Effects.h>
#include <VertexTypes.h> // DirectXTKの標準頂点型定義
#include <functional>
#include <algorithm>
#include <WICTextureLoader.h>
#include <functional>  // std::function用

#include "InstanceData.h"

#pragma comment(lib, "assimp-vc143-mt.lib")

using namespace DirectX;

using std::min;

//using namespace Colors;

Model::Model()
{
}


std::string Model::GetShortName(const std::string& fullName)
{
	std::string name = fullName;

	// コロンより前の部分を削除
	size_t colonPos = name.find(':');
	if (colonPos != std::string::npos)
	{
		name = name.substr(colonPos + 1);
	}


	// Assimpが付ける余計なサフィックスを削除する処理を追加（最小限）
	const std::string assimpSuffix = "_$AssimpFbx$_";
	size_t assimpPos = name.find(assimpSuffix);
	if (assimpPos != std::string::npos)
	{
		name = name.substr(0, assimpPos);
	}

	return name;
}


//	ファイルから3Dモデルを読み込む
bool Model::LoadFromFile(ID3D11Device* device, const std::string& filename)
{
	//	=== Assimp ファイル読み込み ===
	Assimp::Importer importer;

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	//	ファイルを読み込む
	//	【フラグ】どう処理するか指定
	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_Triangulate |			//	全て三角形に変換
		aiProcess_ConvertToLeftHanded |	//	DirectX用に左手座標系に
		aiProcess_GenNormals |			//	法線を自動生成
		aiProcess_CalcTangentSpace |	    //	タンジェント計算
		aiProcess_FlipUVs               // 座標を反転 (テクスチャが上下逆になるのを修正。ディフューズカラーには無関係)
		//aiProcess_FlipNormals             // 
	);

	//	読み込み失敗チェック
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		//	エラーメッセージを出力
		/*OutputDebugStringA("Model::LoadFromFIle - Failed to load: ");
		OutputDebugStringA(filename.c_str());
		OutputDebugStringA("\n");
		OutputDebugStringA(importer.GetErrorString());
		OutputDebugStringA("\n");*/
		return false;
	}

	//	=== DirectXTK のエッフェクト作成 ===
	//	BasicEffectを"ライティング用に設定"
	m_effect = std::make_unique<DirectX::SkinnedEffect>(device);	//	DirectXTKの基本シェーダ(描画効果)を作成
	m_effect->SetWeightsPerVertex(4);
	m_states = std::make_unique<DirectX::CommonStates>(device);

	//	---	初期ボーン行列を設定
	DirectX::XMMATRIX id = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX bones[DirectX::SkinnedEffect::MaxBones];

	for (int i = 0; i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		bones[i] = id;
	}

	m_effect->SetBoneTransforms(bones, DirectX::SkinnedEffect::MaxBones);

	m_effect->SetDiffuseColor(DirectX::Colors::White);

	m_effect->SetAlpha(1.0f);

	//	入力レイアウトを作成
	//	BasicEffectが内部で使う頂点シェーダのバイトコードを取り出す
	void const* shaderByteCode;
	size_t byteCodeLength;
	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);


	//	---	入力エレメント記述配列を定義	---
	D3D11_INPUT_ELEMENT_DESC inputElements[] = {
		{ "SV_Position",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",         0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",       0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES",   0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};


	//	---	入力レイアウトを作成	---
	HRESULT hr = device->CreateInputLayout(
		inputElements,	//	エレメント配列
		_countof(inputElements),	//	エレメント数
		shaderByteCode,
		byteCodeLength,
		m_inputLayout.GetAddressOf()
	);

	if (FAILED(hr))
	{
		//	---	エラーの詳細を出力	---
		/*char errorMsg[512];
		sprintf_s(errorMsg, "Model::LoadFromFile - Failed to create input layout (HRESULT: 0x%08X)\n", hr);
		OutputDebugStringA(errorMsg);*/
		return false;
	}

	//OutputDebugStringA("Model::LoadFromFile - Input layout created successfully\n");

	//	===	各メッシュを処理	===
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* aiMesh = scene->mMeshes[i];

		Mesh mesh;

		//	===頂点データを変換	===
		for (unsigned int v = 0; v < aiMesh->mNumVertices; v++)
		{
			ModelVertex vertex;

			//	位置
			vertex.position.x = aiMesh->mVertices[v].x;
			vertex.position.y = aiMesh->mVertices[v].y;
			vertex.position.z = aiMesh->mVertices[v].z;

			//	法線	(面の向きをを表す矢印)
			if (aiMesh->HasNormals())
			{
				vertex.normal.x = aiMesh->mNormals[v].x;
				vertex.normal.y = aiMesh->mNormals[v].y;
				vertex.normal.z = aiMesh->mNormals[v].z;
			}
			else
			{
				vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0);	//	デフォルト(上向き)
			}

			//	テクスチャ座標
			if (aiMesh->HasTextureCoords(0))
			{
				//	テクスチャーは２次元
				vertex.texCoord.x = aiMesh->mTextureCoords[0][v].x;
				vertex.texCoord.y = aiMesh->mTextureCoords[0][v].y;
			}
			else
			{
				vertex.texCoord = XMFLOAT2(0, 0);
			}
			//	頂点をひとつづつ完成させて、描画用の頂点配列に積み上げる。
			mesh.vertices.push_back(vertex);
		}


		//	===	ボーン情報の読み込む	===

		//	---	子のメッシュにボーンがあるか確認	---
		if (aiMesh->HasBones())
		{
			char debugBones[256];
			sprintf_s(debugBones, "Mesh %d has %d bones\n", i, aiMesh->mNumBones);
			OutputDebugStringA(debugBones);

			//	各ボーンをループ
			//	全てのウェイトを収集
			for (unsigned int b = 0; b < aiMesh->mNumBones; b++)
			{
				aiBone* bone = aiMesh->mBones[b];

				//---	ボーン名を取得	---
				std::string boneName = bone->mName.C_Str();

				//	---子のボーンがm_bonesに存在するか確認	---
				//	ボーンインデックスを取得(何番目のボーンか)
				int boneIndex = -1;
				for (size_t j = 0; j < m_bones.size(); j++)
				{
					if (m_bones[j].name == boneName)
					{
						boneIndex = static_cast<int>(j);
						break;
					}
				}

				//	---	見つからない場合は新規追加	---
				if (boneIndex == -1)
				{
					//	新しいボーンを作成
					Bone newBone;
					newBone.name = boneName;

					//	オフセット行列を設定
					//	初期姿勢からボーン空間への変換
					//	頂点をボーン座標系に変換するために必要
					aiMatrix4x4 offsetMatrix = bone->mOffsetMatrix;

					//	aiMatrix4x4 → DirectX::XMMATRIX に変換
					newBone.offsetMatrix = DirectX::XMMATRIX(
						offsetMatrix.a1, offsetMatrix.b1, offsetMatrix.c1, offsetMatrix.d1,
						offsetMatrix.a2, offsetMatrix.b2, offsetMatrix.c2, offsetMatrix.d2,
						offsetMatrix.a3, offsetMatrix.b3, offsetMatrix.c3, offsetMatrix.d3,
						offsetMatrix.a4, offsetMatrix.b4, offsetMatrix.c4, offsetMatrix.d4
					);

					//	親ボーンは後で
					newBone.parentIndex = -1;

					//	ボーン配列に追加
					m_bones.push_back(newBone);

					//	このボーンのインデックス
					boneIndex = static_cast<int>(m_bones.size() - 1);

					char debugNewBone[256];
					sprintf_s(debugNewBone, "  Added bone[%d]: %s\n", boneIndex, boneName.c_str());
					OutputDebugStringA(debugNewBone);
				}

				//	---	このボーンが影響する頂点をループ	---
				//	各頂点にボーンインデックスとウェイトを設定
				for (unsigned int w = 0; w < bone->mNumWeights; w++)
				{
					aiVertexWeight weight = bone->mWeights[w];

					unsigned int vertexId = weight.mVertexId;
					float vertexWeight = weight.mWeight;

					//	---	頂点番号が範囲内か確認	---
					if (vertexId >= mesh.vertices.size())
					{
						//	範囲外（エラー）
						OutputDebugStringA("Warning: Vertex ID out of range\n");
						continue;
					}

					//	---この頂点の空いているスロットを探す	---
					ModelVertex& vertex = mesh.vertices[vertexId];

					bool added = false;
					for (int slot = 0; slot < 4; slot++)
					{
						//	子のスロットが空いているか確認
						if (vertex.boneWeights[slot] == 0.0f)
						{
							//	空いているここに設定
							vertex.boneIndices[slot] = boneIndex;
							vertex.boneWeights[slot] = vertexWeight;
							added = true;
							break;
						}
					}

					//	---	スロットが埋まってる場合	---
					if (!added)
					{
						//OutputDebugStringA("Warning: Vertex has more than 4 bone weights\n");
					}
				}
			}//nn

			//	---	ウェイト正規化	---
			//	各頂点のウェイトを合計1.0にする
			for (auto& vertex : mesh.vertices)
			{
				//	ウェイトの合計を計算
				float totalWeight = 0.0f;
				for (int slot = 0; slot < 4; slot++)
				{
					totalWeight += vertex.boneWeights[slot];
				}

				//	合計が0の場合
				if (totalWeight == 0.0f)
				{
					vertex.boneIndices[0] = 0;
					vertex.boneWeights[0] = 1.0f;
				}
				else if (totalWeight != 1.0f)
				{
					//	正規化(合計を1.0にする)
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


		//	===	indexデータを変換		=== (どの頂点をつないで三角形を作るか)
		//	このメッシュに含まれる全ての面を１つずつ処理するループ
		for (unsigned int f = 0; f < aiMesh->mNumFaces; f++)
		{
			//	[f]番目のデータをローカル変数にコピー
			aiFace face = aiMesh->mFaces[f];

			//	その面が持つindex(頂点番号)を戦闘から順に読む
			for (unsigned int idx = 0; idx < face.mNumIndices; idx++)
			{
				//	読み取った頂点座標を、自前メッシュのインデックス配列に追加
				mesh.indices.push_back(face.mIndices[idx]);
			}
		}

		//	GPUにバッファを作成
		if (!CreateBuffers(device, mesh))
		{
			//OutputDebugStringA("Model::LoadFromFile - Failed to create buffers\n");
			return false;
		}

		//	メッシュを保存
		m_meshes.push_back(std::move(mesh));

		//// デバッグ：メッシュごとの頂点ボーン情報
		//char meshDebug[256];
		//sprintf_s(meshDebug, "Mesh %d: %zu vertices, first vertex bone[0]=%d, weight=%.2f\n",
		//	i, mesh.vertices.size(),
		//	mesh.vertices[0].boneIndices[0],
		//	mesh.vertices[0].boneWeights[0]);
		//OutputDebugStringA(meshDebug);
	}


	// ルートノードの変換行列の「逆」を持つことで、メッシュを原点に戻して計算できます
	aiMatrix4x4 globalInv = scene->mRootNode->mTransformation;
	globalInv.Inverse();
	m_globalInverseTransform = DirectX::XMMATRIX(
		globalInv.a1, globalInv.b1, globalInv.c1, globalInv.d1,
		globalInv.a2, globalInv.b2, globalInv.c2, globalInv.d2,
		globalInv.a3, globalInv.b3, globalInv.c3, globalInv.d3,
		globalInv.a4, globalInv.b4, globalInv.c4, globalInv.d4
	);

	// 2. ノード階層を構築
	m_nodes.clear();
	ReadNodeHierarchy(scene->mRootNode, -1);

	//DebugDumpSkeleton();

	//デバッグ出力
	char debug[256];
	sprintf_s(debug, "Model::LoadFromFIle = Success: %s (%zu meshes)\n",
		filename.c_str(), m_meshes.size());
	OutputDebugStringA(debug);

	//	===	元のオフセット行列を保存	===
	m_originalOffsetMatrices.clear();
	m_originalOffsetMatrices.reserve(m_bones.size());
	for (const auto& bone : m_bones)
	{
		m_originalOffsetMatrices.push_back(bone.offsetMatrix);
	}

	// =============================================================
	//  埋め込みテクスチャ（Embedded Texture）を読み込む
	//  MixamoのFBX BinaryはPBRワークフローを使うことが多く、
	//  DIFFUSE ではなく BASE_COLOR 等にテクスチャが登録されている
	// =============================================================
	if (scene->mNumMaterials > 0)
	{
		// --- 試すテクスチャタイプの一覧---
		// Mixamoは BASE_COLOR を使うことが多い
		// 従来のモデルは DIFFUSE を使う
		aiTextureType textureTypes[] = {
			aiTextureType_DIFFUSE,
			aiTextureType_BASE_COLOR,
			aiTextureType_UNKNOWN,
		};

		bool textureLoaded = false;

		// --- 全マテリアルを走査 ---
		for (unsigned int matIdx = 0; matIdx < scene->mNumMaterials && !textureLoaded; matIdx++)
		{
			aiMaterial* material = scene->mMaterials[matIdx];

			// デバッグ：マテリアル名を出力
			aiString matName;
			material->Get(AI_MATKEY_NAME, matName);
			char debugMat[512];
			sprintf_s(debugMat, "[TEXTURE] Checking material[%d]: %s\n", matIdx, matName.C_Str());
			OutputDebugStringA(debugMat);

			// --- 各テクスチャタイプを順番に試す ---
			for (int t = 0; t < _countof(textureTypes) && !textureLoaded; t++)
			{
				aiString texturePath;

				if (material->GetTexture(textureTypes[t], 0, &texturePath) == AI_SUCCESS)
				{
					const char* path = texturePath.C_Str();

					char debugTex[512];
					sprintf_s(debugTex, "[TEXTURE] Found texture (type=%d): %s\n", textureTypes[t], path);
					OutputDebugStringA(debugTex);

					// --- 埋め込みテクスチャを取得 ---
					const aiTexture* embeddedTex = scene->GetEmbeddedTexture(path);

					if (embeddedTex)
					{
						OutputDebugStringA("[TEXTURE] Embedded texture found!\n");

						if (embeddedTex->mHeight == 0)
						{
							// 圧縮形式（PNG/JPG）のバイナリデータ
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
						// 外部ファイルとして試す
						std::string texFile = path;
						std::wstring wTexFile(texFile.begin(), texFile.end());

						HRESULT hr = DirectX::CreateWICTextureFromFile(
							device, wTexFile.c_str(), nullptr,
							m_diffuseTexture.ReleaseAndGetAddressOf());

						if (FAILED(hr))
						{
							// モデルと同じフォルダも試す
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

			// === 最終手段：scene->mTextures[] を直接チェック ===
			// マテリアルに登録されてなくても、FBX内に埋め込まれてる場合がある
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



//	頂点バッファ・インデックスバッファを作成
//	【役割】CPU のデータを GPUに転送
bool Model::CreateBuffers(ID3D11Device* device, Mesh& mesh)
{
	//	===	頂点バッファ作成	===
	D3D11_BUFFER_DESC vbDesc = {};	//	バッファのサイズ・用途・アクセス権をまとめる構造体を0で初期化
	vbDesc.Usage = D3D11_USAGE_DEFAULT;	//	GPU 専用メモリ	使用方法デフォルト
	vbDesc.ByteWidth = static_cast<UINT>(mesh.vertices.size() * sizeof(ModelVertex));	//	サイズ
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;	//	頂点バッファとして使う
	vbDesc.CPUAccessFlags = 0;	//	CPUからアクセスしない


	//	初期データ (CPUからコピーする元データ)
	D3D11_SUBRESOURCE_DATA vbData = {};	//	初期データ用の入れ物
	vbData.pSysMem = mesh.vertices.data();	//	CPU側の元データ先頭アドレスを指定


	//	GPUにバッファを作成　＋ データコピー
	//	&vbDesc どんなバッファかの仕様書	&vbData 初期データの箱
	HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, mesh.vertexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	//	===	インデックスバッファの作成	===
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));	//	indexの個数　* 一個当たりのサイズ = 総バイト数
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;	//	インデックスバッファとして扱う
	ibDesc.CPUAccessFlags = 0;	//	CPUから触らない

	D3D11_SUBRESOURCE_DATA ibData{};
	ibData.pSysMem = mesh.indices.data();

	hr = device->CreateBuffer(&ibDesc, &ibData, mesh.indexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		return false;
	}

	return true;

}

//	モデルを描画
void Model::Draw(ID3D11DeviceContext* context,
	XMMATRIX world,
	XMMATRIX view,
	XMMATRIX projection,
	XMVECTOR color)
{
	//	===	エッフェクトに行列を設定	===
	m_effect->SetWorld(world);				//	モデル固有の変換(位置・回転・拡縮)
	m_effect->SetView(view);				//	カメラ(視点)の変換(ワールドビュー→ビュー)
	m_effect->SetProjection(projection);	//	投影(ビュー→クリップ:透視/正射影)

	m_effect->SetDiffuseColor(color);

	//	===	ライトの設定	===
	m_effect->EnableDefaultLighting();

	//	環境工設定
	m_effect->SetAmbientLightColor(XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));
	//	ディフューズカラーはモデル全体の色と光の反射色を兼ねる
	m_effect->SetDiffuseColor(color);

	//	===	エフェクト適用	===
	m_effect->Apply(context);
	// デバッグログ追加
	//OutputDebugStringA("[MODEL] Draw called\n");

	// 深度書き込みを強制有効化
	if (m_states)
	{
		context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
		//OutputDebugStringA("[MODEL] Depth state set\n");
	}
	else
	{
		OutputDebugStringA("[MODEL] ERROR: m_states is null!\n");
	}
	//	===	入力レイアウトを設定	===
	context->IASetInputLayout(m_inputLayout.Get());	//	頂点データはこの並び(InputLayout)で読む
	//	これから送る頂点/indexを三角形リストとして解釈
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//	===	各メッシュを描画	===
	for (const auto& mesh : m_meshes)
	{
		//	頂点バッファを設定
		UINT stride = sizeof(ModelVertex);	//	1頂点のサイズ
		UINT offset = 0;					//	オフセット
		context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);

		//	インデックスバッファを設定
		context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//	描画命令
		context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}

}


//	===	アニメーションファイルを読み込む	===
bool Model::LoadAnimation(const std::string& filename, const std::string& animationName)
{
	//	---	Assimpでファイルを読み込む	---
	Assimp::Importer importer;	//	ファイルを読み込むクラス

	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

	const aiScene* scene = importer.ReadFile(
		filename,
		aiProcess_Triangulate | 
		aiProcess_ConvertToLeftHanded |
		aiProcess_FlipUVs);

	//	---	エラーチェック	---
	if (!scene || !scene->mRootNode)
	{
		//	読み込み失敗
		/*OutputDebugStringA("Model::LoadAnimation - Failed to load file:");
		OutputDebugStringA(filename.c_str());
		OutputDebugStringA("\n");*/
		return false;
	}

	if (scene->mNumAnimations == 0)
	{
		//	アニメーションが含まれていない
		/*OutputDebugStringA("Model::LoadAnimation - No animations in file");
		OutputDebugStringA(filename.c_str());
		OutputDebugStringA("\n");*/
		return false;
	}

	//	---	最初のアニメーションを取得	---
	aiAnimation* anim = scene->mAnimations[0];

	//	AnimationClip 構造体を作成
	AnimationClip clip;
	
	//	名前を設定
	clip.name = animationName;	//	引数で渡された名前

	//	再生時間を設定
	clip.duration = static_cast<float>(anim->mDuration / anim->mTicksPerSecond);

	//	再生速度を設定
	clip.ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);


	//	---	各ボーンのアニメーションデータ(チャンネル)を抽出
	for (unsigned int i = 0; i < anim->mNumChannels; i++)
	{
		aiNodeAnim* channel = anim->mChannels[i];

		//	AnimationChannel　構造体を作成
		AnimationChannel animChannel;

		//	ボーン名を設定
		animChannel.boneName = channel->mNodeName.C_Str();


		//	---	キーフレーム抽出	---

		//	キーフレーム数の最大値を取得
		unsigned int numKeys = channel->mNumPositionKeys;	//	とりあえず位置の数
		if (channel->mNumRotationKeys > numKeys)
			numKeys = channel->mNumRotationKeys;	//	回転のほうが多い
		if (channel->mNumScalingKeys > numKeys)
			numKeys = channel->mNumScalingKeys;		//	スケールのほうが多い

		//	キーフレーム配列のサイズを確保
		animChannel.keyframes.reserve(numKeys);

		//	各フレームを処理
		for (unsigned int k = 0; k < numKeys; k++)
		{
			KeyFrame keyframe;

			//	---	時刻を設定	---
			if (k < channel->mNumPositionKeys)
			{
				keyframe.time = static_cast<float>(
					channel->mPositionKeys[k].mTime / anim->mTicksPerSecond);
			
				//	【計算】ティック　→　秒に変換
			}
			else
			{
				//	位置キーフレームより多い場合
				keyframe.time = 0.0f;
			}

			//	---	位置を設定	---
			if (k < channel->mNumPositionKeys)
			{
				aiVector3D pos = channel->mPositionKeys[k].mValue;
				keyframe.position = DirectX::XMFLOAT3(pos.x, pos.y, pos.z);
				//	【変換】AssimpのaiVector3D　→　DirectX の XMFLOAT3
			}
			else
			{
				//	位置キーフレームがない場合
				keyframe.position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			}

			//	---	回転を設定(クォータニオン)	---
			//	補間がスムーズ
			//	メモリ効率がいい
			if (k < channel->mNumRotationKeys)
			{
				aiQuaternion rot = channel->mRotationKeys[k].mValue;
				keyframe.rotation = DirectX::XMFLOAT4(rot.x, rot.y, rot.z, rot.w);
				//	【変換】Assimp の aiQuaternion → DirectX の XMFLOAT4
			}
			else
			{
				//	回転キーフレームがない場合
				keyframe.rotation = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
			}

			//	---	スケール設定	---
			if (k < channel->mNumScalingKeys)
			{
				aiVector3D scale = channel->mScalingKeys[k].mValue;
				keyframe.scale = DirectX::XMFLOAT3(scale.x, scale.y, scale.z);

			}
			else
			{
				keyframe.scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
			}

			//	キーフレームっ配列に追加
			animChannel.keyframes.push_back(keyframe);
		}
		//	チャンネルをアニメーションクリップに追加
		clip.channels.push_back(animChannel);
	}

	//---	アニメーションクリップを保存	---
	m_animations[animationName] = clip;

	// デバッグ出力
	/*char debugMsg[256];
	sprintf_s(debugMsg, "Model::LoadAnimation - Loaded: %s (%d channels, %.2f sec)\n",
		animationName.c_str(),
		static_cast<int>(clip.channels.size()),
		clip.duration);
	OutputDebugStringA(debugMsg);*/

	//for (const auto& channel : clip.channels)
	//{
	//	// 元のログ
	//	OutputDebugStringA(("AnimChannel: " + channel.boneName + "\n").c_str());

	//	// 追加の詳細ログ
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





	return true;  // 成功

}

//	===	アニメーションの長さを取得	===
float Model::GetAnimationDuration(const std::string& animationName) const
{
	//	---	アニメーションを検索	---
	auto it = m_animations.find(animationName);

	//	見つかったか確認
	if (it != m_animations.end())
	{
		return it->second.duration;
	}
	return 0.0f;
}

//	===	アニメーションが読み込まれているか確認	===
bool Model::HasAnimation(const std::string& animationName) const
{
	return m_animations.find(animationName) != m_animations.end();
}

//	===	ボーン変換行列を計算	===
void Model::CalculateBoneTransforms(
	const std::string& animationName,
	float animationTime,
	std::vector<DirectX::XMMATRIX>& outTransforms)
{
	//	========================================================================
	//	アニメーションを検索
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

	//	配列のサイズを確保
	outTransforms.resize(m_bones.size());

	//	全ての行列を単位行列で初期化
	for (size_t i = 0; i < outTransforms.size(); i++)
	{
		outTransforms[i] = DirectX::XMMatrixIdentity();
	}

	//	========================================================================
	//	ステップ1: 各ボーンのローカル変換を計算
	//	========================================================================
	std::vector<DirectX::XMMATRIX> localTransforms(m_bones.size());

	for (size_t i = 0; i < m_bones.size(); i++)
	{
		localTransforms[i] = DirectX::XMMatrixIdentity();
	}

	//	チャンネル（ボーン）ごとにループ
	for (const auto& channel : clip.channels)
	{
		//	このチャンネルのボーンを探す
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
		//	キーフレーム検索
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
		//	補間係数を計算
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
		//	位置の補間（線形補間）
		//	====================================================================
		DirectX::XMFLOAT3 pos0 = channel.keyframes[frame0].position;
		DirectX::XMFLOAT3 pos1 = channel.keyframes[frame1].position;
		DirectX::XMVECTOR position0 = DirectX::XMLoadFloat3(&pos0);
		DirectX::XMVECTOR position1 = DirectX::XMLoadFloat3(&pos1);
		DirectX::XMVECTOR position = DirectX::XMVectorLerp(position0, position1, t);

		//	====================================================================
		//	回転の補間（球面線形補間）
		//	====================================================================
		DirectX::XMFLOAT4 rot0 = channel.keyframes[frame0].rotation;
		DirectX::XMFLOAT4 rot1 = channel.keyframes[frame1].rotation;
		DirectX::XMVECTOR rotation0 = DirectX::XMLoadFloat4(&rot0);
		DirectX::XMVECTOR rotation1 = DirectX::XMLoadFloat4(&rot1);
		DirectX::XMVECTOR rotation = DirectX::XMQuaternionSlerp(rotation0, rotation1, t);

		//	====================================================================
		//	スケールの補間（線形補間）
		//	====================================================================
		DirectX::XMFLOAT3 scl0 = channel.keyframes[frame0].scale;
		DirectX::XMFLOAT3 scl1 = channel.keyframes[frame1].scale;
		DirectX::XMVECTOR scale0 = DirectX::XMLoadFloat3(&scl0);
		DirectX::XMVECTOR scale1 = DirectX::XMLoadFloat3(&scl1);
		DirectX::XMVECTOR scale = DirectX::XMVectorLerp(scale0, scale1, t);

		//	====================================================================
		//	変換行列を作成
		//	====================================================================
		DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScalingFromVector(scale);
		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationQuaternion(rotation);
		DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(position);

		//	ローカル変換を保存
		localTransforms[boneIndex] = scaleMatrix * rotationMatrix * translationMatrix;
	}

	//	========================================================================
	//	ステップ2: 親子関係を考慮してワールド変換を計算
	//	========================================================================
	//	【重要】親から順に計算する必要がある

	for (size_t i = 0; i < m_bones.size(); i++)
	{
		if (m_bones[i].parentIndex == -1)
		{
			//	親がいない（ルートボーン）
			outTransforms[i] = localTransforms[i];
		}
		else
		{
			//	親がいる
			int parentIndex = m_bones[i].parentIndex;
			outTransforms[i] = localTransforms[i] * outTransforms[parentIndex];
		}
	}

	//	========================================================================
	//	ステップ3: オフセット行列を適用
	//	========================================================================
	for (size_t i = 0; i < outTransforms.size(); i++)
	{
		outTransforms[i] = m_bones[i].offsetMatrix * outTransforms[i];
	}

	//	========================================================================
	//	デバッグ出力（最初のフレームのみ）
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

		// 最初の5ボーンの親子関係を表示
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
	//	各ボーンの親を探してparentIndexに設定

	if (!scene->mRootNode)
		return;

	//OutputDebugStringA("===	Building Bone Hierarchy	===\n");

	//	---	ボーン名→インデックスマップを作製
	std::map<std::string, int> boneNameToIndex;
	for (size_t i = 0; i < m_bones.size(); i++)
	{
		boneNameToIndex[m_bones[i].name] = static_cast<int>(i);
	}

	//	---	ノード改装を辿って親子関係を設定	---
	std::function<void(aiNode*, int)> processNode = [&](aiNode* node, int parentIndex)
		{
			//	このノードがボーンか確認
			std::string nodeName = node->mName.C_Str();
			auto it = boneNameToIndex.find(nodeName);

			int currentIndex = -1;
			if (it != boneNameToIndex.end())
			{
				//	ボーンが見つかった
				currentIndex = it->second;
				m_bones[currentIndex].parentIndex = parentIndex;

				////	デバッグ出力
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

			//	子ノードを再帰的に処理
			//	親のインデックスを渡す
			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				processNode(node->mChildren[i], currentIndex != -1 ? currentIndex : parentIndex);
			}
		};

	//	ルートノードから開始
	processNode(scene->mRootNode, -1);

	//OutputDebugStringA("=== Bone Hierarchy Complete ===\n");
}

void Model::ReadNodeHierarchy(const aiNode* srcNode, int parentIndex)
{
	Node node;
	node.name = srcNode->mName.C_Str();
	node.parentIndex = parentIndex;

	// Assimp行列 -> DirectX行列
	aiMatrix4x4 t = srcNode->mTransformation;
	node.transformation = DirectX::XMMATRIX(
		t.a1, t.b1, t.c1, t.d1,
		t.a2, t.b2, t.c2, t.d2,
		t.a3, t.b3, t.c3, t.d3,
		t.a4, t.b4, t.c4, t.d4
	);

	// このノード名に対応するボーンがあるか探す
	// (高速化のため本来はmapを使うべきですが、今は既存のm_bonesループで対応)
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

	// ノードリストに追加
	int currentIndex = static_cast<int>(m_nodes.size());
	m_nodes.push_back(node);

	// 親の子リストに自分を追加
	if (parentIndex != -1) {
		m_nodes[parentIndex].children.push_back(currentIndex);
	}
	else {
		m_rootNodeIndex = currentIndex;
	}

	// 子ノードを再帰処理
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

	//	基本はこのノードの初期変換行列
	DirectX::XMMATRIX nodeTransform = node.transformation;

	//	アニメーションデータを探す
	const AnimationChannel* channel = nullptr;
	std::string shortNodeName = GetShortName(node.name);
	for (const auto& c : clip.channels) {
		if (GetShortName(c.boneName) == shortNodeName) {
			channel = &c;
			break;
		}
	}

	// ★ デバッグ：対応関係を一度だけ出す
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
	//	// 1フレーム分出したら止めたいときは true にしてもOK
	//	// sPrinted = true;
	//}


	//	アニメーションがあれば、行列を計算して上書き
	if (channel && !channel->keyframes.empty()) {
		//	---	キーフレーム検索	---
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
			//	終端チェック
			if (frame1 == 0 && channel->keyframes.size() > 1) {
				frame0 = frame1 = channel->keyframes.size() - 1;
			}
		}

		//	---	補間係数 t	---
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

		//	---	補間計算 (Lerp / Slerp)	---
		//	位置
		DirectX::XMFLOAT3 pos0 = channel->keyframes[frame0].position;
		DirectX::XMFLOAT3 pos1 = channel->keyframes[frame1].position;
		DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&pos0);
		DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&pos1);
		DirectX::XMVECTOR position = DirectX::XMVectorLerp(p0, p1, t);

		//	回転
		DirectX::XMFLOAT4 rot0 = channel->keyframes[frame0].rotation;
		DirectX::XMFLOAT4 rot1 = channel->keyframes[frame1].rotation;
		DirectX::XMVECTOR r0 = DirectX::XMLoadFloat4(&rot0);
		DirectX::XMVECTOR r1 = DirectX::XMLoadFloat4(&rot1);
		DirectX::XMVECTOR rotation = DirectX::XMQuaternionSlerp(r0, r1, t);

		//	スケール
		DirectX::XMFLOAT3 scl0 = channel->keyframes[frame0].scale;
		DirectX::XMFLOAT3 scl1 = channel->keyframes[frame1].scale;
		DirectX::XMVECTOR s0 = DirectX::XMLoadFloat3(&scl0);
		DirectX::XMVECTOR s1 = DirectX::XMLoadFloat3(&scl1);
		DirectX::XMVECTOR scale = DirectX::XMVectorLerp(s0, s1, t);

		//	--- 行列合成 ---
		DirectX::XMMATRIX scaleM = DirectX::XMMatrixScalingFromVector(scale);

		//	===============================
		//	===	ボーンスケールの強制適用	===
		//	===============================
		std::string shortName = GetShortName(node.name);
		if (m_boneScales.find(shortName) != m_boneScales.end())
		{
			float s = m_boneScales[shortName];
			//	アニメーションのスケールを無視して、指定したスケールで上書きする
			scaleM = DirectX::XMMatrixScaling(s, s, s);
		}


		DirectX::XMMATRIX rotM = DirectX::XMMatrixRotationQuaternion(rotation);
		DirectX::XMMATRIX transM = DirectX::XMMatrixTranslationFromVector(position);

		//	ノード行列をアニメーション姿勢で上書き
		nodeTransform = scaleM * rotM * transM;
	}

	//	グローバル変換行列 (親 * 自分)
	DirectX::XMMATRIX globalTransform = nodeTransform * parentTransform;

	//	ボーンなら最終行列を格納
	if (node.boneIndex != -1) {
		// オフセット * グローバル * グローバル逆変換(原点補正)
		outTransforms[node.boneIndex] =
			m_bones[node.boneIndex].offsetMatrix * globalTransform * m_globalInverseTransform;
	}

	//	子ノードへ再帰
	for (int childIndex : node.children) {
		UpdateNodeTransforms(childIndex, clip, animationTime, globalTransform, outTransforms);
	}
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
	
	//	ステップ1: アニメーションがあるか確認
	auto it = m_animations.find(animationName);
	if (it == m_animations.end())
	{
		//	アニメーションが見つからない場合は通常描画
		/*OutputDebugStringA("Model::DrawAnimated - Animation not found: ");
		OutputDebugStringA(animationName.c_str());
		OutputDebugStringA("\n");
		Draw(context, world, view, projection, color);*/
		return;
	}

	const AnimationClip& clip = it->second;


	//	全ての行列を単位行列で初期化
	std::vector<DirectX::XMMATRIX> boneTransforms;
	boneTransforms.resize(m_bones.size());

	//	全ての行列を単位行列で初期化
	for (size_t i = 0; i < boneTransforms.size(); i++)
	{
		boneTransforms[i] = DirectX::XMMatrixIdentity();
	}

	//	ノード階層を使ってボーン変換を計算
	if (!m_nodes.empty())
	{
		//	ここが肝心！UpdateNodeTransforms を呼ぶ
		UpdateNodeTransforms(
			m_rootNodeIndex,
			clip,
			animationTime,
			DirectX::XMMatrixIdentity(),
			boneTransforms
		);
	}


	//	ステップ3: ボーン行列をエフェクトに設定
	//	最大72個のボーン（DirectXTK の SkinnedEffect の制限）
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

	//	エフェクトにボーン行列を設定
	m_effect->SetBoneTransforms(finalBones, DirectX::SkinnedEffect::MaxBones);


	//	エフェクトに行列を設定
	m_effect->SetWorld(world);
	m_effect->SetView(view);
	m_effect->SetProjection(projection);

	//	ライトの設定
	m_effect->EnableDefaultLighting();
	m_effect->SetAmbientLightColor(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));
	m_effect->SetDiffuseColor(color);

	//	エフェクト適用
	m_effect->Apply(context);

	//	入力レイアウトを設定
	context->IASetInputLayout(m_inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	各メッシュを描画
	for (const auto& mesh : m_meshes)
	{
		//	頂点バッファを設定
		UINT stride = sizeof(ModelVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);

		//	インデックスバッファを設定
		context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//	描画命令
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
	//	===	アニメーションが存在しているか確認	===
	auto it = m_animations.find(animationName);
	if (it == m_animations.end())
	{/*
		OutputDebugStringA("Model::DrawInstanced - Animation not found: ");
		OutputDebugStringA(animationName.c_str());
		OutputDebugStringA("\n");*/
		return;
	}

	const AnimationClip& clip = it->second;


	//	===	ボーン変換行列絵を計算	===
	std::vector<DirectX::XMMATRIX> boneTransforms;
	boneTransforms.resize(m_bones.size());	//	ボーンの数だけ配列を確保

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


	//	===	ボーン行列をエフェクトに設定	===
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

	m_effect->SetBoneTransforms(finalBones, DirectX::SkinnedEffect::MaxBones);	//	ボーン行列を送る


	//	===	ビュー・プロジェクション行列を設定	===
	m_effect->SetView(view);	//	カメラの設定を送る
	m_effect->SetProjection(projection);	//	投影を設定

	//	===	ライトの設定	===
	m_effect->EnableDefaultLighting();
	m_effect->SetAmbientLightColor(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));


	//	===	入力レイアウト設定	===
	context->IASetInputLayout(m_inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//	===	各インスタンスを描画	===
	for (const auto& instance : instances)
	{
		//	---	ワールド行列を設定	---
		DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&instance.world);
		m_effect->SetWorld(world);

		//	---	色を設定	---
		DirectX::XMVECTOR color = DirectX::XMLoadFloat4(&instance.color);
		m_effect->SetDiffuseColor(color);

		//	---	エフェクト適用	---
		m_effect->Apply(context);
		
		// デバッグログ追加
		/*char debugDraw[256];
		sprintf_s(debugDraw, "[MODEL] DrawInstanced called, instances=%zu\n", instances.size());
		OutputDebugStringA(debugDraw);*/

		// 深度書き込みを強制有効化
		if (m_states)
		{
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			//OutputDebugStringA("[MODEL] Depth state set\n");
		}
		else
		{
			//OutputDebugStringA("[MODEL] ERROR: m_states is null!\n");
		}
		//	---	各メッシュを描画	---
		for (const auto& mesh : m_meshes)
		{
			UINT stride = sizeof(ModelVertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
		}
	}

	////	デバッグ出力
	//char debug[256];
	//sprintf_s(debug, "DrawInstanced: %zu instances, anim=%s, time=%.2f\n",
	//	instances.size(), animationName.c_str(), animationTime);
	//OutputDebugStringA(debug);

}


//	===	マップに値を登録するだけの処理	===
void Model::SetBoneScale(const std::string& boneName, float scale)
{
	//	指定されたボーン名をキーにして、スケール値を保存
	m_boneScales[boneName] = scale;
}

void Model::SetTexture(ID3D11ShaderResourceView* texture)
{
	m_diffuseTexture = texture;

	// SkinnedEffectにテクスチャを設定
	if (m_effect && texture)
	{
		m_effect->SetTexture(texture);
	}
}

// ボーン名を出力
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
	// ボーン行列を準備（すべてIdentity = 元のDrawと同じ）
	DirectX::XMMATRIX finalBones[DirectX::SkinnedEffect::MaxBones];
	for (int i = 0; i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		finalBones[i] = DirectX::XMMatrixIdentity();
	}

	// スケール0のボーンだけ特別処理（非表示にする）
	for (size_t i = 0; i < m_bones.size() && i < DirectX::SkinnedEffect::MaxBones; i++)
	{
		std::string shortName = GetShortName(m_bones[i].name);
		if (m_boneScales.find(shortName) != m_boneScales.end())
		{
			float s = m_boneScales[shortName];
			if (s == 0.0f)
			{
				// スケール0 → 頂点を見えなくする
				finalBones[i] = DirectX::XMMatrixScaling(0.0f, 0.0f, 0.0f);
			}
		}
	}

	m_effect->SetBoneTransforms(finalBones, DirectX::SkinnedEffect::MaxBones);

	// === 以下は元のDrawと同じ ===
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
		// プレフィックスで始まるボーンを全て設定
		if (shortName.rfind(prefix, 0) == 0)  // starts_with
		{
			m_boneScales[shortName] = scale;
		}
	}
}