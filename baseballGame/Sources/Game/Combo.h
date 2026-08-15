#pragma once
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include "FontRenderer.h"
#include "json.hpp"

using json = nlohmann::json;

class Combo
{
public:
	static Combo& Instance()
	{
		static Combo instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	void ResetCombo() { currentCombo = 0; }
	void AddCombo(int amount);
	int GetMaxCombo() { return maxCombo; }

private:
	int currentCombo = 0;
	int maxCombo = 0;
	FontRenderer numberFont;
	DirectX::XMFLOAT2 numberFontPosition = { 100.0f, 100.0f };
	float numberFontSize = 24.0f;
	DirectX::XMFLOAT4 numberFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	FontRenderer labelFont;
	DirectX::XMFLOAT2 labelFontPosition = { 100.0f, 150.0f };
	float labelFontSize = 24.0f;
	DirectX::XMFLOAT4 labelFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};