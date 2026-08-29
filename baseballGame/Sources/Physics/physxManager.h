#pragma once

#include <vector>
#include <DirectXMath.h>
#include <PxPhysicsAPI.h>
#include <string>
#include "Effect.h"

class Physics 
	:public physx::PxSimulationEventCallback
	
{
private:
	Physics() = default;
	~Physics() = default;

public:
	//インスタンス
	static Physics& Instance()
	{
		static Physics instance;
		return instance;
	}

	//初期化
	void Initialize();

	//終了化
	void Finalize();

	//更新処理
	void Update(float elapsedTime);

	//描画処理
	void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection, const DirectX::XMFLOAT3& lightDirection);

	//描画オプション設定
	void SetRenderSimpleShapesOnly(bool enable) { renderSimpleShapesOnly = enable; }
	void SetSkipSleepingActors(bool enable) { skipSleepingActors = enable; }
	bool GetRenderSimpleShapesOnly() const { return renderSimpleShapesOnly; }
	bool GetSkipSleepingActors() const { return skipSleepingActors; }

	//Physics取得
	physx::PxPhysics* GetPhysics() { return pxPhysics; }

	//シーン取得
	physx::PxScene* GetScene() { return pxScene; }

	//コントローラーマネージャー取得
	physx::PxControllerManager* GetControllerManager() { return pxControllerManager; }

	//マテリアル取得
	physx::PxMaterial* GetMaterial() { return pxMaterial; }

protected:
	

	//衝突イベントインターフェース関数
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override {};
	void onWake(physx::PxActor** actors, physx::PxU32 count) override {};
	void onSleep(physx::PxActor** actors, physx::PxU32 count) override {};
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override {};

private:

	//衝突検出フィルタリング
	static physx::PxFilterFlags SimulationFilterShader(
		physx::PxFilterObjectAttributes	attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes	attributes1, physx::PxFilterData	filterData1,
		physx::PxPairFlags& pairFlags,
		const void* constantBlock, physx::PxU32 constantBlockSize);

private:

	physx::PxDefaultAllocator			pxAllocator;
	physx::PxDefaultErrorCallback		pxErrorCallback;
	physx::PxFoundation* pxFoundation = nullptr;
	physx::PxPhysics* pxPhysics = nullptr;
	physx::PxDefaultCpuDispatcher* pxDispatcher = nullptr;
	physx::PxScene* pxScene = nullptr;
	physx::PxControllerManager* pxControllerManager = nullptr;

	physx::PxMaterial* pxMaterial = nullptr;

	physx::PxPvd* pxPvd = nullptr;

	struct Line
	{
		DirectX::XMFLOAT3	start;
		DirectX::XMFLOAT3	end;
		DirectX::XMFLOAT4	color;
	};

	struct Capsule
	{
		DirectX::XMFLOAT4X4	transform;
		float				radius;
		float				height;
		DirectX::XMFLOAT4	color;
	};
	std::vector<Line>		lines;
	std::vector<Capsule>	capsules;

	bool renderSimpleShapesOnly = false;
	bool skipSleepingActors = false;

public:
	//コンソールログへのポインタ
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;

public:
	bool ballWasHit = false;
	bool GetBallWasHit() const { return ballWasHit; }
	void ClearBallWasHit() { ballWasHit = false; }

	//打球速度・打球角度・打球方向を取得するゲッター
	float GetBallSpeed() const { return outSpeed; }//打球速度
	float GetBallAngle() const { return outAngle; }//打球角度
	float GetBallDirection() const { return outDirection; }//打球方向
	float GetBallOriginalDirection() const { return outOriginalDirection; }//打球方向（元の方向）
	void SetBallOriginalDirection(float value) { outOriginalDirection = value; }//打球方向（元の方向）を設定するセッター
	float GetBallHorizontalDistance() const { return ballHorizontalDistance; }//打球の水平距離
	float GetBallTotalDistance() const { return ballTotalDistance; }//打球の総距離

	float outSpeed = 0.0f; //打球速度
	float outAngle = 0.0f; //打球角度
	float outDirection = 0.0f; //打球方向
	float outOriginalDirection = 0.0f; //打球方向（元の方向）
	float ballHorizontalDistance = 0.0f; //打球の水平距離
	float ballTotalDistance = 0.0f; //打球の総距離

	//確信ホームランかどうかを判定する変数とゲッター
	bool isHomeRun = false;
	bool GetIsHomeRun() const { return isHomeRun; }
	void SetIsHomeRun(bool value) { isHomeRun = value; }

	bool lastDistanceWasTotal = false; //前回の距離が総距離だったかどうかを判定する変数
	bool GetLastDistanceWasTotal() const { return lastDistanceWasTotal; }

private:
	std::unique_ptr<Effect> hitEffect;//打球エフェクトのインスタンス
	std::unique_ptr<Effect> hitSmallEffect;//打球小エフェクトのインスタンス
	std::unique_ptr<Effect> hitBigEffect;//打球大エフェクトのインスタンス
};
