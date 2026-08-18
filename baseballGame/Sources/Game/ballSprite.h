#pragma once
#include <d3d11.h>
#include <directXmath.h>
#include <memory>
#include <wrl.h>
#include <deque>
#include <array>
#include "sprite.h"
#include "json.hpp"
#include "Pitcher.h"
#include "FontRenderer.h"


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

	bool GetShowBallBoard() const { return showBallBoard; }
	void SetShowBallBoard(bool value) { showBallBoard = value; }

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
	std::unique_ptr<sprite> ballBoardSprite;
	std::unique_ptr<Sprite> ballBoardSpriteData;
	std::unique_ptr<sprite> ballTargetSprite;
	std::unique_ptr<Sprite> ballTargetSpriteData;

	bool showBallBoard = false;
	

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	float showSpriteDelay = 0.7f; // スプライト表示の遅延時間
	float showSpriteTimer = 0.0f; // スプライト表示のタイマー

	bool isStrike = false;
	bool isBall = false;
	bool isPitchJudgedStrike = false;

public:

	bool IsStrike() const { return isStrike; }
	bool IsBall() const { return isBall; }
	bool IsPitchJudgedStrike() const { return isPitchJudgedStrike; }

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
	ballBreak2D pitchBreaks[19] =
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
		{  20.0f,  0.0f },  // Sweeper
		{  3.0f,  -5.0f },  // Palm
		{  0.0f,  0.0f },  // NaturalShoot
		{  0.0f,  0.0f },  // TrueSlider
		{  0.0f,  0.0f }   // BlazingFastball
	};

	const int PITCH_TYPE_COUNT = 19;  // 球種の数

	int currentPitchIndex = 0;  // 現在の球種インデックス
	int GetCurrentPitchIndex() const { return currentPitchIndex; }

	//ストライクゾーンのグリッド（3x3）
	DirectX::XMFLOAT2 strikeZoneGrid[3][3] =
	{
		{{ -0.2f,  0.52f }, {  0.0f,  0.52f }, {  0.2f,  0.52f } },
		{ { -0.2f,  0.8f }, {  0.0f,  0.8f }, {  0.2f,  0.8f } },
		{ { -0.2f, 1.08f }, {  0.0f, 1.08f }, {  0.2f, 1.08f }},
	};

	//ボールゾーンのグリッド（5x5）
	DirectX::XMFLOAT2 ballZoneGrid[5][5] =
	{
		{ { -0.21f,  0.5f }, { -0.143f,  0.5f }, {  0.0f,  0.5f }, {  0.143f,  0.5f }, { 0.21f, 0.5f } },
		{ { -0.21f,  0.6f }, { -0.143f,  0.6f }, {  0.0f,  0.6f }, {  0.143f,  0.6f }, { 0.21f, 0.6f } },
		{ { -0.21f,  0.8f }, { -0.143f,  0.8f }, {  0.0f,  0.8f }, {  0.143f,  0.8f }, { 0.21f, 0.8f } },
		{ { -0.21f, 1.0f }, { -0.143f, 1.0f }, {  0.0f, 1.0f }, {  0.143f, 1.0f }, { 0.21f, 1.0f } },
		{ { -0.21f, 1.1f }, { -0.143f, 1.1f }, {  0.0f, 1.1f }, {  0.143f, 1.1f }, { 0.21f, 1.1f } },
	};

	bool useBallBreak = false;

	// グレード(F～S)を表示用文字列に変換
	static const char* GetBreakGradeLabel(Pitcher::BreakGrade grade)
	{
		switch (grade)
		{
		case Pitcher::BreakGrade::F: return "F";
		case Pitcher::BreakGrade::E: return "E";
		case Pitcher::BreakGrade::D: return "D";
		case Pitcher::BreakGrade::C: return "C";
		case Pitcher::BreakGrade::B: return "B";
		case Pitcher::BreakGrade::A: return "A";
		case Pitcher::BreakGrade::S: return "S";
		default: return "";
		}

		return "C";
	}

	//グレードを基準変化量のCに対する倍率に変換
	static float GetBreakGradeScale(Pitcher::BreakGrade grade)
	{
		switch (grade)
		{
		case Pitcher::BreakGrade::F: return 0.5f;
		case Pitcher::BreakGrade::E: return 0.75f;
		case Pitcher::BreakGrade::D: return 0.9f;
		case Pitcher::BreakGrade::C: return 1.0f;
		case Pitcher::BreakGrade::B: return 1.1f;
		case Pitcher::BreakGrade::A: return 1.25f;
		case Pitcher::BreakGrade::S: return 1.5f;
		default: return 1.0f;
		}

		return 1.0f;
	}

	static const char* GetPowerGradeLabel(Pitcher::Power power)
	{
		switch (power)
		{
		case Pitcher::Power::F: return "F";
		case Pitcher::Power::E: return "E";
		case Pitcher::Power::D: return "D";
		case Pitcher::Power::C: return "C";
		case Pitcher::Power::B: return "B";
		case Pitcher::Power::A: return "A";
		case Pitcher::Power::S: return "S";
		default: return "";
		}
	}

	// 球威グレード → 打球の「弾き返しにくさ」倍率
	// 大きいほど球威が強く、打球速度計算時にこの値で割ることで打球を弱くする
	static float GetPowerGradeScale(Pitcher::Power power)
	{
		switch (power)
		{
		case Pitcher::Power::F: return 1.10f; // 弾き返しやすい（軽い球）
		case Pitcher::Power::E: return 1.06f;
		case Pitcher::Power::D: return 1.03f;
		case Pitcher::Power::C: return 1.0f;  // 基準
		case Pitcher::Power::B: return 0.97f;
		case Pitcher::Power::A: return 0.94f;
		case Pitcher::Power::S: return 0.9f; // 弾き返しにくい（重い球）
		default: return 1.0f;
		}
	}

	//投手1人分・球種16個分の変化量を設定する
	struct PitchBreakSet
	{
		ballBreak2D baseBreaks[19];
		ballBreak2D breaks[19];
		Pitcher::BreakGrade grades[19] = {
			Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C,
			Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C,
			Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C,
			Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C,
			Pitcher::BreakGrade::C, Pitcher::BreakGrade::C, Pitcher::BreakGrade::C
		};

		Pitcher::Power powerGrades[19] = {
			Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
			Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
			Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
			Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
			Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C
		};
		bool initialized = false;
	};

	Pitcher::Power pitchPowers[19] = {
	Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
	Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
	Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
	Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C,
	Pitcher::Power::C, Pitcher::Power::C, Pitcher::Power::C
	};

	//現在投げている球種の球威スケールを取得
	float GetCurrentPitchPowerScale() const
	{
		return GetPowerGradeScale(realPitcherBreaks[static_cast<size_t>(lastAppliedPitcher)].powerGrades[currentPitchIndex]);
	}

	// インデックスは Pitcher::RealPitcher の値（Noneは未使用）
	// 投手ごとに完全に独立したデータを持つため、他の投手の値を書き換えることはない
	std::array<PitchBreakSet, static_cast<size_t>(Pitcher::RealPitcher::Count)> realPitcherBreaks;

	Pitcher::RealPitcher lastAppliedPitcher = Pitcher::RealPitcher::None; // 最後に適用した投手の種類

	// 実在投手の選択が変わったことを検知し、その投手専用の変化量をpitchBreaksへ反映する
	void SyncRealPitcherBreaks();

	// 指定投手の変化量セットを（初回のみ）Pitcherの持ち球データから生成する
	void BuildRealPitcherBreakSet(Pitcher::RealPitcher rp);

public:
	DirectX::XMFLOAT2 aiTargetScreen = { 0.0f, 0.0f }; // AIが狙うターゲット位置（スクリーン座標）
	bool hasAITarget = false; // AIがターゲット位置を設定したかどうか

	//外部から3D座標に変換して取得する
	DirectX::XMFLOAT2 GetAITarget3D() const;
	void SetAITargetFromWorld(float worldX, float worldY);

	void GetBallZoneScreenBounds(DirectX::XMFLOAT2& outTopLeft, DirectX::XMFLOAT2& outBottomRight) const;

	DirectX::XMFLOAT2 GetBallSpritePosition() const
	{
		if (!ballDebugSpriteData) return {};
		return ballDebugSpriteData->position;
	}
	DirectX::XMFLOAT2 GetBallSpriteSize() const
	{
		if (!ballDebugSpriteData) return { 20.f, 20.f };
		return ballDebugSpriteData->size;
	}

	bool showHitJudgeDebug = false;

	void GetStrikeZoneScreenBounds(DirectX::XMFLOAT2& outTopLeft, DirectX::XMFLOAT2& outBottomRight) const;

	bool strikeJudgeDone = false;// ストライク判定が完了したかどうか

	DirectX::XMFLOAT2 startScreenPos = { 0.0f, 0.0f }; // 投球開始時のボール位置（スクリーン座標）
	DirectX::XMFLOAT2 finalScreenPos = { 0.0f, 0.0f }; // AIが最終的に狙うターゲット位置（スクリーン座標）
	bool aiTargetLocked = false; // AIがターゲット位置をロックしたかどうか
	float zoneCenterY = 0.0f; // ストライクゾーンの中心Y座標（スクリーン座標）

	float GetZoneCenterY() const { return zoneCenterY; }
	DirectX::XMFLOAT2 GetStartScreenPos() const { return startScreenPos; }
	DirectX::XMFLOAT2 GetFinalScreenPos() const { return finalScreenPos; }
	float yMoveScale = 1.0f; // ストライクゾーンの縦方向の動きのスケール（1.0で通常、0.5で半分の動き）
	float GetYMoveScale(int currentPitchIndex, float finalScreenPosY, float startScreenPosY, float zoneCenterY);

	//3Dのボールとバットが当たった段階で、2Dボールの動きを止める
	// これをtrueにすると、2Dボールは当たった位置で止まる
	bool stopBallOnHit = false;
	bool SetStopBallOnHit(bool value) { stopBallOnHit = value; return stopBallOnHit; }

	bool prevPitchingState = false;
	ballBreak2D activePitchBreak = {};
	int activePitchIndex = 0;

public:
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;

public:
	FontRenderer pitchInfoFont;
	float pitchInfoFontScale = 1.0f;

	// 球種ごとの表示位置オフセット（19球種分）
	DirectX::XMFLOAT2 pitchNameOffsets[19] = {
		{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},
		{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},
		{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f},{50.0f,10.0f}
	};
	DirectX::XMFLOAT2 pitchSpeedOffsets[19] = {
		{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},
		{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},
		{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f},{0.0f,10.0f}
	};

	// 表示色（球種名は固定なのでここでは球速の通常色のみ使う）
	DirectX::XMFLOAT4 pitchSpeedNormalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 pitchSpeedFastColor = { 1.0f, 0.9f, 0.0f, 1.0f }; // 黄色
	DirectX::XMFLOAT4 pitchSpeedHighFastColor = { 1.0f, 0.5f, 0.0f, 1.0f }; // オレンジ色
	float pitchSpeedFastThresholdKmh = 150.0f;
	float pitchSpeedHighFastThresholdKmh = 160.0f; // これ以上の球速はさらに強調表示

	enum class BallDisplayMode
	{
		None,
		Target,
		Ball,
	};
	BallDisplayMode display = BallDisplayMode::Target;

public:
	struct BallSpinFlip
	{
		std::unique_ptr<Sprite> spriteData;
		std::unique_ptr<sprite> sprite;
		int frameCount = 0;//スプライトシートのフレーム数
		int frameW = 400;//スプライトシートのフレーム幅
		int frameH = 400;//スプライトシートのフレーム高さ
		float frameAccum = 0.0f;//フレームの累積時間
 	};

	BallSpinFlip straightFlip;
	BallSpinFlip forkFlip;
	BallSpinFlip leftCurveFlip;
	BallSpinFlip rightCurveFlip;
	BallSpinFlip sliderFlip;
	BallSpinFlip verticalSliderFlip;
	BallSpinFlip* currentSpinFlip = nullptr; // 今の球種に対応するもの

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> scissorRasterizerState;

	void InitSpinFlip(ID3D11Device* device, ID3D11DeviceContext* context);
	void SelectSpinFlipForPitch(int pitchBreakIndex, bool isRightPitcher);
	void UpdateSpinFlip(float elapsedTime);
	void RenderSpinFlip(ID3D11DeviceContext* dc, BallSpinFlip& flip, bool reverse, 
		const DirectX::XMFLOAT2& screenPos, const DirectX::XMFLOAT2& screenSize,
		const DirectX::XMFLOAT4& color);

	bool currentSpinReverse = false; // 現在の球種のスピン反転状態（右投手か左投手か）
	float currentSpinRPM = 1500.0f; // 現在の球種のスピン回転数（RPM）
};