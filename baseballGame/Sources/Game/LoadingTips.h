#pragma once
#include <d3d11.h>
#include <vector>
#include <string>
#include <random>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "shader.h"
#include "FontRenderer.h"

#define TIP_COUNT 16

class LoadingTips
{
public:
	static LoadingTips& Instance()
	{
		static LoadingTips instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(float alpha);
	void DrawGUI();
	//ランダムで一つのチップを選択する関数
	void SelectRandomTip();

	//矢印が押されたら次のチップを表示する関数
	void ShowNextTip();

	//矢印が押されたら前のチップを表示する関数
	void ShowPreviousTip();

	struct TipSpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<TipSpriteData> tipSpriteData[TIP_COUNT];
	std::unique_ptr<sprite> tipSprite[TIP_COUNT];

	std::unique_ptr<TipSpriteData> loadingBallSpriteData;
	std::unique_ptr<sprite> loadingBallSprite;

	std::unique_ptr<TipSpriteData> leftArrowData;
	std::unique_ptr<TipSpriteData> rightArrowData;
	std::unique_ptr<sprite> leftArrowSprite;
	std::unique_ptr<sprite> rightArrowSprite;

	const DirectX::XMFLOAT2 originalArrowSize = { 100.0f, 50.0f };
	const DirectX::XMFLOAT2 targetArrowSize = { 120.0f, 60.0f }; // 拡大後のサイズ

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	int selectedTipIndex = 0; // 選択されたチップのインデックス

	FontRenderer tipFont;

	DirectX::XMFLOAT2 tipFontPosition = { 100.0f, 900.0f }; // チップの位置
	float tipFontScale = 1.0f; // チップのスケール
	DirectX::XMFLOAT4 tipFontColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // チップの色

	FontRenderer loadingFont;

	DirectX::XMFLOAT2 loadingFontPosition = { 100.0f, 100.0f }; // ローディング文字の位置
	float loadingFontScale = 1.0f; // ローディング文字のスケール
	DirectX::XMFLOAT4 loadingFontColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // ローディング文字の色

	//loadingのドットアニメーション
	float dotAnimationTime = 0.0f;
	float dotAnimationInterval = 0.5f; // ドットのアニメーション間隔
	int currentDotCount = 3; // 現在のドットの数

	float angle = 0.0f; // ローディングボールの回転角度
};