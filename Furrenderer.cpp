// =============================================================
// FurRenderer.cpp - �t�@�[/�ۃ����_���[�̎���
// =============================================================

#include "FurRenderer.h"
#include <d3dcompiler.h>    // D3DCompileFromFile
#include <vector>

// d3dcompiler.lib �������N
#pragma comment(lib, "d3dcompiler.lib")

// =============================================================
// �R���X�g���N�^
// =============================================================
FurRenderer::FurRenderer()
    : m_shellCount(12)
    , m_furLength(0.08f)     // �ۂ̍����i���[�g���P�ʁj
    , m_furDensity(0.80f)     // ���x�i0.0?1.0�j
    , m_indexCount(0)
{
}

// =============================================================
// Initialize - ������
// =============================================================
bool FurRenderer::Initialize(ID3D11Device* device)
{
    if (!CompileShaders(device))
    {
        //OutputDebugStringA("[FUR] Shader compilation FAILED\n");
        return false;
    }

    if (!CreateGroundQuad(device))
    {
        //OutputDebugStringA("[FUR] Ground quad creation FAILED\n");
        return false;
    }

    // --- �萔�o�b�t�@�쐬 ---
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(FurCB);           // �o�b�t�@�T�C�Y
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;          // CPU���������\
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
    if (FAILED(hr))
    {
       // OutputDebugStringA("[FUR] Constant buffer creation FAILED\n");
        return false;
    }

    // --- �A���t�@�u�����h�X�e�[�g ---
    // �т̐�[���������ŉ��̑w�������Č�����K�v������
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;        // �\�[�X�̃A���t�@
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   // 1-�\�[�X�A���t�@
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&blendDesc, m_alphaBlendState.GetAddressOf());
    if (FAILED(hr))
    {
        //OutputDebugStringA("[FUR] Blend state creation FAILED\n");
        return false;
    }

    // --- ���ʕ`��i�J�����O�Ȃ��j---
    // �ۂ͔����w�Ȃ̂ŗ��ʂ�������K�v������
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;    // ���ʕ`��
    rsDesc.DepthClipEnable = TRUE;

    hr = device->CreateRasterizerState(&rsDesc, m_noCullState.GetAddressOf());
    if (FAILED(hr))
    {
        //OutputDebugStringA("[FUR] Rasterizer state creation FAILED\n");
        return false;
    }

    // --- �[�x�e�X�g����E�������݂Ȃ� ---
    // ���̃I�u�W�F�N�g�̑O��֌W�͕ۂ��A�t�@�[�w���m�͐[�x�������Ȃ�
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;                          // �[�x�e�X�gON
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // �[�x��������OFF
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    hr = device->CreateDepthStencilState(&dsDesc, m_depthWriteOff.GetAddressOf());
    if (FAILED(hr))
    {
        ////OutputDebugStringA("[FUR] Depth stencil state creation FAILED\n");
        return false;
    }

    OutputDebugStringA("[FUR] Initialized successfully!\n");
    return true;
}

// =============================================================
// CompileShaders - HLSL�t�@�C������V�F�[�_�[���R���p�C��
// =============================================================
bool FurRenderer::CompileShaders(ID3D11Device* device)
{
    HRESULT hr;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;

    // --- ���_�V�F�[�_�[�̃R���p�C�� ---
    hr = D3DCompileFromFile(
        L"FurVS.hlsl",  // �t�@�C���p�X
        nullptr,                        // �}�N����`
        nullptr,                        // �C���N���[�h
        "main",                         // �G���g���[�|�C���g
        "vs_5_0",                       // �V�F�[�_�[���f��
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,  // �f�o�b�O�p�t���O
        0,
        vsBlob.GetAddressOf(),
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            char buf[1024];
            sprintf_s(buf, "[FUR] VS compile error: %s\n",
                (char*)errorBlob->GetBufferPointer());
            //OutputDebugStringA(buf);
        }
        return false;
    }

    // --- �s�N�Z���V�F�[�_�[�̃R���p�C�� ---
    hr = D3DCompileFromFile(
        L"FurPS.hlsl",
        nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        psBlob.GetAddressOf(),
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            char buf[1024];
            sprintf_s(buf, "[FUR] PS compile error: %s\n",
                (char*)errorBlob->GetBufferPointer());
            //OutputDebugStringA(buf);
        }
        return false;
    }

    // --- �V�F�[�_�[�I�u�W�F�N�g�쐬 ---
    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        m_vertexShader.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        m_pixelShader.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    // --- ���̓��C�A�E�g ---
    // ���_�V�F�[�_�[���u�ǂ�ȃf�[�^�����邩�v��m�邽�߂̒�`
    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(
        inputDesc,
        3,                                // �v�f��
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        m_inputLayout.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    //OutputDebugStringA("[FUR] Shaders compiled OK\n");
    return true;
}

// =============================================================
// CreateGroundQuad - �n�ʂ̎l�p�`���b�V�����쐬
// 
// �����̒n�ʁi50x50�j��菭���傫�߂̃N�A�b�h�����
// �@���͏����(0,1,0)�AUV��0?1
//
// =============================================================
bool FurRenderer::CreateGroundQuad(ID3D11Device* device)
{
    // �n�ʂ̃T�C�Y�iMapSystem��Floor�ɍ��킹��F50x50�j
    float halfSize = 25.0f;
    float groundY = 0.0f;  // �n�ʂ̍����i�n�ʂ�Box�̏�ʂɍ��킹��j

    // --- ���_�f�[�^ ---
    FurVertex vertices[] = {
        // Position                        Normal           TexCoord
        { {-halfSize, groundY, -halfSize}, {0, 1, 0},      {0, 0} },  // ����
        { { halfSize, groundY, -halfSize}, {0, 1, 0},      {1, 0} },  // �E��
        { {-halfSize, groundY,  halfSize}, {0, 1, 0},      {0, 1} },  // ����O
        { { halfSize, groundY,  halfSize}, {0, 1, 0},      {1, 1} },  // �E��O
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

    // --- �C���f�b�N�X�f�[�^�i�O�p�`2�Ŏl�p�`�j---
    UINT indices[] = {
        0, 1, 2,    // �O�p�`1
        1, 3, 2     // �O�p�`2
    };
    m_indexCount = 6;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;

    hr = device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

   // OutputDebugStringA("[FUR] Ground quad created OK\n");
    return true;
}

// =============================================================
// DrawGroundMoss - �n�ʂɑۂ�`��
//
// �y�d�g�݁z�����N�A�b�h��m_shellCount��`�悷��
// �e��� CurrentLayer �̒l��ς��邱�ƂŁA
// �@�������ւ̉����o���ʂ��ς�� �� �w���ςݏd�Ȃ�
// =============================================================
void FurRenderer::DrawGroundMoss(
    ID3D11DeviceContext* context,
    DirectX::XMMATRIX view,
    DirectX::XMMATRIX projection,
    float elapsedTime)
{
    // --- ���݂̕`��X�e�[�g��ۑ� ---
    ComPtr<ID3D11BlendState> prevBlend;
    FLOAT prevBlendFactor[4];
    UINT prevSampleMask;
    context->OMGetBlendState(prevBlend.GetAddressOf(), prevBlendFactor, &prevSampleMask);

    ComPtr<ID3D11RasterizerState> prevRS;
    context->RSGetState(prevRS.GetAddressOf());

    ComPtr<ID3D11DepthStencilState> prevDS;
    UINT prevStencilRef;
    context->OMGetDepthStencilState(prevDS.GetAddressOf(), &prevStencilRef);

    // --- �`��X�e�[�g��ݒ� ---
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
    context->RSSetState(m_noCullState.Get());
    context->OMSetDepthStencilState(m_depthWriteOff.Get(), 0);

    // --- �V�F�[�_�[���Z�b�g ---
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(m_inputLayout.Get());

    // --- ���_�o�b�t�@���Z�b�g ---
    UINT stride = sizeof(FurVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- �萔�o�b�t�@���Z�b�g ---
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // --- ���[���h�s��i�n�ʂ͌��_�ɔz�u�j---
    DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

    // ==============================================
    // �e�w��`��
    // ==============================================
    for (int i = 0; i < m_shellCount; i++)
    {
        // --- �萔�o�b�t�@���X�V ---
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) continue;

        FurCB* cb = (FurCB*)mapped.pData;

        // �s����i�[�iXMMATRIX����XMFLOAT4X4�ɕϊ��j
        DirectX::XMStoreFloat4x4(&cb->World, DirectX::XMMatrixTranspose(world));
        DirectX::XMStoreFloat4x4(&cb->View, DirectX::XMMatrixTranspose(view));
        DirectX::XMStoreFloat4x4(&cb->Projection, DirectX::XMMatrixTranspose(projection));

        // �t�@�[�p�����[�^
        cb->FurLength = m_furLength;
        cb->CurrentLayer = (float)i;
        cb->TotalLayers = (float)m_shellCount;
        cb->Time = elapsedTime;

        // ���i�ɂ₩�ɗh�����x�j
        cb->WindDirection = DirectX::XMFLOAT3(1.0f, 0.0f, 0.3f);
        cb->WindStrength = 0.015f;

        // �ۂ̐F�i�S�V�b�N�ȈÂ���?���邢�΁j
        cb->BaseColor = DirectX::XMFLOAT3(0.05f, 0.12f, 0.03f);  // �����F�Â���
        cb->FurDensity = m_furDensity;
        cb->TipColor = DirectX::XMFLOAT3(0.15f, 0.35f, 0.08f);   // ��[�F���邢��
        cb->Padding2 = 0.0f;

        // ���C�e�B���O
        cb->LightDir = DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f);
        cb->AmbientStrength = 0.4f;

        context->Unmap(m_constantBuffer.Get(), 0);

        // --- ���̑w��`�� ---
        context->DrawIndexed(m_indexCount, 0, 0);
    }

    // --- �`��X�e�[�g�𕜌� ---
    context->OMSetBlendState(prevBlend.Get(), prevBlendFactor, prevSampleMask);
    context->RSSetState(prevRS.Get());
    context->OMSetDepthStencilState(prevDS.Get(), prevStencilRef);
}
