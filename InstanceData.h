//	InstanceData.h
//	【役割】インスタン寝具描画用のデータ構造
//	【説明】各敵の「位置・回転・色」をGPUに送るための構造体
#pragma once

#include <DirectXMath.h>

//	【構造体】InstanceDate
//	【役割】敵一体分のインスタンスデータ
//	【メンバー】
//	world = {位置・回転・色}
struct InstanceData
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4 color;
};
