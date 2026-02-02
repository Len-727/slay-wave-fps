//MapSystem.cpp

#include "MapSystem.h"

//	===	コンストラクト	===
MapSystem::MapSystem()
{

}

//	===	初期化	===
bool MapSystem::Initialize(ID3D11DeviceContext* context, ID3D11Device* device)
{
	//	===	Box	===
	m_box = GeometricPrimitive::CreateBox(
		context,
		XMFLOAT3(1.0f, 1.0f, 1.0f),	//	サイズ
		true,						//	右手座標系
		false						//	法線反転なし
	);
	//	=== Cylinder	==
	m_cylinder = GeometricPrimitive::CreateCylinder(
		context,
		1.0f,	//	高さ
		1.0f,	//	直径
		32,		//	分割数
		true	//	右手座標系
	);

	m_states = std::make_unique<DirectX::CommonStates>(device);

	OutputDebugStringA("MapSystem::Initialize - Success\n");

	return true;
}

void MapSystem::Draw(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection)
{
	//	===	全オブジェクトをループで描画	===
	for (const auto& obj : m_objects)
	{
		//	===	ワールド行列の作成

		//	スケール行列
		XMMATRIX scale = XMMatrixScaling(obj.scale.x, obj.scale.y, obj.scale.z);
		// 回転行列
		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
			obj.rotation.x,
			obj.rotation.y,
			obj.rotation.z
		);
		//	位置行列
		XMMATRIX translation = XMMatrixTranslation(
			obj.position.x,
			obj.position.y,
			obj.position.z
		);

		//	===	合成	===
		XMMATRIX world = scale * rotation * translation;

		//	色の設定
		XMVECTOR color = XMLoadFloat4(&obj.color);

		//	===	種類に応じて描画
		switch (obj.type)
		{
		case MapObjectType::FLOOR:	//	床
		case MapObjectType::WALL:	//	壁
		case MapObjectType::BOX:	//	箱
			
			//	Box プリミティブで描画
			m_box->Draw(world, view, projection, color);
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			break;

		case MapObjectType::CYLINDER:	//	柱
			m_cylinder->Draw(world, view, projection, color);
			context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
			break;


		}
	}
}

//	AddFloor	- 床を追加
//	【役割】床オブジェクトをm_objects配列に追加
void MapSystem::AddFloor(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color)
{
	MapObject obj;

	obj.type = MapObjectType::FLOOR;	//	種類　床
	obj.position = position;
	obj.scale = scale;
	obj.rotation = XMFLOAT3(0, 0, 0);
	obj.color = color;
	obj.hasCollision = false;

	//	m_objects 配列の末尾に追加
	m_objects.push_back(obj);
}
//	===	AddWall	- 壁を追加===
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
//	===	AddBox	- 箱		===
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

//	===	CheckCollisiton	- 当たり判定チェック	===
bool MapSystem::CheckCollision(XMFLOAT3 position, float radius)
{
	//	===	全オブジェクトをチェック
	for (const auto& obj : m_objects)
	{
		//	---	当たり判定がないオブジェクトはスキップ	---
		if (!obj.hasCollision)
			continue;

		//	AABB 当たり判定
		float dx = abs(position.x - obj.position.x);
		float dz = abs(position.z - obj.position.z);
		// --- オブジェクトの半分のサイズを計算 ---
		float halfWidth = obj.scale.x / 2.0f;
		float halfDepth = obj.scale.z / 2.0f;

		// === デバッグ出力	===
		/*char debug[512];
		sprintf_s(debug, "CheckCollision: Player(%.1f, %.1f, %.1f) vs Object(%.1f, %.1f, %.1f) "
			"dx=%.2f dz=%.2f halfW=%.2f halfD=%.2f radius=%.2f\n",
			position.x, position.y, position.z,
			obj.position.x, obj.position.y, obj.position.z,
			dx, dz, halfWidth, halfDepth, radius);
		OutputDebugStringA(debug);*/

		if (dx < (halfWidth + radius) && dz < (halfDepth + radius))
		{
			OutputDebugStringA(">>> HIT!\n");
			return true;
		}
	}

	return false;
}

//	===	CreateDefaultMap	- デフォルトマップ	===
void MapSystem::CreateDefaultMap()
{
	//	===	既存のオブジェクトをクリア	===
	//	前回のマップが残っていたら削除
	m_objects.clear();

	//	===	床生成	===
	AddFloor(
		XMFLOAT3(0, -0.05f, 0),
		XMFLOAT3(50, 0.1f, 50),
		XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f)
	);


	//	===	外壁作成	===
	//	奥
	AddWall(
		XMFLOAT3(0, 1, -25),
		XMFLOAT3(50, 4, 1),
		XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f)
	);
	//	手前
	AddWall(
		XMFLOAT3(0, 1, 25),
		XMFLOAT3(50, 4, 1),
		XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f)
	);
	//	左
	AddWall(
		XMFLOAT3(-25, 1, 0),
		XMFLOAT3(1, 4, 50),
		XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f)
	);
	//	右
	AddWall(
		XMFLOAT3(25, 1, 0),
		XMFLOAT3(1, 4, 50),
		XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f)
	);


	//	===	箱	===
	// --- 左上の箱 ---
	AddBox(
		XMFLOAT3(-15, 1, -15),               // 位置（左上）
		XMFLOAT3(3, 2, 3),                   // サイズ（幅3, 高さ2, 奥行き3）
		XMFLOAT4(0.6f, 0.3f, 0.1f, 1.0f)    // 茶色
	);
	// --- 右上の箱 ---
	AddBox(
		XMFLOAT3(15, 1, -15),
		XMFLOAT3(3, 2, 3),
		XMFLOAT4(0.6f, 0.3f, 0.1f, 1.0f)
	);
	// --- 左下の箱 ---
	AddBox(
		XMFLOAT3(-15, 1, 15),
		XMFLOAT3(3, 2, 3),
		XMFLOAT4(0.6f, 0.3f, 0.1f, 1.0f)
	);
	// --- 右下の箱 ---
	AddBox(
		XMFLOAT3(15, 1, 15),
		XMFLOAT3(3, 2, 3),
		XMFLOAT4(0.6f, 0.3f, 0.1f, 1.0f)
	);


	//	===	中央の壁	===
	AddWall(
		XMFLOAT3(0, 1.5, 4),
		XMFLOAT3(10, 3, 1),
		XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
		);

	// === デバッグ出力 ===
	char debug[256];
	sprintf_s(debug, "MapSystem::CreateDefaultMap - Created %zu objects\n", m_objects.size());
	OutputDebugStringA(debug);
}