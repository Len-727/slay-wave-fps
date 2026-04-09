// ============================================================
//  KeyPrompt.cpp - �L�[�v�����v�g �r���{�[�h�̎���
//
//  �y�݌v���f�z
//  StunRingVS.cso ���ė��p���A�e�N�X�`���pPS�����V�K�B
//  StunRing �Ƃ͕ʃN���X�ɂ��ĐӖ��𕪗��B
//  �u�`���V�F�[�_�[�ŕ`���iStunRing�j�v��
//  �u�e�N�X�`����\��iKeyPrompt�j�v�͕ʂ̎d���B
// ============================================================
#include "KeyPrompt.h"
#include <WICTextureLoader.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

KeyPrompt::KeyPrompt() {}
KeyPrompt::~KeyPrompt() {}

// ============================================================
//  Initialize - �S���\�[�X�̏�����
// ============================================================
bool KeyPrompt::Initialize(ID3D11Device* device, const wchar_t* texturePath)
{
    if (!CreateShaders(device))        return false;
    if (!CreateQuad(device))           return false;
    if (!CreateConstantBuffer(device)) return false;
    if (!LoadTexture(device, texturePath)) return false;

    // === �A���t�@�u�����h�X�e�[�g ===
    // �ʏ�̃A���t�@�u�����h�iSrcAlpha * Src + InvSrcAlpha * Dest�j
    // StunRing�̉��Z�u�����h�Ƃ͈قȂ�B�e�N�X�`���̔������𐳂����`�����߁B
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());

    // === �[�x�X�e�[�g�i�e�X�g���������݂�OFF�j===
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    device->CreateDepthStencilState(&depthDesc, m_depthState.GetAddressOf());

    // === ���X�^���C�U�i���ʕ`��j===
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    device->CreateRasterizerState(&rasterDesc, m_rasterState.GetAddressOf());

    // === �e�N�X�`���T���v���[ ===
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // �o�C���j�A�t�B���^
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf());

    m_initialized = true;
    OutputDebugStringA("[KeyPrompt] Initialize complete!\n");
    return true;
}

// ============================================================
//  �V�F�[�_�[�̓ǂݍ���
// ============================================================
bool KeyPrompt::CreateShaders(ID3D11Device* device)
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // --- ���_�V�F�[�_�[: StunRingVS.cso ���ė��p ---
    hr = D3DReadFileToBlob(L"Assets/Shaders/StunRingVS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[KeyPrompt] StunRingVS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_vs.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- ���̓��C�A�E�g�iStunRing�Ɠ����j---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = device->CreateInputLayout(
        layout, ARRAYSIZE(layout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        m_inputLayout.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- �s�N�Z���V�F�[�_�[: KeyPromptPS.cso ---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/KeyPromptPS.cso", &blob);
    if (FAILED(hr))
    {
        OutputDebugStringA("[KeyPrompt] KeyPromptPS.cso load FAILED!\n");
        return false;
    }

    hr = device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_ps.GetAddressOf());
    if (FAILED(hr)) return false;

    OutputDebugStringA("[KeyPrompt] Shaders loaded OK\n");
    return true;
}

// ============================================================
//  �N���b�h�쐬�iStunRing�Ɠ���6���_�j
// ============================================================
bool KeyPrompt::CreateQuad(ID3D11Device* device)
{
    BillboardVertex vertices[] = {
        { XMFLOAT3(-1,  1, 0), XMFLOAT2(0, 0) },
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },
        { XMFLOAT3(1,  1, 0), XMFLOAT2(1, 0) },
        { XMFLOAT3(1, -1, 0), XMFLOAT2(1, 1) },
        { XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    return SUCCEEDED(device->CreateBuffer(&vbDesc, &initData, m_vertexBuffer.GetAddressOf()));
}

// ============================================================
//  �萔�o�b�t�@�쐬
// ============================================================
bool KeyPrompt::CreateConstantBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(PromptCB);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    return SUCCEEDED(device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf()));
}

// ============================================================
//  �e�N�X�`���ǂݍ���
// ============================================================
bool KeyPrompt::LoadTexture(ID3D11Device* device, const wchar_t* path)
{
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        device, path, nullptr, m_texture.GetAddressOf());

    if (FAILED(hr))
    {
        OutputDebugStringA("[KeyPrompt] Texture load FAILED!\n");
        return false;
    }

    OutputDebugStringA("[KeyPrompt] Texture loaded OK\n");
    return true;
}

// ============================================================
//  �r���{�[�h�`��
// ============================================================
void KeyPrompt::Render(
    ID3D11DeviceContext* context,
    XMFLOAT3 worldPos,
    float size,
    float time,
    XMMATRIX view,
    XMMATRIX projection)
{
    if (!m_initialized) return;

    // --- �萔�o�b�t�@�X�V ---
    PromptCB cb;
    cb.View = XMMatrixTranspose(view);
    cb.Projection = XMMatrixTranspose(projection);
    cb.EnemyPos = worldPos;
    cb.Size = size;
    cb.Time = time;
    cb.Padding = XMFLOAT3(0, 0, 0);
    context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // --- �����_�����O�X�e�[�g��ۑ� ---
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

    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // �e�N�X�`���ƃT���v���[���Z�b�g
    context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

    // �u�����h + �[�x + ���X�^���C�U
    float blendFactor[] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_depthState.Get(), 0);
    context->RSSetState(m_rasterState.Get());

    // ���_�o�b�t�@
    UINT stride = sizeof(BillboardVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- �`�� ---
    context->Draw(6, 0);

    // --- �e�N�X�`�����N���A�i���̕`��ɉe�����Ȃ��悤�Ɂj---
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    // --- �����_�����O�X�e�[�g��߂� ---
    context->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
    context->OMSetDepthStencilState(prevDepth.Get(), prevStencilRef);
    context->RSSetState(prevRaster.Get());
}
