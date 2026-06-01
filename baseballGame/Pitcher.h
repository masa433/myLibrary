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

		// 風の影響を受けるエリアにボールが入っているか
		bool IsBallInWindArea() const;

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
		DirectX::XMFLOAT3 strikeZonePosition = { 0.0f, 0.8f, 0.0f }; // ストライクゾーンの中心位置
		DirectX::XMFLOAT3 strikeZoneSize = { 0.2f, 0.3f, 0.001f }; // ストライクゾーンのサイズ（幅、高さ、奥行き）
		DirectX::XMFLOAT4 strikeZoneColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // ストライクゾーンの色（透明度付き）
		bool hasBeenJudged = false; // 判定済みフラグ

		physx::PxRigidDynamic* ballCollider = nullptr; // ボールのコライダー
		physx::PxMaterial* pxBallMaterial = nullptr;//ボール専用のマテリアル

		float throwCounter = 0.0f; // 投球カウンター
		bool hasReachedZero = false; // z = 0.0f に到達したかどうか

		float theoreticalDistance = 0.0f; // 理論上の飛距離（追加）

private:

	// ボールの軌跡保存用
	std::deque<DirectX::XMFLOAT3> ballTrail;
	float MaxTrailLength = 50; // 軌跡の最大保存数
	const float TrailRecordInterval = 0.016f; // 記録間隔
	float trailRecordTimer = 0.0f;
	float trailWidth = 0.05f; // 軌跡の幅
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
		bool GetHasCollided() const { return hasCollided; }

		const State GetCurrentState() const { return currentState; }

		// フェンスとの衝突フラグ
		bool hasCollidedWithFence = false;
		void SetHasCollidedWithFence(bool collided) { hasCollidedWithFence = collided; }
		bool GetHasCollidedWithFence() const { return hasCollidedWithFence; }

		// バット衝突時の位置を記録
		DirectX::XMFLOAT3 ballHitPosition = { 0.0f, 0.0f, 0.0f };
		void SetBallHitPosition(const DirectX::XMFLOAT3& pos) { ballHitPosition = pos; }
		const DirectX::XMFLOAT3& GetBallHitPosition() const { return ballHitPosition; }

		private:
			// ===== 新規追加 =====
			physx::PxVec3 GetSpinAxisFromPitchType() const;

private:
	//風表現用
	struct WindLine
	{
		DirectX::XMFLOAT3 position;
		float baseYOffset; // 厚みの範囲に対する相対的な高さ割合
		float speed;// 風の線の移動速度
		float length;// 風の線の長さ
		float phase;// 風の線の位相（時間経過で変化させるための変数）
	};

	std::vector<WindLine> windLines;
	DirectX::XMFLOAT3 windDirection{ -1.0f, 0.0f, 0.2f };
	float windStrength = 5.0f;
	float windHeight = 0.0f; // 風の位置（時間経過で変化させるための変数）
	float windThickness = 6.0f;// 風の線の厚み

public:
	//スプライト関連
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};
	std::unique_ptr<Sprite> windDirectionSprite;
	std::unique_ptr<sprite> windDirectionSpriteRenderer;

	std::unique_ptr<sprite> windStrengthFontRenderer;
};

