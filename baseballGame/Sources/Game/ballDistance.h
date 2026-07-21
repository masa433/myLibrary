#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <wrl.h>
#include <d3d11.h>
#include "FontRenderer.h"
#include <memory>
#include "json.hpp"

using json = nlohmann::json;


class BallDistance
{
public:
	//インスタンス
	static BallDistance& Instance()
	{
		static BallDistance instance;
		return instance;
	}

	BallDistance() = default;
	~BallDistance() = default;
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(nlohmann::json& j);
	void LoadFromJson(const nlohmann::json& j);

private:

	FontRenderer ballDistanceFont;

	//フォントの位置とサイズと色
	DirectX::XMFLOAT2 fontPosition;
	float fontSize = 1.0f;
	DirectX::XMFLOAT4 fontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	float currentDistance = 0.0f;
	char distanceText[32] = "";
	bool hasDistanceText = false;
};
