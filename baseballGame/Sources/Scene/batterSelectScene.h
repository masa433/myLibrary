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

#define PITCHER_IMAGE_COUNT 4 // ピッチャー画像の数
#define PITCHER_COUNT 21 // ピッチャーの数

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

	int GetCurrentChangePitcherCount() const { return currentChangePitcherCount; }
	int GetMaxChangePitcherCount() const { return maxChangePitcherCount; }

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

	
	//21人のピッチャーのスプライトデータを保持する配列
	std::unique_ptr<BatterSelectSpriteData> pitcherSpriteDataArray[PITCHER_COUNT];
	std::unique_ptr<sprite> pitcherSprites[PITCHER_COUNT];
	
	std::unique_ptr<BatterSelectSpriteData> pitcherNameSpriteData[PITCHER_COUNT];
	std::unique_ptr<sprite> pitcherNameSprite[PITCHER_COUNT];
	DirectX::XMFLOAT2 pitcherNamePosition = { 960.0f, 100.0f };
	DirectX::XMFLOAT2 pitcherNameSize = { 400.0f, 100.0f };
	DirectX::XMFLOAT4 pitcherNameColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	std::unique_ptr<BatterSelectSpriteData> closeButtonData;
	std::unique_ptr<sprite> closeButtonSprite;

	std::unique_ptr<BatterSelectSpriteData> pitcherImageSpriteDataArray[PITCHER_IMAGE_COUNT];
	std::unique_ptr<sprite> pitcherImageSprites[PITCHER_IMAGE_COUNT];
	int randomPitcherImageIndex = 0;
	
	Pitcher::RealPitcher selectedPitcher = Pitcher::RealPitcher::None; // 選択されたピッチャーの初期値をNoneに設定
	size_t selectedPitcherIndex = 0; // 選択されたピッチャーのインデックスを保持する変数

	std::unique_ptr<BatterSelectSpriteData> pitcherParamBackGroundData; // 選択されたピッチャーのパラメータ画像データを保持するポインタ
	std::unique_ptr<sprite> pitcherParamBackGroundSprite; // 選択されたピッチャーのパラメータ画像スプライトを保持するスマートポインタ

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

	std::vector<BurstEffectParam> burstList; // バーストエフェクトのパラメータを格納するベクター

	bool isNameTagClicked = false;// 名前タグがクリックされたかどうかのフラグ

private:

	enum class SequenceState
	{
		Selecting,// 選手選択中
		Transition,// 決定ボタン押下後のフェード演出
		Finished,// 遷移準備完了
		Reverting,// 戻るボタン押下後のフェード演出
		ShowPitcherParam,// ピッチャーのパラメータ表示中
	};

	SequenceState currentState = SequenceState::Selecting;

	float transitionTimer = 0.0f;// 遷移演出の経過時間

	float uiAlpha = 1.0f; // UIの透明度
	float returnAlpha = 0.0f; // 戻るボタンの透明度
	float batterImageAlpha = 0.0f;
	float paramImageAlpha = 0.0f; // パラメータ画像の透明度

	bool isChangingScene = false; // 設定がロードされたかどうかのフラグ

	DirectX::XMFLOAT2 modalPitcherPos = { 300.0f,100.0f };
	DirectX::XMFLOAT2 modalPitcherSize = { 600.0f,800.0f };

	DirectX::XMFLOAT2 modalBurstPosition = { 600.0f,550.0f };
	DirectX::XMFLOAT2 modalBurstSize = { 700.0f,700.0f };

	int currentChangePitcherCount = 0;
	int maxChangePitcherCount = 3;

	
};