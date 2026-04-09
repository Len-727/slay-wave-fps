#pragma once
#include <DirectXMath.h>

// ========================================
// �x�W�F�Ȑ��N���X�i3���x�W�F�Ȑ��j
// ========================================

class BezierCurve
{
public:
    BezierCurve();
    ~BezierCurve();

    // ����_��ݒ�
    void SetControlPoints(
        DirectX::XMFLOAT3 p0,  // �J�n�_
        DirectX::XMFLOAT3 p1,  // ����_1
        DirectX::XMFLOAT3 p2,  // ����_2
        DirectX::XMFLOAT3 p3   // �I���_
    );

    // t�i0.0�`1.0�j�ɂ�����ʒu���擾
    DirectX::XMFLOAT3 GetPosition(float t) const;

    // t �ɂ�����ڐ��x�N�g���i�i�s�����j���擾
    DirectX::XMFLOAT3 GetTangent(float t) const;

private:
    DirectX::XMFLOAT3 m_p0;  // �J�n�_
    DirectX::XMFLOAT3 m_p1;  // ����_1
    DirectX::XMFLOAT3 m_p2;  // ����_2
    DirectX::XMFLOAT3 m_p3;  // �I���_
};