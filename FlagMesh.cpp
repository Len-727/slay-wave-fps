// ========================================
// FlagMesh.cpp - 旗のメッシュ実装
// ========================================

#include "FlagMesh.h"
#include <stdexcept>

FlagMesh::FlagMesh()
    : m_width(0)
    , m_height(0)
    , m_vertexCount(0)
    , m_indexCount(0)
{
}

FlagMesh::~FlagMesh()
{
}

void FlagMesh::Initialize(ID3D11Device* device, int width, int height)
{
    m_width = width;
    m_height = height;
    m_vertexCount = width * height;
    m_indexCount = (width - 1) * (height - 1) * 6;  // 各四角形 = 2三角形 = 6頂点

    // === 頂点データとインデックスを生成 ===
    std::vector<FlagVertex> vertices;
    std::vector<uint16_t> indices;

    CreateMesh(vertices, indices);

    // === 頂点バッファを作成 ===
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(FlagVertex) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices.data();

    HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create flag vertex buffer");
    }

    // === インデックスバッファを作成 ===
    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(uint16_t) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();

    hr = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create flag index buffer");
    }
}

void FlagMesh::CreateMesh(
    std::vector<FlagVertex>& vertices,
    std::vector<uint16_t>& indices)
{
    vertices.clear();
    indices.clear();

    // === 頂点を生成（グリッド状） ===
    for (int y = 0; y < m_height; y++)
    {
        for (int x = 0; x < m_width; x++)
        {
            FlagVertex vertex;

            // 位置（-1.0 ~ 1.0 の範囲）
            // X: 左右（-1.0が左端、1.0が右端）
            // Y: 上下（-1.0が下端、1.0が上端）
            // Z: 奥行き（0.0で固定、シェーダーで動かす）
            vertex.position.x = (float)x / (m_width - 1) * 2.0f - 1.0f;
            vertex.position.y = (float)y / (m_height - 1) * 2.0f - 1.0f;
            vertex.position.z = 0.0f;

            // UV座標（0.0 ~ 1.0）
            vertex.texCoord.x = (float)x / (m_width - 1);
            vertex.texCoord.y = (float)y / (m_height - 1);

            // 法線（初期状態は正面向き）
            vertex.normal = DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f);

            vertices.push_back(vertex);
        }
    }

    // === インデックスを生成（三角形リスト） ===
    for (int y = 0; y < m_height - 1; y++)
    {
        for (int x = 0; x < m_width - 1; x++)
        {
            // 四角形の4隅
            int topLeft = y * m_width + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * m_width + x;
            int bottomRight = bottomLeft + 1;

            // 三角形1（左上）
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // 三角形2（右下）
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

void FlagMesh::Draw(ID3D11DeviceContext* context)
{
    // 頂点バッファをセット
    UINT stride = sizeof(FlagVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    // インデックスバッファをセット
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // プリミティブトポロジをセット
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画
    context->DrawIndexed(m_indexCount, 0, 0);
}