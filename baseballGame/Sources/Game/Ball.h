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
	void Render(const RenderContext& rc, ModelRenderer* renderer, bool isThrown);
	void DrawGUI();

	void AttachToHand(const std::vector<gltf_model::node>& animatedNodes, const DirectX::XMFLOAT4X4& ownerTransform, const char* handName);
	void UpdateFromPhysics(float elapsedTime, const DirectX::XMFLOAT3& rotationSpeed);
	void UpdateCollider();
	void ApplyPitchPhysics(bool isKnuckleball, const physx::PxVec3& windVelocity);
	void Throw(const physx::PxVec3& initialVelocity, const physx::PxVec3& angularVelocity);
	void ResetMotion();

	const DirectX::XMFLOAT3& GetBallPosition() const { return position; }
	const DirectX::XMFLOAT3& GetBallScale() const { return scale; }
	const DirectX::XMFLOAT3& GetBallAngle() const { return angle; }
	DirectX::XMFLOAT3& GetBallPosition() { return position; }
	DirectX::XMFLOAT3& GetBallScale() { return scale; }
	DirectX::XMFLOAT3& GetBallAngle() { return angle; }

	const DirectX::XMFLOAT3& GetWorldPosition() const { return worldPosition; }
	const DirectX::XMFLOAT3& GetWorldScale() const { return worldScale; }
	const DirectX::XMFLOAT3& GetWorldAngle() const { return worldAngle; }
	DirectX::XMFLOAT3& GetWorldPosition() { return worldPosition; }
	DirectX::XMFLOAT3& GetWorldScale() { return worldScale; }
	DirectX::XMFLOAT3& GetWorldAngle() { return worldAngle; }
	const DirectX::XMFLOAT4X4& GetWorldTransform() const { return worldTransform; }
	const DirectX::XMFLOAT4X4& GetHandTransform() const { return handTransform; }

	const DirectX::XMFLOAT3& GetVelocity() const { return velocity; }
	void SetVelocity(const DirectX::XMFLOAT3& newVelocity) { velocity = newVelocity; }
	float GetDebugRadius() const { return debugRadius; }
	float GetReducedRadius() const { return reducedRadius; }
	const DirectX::XMFLOAT3& GetStartPosition() const { return startPosition; }

	physx::PxRigidDynamic* GetBallCollider() const { return collider; }

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

private:
	// ボールの軌跡保存用
	std::deque<DirectX::XMFLOAT3> ballTrail;
	float MaxTrailLength = 50; // 軌跡の最大保存数
	const float TrailRecordInterval = 0.016f; // 記録間隔
	float trailRecordTimer = 0.0f;
	float trailWidth = 0.05f; // 軌跡の幅

public:
	bool hasCollided = false; // 衝突フラグ
	void SetHasCollided(bool collided) { hasCollided = collided; }
	bool GetHasCollided() const { return hasCollided; }

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

	bool foulLogged = false;
	bool GetFoulLogged() const { return foulLogged; }
	void SetFoulLogged(bool value) { foulLogged = value; }

	bool throughStrikeZone = false;
	bool GetThroughStrikeZone() const { return throughStrikeZone; }
	void SetThroughStrikeZone(bool value) { throughStrikeZone = value; }
};
