#pragma once
#include "sprite.h"
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <memory>

class ScrollView
{
public:
	ScrollView(ID3D11Device* device, float topX, float topY, float width, float height);
	//ScrollView(ID3D11Device* device, const char* filePath, float topX, float topY, float width, float height);
	~ScrollView() {}
	void Render();
	void Update(float elapsedTime);
	void DrawGUI();

	

private:
	//スクロール背景スプライトデータ
	struct ScrollBackData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::vector<ScrollBackData> scrollBackgroundSpriteData;
	std::vector<std::unique_ptr<sprite>> scrollBackgroundSprite;

	ScrollBackData topCapData;    // 上端のフタ
	ScrollBackData bottomCapData; // 下端のフタ
	std::unique_ptr<sprite> topCapSprite;
	std::unique_ptr<sprite> bottomCapSprite;


	//スクロールビューのプレイヤーボタンデータ
	struct PlayerButtonData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::vector<PlayerButtonData> playerButtonDataList;
	std::vector<std::unique_ptr<sprite>> playerButtonSprites;


	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;

	int buttonCount = 24; // プレイヤーボタンの数
	float startPosX = 200.0f; // プレイヤーボタンの開始位置X
	float startPosY = 150.0f; // プレイヤーボタンの開始位置Y
	float buttonWidth = 300.0f; // プレイヤーボタンの幅
	float buttonHeight = 80.0f; // プレイヤーボタンの高さ
	float buttonSpacing = 10.0f; // プレイヤーボタンの間隔
	float scrollOffsetY = 0.0f; // スクロールのオフセットY

	int selectedIndex = -1; // 選択されたボタンのインデックス

public:
	void SetBackGroundTransform(float x, float y, float width, float height)
	{
		if (!scrollBackgroundSpriteData.empty())
		{
			scrollBackgroundSpriteData[0].position = { x + width / 2.0f, y + height / 2.0f };
			scrollBackgroundSpriteData[0].size = { width, height };
		}
	}

	PlayerButtonData* GetButtonData(size_t index)
	{
		if (index < playerButtonDataList.size())
		{
			return &playerButtonDataList[index];
		}
		return nullptr;
	}
};