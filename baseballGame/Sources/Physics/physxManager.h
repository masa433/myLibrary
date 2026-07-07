#pragma once

#include <vector>
#include <DirectXMath.h>
#include <PxPhysicsAPI.h>
#include <string>


// フィジクス
class Physics 
	:public physx::PxSimulationEventCallback
	
{
private:
	Physics() = default;
	~Physics() = default;

public:
	// インスタンス取得
	static Physics& Instance()
	{
		static Physics instance;
		return instance;
	}

	// 初期化
	void Initialize();

	// 終了化
	void Finalize();

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection, const DirectX::XMFLOAT3& lightDirection);

	// 描画オプション設定
	void SetRenderSimpleShapesOnly(bool enable) { renderSimpleShapesOnly = enable; }
	void SetSkipSleepingActors(bool enable) { skipSleepingActors = enable; }
	bool GetRenderSimpleShapesOnly() const { return renderSimpleShapesOnly; }
	bool GetSkipSleepingActors() const { return skipSleepingActors; }

	// フィジクス取得
	physx::PxPhysics* GetPhysics() { return pxPhysics; }

	// シーン取得
	physx::PxScene* GetScene() { return pxScene; }

	// コントローラーマネージャー取得
	physx::PxControllerManager* GetControllerManager() { return pxControllerManager; }

	// マテリアル取得
	physx::PxMaterial* GetMaterial() { return pxMaterial; }

	bool IsBoxCollider(physx::PxActor* actor);

protected:
	//--------------------------
	// NOTE:③フィルタリングインターフェース関数
	//--------------------------
	//physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override;
	//physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit, const physx::PxShape* shape, const physx::PxRigidActor* actor) override;

	//--------------------------
	// NOTE:⑦衝突イベントインターフェース関数
	//--------------------------
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override {};
	void onWake(physx::PxActor** actors, physx::PxU32 count) override {};
	void onSleep(physx::PxActor** actors, physx::PxU32 count) override {};
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override {};

private:
	//--------------------------
	// NOTE:⑧衝突検出フィルタリング
	//--------------------------
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
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;

public:
	bool ballWasHit = false;
	bool GetBallWasHit() const { return ballWasHit; }
	void ClearBallWasHit() { ballWasHit = false; }
};
