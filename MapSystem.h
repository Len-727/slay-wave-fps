//	MapSystem.h
//	【役割】
// ゲームの3Dマップ
// プリミティブを使って簡易的なマップを作製
// 将来的にはBlenderで作った3Dモデルに置き換え
//
#pragma once
#include <d3d11.h>			//	レンダリングの土台　デバイス生成、スワップチェイン、バッファ/テクスチャ作成
#include <DirectXMath.h>	//	SIMD XMVECTOR/XMMATRIX で位置・回転・拡縮・投影など
#include <DirectXColors.h>	//	色
#include <wrl/client.h>		//	ComPtrスマートポインタ　参照カウントを自動管理
#include <memory>			//	C++スマートポインタ
#include <vector>
#include <GeometricPrimitive.h>
#include <Effects.h>
#include <CommonStates.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

//	マップオブジェクトの種類
enum class MapObjectType
{
	FLOOR,		//	床
	WALL,		//	壁
	BOX,		//	箱
	CYLINDER	//	柱
};

//	マップオブジェクト
struct MapObject
{
	MapObjectType type;	//	種類
	XMFLOAT3 position;	//	位置
	XMFLOAT3 scale;		//	スケール(幅、高さ, 奥行)x,y,z
	XMFLOAT3 rotation;	//	回転(ラジアン)
	XMFLOAT4 color;		//	色(RGBA)
	bool hasCollision;	//	当たり判定
};

//	マップ管理システム
class MapSystem
{
public:
	//	===	コンストラクタ・デストラクタ	===
	MapSystem();			//	コンストラクタ(初期化)
	~MapSystem() = default;	//	デストラクタ(自動でメモリを解放)

	// Initialize - 初期化
	bool Initialize(ID3D11DeviceContext* context, ID3D11Device* device);

	// Draw - マップ全体を描画
	void Draw(ID3D11DeviceContext* context,
				XMMATRIX view,
				XMMATRIX projection);

	//	AddFloor	- 床を描画
	void AddFloor(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color = { 0.3f, 0.3f, 0.3f, 1.0f });
	//	AddWall		- 壁を描画
	void AddWall(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color = { 0.5f, 0.5f, 0.5f, 1.0f });
	//	AddBox		- 箱を追加
	void AddBox(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 cplor = { 0.6f, 0.4f, 0.2f, 1.0f });

	//	CheckCollision	- 当たり判定チェック
	bool CheckCollision(XMFLOAT3 position, float radius);


	//	CreateDefaultMap	- デフォルトマップ作成
	void CreateDefaultMap();

	//	GetObjects			- オブジェクトリストを取得(デバッグ用)
	const std::vector<MapObject>& GetObjects() const { return m_objects; }


private:
	//	===	メンバ変数	===

	//	m_objects	- マップオブジェクトのリスト
	std::vector<MapObject> m_objects;

	//	m_box		- Box プリミティブ(立方体)
	std::unique_ptr<GeometricPrimitive> m_box;

	//	m_cylinder	- Cylinder プリミティブ(円柱)
	std::unique_ptr<GeometricPrimitive> m_cylinder;


	std::unique_ptr<DirectX::CommonStates> m_states;
};