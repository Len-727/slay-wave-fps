// MeshSlicer.cpp
// リアルタイムメッシュ切断の実装
#include "MeshSlicer.h"
#include <cmath>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace DirectX;

// ============================================
//  頂点が平面のどちら側にあるか
//  正の値 = 法線側（上）
//  負の値 = 法線の反対側（下）
//  0 = 平面上
// ============================================
float MeshSlicer::DistanceToPlane(
    const XMFLOAT3& point,
    const XMFLOAT3& planePoint,
    const XMFLOAT3& planeNormal)
{
    // ベクトル: 平面上の点 → 頂点
    float dx = point.x - planePoint.x;
    float dy = point.y - planePoint.y;
    float dz = point.z - planePoint.z;

    // 法線との内積 = 符号付き距離
    return dx * planeNormal.x + dy * planeNormal.y + dz * planeNormal.z;
}

// ============================================
//  2頂点の間を線形補間（切断面との交点を作る）
//  t = 0.0 → aの位置、t = 1.0 → bの位置
// ============================================
SliceVertex MeshSlicer::LerpVertex(
    const SliceVertex& a,
    const SliceVertex& b,
    float t)
{
    SliceVertex result;

    // 位置を補間
    result.position.x = a.position.x + (b.position.x - a.position.x) * t;
    result.position.y = a.position.y + (b.position.y - a.position.y) * t;
    result.position.z = a.position.z + (b.position.z - a.position.z) * t;

    // 法線を補間
    result.normal.x = a.normal.x + (b.normal.x - a.normal.x) * t;
    result.normal.y = a.normal.y + (b.normal.y - a.normal.y) * t;
    result.normal.z = a.normal.z + (b.normal.z - a.normal.z) * t;

    // 法線を正規化
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

    // UVを補間
    result.uv.x = a.uv.x + (b.uv.x - a.uv.x) * t;
    result.uv.y = a.uv.y + (b.uv.y - a.uv.y) * t;

    return result;
}

// ============================================
//  メインの切断関数
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

    // 三角形の数
    size_t triCount = indices.size() / 3;

    bool hasUpper = false;  // 上側に三角形があったか
    bool hasLower = false;  // 下側に三角形があったか

    // === 各三角形を処理 ===
    for (size_t t = 0; t < triCount; t++)
    {
        // 三角形の3頂点を取得
        const SliceVertex& v0 = vertices[indices[t * 3 + 0]];
        const SliceVertex& v1 = vertices[indices[t * 3 + 1]];
        const SliceVertex& v2 = vertices[indices[t * 3 + 2]];

        // 各頂点の平面からの距離（符号付き）
        float d0 = DistanceToPlane(v0.position, planePoint, planeNormal);
        float d1 = DistanceToPlane(v1.position, planePoint, planeNormal);
        float d2 = DistanceToPlane(v2.position, planePoint, planeNormal);

        // 微小値（ほぼ平面上の頂点を上側扱いにする）
        const float epsilon = 0.0001f;

        // 各頂点が上側(+)か下側(-)かを判定
        bool above0 = (d0 >= -epsilon);
        bool above1 = (d1 >= -epsilon);
        bool above2 = (d2 >= -epsilon);

        int aboveCount = (above0 ? 1 : 0) + (above1 ? 1 : 0) + (above2 ? 1 : 0);

        // ヘルパー: 三角形を指定メッシュに//
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
        // パターンA: 3頂点とも上側 → 上メッシュへ
        // -----------------------------------------
        if (aboveCount == 3)
        {
            addTriangle(result.upperVertices, result.upperIndices, v0, v1, v2);
            hasUpper = true;
        }
        // -----------------------------------------
        // パターンB: 3頂点とも下側 → 下メッシュへ
        // -----------------------------------------
        else if (aboveCount == 0)
        {
            addTriangle(result.lowerVertices, result.lowerIndices, v0, v1, v2);
            hasLower = true;
        }
        // -----------------------------------------
        // パターンC: 混在 → 三角形を分割！
        // -----------------------------------------
        else
        {
            hasUpper = true;
            hasLower = true;

            // 「孤立した1頂点」と「反対側の2頂点」を特定
            // 1つだけ上側 → その1つが孤立
            // 2つ上側    → 下側の1つが孤立
            const SliceVertex* alone;       // 孤立した頂点
            const SliceVertex* pair1;       // 反対側の頂点1
            const SliceVertex* pair2;       // 反対側の頂点2
            float dAlone, dPair1, dPair2;
            bool aloneIsAbove;

            if (aboveCount == 1)
            {
                // 1つだけ上側
                if (above0) { alone = &v0; pair1 = &v1; pair2 = &v2; dAlone = d0; dPair1 = d1; dPair2 = d2; }
                else if (above1) { alone = &v1; pair1 = &v0; pair2 = &v2; dAlone = d1; dPair1 = d0; dPair2 = d2; }
                else { alone = &v2; pair1 = &v0; pair2 = &v1; dAlone = d2; dPair1 = d0; dPair2 = d1; }
                aloneIsAbove = true;
            }
            else
            {
                // 2つ上側 → 1つだけ下側が孤立
                if (!above0) { alone = &v0; pair1 = &v1; pair2 = &v2; dAlone = d0; dPair1 = d1; dPair2 = d2; }
                else if (!above1) { alone = &v1; pair1 = &v0; pair2 = &v2; dAlone = d1; dPair1 = d0; dPair2 = d2; }
                else { alone = &v2; pair1 = &v0; pair2 = &v1; dAlone = d2; dPair1 = d0; dPair2 = d1; }
                aloneIsAbove = false;
            }

            // 切断面との交点を計算
            // alone→pair1 の辺上の交点
            float t1 = dAlone / (dAlone - dPair1);  // 0?1の補間係数
            SliceVertex cross1 = LerpVertex(*alone, *pair1, t1);

            // alone→pair2 の辺上の交点
            float t2 = dAlone / (dAlone - dPair2);
            SliceVertex cross2 = LerpVertex(*alone, *pair2, t2);

            // 断面の輪郭点を記録（Day 2で使う）
            result.crossPoints.push_back(cross1.position);
            result.crossPoints.push_back(cross2.position);

            if (aloneIsAbove)
            {
                // 孤立頂点が上側:
                //   上メッシュ: alone-cross1-cross2（三角形1つ）
                //   下メッシュ: cross1-pair1-pair2 + cross1-pair2-cross2（三角形2つ）
                addTriangle(result.upperVertices, result.upperIndices,
                    *alone, cross1, cross2);

                addTriangle(result.lowerVertices, result.lowerIndices,
                    cross1, *pair1, *pair2);
                addTriangle(result.lowerVertices, result.lowerIndices,
                    cross1, *pair2, cross2);
            }
            else
            {
                // 孤立頂点が下側:
                //   下メッシュ: alone-cross1-cross2（三角形1つ）
                //   上メッシュ: cross1-pair1-pair2 + cross1-pair2-cross2（三角形2つ）
                addTriangle(result.lowerVertices, result.lowerIndices,
                    *alone, cross1, cross2);

                addTriangle(result.upperVertices, result.upperIndices,
                    cross1, *pair1, *pair2);
                addTriangle(result.upperVertices, result.upperIndices,
                    cross1, *pair2, cross2);
            }
        }
    }

    // 両側に三角形がある場合のみ切断成功
    result.success = (hasUpper && hasLower);

    // 切断成功したら断面ポリゴンを生成
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
//  断面ポリゴン生成（切り口の蓋）
// ============================================
void MeshSlicer::GenerateCap(
    const std::vector<XMFLOAT3>& crossPoints,
    XMFLOAT3 planeNormal,
    std::vector<SliceVertex>& upperVerts,
    std::vector<uint32_t>& upperInds,
    std::vector<SliceVertex>& lowerVerts,
    std::vector<uint32_t>& lowerInds)
{
    // 交点が少なすぎたら断面を作れない
    if (crossPoints.size() < 3) return;

    // ---  全交点の重心を計算（中心点） ---
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

    // ---  交点を角度でソート（ファンを正しく張るため） ---
    // 切断面上のローカル座標系を構築
    // right = 法線に垂直な軸1
    // up    = 法線に垂直な軸2
    XMVECTOR nVec = XMLoadFloat3(&planeNormal);

    // 法線と最も直交するベクトルを選んで外積
    XMFLOAT3 hint = { 0, 1, 0 };
    if (fabsf(planeNormal.y) > 0.9f)
        hint = { 1, 0, 0 };  // 法線がほぼY軸なら別の軸を使う

    XMVECTOR hintVec = XMLoadFloat3(&hint);
    XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(nVec, hintVec));
    XMVECTOR upVec = XMVector3Normalize(XMVector3Cross(rightVec, nVec));

    XMFLOAT3 rightF, upF;
    XMStoreFloat3(&rightF, rightVec);
    XMStoreFloat3(&upF, upVec);

    // 各交点の角度を計算してソート
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

        // ローカル座標に変換
        float localX = dx * rightF.x + dy * rightF.y + dz * rightF.z;
        float localY = dx * upF.x + dy * upF.y + dz * upF.z;

        float angle = atan2f(localY, localX);
        sorted.push_back({ angle, p });
    }

    // 角度でソート
    std::sort(sorted.begin(), sorted.end(),
        [](const AnglePoint& a, const AnglePoint& b) { return a.angle < b.angle; });

    // 重複を除去（近すぎる点を削除）
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
        if (distSq > 0.0001f)  // 0.01m以上離れてたら//
        {
            uniquePoints.push_back(sorted[i].pos);
        }
    }

    if (uniquePoints.size() < 3) return;

    // ---  ファントライアングレーション ---
    // 中心点から各辺に三角形を張る
    // 上メッシュ: 法線方向を表にする
    // 下メッシュ: 法線の逆方向を表にする

    // 断面用のUV計算（中心=0.5, 半径で広がる）
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

    // 上メッシュの断面（法線 = planeNormal）
    XMFLOAT3 upperNormal = planeNormal;
    // 下メッシュの断面（法線 = -planeNormal）
    XMFLOAT3 lowerNormal = { -planeNormal.x, -planeNormal.y, -planeNormal.z };

    for (size_t i = 0; i < uniquePoints.size(); i++)
    {
        size_t next = (i + 1) % uniquePoints.size();

        // 上メッシュの断面三角形（中心→i→next）
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

        // 下メッシュの断面三角形（中心→next→i）← 逆順で裏面
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