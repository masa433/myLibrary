#pragma once
#include "camera.h"
#include "Light.h"
#include "RenderState.h"

class RenderContext
{
public:
	ID3D11DeviceContext* deviceContext;
	const RenderState* renderState = nullptr;
	DirectX::XMFLOAT4X4		view;
	DirectX::XMFLOAT4X4		projection;
	DirectX::XMFLOAT3		lightDirection = { 0, -1, 0 };
	DirectX::XMFLOAT3		lightColor = { 1, 1, 1 };
	DirectX::XMFLOAT4       ambientColor = { 0.2f, 0.2f, 0.2f, 1.0f };

	DirectX::XMFLOAT3       cameraPosition = { 0, 0, 0 };
};