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

	void ResetCombo() 
	{ 
		if (hasCountedHit) return; // ヒットがカウントされた場合はリセットしない

		currentCombo = 0; 
		if (consoleLog)
		{
			consoleLog->push_back(u8"[Info] コンボをリセットしました");
		}
		hasCountedHit = true; // ヒットがカウントされたことを記録
	}
	void AddCombo(int amount);
	int GetMaxCombo() { return maxCombo; }
	void ResetHitFlag() { hasCountedHit = false; } // ヒットカウントフラグをリセットする関数
	int GetCurrentCombo() { return currentCombo; } // 現在のコンボ数を取得する関数

private:
	int currentCombo = 0;
	int maxCombo = 0;
	bool hasCountedHit = false; // ヒットがカウントされたかどうかのフラグ
	FontRenderer numberFont;
	DirectX::XMFLOAT2 numberFontPosition = { 100.0f, 100.0f };
	float numberFontSize = 24.0f;
	DirectX::XMFLOAT4 numberFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	FontRenderer labelFont;
	DirectX::XMFLOAT2 labelFontPosition = { 100.0f, 150.0f };
	float labelFontSize = 24.0f;
	DirectX::XMFLOAT4 labelFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

public:
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;

};