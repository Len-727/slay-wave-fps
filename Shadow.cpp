// Shadow.cpp

#include "Shadow.h"
#include <DirectXColors.h>
#include <VertexTypes.h>
#include <CommonStates.h> 
#include <vector>
#include <cmath>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// 位置(Position) + 画像座標(UV) を持つ頂点
struct ShadowVertex {
	XMFLOAT3 position;
	XMFLOAT2 uv;
};

Shadow::Shadow()
	: m_shadowColor(0.0f, 0.0f, 0.0f, 0.5f) // デフォルト黒
{
}

Shadow::~Shadow()
{
}

bool Shadow::Initialize(ID3D11Device* device)
{
	m_effect = std::make_unique<BasicEffect>(device);
	m_effect->SetVertexColorEnabled(false);

	//テクスチャを有効にする
	m_effect->SetTextureEnabled(true);
	m_effect->SetLightingEnabled(false);

	//丸い影のテクスチャを作成
	if (!CreateCircleTexture(device)) return false;

	// エフェクトにテクスチャをセット
	m_effect->SetTexture(m_textureView.Get());

	void const* shaderByteCode;
	size_t byteCodeLength;
	m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	// 入力レイアウトに UV (TEXCOORD) を//
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

// プログラムで「ボヤけた円」の画像を作る関数
bool Shadow::CreateCircleTexture(ID3D11Device* device)
{
	const int size = 64; // 64x64ピクセル
	std::vector<uint32_t> pixels(size * size);

	float center = size / 2.0f;
	float radius = size / 2.0f;

	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			float dx = x - center;
			float dy = y - center;
			float dist = sqrt(dx * dx + dy * dy); // 中心からの距離

			// 白色のピクセルを作る
			uint8_t r = 255;
			uint8_t g = 255;
			uint8_t b = 255;
			uint8_t a = 0;

			// 円の内側ならアルファ値（不透明度）をつける
			if (dist < radius)
			{
				float t = dist / radius;     // 0.0(中心) ～ 1.0(端)
				float alpha = 1.0f - t;      // 中心ほど濃く
				alpha = alpha * alpha;       // 2乗してフチを柔らかくする
				a = static_cast<uint8_t>(alpha * 255);
			}

			// メモリに書き込み (BGRA順)
			pixels[y * size + x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}

	// DirectXのテクスチャを作成
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

	// 1. 行列計算（ペチャンコにして配置）
	XMMATRIX scale = XMMatrixScaling(size, 1.0f, size);
	XMMATRIX flatten = XMMatrixScaling(1.0f, 0.0f, 1.0f);
	XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX bias = XMMatrixTranslation(0.0f, 0.02f, 0.0f); // 2cm浮かせる

	XMMATRIX world = scale * flatten * trans * bias;

	// 2. エフェクト設定
	m_effect->SetWorld(world);
	m_effect->SetView(view);
	m_effect->SetProjection(projection);

	// 色設定：影の色(黒) × テクスチャの白 = 黒い影になる
	m_effect->SetDiffuseColor(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	m_effect->SetAlpha(0.6f); // 全体の薄さ

	// 3. 描画ステート
	context->OMSetBlendState(states.AlphaBlend(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(states.DepthRead(), 0);
	context->RSSetState(states.CullNone());

	// 4. 描画
	m_effect->Apply(context);

	UINT stride = sizeof(ShadowVertex);
	UINT offset = 0;
	context->IASetInputLayout(m_inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	context->DrawIndexed(6, 0, 0);

	// 復元
	context->OMSetDepthStencilState(states.DepthDefault(), 0);
	context->RSSetState(states.CullCounterClockwise());
	context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

// バッファ作成（UV座標を//）
bool Shadow::CreateBuffers(ID3D11Device* device)
{
	ShadowVertex vertices[] = {
		// 位置(x,y,z)                  // UV(u,v) 画像のどの部分を使うか
		{ XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT2(0.0f, 1.0f) }, // 左下
		{ XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT2(1.0f, 1.0f) }, // 右下
		{ XMFLOAT3(0.5f, 0.0f,  0.5f), XMFLOAT2(1.0f, 0.0f) }, // 右上
		{ XMFLOAT3(-0.5f, 0.0f,  0.5f), XMFLOAT2(0.0f, 0.0f) }, // 左上
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

// ※以下の関数は今回は使いませんが、エラー回避のため残しておきます
//XMMATRIX Shadow::CalculateShadowMatrix(XMFLOAT3 l, float g) { return XMMatrixIdentity(); }
bool Shadow::CreateBlendState(ID3D11Device* d) { return true; }
bool Shadow::CreateDepthStencilState(ID3D11Device* d) { return true; }
bool Shadow::CreateRasterizerState(ID3D11Device* d) { return true; }