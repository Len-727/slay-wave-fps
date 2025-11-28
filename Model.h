//	Model.h	3Dモデル読み込み・描画システム

#pragma once

#include <d3d11.h>			//	GPUにデータを送る		ID3D11Device, Id3D11DeviceContext, ID3D11Buffer 等
#include <DirectXMath.h>	//	3Dモデルの位置・回転・スケールを計算する
#include <wrl/client.h>		//	メモリリークを防ぐ
#include <vector>			//	頂点データやメッシュを保存するため
#include <string>			//	ファイルパスを扱う
#include <memory>			//	BasicEffect　等のオブジェクトを自動管理するため
#include <map>				//	アニメーション名→データのマップ
#include <Effects.h>
#include <CommonStates.h>

#include "Entities.h"

struct aiScene;
struct aiNode;


//	===	頂点データ構造体	===
struct ModelVertex {
	DirectX::XMFLOAT3 position;	//	位置(x, y, z)
	DirectX::XMFLOAT3 normal;	//	法線(光の計算用) - 面の向き
	DirectX::XMFLOAT2 texCoord;	//	テクスチャ座標(u, v) - 画像の貼り付け位置

	//	---	ボーンスキニング	---
	uint32_t boneIndices[4];	//	ぼーんインデックス
	float boneWeights[4];	//	ボーンウェイト


	//	コンストラクタ
	ModelVertex()
		: position(0, 0, 0)
		, normal(0, 1, 0)
		, texCoord(0, 0)
		, boneIndices{ 0, 0, 0, 0 }
		, boneWeights{ 0.0f, 0.0f, 0.0f, 0.0f }
	{

	}
};

//	===	階層構造を保持するためのノード	===
struct Node
{
	std::string name;
	DirectX::XMMATRIX transformation;	//	初期変換行列
	int parentIndex;	
	std::vector<int> children;

	int boneIndex = -1;
};


//	===	メッシュ構造体 ===
//	【意味】3Dモデルの一部分(体、頭、武器など)
struct Mesh {
	std::vector<ModelVertex> vertices;	//	頂点データ(三角形の角の部分)
	std::vector<uint32_t> indices;		//	インデックス(頂点の接続順序)

	//	DirectXのGPUリリース
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;	//	GPUの頂点データ
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;	//	GPU上のindexデータ
};


//	===	3Dモデルクラス ===
class Model {
public:

	//	コンストラクタ
	Model();

	//	FBX/OBJ　等のファイルを読み込む
	//	【引数】device: DirectX デバイス (GPU　を操作するオブジェクト)
	//			filename: ファイルパス
	//	【戻り値】成功 true, 失敗 false
	bool LoadFromFile(ID3D11Device* device, const std::string& filename);

	//	モデル描画
	//	【引数】context: DirectX　デバイスコンテキスト(描画命令を送るオブジェクト)
	//			world: ワールド変換行列（位置・回転・スケール）
	//			view: ビュー行列（カメラの位置・向き）
	//			projection: プロジェクション行列（投資投影）
	//			color: 色（RGBA）
	void Draw(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMVECTOR color);

	size_t GetMeshCount() const { return m_meshes.size(); }

	//	FBXファイルからアニメーションを読み込む
	bool LoadAnimation(const std::string& filename, const std::string& animationName);

	//	アニメーション付きで描画
	void DrawAnimated(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMVECTOR color,
		const std::string& animationName,
		float animationTime);

	//	アニメーションの長さを取得
	//	アニメーションのループ判定に使う
	float GetAnimationDuration(const std::string& animationName) const;

	//	アニメーションが読み込まれているか確認
	bool HasAnimation(const std::string& animationName) const;

private:

	std::vector<Node> m_nodes;	//	全ノードリスト
	int m_rootNodeIndex = 0;	//	ルートノードのインデックス

	//	グローバルを逆変換行列
	DirectX::XMMATRIX m_globalInverseTransform;

	//	再帰的にノードを更新するヘルパー
	void ReadNodeHierarchy(const aiNode* srcNode, int parentIndex);

	//	再帰的に行列を更新するヘルパー
	void UpdateNodeTransforms(
		int nodeIndex,
		const AnimationClip& clip,
		float animationTime,
		DirectX::XMMATRIX parentTransform,
		std::vector<DirectX::XMMATRIX>& outTransforms
	);

	//	メッシュデータ(複数のメッシュを捨てる)
	std::vector<Mesh> m_meshes;

	//	DirectXエッフェクト(シェーダー)
	std::unique_ptr<DirectX::SkinnedEffect> m_effect;				//	描画エッフェクト
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;	//	頂点データの構造定義

	//	ボーン構造
	std::vector<Bone> m_bones;

	//	アニメーションクリップのマップ
	//	複数のアニメーションを名前で管理
	std::map<std::string, AnimationClip> m_animations;

	//	頂点バッファ・インデックスバッファを作成
	//	【役割】GPUのデータを　GPU に送信
	bool CreateBuffers(ID3D11Device* device, Mesh& mesh);

	//	ボーン変換行列を計算
	//	アニメーション時刻に対応するボーン行列を計算
	void CalculateBoneTransforms(
		const std::string& animationName,
		float animationTime,
		std::vector<DirectX::XMMATRIX>& outTransforms
	);

	void BuildBoneHierarchy(const aiScene* scene);

	void DebugDumpSkeleton();

	//std::string GetShortName(const std::string& fullName);
};

