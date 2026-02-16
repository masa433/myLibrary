#include "physxManager.h"
#include <stdexcept>
#include <Windows.h>

using namespace physx;

PhysXManager& PhysXManager::Instance()
{
    static PhysXManager instance;
    return instance;
}

void PhysXManager::Initialize()
{
    // Foundationの作成
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
    if (!mFoundation)
        throw std::runtime_error("PxCreateFoundation failed!");

    // Physicsの作成
    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale());
    if (!mPhysics)
        throw std::runtime_error("PxCreatePhysics failed!");

    // Sceneの作成
    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    mDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    mScene = mPhysics->createScene(sceneDesc);
    if (!mScene)
        throw std::runtime_error("createScene failed!");

    // デフォルトのマテリアルを作成
    mDefaultMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);
    if (!mDefaultMaterial)
        throw std::runtime_error("createMaterial failed!");

    // デバッグ可視化を有効化
    mScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 1.0f); // 全体のスケール
    mScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f); // コライダーの可視化
    mScene->setVisualizationParameter(physx::PxVisualizationParameter::eACTOR_AXES, 1.0f); // アクターの軸を可視化

    // 衝突コールバックを設定
    CollisionCallback* collisionCallback = new CollisionCallback();
    mScene->setSimulationEventCallback(collisionCallback);
}

void PhysXManager::Uninitialize()
{
    if (mScene) mScene->release();
    if (mDispatcher) mDispatcher->release();
    if (mDefaultMaterial) mDefaultMaterial->release();
    if (mPhysics) mPhysics->release();
    if (mFoundation) mFoundation->release();
}

void PhysXManager::Update(float elapsedTime)
{
    if (mScene)
    {
        mScene->simulate(elapsedTime);
        mScene->fetchResults(true);
    }
}

void PhysXManager::AddActor(physx::PxActor* actor)
{
    if (mScene && actor)
    {
        mScene->addActor(*actor);
    }
}

void CollisionCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
    OutputDebugStringA("onContact called\n");

    for (physx::PxU32 i = 0; i < nbPairs; i++)
    {
        const physx::PxContactPair& pair = pairs[i];

        if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            OutputDebugStringA("Collision detected\n");

            // 衝突したオブジェクトを取得
            physx::PxRigidDynamic* bat = pairHeader.actors[0]->is<physx::PxRigidDynamic>();
            physx::PxRigidDynamic* ball = pairHeader.actors[1]->is<physx::PxRigidDynamic>();

            if (bat && ball)
            {
                OutputDebugStringA("Bat and ball collision\n");
            }
        }
    }
}