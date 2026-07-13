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

class GameTimer
{
public:
	//インスタンス
	static GameTimer& Instance()
	{
		static GameTimer instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(nlohmann::json& j);
	void LoadFromJson(const nlohmann::json& j);

private:
	//スプライトデータ
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<sprite> timerSprite;
	std::unique_ptr<Sprite> timerSpriteData;

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	FontRenderer timerFont;

	//フォントの位置、サイズ、色を変えるための変数
	DirectX::XMFLOAT2 fontPosition;
	float fontSize;
	DirectX::XMFLOAT4 fontColor;

	//タイマーの値
	float startTime = 120.0f; // 120秒からスタート(2分)
	float remainingTime = 120.0f;

	int startCountdown = 10; // カウントダウンの初期値
};