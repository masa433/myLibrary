#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "FontRenderer.h"
#include "Pitcher.h"
#include "json.hpp"

using json = nlohmann::json;

class Money
{
public:
	static Money& Instance()
	{
		static Money instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	int GetCurrentMoney() const { return currentMoney; }
	void AddMoney(int amount) { targetMoney += amount; }
	void SubtractMoney(int amount) { targetMoney -= amount; if (targetMoney < 0) targetMoney = 0; }
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	// 外からボール判定時に呼ぶ関数
	void IncrementBallZoneBonus() 
	{
		currentBallZoneBonus += ballZoneBonusIncrement;
		if (consoleLog)
		{
			consoleLog->push_back(u8"[Info]ボールゾーンボーナスが増加しました。");
			consoleLog->push_back(u8"[Info]現在のボールゾーンボーナス倍率: " + std::to_string(currentBallZoneBonus));
		}
	}

	// 倍率を初期値に戻す関数
	void ResetBallZoneBonus() 
	{ 
		currentBallZoneBonus = baseBallZoneBonus;
		if(consoleLog)
		{
			consoleLog->push_back(u8"[Info]ボールゾーンボーナスがリセットされました。");
			consoleLog->push_back(u8"[Info]現在のボールゾーンボーナス倍率: " + std::to_string(currentBallZoneBonus));
		}
		
	}

private:
	int currentMoney = 0;
	int targetMoney = 0; // 目標金額

	//ホームラン時の倍率ボーナス
	float homerunBonus = 1.05f;
	float baseBallZoneBonus = 1.0f; //ボールゾーンを見逃したときの倍率ボーナス
	float currentBallZoneBonus = 1.0f; //現在のボールゾーン倍率ボーナス
	float ballZoneBonusIncrement = 0.05f; // ボールゾーンボーナスの増加量


	//変化球を打ったときの倍率ボーナス
	float breakingBallBonus = 1.05f;

	struct MoneyData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<MoneyData> moneyData;
	std::unique_ptr<sprite> moneySprite;

	DirectX::XMFLOAT2 moneyPosition = { 50.0f, 50.0f };
	DirectX::XMFLOAT2 moneySize = { 200.0f, 50.0f };
	DirectX::XMFLOAT4 moneyColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	FontRenderer moneyFont; // お金表示用のフォントレンダラー

	DirectX::XMFLOAT2 moneyTextPosition = { 60.0f, 60.0f };
	float moneyTextScale = 1.0f;
	DirectX::XMFLOAT4 moneyTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	bool prevDistanceLocked = false; // 前フレームの距離ロック状態を保存する変数
	
public:
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;
};