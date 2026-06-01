#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>

#include "sprite.h"

class TextureManager
{
public:
	struct TextureAsset
	{
		std::wstring path;
		std::string name;
		D3D11_TEXTURE2D_DESC desc{};
	};

	struct TextureInstance
	{
		int assetIndex = 0;
		bool visible = true;
		DirectX::XMFLOAT2 position{ 0.0f, 0.0f };
		DirectX::XMFLOAT2 size{ 128.0f, 128.0f };
		float rotation = 0.0f;
		DirectX::XMFLOAT4 tint{ 1.0f, 1.0f, 1.0f, 1.0f };
	};

	void Initialize(ID3D11Device* device, const wchar_t* directory);
	void Render(ID3D11DeviceContext* context);
	void DrawGUI();
	void Clear();

private:
	void AddInstance(int assetIndex);
	void MoveSelected(int direction);
	void RemoveSelected();

	std::vector<TextureAsset> assets;
	std::vector<std::unique_ptr<sprite>> sprites;
	std::vector<TextureInstance> instances;
	int selectedAsset = 0;
	int selectedInstance = -1;
};
