#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "sprite.h"
#include "RenderContext.h"

class Wind
{
	public:
	static Wind& Instance()
	{
		static Wind instance;
		return instance;
	}
	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc);
	void DrawGUI();
	const DirectX::XMFLOAT3& GetWindDirection() const { return windDirection; }
	float GetWindStrength() const { return windStrength; }
	float GetWindHeight() const { return windHeight; }
	float GetWindThickness() const { return windThickness; }

	// 風の影響を受けるエリアにボールが入っているか
	bool IsBallInWindArea() const;

	const DirectX::XMFLOAT3 GetWindVector() const { return DirectX::XMFLOAT3(windDirection.x * windStrength, windDirection.y * windStrength, windDirection.z * windStrength); }

private:
	struct WindLine
	{
		DirectX::XMFLOAT3 position;
		float baseYOffset; // Y軸の相対的な位置割合 (0.0 ～ 1.0)
		float speed;
		float length;
		float phase; // アニメーションの位相
	};
	std::vector<WindLine> windLines;
	DirectX::XMFLOAT3 windDirection = { -1.0f, 0.0f, -0.5f };
	float windStrength = 5.0f;
	float windHeight = 20.0f;
	float windThickness = 50.0f;

	//スプライト関連
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};
	std::unique_ptr<Sprite> windDirectionSprite;
	std::unique_ptr<sprite> windDirectionSpriteRenderer;
	std::unique_ptr<Sprite> windGroundSprite;
	std::unique_ptr<sprite> windGroundSpriteRenderer;
	std::unique_ptr<sprite> windStrengthFontRenderer;
};