#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "gltf_model.h"
#include "game_object.h"
#include "RenderContext.h"
#include "physxManager.h"
#include "ModelRenderer.h"

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

public:
		const DirectX::XMFLOAT3& GetBallPosition() const { return ballWorldPosition; }
		const DirectX::XMFLOAT3& GetBallScale() const { return ballWorldScale; }
		const DirectX::XMFLOAT3& GetBallAngle() const { return ballWorldAngle; }

		const DirectX::XMFLOAT3& GetBallVelocity() const { return ballVelocity; }
		void SetBallVelocity(const DirectX::XMFLOAT3& velocity) { ballVelocity = velocity; }
		const float GetBallDebugRadius() const { return ballDebugRadius; }
		const float GetReducedRadius() const { return reducedRadius; }

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

		//ボール関連
		std::unique_ptr<gltf_model> ball;
		DirectX::XMFLOAT4X4 ballTransform = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
		DirectX::XMFLOAT3 ballPosition = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT3 ballScale = { 1.0f,1.0f,1.0f };
		DirectX::XMFLOAT3 ballAngle = { 0.0f,0.0f,0.0f };

		// ボール専用のトランスフォーム情報
		DirectX::XMFLOAT3 ballWorldPosition = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 ballWorldAngle = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 ballWorldScale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4X4 ballWorldTransform = {
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			0,0,0,1
		};

		DirectX::XMFLOAT3 ballVelocity = { 0.0f, 0.0f, 0.0f }; // ボールの速度
		DirectX::XMFLOAT3 ballStartPosition = { 0.0f, 0.0f, 0.0f }; // 投球開始位置（追加）
		float throwTiming = 0.4f; // ボールを離すタイミング（アニメーション時間の比率）
		float gravity = -9.8f; // 重力加速度
		bool isBallThrown = false; // ボールが投げられたか
		// ボール投球制御
		float ballSpeedKmh = 150.0f; // 投球速度（km/h） - デバッグ可能
		float launchAngleDegrees = -2.5f; // 発射角度（度）
		DirectX::XMFLOAT3 rotationSpeed = { 0.0f, 0.0f, 0.0f }; // 回転速度（度/秒）
		DirectX::XMFLOAT3 throwDirection = { 0.02f, 0.2f, -1.0f }; // 投球方向

		// 変化球パラメータ
		float horizontalBreak = 0.0f; // 横方向の変化量（正:右、負:左）
		float verticalBreak = 0.0f;   // 縦方向の変化量（正:上、負:下）
		float breakStartDistance = 0.0f; // 変化が始まる距離

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
		};

		PitchType selectedPitchType;

		//ストライクゾーンの判定
		DirectX::XMFLOAT3 strikeZonePosition = { 0.0f, 2.55f, 43.0f }; // ストライクゾーンの中心位置
		DirectX::XMFLOAT3 strikeZoneSize = { 0.6f, 0.8f, 0.001f }; // ストライクゾーンのサイズ（幅、高さ、奥行き）
		DirectX::XMFLOAT4 strikeZoneColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // ストライクゾーンの色（透明度付き）
		bool hasBeenJudged = false; // 判定済みフラグ

		physx::PxRigidDynamic* ballCollider = nullptr; // ボールのコライダー
		physx::PxMaterial* pxBallMaterial = nullptr;//ボール専用のマテリアル

		float throwCounter = 0.0f; // 投球カウンター
		bool hasReachedZero = false; // z = 0.0f に到達したかどうか

public:

	// 状態管理
	enum class State 
	{
		SelectingPitch,// 球種選択中
		Throwing,// 投球中

	};

	State currentState = State::SelectingPitch;
	float stateTime = 0.0f; // 現在の状態に入ってからの経過時間

	public:
		physx::PxRigidDynamic* GetBallCollider() const { return ballCollider; }

		bool hasCollided = false; // 衝突フラグ

		void SetHasCollided(bool collided) { hasCollided = collided; }

		const State GetCurrentState() const { return currentState; }
};

