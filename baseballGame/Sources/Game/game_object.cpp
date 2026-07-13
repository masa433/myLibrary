#include "game_object.h"

void GameObject::UpdateTransform()
{
	//右手系とか左手系を選択できるようにする
	
	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]{
		{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },	// 0:RHS Y-UP
		{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },		// 1:LHS Y-UP
		{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },	// 2:RHS Z-UP
		{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },		// 3:LHS Z-UP
	};

	//左手系Y-UPで描画する
	DirectX::XMMATRIX C = DirectX::XMLoadFloat4x4(&coordinate_system_transforms[1]);
	
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX Transform = C * S * R * T;
	DirectX::XMStoreFloat3(&right, R.r[0]);
	DirectX::XMStoreFloat3(&up, R.r[1]);
	DirectX::XMStoreFloat3(&front, R.r[2]);
	DirectX::XMStoreFloat4x4(&transform, Transform);
}