#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "RenderContext.h"
#include "json.hpp"
#include "FontRenderer.h"

using json = nlohmann::json;

class ballCount
{
public:
	static ballCount& Instance()
	{
		static ballCount instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	int GetRemainingBalls() const { return remainingBalls; }
	void SetRemainingBalls(int balls) { remainingBalls = balls; }
	void ResetRemainingBalls() { remainingBalls = 10; } //球数をリセットする関数

	//球数を減らす関数
	void DecreaseRemainingBalls(int amount)
	{
		if (hasCountedHit) return;


		remainingBalls -= amount;
		if (remainingBalls < 0)
		{
			remainingBalls = 0;
		}
		hasCountedHit = true; // ヒットがカウントされたことを記録
	}
	void ResetHitFlag() { hasCountedHit = false; }

	FontRenderer pitchInfoFont; // 球種名と球速表示用のフォントレンダラー
private:
	
	struct BallCountData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<BallCountData> ballCountData;
	std::unique_ptr<sprite> ballCountSprite;
	DirectX::XMFLOAT2 ballCountPosition = { 100.0f, 50.0f }; // 画面左上に配置
	DirectX::XMFLOAT2 ballCountSize = { 100.0f, 50.0f };
	
	DirectX::XMFLOAT2 pitchInfoPosition = { 100.0f, 110.0f }; // 球種名と球速表示の位置
	float pitchInfoScale = 1.0f; // 球種名と球速表示のスケール
	DirectX::XMFLOAT4 pitchInfoColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 球種名と球速表示の色

	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;


	//残りの球数
	int remainingBalls = 10;
	bool hasCountedHit = false; //ヒット判定済みかどうか

};