#pragma once
#include <DirectXMath.h>

// ========================================
// ベジェ曲線クラス（3次ベジェ曲線）
// ========================================

class BezierCurve
{
public:
    BezierCurve();
    ~BezierCurve();

    // 制御点を設定
    void SetControlPoints(
        DirectX::XMFLOAT3 p0,  // 開始点
        DirectX::XMFLOAT3 p1,  // 制御点1
        DirectX::XMFLOAT3 p2,  // 制御点2
        DirectX::XMFLOAT3 p3   // 終了点
    );

    // t（0.0～1.0）における位置を取得
    DirectX::XMFLOAT3 GetPosition(float t) const;

    // t における接線ベクトル（進行方向）を取得
    DirectX::XMFLOAT3 GetTangent(float t) const;

private:
    DirectX::XMFLOAT3 m_p0;  // 開始点
    DirectX::XMFLOAT3 m_p1;  // 制御点1
    DirectX::XMFLOAT3 m_p2;  // 制御点2
    DirectX::XMFLOAT3 m_p3;  // 終了点
};