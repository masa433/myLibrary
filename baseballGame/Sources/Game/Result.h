#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "FontRenderer.h"
#include <json.hpp>
#include "HomeRunCount.h"
#include "ballDistance.h"
#include "ButtonManager.h"
#include "Hextransitioneffect.h"

enum class State
{
	Result,
	Transition,
};


using json = nlohmann::json;

class Result
{
public:
	//インスタンス
	static Result& Instance()
	{
		static Result instance;
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

	std::unique_ptr<Sprite> resultSpriteData;
	std::unique_ptr<sprite> resultSprite;

	DirectX::XMFLOAT2 spritePosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 spriteSize = { 1920.0f, 1080.0f };
	DirectX::XMFLOAT4 spriteColor = { 1.0f, 1.0f, 1.0f, 0.5f };

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	FontRenderer resultFont;

	DirectX::XMFLOAT2 homeRunFontPosition = { 700.0f, 400.0f };
	float homeRunFontSize = 3.0f;
	DirectX::XMFLOAT4 homeRunFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	DirectX::XMFLOAT2 distanceFontPosition = { 700.0f, 600.0f };
	float distanceFontSize = 3.0f;
	DirectX::XMFLOAT4 distanceFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };


	ButtonManager buttonManager;
	bool isResultToTitle = false;
	bool isResultToRetry = false;
	bool isResultToBatterSelect = false;


	HexTransitionEffect hexTransitionEffect;

	State currentState = State::Result;
};