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

	void IncrementCount() { homeRunCount++; }
	int GetHomeRunCount() const { return homeRunCount; }

	void ResetCount()
	{
		homeRunCount = 0;
		previousHomeRunCount = 0;
		numberDisplayScale = 3.0f; 
		numberAlpha = 1.0f; 
	}

private:
	int homeRunCount = 0;//ホームラン数を保持する変数
	int previousHomeRunCount = 0;//前回のホームラン数を保持する変数

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
	FontRenderer homeRunCountLabelFont;

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
	DirectX::XMFLOAT4 numberColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//カウントポップアニメーション
	float numberDisplayScale = 3.0f;      // 実際に描画に使う現在のスケール
	float numberPopScaleMultiplier = 1.8f; // 増えた瞬間に何倍まで大きくするか
	float numberScaleAnimSpeed = 6.0f;    // 元のサイズへ戻る速度（大きいほど速い）

	//透明度を徐々に0にするための変数
	float numberAlpha = 1.0f; // 現在の透明度
	float alphaDecreaseSpeed = 1.0f; // 透明度を減少させる速度（大きいほど速い）
};