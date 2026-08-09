#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <wrl.h>
#include <d3d11.h>
#include "FontRenderer.h"
#include <memory>
#include "json.hpp"
#include "sprite.h"

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

	//最高飛距離を取得する関数
	float GetMaxDistance() const { return std::round(maxDistance); }
	float GetCurrentDistance() const { return std::round(currentDistance); }

	//最高飛距離をリセットする関数
	void ResetMaxDistance() { maxDistance = 0.0f; }

	bool GetDistanceLock() const { return isDistanceLocked; }

private:

	FontRenderer ballDistanceFont;

	//フォントの位置とサイズと色
	DirectX::XMFLOAT2 fontPosition;
	float fontSize = 1.0f;
	DirectX::XMFLOAT4 fontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	float currentDistance = 0.0f;
	float maxDistance = 0.0f;
	char distanceText[32] = "";
	bool hasDistanceText = false;
	bool isDistanceLocked = false;
	
	struct DistanceBackData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<DistanceBackData> distanceBackData;
	std::unique_ptr<sprite> distanceBackSprite;


	DirectX::XMFLOAT2 distanceBackSize = { 200.0f, 50.0f };
	DirectX::XMFLOAT2 distanceBackPosition = { 0.5f, 0.1f }; // 画面中央上部
	DirectX::XMFLOAT4 distanceBackColor = { 1.0f, 1.0f, 1.0f, 0.8f }; // 半透明黒

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;
};
