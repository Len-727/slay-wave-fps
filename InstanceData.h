//	InstanceData.h
//	�y�����z�C���X�^���Q��`��p�̃f�[�^�\��
//	�y�����z�e�G�́u�ʒu�E��]�E�F�v��GPU�ɑ��邽�߂̍\����
#pragma once

#include <DirectXMath.h>

//	�y�\���́zInstanceDate
//	�y�����z�G��̕��̃C���X�^���X�f�[�^
//	�y�����o�[�z
//	world = {�ʒu�E��]�E�F}
struct InstanceData
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4 color;
};
