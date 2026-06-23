#pragma once
#include <d3d11.h>
#include <directXmath.h>
#include <memory>
#include <wrl.h>
#include <deque>
#include "sprite.h"
#include "json.hpp"
#include "Pitcher.h"

using json = nlohmann::json;

class ballSprite
{
public:
	//インスタンス
	static ballSprite& Instance()
	{
		static ballSprite instance;
		return instance;
	}

	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	//3D座標から2Dスクリーン座標への変換
	DirectX::XMFLOAT2 CalculateBallTarget(const DirectX::XMFLOAT3& ballPosition, const DirectX::XMFLOAT3& ballVelocity);

	DirectX::XMFLOAT3 Convert2DTargetTo3D(const DirectX::XMFLOAT2& screenPos);

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

	std::unique_ptr<sprite> strikeZoneSprite;
	std::unique_ptr<Sprite> strikeZoneSpriteData;
	std::unique_ptr<sprite> ballDebugSprite;
	std::unique_ptr<Sprite> ballDebugSpriteData;

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

public:
	//2Dスクリーン座標上のストライクゾーンの中心とサイズ
	DirectX::XMFLOAT2 zone3DCenter = { 0.0f, 0.8f };
	DirectX::XMFLOAT2 zone3DSize = { 0.43f,0.6f };

	//ボールの軌跡
	std::deque<DirectX::XMFLOAT2> ballTrail2D;
	static constexpr int MAX_TRAIL = 60;
	bool showTrail = true;

	//前回の投球状態
	bool prevThrown = false;

	struct ballBreak2D
	{
		float breakX;
		float breakY;
	};

	//各球種の変化量
	ballBreak2D pitchBreaks[14] =
	{
		{  0.0f,  0.0f },  // Fastball
		{ -4.0f,  2.0f },  // TwoSeam
		{  5.0f,  1.0f },  // Cutter
		{ 12.0f, -4.0f },  // Slider
		{  8.0f,-14.0f },  // Curveball
		{ -3.0f, -5.0f },  // Changeup
		{ -1.0f,-12.0f },  // Forkball
		{ -8.0f, -6.0f },  // Sinker
		{  4.0f,-12.0f },  // VerticalSlider
		{  1.0f,-10.0f },  // Splitter
		{ 12.0f,-18.0f },  // SlowCurve
		{-10.0f, -2.0f },  // Shooter
		{  0.0f, -2.0f },  // Knuckleball
		{  0.0f, -3.0f },  // SlowBall
	};

	int currentPitchIndex = 0;  // 現在の球種インデックス

	//ストライクゾーンのグリッド（3x3）
	DirectX::XMFLOAT2 strikeZoneGrid[3][3] =
	{
		{{ -0.143f,  0.6f }, {  0.0f,  0.6f }, {  0.143f,  0.6f } },
		{ { -0.143f,  0.8f }, {  0.0f,  0.8f }, {  0.143f,  0.8f } },
		{ { -0.143f, 1.0f }, {  0.0f, 1.0f }, {  0.143f, 1.0f }},
	};

	//ボールゾーンのグリッド（5x5）
	DirectX::XMFLOAT2 ballZoneGrid[5][5] =
	{
		{ { -0.286f,  0.4f }, { -0.143f,  0.4f }, {  0.0f,  0.4f }, {  0.143f,  0.4f }, { 0.286f, 0.4f } },
		{ { -0.286f,  0.6f }, { -0.143f,  0.6f }, {  0.0f,  0.6f }, {  0.143f,  0.6f }, { 0.286f, 0.6f } },
		{ { -0.286f,  0.8f }, { -0.143f,  0.8f }, {  0.0f,  0.8f }, {  0.143f,  0.8f }, { 0.286f, 0.8f } },
		{ { -0.286f, 1.0f }, { -0.143f, 1.0f }, {  0.0f, 1.0f }, {  0.143f, 1.0f }, { 0.286f, 1.0f } },
		{ { -0.286f, 1.2f }, { -0.143f, 1.2f }, {  0.0f, 1.2f }, {  0.143f, 1.2f }, { 0.286f, 1.2f } },
	};

	bool useBallBreak = false;
};