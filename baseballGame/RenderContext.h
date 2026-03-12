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
};