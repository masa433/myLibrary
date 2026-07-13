#pragma once

#include <DirectXMath.h>

struct DirectionalLight
{
	DirectX::XMFLOAT3 direction = { 0.0f, -1.0f, 0.0f };
	DirectX::XMFLOAT3 color = { 1.0f, 1.0f, 1.0f };
};

struct PointLight
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 color;
	float range;
	float pad0;
};

struct SpotLight 
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 direction;
	DirectX::XMFLOAT3 color;
	float range;
	float innerConeAngle;
	float outerConeAngle;
};

class Light
{
public:
	// ディレクショナルライト設定
	void SetDirectionalLight(DirectionalLight& light) { directionalLight = light; }
	void SetPointLight(PointLight& light) { pointLight = light; }
	void SetSpotLight(SpotLight& light) { spotLight = light; }

	// ディレクショナルライト取得
	const DirectionalLight& GetDirectionalLight() const { return directionalLight; }
	const PointLight& GetPointLight() const { return pointLight; }
	const SpotLight& GetSpotLight() const { return spotLight; }

private:
	DirectionalLight directionalLight;
	PointLight pointLight;
	SpotLight spotLight;
};