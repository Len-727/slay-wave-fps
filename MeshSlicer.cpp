// MeshSlicer.cpp
// ���A���^�C�����b�V���ؒf�̎���
#include "MeshSlicer.h"
#include <cmath>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace DirectX;

// ============================================
//  ���_�����ʂ̂ǂ��瑤�ɂ��邩
//  ���̒l = �@�����i��j
//  ���̒l = �@���̔��Α��i���j
//  0 = ���ʏ�
// ============================================
float MeshSlicer::DistanceToPlane(
    const XMFLOAT3& point,
    const XMFLOAT3& planePoint,
    const XMFLOAT3& planeNormal)
{
    // �x�N�g��: ���ʏ�̓_ �� ���_
    float dx = point.x - planePoint.x;
    float dy = point.y - planePoint.y;
    float dz = point.z - planePoint.z;

    // �@���Ƃ̓��� = �����t������
    return dx * planeNormal.x + dy * planeNormal.y + dz * planeNormal.z;
}

// ============================================
//  2���_�̊Ԃ���`��ԁi�ؒf�ʂƂ̌�_�����j
//  t = 0.0 �� a�̈ʒu�At = 1.0 �� b�̈ʒu
// ============================================
SliceVertex MeshSlicer::LerpVertex(
    const SliceVertex& a,
    const SliceVertex& b,
    float t)
{
    SliceVertex result;

    // �ʒu����
    result.position.x = a.position.x + (b.position.x - a.position.x) * t;
    result.position.y = a.position.y + (b.position.y - a.position.y) * t;
    result.position.z = a.position.z + (b.position.z - a.position.z) * t;

    // �@������
    result.normal.x = a.normal.x + (b.normal.x - a.normal.x) * t;
    result.normal.y = a.normal.y + (b.normal.y - a.normal.y) * t;
    result.normal.z = a.normal.z + (b.normal.z - a.normal.z) * t;

    // �@���𐳋K��
    float len = sqrtf(
        result.normal.x * result.normal.x +
        result.normal.y * result.normal.y +
        result.normal.z * result.normal.z);
    if (len > 0.0001f)
    {
        result.normal.x /= len;
        result.normal.y /= len;
        result.normal.z /= len;
    }

    // UV����
    result.uv.x = a.uv.x + (b.uv.x - a.uv.x) * t;
    result.uv.y = a.uv.y + (b.uv.y - a.uv.y) * t;

    return result;
}

// ============================================
//  ���C���̐ؒf�֐�
// ============================================
SliceResult MeshSlicer::Slice(
    const std::vector<SliceVertex>& vertices,
    const std::vector<uint32_t>& indices,
    XMFLOAT3 planePoint,
    XMFLOAT3 planeNormal)
{
    SliceResult result;
    result.success = false;

    if (vertices.empty() || indices.empty()) return result;

    // �O�p�`�̐�
    size_t triCount = indices.size() / 3;

    bool hasUpper = false;  // �㑤�ɎO�p�`����������
    bool hasLower = false;  // �����ɎO�p�`����������

    // === �e�O�p�`������ ===
    for (size_t t = 0; t < triCount; t++)
    {
        // �O�p�`��3���_���擾
        const SliceVertex& v0 = vertices[indices[t * 3 + 0]];
        const SliceVertex& v1 = vertices[indices[t * 3 + 1]];
        const SliceVertex& v2 = vertices[indices[t * 3 + 2]];

        // �e���_�̕��ʂ���̋����i�����t���j
        float d0 = DistanceToPlane(v0.position, planePoint, planeNormal);
        float d1 = DistanceToPlane(v1.position, planePoint, planeNormal);
        float d2 = DistanceToPlane(v2.position, planePoint, planeNormal);

        // �����l�i�قڕ��ʏ�̒��_���㑤�����ɂ���j
        const float epsilon = 0.0001f;

        // �e���_���㑤(+)������(-)���𔻒�
        bool above0 = (d0 >= -epsilon);
        bool above1 = (d1 >= -epsilon);
        bool above2 = (d2 >= -epsilon);

        int aboveCount = (above0 ? 1 : 0) + (above1 ? 1 : 0) + (above2 ? 1 : 0);

        // �w���p�[: �O�p�`���w�胁�b�V����//
        auto addTriangle = [](
            std::vector<SliceVertex>& verts,
            std::vector<uint32_t>& inds,
            const SliceVertex& a, const SliceVertex& b, const SliceVertex& c)
            {
                uint32_t base = (uint32_t)verts.size();
                verts.push_back(a);
                verts.push_back(b);
                verts.push_back(c);
                inds.push_back(base + 0);
                inds.push_back(base + 1);
                inds.push_back(base + 2);
            };

        // -----------------------------------------
        // �p�^�[��A: 3���_�Ƃ��㑤 �� �チ�b�V����
        // -----------------------------------------
        if (aboveCount == 3)
        {
            addTriangle(result.upperVertices, result.upperIndices, v0, v1, v2);
            hasUpper = true;
        }
        // -----------------------------------------
        // �p�^�[��B: 3���_�Ƃ����� �� �����b�V����
        // -----------------------------------------
        else if (aboveCount == 0)
        {
            addTriangle(result.lowerVertices, result.lowerIndices, v0, v1, v2);
            hasLower = true;
        }
        // -----------------------------------------
        // �p�^�[��C: ���� �� �O�p�`�𕪊��I
        // -----------------------------------------
        else
        {
            hasUpper = true;
            hasLower = true;

            // �u�Ǘ�����1���_�v�Ɓu���Α���2���_�v�����
            // 1�����㑤 �� ����1���Ǘ�
            // 2�㑤    �� ������1���Ǘ�
            const SliceVertex* alone;       // �Ǘ��������_
            const SliceVertex* pair1;       // ���Α��̒��_1
            const SliceVertex* pair2;       // ���Α��̒��_2
            float dAlone, dPair1, dPair2;
            bool aloneIsAbove;

            if (aboveCount == 1)
            {
                // 1�����㑤
                if (above0) { alone = &v0; pair1 = &v1; pair2 = &v2; dAlone = d0; dPair1 = d1; dPair2 = d2; }
                else if (above1) { alone = &v1; pair1 = &v0; pair2 = &v2; dAlone = d1; dPair1 = d0; dPair2 = d2; }
                else { alone = &v2; pair1 = &v0; pair2 = &v1; dAlone = d2; dPair1 = d0; dPair2 = d1; }
                aloneIsAbove = true;
            }
            else
            {
                // 2�㑤 �� 1�����������Ǘ�
                if (!above0) { alone = &v0; pair1 = &v1; pair2 = &v2; dAlone = d0; dPair1 = d1; dPair2 = d2; }
                else if (!above1) { alone = &v1; pair1 = &v0; pair2 = &v2; dAlone = d1; dPair1 = d0; dPair2 = d2; }
                else { alone = &v2; pair1 = &v0; pair2 = &v1; dAlone = d2; dPair1 = d0; dPair2 = d1; }
                aloneIsAbove = false;
            }

            // �ؒf�ʂƂ̌�_���v�Z
            // alone��pair1 �̕ӏ�̌�_
            float t1 = dAlone / (dAlone - dPair1);  // 0?1�̕�ԌW��
            SliceVertex cross1 = LerpVertex(*alone, *pair1, t1);

            // alone��pair2 �̕ӏ�̌�_
            float t2 = dAlone / (dAlone - dPair2);
            SliceVertex cross2 = LerpVertex(*alone, *pair2, t2);

            // �f�ʂ̗֊s�_���L�^�iDay 2�Ŏg���j
            result.crossPoints.push_back(cross1.position);
            result.crossPoints.push_back(cross2.position);

            if (aloneIsAbove)
            {
                // �Ǘ����_���㑤:
                //   �チ�b�V��: alone-cross1-cross2�i�O�p�`1�j
                //   �����b�V��: cross1-pair1-pair2 + cross1-pair2-cross2�i�O�p�`2�j
                addTriangle(result.upperVertices, result.upperIndices,
                    *alone, cross1, cross2);

                addTriangle(result.lowerVertices, result.lowerIndices,
                    cross1, *pair1, *pair2);
                addTriangle(result.lowerVertices, result.lowerIndices,
                    cross1, *pair2, cross2);
            }
            else
            {
                // �Ǘ����_������:
                //   �����b�V��: alone-cross1-cross2�i�O�p�`1�j
                //   �チ�b�V��: cross1-pair1-pair2 + cross1-pair2-cross2�i�O�p�`2�j
                addTriangle(result.lowerVertices, result.lowerIndices,
                    *alone, cross1, cross2);

                addTriangle(result.upperVertices, result.upperIndices,
                    cross1, *pair1, *pair2);
                addTriangle(result.upperVertices, result.upperIndices,
                    cross1, *pair2, cross2);
            }
        }
    }

    // �����ɎO�p�`������ꍇ�̂ݐؒf����
    result.success = (hasUpper && hasLower);

    // �ؒf����������f�ʃ|���S���𐶐�
    if (result.success && !result.crossPoints.empty())
    {
        GenerateCap(
            result.crossPoints, planeNormal,
            result.upperVertices, result.upperIndices,
            result.lowerVertices, result.lowerIndices);
    }

    if (result.success)
    {
        char buf[256];
        sprintf_s(buf, "[MeshSlicer] Slice OK: upper=%zu tris, lower=%zu tris, cross=%zu points\n",
            result.upperIndices.size() / 3,
            result.lowerIndices.size() / 3,
            result.crossPoints.size());
        OutputDebugStringA(buf);
    }

    return result;
}

// ============================================
//  �f�ʃ|���S�������i�؂���̊W�j
// ============================================
void MeshSlicer::GenerateCap(
    const std::vector<XMFLOAT3>& crossPoints,
    XMFLOAT3 planeNormal,
    std::vector<SliceVertex>& upperVerts,
    std::vector<uint32_t>& upperInds,
    std::vector<SliceVertex>& lowerVerts,
    std::vector<uint32_t>& lowerInds)
{
    // ��_�����Ȃ�������f�ʂ����Ȃ�
    if (crossPoints.size() < 3) return;

    // ---  �S��_�̏d�S���v�Z�i���S�_�j ---
    XMFLOAT3 center = { 0, 0, 0 };
    for (const auto& p : crossPoints)
    {
        center.x += p.x;
        center.y += p.y;
        center.z += p.z;
    }
    float invCount = 1.0f / (float)crossPoints.size();
    center.x *= invCount;
    center.y *= invCount;
    center.z *= invCount;

    // ---  ��_���p�x�Ń\�[�g�i�t�@���𐳂������邽�߁j ---
    // �ؒf�ʏ�̃��[�J�����W�n���\�z
    // right = �@���ɐ����Ȏ�1
    // up    = �@���ɐ����Ȏ�2
    XMVECTOR nVec = XMLoadFloat3(&planeNormal);

    // �@���ƍł���������x�N�g����I��ŊO��
    XMFLOAT3 hint = { 0, 1, 0 };
    if (fabsf(planeNormal.y) > 0.9f)
        hint = { 1, 0, 0 };  // �@�����ق�Y���Ȃ�ʂ̎����g��

    XMVECTOR hintVec = XMLoadFloat3(&hint);
    XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(nVec, hintVec));
    XMVECTOR upVec = XMVector3Normalize(XMVector3Cross(rightVec, nVec));

    XMFLOAT3 rightF, upF;
    XMStoreFloat3(&rightF, rightVec);
    XMStoreFloat3(&upF, upVec);

    // �e��_�̊p�x���v�Z���ă\�[�g
    struct AnglePoint
    {
        float angle;
        XMFLOAT3 pos;
    };
    std::vector<AnglePoint> sorted;
    sorted.reserve(crossPoints.size());

    for (const auto& p : crossPoints)
    {
        float dx = p.x - center.x;
        float dy = p.y - center.y;
        float dz = p.z - center.z;

        // ���[�J�����W�ɕϊ�
        float localX = dx * rightF.x + dy * rightF.y + dz * rightF.z;
        float localY = dx * upF.x + dy * upF.y + dz * upF.z;

        float angle = atan2f(localY, localX);
        sorted.push_back({ angle, p });
    }

    // �p�x�Ń\�[�g
    std::sort(sorted.begin(), sorted.end(),
        [](const AnglePoint& a, const AnglePoint& b) { return a.angle < b.angle; });

    // �d���������i�߂�����_���폜�j
    std::vector<XMFLOAT3> uniquePoints;
    uniquePoints.reserve(sorted.size());
    for (size_t i = 0; i < sorted.size(); i++)
    {
        if (i == 0)
        {
            uniquePoints.push_back(sorted[i].pos);
            continue;
        }
        float dx = sorted[i].pos.x - uniquePoints.back().x;
        float dy = sorted[i].pos.y - uniquePoints.back().y;
        float dz = sorted[i].pos.z - uniquePoints.back().z;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > 0.0001f)  // 0.01m�ȏ㗣��Ă���//
        {
            uniquePoints.push_back(sorted[i].pos);
        }
    }

    if (uniquePoints.size() < 3) return;

    // ---  �t�@���g���C�A���O���[�V���� ---
    // ���S�_����e�ӂɎO�p�`�𒣂�
    // �チ�b�V��: �@��������\�ɂ���
    // �����b�V��: �@���̋t������\�ɂ���

    // �f�ʗp��UV�v�Z�i���S=0.5, ���a�ōL����j
    auto makeCapVertex = [&](const XMFLOAT3& pos, const XMFLOAT3& normal) -> SliceVertex
        {
            SliceVertex sv;
            sv.position = pos;
            sv.normal = normal;
            float dx = pos.x - center.x;
            float dy = pos.y - center.y;
            float dz = pos.z - center.z;
            float localX = dx * rightF.x + dy * rightF.y + dz * rightF.z;
            float localY = dx * upF.x + dy * upF.y + dz * upF.z;
            sv.uv.x = localX * 0.5f + 0.5f;
            sv.uv.y = localY * 0.5f + 0.5f;
            return sv;
        };

    // �チ�b�V���̒f�ʁi�@�� = planeNormal�j
    XMFLOAT3 upperNormal = planeNormal;
    // �����b�V���̒f�ʁi�@�� = -planeNormal�j
    XMFLOAT3 lowerNormal = { -planeNormal.x, -planeNormal.y, -planeNormal.z };

    for (size_t i = 0; i < uniquePoints.size(); i++)
    {
        size_t next = (i + 1) % uniquePoints.size();

        // �チ�b�V���̒f�ʎO�p�`�i���S��i��next�j
        SliceVertex cUp = makeCapVertex(center, upperNormal);
        SliceVertex aUp = makeCapVertex(uniquePoints[i], upperNormal);
        SliceVertex bUp = makeCapVertex(uniquePoints[next], upperNormal);

        uint32_t baseU = (uint32_t)upperVerts.size();
        upperVerts.push_back(cUp);
        upperVerts.push_back(aUp);
        upperVerts.push_back(bUp);
        upperInds.push_back(baseU);
        upperInds.push_back(baseU + 1);
        upperInds.push_back(baseU + 2);

        // �����b�V���̒f�ʎO�p�`�i���S��next��i�j�� �t���ŗ���
        SliceVertex cLo = makeCapVertex(center, lowerNormal);
        SliceVertex aLo = makeCapVertex(uniquePoints[next], lowerNormal);
        SliceVertex bLo = makeCapVertex(uniquePoints[i], lowerNormal);

        uint32_t baseL = (uint32_t)lowerVerts.size();
        lowerVerts.push_back(cLo);
        lowerVerts.push_back(aLo);
        lowerVerts.push_back(bLo);
        lowerInds.push_back(baseL);
        lowerInds.push_back(baseL + 1);
        lowerInds.push_back(baseL + 2);
    }

    char buf[256];
    sprintf_s(buf, "[MeshSlicer] Cap generated: %zu triangles per side\n",
        uniquePoints.size());
    OutputDebugStringA(buf);
}