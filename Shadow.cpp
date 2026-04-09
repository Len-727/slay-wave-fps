// Shadow.cpp

#include "Shadow.h"
#include <DirectXColors.h>
#include <VertexTypes.h>
#include <CommonStates.h> 
#include <vector>
#include <cmath>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// �ʒu(Position) + �摜���W(UV) �������_
struct ShadowVertex {
	XMFLOAT3 position;
	XMFLOAT2 uv;
};

Shadow::Shadow()
	: m_shadowColor(0.0f, 0.0f, 0.0f, 0.5f) // �f�t�H���g��
{
}

Shadow::~Shadow()
{
}

bool Shadow::Initialize(ID3D11Device* device)
{
	m_effect = std::make_unique<BasicEffect>(device);
	m_effect->SetVertexColorEnabled(false);

	//�e�N�X�`����L���ɂ���
	m_effect->SetTextureEnabled(true);
	m_effect->SetLightingEnabled(false);

	//�ۂ��e�̃e�N�X�`�����쐬
	if (!CreateCircleTexture(device)) return false;

	// �G�t�F�N�g�Ƀe�N�X�`�����Z�b�g
	m_effect->SetTexture(m_textureView.Get());

	void const* shaderByteCode;
	size_t byteCodeLength;
	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	// ���̓��C�A�E�g�� UV (TEXCOORD) ��//
	D3D11_INPUT_ELEMENT_DESC inputElements[] = {
		{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	HRESULT hr = device->CreateInputLayout(inputElements, 2, shaderByteCode, byteCodeLength, m_inputLayout.GetAddressOf());
	if (FAILED(hr)) return false;

	if (!CreateBuffers(device)) return false;
	if (!CreateBlendState(device)) return false;
	if (!CreateDepthStencilState(device)) return false;
	if (!CreateRasterizerState(device)) return false;

	return true;
}

// �v���O�����Łu�{�������~�v�̉摜�����֐�
bool Shadow::CreateCircleTexture(ID3D11Device* device)
{
	const int size = 64; // 64x64�s�N�Z��
	std::vector<uint32_t> pixels(size * size);

	float center = size / 2.0f;
	float radius = size / 2.0f;

	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			float dx = x - center;
			float dy = y - center;
			float dist = sqrt(dx * dx + dy * dy); // ���S����̋���

			// ���F�̃s�N�Z�������
			uint8_t r = 255;
			uint8_t g = 255;
			uint8_t b = 255;
			uint8_t a = 0;

			// �~�̓����Ȃ�A���t�@�l�i�s�����x�j������
			if (dist < radius)
			{
				float t = dist / radius;     // 0.0(���S) �` 1.0(�[)
				float alpha = 1.0f - t;      // ���S�قǔZ��
				alpha = alpha * alpha;       // 2�悵�ăt�`���_�炩������
				a = static_cast<uint8_t>(alpha * 255);
			}

			// �������ɏ������� (BGRA��)
			pixels[y * size + x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}

	// DirectX�̃e�N�X�`�����쐬
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = size;
	desc.Height = size;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pixels.data();
	initData.SysMemPitch = size * sizeof(uint32_t);

	ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = device->CreateTexture2D(&desc, &initData, texture.GetAddressOf());
	if (FAILED(hr)) return false;

	return SUCCEEDED(device->CreateShaderResourceView(texture.Get(), nullptr, m_textureView.GetAddressOf()));
}

void Shadow::RenderShadow(
	ID3D11DeviceContext* context,
	DirectX::XMFLOAT3 position,
	float size,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection,
	DirectX::XMFLOAT3 lightDirection,
	float groundY)
{
	ComPtr<ID3D11Device> device;
	context->GetDevice(device.GetAddressOf());
	CommonStates states(device.Get());

	// 1. �s��v�Z�i�y�`�����R�ɂ��Ĕz�u�j
	XMMATRIX scale = XMMatrixScaling(size, 1.0f, size);
	XMMATRIX flatten = XMMatrixScaling(1.0f, 0.0f, 1.0f);
	XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX bias = XMMatrixTranslation(0.0f, 0.02f, 0.0f); // 2cm��������

	XMMATRIX world = scale * flatten * trans * bias;

	// 2. �G�t�F�N�g�ݒ�
	m_effect->SetWorld(world);
	m_effect->SetView(view);
	m_effect->SetProjection(projection);

	// �F�ݒ�F�e�̐F(��) �~ �e�N�X�`���̔� = �����e�ɂȂ�
	m_effect->SetDiffuseColor(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	m_effect->SetAlpha(0.6f); // �S�̂̔���

	// 3. �`��X�e�[�g
	context->OMSetBlendState(states.AlphaBlend(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(states.DepthRead(), 0);
	context->RSSetState(states.CullNone());

	// 4. �`��
	m_effect->Apply(context);

	UINT stride = sizeof(ShadowVertex);
	UINT offset = 0;
	context->IASetInputLayout(m_inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	context->DrawIndexed(6, 0, 0);

	// ����
	context->OMSetDepthStencilState(states.DepthDefault(), 0);
	context->RSSetState(states.CullCounterClockwise());
	context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

// �o�b�t�@�쐬�iUV���W��//�j
bool Shadow::CreateBuffers(ID3D11Device* device)
{
	ShadowVertex vertices[] = {
		// �ʒu(x,y,z)                  // UV(u,v) �摜�̂ǂ̕������g����
		{ XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT2(0.0f, 1.0f) }, // ����
		{ XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT2(1.0f, 1.0f) }, // �E��
		{ XMFLOAT3(0.5f, 0.0f,  0.5f), XMFLOAT2(1.0f, 0.0f) }, // �E��
		{ XMFLOAT3(-0.5f, 0.0f,  0.5f), XMFLOAT2(0.0f, 0.0f) }, // ����
	};

	D3D11_BUFFER_DESC vbDesc = { sizeof(vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA vbData = { vertices, 0, 0 };
	if (FAILED(device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf()))) return false;

	uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };
	D3D11_BUFFER_DESC ibDesc = { sizeof(indices), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA ibData = { indices, 0, 0 };
	if (FAILED(device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf()))) return false;

	return true;
}

// ���ȉ��̊֐��͍���͎g���܂��񂪁A�G���[����̂��ߎc���Ă����܂�
//XMMATRIX Shadow::CalculateShadowMatrix(XMFLOAT3 l, float g) { return XMMatrixIdentity(); }
bool Shadow::CreateBlendState(ID3D11Device* d) { return true; }
bool Shadow::CreateDepthStencilState(ID3D11Device* d) { return true; }
bool Shadow::CreateRasterizerState(ID3D11Device* d) { return true; }