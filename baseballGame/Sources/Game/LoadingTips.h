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

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	int selectedTipIndex = 0; // 選択されたチップのインデックス

	FontRenderer tipFont;

	DirectX::XMFLOAT2 tipFontPosition = { 100.0f, 900.0f }; // チップの位置
	float tipFontScale = 1.0f; // チップのスケール
	DirectX::XMFLOAT4 tipFontColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // チップの色
};