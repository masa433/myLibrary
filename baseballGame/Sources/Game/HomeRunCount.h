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

private:
	int homeRunCount = 0;

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
};