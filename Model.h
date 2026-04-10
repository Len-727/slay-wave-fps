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
#include "InstanceData.h"

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

	// メッシュデータへのアクセス（切断システム用）
	const std::vector<Mesh>& GetMeshes() const { return m_meshes; }

	// テクスチャの取得
	ID3D11ShaderResourceView* GetTexture() const { return m_diffuseTexture.Get(); }

	//	FBXファイルからアニメーションを読み込む
	bool LoadAnimation(const std::string& filename, const std::string& animationName);

	void DrawAnimated_Legacy(
		ID3D11DeviceContext* context,
		DirectX::XMMATRIX world, DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection, DirectX::XMVECTOR color,
		const std::string& animationName, float animationTime);

	//	アニメーション付きで描画
	void DrawAnimated(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMVECTOR color,
		const std::string& animationName,
		float animationTime);

	// ボーンスケール適用付き描画（アニメーションなし）
	void DrawWithBoneScale(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMVECTOR color);

	//	インスタンシング描画
	void DrawInstanced(
		ID3D11DeviceContext* context,
		const std::vector<InstanceData>& instances,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		const std::string& animationName,
		float animationTime
	);

	//	アニメーションの長さを取得
	//	アニメーションのループ判定に使う
	float GetAnimationDuration(const std::string& animationName) const;

	//	アニメーションが読み込まれているか確認
	bool HasAnimation(const std::string& animationName) const;

	//	特定のボーンのスケールを変更する
	void SetBoneScale(const std::string& boneName, float scale);

	void SetBoneScaleByPrefix(const std::string& prefix, float scale);

	void PrintBoneNames() const;

	//	テクスチャを設定
	void SetTexture(ID3D11ShaderResourceView* texture);

	// カスタムシェーダーを読み込む
	bool LoadCustomShaders(ID3D11Device* device);


	// インスタンシング用バッファを作成
	bool CreateInstanceBuffers(ID3D11Device* device, int maxInstances);

	// インスタンシング描画（同じモデルの敵をまとめて描画）
	void DrawInstanced_Custom(
		ID3D11DeviceContext* context,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		const std::vector<DirectX::XMMATRIX>& worlds,
		const std::vector<DirectX::XMVECTOR>& colors,
		const std::vector<std::string>& animNames,
		const std::vector<float>& animTimes
	);

	// バインドポーズの頂点座標を取得（切断システム用）
	// アニメーションの最初のフレームを使って頂点を展開する
	std::vector<ModelVertex> GetBindPoseVertices(int meshIndex);

	// アニメーションポーズ適用済みの頂点を取得（切断用）
	std::vector<ModelVertex> GetAnimatedVertices(
		int meshIndex,
		const std::string& animName,
		float animTime);

private:

	std::vector<DirectX::XMMATRIX> m_cachedBoneTransforms;  //  事前確保キャッシュ

	// === インスタンシング用 ===
	struct InstanceGPU {
		DirectX::XMMATRIX World;       // 64 bytes
		DirectX::XMFLOAT4 Color;       // 16 bytes
		uint32_t BoneOffset;           // 4 bytes
		uint32_t pad[3];               // 12 bytes (16バイト境界に揃える)
	};  // 合計 96 bytes

	struct FrameCB {
		DirectX::XMMATRIX View;
		DirectX::XMMATRIX Projection;
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer>              m_instanceBuffer;     // インスタンスバッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer>              m_boneStructuredBuf;  // StructuredBuffer
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_boneSRV;           // ↑のSRV
	Microsoft::WRL::ComPtr<ID3D11Buffer>              m_frameCB;           // View/Proj定数バッファ
	int m_maxInstances = 0;
	int m_maxBoneEntries = 0;

	// === カスタムシェーダー ===
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_customVS;        // 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_customPS;        // ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_customInputLayout; // 頂点データの構造定義
	Microsoft::WRL::ComPtr<ID3D11Buffer>        m_transformCB;     // 定数バッファ: Transform
	Microsoft::WRL::ComPtr<ID3D11Buffer>        m_boneCB;          // 定数バッファ: Bones
	Microsoft::WRL::ComPtr<ID3D11Buffer>        m_lightCB;         // 定数バッファ: Light
	Microsoft::WRL::ComPtr<ID3D11SamplerState>  m_customSampler;   // テクスチャサンプラー
	bool m_useCustomShader = false;  // カスタムシェーダーを使うか

	// 定数バッファの構造体（HLSLと完全に一致させる）
	struct TransformCB {
		DirectX::XMMATRIX World;
		DirectX::XMMATRIX View;
		DirectX::XMMATRIX Projection;
	};

	struct BoneCB {
		DirectX::XMMATRIX Bones[72];  // SkinnedEffect::MaxBones と同じ
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

	std::vector<Node> m_nodes;	//	全ノードリスト
	int m_rootNodeIndex = 0;	//	ルートノードのインデックス

	//	ボーン名とスケールのマップ
	std::map<std::string, float> m_boneScales;


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

	//	元のオフセット行列を保存
	std::vector<DirectX::XMMATRIX> m_originalOffsetMatrices;

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

	//void DebugDumpSkeleton();

	std::string GetShortName(const std::string& fullName);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_diffuseTexture;
};

