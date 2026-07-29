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
#include "json.hpp"
#include "Hextransitioneffect.h"

using json = nlohmann::json;

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
	void SaveSetting();
	void LoadSetting();

private:

	HexTransitionEffect hexTransitionEffect; // ヘックス遷移エフェクトのインスタンス

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


	Microsoft::WRL::ComPtr<ID3D11VertexShader> burstVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> burstPixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> burstTransformBuffer; // VS用
	Microsoft::WRL::ComPtr<ID3D11Buffer> burstColorBuffer;     // PS用

	struct BurstTransformBuffer
	{
		DirectX::XMFLOAT2 center;
		DirectX::XMFLOAT2 size;
		DirectX::XMFLOAT2 screenSize;
		DirectX::XMFLOAT2 padding; // 16バイト境界に合わせるためのパディング
	};

	struct BurstBuffer
	{
		float time;
		float aspectRatio;
		float progress;
		float padding; // 16バイト境界に合わせるためのパディング
	};

	struct BurstEffectParam
	{
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float alpha;
		float time;
	};
	
	float burstElapsedTime = 0.0f;

	std::vector<BurstEffectParam> burstList; // バーストエフェクトのパラメータを格納するベクター

private:

	enum class SequenceState
	{
		Selecting,// 選手選択中
		Transition,// 決定ボタン押下後のフェード演出
		Finished,// 遷移準備完了
		Reverting,// 戻るボタン押下後のフェード演出
	};

	SequenceState currentState = SequenceState::Selecting;

	float transitionTimer = 0.0f;// 遷移演出の経過時間
	const float transitionDuration = 1.0f; // 遷移演出の総時間(秒)

	float uiAlpha = 1.0f; // UIの透明度(0.0f:完全透明, 1.0f:完全不透明)
	float burstAlpha = 0.0f; // バーストエフェクトの透明度(0.0f:完全透明, 1.0f:完全不透明)
	float returnAlpha = 1.0f; // 戻るボタンの透明度(0.0f:完全透明, 1.0f:完全不透明)

	DirectX::XMFLOAT2 burstPosition = { 960.0f, 540.0f }; // バーストエフェクトの中心位置
	DirectX::XMFLOAT2 burstSize = { 1920.0f, 1080.0f }; // バーストエフェクトのサイズ
	

	bool isChangingScene = false; // 設定がロードされたかどうかのフラグ
};