#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <wrl.h>
#include "FontRenderer.h"
#include "sprite.h"
#include "json.hpp"

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

	DirectX::XMFLOAT2 startPosition = { 960.0f, -540.0f }; // 初期位置を画面外の上部に設定
	DirectX::XMFLOAT2 currentPosition = { 960.0f, -540.0f }; // 初期位置を画面外の上部に設定
	DirectX::XMFLOAT2 targetPosition = { 960.0f, 540.0f }; // 目標位置を画面中央に設定

	float easingDuration = 1.0f; // イージングの時間（秒）
	float easingTimer = 0.0f; // イージングのタイマー
	bool isAnimating = false; // アニメーション中かどうかのフラグ
	bool isShopOpen = false; // ショップが開いているかどうかのフラグ
	bool isShopClosed = true; // ショップが閉じているかどうかのフラグ

	FontRenderer moneyFont;

	DirectX::XMFLOAT2 moneyFontPosition = { 1550.0f, 128.0f };	
	float moneyFontScale = 1.0f;
	DirectX::XMFLOAT4 moneyFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	DirectX::XMFLOAT2 moneyFontStartPosition = { 1550.0f, -128.0f };
	DirectX::XMFLOAT2 moneyFontCurrentPosition = { 1550.0f, -128.0f };
	DirectX::XMFLOAT2 moneyFontTargetPosition = { 1550.0f, -28.0f };

public:
	void OpenShop()
	{
		isAnimating = true;
		isShopOpen = true;
		easingTimer = 0.0f;
		startPosition = { 960.0f, -540.0f };
		targetPosition = { 960.0f, 540.0f }; // 目標位置を画面中央に設定
		currentPosition = startPosition; // 現在位置を初期位置に設定
		moneyFontStartPosition = { 1550.0f, -872.0f }; // お金フォントの初期位置を設定
		moneyFontTargetPosition = { 1550.0f, 128.0f }; // お金フォントの目標位置を設定
		moneyFontCurrentPosition = moneyFontStartPosition; // 現在位置を初期位置に設定
	}

	void CloseShop()
	{
		isAnimating = true;
		isShopClosed = true;
		isShopOpen = false;
		easingTimer = 0.0f;
		startPosition = currentPosition; // 現在位置を初期位置に設定
		targetPosition = { 960.0f, -540.0f }; // 目標位置を画面外の上部に設定
		moneyFontStartPosition = moneyFontCurrentPosition; // お金フォントの現在位置を初期位置に設定
		moneyFontTargetPosition = { 1550.0f, -872.0f }; // お金フォントの目標位置を画面外の上部に設定
	}

	bool IsShopOpen() const
	{
		return isShopOpen && !isAnimating;
	}

	bool IsClosing() const
	{
		return isShopClosed && isAnimating;
	}
};