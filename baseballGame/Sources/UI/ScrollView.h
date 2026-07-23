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
	ScrollView(ID3D11Device* device, const char* filePath, float topX, float topY, float width, float height);
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

	std::unique_ptr<ScrollBackData> scrollBackgroundSpriteData;
	std::unique_ptr<sprite> scrollBackgroundSprite;


	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
};