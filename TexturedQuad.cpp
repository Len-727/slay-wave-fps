//	TextureQuad.cpp
#include "TexturedQuad.h"
#include "WICTextureLoader.h"
#include <stdexcept>

using namespace DirectX;
using Microsoft::WRL::ComPtr;


//	===	���̓��C�A�E�g��`	===
const D3D11_INPUT_ELEMENT_DESC VertexPositionTexture::InputElements[] =
{
	{"SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
};


//	===	�R���X�g���N�^	===
TexturedQuad::TexturedQuad()
{

}


//	===	������	===
bool TexturedQuad::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	//	---	���_�f�[�^(�̌`��)	---

	//	�̃T�C�Y(1 * 1)
	VertexPositionTexture vertices[] =
	{
		//	�ʒu�@UV���W
		{XMFLOAT3(-0.5f, 0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f)},	//	����
		{XMFLOAT3(0.5f, 0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f)},		//	�E��
		{XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f)},	//	����
		{XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f)}		//	�E��
	};

	//	���_�o�b�t�@�̍쐬
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = vertices;

	HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &vertexData, m_vertexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		OutputDebugStringA("TexturedQuad::Initialize - Failed to create vertex buffer\n");
		return false;
	}


	//	===	�C���f�b�N�X�f�[�^(�O�p�`�̕���)	===

	WORD indices[] =
	{
		0, 1, 2,
		2, 1, 3
	};

	//	�C���f�b�N�X�o�b�t�@�̍쐬
	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(indices);
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	hr = device->CreateBuffer(&indexBufferDesc, &indexData, m_indexBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		OutputDebugStringA("TexturedQuad::Initialize - Failed to create index buffer\n");
		return false;
	}


	//	===	�G�t�F�N�g�쐬	===
	m_effect = std::make_unique<BasicEffect>(device);
	m_effect->SetTextureEnabled(true);		//	�e�N�X�`�����g�p
	m_effect->SetVertexColorEnabled(false);	//	���_�J���[�͕s�g�p


	//	===	���̓��C�A�E�g�쐬	===
	void const* shaderByteCode;
	size_t byteCodeLength;
	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	hr = device->CreateInputLayout(
		VertexPositionTexture::InputElements,
		VertexPositionTexture::InputElementCount,
		shaderByteCode,
		byteCodeLength,
		m_inputLayout.GetAddressOf()
	);

	if (FAILED(hr))
	{
		OutputDebugStringA("TexturedQuad::Initialize - Failed to create input layout\n");
		return false;
	}



	//	===	���ʃX�e�[�g�쐬	===
	
	m_states = std::make_unique<CommonStates>(device);

	OutputDebugStringA("TexturedQuad::Initialize - Success\n");
	return true;

}


//	�e�N�X�`���ǂݍ���
bool TexturedQuad::LoadTexture(ID3D11Device* device, const wchar_t* filename)
{
	//	PNG/JPG��ǂݍ���
	HRESULT hr = CreateWICTextureFromFile(
		device,
		filename,
		nullptr,
		m_texture.GetAddressOf()
	);

	if (FAILED(hr))
	{
		char debug[512];
		sprintf_s(debug, "TexturedQuad::LoadTexture - Failed to load: %ls\n", filename);
		OutputDebugStringA(debug);
		return false;
	}

	char debug[512];
	sprintf_s(debug, "TexturedQuad::LoadTexture - Success: %ls\n", filename);
	OutputDebugStringA(debug);
	return true;


}


//	===	�`��	===
void TexturedQuad::Draw(ID3D11DeviceContext* context,
	DirectX::XMMATRIX world,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection)
{
	if (!m_texture)
	{
		//	�e�N�X�`�����Ȃ��ꍇ�͕`�悵�Ȃ�
		return;
	}


	//	---	�G�b�t�F�N�g�ݒ�	---
	
	m_effect->SetWorld(world);
	m_effect->SetView(view);
	m_effect->SetProjection(projection);
	m_effect->SetTexture(m_texture.Get());

	m_effect->Apply(context);

	//	---	�`��ݒ�	---

	//	���̓��C�A�E�g
	context->IASetInputLayout(m_inputLayout.Get());

	//	���_�o�b�t�@
	UINT stride = sizeof(VertexPositionTexture);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	//	�C���f�b�N�X�o�b�t�@
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	//	�v���~�e�B�u(�O�p�`���X�g)
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//	===	�����_�[�X�e�[�g(���߂�L����)	===

	//	�A���t�@�u�����h�L��(����PNG�Ή�)
	context->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xFFFFFFFF);

	//	�[�x�e�X�g�͗L���@�������ݖ���(�������I�u�W�F�N�g�p)
	context->OMSetDepthStencilState(m_states->DepthRead(), 0);

	//	���ʕ`��(�������������)
	context->RSSetState(m_states->CullNone());


	//	===	�`����s	===
	context->DrawIndexed(6, 0, 0);	//	6���_(2�O�p�`)

}