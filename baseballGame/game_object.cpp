#include "game_object.h"

void GameObject::UpdateTransform()
{
	// ç∂éËån Y-UP ÇÃç¿ïWånïœä∑çsóÒ
	const DirectX::XMFLOAT4X4 lhs_coordinate_transform{
		-1, 0, 0, 0,  // Xé≤ÇîΩì]
		 0, 1, 0, 0,  // Yé≤ÇÕÇªÇÃÇ‹Ç‹
		 0, 0, 1, 0,  // Zé≤ÇÕÇªÇÃÇ‹Ç‹
		 0, 0, 0, 1
	};
	DirectX::XMMATRIX C = DirectX::XMLoadFloat4x4(&lhs_coordinate_transform);

	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX Transform = C * S * R * T;
	DirectX::XMStoreFloat3(&right, R.r[0]);
	DirectX::XMStoreFloat3(&up, R.r[1]);
	DirectX::XMStoreFloat3(&front, R.r[2]);
	DirectX::XMStoreFloat4x4(&transform, Transform);
}