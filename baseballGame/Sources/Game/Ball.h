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
#include "json.hpp"
using json = nlohmann::json;

enum class BallMode
{
	Attached,// 手に持たれている状態
	BezierPitch,// 投球中（ベジェ曲線での投球）
	PhysicsHit,// 物理演算での衝突後の挙動
};


class Ball : public GameObject
{
public:
	static Ball& Instance()
	{
		static Ball instance;
		return instance;
	}

	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer, bool isThrown, bool renderTrail = true);
	void DrawGUI();

	void AttachToHand(const std::vector<gltf_model::node>& animatedNodes, const DirectX::XMFLOAT4X4& ownerTransform, const char* handName);
	void UpdateFromPhysics(float elapsedTime);
	void UpdateCollider();
	void ApplyPitchPhysics(bool isKnuckleball, const physx::PxVec3& windVelocity);
	void Throw(const physx::PxVec3& initialVelocity, const physx::PxVec3& angularVelocity, const DirectX::XMFLOAT3& visualRotationSpeed, const DirectX::XMFLOAT3& visualAngle);
	void ResetMotion();

	void ApplyReplayFrame(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& angle);

	const DirectX::XMFLOAT3& GetBallPosition() const { return position; }
	const DirectX::XMFLOAT3& GetBallScale() const { return scale; }
	const DirectX::XMFLOAT3& GetBallAngle() const { return angle; }
	DirectX::XMFLOAT3& GetBallPosition() { return position; }
	DirectX::XMFLOAT3& GetBallScale() { return scale; }
	DirectX::XMFLOAT3& GetBallAngle() { return angle; }
	void SetBallPosition(const DirectX::XMFLOAT3& newPosition) { position = newPosition; }
	void SetBallScale(const DirectX::XMFLOAT3& newScale) { scale = newScale; }
	void SetBallAngle(const DirectX::XMFLOAT3& newAngle) { angle = newAngle; }

	const DirectX::XMFLOAT3& GetWorldPosition() const { return worldPosition; }
	const DirectX::XMFLOAT3& GetWorldScale() const { return worldScale; }
	const DirectX::XMFLOAT3& GetWorldAngle() const { return worldAngle; }
	DirectX::XMFLOAT3& GetWorldPosition() { return worldPosition; }
	DirectX::XMFLOAT3& GetWorldScale() { return worldScale; }
	DirectX::XMFLOAT3& GetWorldAngle() { return worldAngle; }
	const DirectX::XMFLOAT4X4& GetWorldTransform() const { return worldTransform; }
	const DirectX::XMFLOAT4X4& GetHandTransform() const { return handTransform; }
	void SetWorldPosition(const DirectX::XMFLOAT3& newPosition) { worldPosition = newPosition; }
	void SetWorldScale(const DirectX::XMFLOAT3& newScale) { worldScale = newScale; }
	void SetWorldAngle(const DirectX::XMFLOAT3& newAngle) { worldAngle = newAngle; }

	const DirectX::XMFLOAT3& GetVelocity() const { return velocity; }
	void SetVelocity(const DirectX::XMFLOAT3& newVelocity) { velocity = newVelocity; }
	float GetDebugRadius() const { return debugRadius; }
	float GetReducedRadius() const { return reducedRadius; }
	const DirectX::XMFLOAT3& GetStartPosition() const { return startPosition; }

	physx::PxRigidDynamic* GetBallCollider() const { return collider; }

	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

private:
	void UpdateWorldTransform();
	void SyncColliderToWorldPosition();

private:
	std::unique_ptr<gltf_model> model;
	DirectX::XMFLOAT4X4 handTransform = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	DirectX::XMFLOAT4X4 worldTransform = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	DirectX::XMFLOAT3 worldPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 worldAngle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 worldScale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 startPosition = { 0.0f, 0.0f, 0.0f };
	float debugRadius = 0.037f;
	float reducedRadius = 0.0f;

	physx::PxRigidDynamic* collider = nullptr;
	physx::PxMaterial* material = nullptr;

	// モデルの回転管理
	DirectX::XMFLOAT3 modelAngle = { 0.0f, 0.0f, 0.0f };       // モデル独自の累積回転角
	DirectX::XMFLOAT3 modelRotationSpeed = { 0.0f, 0.0f, 0.0f }; // deg/sec、Throw時に設定



private:
	// ボールの軌跡保存用
	std::deque<DirectX::XMFLOAT3> ballTrail;
	float MaxTrailLength = 50; // 軌跡の最大保存数
	const float TrailRecordInterval = 0.016f; // 記録間隔
	float trailRecordTimer = 0.0f;
	float trailWidth = 0.05f; // 軌跡の幅
	float trailRecordDelayTime = 0.0f; // 軌跡の記録開始までの遅延時間

public:
	
	bool hasCollidedWithBat = false;
	bool GetHasCollidedWithBat() const { return hasCollidedWithBat; }
	void SetHasCollidedWithBat(bool value) { hasCollidedWithBat = value; }

	// フェンスとの衝突フラグ
	bool hasCollidedWithFence = false;
	void SetHasCollidedWithFence(bool collided) { hasCollidedWithFence = collided; }
	bool GetHasCollidedWithFence() const { return hasCollidedWithFence; }

	//グラウンドとの衝突フラグ
	bool hasCollidedWithGround = false;
	void SetHasCollidedWithGround(bool collided) { hasCollidedWithGround = collided; }
	bool GetHasCollidedWithGround() const { return hasCollidedWithGround; }

	//ポールとの衝突
	bool hasCollidedWithPole = false;
	void SetHasCollidedWithPole(bool collided) { hasCollidedWithPole = collided; }
	bool GetHasCollidedWithPole() const { return hasCollidedWithPole; }

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

	bool foulLogged = false;
	bool GetFoulLogged() const { return foulLogged; }
	void SetFoulLogged(bool value) { foulLogged = value; }

	bool isFoulConfirmed = false;
	bool GetIsFoulConfirmed() const { return isFoulConfirmed; }
	void SetIsFoulConfirmed(bool value) { isFoulConfirmed = value; }

	bool hasCollidedWithNet = false;
	bool GetHasCollidedWithNet() const { return hasCollidedWithNet; }
	void SetHasCollidedWithNet(bool value) { hasCollidedWithNet = value; }

	void SetModelRotationSpeed(const DirectX::XMFLOAT3& speed) { modelRotationSpeed = speed; }
	const DirectX::XMFLOAT3& GetModelRotationSpeed() const { return modelRotationSpeed; }
	const DirectX::XMFLOAT3& GetModelAngle() const { return modelAngle; }

	void SetModelAngle(const DirectX::XMFLOAT3& angle) { modelAngle = angle; }

public:
	// 物理コライダーから現在の正確な速度ベクトル(m/s)を取得する関数
	physx::PxVec3 GetLinearVelocity() const {
		return collider ? collider->getLinearVelocity() : physx::PxVec3(0.0f, 0.0f, 0.0f);
	}

	//ベジェ曲線のターゲット位置を設定する関数
	void SetBezierTargetPosition(const DirectX::XMFLOAT3& targetPosition);

	DirectX::XMFLOAT4 GetRotationQuat() const { return rotationQuat; }
	void SetRotationQuat(const DirectX::XMFLOAT4& quat) { rotationQuat = quat; }

private:

	BallMode ballMode = BallMode::Attached;
	float pitchTimer = 0.0f;// 投球中のタイマー
	float pitchDuration = 1.0f;// 投球中の時間

	DirectX::XMFLOAT3 bezierP0;// ベジェ曲線の始点
	DirectX::XMFLOAT3 bezierP1;// ベジェ曲線の制御点1
	DirectX::XMFLOAT3 bezierP2;// ベジェ曲線の制御点2
	DirectX::XMFLOAT3 bezierP3;// ベジェ曲線の終点

	DirectX::XMFLOAT3 previousBezierPos;// 前回のベジェ曲線上の位置

	DirectX::XMFLOAT4 rotationQuat = { 0.0f, 0.0f, 0.0f, 1.0f };// 回転クォータニオン

public:

	// ===== ベジェ曲線投球 =====
	struct BezierPitchData
	{
		DirectX::XMFLOAT3 p0;   // スタート（手）
		DirectX::XMFLOAT3 p1;   // 制御点1（変化球の"曲がり"を作る）
		DirectX::XMFLOAT3 p2;   // 制御点2
		DirectX::XMFLOAT3 p3;   // エンド（ホームプレート付近）
		float durationSec;       // 到達時間（球速から計算）
	};

	void ThrowBezier(const BezierPitchData& data,
		const DirectX::XMFLOAT3& visualRotationSpeed,
		const DirectX::XMFLOAT3& visualAngle);
	void UpdateBezierFlight(float elapsedTime);  // 毎フレーム呼ぶ

	bool  IsBezierFlying()  const { return bezierFlying; }
	float GetBezierRemainingTime() const
	{
		if (!bezierFlying) return -1.0f;// 飛行中でない場合は-1を返す
		return bezierData.durationSec * (1.0f - bezierT);
	}
	void  CancelBezier();   // バット衝突時に呼ぶ

	// ===== 2Dスプライト側で同じ曲線を再現するための公開API =====
	float GetBezierT() const { return bezierT; }
	const BezierPitchData& GetBezierData() const { return bezierData; }
	// EvalCubicBezierはprivateのまま、外部からはこのラッパー経由で呼ぶ
	DirectX::XMFLOAT3 GetBezierPositionAt(float t) const { return EvalCubicBezier(t); }

	DirectX::XMFLOAT3 GetBezierP3() const { return bezierData.p3; } // 終点を取得するための関数

private:
	BezierPitchData bezierData = {};
	float           bezierT = 0.0f;   // 0→1の進行度
	bool            bezierFlying = false;

	// ヘルパー
	DirectX::XMFLOAT3 EvalCubicBezier(float t) const;

	// ヘルパー: ベジェ曲線の微分を評価する関数
	DirectX::XMFLOAT3 EvalCubicBezierDerivative(float t) const;

};