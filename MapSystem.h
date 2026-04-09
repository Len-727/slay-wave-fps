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

//  ���̍����]�[���i�K�i�E2�K�Ή��j
struct FloorZone
{
    float minX, maxX;      // �]�[����XZ�͈�
    float minZ, maxZ;
    float heightStart;     // �]�[�������̍���
    float heightEnd;       // �]�[���o���̍���
    bool  isSlopeZ;        // true=Z�����ɃX���[�v
};

//  === ���b�V���R���C�_�[�p : CPU���̎O�p�`�f�[�^
struct CollisionTriangle
{
    XMFLOAT3 v0, v1, v2;
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

    // === ���C�g�����p�����[�^�iImGui�ŐG��p�j ===
    XMFLOAT3 m_lightDir0 = { 0.0f, -1.0f, 0.3f };       // ���C�����C�g����
    XMFLOAT4 m_lightColor0 = { 0.6f, 0.45f, 0.3f, 1.0f }; // ���C�����C�g�F
    XMFLOAT3 m_lightDir1 = { -0.5f, -0.3f, -0.7f };      // �⏕���C�g����
    XMFLOAT4 m_lightColor1 = { 0.15f, 0.15f, 0.25f, 1.0f };// �⏕���C�g�F
    XMFLOAT4 m_ambientColor = { 0.08f, 0.06f, 0.05f, 1.0f };// ����


    // ���C�g�𔽉f����֐�
    void ApplyLightSettings();

    void SetMapTransform(XMFLOAT3 pos, float rotY, float scale);

    const std::vector<MapObject>& GetObjects() const { return m_objects; }

    //  ���b�V���R���C�_�[�p : �S�O�p�`��Ԃ�
    const std::vector<CollisionTriangle>& GetCollisionTriangles() const
    {
        return m_collisionTriangles;
    }

    float GetFloorHeight(float x, float z) const;

private:
    void DrawMapModel(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection);




    void DrawPrimitives(ID3D11DeviceContext* context, XMMATRIX view, XMMATRIX projection);

    std::vector<MapObject> m_objects;
    std::unique_ptr<GeometricPrimitive> m_box;
    std::unique_ptr<GeometricPrimitive> m_cylinder;
    std::unique_ptr<DirectX::CommonStates> m_states;

    ComPtr<ID3D11ShaderResourceView> m_floorTexture;
    ComPtr<ID3D11ShaderResourceView> m_wallTexture;

    //  �^�C���`��p InputLayout�im_mapEffect �� GeometricPrimitive ���q���j
    ComPtr<ID3D11InputLayout> m_boxInputLayout;

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

   
    std::vector<CollisionTriangle> m_collisionTriangles;
};