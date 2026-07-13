#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "sprite.h"
#include "shader.h"
#include "RenderContext.h"
#include "json.hpp"

using json = nlohmann::json;

class FoulSprite
{
public:
	//インスタンス
	static FoulSprite& Instance()
	{
		static FoulSprite instance;
		return instance;
	}

	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	bool GetShowFoulSprite() const { return showFoulSprite; }
	void SetShowFoulSprite(bool show)
	{
		showFoulSprite = show;
		if (show) showDuration = 0.0f; // フール表示時間をリセット
	}


private:
	// スプライトのデータ
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<Sprite> foulSprite;
	std::unique_ptr<sprite> foulSpriteRenderer;

	bool showFoulSprite = false;

	float showDuration = 0.0f; // フール表示時間（秒）
	float maxShowTime = 1.5f; // フール表示時間の最大値（秒）

	//アルファ値の変化速度
	float alpha = 0.0f; // 初期アルファ値
	float fadeSpeed = 2.0f; // 1秒で完全に透明になる
	float fadeInTime = 0.1f;
	float displayTime = 1.0f;
	float fadeOutTime = 0.1f;

	//スケールアニメーション
	float scale = foulSpriteRenderer ? foulSprite->size.x * 1.5f : 1.0f; // 初期スケール


	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

};