#pragma once
#include "physxManager.h"
#include "Misc.h"
#include "sprite.h"
#include "Graphics.h"
#include "FontRenderer.h"
#include "json.hpp"

using json = nlohmann::json;

// physxManagerクラスで算出したトラッキングデータ(打球角度と打球速度)をスプライトで表示するクラス

class TrackingData
{
public:

	//インスタンス
	static TrackingData& Instance()
	{
		static TrackingData instance;
		return instance;
	}

	TrackingData() {}
	~TrackingData() {}

	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	//トラッキングデータが表示されているかどうかのゲッター
	bool IsTrackingDataVisible() const { return showTrackingData; }

	void Reset();

private:
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};
	std::unique_ptr<sprite> trackingDataSprite;
	std::unique_ptr<Sprite> trackingDataSpriteData;
	bool showTrackingData = false;
	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	FontRenderer trackingDataFont;
	float trackingDataFontScale = 1.0f;
	float trackingDataValueFontScale = 1.5f; // 数値用スケール（大きめ）
	float showTrackingDelay = 0.0f; // トラッキングデータ表示までの遅延時間
	//表示開始時間
	float displayStartTime = 0.7f;

	// 各行のラベル/数値位置を個別に微調整するためのオフセット
	DirectX::XMFLOAT2 angleLabelOffset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 angleValueOffset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 speedLabelOffset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 speedValueOffset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 directionLabelOffset = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 directionValueOffset = { 0.0f, 0.0f };

};