#pragma once
#include "scene.h"
#include <array>
#include <string>
#include <vector>
#include <memory>
#include "imgui.h"
#include "Player.h"
#include "ScrollView.h"
#include "ButtonManager.h"
#include "FontRenderer.h"
#include "sprite.h"
#include "Pitcher.h"

class batterSelectScene : public scene
{
public:
	batterSelectScene() {};
	~batterSelectScene() = default;
	void initialize() override;
	void update(float elapsed_time) override;
	void render(float elapsedTime) override;
	void uninitialize() override;
	void DrawGUI() override;

private:

	//	選手のリスト
	std::unique_ptr<ScrollView> playerScrollView;
	
	ButtonManager buttonManager; // ボタンマネージャーのインスタンス
	FontRenderer fontRenderer;
	char text[32] = "batter";
	DirectX::XMFLOAT2 fontPosition = { 960.0f, 540.0f };
	float fontSize = 2.0f;
	DirectX::XMFLOAT4 fontColor = { 1.0f,1.0f,1.0f,1.0f };

	struct BatterSelectSpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<BatterSelectSpriteData> batterParamData;
	std::unique_ptr<sprite> batterParamSprite;
	std::unique_ptr<BatterSelectSpriteData> backGroundData;
	std::unique_ptr<sprite> backGroundSprite;

	DirectX::XMFLOAT2 scrollViewPosition = { 100.0f, 100.0f };
	DirectX::XMFLOAT2 scrollViewSize = { 400.0f, 400.0f };

	//シェーダー関係
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;

	//ピッチャーのスプライト
	struct PitcherSpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	int pitcherCount = 21; // ピッチャーの数

	//21人のピッチャーのスプライトデータを保持する配列
	std::unique_ptr<PitcherSpriteData> pitcherSpriteDataArray[21];
	std::unique_ptr<sprite> pitcherSprites[21];

	
	Pitcher::RealPitcher selectedPitcher = Pitcher::RealPitcher::None; // 選択されたピッチャーの初期値をNoneに設定
	size_t selectedPitcherIndex = 0; // 選択されたピッチャーのインデックスを保持する変数

public:
	void SelectRandomPitcher(); // ランダムにピッチャーを選択する関数

};