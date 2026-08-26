#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include "FontRenderer.h"
#include "sprite.h"
#include "json.hpp"

#define ABILITY_COUNT 17

using json = nlohmann::json;

class SpecialAbility
{
public:
	static SpecialAbility& Instance()
	{
		static SpecialAbility instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	void InitializeAbilities(ID3D11Device* device, ID3D11DeviceContext* context);

private:

	//テクスチャ関連
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<Sprite> abilitySpriteData[ABILITY_COUNT];
	std::unique_ptr<sprite> abilitySprite[ABILITY_COUNT];
};