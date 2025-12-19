#include "BezierCurve.h"

using namespace DirectX;

BezierCurve::BezierCurve()
    : m_p0(0.0f, 0.0f, 0.0f)
    , m_p1(0.0f, 0.0f, 0.0f)
    , m_p2(0.0f, 0.0f, 0.0f)
    , m_p3(0.0f, 0.0f, 0.0f)
{
}

BezierCurve::~BezierCurve()
{
}

void BezierCurve::SetControlPoints(
    XMFLOAT3 p0,
    XMFLOAT3 p1,
    XMFLOAT3 p2,
    XMFLOAT3 p3)
{
    m_p0 = p0;
    m_p1 = p1;
    m_p2 = p2;
    m_p3 = p3;
}

XMFLOAT3 BezierCurve::GetPosition(float t) const
{
   
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;

    // === 3次ベジェ曲線の計算 ===
    // B(t) = (1-t)?P? + 3(1-t)?tP? + 3(1-t)t?P? + t?P?

    float t2 = t * t;        // t?
    float t3 = t2 * t;       // t?
    float mt = 1.0f - t;     // (1-t)
    float mt2 = mt * mt;     // (1-t)?
    float mt3 = mt2 * mt;    // (1-t)?

    // 各項の係数
    float c0 = mt3;          // (1-t)?
    float c1 = 3.0f * mt2 * t;  // 3(1-t)?t
    float c2 = 3.0f * mt * t2;  // 3(1-t)t?
    float c3 = t3;           // t?

    // 各軸を計算
    XMFLOAT3 result;
    result.x = c0 * m_p0.x + c1 * m_p1.x + c2 * m_p2.x + c3 * m_p3.x;
    result.y = c0 * m_p0.y + c1 * m_p1.y + c2 * m_p2.y + c3 * m_p3.y;
    result.z = c0 * m_p0.z + c1 * m_p1.z + c2 * m_p2.z + c3 * m_p3.z;

    return result;
}

XMFLOAT3 BezierCurve::GetTangent(float t) const
{
   
    t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;

    // === ベジェ曲線の微分（接線ベクトル）===
    // B'(t) = 3(1-t)?(P?-P?) + 6(1-t)t(P?-P?) + 3t?(P?-P?)

    float t2 = t * t;
    float mt = 1.0f - t;
    float mt2 = mt * mt;

    float c0 = 3.0f * mt2;
    float c1 = 6.0f * mt * t;
    float c2 = 3.0f * t2;

    XMFLOAT3 d0, d1, d2;
    d0.x = m_p1.x - m_p0.x;
    d0.y = m_p1.y - m_p0.y;
    d0.z = m_p1.z - m_p0.z;

    d1.x = m_p2.x - m_p1.x;
    d1.y = m_p2.y - m_p1.y;
    d1.z = m_p2.z - m_p1.z;

    d2.x = m_p3.x - m_p2.x;
    d2.y = m_p3.y - m_p2.y;
    d2.z = m_p3.z - m_p2.z;

    XMFLOAT3 tangent;
    tangent.x = c0 * d0.x + c1 * d1.x + c2 * d2.x;
    tangent.y = c0 * d0.y + c1 * d1.y + c2 * d2.y;
    tangent.z = c0 * d0.z + c1 * d1.z + c2 * d2.z;

    // 正規化
    XMVECTOR v = XMLoadFloat3(&tangent);
    v = XMVector3Normalize(v);
    XMStoreFloat3(&tangent, v);

    return tangent;
}