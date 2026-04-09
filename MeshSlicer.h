// MeshSlicer.h
// ���A���^�C�����b�V���ؒf�V�X�e��
#pragma once
#include <vector>
#include <DirectXMath.h>

// === �ؒf�p�̒��_�f�[�^ ===
struct SliceVertex
{
    DirectX::XMFLOAT3 position;     // �ʒu
    DirectX::XMFLOAT3 normal;       // �@��
    DirectX::XMFLOAT2 uv;          // �e�N�X�`�����W
};

// === �ؒf���� ===
struct SliceResult
{
    // �㑤���b�V���i�@���̕������j
    std::vector<SliceVertex> upperVertices;
    std::vector<uint32_t>    upperIndices;

    // �������b�V���i�@���̔��Α��j
    std::vector<SliceVertex> lowerVertices;
    std::vector<uint32_t>    lowerIndices;

    // �f�ʂ̗֊s�_
    std::vector<DirectX::XMFLOAT3> crossPoints;

    bool success = false;  // �ؒf�����ۂɍs��ꂽ��
};

// === ���b�V���ؒf�N���X ===
class MeshSlicer
{
public:
    // ���b�V���𕽖ʂŐؒf����
    // vertices:    ���̃��b�V���̒��_�z��
    // indices:     ���̃��b�V���̃C���f�b�N�X�z��
    // planePoint:  �ؒf�ʏ��1�_�i���[���h���W�j
    // planeNormal: �ؒf�ʂ̖@���i���K���ς݁j
    static SliceResult Slice(
        const std::vector<SliceVertex>& vertices,
        const std::vector<uint32_t>& indices,
        DirectX::XMFLOAT3 planePoint,
        DirectX::XMFLOAT3 planeNormal);

    // �f�ʃ|���S���𐶐��i�؂���̊W�j
    // crossPoints: �ؒf�ʏ�̌�_�y�A�iSlice�Ŏ擾�ς݁j
    // planeNormal: �ؒf�ʂ̖@��
    // upperVerts/Inds: �チ�b�V���ɒf�ʂ�//
    // lowerVerts/Inds: �����b�V���ɒf�ʂ�//
    static void GenerateCap(
        const std::vector<DirectX::XMFLOAT3>& crossPoints,
        DirectX::XMFLOAT3 planeNormal,
        std::vector<SliceVertex>& upperVerts,
        std::vector<uint32_t>& upperInds,
        std::vector<SliceVertex>& lowerVerts,
        std::vector<uint32_t>& lowerInds);

private:
    // ���_�����ʂ̂ǂ��瑤�ɂ��邩�i��=��, ��=��, 0=�ʏ�j
    static float DistanceToPlane(
        const DirectX::XMFLOAT3& point,
        const DirectX::XMFLOAT3& planePoint,
        const DirectX::XMFLOAT3& planeNormal);

    // 2���_�̊Ԃŕ��ʂƂ̌�_���v�Z�i���`��ԁj
    static SliceVertex LerpVertex(
        const SliceVertex& a,
        const SliceVertex& b,
        float t);
};