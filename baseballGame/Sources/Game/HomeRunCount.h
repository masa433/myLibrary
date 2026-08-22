#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "shader.h"
#include "sprite.h"
#include "FontRenderer.h"
#include <json.hpp>

using json = nlohmann::json;

class HomeRunCount
{
public:
	//インスタンス
	static HomeRunCount& Instance()
	{
		static HomeRunCount instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(nlohmann::json& j);
	void LoadFromJson(const nlohmann::json& j);

	void IncrementCount() { homeRunCount++; totalHomeRunCount++; }
	int GetHomeRunCount() const { return homeRunCount; }
	int GetTotalHomeRunCount() const { return totalHomeRunCount; }

	void ResetCount()
	{
		homeRunCount = 0;
		previousHomeRunCount = 0;
		numberDisplayScale = 1.0f; 
		numberAlpha = 1.0f; 
	}

	//現在のホームラン数が目標値を超えているかどうかを判定する関数
	bool IsHomeRunCountExceeded(int targetCount) const
	{
		return homeRunCount >= targetCount;
	}

private:
	int homeRunCount = 0;//ホームラン数を保持する変数
	int previousHomeRunCount = 0;//前回のホームラン数を保持する変数
	int totalHomeRunCount = 0; //累計ホームラン数を保持する変数

	//スプライトデータ
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<sprite> homeRunCountSprite;
	std::unique_ptr<Sprite> homeRunCountSpriteData;

	FontRenderer homeRunCountFont;
	FontRenderer missionFont;

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	//フォントの位置やサイズ、色などの設定
	float labelPositionX = 10.0f;
	float labelPositionY = 10.0f;
	float labelScale = 1.0f;
	float numberPositionX = 10.0f;
	float numberPositionY = 50.0f;
	float numberScale = 1.0f;
	DirectX::XMFLOAT4 countColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 slashColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 targetColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float goldColorTime = 0.0f; // ゴールドカラーの時間経過を追跡する変数

	DirectX::XMFLOAT2 missionLabelPosition = { 10.0f, 100.0f };
	float missionLabelScale = 1.0f;

	//カウントポップアニメーション
	float numberDisplayScale = 1.0f;      // 実際に描画に使う現在のスケール
	float numberPopScaleMultiplier = 2.0f; // 増えた瞬間に何倍まで大きくするか
	float numberScaleAnimSpeed = 3.0f;    // 元のサイズへ戻る速度（大きいほど速い）
	float slashDisplayScale = 1.0f;      // 実際に描画に使う現在のスケール
	float targetDisplayScale = 1.0f; // 元のサイズに戻すための目標スケール
	//透明度を徐々に0にするための変数
	float numberAlpha = 1.0f; // 現在の透明度
	float alphaDecreaseSpeed = 1.0f; // 透明度を減少させる速度（大きいほど速い）

	bool isAnimating = false; // アニメーション中かどうかのフラグ
};