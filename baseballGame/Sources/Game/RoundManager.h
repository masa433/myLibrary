#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <wrl.h>
#include "FontRenderer.h"
#include "sprite.h"
#include "json.hpp"
#include "SpecialAbility.h"
#include "../Sources/Audio/AudioSource.h"
#include "../Sources/Audio/Audio.h"

using json = nlohmann::json;

class RoundManager
{

public:
	static RoundManager& Instance()
	{
		static RoundManager instance;
		return instance;
	}

	//ラウンドステート
	enum class RoundState
	{
		Playing,
		SelectAbility,
		Shop,
	};

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

	bool IsPlaying() const { return currentState == RoundState::Playing; }
	bool IsAbilitySelecting() const { return currentState == RoundState::SelectAbility; }
	bool IsShopState() const { return currentState == RoundState::Shop; }

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

	//次のラウンドの目標数を増やす関数
	void IncreaseTargetHomeRuns(int amount)
	{
		int index = currentRound;
		if(index >= 0 && index < targetHomeRuns.size())
		{
			targetHomeRuns[index] += amount;
		}
	}

private:
	int currentRound = 1;
	int totalRounds = 7;
	std::vector<int> targetHomeRuns = { 0,1,2,2,3,3,5 };

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

	RoundState currentState = RoundState::Playing;

	std::vector<SpecialAbility::AbilityID> abilitiesChoice; // 選択された特殊能力のIDを保持するベクター
	int hoveredAbilityIndex = -1; // ホバー中の特殊能力のインデックス

	//アイコンの位置は三角形に配置
	DirectX::XMFLOAT2 abilityIconPositions[3] =
	{
		{ 960.0f, 350.0f }, // 1つ目のアイコンの位置
		{ 560.0f, 550.0f }, // 2つ目のアイコンの位置
		{ 1360.0f, 550.0f }  // 3つ目のアイコンの位置
	};

	DirectX::XMFLOAT2 abilityIconSize = { 600.0f, 100.0f }; // アイコンのサイズ

	std::unique_ptr<RoundSpriteData> abilityBackSpriteData;
	std::unique_ptr<sprite> abilityBackSprite;

	FontRenderer abilityBonusFont;
	DirectX::XMFLOAT2 abilityBonusFontPosition = { 960.0f, 200.0f };
	float abilityBonusFontScale = 1.0f;
	DirectX::XMFLOAT4 abilityBonusFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	void EnterSelectAbilityState();
	void UpdateSelectAbilityState();
	void ProcessedToNextRound();

	void EnterShopState();
	void UpdateShopState();

	struct SpeedMode
	{
		std::unique_ptr<RoundSpriteData> speedModeSpriteData;
		std::unique_ptr<sprite> speedModeSprite;
		std::string name;
		bool isActive = false;

		DirectX::XMFLOAT2 position = { 1700.0f,950.0f };
		DirectX::XMFLOAT2 size = { 400.0f,120.0f };
		DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
	};
	
	std::vector<SpeedMode> speedModes;

	void TriggerSpeedModeActive(int currentRound);

	bool isShopClosingStarted = false; // ショップが閉じるアニメーションを開始したかどうかのフラグ


	AudioSource* selectAbilitySound = nullptr;
	AudioSource* selectAbilityHoverSound = nullptr;
	AudioSource* closeShopSound = nullptr;
};