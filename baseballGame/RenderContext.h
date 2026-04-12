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

    // ポイントライト
    DirectX::XMFLOAT3 pointLightPosition = { 0, 3, 0 };
    float pointLightRange = 20.0f;
    DirectX::XMFLOAT3 pointLightColor = { 1, 1, 1 };


    // スポットライト
    DirectX::XMFLOAT3 spotLightPosition = { 0, 5, 0 };
    float spotLightRange = 50.0f;
    DirectX::XMFLOAT3 spotLightDirection = { 0, -1, 0 };
    float spotLightInnerAngle = 0.866f;  // cos(30°)
    DirectX::XMFLOAT3 spotLightColor = { 1, 1, 1 };
    float spotLightOuterAngle = 0.707f;  // cos(45°)

	DirectX::XMFLOAT3       cameraPosition = { 0, 0, 0 };
};