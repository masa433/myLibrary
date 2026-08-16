#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <wrl.h>
#include "FontRenderer.h"
#include "sprite.h"
#include "json.hpp"

using json = nlohmann::json;

class RoundManager
{

public:
	static RoundManager& Instance()
	{
		static RoundManager instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);
	void IncreaseRound() { if (currentRound < totalRounds) currentRound++; }
	//最終ラウンドに到達したかを判定する関数
	bool IsFinalRound() const { return currentRound >= totalRounds; }
	bool IsGameOver() const {return isGameOver;}
	bool IsGameClear() const { return isGameClear; }
	int GetCurrentRound() const { return currentRound; }

	int GetCurrentTarget() const
	{
		int index = currentRound - 1;
		// 配列の範囲内かどうかを確認
		if(index >= 0 && index < targetHomeRuns.size())
		{
			return targetHomeRuns[index];// 現在のラウンドに対応する目標本塁打数を返す
		}
		return 0; // デフォルト値
	}

private:
	int currentRound = 1;
	int totalRounds = 5;
	std::vector<int> targetHomeRuns = { 0,1,2,3,5 };

	FontRenderer roundFont;
	DirectX::XMFLOAT2 roundTextPosition = { 20.0f, 20.0f };
	float roundTextScale = 1.0f;
	DirectX::XMFLOAT4 roundTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	bool isGameOver = false; // ゲームオーバー状態を示すフラグ
	bool isGameClear = false; // ゲームクリア状態を示すフラグ

	struct RoundSpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<RoundSpriteData> roundSpriteData;
	std::unique_ptr<sprite> roundSprite;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
	
	DirectX::XMFLOAT2 spritePosition = { 100.0f, 100.0f };
	DirectX::XMFLOAT2 spriteSize = { 200.0f, 50.0f };
	DirectX::XMFLOAT4 spriteColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};