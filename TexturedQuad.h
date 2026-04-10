//	TexturedQuad.h
//	3D空間にテクスチャを貼った板を描画
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include "Effects.h"
#include "CommonStates.h"

//	===	テクスチャ付き頂点データ
struct VertexPositionTexture
{
	DirectX::XMFLOAT3 position;	//	位置(3D座標): 頂点が3D空間のどこにあるのか
	DirectX::XMFLOAT2 texCoord;	//	UV座標:	頂点のテクスチャ上の対応位置

	VertexPositionTexture() = default;	//	デフォルトコンストラクタ

	//	初期化コンストラクタ
	VertexPositionTexture(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 uv)
		: position(pos), texCoord(uv)
	{
	}

	//	DIrectXTK用の入力レイアウト
	static const D3D11_INPUT_ELEMENT_DESC InputElements[];
	static const int InputElementCount = 2;
};


//	===	テクスチャ付き板ポリゴンクラス	===
class TexturedQuad
{
public:

	//	---	コンストラクタ	---
	TexturedQuad();

	//	---	初期化	---
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);

	//	---	テクスチャ読み込み	---
	bool LoadTexture(ID3D11Device* device, const wchar_t* filename);

	//	---	描画	---
	void Draw(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection);

private:
	
	//	頂点バッファ(板の形状でーた)
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;

	//	インデックスバッファ(三角形の並び)
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

	//	テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;

	//	エッフェクト(シェーダー)
	std::unique_ptr<DirectX::BasicEffect> m_effect;

	//	入力レイアウト
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

	//	共通ステート
	std::unique_ptr<DirectX::CommonStates> m_states;
};

