#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <wrl.h>
#include "FontRenderer.h"
#include "sprite.h"
#include "json.hpp"
#include "Player.h"

#define BATTER_COUNT 24

using json = nlohmann::json;
class ShopManager
{
public:
	static ShopManager& Instance()
	{
		static ShopManager instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);


private:

	struct ShopSprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<ShopSprite> shopBackSpriteData;
	std::unique_ptr<sprite> shopBackSprite;


	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;


	float easingDuration = 1.0f; // イージングの時間（秒）
	float easingTimer = 0.0f; // イージングのタイマー
	bool isAnimating = false; // アニメーション中かどうかのフラグ
	bool isShopOpen = false; // ショップが開いているかどうかのフラグ
	bool isShopClosed = true; // ショップが閉じているかどうかのフラグ

	FontRenderer moneyFont;

	DirectX::XMFLOAT2 moneyFontPosition = { 1550.0f, 128.0f };	
	float moneyFontScale = 1.0f;
	DirectX::XMFLOAT4 moneyFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	int selectedBatterIndex = -1; // 選択されたバッターのインデックス

	FontRenderer fontRenderer;

	std::unique_ptr<ShopSprite> batterSpriteData[BATTER_COUNT];
	std::unique_ptr<sprite> batterSprites[BATTER_COUNT];

	DirectX::XMFLOAT2 batterParamBackPosition = { 1450.0f, 550.0f };
	DirectX::XMFLOAT2 batterParamBackSize = { 550.0f, 450.0f };

	struct BatterParamFontData
	{
		DirectX::XMFLOAT2 position;
		float scale;
		DirectX::XMFLOAT4 color;
	};

	BatterParamFontData powerFontData;
	BatterParamFontData contactFontData;
	BatterParamFontData powerRankFontData;
	BatterParamFontData contactRankFontData;

	float currentOffsetY = -1080.0f; // 初期位置を画面外の上部に設定
	float startOffsetY = -1080.0f; // 初期位置を画面外の上部に設定
	float targetOffsetY = 0.0f; // 目標位置を画面中央に設定

	const DirectX::XMFLOAT2 baseShopBackPos = { 960.0f, 540.0f };
	const DirectX::XMFLOAT2 baseMoneyFontPos = { 1550.0f, 128.0f };
	const DirectX::XMFLOAT2 baseBatterParamPos = { 1450.0f, 550.0f };

public:
	void OpenShop()
	{
		isAnimating = true;
		isShopOpen = true;
		easingTimer = 0.0f;
		startOffsetY = currentOffsetY; // 現在位置を初期位置に設定
		targetOffsetY = 0.0f; // 目標位置を画面中央に設定
		
	}

	void CloseShop()
	{
		isAnimating = true;
		isShopClosed = true;
		isShopOpen = false;
		easingTimer = 0.0f;
		startOffsetY = currentOffsetY; // 現在位置を初期位置に設定
		targetOffsetY = -1080.0f; // 目標位置を画面外の上部に設定
		
	}

	bool IsShopOpen() const
	{
		return isShopOpen && !isAnimating;
	}

	bool IsClosing() const
	{
		return isShopClosed && isAnimating;
	}

private:

	static const char* GetBatterPowerRankString(Player::BatterPowerRank rank)
	{
		switch (rank)
		{
		case Player::BatterPowerRank::F: return "F";
		case Player::BatterPowerRank::E: return "E";
		case Player::BatterPowerRank::D: return "D";
		case Player::BatterPowerRank::C: return "C";
		case Player::BatterPowerRank::B: return "B";
		case Player::BatterPowerRank::A: return "A";
		case Player::BatterPowerRank::S: return "S";
		default: return "";
		}
	}

	static const char* GetBatterContactRankString(Player::BatterContactRank rank)
	{
		switch (rank)
		{
		case Player::BatterContactRank::F: return "F";
		case Player::BatterContactRank::E: return "E";
		case Player::BatterContactRank::D: return "D";
		case Player::BatterContactRank::C: return "C";
		case Player::BatterContactRank::B: return "B";
		case Player::BatterContactRank::A: return "A";
		case Player::BatterContactRank::S: return "S";
		default: return "";
		}
	}
};