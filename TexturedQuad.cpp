//	TextureQuad.cpp
#include "TexturedQuad.h"
#include "WICTextureLoader.h"
#include <stdexcept>

using namespace DirectX;
using Microsoft::WRL::ComPtr;


//	===	入力レイアウト定義	===
const D3D11_INPUT_ELEMENT_DESC VertexPositionTexture::InputElements[] =
{
	{"SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
};


//	===	コンストラクタ	===
TexturedQuad::TexturedQuad()
{

}


//	===	初期化	===
bool TexturedQuad::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	//	---	頂点データ(板の形状)	---

	//	板のサイズ(1 * 1)
	VertexPositionTexture vertices[] =
	{
		//	位置　UV座標
		{XMFLOAT3(-0.5f, 0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f)},	//	左上
		{XMFLOAT3(0.5f, 0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f)},		//	右上
		{XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f)},	//	左下
		{XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f)}		//	右下
	};

	//	頂点バッファの作成
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


	//	===	インデックスデータ(三角形の並び)	===

	WORD indices[] =
	{
		0, 1, 2,
		2, 1, 3
	};

	//	インデックスバッファの作成
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


	//	===	エフェクト作成	===
	m_effect = std::make_unique<BasicEffect>(device);
	m_effect->SetTextureEnabled(true);		//	テクスチャを使用
	m_effect->SetVertexColorEnabled(false);	//	頂点カラーは不使用


	//	===	入力レイアウト作成	===
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



	//	===	共通ステート作成	===
	
	m_states = std::make_unique<CommonStates>(device);

	OutputDebugStringA("TexturedQuad::Initialize - Success\n");
	return true;

}


//	テクスチャ読み込み
bool TexturedQuad::LoadTexture(ID3D11Device* device, const wchar_t* filename)
{
	//	PNG/JPGを読み込む
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


//	===	描画	===
void TexturedQuad::Draw(ID3D11DeviceContext* context,
	DirectX::XMMATRIX world,
	DirectX::XMMATRIX view,
	DirectX::XMMATRIX projection)
{
	if (!m_texture)
	{
		//	テクスチャがない場合は描画しない
		return;
	}


	//	---	エッフェクト設定	---
	
	m_effect->SetWorld(world);
	m_effect->SetView(view);
	m_effect->SetProjection(projection);
	m_effect->SetTexture(m_texture.Get());

	m_effect->Apply(context);

	//	---	描画設定	---

	//	入力レイアウト
	context->IASetInputLayout(m_inputLayout.Get());

	//	頂点バッファ
	UINT stride = sizeof(VertexPositionTexture);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	//	インデックスバッファ
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	//	プリミティブ(三角形リスト)
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//	===	レンダーステート(透過を有効か)	===

	//	アルファブレンド有効(透過PNG対応)
	context->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xFFFFFFFF);

	//	深度テストは有効　書き込み無効(半透明オブジェクト用)
	context->OMSetDepthStencilState(m_states->DepthRead(), 0);

	//	両面描画(裏からも見える)
	context->RSSetState(m_states->CullNone());


	//	===	描画実行	===
	context->DrawIndexed(6, 0, 0);	//	6頂点(2三角形)

}