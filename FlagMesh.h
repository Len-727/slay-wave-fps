// ========================================
// FlagMesh.h - 旗のメッシュクラス
// ========================================

#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

// 旗の頂点構造体
struct FlagVertex
{
    DirectX::XMFLOAT3 position;  // 位置
    DirectX::XMFLOAT2 texCoord;  // UV座標
    DirectX::XMFLOAT3 normal;    // 法線（ライティング用）
};

class FlagMesh
{
public:
    FlagMesh();
    ~FlagMesh();

    // 初期化
    void Initialize(ID3D11Device* device, int width, int height);

    // 描画
    void Draw(ID3D11DeviceContext* context);

    // ゲッター
    int GetVertexCount() const { return m_vertexCount; }
    int GetIndexCount() const { return m_indexCount; }

private:
    // DirectX バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

    // メッシュ情報
    int m_width;         // 横方向の頂点数
    int m_height;        // 縦方向の頂点数
    int m_vertexCount;   // 総頂点数
    int m_indexCount;    // 総インデックス数

    // メッシュ生成
    void CreateMesh(
        std::vector<FlagVertex>& vertices,
        std::vector<uint16_t>& indices
    );
};