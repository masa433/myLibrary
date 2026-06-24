#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "../Model/gltf_model.h"
#include "game_object.h"
#include "RenderContext.h"
#include "physxManager.h"
#include "ModelRenderer.h"
#include <deque>
#include "sprite.h"
#include "Ball.h"
#include "json.hpp"

using json = nlohmann::json;

class Pitcher : public GameObject
{
public:
	//インスタンス
	static Pitcher& Instance()
	{
		static Pitcher instance;
		return instance;
	}
	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();

	void AttachBallToHand(float elapsedTime);

	void UpdateAnimation(float elapsedTime);

	void UpdateBallCollider();

	void ApplyPhysicsToBall(float elapsedTime);

	void SelectPitchType();

	void ResetBall();

	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	void ThrowBallBezier();

public:

	enum class PitchType
	{
		Fastball,//ストレート
		Slider,//スライダー
		Curveball,//カーブ
		Changeup,//チェンジアップ
		Forkball,//フォーク
		TwoSeam,//ツーシーム
		Cutter,//カットボール
		Sinker,//シンカー
		VerticalSlider,//縦スライダー	
		Splitter,//スプリット
		SlowCurve,//スローカーブ
		Shooter,//シュート
		Knuckleball,//ナックル
		SlowBall,//スローボール
	};

	bool IsBallInStrikeZone() const;

	PitchType GetSelectedPitchType() const { return selectedPitchType; }
	bool GetIsBallThrown() const { return isBallThrown; }

private:
	// モデル関連
	std::unique_ptr<gltf_model> pitcher;
	std::vector<gltf_model::node> animated_nodes;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
	// アニメーション関連
	float animation_time = 0.0f;
	int current_animation_index = 0;
	bool animation_playing = true;

	DirectX::XMFLOAT3 ballStartPosition = { 0.0f, 0.0f, 0.0f };
	float throwTiming = 0.4f;
	bool isBallThrown = false;

	// ボール投球制御
	float ballSpeedKmh = 150.0f; // 投球速度（km/h） - デバッグ可能
	float launchAngleDegrees = -2.5f; // 発射角度（度）
	DirectX::XMFLOAT3 rotationSpeed = { 0.0f, 0.0f, 0.0f }; // 回転速度（度/秒）
	DirectX::XMFLOAT3 throwDirection = { 0.02f, 0.2f, -1.0f }; // 投球方向

	float ballDebugRadius = 0.15f; // デフォルトのスケール倍率
	float reducedRadius = 0.0f;

public:


	// ballSprite::pitchBreaks[14] のインデックス（GUIコンボボックス順）と
	// PitchType enum の値を対応させる変換関数。
	// pitchBreaks の並び順: Fastball,TwoSeam,Cutter,Slider,Curveball,Changeup,
	//                        Forkball,Sinker,VerticalSlider,Splitter,SlowCurve,
	//                        Shooter,Knuckleball,SlowBall
	static int PitchTypeToBreakIndex(PitchType type)
	{
		switch (type)
		{
		case PitchType::Fastball:       return 0;
		case PitchType::TwoSeam:        return 1;
		case PitchType::Cutter:         return 2;
		case PitchType::Slider:         return 3;
		case PitchType::Curveball:      return 4;
		case PitchType::Changeup:       return 5;
		case PitchType::Forkball:       return 6;
		case PitchType::Sinker:         return 7;
		case PitchType::VerticalSlider: return 8;
		case PitchType::Splitter:       return 9;
		case PitchType::SlowCurve:      return 10;
		case PitchType::Shooter:        return 11;
		case PitchType::Knuckleball:    return 12;
		case PitchType::SlowBall:       return 13;
		default:                        return 0;
		}
	}

private:
	PitchType selectedPitchType = PitchType::Fastball;

	struct PitchParameter
	{
		float ballSpeedKmh; // 投球速度（km/h）
		float launchAngleDegrees; // 発射角度（度）
		DirectX::XMFLOAT3 throwDirection; // 投球方向
		DirectX::XMFLOAT3 spinAxis; // 回転軸
		float rpm; // 回転数（回転/分）
		DirectX::XMFLOAT3 visualRotationSpeed;// 見た目の回転速度（度/秒）
		DirectX::XMFLOAT3 visualAngle; // 見た目の角度（度）

		//ベジェ曲線制御用オフセット
		DirectX::XMFLOAT3 bezierCtrl1 = { 0.0f, 0.0f, 0.0f }; //第1制御点
		DirectX::XMFLOAT3 bezierCtrl2 = { 0.0f, 0.0f, 0.0f }; //第2制御点
		DirectX::XMFLOAT3 bezierTarget = { 0.0f, 0.0f, 0.0f }; //ターゲット点

	};
	std::vector<PitchParameter> pitchParameters;

	static constexpr int PITCH_TYPE_COUNT = 14; // 球種の数

	int editerPitchIndex = 0; // エディタで選択された球種のインデックス

	void InitializePitchSettings(); // 球種のパラメーターを初期化する関数
	void SelectPitchTypeByAI();
	void ApplyAIGridTargetToPitch();
	PitchType ChooseAIPitchType() const;
	float GetSpeedVarianceKmh(PitchType pitchType) const;
	const char* GetPitchTypeName(PitchType pitchType) const;

	// ===== 2Dスプライトの変化量(breakX/breakY)から3Dの回転(角速度ベクトル)を逆算する =====
	// useBallBreakがオンの場合、ballSprite側のbreakX/breakY(cm)から
	// 目標の横変化・縦変化を再現するための回転軸とrpmを逆算して返す。
	// useBallBreakがオフの場合は従来のGetSpinAxisFromPitchType()と同じ結果を返す。
	physx::PxVec3 GetSpinFromBreakOrDefault(float speedMs) const;

	bool usePitchAI = true;
	float aiStrikeRate = 0.92f;
	float aiNearBallMargin = 0.06f;


public:
	// 5x5グリッド内でAIが狙う内側3x3のセルインデックス (0〜8、row-major)
	
	int aiTargetZoneIndex = 4;//ストライクゾーン内のインデックス
	int aiTargetZoneRow = 2; //5x5グリッドの行番号(0〜4)
	int aiTargetZoneCol = 2; //5x5グリッドの列番号(0〜4)

	DirectX::XMFLOAT2 aiTarget2D = { 0.0f, 0.0f }; // AIが狙うターゲット位置（2D平面上のX,Z座標）

	const DirectX::XMFLOAT2& GetAITarget2D() const { return aiTarget2D; }// AIが狙うターゲット位置（2D平面上のX,Z座標）を取得
	int GetAITargetRow() const { return aiTargetZoneRow; } // AIが狙うターゲット位置の行番号を取得
	int GetAITargetCol() const { return aiTargetZoneCol; } // AIが狙うターゲット位置の列番号を取得

private:
	//bool hasBeenJudged = false; // 判定済みフラグ


	float throwCounter = 0.0f; // 投球カウンター
	bool hasReachedZero = false; // z = 0.0f に到達したかどうか

	bool isRightPitcher = true; // 右投げかどうか

public:
	bool IsRightPitcher() const { return isRightPitcher; }

private:

	//ストライクゾーンのトリガーボックス
	physx::PxRigidStatic* strikeZoneTrigger = nullptr;
	DirectX::XMFLOAT3 boxSize = { 0.43f, 0.6f, 0.001f }; // トリガーボックスのサイズ
	DirectX::XMFLOAT3 boxPosition = { 0.0f, 0.8f, 0.0f }; // トリガーボックスの位置

public:

	// 状態管理
	enum class State
	{
		SelectingPitch,// 球種選択中
		Throwing,// 投球中

	};

	State currentState = State::SelectingPitch;
	float stateTime = 0.0f; // 現在の状態に入ってからの経過時間
	const State GetCurrentState() const { return currentState; }



private:
	// ===== 新規追加 =====
	physx::PxVec3 GetSpinAxisFromPitchType() const;


public:
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;

	DirectX::XMFLOAT2 targetPosition3D = { 0.0f, 0.0f }; // AIが狙うターゲット位置（3D空間上のX,Z座標）
	bool hasTargetSet = false; // AIがターゲット位置の接線を設定したかどうか
};