#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "FontRenderer.h"
#include "Pitcher.h"
#include "UiEasing.h"
#include "Combo.h"
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
		currentBallZoneBonusMoney += ballZoneBonusIncrement;
		if (consoleLog)
		{
			consoleLog->push_back(u8"[Info]ボールゾーンボーナスが増加しました。");
			consoleLog->push_back(u8"[Info]現在のボールゾーンボーナス倍率: " + std::to_string(currentBallZoneBonusMoney));
		}
	}

	// 倍率を初期値に戻す関数
	void ResetBallZoneBonus() 
	{ 
		currentBallZoneBonusMoney = baseBallZoneBonusMoney;
		if(consoleLog)
		{
			consoleLog->push_back(u8"[Info]ボールゾーンボーナスがリセットされました。");
			consoleLog->push_back(u8"[Info]現在のボールゾーンボーナス倍率: " + std::to_string(currentBallZoneBonusMoney));
		}
		
	}

	//現在のボール球見逃しボーナスを適用する関数
	void ApplyBallZoneBonus()
	{
		AddMoney(currentBallZoneBonusMoney);
		TriggerBallZoneBonusAnimation();
		if (consoleLog)
		{
			consoleLog->push_back(u8"[Info]ボールゾーンボーナスが適用されました。");
			consoleLog->push_back(u8"[Info]現在のボールゾーンボーナス倍率: " + std::to_string(currentBallZoneBonusMoney));
		}
	}

private:
	

	int currentMoney = 0;
	int targetMoney = 0; // 目標金額

	//ボール球を見送った時の倍率ボーナス
	int baseBallZoneBonusMoney = 50;//ボール球を見送った時の基本ボーナス金額
	int currentBallZoneBonusMoney = 50; //現在のボールゾーンボーナス金額
	int ballZoneBonusIncrement = 50; // ボールゾーンボーナスの増加量

	//ホームラン時の倍率ボーナス
	float homerunBonus = 1.05f;
	int finalDistance = 0; //最終的な飛距離

	//変化球を打ったときの倍率ボーナス
	float breakingBallBonus = 1.05f;

	//コンボ時の倍率ボーナス
	float comboBonus = 1.0f; // コンボボーナスの倍率
	float comboBonusIncrement = 0.1f; // コンボボーナスの増加量

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

	struct BonusInfo
	{
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		DirectX::XMFLOAT4 color;
	};


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

private:

	struct BonusItem
	{
		std::string name;
		std::unique_ptr<MoneyData> data;
		std::unique_ptr<sprite> sprite;
		std::unique_ptr<FontRenderer> fontRenderer;
		BonusInfo info;
		bool isActive = false;

		//アニメーション制御
		DirectX::XMFLOAT2 startPos = { 2000.0f, 0.0f };
		DirectX::XMFLOAT2 targetPos = { 1500.0f, 0.0f };
		DirectX::XMFLOAT2 currentPos = { 2000.0f, 0.0f };
	};

	std::vector<BonusItem> bonusItems;
	float bonusAnimTimer = 0.0f;
	float BONUS_ANIM_DURATION = 2.5f; // ボーナスアニメーションの時間(秒)
	bool isBonusAnimating = false; // ボーナスアニメーション中かどうかのフラグ
	float textXOffset = 0.0f; // ボーナステキストのX座標
	float textYOffset = 0.0f; // ボーナステキストのY座標

	//ボーナスのレイアウト再計算処理

	void TriggerBonusAnimation(bool isHomeRun, bool isBreaking);
	void TriggerBallZoneBonusAnimation();

};