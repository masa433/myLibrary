#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "BroadcastCamera.h"
#include "sprite.h"

#define PITCHER_COUNT 21
#define BATTER_COUNT 24

class GameIntroSequence
{
public:

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
		ShowingPitcher,
		ShowingBatter,
		Playing,
	};

	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void UpdateIntro(float elapsedTime,BroadcastCamera& broadcastCamera);// イントロの更新処理
	void Render();

	void DrawFillHorizontal(sprite* spr, IntroData* data, ID3D11DeviceContext* context,
		DirectX::XMFLOAT2& pos,DirectX::XMFLOAT2& size, float amount);

	GameIntroState GetIntroState() const { return introState; }
	bool IsPlaying() const { return introState == GameIntroState::Playing; }

private:
	GameIntroState introState = GameIntroState::ShowingPitcher;
	float introTimer = 0.0f;// イントロのタイマー
	bool introStarted = false;
	float introDuration = 10.0f; // イントロの表示時間(カメラのズーム終了までにかかる時間)

	int selectedPitcherIndex = 1; // 選択されたピッチャーのインデックス
	int selectedBatterIndex = 1; // 選択されたバッターのインデックス
	float introAmount = 1.0f; // イントロの進行度（0.0から1.0）

	float amountTimer = 0.0f; // 進行度のタイマー
	float amountDuration = 1.0f; // 進行度が1.0になるまでの時間

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

	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;
};