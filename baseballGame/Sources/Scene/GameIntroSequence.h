#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "BroadcastCamera.h"
#include "sprite.h"
#include "FontRenderer.h"
#include "json.hpp"
#include "ButtonManager.h"

#define PITCHER_COUNT 21
#define BATTER_COUNT 24
#define GRAPH_COUNT 6

using json = nlohmann::json;

class GameIntroSequence
{
public:

	FontRenderer pitchParamFont;// ピッチャーのパラメータ表示用フォントレンダラー

	ButtonManager buttonManager; // ボタン管理クラスのインスタンス

	struct IntroData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	enum class GameIntroState
	{
		ShowingStand,
		ShowingGround,
		ShowingPitcher,
		ShowingBatter,
		ShowingIntroBoard,
		Playing,
	};

	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void UpdateIntro(float elapsedTime,BroadcastCamera& broadcastCamera);// イントロの更新処理
	void Render();
	void UpdateFadeIn(float elapsedTime); // フェードインの更新処理
	void UpdateFadeOut(float elapsedTime); // フェードアウトの更新処理
	void DrawGUI(); // GUIの描画処理
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	// 水平方向に塗りつぶす描画関数
	void DrawFillHorizontal(sprite* spr, IntroData* data, ID3D11DeviceContext* context,
		DirectX::XMFLOAT2& pos,DirectX::XMFLOAT2& size, float amount);

	// 中心から水平方向に塗りつぶす描画関数
	void DrawFillHorizontalFromCenter(sprite* spr, IntroData* data, ID3D11DeviceContext* context,
		DirectX::XMFLOAT2& pos, DirectX::XMFLOAT2& size, float amount);

	GameIntroState GetIntroState() const { return introState; }
	//ゲームが始まっているか
	bool IsPlaying() const { return introState == GameIntroState::Playing; }

	//グラフを左から右に投球割合の分だけ塗りつぶす描画関数
	void DrawFillGraph(sprite* spr, IntroData* data, ID3D11DeviceContext* context,
		DirectX::XMFLOAT2& pos, DirectX::XMFLOAT2& size, float amount);

private:
	GameIntroState introState = GameIntroState::ShowingGround;
	float introTimer = 0.0f;// イントロのタイマー
	bool introStarted = false;
	float introDuration = 10.0f; // イントロの表示時間(カメラのズーム終了までにかかる時間)

	int selectedPitcherIndex = 1; // 選択されたピッチャーのインデックス
	int selectedBatterIndex = 1; // 選択されたバッターのインデックス
	float introAmount = 1.0f; // イントロの進行度（0.0から1.0）

	float amountTimer = 0.0f; // 進行度のタイマー
	float amountDuration = 1.0f; // 進行度が1.0になるまでの時間

	float showNameBoardTimer = 0.0f; // スタジアム名ボードの表示タイマー
	float maxShowNameBoardTime = 9.0f; // スタジアム名ボードの最大表示時間

	//カメラのフェード用の変数
	float fadeTimer = 0.0f; // フェードのタイマー
	float fadeDuration = 1.0f; // フェードが完了するまでの時間
	float fadeAlpha = 1.0f; // フェードの透明度（0.0から1.0）
	bool isFadingIn = true; // フェードイン中かどうか
	bool isFadingOut = false; // フェードアウト中かどうか

	float graphAmount = 0.0f; // グラフの進行度（0.0から1.0）
	float graphDuration = 1.3f; // グラフが塗りつぶされるまでの時間
	float graphTimer = 0.0f; // グラフのタイマー

	float introBoardAlpha = 1.0f; // イントロボードの透明度（1.0から0.0）
	float introBoardFadeTimer = 0.0f; // イントロボードのフェードタイマー
	float introBoardFadeDuration = 2.0f; // イントロボードがフェードアウトするまでの時間

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;

private:
	std::unique_ptr<IntroData> pitcherIntroSpriteData[PITCHER_COUNT];
	std::unique_ptr<sprite> pitcherIntroSprite[PITCHER_COUNT];

	DirectX::XMFLOAT2 pitcherIntroPosition = { 960.0f, 900.0f };
	DirectX::XMFLOAT2 pitcherIntroSize = { 1200.0f, 150.0f };

	std::unique_ptr<IntroData> batterIntroSpriteData[BATTER_COUNT];
	std::unique_ptr<sprite> batterIntroSprite[BATTER_COUNT];

	DirectX::XMFLOAT2 batterIntroPosition = { 960.0f, 900.0f };
	DirectX::XMFLOAT2 batterIntroSize = { 1200.0f, 150.0f };

	std::unique_ptr<IntroData> stadiumNameBoardData;
	std::unique_ptr<sprite> stadiumNameBoardSprite;

	DirectX::XMFLOAT2 stadiumNameBoardPosition = { 960.0f, 900.0f };
	DirectX::XMFLOAT2 stadiumNameBoardSize = { 1200.0f, 100.0f };

	std::unique_ptr<IntroData> cameraFadeData;
	std::unique_ptr<sprite> cameraFadeSprite;

	std::unique_ptr<IntroData> pitchParamData;
	std::unique_ptr<sprite> pitchParamSprite;

	DirectX::XMFLOAT2 pitchParamPosition = { 1350.0f, 450.0f };
	DirectX::XMFLOAT2 pitchParamSize = { 700.0f, 350.0f };

	std::unique_ptr<IntroData> graphData[GRAPH_COUNT];
	std::unique_ptr<sprite> graphSprite[GRAPH_COUNT];

	DirectX::XMFLOAT2 graphPosition = {1510.0f, 345.0f};
	DirectX::XMFLOAT2 graphSize = { 340.0f, 20.0f };

	std::unique_ptr<IntroData> introBoardData;
	std::unique_ptr<sprite> introBoardSprite;

	DirectX::XMFLOAT2 introBoardPosition = { 960.0f, 540.0f };
	DirectX::XMFLOAT2 introBoardSize = { 700.0f, 420.0f };
	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	DirectX::XMFLOAT2 pitchTypeFontPosition = { 1300.0f, 400.0f };
	float pitchTypeFontScale = 0.5f;

	DirectX::XMFLOAT2 pitchWeightFontPosition = { 1500.0f, 400.0f };
	float pitchWeightFontScale = 0.5f;

	DirectX::XMFLOAT2 pitchTypeLabelPosition = { 1300.0f, 350.0f };
	float pitchTypeLabelScale = 0.5f;

	DirectX::XMFLOAT2 pitchWeightLabelPosition = { 1500.0f, 350.0f };
	float pitchWeightLabelScale = 0.5f;

	float offsetY = 0.0f; // Y方向のオフセット値
};