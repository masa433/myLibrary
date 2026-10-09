#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "FontRenderer.h"
#include <json.hpp>
#include "HomeRunCount.h"
#include "ballDistance.h"
#include "ButtonManager.h"
#include "Hextransitioneffect.h"
#include "BroadcastCamera.h"
#include "ReplayManager.h"

enum class State
{
	Replay,
	Result,
	Transition,
};


using json = nlohmann::json;

class Result
{
public:
	//インスタンス
	static Result& Instance()
	{
		static Result instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(nlohmann::json& j);
	void LoadFromJson(const nlohmann::json& j);

	//リプレイロゴを補間で移動させる関数
	void UpdateReplayLogoPosition(float elapsedTime);

	//リプレイロゴが半分移動したかどうかを取得する関数
	bool IsReplayLogoMovedHalf() const { return isReplayLogoMovingHalf; }

	//ロゴアニメーションがスタートしたかどうか
	bool IsReplayLogoAnimationStarted() const { return replayLogoAnimation.currentPosition.x != replayLogoAnimation.startPosition.x || replayLogoAnimation.currentPosition.y != replayLogoAnimation.startPosition.y; }

	//現在のステートを取得する関数
	State GetCurrentState() const { return currentState; }

	float CalcStartProgress(const ReplayFrame& frame) const;

	//ロゴの表示率を計算する関数
	float CalcLogoRevealRate(float t) const;

	float CalcLogoHoleRate(float t) const;

	void DrawLogoClipped(ID3D11DeviceContext* dc,
		float l, float t, float r, float b,
		const DirectX::XMFLOAT2& topLeft,
		const DirectX::XMFLOAT2& size,
		LONG screenW, LONG screenH);

	BroadcastCamera broadcastCamera;

private:

	bool hasReplay = false;//再生可能なリプレイがあるかどうか
	bool hasEnteredResult = false;//リザルト画面に入ったかどうか

	//スプライトデータ
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<Sprite> resultSpriteData;
	std::unique_ptr<sprite> resultSprite;

	DirectX::XMFLOAT2 spritePosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 spriteSize = { 1920.0f, 1080.0f };
	DirectX::XMFLOAT4 spriteColor = { 1.0f, 1.0f, 1.0f, 0.5f };

	std::unique_ptr<Sprite> replayTrackingBoard;
	std::unique_ptr<sprite> replayTrackingSprite;

	DirectX::XMFLOAT2 replayTrackingPosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 replayTrackingSize = { 500.0f, 350.0f };

	std::unique_ptr<Sprite> trackingArrow;
	std::unique_ptr<sprite> trackingArrowSprite;

	DirectX::XMFLOAT2 trackingArrowPosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 trackingArrowSize = { 100.0f, 100.0f };

	std::unique_ptr<Sprite> replayLogo;
	std::unique_ptr<sprite> replayLogoSprite;

	struct ReplayLogoAnimation
	{
		DirectX::XMFLOAT2 startPosition = { 960.0f, 540.0f };
		DirectX::XMFLOAT2 targetPosition = { 960.0f, 540.0f };
		DirectX::XMFLOAT2 endPosition = { 960.0f, 540.0f };
		DirectX::XMFLOAT2 currentPosition = startPosition;

		DirectX::XMFLOAT2 startSize = { 3840.0f, 2160.0f };
		DirectX::XMFLOAT2 targetSize = { 1920.0f, 1080.0f };
		DirectX::XMFLOAT2 endSize = { 3840.0f, 2160.0f };
		DirectX::XMFLOAT2 currentSize = startSize;

		float startRotation = -135.0f;
		float targetRotation = 0.0f;
		float endRotation = 0.0f;
		float currentRotation = startRotation;
		
	};

	ReplayLogoAnimation replayLogoAnimation;

	float replayLogoMoveTime = 0.0f;
	float replayLogoMoveDuration = 2.5f;
	bool isReplayLogoMovingHalf = false;
	bool isReplayLogoMovingAll = false;
	float startTimer = 0.0f;
	bool isNextButtonPressed = false;
	//ネクストボタンが描画されているか
	bool isNextButtonDrawn = false;

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	FontRenderer resultFont;

	//総ホームラン数の表示
	DirectX::XMFLOAT2 homeRunFontPosition = { 700.0f, 400.0f };
	float homeRunFontSize = 1.0f;
	DirectX::XMFLOAT4 homeRunFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//最高飛距離表示
	DirectX::XMFLOAT2 distanceFontPosition = { 700.0f, 600.0f };
	float distanceFontSize = 1.0f;
	DirectX::XMFLOAT4 distanceFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//最終的な所持金表示
	DirectX::XMFLOAT2 moneyFontPosition = { 700.0f, 800.0f };
	float moneyFontSize = 1.0f;
	DirectX::XMFLOAT4 moneyFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//最大コンボ数表示
	DirectX::XMFLOAT2 comboFontPosition = { 700.0f, 1000.0f };
	float comboFontSize = 1.0f;
	DirectX::XMFLOAT4 comboFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//到達ラウンド数表示
	DirectX::XMFLOAT2 roundFontPosition = { 700.0f, 1200.0f };
	float roundFontSize = 1.0f;
	DirectX::XMFLOAT4 roundFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//打球速度表示
	DirectX::XMFLOAT2 speedFontPosition = { 700.0f, 1400.0f };
	float speedFontSize = 1.0f;

	//打球角度表示
	DirectX::XMFLOAT2 angleFontPosition = { 700.0f, 1600.0f };
	float angleFontSize = 1.0f;

	ButtonManager buttonManager;
	bool isResultToTitle = false;
	bool isResultToRetry = false;
	bool isResultToBatterSelect = false;


	HexTransitionEffect hexTransitionEffect;

	State currentState = State::Result;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
};