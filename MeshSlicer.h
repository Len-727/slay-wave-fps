// MeshSlicer.h
// リアルタイムメッシュ切断システム
#pragma once
#include <vector>
#include <DirectXMath.h>

// === 切断用の頂点データ ===
struct SliceVertex
{
    DirectX::XMFLOAT3 position;     // 位置
    DirectX::XMFLOAT3 normal;       // 法線
    DirectX::XMFLOAT2 uv;          // テクスチャ座標
};

// === 切断結果 ===
struct SliceResult
{
    // 上側メッシュ（法線の方向側）
    std::vector<SliceVertex> upperVertices;
    std::vector<uint32_t>    upperIndices;

    // 下側メッシュ（法線の反対側）
    std::vector<SliceVertex> lowerVertices;
    std::vector<uint32_t>    lowerIndices;

    // 断面の輪郭点
    std::vector<DirectX::XMFLOAT3> crossPoints;

    bool success = false;  // 切断が実際に行われたか
};

// === メッシュ切断クラス ===
class MeshSlicer
{
public:
    // メッシュを平面で切断する
    // vertices:    元のメッシュの頂点配列
    // indices:     元のメッシュのインデックス配列
    // planePoint:  切断面上の1点（ワールド座標）
    // planeNormal: 切断面の法線（正規化済み）
    static SliceResult Slice(
        const std::vector<SliceVertex>& vertices,
        const std::vector<uint32_t>& indices,
        DirectX::XMFLOAT3 planePoint,
        DirectX::XMFLOAT3 planeNormal);

    // 断面ポリゴンを生成（切り口の蓋）
    // crossPoints: 切断面上の交点ペア（Sliceで取得済み）
    // planeNormal: 切断面の法線
    // upperVerts/Inds: 上メッシュに断面を//
    // lowerVerts/Inds: 下メッシュに断面を//
    static void GenerateCap(
        const std::vector<DirectX::XMFLOAT3>& crossPoints,
        DirectX::XMFLOAT3 planeNormal,
        std::vector<SliceVertex>& upperVerts,
        std::vector<uint32_t>& upperInds,
        std::vector<SliceVertex>& lowerVerts,
        std::vector<uint32_t>& lowerInds);

private:
    // 頂点が平面のどちら側にあるか（正=上, 負=下, 0=面上）
    static float DistanceToPlane(
        const DirectX::XMFLOAT3& point,
        const DirectX::XMFLOAT3& planePoint,
        const DirectX::XMFLOAT3& planeNormal);

    // 2頂点の間で平面との交点を計算（線形補間）
    static SliceVertex LerpVertex(
        const SliceVertex& a,
        const SliceVertex& b,
        float t);
};