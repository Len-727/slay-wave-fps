//	MapSystem.h
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <GeometricPrimitive.h>
#include <Effects.h>
#include <CommonStates.h>
#include <WICTextureLoader.h>
#include <VertexTypes.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

enum class MapObjectType { FLOOR, WALL, BOX, CYLINDER };



struct MapObject
{
    MapObjectType type;
    XMFLOAT3 position;
    XMFLOAT3 scale;
    XMFLOAT3 rotation;
    XMFLOAT4 color;
    bool hasCollision;
};

struct StaticSubMesh
{
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT vertexCount = 0;
    UINT indexCount = 0;
    int  materialIndex = -1;
    XMFLOAT4X4 nodeTransform;
    std::string meshName;
};

struct MapMaterial
{
    ComPtr<ID3D11ShaderResourceView> diffuseTexture;
    XMFLOAT4 diffuseColor = { 1, 1, 1, 1 };
    std::string name;
};

//  床の高さゾーン（階段・2階対応）
struct FloorZone
{
    float minX, maxX;      // ゾーンのXZ範囲
    float minZ, maxZ;
    float heightStart;     // ゾーン入口の高さ
    float heightEnd;       // ゾーン出口の高さ
    bool  isSlopeZ;        // true=Z方向にスロープ
};


class MapSystem
{
public:
    MapSystem();

    ~MapSystem() = default;

    bool Initialize(ID3D11DeviceContext* context, ID3D11Device* device);

    void Draw(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection);

    bool LoadMapFBX(const std::string& fbxPath,
        const std::wstring& textureDir, float scale = 1.0f);

    void AddFloor(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color = { 0.3f, 0.3f, 0.3f, 1.0f });
    void AddWall(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color = { 0.5f, 0.5f, 0.5f, 1.0f });
    void AddBox(XMFLOAT3 position, XMFLOAT3 scale, XMFLOAT4 color = { 0.6f, 0.4f, 0.2f, 1.0f });

    bool CheckCollision(XMFLOAT3 position, float radius);
    void CreateDefaultMap();

    void SetDrawPrimitives(bool draw) { m_drawPrimitives = draw; }
    void SetMapTransform(XMFLOAT3 pos, float rotY, float scale);

    const std::vector<MapObject>& GetObjects() const { return m_objects; }

    float GetFloorHeight(float x, float z) const;

private:
    void DrawMapModel(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection);




    void DrawPrimitives(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection);

    std::vector<MapObject> m_objects;
    std::unique_ptr<GeometricPrimitive> m_box;
    std::unique_ptr<GeometricPrimitive> m_cylinder;
    std::unique_ptr<DirectX::CommonStates> m_states;




    std::vector<StaticSubMesh> m_subMeshes;
    std::vector<MapMaterial>   m_materials;
    std::unique_ptr<BasicEffect> m_mapEffect;
    ComPtr<ID3D11InputLayout>    m_mapInputLayout;

    bool m_mapLoaded = false;



    XMFLOAT3 m_mapPosition = { 0, 0, 0 };
    float    m_mapRotationY = 0.0f;

    float    m_mapScale = 1.0f;

    bool m_drawPrimitives = false;
    ID3D11Device* m_device = nullptr;


    std::vector<FloorZone> m_floorZones;
};