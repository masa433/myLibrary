#pragma once
#include "External/PhysX-main/PhysX-main/physx/include/PxPhysicsAPI.h"


class PhysXManager
{
public:
    static PhysXManager& Instance();

    void Initialize();
    void Uninitialize();
    void Update(float elapsedTime);

    // 物理オブジェクトをシーンに追加
    void AddActor(physx::PxActor* actor);

    physx::PxPhysics* GetPhysics() const { return mPhysics; }
    physx::PxMaterial* GetDefaultMaterial() const { return mDefaultMaterial; }
    physx::PxScene* GetScene() const { return mScene; }

private:
    PhysXManager() = default;
    ~PhysXManager() = default;

    // コピー禁止
    PhysXManager(const PhysXManager&) = delete;
    PhysXManager& operator=(const PhysXManager&) = delete;

    // PhysX関連のメンバ変数
    physx::PxFoundation* mFoundation = nullptr;
    physx::PxPhysics* mPhysics = nullptr;
    physx::PxScene* mScene = nullptr;
    physx::PxDefaultCpuDispatcher* mDispatcher = nullptr;
    physx::PxMaterial* mDefaultMaterial = nullptr;
    physx::PxDefaultAllocator mAllocator;
    physx::PxDefaultErrorCallback mErrorCallback;
};

class CollisionCallback : public physx::PxSimulationEventCallback
{
public:
    void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
    void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override {}
    void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override {}
    void onWake(physx::PxActor** actors, physx::PxU32 count) override {}
    void onSleep(physx::PxActor** actors, physx::PxU32 count) override {}
    void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override {}
};