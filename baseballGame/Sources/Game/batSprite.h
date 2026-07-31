#pragma once
#include <d3d11.h>
#include <directXmath.h>
#include <memory>
#include <wrl.h>
#include <deque>
#include "sprite.h"
#include "json.hpp"


using json = nlohmann::json;


class BatSprite
{
public:

	//インスタンス
	static BatSprite& Instance()
	{
		static BatSprite instance;
		return instance;
	}

	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	//バットのサイズとポジションのゲッター
	DirectX::XMFLOAT2 GetBatSpriteSize() const { return batSpriteData->size; }
	DirectX::XMFLOAT2 GetBatSpritePosition() const { return batSpriteData->position; }

	//バットカーソルのサイズとポジションのゲッター
	DirectX::XMFLOAT2 GetBatCursorSpriteSize() const { return batCursorSpriteData->size; }
	DirectX::XMFLOAT2 GetBatCursorSpritePosition() const { return batCursorSpriteData->position; }
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

	std::unique_ptr<sprite> batSprite;
	std::unique_ptr<Sprite> batSpriteData;
	std::unique_ptr<sprite> batCursorSprite;
	std::unique_ptr<Sprite> batCursorSpriteData;


	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	bool cursorClipped = false;

	DirectX::XMFLOAT2 originalCursorSize = { 100.0f,100.0f };
	float minCursorScale = 0.6f;
	float maxCursorScale = 1.3f;

public:
	void UpdateCursorSizeByContact(int contact);
};