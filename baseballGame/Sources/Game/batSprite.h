#pragma once
#include <d3d11.h>
#include <directXmath.h>
#include <memory>
#include <wrl.h>
#include <deque>
#include "sprite.h"
#include "json.hpp"
#include "Player.h"

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


};