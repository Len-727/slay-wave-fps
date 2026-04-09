// StunRing.cpp - �X�^�������O�̏������ƕ`��
#include "StunRing.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

StunRing::StunRing() {}
StunRing::~StunRing() {}

// ============================================================
// �������i�f�o�C�X�쐬���1�񂾂��Ăԁj
// ============================================================
bool StunRing::Initialize(ID3D11Device* device)
{
    if (!CreateShaders(device))  return false;
    if (!CreateQuad(device))     return false;
    if (!CreateConstantBuffer(device)) return false;

    // === ���Z�u�����h�X�e�[�g ===
    // �����O�̌����u�����v�i���̐F + �����O�̐F�j
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;       // �\�[�X�̃A���t�@
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;            // ���Z�I
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());

    // === �[�x�X�e�[�g�i�[�x�e�X�g�͂��邪�������݂͂��Ȃ��j===
    // �G�̌��ɉB��邯�ǁA�����O���̂��[�x�������Ȃ�
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // �������܂Ȃ�
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    device->CreateDepthStencilState(&depthDesc, m_depthState.GetAddressOf());

    // === ���X�^���C�U�i���ʕ`��j===
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;  // ���ʂ��`��i�r���{�[�h�Ȃ̂Łj
    device->CreateRasterizerState(&rasterDesc, m_rasterState.GetAddressOf());

    m_initialized = true;
    OutputDebugStringA("[StunRing] Initialize complete!\n");
    return true;
}

// ============================================================
// �V�F�[�_�[�̃R���p�C��
// ============================================================
bool StunRing::CreateShaders(ID3D11Device* device)
{
    // ========================================
    // �y�����z�X�^�������O�p�V�F�[�_�[�� .cso ����ǂݍ���
    // �y���R�z�����^�C���R���p�C���p�~ �� �N�����������z�z���� .hlsl �s�v
    // ========================================

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // --- ���_�V�F�[�_�[ ---
    hr = D3DReadFileToBlob(L"Assets/Shaders/StunRingVS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[StunRing] StunRingVS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_vs.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    // --- ���̓��C�A�E�g ---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        m_inputLayout.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    // --- �s�N�Z���V�F�[�_�[ ---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/StunRingPS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[StunRing] StunRingPS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_ps.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    OutputDebugStringA("[StunRing] Shaders loaded from CSO\n");
    return true;
}
// ============================================================
// �N���b�h�i�l�p�`�j�쐬
// ============================================================
bool StunRing::CreateQuad(ID3D11Device* device)
{
    // �r���{�[�h�p�̎l�p�`�i2�O�p�` = 6���_�j
    // Position: -1?+1, TexCoord: 0?1
    RingVertex vertices[] = {
        // �O�p�`1
        { XMFLOAT3(-1,  1, 0), XMFLOAT2(0, 0) },  // ����
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },  // �E��
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },  // ����
        // �O�p�`2
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },  // �E��
        { XMFLOAT3(1, -1, 0), XMFLOAT2(1, 1) },  // �E��
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },  // ����
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    HRESULT hr = device->CreateBuffer(&vbDesc, &initData, m_vertexBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

// ============================================================
// �萔�o�b�t�@�쐬
// ============================================================
bool StunRing::CreateConstantBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(RingCB);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
    return SUCCEEDED(hr);
}

// ============================================================
// �����O��1�`��
// ============================================================
void StunRing::Render(
    ID3D11DeviceContext* context,
    XMFLOAT3 enemyPos,
    float ringSize,
    float time,
    XMMATRIX view,
    XMMATRIX projection)
{
    if (!m_initialized) return;

    // --- �萔�o�b�t�@�X�V ---
    RingCB cb;
    cb.View = XMMatrixTranspose(view);
    cb.Projection = XMMatrixTranspose(projection);
    cb.EnemyPos = enemyPos;
    cb.RingSize = ringSize;
    cb.Time = time;
    cb.Padding = XMFLOAT3(0, 0, 0);
    context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // --- �����_�����O�X�e�[�g��ۑ� ---
    // �i���̕`��ɉe�����Ȃ��悤�ɁA��Ŗ߂��j
    Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlend;
    FLOAT prevBlendFactor[4];
    UINT prevSampleMask;
    context->OMGetBlendState(prevBlend.GetAddressOf(), prevBlendFactor, &prevSampleMask);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> prevDepth;
    UINT prevStencilRef;
    context->OMGetDepthStencilState(prevDepth.GetAddressOf(), &prevStencilRef);

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> prevRaster;
    context->RSGetState(prevRaster.GetAddressOf());

    // --- �p�C�v���C���Z�b�g ---
    context->VSSetShader(m_vs.Get(), nullptr, 0);
    context->PSSetShader(m_ps.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());

    // �萔�o�b�t�@��VS��PS�����ɃZ�b�g
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // ���Z�u�����h + �[�x��������OFF + ���ʕ`��
    float blendFactor[] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthState.Get(), 0);
    context->RSSetState(m_rasterState.Get());

    // ���_�o�b�t�@
    UINT stride = sizeof(RingVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- �`��I ---
    context->Draw(6, 0);

    // --- �����_�����O�X�e�[�g��߂� ---
    context->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
    context->OMSetDepthStencilState(prevDepth.Get(), prevStencilRef);
    context->RSSetState(prevRaster.Get());
}