//	TexturedQuad.h
//	3D��ԂɃe�N�X�`����\������`��
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <memory>
#include "Effects.h"
#include "CommonStates.h"

//	===	�e�N�X�`���t�����_�f�[�^
struct VertexPositionTexture
{
	DirectX::XMFLOAT3 position;	//	�ʒu(3D���W): ���_��3D��Ԃ̂ǂ��ɂ���̂�
	DirectX::XMFLOAT2 texCoord;	//	UV���W:	���_�̃e�N�X�`����̑Ή��ʒu

	VertexPositionTexture() = default;	//	�f�t�H���g�R���X�g���N�^

	//	�������R���X�g���N�^
	VertexPositionTexture(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 uv)
		: position(pos), texCoord(uv)
	{
	}

	//	DIrectXTK�p�̓��̓��C�A�E�g
	static const D3D11_INPUT_ELEMENT_DESC InputElements[];
	static const int InputElementCount = 2;
};


//	===	�e�N�X�`���t���|���S���N���X	===
class TexturedQuad
{
public:

	//	---	�R���X�g���N�^	---
	TexturedQuad();

	//	---	������	---
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);

	//	---	�e�N�X�`���ǂݍ���	---
	bool LoadTexture(ID3D11Device* device, const wchar_t* filename);

	//	---	�`��	---
	void Draw(ID3D11DeviceContext* context,
		DirectX::XMMATRIX world,
		DirectX::XMMATRIX view,
		DirectX::XMMATRIX projection);

private:
	
	//	���_�o�b�t�@(�̌`��Ł[��)
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;

	//	�C���f�b�N�X�o�b�t�@(�O�p�`�̕���)
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

	//	�e�N�X�`��
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;

	//	�G�b�t�F�N�g(�V�F�[�_�[)
	std::unique_ptr<DirectX::BasicEffect> m_effect;

	//	���̓��C�A�E�g
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

	//	���ʃX�e�[�g
	std::unique_ptr<DirectX::CommonStates> m_states;
};

