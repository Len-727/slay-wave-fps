// ========================================
// FlagMesh.cpp - ���̃��b�V������
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
    m_indexCount = (width - 1) * (height - 1) * 6;  // �e�l�p�` = 2�O�p�` = 6���_

    // === ���_�f�[�^�ƃC���f�b�N�X�𐶐� ===
    std::vector<FlagVertex> vertices;
    std::vector<uint16_t> indices;

    CreateMesh(vertices, indices);

    // === ���_�o�b�t�@���쐬 ===
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

    // === �C���f�b�N�X�o�b�t�@���쐬 ===
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

    // === ���_�𐶐��i�O���b�h��j ===
    for (int y = 0; y < m_height; y++)
    {
        for (int x = 0; x < m_width; x++)
        {
            FlagVertex vertex;

            // �ʒu�i-1.0 ~ 1.0 �͈̔́j
            // X: ���E�i-1.0�����[�A1.0���E�[�j
            // Y: �㉺�i-1.0�����[�A1.0����[�j
            // Z: ���s���i0.0�ŌŒ�A�V�F�[�_�[�œ������j
            vertex.position.x = (float)x / (m_width - 1) * 2.0f - 1.0f;
            vertex.position.y = (float)y / (m_height - 1) * 2.0f - 1.0f;
            vertex.position.z = 0.0f;

            // UV���W�i0.0 ~ 1.0�j
            vertex.texCoord.x = (float)x / (m_width - 1);
            vertex.texCoord.y = (float)y / (m_height - 1);

            // �@���i������Ԃ͐��ʌ����j
            vertex.normal = DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f);

            vertices.push_back(vertex);
        }
    }

    // === �C���f�b�N�X�𐶐��i�O�p�`���X�g�j ===
    for (int y = 0; y < m_height - 1; y++)
    {
        for (int x = 0; x < m_width - 1; x++)
        {
            // �l�p�`��4��
            int topLeft = y * m_width + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * m_width + x;
            int bottomRight = bottomLeft + 1;

            // �O�p�`1�i����j
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // �O�p�`2�i�E���j
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

void FlagMesh::Draw(ID3D11DeviceContext* context)
{
    // ���_�o�b�t�@���Z�b�g
    UINT stride = sizeof(FlagVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    // �C���f�b�N�X�o�b�t�@���Z�b�g
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // �v���~�e�B�u�g�|���W���Z�b�g
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // �`��
    context->DrawIndexed(m_indexCount, 0, 0);
}