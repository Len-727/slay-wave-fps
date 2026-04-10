// Shadow.h

#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include <Effects.h> // DirectXTK

using Microsoft::WRL::ComPtr;

class Shadow
{
public:
	Shadow();
	~Shadow();

	bool Initialize(ID3D11Device* device);

	void RenderShadow(
		ID3D11DeviceContext* context,
		DirectX::XMFLOAT3 position,
		float size,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection,
		DirectX::XMFLOAT3 lightDirection,
		float groundY);

	void SetShadowColor(DirectX::XMFLOAT4 color) { m_shadowColor = color; }

private:
	std::unique_ptr<DirectX::BasicEffect> m_effect;
	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11Buffer> m_vertexBuffer;
	ComPtr<ID3D11Buffer> m_indexBuffer;

	// 丸い影の画像データ
	ComPtr<ID3D11ShaderResourceView> m_textureView;

	// ステート
	ComPtr<ID3D11BlendState> m_blendState;
	ComPtr<ID3D11DepthStencilState> m_depthStencilState;
	ComPtr<ID3D11RasterizerState> m_rasterizerStateNoCull;

	DirectX::XMFLOAT4 m_shadowColor;

	// 内部関数
	bool CreateBuffers(ID3D11Device* device);
	bool CreateBlendState(ID3D11Device* device);
	bool CreateDepthStencilState(ID3D11Device* device);
	bool CreateRasterizerState(ID3D11Device* device);

	// 丸い画像をプログラムで作る関数
	bool CreateCircleTexture(ID3D11Device* device);
};