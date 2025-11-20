//	Model.cpp
#include "Model.h"
#include <DirectXColors.h>
#include <assimp/Importer.hpp>	//	Assimp　のインポーター	(FBX, OBJ)
#include <assimp/scene.h>		//	読み込んだシーン(ノード階層・メッシュ・マテリアル・アニメーション)にアクセスする
#include <assimp/postprocess.h>	//	三角形化・法線/接戦生成・頂点結合などの後処理フラグ
#include <CommonStates.h>
#include <Effects.h>
#include <VertexTypes.h> // DirectXTKの標準頂点型定義

#pragma comment(lib, "assimp-vc143-mt.lib")

using namespace DirectX;
//using namespace Colors;

Model::Model()
{
}

//	ファイルから3Dモデルを読み込む
bool Model::LoadFromFile(ID3D11Device* device, const std::string& filename)
{
	//	=== Assimp ファイル読み込み ===
	Assimp::Importer importer;

	//	ファイルを読み込む
	//	【フラグ】どう処理するか指定
	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_Triangulate |			//	全て三角形に変換
		aiProcess_ConvertToLeftHanded |	//	DirectX用に左手座標系に
		aiProcess_GenNormals |			//	法線を自動生成
		aiProcess_CalcTangentSpace |	    //	タンジェント計算
		aiProcess_FlipUVs               // ★UV座標を反転 (テクスチャが上下逆になるのを修正。ディフューズカラーには無関係)
		//aiProcess_FlipNormals             // 
	);

	//	読み込み失敗チェック
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		//	エラーメッセージを出力
		OutputDebugStringA("Model::LoadFromFIle - Failed to load: ");
		OutputDebugStringA(filename.c_str());
		OutputDebugStringA("\n");
		OutputDebugStringA(importer.GetErrorString());
		OutputDebugStringA("\n");
		return false;
	}

	//	=== DirectXTK のエッフェクト作成 ===
	//	BasicEffectを"ライティング用に設定"
	m_effect = std::make_unique<BasicEffect>(device);	//	DirectXTKの基本シェーダ(描画効果)を作成
	m_effect->SetVertexColorEnabled(false);	//	法線カラー無効	色は"頂点の色"ではなく、"マテリアル色やテクスチャで決める"
	m_effect->SetLightingEnabled(true);		//	ライティング有効 頂点の法線と設定したライトを使って明るさを計算する
	m_effect->SetTextureEnabled(false);      //  テクスチャを有効化 (FBX利用のため追加)


	//	入力レイアウトを作成
	//	BasicEffectが内部で使う頂点シェーダのバイトコードを取り出す
	void const* shaderByteCode;
	size_t byteCodeLength;
	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);


	//	頂点フォーマットの定義
	//	【ここが修正点】DirectXTKの標準頂点レイアウトを使用
	HRESULT hr = device->CreateInputLayout(
		DirectX::VertexPositionNormalTexture::InputElements, // POSITION, NORMAL, TEXCOORD の定義
		DirectX::VertexPositionNormalTexture::InputElementCount, // 要素数 3
		shaderByteCode, byteCodeLength,	//	VSバイトコード&サイズ(入力シグネチャ検証に必須)
		m_inputLayout.GetAddressOf()	//	生成されたID3D11InputLayout* がここに帰る
	);

	//	失敗判定のマクロ	失敗したらtrue
	if (FAILED(hr))
	{
		OutputDebugStringA("Model::LoadFromFIle - Filed to create input layout\n");
		return false;
	}

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
			OutputDebugStringA("Model::LoadFromFile - Failed to create buffers\n");
			return false;
		}

		//	メッシュを保存
		m_meshes.push_back(std::move(mesh));
	}

	//	デバッグ出力
	char debug[256];
	sprintf_s(debug, "Model::LoadFromFIle = Success: %s (%zu meshes)\n",
		filename.c_str(), m_meshes.size());
	OutputDebugStringA(debug);

	return true;

}



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

	//	===	ライトの設定	===
	m_effect->SetLightEnabled(0, true);
	m_effect->SetLightDiffuseColor(0, Colors::White);
	m_effect->SetLightDirection(0, XMVectorSet(0, -1, 1, 0));

	//	環境工設定
	m_effect->SetAmbientLightColor(XMVectorSet(0.3f, 0.3f, 0.3f, 1.0f));
	//	ディフューズカラーはモデル全体の色と光の反射色を兼ねる
	m_effect->SetDiffuseColor(color);

	//	===	エフェクト適用	===
	m_effect->Apply(context);


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