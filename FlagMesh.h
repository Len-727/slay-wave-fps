// ========================================
// FlagMesh.h - ���̃��b�V���N���X
// ========================================

#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

// ���̒��_�\����
struct FlagVertex
{
    DirectX::XMFLOAT3 position;  // �ʒu
    DirectX::XMFLOAT2 texCoord;  // UV���W
    DirectX::XMFLOAT3 normal;    // �@���i���C�e�B���O�p�j
};

class FlagMesh
{
public:
    FlagMesh();
    ~FlagMesh();

    // ������
    void Initialize(ID3D11Device* device, int width, int height);

    // �`��
    void Draw(ID3D11DeviceContext* context);

    // �Q�b�^�[
    int GetVertexCount() const { return m_vertexCount; }
    int GetIndexCount() const { return m_indexCount; }

private:
    // DirectX �o�b�t�@
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

    // ���b�V�����
    int m_width;         // �������̒��_��
    int m_height;        // �c�����̒��_��
    int m_vertexCount;   // �����_��
    int m_indexCount;    // ���C���f�b�N�X��

    // ���b�V������
    void CreateMesh(
        std::vector<FlagVertex>& vertices,
        std::vector<uint16_t>& indices
    );
};