// ============================================================
//  GPUParticleSystem.cpp
//  GPU Compute Shader�ɂ�錌�p�[�e�B�N���V�X�e��
// ============================================================

#include "GPUParticleSystem.h"
#include <d3dcompiler.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <WICTextureLoader.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// ============================================================
//  �R���X�g���N�^
// ============================================================
GPUParticleSystem::GPUParticleSystem()
{
    m_emitQueue.reserve(512);  // ���˃L���[�̏����e��
}

// ============================================================
//  ������
// ============================================================
bool GPUParticleSystem::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
    int maxParticles, int screenWidth, int screenHeight)
{
    m_device = device;
    m_context = context;
    m_maxParticles = maxParticles;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    // �V�F�[�_�[�R���p�C��
    if (!CompileShaders())
    {
        OutputDebugStringA("[GPUParticle] Shader compile FAILED!\n");
        return false;
    }

    
    // GPU�o�b�t�@�쐬
    if (!CreateBuffers())
    {
        OutputDebugStringA("[GPUParticle] Buffer creation FAILED!\n");
        return false;
    }

    // �u�����h/�[�x�X�e�[�g�쐬
    if (!CreateStates())
    {
        OutputDebugStringA("[GPUParticle] State creation FAILED!\n");
        return false;
    }

    char msg[128];
    sprintf_s(msg, "[GPUParticle] Initialized OK! Max particles: %d\n", m_maxParticles);
    OutputDebugStringA(msg);

    //  ���e�N�X�`���ǂݍ���
    {
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_device,
            L"Assets/Texture/blood_mist_v2.png",
            nullptr,
            m_bloodMistSRV.GetAddressOf()
        );
        if (SUCCEEDED(hr))
            OutputDebugStringA("[GPUParticle] Blood MIST texture loaded!\n");
        else
            OutputDebugStringA("[GPUParticle] WARNING: mist texture NOT found!\n");
    }

    //  �t�̃X�v���b�V���e�N�X�`���ǂݍ���
    {
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            m_device,
            L"Assets/Texture/blood_splash_v2.png",
            nullptr,
            m_bloodSplashSRV.GetAddressOf()
        );
        if (SUCCEEDED(hr))
            OutputDebugStringA("[GPUParticle] Blood SPLASH texture loaded!\n");
        else
            OutputDebugStringA("[GPUParticle] WARNING: splash texture NOT found!\n");
    }

    //  ���j�A�T���v���[�쐬
   // �y�������Ă���H�z
   // �e�N�X�`������s�N�Z����ǂނƂ��́u��ԃ��[���v���`����B
   //
   // Filter = LINEAR �� �אڃs�N�Z�������炩�ɕ�ԁi�ڂ������ʁj
   //          POINT �ɂ���ƕ�ԂȂ��i�h�b�g�G���A�J�N�J�N�j
   //
   // AddressU/V = CLAMP �� �e�N�X�`���̒[���͂ݏo������[�̐F���g��
   //              WRAP �ɂ���ƌJ��Ԃ��i�^�C����j
    {
        D3D11_SAMPLER_DESC sd = {};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // ���炩���
        sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;      // �͂ݏo���h�~
        sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;

        HRESULT hr = m_device->CreateSamplerState(&sd, m_linearSampler.GetAddressOf());
        if (SUCCEEDED(hr))
            OutputDebugStringA("[GPUParticle] Linear sampler created!\n");
    }



    // // ���̃����_�����O�p���\�[�X(���s���Ă��ʏ�`��Ƀt�H�[���o�b�N)
    if (CompileFluidShaders())
    {
        CreateFluidResources();
        OutputDebugStringA("[GPUParticle] Fluid rendering READY!\n");
    }
    else
    {
        OutputDebugStringA("[GPUParticle] Fluid shaders failed - billboard fallback\n");
    }

    return true;
}

bool GPUParticleSystem::CompileShaders()
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // === Compute Shader (�������Z) ===
    hr = D3DReadFileToBlob(L"Assets/Shaders/BloodParticleCS.cso", blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GPUParticle] BloodParticleCS.cso load failed\n");
        return false;
    }
    hr = m_device->CreateComputeShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_computeShader.GetAddressOf());
    if (FAILED(hr)) return false;
    OutputDebugStringA("[GPUParticle] CS loaded from CSO\n");

    // === Vertex Shader (�r���{�[�h�W�J) ===
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/BloodParticleVS.cso", blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GPUParticle] BloodParticleVS.cso load failed\n");
        return false;
    }
    hr = m_device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_vertexShader.GetAddressOf());
    if (FAILED(hr)) return false;
    OutputDebugStringA("[GPUParticle] VS loaded from CSO\n");

    // === Pixel Shader (���̕`��) ===
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/BloodParticlePS.cso", blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GPUParticle] BloodParticlePS.cso load failed\n");
        return false;
    }
    hr = m_device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_pixelShader.GetAddressOf());
    if (FAILED(hr)) return false;
    OutputDebugStringA("[GPUParticle] PS loaded from CSO\n");

    return true;
}

// ============================================================
//  GPU�o�b�t�@�쐬
// ============================================================
bool GPUParticleSystem::CreateBuffers()
{
    HRESULT hr;

    // === �p�[�e�B�N�� StructuredBuffer(GPU�ǂݏ���) ===
    {
        // �S�p�[�e�B�N�������S���(life=0)�ŏ�����
        std::vector<Particle> initData(m_maxParticles);
        memset(initData.data(), 0, sizeof(Particle) * m_maxParticles);

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Particle) * m_maxParticles;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(Particle);  // 1�v�f�̃T�C�Y

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = initData.data();

        hr = m_device->CreateBuffer(&desc, &init, m_particleBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // UAV(Compute Shader�p: �ǂݏ����\)
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = m_maxParticles;

        hr = m_device->CreateUnorderedAccessView(
            m_particleBuffer.Get(), &uavDesc, m_particleUAV.GetAddressOf());
        if (FAILED(hr)) return false;

        // SRV(Vertex Shader�p: �ǂݎ���p)
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements = m_maxParticles;

        hr = m_device->CreateShaderResourceView(
            m_particleBuffer.Get(), &srvDesc, m_particleSRV.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === �X�e�[�W���O�o�b�t�@(CPU->GPU�A�b�v���[�h�p) ===
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Particle) * m_maxParticles;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

        hr = m_device->CreateBuffer(&desc, nullptr, m_stagingBuffer.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === �C���f�b�N�X�o�b�t�@(1�N���b�h = 6�C���f�b�N�X = �O�p�`2��) ===
    {
        std::vector<uint32_t> indices(m_maxParticles * 6);
        for (int i = 0; i < m_maxParticles; i++)
        {
            uint32_t base = i * 4;  // 4���_����
            indices[i * 6 + 0] = base + 0;  // �O�p�`1: ����-�E��-����
            indices[i * 6 + 1] = base + 1;
            indices[i * 6 + 2] = base + 2;
            indices[i * 6 + 3] = base + 1;  // �O�p�`2: �E��-�E��-����
            indices[i * 6 + 4] = base + 3;
            indices[i * 6 + 5] = base + 2;
        }

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(sizeof(uint32_t) * indices.size());
        desc.Usage = D3D11_USAGE_IMMUTABLE;  // ��x�������ύX���Ȃ�
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = indices.data();

        hr = m_device->CreateBuffer(&desc, &init, m_indexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === Compute Shader�p�̒萔�o�b�t�@ ===
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(UpdateCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;      // ���t���[����������
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&desc, nullptr, m_updateCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === �J�����p�̒萔�o�b�t�@ ===
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(CameraCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&desc, nullptr, m_cameraCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    return true;
}

// ============================================================
//  �u�����h/�[�x�X�e�[�g�쐬
// ============================================================
bool GPUParticleSystem::CreateStates()
{
    HRESULT hr;

    // === �A���t�@�u�����h(�������̌�) ===
    {
        D3D11_BLEND_DESC desc = {};
        desc.AlphaToCoverageEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;       // �\�[�X�̃A���t�@
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   // 1-�\�[�X�̃A���t�@
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;          // ���Z
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = m_device->CreateBlendState(&desc, m_blendState.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // === �[�x: �ǂݎ���p(�p�[�e�B�N���͔������Ȃ̂Ő[�x�������Ȃ�) ===
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // �������ݖ���
        desc.DepthFunc = D3D11_COMPARISON_LESS;

        hr = m_device->CreateDepthStencilState(&desc, m_depthState.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    return true;
}

// ============================================================
//  �p�[�e�B�N������(CPU�� - ����Update��GPU�ɃA�b�v���[�h)
// ============================================================
void GPUParticleSystem::Emit(XMFLOAT3 position, int count, float power, float sizeMin, float sizeMax)
{
    for (int i = 0; i < count; i++)
    {
        Particle p = {};
        p.position = position;

        // �����_���ȕ��˕���(�㔼��)
        float theta = ((float)rand() / RAND_MAX) * 6.28318f;          // �����p(0~360�x)
        float phi = ((float)rand() / RAND_MAX) * 3.14159f * 0.5f;   // �p(0~90�x)
        float speed = power * (0.5f + ((float)rand() / RAND_MAX) * 1.0f);

        p.velocity.x = cosf(theta) * sinf(phi) * speed * 1.5f;
        p.velocity.y = cosf(phi) * speed * 1.2f + 3.0f;   // ��ɋ�����΂�
        p.velocity.z = sinf(theta) * sinf(phi) * speed * 1.5f;

        p.life = 1.5f + ((float)rand() / RAND_MAX) * 2.5f;   // 1.5~4�b
        p.maxLife = p.life;
        p.size = sizeMin + ((float)rand() / RAND_MAX) * (sizeMax - sizeMin);

        m_emitQueue.push_back(p);
    }
}

// ============================================================
//  EmitSplash: ���̔򖗁i�ׂ�����юU��j
//
//  �y�C���[�W�z
//  �����D�����ꂽ�u�Ԃ̃p�V���b�I
//  �ׂ����򖗂��e�̕����Ƀo�[�b�ƒe����B
//
//  �y�p�����[�^�݌v�z
//  �T�C�Y: 0.03~0.12�i�ƂĂ������� �� �ׂ����򖗊��j
//  ��:     30~60�i���� �� ���x���j
//  ���x:   �����ipower �~ 1.5~3.0 �� �p�b�ƒe����j
//  ����:   0.3~1.0�b�i�Z�� �� �p�b�Əo�ăT�b�Ə�����j
//  ����:   �e�̐i�s�����ɕ΂� + �����_���U�炵
// ============================================================
void GPUParticleSystem::EmitSplash(XMFLOAT3 position, XMFLOAT3 direction, int count, float power)
{
    // �����x�N�g���𐳋K��
    float len = sqrtf(direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z);
    if (len < 0.001f)
    {
        direction = XMFLOAT3(0.0f, 1.0f, 0.0f);
        len = 1.0f;
    }
    float inv = 1.0f / len;
    direction.x *= inv;
    direction.y *= inv;
    direction.z *= inv;

    for (int i = 0; i < count; i++)
    {
        Particle p = {};
        p.position = position;

        // �����_���U�炵�i-1~+1�j
        float sx = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float sy = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float sz = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        // ���x: ���߂Ńo��������ipower �~ 1.5~3.0�j
        float speed = power * (1.5f + ((float)rand() / RAND_MAX) * 1.5f);

        // ���C������ + �傫�߂̎U�炵 �� �e���銴��
        p.velocity.x = direction.x * speed + sx * power * 1.5f;
        p.velocity.y = direction.y * speed + sy * power * 1.0f + 2.0f;
        p.velocity.z = direction.z * speed + sz * power * 1.5f;

        // ����: �Z���i0.3~1.0�b�j�� �p�b�Ə�����
        p.life = 0.5f + ((float)rand() / RAND_MAX) * 1.0f;
        p.maxLife = p.life;

        // �T�C�Y: �������i0.03~0.12�j�� �ׂ�����
        p.size = 0.001f + ((float)rand() / RAND_MAX) * 0.17f;

        m_emitQueue.push_back(p);
    }
}

// ============================================================
//  EmitMist: ���̖��i�ӂ���ƍL����Ԃ��Ɂj
//
//  �y�C���[�W�z
//  �Ԃ��X�v���[���V���b�Ɛ����������B
//  �ׂ������q���S�����ɂӂ�?���ƍL�����ĕY���B
//
//  �y�p�����[�^�݌v�z
//  �T�C�Y: 0.02~0.08�i�ɏ� �� �����ۂ��j
//  ��:     80~200�i��� �� ���x�Ŗ������o���j
//  ���x:   �x���ipower �~ 0.3~1.0 �� �������L����j
//  ����:   1.0~3.0�b�i���� �� ����?���Ə�����j
//  ����:   �S�����ϓ��i���ʃ����_���j
// ============================================================
void GPUParticleSystem::EmitMist(XMFLOAT3 position, int count, float power)
{
    for (int i = 0; i < count; i++)
    {
        Particle p = {};
        p.position = position;

        // --- ���ʏ�̋ϓ������_������ ---
        // theta: 0~2�΁i�����p�j
        // phi:   acos(2r-1)�i�p�A�ϓ����z�ɂ��邽�߂̃e�N�j�b�N�j
        float theta = ((float)rand() / RAND_MAX) * 6.2832f;
        float u = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float phi = acosf(u);

        // ���x: �x�߁ipower �~ 0.3~1.0�j�� �ӂ����
        float speed = power * (0.5f + ((float)rand() / RAND_MAX) * 1.0f);

        p.velocity.x = cosf(theta) * sinf(phi) * speed;
        p.velocity.y = cosf(phi) * speed + 0.5f;   // �����ɏ㏸�i���͏�ɍs���j
        p.velocity.z = sinf(theta) * sinf(phi) * speed;

        // ����: ���߁i1.0~3.0�b�j�� ����?����
        p.life = 1.0f + ((float)rand() / RAND_MAX) * 2.0f;
        p.maxLife = p.life;

        // �T�C�Y: �ɏ��i0.02~0.08�j�� ���̗�
        p.size = 0.04f + ((float)rand() / RAND_MAX) * 0.08f;

        m_emitQueue.push_back(p);
    }
}

// ============================================================
//  ���˃L���[��GPU�ɃA�b�v���[�h(�����O�o�b�t�@����)
// ============================================================
void GPUParticleSystem::UploadEmitQueue()
{
    if (m_emitQueue.empty()) return;

    int count = (int)m_emitQueue.size();
    if (count > m_maxParticles) count = m_maxParticles;

    // === �����O�o�b�t�@�̘A���̈悲�Ƃ�UpdateSubresource ===
    // GPU��CPU�����R�s�[�s�v�I����GPU�o�b�t�@�ɏ�������
    int emitIdx = 0;

    // �`�����N1: m_nextSlot �` �o�b�t�@����
    int firstChunk = (std::min)(count, m_maxParticles - m_nextSlot);
    if (firstChunk > 0)
    {
        D3D11_BOX box = {};
        box.left = m_nextSlot * sizeof(Particle);   // �o�C�g�I�t�Z�b�g�J�n
        box.right = box.left + firstChunk * sizeof(Particle); // �o�C�g�I�t�Z�b�g�I��
        box.top = 0; box.bottom = 1;
        box.front = 0; box.back = 1;

        m_context->UpdateSubresource(
            m_particleBuffer.Get(), 0, &box,
            &m_emitQueue[emitIdx], sizeof(Particle), 0);
        emitIdx += firstChunk;
    }

    // �`�����N2: �o�b�t�@�擪�ɐ܂�Ԃ��i�����O�o�b�t�@�j
    int secondChunk = count - firstChunk;
    if (secondChunk > 0)
    {
        D3D11_BOX box = {};
        box.left = 0;
        box.right = secondChunk * sizeof(Particle);
        box.top = 0; box.bottom = 1;
        box.front = 0; box.back = 1;

        m_context->UpdateSubresource(
            m_particleBuffer.Get(), 0, &box,
            &m_emitQueue[emitIdx], sizeof(Particle), 0);
    }

    m_nextSlot = (m_nextSlot + count) % m_maxParticles;
    m_emitQueue.clear();
    m_activeCount = (std::min)(m_activeCount + count, m_maxParticles);
}

// ============================================================
//  �X�V(GPU Compute Shader�����s)
// ============================================================
void GPUParticleSystem::Update(float deltaTime)
{
    m_totalTime += deltaTime;

    // ���܂��Ă���p�[�e�B�N����GPU�ɃA�b�v���[�h
    UploadEmitQueue();

    // === �萔�o�b�t�@���X�V ===
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_context->Map(m_updateCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            UpdateCBData* cb = (UpdateCBData*)mapped.pData;
            cb->DeltaTime = deltaTime;
            cb->Gravity = -6.0f;     // ���߂̏d��(���͏d��)
            cb->Drag = 0.975f;     // �y����C��R
            cb->FloorY = m_floorY;
            cb->BounceFactor = 0.35f;       // ���͂��܂�o�E���X���Ȃ�
            cb->Time = m_totalTime;
            cb->Padding[0] = 0.0f;
            cb->Padding[1] = 0.0f;
            m_context->Unmap(m_updateCB.Get(), 0);
        }
    }

    // === Compute Shader�����s ===
    m_context->CSSetShader(m_computeShader.Get(), nullptr, 0);
    m_context->CSSetConstantBuffers(0, 1, m_updateCB.GetAddressOf());

    // UAV���o�C���h(Compute Shader���ǂݏ�������o�b�t�@)
    ID3D11UnorderedAccessView* uavs[] = { m_particleUAV.Get() };
    UINT initCounts[] = { (UINT)-1 };
    m_context->CSSetUnorderedAccessViews(0, 1, uavs, initCounts);

    // �f�B�X�p�b�`: ceil(�ő�p�[�e�B�N���� / 256) �O���[�v
    UINT groups = (m_maxParticles + 255) / 256;
    m_context->Dispatch(groups, 1, 1);

    // === UAV���A���o�C���h(SRV�Ƃ��Ďg���O�ɕK�{) ===
    ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
    m_context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    m_context->CSSetShader(nullptr, nullptr, 0);
}

// ============================================================

// ============================================================
//  �X�e�[�g�ۑ�(�`��O�̃p�C�v���C����Ԃ��L��)
// ============================================================
void GPUParticleSystem::SaveState(SavedState& s)
{
    m_context->IAGetInputLayout(s.IL.GetAddressOf());
    m_context->IAGetPrimitiveTopology(&s.Topology);
    s.IBFormat = DXGI_FORMAT_UNKNOWN; s.IBOffset = 0;
    m_context->IAGetIndexBuffer(s.IB.GetAddressOf(), &s.IBFormat, &s.IBOffset);
    s.VBStride = 0; s.VBOffset = 0;
    m_context->IAGetVertexBuffers(0, 1, s.VB.GetAddressOf(), &s.VBStride, &s.VBOffset);

    m_context->VSGetShader(s.VS.GetAddressOf(), nullptr, nullptr);
    m_context->VSGetShaderResources(0, 1, s.VS_SRV.GetAddressOf());
    m_context->VSGetConstantBuffers(0, 1, s.VS_CB.GetAddressOf());

    m_context->PSGetShader(s.PS.GetAddressOf(), nullptr, nullptr);
    m_context->PSGetShaderResources(0, 1, s.PS_SRV0.GetAddressOf());
    m_context->PSGetShaderResources(1, 1, s.PS_SRV1.GetAddressOf());
    m_context->PSGetConstantBuffers(0, 1, s.PS_CB.GetAddressOf());
    m_context->PSGetSamplers(0, 1, s.PS_Sampler.GetAddressOf());

    m_context->OMGetBlendState(s.Blend.GetAddressOf(), s.BlendFactor, &s.SampleMask);
    m_context->OMGetDepthStencilState(s.Depth.GetAddressOf(), &s.StencilRef);
    m_context->OMGetRenderTargets(1, s.RTV.GetAddressOf(), s.DSV.GetAddressOf());

    m_context->RSGetState(s.RS.GetAddressOf());
}

// ============================================================
//  �X�e�[�g����(�`��O�̏�ԂɊ��S�ɖ߂�)
// ============================================================
void GPUParticleSystem::RestoreState(const SavedState& s)
{
    // �܂�SRV���N���A(�����h�~)
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    m_context->VSSetShaderResources(0, 1, nullSRV);
    m_context->PSSetShaderResources(0, 1, nullSRV);
    m_context->PSSetShaderResources(1, 1, nullSRV);

    m_context->IASetInputLayout(s.IL.Get());
    m_context->IASetPrimitiveTopology(s.Topology);
    m_context->IASetIndexBuffer(s.IB.Get(), s.IBFormat, s.IBOffset);
    UINT stride = s.VBStride, offset = s.VBOffset;
    ID3D11Buffer* vb = s.VB.Get();
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    m_context->VSSetShader(s.VS.Get(), nullptr, 0);
    m_context->VSSetShaderResources(0, 1, s.VS_SRV.GetAddressOf());
    m_context->VSSetConstantBuffers(0, 1, s.VS_CB.GetAddressOf());

    m_context->PSSetShader(s.PS.Get(), nullptr, 0);
    ID3D11ShaderResourceView* srvs[2] = { s.PS_SRV0.Get(), s.PS_SRV1.Get() };
    m_context->PSSetShaderResources(0, 2, srvs);
    m_context->PSSetConstantBuffers(0, 1, s.PS_CB.GetAddressOf());
    m_context->PSSetSamplers(0, 1, s.PS_Sampler.GetAddressOf());

    m_context->OMSetBlendState(s.Blend.Get(), s.BlendFactor, s.SampleMask);
    m_context->OMSetDepthStencilState(s.Depth.Get(), s.StencilRef);
    ID3D11RenderTargetView* rtv = s.RTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, s.DSV.Get());

    m_context->RSSetState(s.RS.Get());
}

// ============================================================
//  �ʏ�`��(�r���{�[�h���� - �X�e�[�g�ۑ�/�����t��)
// ============================================================
void GPUParticleSystem::Draw(XMMATRIX view, XMMATRIX proj, XMFLOAT3 cameraPos)
{
    // // ���̃��[�h��ON�Ȃ�ʏ�`��̓X�L�b�v(DrawFluid���g��)
    // �����ł͏]���̃r���{�[�h�`��(�t�H�[���o�b�N�p)

    SavedState saved;
    SaveState(saved);

    // �r���[�t�s�񂩂�J�����̉E/��x�N�g���擾
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMFLOAT4X4 ivF;
    XMStoreFloat4x4(&ivF, invView);
    XMFLOAT3 camRight(ivF._11, ivF._12, ivF._13);
    XMFLOAT3 camUp(ivF._21, ivF._22, ivF._23);

    // �J�����萔�o�b�t�@�X�V
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_cameraCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CameraCBData* cb = (CameraCBData*)mapped.pData;
            XMStoreFloat4x4(&cb->View, XMMatrixTranspose(view));
            XMStoreFloat4x4(&cb->Projection, XMMatrixTranspose(proj));
            cb->CameraRight = camRight;
            cb->Time = m_totalTime;
            cb->CameraUp = camUp;
            cb->SizeScale = 1.0f;
            m_context->Unmap(m_cameraCB.Get(), 0);
        }
    }

    // �p�C�v���C���ݒ�
    m_context->IASetInputLayout(nullptr);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    // // 2�̃e�N�X�`�����o�C���h
    ID3D11ShaderResourceView* mistSRV = m_bloodMistSRV.Get();
    m_context->PSSetShaderResources(1, 1, &mistSRV);      // t1 = ��

    ID3D11ShaderResourceView* splashSRV = m_bloodSplashSRV.Get();
    m_context->PSSetShaderResources(2, 1, &splashSRV);     // t2 = �t��

    m_context->VSSetShaderResources(0, 1, m_particleSRV.GetAddressOf());

    // �T���v���[
    ID3D11SamplerState* sampler = m_linearSampler.Get();
    m_context->PSSetSamplers(0, 1, &sampler);

    m_context->PSSetSamplers(0, 1, &sampler);
    m_context->VSSetConstantBuffers(0, 1, m_cameraCB.GetAddressOf());
    float bf[4] = { 0, 0, 0, 0 };
    m_context->OMSetBlendState(m_blendState.Get(), bf, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_depthState.Get(), 0);

    m_context->DrawIndexed(m_maxParticles * 6, 0, 0);

    RestoreState(saved);

    //  t1�X���b�g���N���A
   // // t1, t2�X���b�g���N���A
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_context->PSSetShaderResources(1, 1, &nullSRV);
    m_context->PSSetShaderResources(2, 1, &nullSRV);


}

bool GPUParticleSystem::CompileFluidShaders()
{
    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // --- �t���X�N���[�� VS ---
    hr = D3DReadFileToBlob(L"Assets/Shaders/FullscreenQuadVS.cso", blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GPUParticle] FullscreenQuadVS.cso load FAILED\n");
        return false;
    }
    m_device->CreateVertexShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_fullscreenVS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FullscreenVS loaded from CSO\n");

    // --- �[�x PS ---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/BloodFluidDepthPS.cso", blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GPUParticle] BloodFluidDepthPS.cso load FAILED\n");
        return false;
    }
    m_device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_fluidDepthPS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FluidDepthPS loaded from CSO\n");

    // --- �u���[ PS ---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/BloodFluidBlurPS.cso", blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GPUParticle] BloodFluidBlurPS.cso load FAILED\n");
        return false;
    }
    m_device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_fluidBlurPS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FluidBlurPS loaded from CSO\n");

    // --- ���� PS ---
    blob.Reset();
    hr = D3DReadFileToBlob(L"Assets/Shaders/BloodFluidCompositePS.cso", blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GPUParticle] BloodFluidCompositePS.cso load FAILED\n");
        return false;
    }
    m_device->CreatePixelShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, m_fluidCompositePS.GetAddressOf());
    OutputDebugStringA("[GPUParticle] FluidCompositePS loaded from CSO\n");

    m_fluidShadersReady = true;
    return true;
}

// ============================================================
//  ���̃e�N�X�`��/�o�b�t�@�쐬
// ============================================================
bool GPUParticleSystem::CreateFluidResources()
{
    HRESULT hr;

    // �Â����\�[�X�����
    m_fluidDepthTex.Reset(); m_fluidDepthRTV.Reset(); m_fluidDepthSRV.Reset();
    m_blurTempTex.Reset();   m_blurTempRTV.Reset();   m_blurTempSRV.Reset();

    // --- �[�x�e�N�X�`��(R32_FLOAT) ---
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = m_screenWidth;
        desc.Height = m_screenHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R32_FLOAT;  // 32bit���������_(�[�x�ۑ��p)
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = m_device->CreateTexture2D(&desc, nullptr, m_fluidDepthTex.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(m_fluidDepthTex.Get(), nullptr, m_fluidDepthRTV.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateShaderResourceView(m_fluidDepthTex.Get(), nullptr, m_fluidDepthSRV.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- �u���[���ԃe�N�X�`��(R32_FLOAT) ---
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = m_screenWidth;
        desc.Height = m_screenHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R32_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = m_device->CreateTexture2D(&desc, nullptr, m_blurTempTex.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(m_blurTempTex.Get(), nullptr, m_blurTempRTV.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateShaderResourceView(m_blurTempTex.Get(), nullptr, m_blurTempSRV.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- �u���[�萔�o�b�t�@ ---
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(BlurCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = m_device->CreateBuffer(&desc, nullptr, m_blurCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- �����萔�o�b�t�@ ---
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(CompositeCBData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = m_device->CreateBuffer(&desc, nullptr, m_compositeCB.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    // --- �|�C���g�T���v���[(�[�x�e�N�X�`���͕�Ԃ��Ȃ�) ---
    if (!m_pointSampler)
    {
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        hr = m_device->CreateSamplerState(&desc, m_pointSampler.GetAddressOf());
        if (FAILED(hr)) return false;
    }

    char msg[256];
    sprintf_s(msg, "[GPUParticle] Fluid resources created (%dx%d)\n", m_screenWidth, m_screenHeight);
    OutputDebugStringA(msg);
    return true;
}

// ============================================================
//  ��ʃT�C�Y�ύX��
// ============================================================
void GPUParticleSystem::OnResize(int width, int height)
{
    if (width == m_screenWidth && height == m_screenHeight) return;
    m_screenWidth = width;
    m_screenHeight = height;
    if (m_fluidShadersReady)
        CreateFluidResources();
}

// ============================================================
//  Pass 1: �[�x�p�X - �p�[�e�B�N���̋��ʐ[�x���e�N�X�`���ɕ`��
// ============================================================
void GPUParticleSystem::FluidDepthPass(XMMATRIX view, XMMATRIX proj, XMFLOAT3 cameraPos)
{
    // �[�x�e�N�X�`�����N���A(0 = �����Ȃ�)
    float clearColor[4] = { 0, 0, 0, 0 };
    m_context->ClearRenderTargetView(m_fluidDepthRTV.Get(), clearColor);

    // �����_�[�^�[�Q�b�g��[�x�e�N�X�`���ɕύX(�[�x�e�X�g�Ȃ�)
    ID3D11RenderTargetView* rtv = m_fluidDepthRTV.Get();
    m_context->OMSetRenderTargets(1, &rtv, nullptr);

    // �J�����萔�o�b�t�@�X�V
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMFLOAT4X4 ivF;
    XMStoreFloat4x4(&ivF, invView);
    XMFLOAT3 camRight(ivF._11, ivF._12, ivF._13);
    XMFLOAT3 camUp(ivF._21, ivF._22, ivF._23);
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_cameraCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CameraCBData* cb = (CameraCBData*)mapped.pData;
            XMStoreFloat4x4(&cb->View, XMMatrixTranspose(view));
            XMStoreFloat4x4(&cb->Projection, XMMatrixTranspose(proj));
            cb->CameraRight = camRight;
            cb->Time = m_totalTime;
            cb->CameraUp = camUp;
            cb->SizeScale = 3.0f;
            m_context->Unmap(m_cameraCB.Get(), 0);
        }
    }

    // �p�C�v���C���ݒ�(����VS���ė��p�APS�����[�x�p�ɕύX)
    m_context->IASetInputLayout(nullptr);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);       // �����r���{�[�hVS
    m_context->PSSetShader(m_fluidDepthPS.Get(), nullptr, 0);       // //�[�xPS
    m_context->VSSetShaderResources(0, 1, m_particleSRV.GetAddressOf());
    m_context->VSSetConstantBuffers(0, 1, m_cameraCB.GetAddressOf());

    // �u�����h: �ŏ��l��������(��O�̐[�x��D��)
    // // ���Z�u�����h�ł͂Ȃ��s�����ŏ㏑��(�[�x�͍����Ȃ�)
    m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(nullptr, 0);

    // �`��
    m_context->DrawIndexed(m_maxParticles * 6, 0, 0);

    // SRV�A���o�C���h
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    m_context->VSSetShaderResources(0, 1, nullSRV);
}

// ============================================================
//  Pass 2: �u���[�p�X - �o�C���e�����u���[�Ő[�x�����炩��
// ============================================================
void GPUParticleSystem::FluidBlurPass()
{
    float texelW = 1.0f / (float)m_screenWidth;
    float texelH = 1.0f / (float)m_screenHeight;

    // --- �����u���[: fluidDepth �� blurTemp ---
    {
        float clearColor[4] = { 0, 0, 0, 0 };
        m_context->ClearRenderTargetView(m_blurTempRTV.Get(), clearColor);

        ID3D11RenderTargetView* rtv = m_blurTempRTV.Get();
        m_context->OMSetRenderTargets(1, &rtv, nullptr);

        // �萔�o�b�t�@: ��������
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_blurCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            BlurCBData* cb = (BlurCBData*)mapped.pData;
            cb->TexelSize = XMFLOAT2(texelW, texelH);
            cb->BlurRadius = 15.0f;        // 7�s�N�Z�����a
            cb->DepthThreshold = 0.05f;    // �[�x��1%�ȏ�͍����Ȃ�
            cb->BlurDirection = XMFLOAT2(1.0f, 0.0f);  // ����
            cb->Padding[0] = cb->Padding[1] = 0.0f;
            m_context->Unmap(m_blurCB.Get(), 0);
        }

        // �t���X�N���[���N���b�h�ŕ`��
        m_context->IASetInputLayout(nullptr);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
        m_context->PSSetShader(m_fluidBlurPS.Get(), nullptr, 0);
        m_context->PSSetShaderResources(0, 1, m_fluidDepthSRV.GetAddressOf());  // ����: �[�x�e�N�X�`��
        m_context->PSSetConstantBuffers(0, 1, m_blurCB.GetAddressOf());
        m_context->PSSetSamplers(0, 1, m_pointSampler.GetAddressOf());
        m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(nullptr, 0);

        m_context->Draw(3, 0);  // �t���X�N���[���O�p�`

        // SRV�A���o�C���h
        ID3D11ShaderResourceView* nullSRV[] = { nullptr };
        m_context->PSSetShaderResources(0, 1, nullSRV);
    }

    // --- �����u���[: blurTemp �� fluidDepth(�㏑��) ---
    {
        float clearColor[4] = { 0, 0, 0, 0 };
        m_context->ClearRenderTargetView(m_fluidDepthRTV.Get(), clearColor);

        ID3D11RenderTargetView* rtv = m_fluidDepthRTV.Get();
        m_context->OMSetRenderTargets(1, &rtv, nullptr);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_blurCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            BlurCBData* cb = (BlurCBData*)mapped.pData;
            cb->TexelSize = XMFLOAT2(texelW, texelH);
            cb->BlurRadius = 15.0f;
            cb->DepthThreshold = 0.05f;
            cb->BlurDirection = XMFLOAT2(0.0f, 1.0f);  // ����
            cb->Padding[0] = cb->Padding[1] = 0.0f;
            m_context->Unmap(m_blurCB.Get(), 0);
        }

        m_context->PSSetShaderResources(0, 1, m_blurTempSRV.GetAddressOf());  // ����: �����u���[����
        m_context->Draw(3, 0);

        ID3D11ShaderResourceView* nullSRV[] = { nullptr };
        m_context->PSSetShaderResources(0, 1, nullSRV);
    }
}

// ============================================================
//  Pass 3: �����p�X - �@������ + PBR���C�e�B���O
// ============================================================
void GPUParticleSystem::FluidCompositePass(XMMATRIX proj, XMFLOAT3 cameraPos,
    ID3D11ShaderResourceView* sceneColorSRV, ID3D11RenderTargetView* finalRTV)
{
    // �ŏI�o�͐���Z�b�g
    m_context->OMSetRenderTargets(1, &finalRTV, nullptr);

    // �����萔�o�b�t�@�X�V
    {
        XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_compositeCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            CompositeCBData* cb = (CompositeCBData*)mapped.pData;
            XMStoreFloat4x4(&cb->InvProjection, XMMatrixTranspose(invProj));
            cb->TexelSize = XMFLOAT2(1.0f / m_screenWidth, 1.0f / m_screenHeight);
            cb->Time = m_totalTime;
            cb->Thickness = 0.3f;           // ���̌���
            cb->LightDir = XMFLOAT3(0.3f, -0.8f, 0.5f);  // �΂ߏォ��̌�
            cb->SpecularPower = 128.0f;     // �s���X�y�L����
            cb->CameraPos = cameraPos;
            cb->FluidAlpha = 0.85f;         // ���̂̕s�����x
            m_context->Unmap(m_compositeCB.Get(), 0);
        }
    }

    // �V�F�[�_�[�ݒ�
    m_context->VSSetShader(m_fullscreenVS.Get(), nullptr, 0);
    m_context->PSSetShader(m_fluidCompositePS.Get(), nullptr, 0);

    // �e�N�X�`���o�C���h
    ID3D11ShaderResourceView* srvs[2] = {
        m_fluidDepthSRV.Get(),  // t0: �u���[�ςݐ[�x
        sceneColorSRV            // t1: �V�[���J���[
    };
    m_context->PSSetShaderResources(0, 2, srvs);
    m_context->PSSetConstantBuffers(0, 1, m_compositeCB.GetAddressOf());
    m_context->PSSetSamplers(0, 1, m_pointSampler.GetAddressOf());

    m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(nullptr, 0);

    // �t���X�N���[���`��
    m_context->Draw(3, 0);

    // �N���[���A�b�v
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    m_context->PSSetShaderResources(0, 2, nullSRVs);
}

// ============================================================
//  ���̕`��(�}���`�p�X: �[�x���u���[������)
// ============================================================
void GPUParticleSystem::DrawFluid(XMMATRIX view, XMMATRIX proj, XMFLOAT3 cameraPos,
    ID3D11ShaderResourceView* sceneColorSRV, ID3D11RenderTargetView* finalRTV)
{
    if (!m_fluidShadersReady || !m_fluidEnabled)
    {
        Draw(view, proj, cameraPos);
        return;
    }

    // �X�e�[�g�ۑ�
    SavedState saved;
    SaveState(saved);

    // �r���[�|�[�g�ݒ�
    D3D11_VIEWPORT vp = {};
    vp.Width = (float)m_screenWidth;
    vp.Height = (float)m_screenHeight;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    // // ���X�^���C�U���f�t�H���g��(�J�����O����h�~)
    m_context->RSSetState(nullptr);

    // Pass 1: �p�[�e�B�N���[�x��`��
    FluidDepthPass(view, proj, cameraPos);

    // Pass 2: �o�C���e�����u���[�Ő[�x�����炩��
    FluidBlurPass();

    // Pass 3: �@������ + PBR���C�e�B���O + �V�[������
    FluidCompositePass(proj, cameraPos, sceneColorSRV, finalRTV);

    // �X�e�[�g����
    RestoreState(saved);
}