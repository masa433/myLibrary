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

public:

	bool IsBallInStrikeZone() const;

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

	PitchType selectedPitchType;

	struct PitchParameter
	{
		float ballSpeedKmh; // 投球速度（km/h）
		float launchAngleDegrees; // 発射角度（度）
		DirectX::XMFLOAT3 throwDirection; // 投球方向
		DirectX::XMFLOAT3 spinAxis; // 回転軸
		float rpm; // 回転数（回転/分）
		DirectX::XMFLOAT3 visualRotationSpeed;// 見た目の回転速度（度/秒）
		DirectX::XMFLOAT3 visualAngle; // 見た目の角度（度）

	};
	std::vector<PitchParameter> pitchParameters;

	static constexpr int PITCH_TYPE_COUNT = 14; // 球種の数

	int editerPitchIndex = 0; // エディタで選択された球種のインデックス

	void InitializePitchSettings(); // 球種のパラメーターを初期化する関数


	//bool hasBeenJudged = false; // 判定済みフラグ


	float throwCounter = 0.0f; // 投球カウンター
	bool hasReachedZero = false; // z = 0.0f に到達したかどうか

	bool isRightPitcher = true; // 右投げかどうか
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

};