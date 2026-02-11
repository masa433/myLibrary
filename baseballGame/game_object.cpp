#include "game_object.h"

void GameObject::UpdateTransform()
{
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX Transform = S * R * T;
	DirectX::XMStoreFloat3(&right, R.r[0]);
	DirectX::XMStoreFloat3(&up, R.r[1]);
	DirectX::XMStoreFloat3(&front, R.r[2]);
	DirectX::XMStoreFloat4x4(&transform, Transform);
}