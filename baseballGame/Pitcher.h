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
#include <deque>
#include "sprite.h"
#include "Ball.h"

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
		const DirectX::XMFLOAT3& GetBallPosition() const { return Ball::Instance().GetWorldPosition(); }
		const DirectX::XMFLOAT3& GetBallScale() const { return Ball::Instance().GetWorldScale(); }
		const DirectX::XMFLOAT3& GetBallAngle() const { return Ball::Instance().GetWorldAngle(); }

		const DirectX::XMFLOAT3& GetBallVelocity() const { return Ball::Instance().GetVelocity(); }
		void SetBallVelocity(const DirectX::XMFLOAT3& velocity) { Ball::Instance().SetVelocity(velocity); }
		const float GetBallDebugRadius() const { return Ball::Instance().GetDebugRadius(); }
		const float GetReducedRadius() const { return Ball::Instance().GetReducedRadius(); }

		bool IsBallInStrikeZone() const;

		void SetTheoreticalDistance(float distance) { theoreticalDistance = distance; }
		float GetTheoreticalDistance() const { return theoreticalDistance; }
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
		};

		PitchType selectedPitchType;

		//ストライクゾーンの判定
		DirectX::XMFLOAT3 strikeZonePosition = { 0.0f, 0.8f, 0.0f }; // ストライクゾーンの中心位置
		DirectX::XMFLOAT3 strikeZoneSize = { 0.2f, 0.3f, 0.001f }; // ストライクゾーンのサイズ（幅、高さ、奥行き）
		DirectX::XMFLOAT4 strikeZoneColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // ストライクゾーンの色（透明度付き）
		//bool hasBeenJudged = false; // 判定済みフラグ


		float throwCounter = 0.0f; // 投球カウンター
		bool hasReachedZero = false; // z = 0.0f に到達したかどうか

		float theoreticalDistance = 0.0f; // 理論上の飛距離（追加）

private:

	
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
		
		bool hasCollided = false; // 衝突フラグ

		void SetHasCollided(bool collided) { hasCollided = collided; }
		bool GetHasCollided() const { return hasCollided; }

		const State GetCurrentState() const { return currentState; }

		// フェンスとの衝突フラグ
		bool hasCollidedWithFence = false;
		void SetHasCollidedWithFence(bool collided) { hasCollidedWithFence = collided; }
		bool GetHasCollidedWithFence() const { return hasCollidedWithFence; }

		//グラウンドとの衝突フラグ
		bool hasCollidedWithGround = false;
		void SetHasCollidedWithGround(bool collided) { hasCollidedWithGround = collided; }
		bool GetHasCollidedWithGround() const { return hasCollidedWithGround; }

		// バット衝突時の位置を記録
		DirectX::XMFLOAT3 ballHitPosition = { 0.0f, 0.0f, 0.0f };
		void SetBallHitPosition(const DirectX::XMFLOAT3& pos) { ballHitPosition = pos; }
		const DirectX::XMFLOAT3& GetBallHitPosition() const { return ballHitPosition; }

		bool m_hasPassedHomeRunZone = false;
		bool GetHasPassedHomeRunZone() const { return m_hasPassedHomeRunZone; }
		void SetHasPassedHomeRunZone(bool value) { m_hasPassedHomeRunZone = value; }

		bool m_hasPassedFairFoulTrigger = false;
		bool GetHasPassedFairFoulTrigger() const { return m_hasPassedFairFoulTrigger; }
		void SetHasPassedFairFoulTrigger(bool value) { m_hasPassedFairFoulTrigger = value; }

		bool hasBeenJudged = false;
		bool GetHasBeenJudged() const { return hasBeenJudged; }
		void SetHasBeenJudged(bool value) { hasBeenJudged = value; }

		private:
			// ===== 新規追加 =====
			physx::PxVec3 GetSpinAxisFromPitchType() const;



};

