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

struct BatterEntry
{
	std::string name;
	int power;
	bool isRightBatter;
};

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
	//スクロールビュー
	void DrawPlayerScrollView();

private:

	//	選手のリスト
	std::vector<BatterEntry> playerList;
	std::unique_ptr<ScrollView> playerScrollView;
	int selectedPlayerIndex = -1; // 選択された選手のインデックス

	ButtonManager buttonManager; // ボタンマネージャーのインスタンス
	FontRenderer fontRenderer;
	char text[32] = "batter";
	DirectX::XMFLOAT2 fontPosition = { 960.0f, 540.0f };
	float fontSize = 2.0f;
	DirectX::XMFLOAT4 fontColor = { 1.0f,1.0f,1.0f,1.0f };

	struct BatterParamSpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<BatterParamSpriteData> batterParamData;
	std::unique_ptr<sprite> batterParamSprite;

	//シェーダー関係
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
	
};