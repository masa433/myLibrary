#include <algorithm>
#include "Misc.h"
#include "Graphics.h"
#include "physxManager.h"
#include "Pitcher.h"
#include <queue>
#include <mutex>
#include <functional>
#include "Player.h"
#include <random>
#include "stage.h"
#include "Wind.h"
#include "Ball.h"
#include "ballSprite.h"
#include "HomeRunCount.h"
#include "BallNet.h"
#include <ballCount.h>
#include "Money.h"
#include "Combo.h"
#include "SpecialAbility.h"
#define NET_COUNT 4

// グローバルまたはクラス内にキューを用意
std::queue<std::function<void()>> velocityUpdateQueue;
std::mutex queueMutex;

// 初期化
void Physics::Initialize()
{
	// 基盤生成
	{
		pxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, pxAllocator, pxErrorCallback);
		_ASSERT_EXPR(pxFoundation != nullptr, "Failed PxCreateFoundation");
	}

	// PVD
	{
		pxPvd = physx::PxCreatePvd(*pxFoundation);
		_ASSERT_EXPR(pxPvd != nullptr, "Failed PxCreatePvd");

		physx::PxPvdTransport* pxPvdTransport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		_ASSERT_EXPR(pxPvdTransport != nullptr, "Failed PxDefaultPvdSocketTransportCreate");

		pxPvd->connect(*pxPvdTransport, physx::PxPvdInstrumentationFlag::eALL);
	}

	// 物理システム生成
	{
		pxPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *pxFoundation, physx::PxTolerancesScale(), true, pxPvd);
		_ASSERT_EXPR(pxPhysics != nullptr, "Failed PxCreatePhysics");

		PxInitExtensions(*pxPhysics, pxPvd);
	}

	// ディスパッチャー生成
	{
		pxDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
		_ASSERT_EXPR(pxDispatcher != nullptr, "Failed PxDefaultCpuDispatcherCreate");
	}

	// シーン生成
	{
		physx::PxSceneDesc pxSceneDesc(pxPhysics->getTolerancesScale());
		pxSceneDesc.gravity = physx::PxVec3(gravity.x, gravity.y, gravity.z);
		pxSceneDesc.cpuDispatcher = pxDispatcher;
		pxSceneDesc.filterShader = SimulationFilterShader;
		pxSceneDesc.simulationEventCallback = this;

		//CCDを有効化
		pxSceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD;

		pxScene = pxPhysics->createScene(pxSceneDesc);
		_ASSERT_EXPR(pxScene != nullptr, "Failed pxPhysics->createScene");
	}

	// PVDシーンクライアント設定
	{
		physx::PxPvdSceneClient* pxPvdSceneClient = pxScene->getScenePvdClient();
		if (pxPvdSceneClient != nullptr)
		{
			pxPvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pxPvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pxPvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
	}

	// コントローラーマネージャー生成
	{
		pxControllerManager = PxCreateControllerManager(*pxScene);
		_ASSERT_EXPR(pxControllerManager != nullptr, "Failed PxCreateControllerManager");
		pxControllerManager->setDebugRenderingFlags(physx::PxControllerDebugRenderFlag::eALL);
	}

	// マテリアル生成
	{
		pxMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.6f);
		_ASSERT_EXPR(pxMaterial != nullptr, "Failed pxPhysics->createMaterial");
	}

	//エフェクト読み込み
	hitEffect = std::make_unique<Effect>("resources/effects/hit.efk");
	hitSmallEffect = std::make_unique<Effect>("resources/effects/hitSmall.efk");
	hitBigEffect = std::make_unique<Effect>("resources/effects/hitBig.efk");

	hitSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/Hit.wav");	
	homeRunSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/HomeRun.wav");
	hitClogSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/Clog.wav");
	longHitSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/LongHit.wav");
	foulWhistleSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/FoulWhistle.wav");
	foulSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/Foul.wav");
	boundSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/BallBound.wav");
	poleHitSound = Audio::Instance().LoadAudioSource("resources/sounds/SE/Pole.wav");
}

// 終了化
void Physics::Finalize()
{
	PxCloseExtensions();

	PX_RELEASE(pxControllerManager);
	PX_RELEASE(pxScene);
	PX_RELEASE(pxMaterial);
	PX_RELEASE(pxDispatcher);
	PX_RELEASE(pxPhysics);

	if (pxPvd != nullptr)
	{
		physx::PxPvdTransport* pxPvdTransport = pxPvd->getTransport();
		pxPvdTransport->disconnect();
		PX_RELEASE(pxPvd);
		PX_RELEASE(pxPvdTransport);
	}

	PX_RELEASE(pxFoundation);

	delete hitSound;
	hitSound = nullptr;
	delete homeRunSound;
	homeRunSound = nullptr;
	delete hitClogSound;
	hitClogSound = nullptr;
	delete longHitSound;
	longHitSound = nullptr;
	delete foulWhistleSound;
	foulWhistleSound = nullptr;
	delete boundSound;
	boundSound = nullptr;
	delete poleHitSound;
	poleHitSound = nullptr;
	delete foulSound;
	foulSound = nullptr;

	consoleLog = nullptr;
}

// 更新処理
void Physics::Update(float elapsedTime)
{
	pxScene->simulate(elapsedTime);
	pxScene->fetchResults(true);

	// キューを処理
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		while (!velocityUpdateQueue.empty())
		{
			velocityUpdateQueue.front()(); // リクエストを実行
			velocityUpdateQueue.pop();    // キューから削除
		}
	}

	foulWhistleSound->Update();
	boundSound->Update();
	foulSound->Update();
}

// 描画
void Physics::Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection, const DirectX::XMFLOAT3& lightDirection)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

	physx::PxShape* pxShapes[128];
	auto drawShape = [&](physx::PxShape* pxShape, const physx::PxTransform& pxShapeTransform, float contactOffset, bool sleeping)
		{
			const physx::PxGeometry& pxGeometry = pxShape->getGeometry();
			const physx::PxMat44 pxShapeMat(pxShapeTransform);
			DirectX::XMFLOAT4X4 shapeTransform = {
				pxShapeMat.column0.x, pxShapeMat.column0.y, pxShapeMat.column0.z, pxShapeMat.column0.w,
				pxShapeMat.column1.x, pxShapeMat.column1.y, pxShapeMat.column1.z, pxShapeMat.column1.w,
				pxShapeMat.column2.x, pxShapeMat.column2.y, pxShapeMat.column2.z, pxShapeMat.column2.w,
				pxShapeMat.column3.x, pxShapeMat.column3.y, pxShapeMat.column3.z, pxShapeMat.column3.w,
			};
			DirectX::XMFLOAT3 shapePosition = {
				pxShapeMat.column3.x, pxShapeMat.column3.y, pxShapeMat.column3.z
			};

			DirectX::XMFLOAT4 color(1.0f, 0.0f, 0.0f, 0.3f);
			if (sleeping)
			{
				const float dark = 0.25f;
				color.x *= dark;
				color.y *= dark;
				color.z *= dark;
			}

			switch (pxGeometry.getType())
			{
			case physx::PxGeometryType::eSPHERE:
			{
				const physx::PxSphereGeometry& pxSphereGeometry = static_cast<const physx::PxSphereGeometry&>(pxGeometry);
				shapeRenderer->DrawSphere(shapeTransform, pxSphereGeometry.radius, color);
				break;
			}
			case physx::PxGeometryType::ePLANE:
			{
				const physx::PxPlaneGeometry& pxPlaneGeometry = static_cast<const physx::PxPlaneGeometry&>(pxGeometry);
				break;
			}
			case physx::PxGeometryType::eCAPSULE:
			{
				const physx::PxCapsuleGeometry& pxCapsuleGeometry = static_cast<const physx::PxCapsuleGeometry&>(pxGeometry);
				DirectX::XMMATRIX ShapeTransform = DirectX::XMLoadFloat4x4(&shapeTransform);
				DirectX::XMMATRIX OffsetTransform = DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2);
				DirectX::XMStoreFloat4x4(&shapeTransform, OffsetTransform * ShapeTransform);
				shapeRenderer->DrawCapsule(shapeTransform, pxCapsuleGeometry.radius + contactOffset, pxCapsuleGeometry.halfHeight * 2.0f, color);
				break;
			}
			case physx::PxGeometryType::eBOX:
			{
				const physx::PxBoxGeometry& pxBoxGeometry = static_cast<const physx::PxBoxGeometry&>(pxGeometry);
				shapeRenderer->DrawBox(shapeTransform, DirectX::XMFLOAT3(pxBoxGeometry.halfExtents.x + contactOffset, pxBoxGeometry.halfExtents.y + contactOffset, pxBoxGeometry.halfExtents.z + contactOffset), color);
				break;
			}
			case physx::PxGeometryType::eCONVEXMESH:
			{
				// シンプルシェイプのみモードではスキップ
				if (renderSimpleShapesOnly)
					break;

				const physx::PxConvexMeshGeometry& pxConvexMeshGeometry = static_cast<const physx::PxConvexMeshGeometry&>(pxGeometry);

				const physx::PxConvexMesh& pxConvexMesh = *pxConvexMeshGeometry.convexMesh;
				const physx::PxVec3* pxVertices = pxConvexMesh.getVertices();
				const physx::PxU8* pxIndices = pxConvexMesh.getIndexBuffer();

				const physx::PxVec3 pxScale = pxConvexMeshGeometry.scale.scale;
				const physx::PxQuat pxRotation = pxConvexMeshGeometry.scale.rotation;
				DirectX::XMMATRIX Scale = DirectX::XMMatrixScaling(pxScale.x, pxScale.y, pxScale.z);
				DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(pxRotation.x, pxRotation.y, pxRotation.z, pxRotation.w));
				DirectX::XMMATRIX ShapeTransform = Scale * Rotation * DirectX::XMLoadFloat4x4(&shapeTransform);

				const physx::PxU32 pxNumPolygons = pxConvexMesh.getNbPolygons();
				for (physx::PxU32 pxPolygonIndex = 0; pxPolygonIndex < pxNumPolygons; ++pxPolygonIndex)
				{
					physx::PxHullPolygon pxHullPolygon;
					pxConvexMesh.getPolygonData(pxPolygonIndex, pxHullPolygon);

					const physx::PxU32 pxNumTriangles = pxHullPolygon.mNbVerts - 2;
					const physx::PxU8 pxIndex0 = pxIndices[pxHullPolygon.mIndexBase + 0];
					const physx::PxVec3& pxVertex0 = pxVertices[pxIndex0];

					for (physx::PxU32 pxTriangleIndex = 0; pxTriangleIndex < pxNumTriangles; ++pxTriangleIndex)
					{
						const physx::PxU8 pxIndex1 = pxIndices[pxHullPolygon.mIndexBase + 0 + pxTriangleIndex + 1];
						const physx::PxU8 pxIndex2 = pxIndices[pxHullPolygon.mIndexBase + 0 + pxTriangleIndex + 2];
						const physx::PxVec3& pxVertex1 = pxVertices[pxIndex1];
						const physx::PxVec3& pxVertex2 = pxVertices[pxIndex2];

						DirectX::XMVECTOR V0 = DirectX::XMVectorSet(pxVertex0.x, pxVertex0.y, pxVertex0.z, 0);
						DirectX::XMVECTOR V1 = DirectX::XMVectorSet(pxVertex1.x, pxVertex1.y, pxVertex1.z, 0);
						DirectX::XMVECTOR V2 = DirectX::XMVectorSet(pxVertex2.x, pxVertex2.y, pxVertex2.z, 0);
						V0 = DirectX::XMVector3Transform(V0, ShapeTransform);
						V1 = DirectX::XMVector3Transform(V1, ShapeTransform);
						V2 = DirectX::XMVector3Transform(V2, ShapeTransform);
						DirectX::XMFLOAT3 v0, v1, v2;
						DirectX::XMStoreFloat3(&v0, V0);
						DirectX::XMStoreFloat3(&v1, V1);
						DirectX::XMStoreFloat3(&v2, V2);

						primitiveRenderer->AddVertex(v0, color);
						primitiveRenderer->AddVertex(v1, color);
						primitiveRenderer->AddVertex(v1, color);
						primitiveRenderer->AddVertex(v2, color);
						primitiveRenderer->AddVertex(v2, color);
						primitiveRenderer->AddVertex(v0, color);
					}
				}
				break;
			}
			case physx::PxGeometryType::ePARTICLESYSTEM:
			{
				const physx::PxParticleSystemGeometry& pxParticleSystemGeometry = static_cast<const physx::PxParticleSystemGeometry&>(pxGeometry);
				break;
			}
			case physx::PxGeometryType::eTETRAHEDRONMESH:
			{
				// シンプルシェイプのみモードではスキップ
				if (renderSimpleShapesOnly)
					break;

				const physx::PxTetrahedronMeshGeometry& pxTetrahedronMeshGeometry = static_cast<const physx::PxTetrahedronMeshGeometry&>(pxGeometry);
				const physx::PxTetrahedronMesh& pxTetrahedronMesh = *pxTetrahedronMeshGeometry.tetrahedronMesh;
				const physx::PxVec3* pxVertices = pxTetrahedronMesh.getVertices();
				const void* pxIndices = pxTetrahedronMesh.getTetrahedrons();
				const physx::PxU32* pxIndices32 = static_cast<const physx::PxU32*>(pxIndices);
				const physx::PxU16* pxIndices16 = static_cast<const physx::PxU16*>(pxIndices);
				const physx::PxU32 pxHas16BitIndices = pxTetrahedronMesh.getTetrahedronMeshFlags() & physx::PxTetrahedronMeshFlag::e16_BIT_INDICES;

				DirectX::XMMATRIX ShapeTransform = DirectX::XMLoadFloat4x4(&shapeTransform);

				physx::PxU32 pxNumTetrahedrons = pxTetrahedronMesh.getNbTetrahedrons();
				for (physx::PxU32 pxTetrahedronIndex = 0; pxTetrahedronIndex < pxNumTetrahedrons; ++pxTetrahedronIndex)
				{
					physx::PxU32 pxIndex[4];
					if (pxHas16BitIndices)
					{
						pxIndex[0] = *pxIndices16++;
						pxIndex[1] = *pxIndices16++;
						pxIndex[2] = *pxIndices16++;
						pxIndex[3] = *pxIndices16++;
					}
					else
					{
						pxIndex[0] = *pxIndices32++;
						pxIndex[1] = *pxIndices32++;
						pxIndex[2] = *pxIndices32++;
						pxIndex[3] = *pxIndices32++;
					}

					const int tetFaces[4][3] = { {0,2,1}, {0,1,3}, {0,3,2}, {1,2,3} };
					for (physx::PxU32 i = 0; i < 4; ++i)
					{
						const physx::PxVec3& pxVertex0 = pxVertices[pxIndex[tetFaces[i][0]]];
						const physx::PxVec3& pxVertex1 = pxVertices[pxIndex[tetFaces[i][1]]];
						const physx::PxVec3& pxVertex2 = pxVertices[pxIndex[tetFaces[i][2]]];

						DirectX::XMVECTOR V0 = DirectX::XMVectorSet(pxVertex0.x, pxVertex0.y, pxVertex0.z, 0);
						DirectX::XMVECTOR V1 = DirectX::XMVectorSet(pxVertex1.x, pxVertex1.y, pxVertex1.z, 0);
						DirectX::XMVECTOR V2 = DirectX::XMVectorSet(pxVertex2.x, pxVertex2.y, pxVertex2.z, 0);
						V0 = DirectX::XMVector3Transform(V0, ShapeTransform);
						V1 = DirectX::XMVector3Transform(V1, ShapeTransform);
						V2 = DirectX::XMVector3Transform(V2, ShapeTransform);
						DirectX::XMFLOAT3 v0, v1, v2;
						DirectX::XMStoreFloat3(&v0, V0);
						DirectX::XMStoreFloat3(&v1, V1);
						DirectX::XMStoreFloat3(&v2, V2);

						primitiveRenderer->AddVertex(v0, color);
						primitiveRenderer->AddVertex(v1, color);
						primitiveRenderer->AddVertex(v1, color);
						primitiveRenderer->AddVertex(v2, color);
						primitiveRenderer->AddVertex(v2, color);
						primitiveRenderer->AddVertex(v0, color);
					}
				}
				break;
			}
			case physx::PxGeometryType::eTRIANGLEMESH:
			{
				// シンプルシェイプのみモードではスキップ
				if (renderSimpleShapesOnly)
					break;

				const physx::PxTriangleMeshGeometry& pxTriangleMeshGeometry = static_cast<const physx::PxTriangleMeshGeometry&>(pxGeometry);
				const physx::PxTriangleMesh& pxTriangleMesh = *pxTriangleMeshGeometry.triangleMesh;
				const physx::PxVec3* pxVertices = pxTriangleMesh.getVertices();
				const void* pxIndices = pxTriangleMesh.getTriangles();
				const physx::PxU32* pxIndices32 = static_cast<const physx::PxU32*>(pxIndices);
				const physx::PxU16* pxIndices16 = static_cast<const physx::PxU16*>(pxIndices);
				const physx::PxU32 pxHas16BitIndices = pxTriangleMesh.getTriangleMeshFlags() & physx::PxTriangleMeshFlag::e16_BIT_INDICES;

				const physx::PxVec3 pxScale = pxTriangleMeshGeometry.scale.scale;
				const physx::PxQuat pxRotation = pxTriangleMeshGeometry.scale.rotation;
				DirectX::XMMATRIX Scale = DirectX::XMMatrixScaling(pxScale.x, pxScale.y, pxScale.z);
				DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(pxRotation.x, pxRotation.y, pxRotation.z, pxRotation.w));
				DirectX::XMMATRIX ShapeTransform = Scale * Rotation * DirectX::XMLoadFloat4x4(&shapeTransform);

				physx::PxU32 pxNumTriangles = pxTriangleMeshGeometry.triangleMesh->getNbTriangles();
				for (physx::PxU32 pxTriangleIndex = 0; pxTriangleIndex < pxNumTriangles; ++pxTriangleIndex)
				{
					physx::PxU32 pxIndex0, pxIndex1, pxIndex2;
					if (pxHas16BitIndices)
					{
						pxIndex0 = *pxIndices16++;
						pxIndex1 = *pxIndices16++;
						pxIndex2 = *pxIndices16++;
					}
					else
					{
						pxIndex0 = *pxIndices32++;
						pxIndex1 = *pxIndices32++;
						pxIndex2 = *pxIndices32++;
					}
					const physx::PxVec3& pxVertex0 = pxVertices[pxIndex0];
					const physx::PxVec3& pxVertex1 = pxVertices[pxIndex1];
					const physx::PxVec3& pxVertex2 = pxVertices[pxIndex2];
					DirectX::XMVECTOR V0 = DirectX::XMVectorSet(pxVertex0.x, pxVertex0.y, pxVertex0.z, 0);
					DirectX::XMVECTOR V1 = DirectX::XMVectorSet(pxVertex1.x, pxVertex1.y, pxVertex1.z, 0);
					DirectX::XMVECTOR V2 = DirectX::XMVectorSet(pxVertex2.x, pxVertex2.y, pxVertex2.z, 0);
					V0 = DirectX::XMVector3Transform(V0, ShapeTransform);
					V1 = DirectX::XMVector3Transform(V1, ShapeTransform);
					V2 = DirectX::XMVector3Transform(V2, ShapeTransform);
					DirectX::XMFLOAT3 v0, v1, v2;
					DirectX::XMStoreFloat3(&v0, V0);
					DirectX::XMStoreFloat3(&v1, V1);
					DirectX::XMStoreFloat3(&v2, V2);

					primitiveRenderer->AddVertex(v0, color);
					primitiveRenderer->AddVertex(v1, color);
					primitiveRenderer->AddVertex(v1, color);
					primitiveRenderer->AddVertex(v2, color);
					primitiveRenderer->AddVertex(v2, color);
					primitiveRenderer->AddVertex(v0, color);
				}
				break;
			}
			case physx::PxGeometryType::eHEIGHTFIELD:
			{
				const physx::PxHeightFieldGeometry& pxHeightFieldGeometry = static_cast<const physx::PxHeightFieldGeometry&>(pxGeometry);
				break;
			}
			case physx::PxGeometryType::eHAIRSYSTEM:
			{
				const physx::PxHairSystemGeometry& pxHairSystemGeometry = static_cast<const physx::PxHairSystemGeometry&>(pxGeometry);
				break;
			}
			case physx::PxGeometryType::eCUSTOM:
			{
				const physx::PxCustomGeometry& pxCustomGeometry = static_cast<const physx::PxCustomGeometry&>(pxGeometry);
				break;
			}
			}
		};
	physx::PxShape* pxShpaes[128] = { nullptr };
	auto drawActor = [&](physx::PxRigidActor* pxActor, float contactOffset)
		{
			const physx::PxU32 pxNumShapes = pxActor->getNbShapes();
			PX_ASSERT(pxNumShapes <= _countof(pxShpaes));
			pxActor->getShapes(pxShapes, pxNumShapes);

			physx::PxRigidDynamic* pxDynamic = pxActor->is<physx::PxRigidDynamic>();
			bool sleeping = pxDynamic ? pxDynamic->isSleeping() : false;

			// スリープ中のアクターをスキップ
			if (skipSleepingActors && sleeping)
				return;

			for (physx::PxU32 pxShapeIndex = 0; pxShapeIndex < pxNumShapes; ++pxShapeIndex)
			{
				physx::PxShape* pxShape = pxShapes[pxShapeIndex];
				drawShape(pxShape, physx::PxShapeExt::getGlobalPose(*pxShape, *pxActor), contactOffset, sleeping);
			}
		};

	// アクター
	{
		physx::PxActorTypeFlags pxActorTypeFlags = physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC;
		physx::PxU32 pxNumActors = pxScene->getNbActors(pxActorTypeFlags);
		if (pxNumActors > 0)
		{
			std::vector<physx::PxRigidActor*> pxActors(pxNumActors);
			pxScene->getActors(pxActorTypeFlags, reinterpret_cast<physx::PxActor**>(pxActors.data()), pxNumActors);

			for (physx::PxU32 pxActorIndex = 0; pxActorIndex < pxNumActors; ++pxActorIndex)
			{
				physx::PxRigidActor* pxActor = pxActors.at(pxActorIndex);

				drawActor(pxActor, 0.0f);
			}
		}
	}
	// コントローラー
	{
		physx::PxU32 pxNumControllers = pxControllerManager->getNbControllers();
		if (pxNumControllers > 0)
		{
			for (physx::PxU32 pxControllerIndex = 0; pxControllerIndex < pxNumControllers; ++pxControllerIndex)
			{
				physx::PxController* pxController = pxControllerManager->getController(pxControllerIndex);
				physx::PxRigidActor* pxActor = pxController->getActor();
				drawActor(pxActor, 0.0f);
				drawActor(pxActor, pxController->getContactOffset());
			}
		}
	}
	//
	{
		physx::PxU32 pxNumArticulations = pxScene->getNbArticulations();
		if (pxNumArticulations > 0)
		{
			std::vector<physx::PxArticulationReducedCoordinate*> pxArticulations(pxNumArticulations);
			pxScene->getArticulations(reinterpret_cast<physx::PxArticulationReducedCoordinate**>(pxArticulations.data()), pxNumArticulations);

			for (physx::PxU32 pxNumArticulationIndex = 0; pxNumArticulationIndex < pxNumArticulations; ++pxNumArticulationIndex)
			{
				physx::PxArticulationReducedCoordinate* pxArticulation = pxArticulations.at(pxNumArticulationIndex);

				physx::PxU32 pxNumLinks = pxArticulation->getNbLinks();
				std::vector<physx::PxArticulationLink*> pxLinks(pxNumLinks);
				pxArticulation->getLinks(pxLinks.data(), pxNumLinks);

				bool sleeping = pxArticulation->isSleeping();
				for (physx::PxU32 pxLinkIndex = 0; pxLinkIndex < pxNumLinks; ++pxLinkIndex)
				{
					physx::PxArticulationLink* pxLink = pxLinks.at(pxLinkIndex);
					const physx::PxU32 pxNumShapes = pxLink->getNbShapes();
					PX_ASSERT(pxNumShapes <= _countof(pxShpaes));
					pxLink->getShapes(pxShapes, pxNumShapes);

					for (physx::PxU32 pxShapeIndex = 0; pxShapeIndex < pxNumShapes; ++pxShapeIndex)
					{
						physx::PxShape* pxShape = pxShapes[pxShapeIndex];
						physx::PxTransform pxShapeTransform = pxLink->getGlobalPose() * pxShape->getLocalPose();
						drawShape(pxShape, pxShapeTransform, 0.0f, sleeping);
					}
				}
			}
		}
	}

	// キャスト
	{
		for (const Line& line : lines)
		{
			primitiveRenderer->AddVertex(line.start, line.color);
			primitiveRenderer->AddVertex(line.end, line.color);
		}
		lines.clear();

		for (const Capsule& capsule : capsules)
		{
			shapeRenderer->DrawCapsule(capsule.transform, capsule.radius, capsule.height, capsule.color);
		}
		capsules.clear();
	}

	// 描画
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	shapeRenderer->Render(dc, view, projection, lightDirection);
	primitiveRenderer->Render(dc, view, projection, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}

//衝突検出フィルタリング
physx::PxFilterFlags Physics::SimulationFilterShader(
	physx::PxFilterObjectAttributes	attributes0, physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes	attributes1, physx::PxFilterData	filterData1,
	physx::PxPairFlags& pairFlags,
	const void* constantBlock, physx::PxU32 constantBlockSize)
{
	// ボックスコライダー（word0 = 1 << 1）との衝突を無効化
	if ((filterData0.word0 & (1 << 1)) || (filterData1.word0 & (1 << 1)))
	{
		return physx::PxFilterFlag::eSUPPRESS; // 衝突を無効化
	}

	if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
		return physx::PxFilterFlag::eDEFAULT;
	}

	pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
	pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND | physx::PxPairFlag::eNOTIFY_TOUCH_LOST | physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS | physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

	pairFlags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;

	return physx::PxFilterFlag::eDEFAULT;
}

void Physics::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	//OutputDebugStringA("onContact called\n"); // ログを追加

	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& pair = pairs[i];

		bool ballIsActor0 = (pairHeader.actors[0] == Ball::Instance().GetBallCollider());
		bool ballIsActor1 = (pairHeader.actors[1] == Ball::Instance().GetBallCollider());
		const char* otherName = ballIsActor0 ? pairHeader.actors[1]->getName()
			: ballIsActor1 ? pairHeader.actors[0]->getName() : nullptr;

		if ((ballIsActor0 || ballIsActor1) && otherName && strcmp(otherName, "Pole") == 0)
		{
			bool isNewTouch = pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;

			if (isNewTouch && Ball::Instance().GetHasCollidedWithBat())
			{
				if (poleHitSound) poleHitSound->PlayOneShot();
			}

			if (!Ball::Instance().GetHasCollidedWithFence() && !Ball::Instance().GetHasCollidedWithGround())
			{
				Ball::Instance().SetHasCollidedWithPole(true);
				Ball::Instance().SetHasCollidedWithFence(true);
				Ball::Instance().SetHasCollidedWithGround(true);
				Ball::Instance().SetIsFoulConfirmed(false); // 念のため明示的にファウルを打ち消す
				
				physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
				if (ballCollider)
				{
					physx::PxVec3 ballPosition = ballCollider->getGlobalPose().p;
					DirectX::XMFLOAT3 ballHitPos = Ball::Instance().GetBallHitPosition();

					float distanceX = ballPosition.x - ballHitPos.x;
					float distanceZ = ballPosition.z - ballHitPos.z;
					float horizontalDistance = sqrtf(distanceX * distanceX + distanceZ * distanceZ);

					physx::PxVec3 ballVelocity = ballCollider->getLinearVelocity();
					float exitVelocity = ballVelocity.magnitude();
					float estimatedDistance = 0.0f;

					if (exitVelocity > 0.1f)
					{
						float launchAngle = std::atan2(ballVelocity.y,
							sqrtf(ballVelocity.x * ballVelocity.x + ballVelocity.z * ballVelocity.z));

						float initialHeight = ballPosition.y - ballHitPos.y;
						float v_y = exitVelocity * sinf(launchAngle);
						float a = 0.5f * 9.81f;
						float b = -v_y;
						float c = -initialHeight;
						float discriminant = b * b - 4.0f * a * c;

						if (discriminant >= 0.0f)
						{
							float t = (-b + sqrtf(discriminant)) / (2.0f * a);
							if (t > 0.0f)
							{
								estimatedDistance = exitVelocity * cosf(launchAngle) * t;
							}
						}
					}

					float totalDistance = horizontalDistance + estimatedDistance;

					ballTotalDistance = totalDistance;
					lastDistanceWasTotal = true;
				}

				HomeRunCount::Instance().IncrementCount();

			
				OutputDebugStringA("ホームラン！：ポールに衝突");
				if (consoleLog)
					consoleLog->push_back(u8"[Hit] ホームラン！：ポールに衝突");

				
			}
			continue;// このペアはホームランとして処理済みなので以降の個別判定はスキップ
		}
	}

	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{

		const physx::PxContactPair& pair = pairs[i];

		// エンタイトルツーベース以外でホームラントリガーをダイレクトで通過した場合、
		// 以降どのオブジェクトに衝突しても無条件でホームランにする
		if (Ball::Instance().GetHasPassedHomeRunZone() && !Ball::Instance().GetHasCollidedWithFence())
		{
			bool ballIsActor0 = (pairHeader.actors[0] == Ball::Instance().GetBallCollider());
			bool ballIsActor1 = (pairHeader.actors[1] == Ball::Instance().GetBallCollider());

			if (ballIsActor0 || ballIsActor1)
			{
				bool isNewTouch = pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;

				if (isNewTouch && Ball::Instance().GetHasCollidedWithBat())
				{
					if (boundSound) boundSound->PlayOneShot();
				}


				Ball::Instance().SetHasCollidedWithFence(true);
				Ball::Instance().SetHasCollidedWithGround(true);
				

				const char* otherName = ballIsActor0 ?
					pairHeader.actors[1]->getName() : pairHeader.actors[0]->getName();

				physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
				if (ballCollider)
				{
					// ===== 飛距離計算 =====
					physx::PxVec3 ballPosition = ballCollider->getGlobalPose().p;
					DirectX::XMFLOAT3 ballHitPos = Ball::Instance().GetBallHitPosition();

					float distanceX = ballPosition.x - ballHitPos.x;
					float distanceZ = ballPosition.z - ballHitPos.z;
					float horizontalDistance = sqrtf(distanceX * distanceX + distanceZ * distanceZ);

					physx::PxVec3 ballVelocity = ballCollider->getLinearVelocity();
					float exitVelocity = ballVelocity.magnitude();
					float estimatedDistance = 0.0f;

					if (exitVelocity > 0.1f)
					{
						float launchAngle = std::atan2(ballVelocity.y,
							sqrtf(ballVelocity.x * ballVelocity.x + ballVelocity.z * ballVelocity.z));

						float initialHeight = ballHitPos.y;
						float v_y = exitVelocity * sinf(launchAngle);
						float a = 0.5f * 9.81f;
						float b = -v_y;
						float c = -initialHeight;
						float discriminant = b * b - 4.0f * a * c;

						if (discriminant >= 0.0f)
						{
							float t = (-b + sqrtf(discriminant)) / (2.0f * a);
							if (t > 0.0f)
							{
								estimatedDistance = exitVelocity * cosf(launchAngle) * t;
							}
						}
					}

					float totalDistance = horizontalDistance + estimatedDistance;

					char debugMessage[768];
					snprintf(debugMessage, sizeof(debugMessage),
						"=== ホームラン！（トリガー直接通過） ===\n"
						"衝突オブジェクト: %s\n"
						"水平飛距離（実測）: %.2f m\n"
						"推定飛距離（スタンドなしでグラウンド着地）: %.2f m\n"
						"総飛距離: %.2f m\n",
						otherName ? otherName : "不明",
						horizontalDistance,
						estimatedDistance,
						totalDistance);
					OutputDebugStringA(debugMessage);

					bool isGround = (otherName && strcmp(otherName, "Ground") == 0);

					if (consoleLog)
					{
						//グラウンドなら実測飛距離、スタンドなら総飛距離を表示

						char logBuf[512];
						snprintf(logBuf, sizeof(logBuf),
							u8"[Hit] ホームラン！ 飛距離: %.1f m",
							isGround ? horizontalDistance : totalDistance);
						consoleLog->push_back(logBuf);
					}

					//ホームランカウントを１増やす
					HomeRunCount::Instance().IncrementCount();
					
					//グラウンドなら実測飛距離、スタンドなら総飛距離を保存
					if(isGround)
					{
						ballHorizontalDistance = horizontalDistance;
						lastDistanceWasTotal = false;
					}
					else
					{
						ballTotalDistance = totalDistance;
						lastDistanceWasTotal = true;
					}
				}

				continue; // このペアはホームランとして処理済みなので以降の個別判定はスキップ
			}
		}


		// ボールとグラウンドの衝突を検知
		if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Ground") ||
			(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Ground"))
		{
			bool isNewTouch = pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;

			if (isNewTouch && Ball::Instance().GetHasCollidedWithBat())
			{
				if (boundSound) boundSound->PlayOneShot();
			}
			// キューに速度変更リクエストを追加
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				velocityUpdateQueue.push([]() {
					physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();

					//ブレーキを適用
					//ballCollider->setLinearDamping(0.8f); // 線形減衰を設定
					//ballCollider->setAngularDamping(3.0f); // 角減衰を設定

					physx::PxVec3 velocity = ballCollider->getLinearVelocity();

					// 速度の大きさをチェック
					float speed = velocity.magnitude();

					// 速度がある程度以上ある場合のみ減衰を適用
					if (speed > 1.0f)
					{
						// フェンス衝突後かどうかで減衰率を変更
						float dampingFactor;
						if (Ball::Instance().GetHasCollidedWithPole())
						{
							// ポール衝突後にグラウンドへ着地: 速度を強く減衰させる
							dampingFactor = 0.90f;
						}
						else if (Ball::Instance().GetHasCollidedWithFence())
						{
							// フェンス衝突後: 速度を大きく減速（1%に低下）
							dampingFactor = 0.97f;
						}
						else
						{
							// フェンス衝突なし: 通常の摩擦ベース減衰
							physx::PxMaterial* stageMaterial = Physics::Instance().GetMaterial();
							float friction = stageMaterial->getDynamicFriction();
							dampingFactor = 1.0f - (friction * 0.01f);
						}

						velocity *= dampingFactor;

						// 回転速度も同じように減衰
						physx::PxVec3 angularVelocity = ballCollider->getAngularVelocity();
						angularVelocity *= dampingFactor;

						ballCollider->setLinearVelocity(velocity);
						ballCollider->setAngularVelocity(angularVelocity);
					}
					else
					{
						// 速度が非常に小さくなったら完全に停止
						ballCollider->setLinearVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
						ballCollider->setAngularVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
					}
					});


				//飛距離計算
				physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
				if (ballCollider && !Ball::Instance().GetHasCollidedWithGround())
				{
					Ball::Instance().SetHasCollidedWithGround(true); // 地面衝突フラグを設定
					Ball::Instance().SetHasBeenJudged(true);

					physx::PxVec3 ballPosition = ballCollider->getGlobalPose().p;
					DirectX::XMFLOAT3 ballHitPos = Ball::Instance().GetBallHitPosition();

					float distanceX = ballPosition.x - ballHitPos.x;
					float distanceZ = ballPosition.z - ballHitPos.z;
					float horizontalDistance = sqrtf(distanceX * distanceX + distanceZ * distanceZ);
					// フェア/ファウル判定
					constexpr float kFoulLineTolerance = 0.05f; // 5cm程度の許容誤差
					bool isFair = (ballPosition.z >= 0.0f) &&
						(std::fabs(ballPosition.x) <= ballPosition.z + kFoulLineTolerance);
					if (!isFair)
					{
						Ball::Instance().SetIsFoulConfirmed(true); // ファウル確定フラグを設定
						if(foulSound && Ball::Instance().GetHasCollidedWithBat() && !isFoulSoundPlayed)
						{
							foulSound->PlayOneShot();
							isFoulSoundPlayed = true;
						}
					}

					if (consoleLog)
					{

						char logBuf[512];
						snprintf(logBuf, sizeof(logBuf),
							u8"[Hit] ボールが地面に着地！ 判定: %s 飛距離: %.1f m",
							isFair ? u8"フェア" : u8"ファウル",
							horizontalDistance);
						consoleLog->push_back(logBuf);
					}

					ballHorizontalDistance = horizontalDistance;
					lastDistanceWasTotal = false; // グラウンド着地時は実測飛距離として扱う

					if (!Ball::Instance().GetHasPassedHomeRunZone() && Ball::Instance().GetHasCollidedWithBat())
					{
						Combo::Instance().ResetCombo(); // グラウンドに着地したらコンボをリセット
					}
				}
				
			}
		}

		//ボールとスタンドの衝突を検知
		if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Stand") ||
			(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Stand"))
		{
			bool isNewTouch = pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			if (isNewTouch && Ball::Instance().GetHasCollidedWithBat())
			{
				if (boundSound) boundSound->PlayOneShot();
			}

			{
				std::lock_guard<std::mutex> lock(queueMutex);
				velocityUpdateQueue.push([]() {
					physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
					if (!ballCollider) return;

					physx::PxVec3 velocity = ballCollider->getLinearVelocity();
					float speed = velocity.magnitude();

					if (speed > 1.0f)
					{
						// スタンドはグラウンドより摩擦が強い想定で少し強めに減衰
						float dampingFactor = 0.95f;

						if(Ball::Instance().GetHasCollidedWithPole())
						{
							// ポール衝突後にスタンドへ着地: 速度を強く減衰させる
							dampingFactor = 0.90f;
						}
						
						velocity *= dampingFactor;

						physx::PxVec3 angularVelocity = ballCollider->getAngularVelocity();
						angularVelocity *= dampingFactor;

						ballCollider->setLinearVelocity(velocity);
						ballCollider->setAngularVelocity(angularVelocity);
					}
					else
					{
						ballCollider->setLinearVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
						ballCollider->setAngularVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
					}
					});
			}

			if (!Ball::Instance().GetHasCollidedWithFence())
			{
				bool wasAlreadyGrounded = Ball::Instance().GetHasCollidedWithGround();

				Ball::Instance().SetHasCollidedWithFence(true);
				Ball::Instance().SetHasCollidedWithGround(true);
				Ball::Instance().SetHasBeenJudged(true);

				//フェアの状態で1度グラウンドについたら、その後のファウル判定と飛距離計算はしない
				if (wasAlreadyGrounded && !Ball::Instance().GetIsFoulConfirmed())
				{
					return;
				}

				physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
				if (ballCollider)
				{
					physx::PxVec3 ballPosition = ballCollider->getGlobalPose().p;

					// フェア/ファウル判定（ホームベースから見た角度が±45度以内ならフェア）
					float standAngleDeg = std::atan2(ballPosition.x, ballPosition.z) * (180.0f / 3.14159265359f);
					bool isFairAtStand = (ballPosition.z >= 0.0f) && (std::fabs(standAngleDeg) <= 45.0f);
					if (!isFairAtStand)
					{
						Ball::Instance().SetIsFoulConfirmed(true); // ファウル確定フラグを設定

						if(foulSound && Ball::Instance().GetHasCollidedWithBat() && !isFoulSoundPlayed)
						{
							foulSound->PlayOneShot();
							isFoulSoundPlayed = true;
						}

						char debugMessage[256];
						snprintf(debugMessage, sizeof(debugMessage),
							"ファウル：スタンドに衝突 x=%.2f y=%.2f z=%.2f 角度=%.1f°\n",
							ballPosition.x, ballPosition.y, ballPosition.z, standAngleDeg);
						OutputDebugStringA(debugMessage);
						if (consoleLog)
							consoleLog->push_back(u8"[Hit] ファウル：スタンドに衝突");
					}
					else
					{

						//グラウンドに当たらずかつホームランゾーンを通過せずにスタンドに当たったらフェンス直撃
						//グラウンドに当たってかつホームランゾーンを通過していなかったらヒット
						if (Ball::Instance().GetHasCollidedWithGround() && !Ball::Instance().GetHasPassedHomeRunZone())
						{
							char debugMessage[256];
							snprintf(debugMessage, sizeof(debugMessage),
								"ヒット！：スタンドに衝突\n");
							OutputDebugStringA(debugMessage);
							if (consoleLog)
								consoleLog->push_back(u8"[Hit] ヒット！：スタンドに衝突");
						}
						else if (!Ball::Instance().GetHasCollidedWithGround() && !Ball::Instance().GetHasPassedHomeRunZone())
						{
							char debugMessage[256];
							snprintf(debugMessage, sizeof(debugMessage),
								"フェンス直撃！：スタンドに衝突\n");
							OutputDebugStringA(debugMessage);
							if (consoleLog)
								consoleLog->push_back(u8"[Hit] フェンス直撃！：スタンドに衝突");
						}
						
					}

					// ===== 飛距離計算 =====
					physx::PxVec3 ballFencePosition = ballCollider->getGlobalPose().p;
					DirectX::XMFLOAT3 ballHitPos = Ball::Instance().GetBallHitPosition();

					float distanceX = ballFencePosition.x - ballHitPos.x;
					float distanceZ = ballFencePosition.z - ballHitPos.z;
					float horizontalDistance = sqrtf(distanceX * distanceX + distanceZ * distanceZ);

					if (consoleLog)
					{
						char logBuf[512];
						snprintf(logBuf, sizeof(logBuf),
							u8"[Hit] ボールがフェンスに入った！ 飛距離: %.1f m",
							horizontalDistance);
						consoleLog->push_back(logBuf);
					}

					ballHorizontalDistance = horizontalDistance;
					lastDistanceWasTotal = false; // フェンス衝突時は実測飛距離として扱う

					if (!Ball::Instance().GetHasPassedHomeRunZone() && Ball::Instance().GetHasCollidedWithBat())
					{
						Combo::Instance().ResetCombo(); // グラウンドに着地したらコンボをリセット
					}
				}

				
			}
		}

		//ボールとネットの衝突を検知
		for (int i = 0; i < NET_COUNT; i++)
		{

			//ネットに当たったら速度を落とす
			if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "NetCollider" + std::to_string(i)) ||
				(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "NetCollider" + std::to_string(i)))
			{
				// キューに速度変更リクエストを追加
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					velocityUpdateQueue.push([]() {
						physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
						if (ballCollider)
						{
							physx::PxVec3 velocity = ballCollider->getLinearVelocity();
							velocity.x *= 0.3f; // 速度を30%にする
							velocity.z *= 0.3f; // 速度を30%にする
							ballCollider->setLinearVelocity(velocity);
						}
						});
				}
			}

			if(Ball::Instance().GetHasCollidedWithNet())
				continue;//ネットに衝突済みなら以降の判定は不要

			if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "NetCollider" + std::to_string(i)) ||
				(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "NetCollider" + std::to_string(i)))
			{
				Ball::Instance().SetHasCollidedWithNet(true);
				OutputDebugStringA("ネットに衝突！\n");
				if (consoleLog)
					consoleLog->push_back(u8"[Hit] ネットに衝突！");
				Combo::Instance().ResetCombo(); // ネットに衝突したらコンボをリセット

				physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();

				physx::PxVec3 ballFencePosition = ballCollider->getGlobalPose().p;
				DirectX::XMFLOAT3 ballHitPos = Ball::Instance().GetBallHitPosition();

				float distanceX = ballFencePosition.x - ballHitPos.x;
				float distanceZ = ballFencePosition.z - ballHitPos.z;
				float horizontalDistance = sqrtf(distanceX * distanceX + distanceZ * distanceZ);

				ballHorizontalDistance = horizontalDistance;
				lastDistanceWasTotal = false; // グラウンド着地時は実測飛距離として扱う
			}
		}

	}

	

	
}

inline float PowerToExitVelocityScale(float power)
{
	// GetSelectedRealBatterPower() の値域
	constexpr float kPowerMin = 1.0f;
	constexpr float kPowerMax = 99.0f;

	// パワー最低時・最高時の打球速度倍率（ここを調整してバランスを取る）
	constexpr float kScaleMin = 0.6f;
	constexpr float kScaleMax = 1.2f;

	// 範囲外の値が来ても安全なようにクランプ
	float clampedPower = std::clamp(power, kPowerMin, kPowerMax);

	// 0.0〜1.0に正規化してから線形補間
	float t = (clampedPower - kPowerMin) / (kPowerMax - kPowerMin);

	return kScaleMin + (kScaleMax - kScaleMin) * t;
}

void Physics::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	for (physx::PxU32 i = 0; i < count; i++)
	{
		const physx::PxTriggerPair& pair = pairs[i];

		// 削除済みアクターは無視
		if (pair.flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER |
			physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
			continue;

		if(Ball::Instance().GetHasCollidedWithPole() || Ball::Instance().GetHasCollidedWithGround() || Ball::Instance().GetHasCollidedWithFence())
		{
			continue; // ポールに衝突済みなら以降の判定は不要
		}

		// ボールが HomeRunTrigger に入った瞬間
		if (pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			bool ballIsTriggerActor = (pair.otherActor == Ball::Instance().GetBallCollider());
			bool triggerIsHomeRun = (pair.triggerActor->getName() &&
				std::string(pair.triggerActor->getName()) == "HomeRunTrigger");

			if (ballIsTriggerActor && triggerIsHomeRun)
			{
				physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
				if (ballCollider)
				{
					physx::PxVec3 ballPos = ballCollider->getGlobalPose().p;


					//ホームランゾーンを通過する前にグラウンドに当たった状態で
					//ホームランゾーンを通過したらエンタイトルツーベース
					//通過する前にグラウンドに当たっていない場合はホームラン
					if (Ball::Instance().GetHasCollidedWithGround())
					{
						OutputDebugStringA("エンタイトルツーベース！\n");
						if (consoleLog)
							consoleLog->push_back(u8"[Hit] エンタイトルツーベース！");
					}
					else
					{
						Ball::Instance().SetHasPassedHomeRunZone(true);
						OutputDebugStringA("ホームランゾーン通過！\n");
						if (consoleLog)
							consoleLog->push_back(u8"[Hit] ホームランゾーン通過！");
					}

				}
			}
		}

		//ボールがFoulTriggerに入った瞬間
		if (pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			bool ballIsTriggerActor = (pair.otherActor == Ball::Instance().GetBallCollider());
			bool triggerIsFoul = (pair.triggerActor->getName() &&
				std::string(pair.triggerActor->getName()) == "FoulTrigger");
			if (ballIsTriggerActor && triggerIsFoul)
			{
				Ball::Instance().SetIsFoulConfirmed(true);
				OutputDebugStringA("ファウルゾーン通過！\n");
				if (consoleLog)
					consoleLog->push_back(u8"[Hit] ファウルゾーン通過！");
				Combo::Instance().ResetCombo(); // ファウルゾーン通過したらコンボをリセット
				if(foulWhistleSound)
					foulWhistleSound->PlayOneShot();
				if (foulSound && Ball::Instance().GetHasCollidedWithBat() && !isFoulSoundPlayed)
				{
					foulSound->PlayOneShot();
					isFoulSoundPlayed = true;
				}
			}
		}

		// ボールとバットの組み合わせか確認
		// onTrigger 内のバット処理ブロックをこれに差し替え
		{
			bool ballAndBat =
				(pair.triggerActor == Player::Instance().GetBatCollider() &&
					pair.otherActor == Ball::Instance().GetBallCollider()) ||
				(pair.triggerActor == Ball::Instance().GetBallCollider() &&
					pair.otherActor == Player::Instance().GetBatCollider());

			if (!ballAndBat) continue;
			if (pair.status != physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) continue;
			if (Ball::Instance().GetHasBeenJudged()) continue;
			if (Ball::Instance().GetHasCollidedWithBat()) continue;
			if (!Player::Instance().IsSwinging()) continue; // スイング中のみ判定

			HitJudge2DResult result;
			if (!HitJudge2D::Instance().EvaluateContact(result))
				continue; // 空振り：トリガーなので何も起きない

			//const HitJudge2DResult& result = HitJudge2D::Instance().GetLastResult();

			
		
			Ball::Instance().SetHasCollidedWithBat(true);
			Ball::Instance().CancelBezier();
			ballSprite::Instance().SetShowBallBoard(true);
			ballSprite::Instance().SetStopBallOnHit(true);
			ballCount::Instance().DecreaseRemainingBalls(1);

			physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
			physx::PxRigidDynamic* batCollider = Player::Instance().GetBatCollider();
			if (!ballCollider || !batCollider) continue;

			physx::PxVec3 hitPos = ballCollider->getGlobalPose().p;
			Ball::Instance().SetBallHitPosition({ hitPos.x, hitPos.y, hitPos.z });

			const float BALL_RADIUS = 0.037f;
			const float PI = 3.14159265359f;

			physx::PxVec3 ballVelocity = ballCollider->getLinearVelocity();
			physx::PxVec3 batVelocity = batCollider->getLinearVelocity();
			float ballSpeed = ballVelocity.magnitude();
			float batSpeed = batVelocity.magnitude();

			// マテリアル取得
			physx::PxMaterial* ballMaterial = nullptr;
			physx::PxMaterial* batMaterial = nullptr;
			{
				physx::PxShape* ballShape = nullptr;
				ballCollider->getShapes(&ballShape, 1);
				if (ballShape) ballShape->getMaterials(&ballMaterial, 1);

				physx::PxShape* batShape = nullptr;
				batCollider->getShapes(&batShape, 1);
				if (batShape) batShape->getMaterials(&batMaterial, 1);
			}
			float combinedRestitution = (ballMaterial && batMaterial)
				? (ballMaterial->getRestitution() + batMaterial->getRestitution()) * 0.5f
				: 0.5f;
			float combinedFriction = (ballMaterial && batMaterial)
				? (ballMaterial->getStaticFriction() + batMaterial->getStaticFriction()) * 0.5f
				: 0.4f;

			// 衝突法線（トリガーは contactPoints が取れないので位置差分で代用）
			physx::PxVec3 contactNormal;

			// 衝突法線がほぼゼロベクトルの場合は、バットの前方方向を使用
			if (batVelocity.magnitude() > 0.1f)
			{
				contactNormal = batVelocity.getNormalized();
			}
			else
			{
				contactNormal = ballCollider->getGlobalPose().p - batCollider->getGlobalPose().p;
				if (contactNormal.normalize() < 1e-4f)
				{
					contactNormal = physx::PxVec3(0.0f, 0.0f, -1.0f);
				}
			}

			contactNormal.y = 0.0f;
			if (contactNormal.normalize() < 1e-4f)
			{
				contactNormal = physx::PxVec3(0.0f, 0.0f, -1.0f);
			}

			//打ち出し方向(exit velosity)の向き
			physx::PxVec3 launchDirection;

			// バットの速度がほぼゼロの場合は、バットの前方方向を使用
			if (batVelocity.magnitude() > 0.1f)
			{
				launchDirection = batVelocity.getNormalized();
			}

			float originalAngleDegEarly = std::atan2(launchDirection.x, launchDirection.z) * (180.0f / PI);
			Physics::Instance().SetBallOriginalDirection(originalAngleDegEarly);
			//else
			//{
			//	// バットの速度がほぼゼロの場合は、バットの前方方向を使用
			//	launchDirection = ballCollider->getGlobalPose().p - batCollider->getGlobalPose().p;
			//	if (launchDirection.normalize() < 1e-4f)
			//	{
			//		launchDirection = physx::PxVec3(0.0f, 0.0f, -1.0f);
			//	}
			//}

			physx::PxVec3 horizDir;

			{
				float angle2DRad = result.launchAngle2DDeg * (3.14159265359f / 180.0f);
				horizDir = launchDirection;
				
				horizDir.y = 0.0f;// 水平方向のみを考慮
				if (horizDir.magnitude() < 1e-3f)
				{
					horizDir = physx::PxVec3(0.0f, 0.0f, -1.0f);
				}
				else
				{
					horizDir.normalize();
				}

				launchDirection = physx::PxVec3(horizDir.x * cosf(angle2DRad), sinf(angle2DRad), horizDir.z * cosf(angle2DRad));
				launchDirection.normalize();
			}

			// 打球速度計算（onContact と同じロジック）
			physx::PxVec3 relativeVelocity = batVelocity - ballVelocity;
			float relativeVelocityAlongNormal = relativeVelocity.dot(contactNormal);

			const float BALL_MASS = 0.145f;
			const float EFFECTIVE_BAT_MASS = 0.3f;

			const float TARGET_Q = 0.2f;
			float massRatio = BALL_MASS / EFFECTIVE_BAT_MASS;
			float adjustedRestitution = std::clamp(
				TARGET_Q + (1.0f + TARGET_Q) * massRatio, 0.5f, 0.95f);
			float q = (adjustedRestitution - massRatio) / (1.0f + massRatio);
			
			//float estimatedExitVelocity = (q * ballSpeed + (1.0f + q) * batSpeed);
			float estimatedExitVelocity = result.exitVelocityMps;
			int batterPower = Player::Instance().GetSelectedRealBatterPower();
			float batterPowerScale = PowerToExitVelocityScale(static_cast<float>(batterPower));
			float pitchPowerScale = ballSprite::Instance().GetCurrentPitchPowerScale();
			//スイングのタイミングによって打球速度を補正する
			//ボールスプライトのターゲットが中心の時はバッターのパワーを1.1倍にして反映させる
			if(Pitcher::Instance().IsBezierTargetCenter())
			{
				batterPowerScale *= 1.1f;
			}
			
			//特殊能力による打球速度補正
			float SpecialAbilityScale = SpecialAbility::Instance().GetBallVelocityBonus();
			
			estimatedExitVelocity *= batterPowerScale;
			estimatedExitVelocity *= pitchPowerScale;
			estimatedExitVelocity *= SpecialAbilityScale;
			
			// 2D判定の仰角を使用
			float launchAngleDeg = result.launchAngle2DDeg;

			float angleScale = 1.0f;
			if (launchAngleDeg <= -20.0f) angleScale = 0.85f - 0.1f * std::clamp((launchAngleDeg + 20.0f) / -40.0f, 0.0f, 1.0f);
			else if (launchAngleDeg < 0.0f)  angleScale = 0.95f - 0.1f * std::clamp(launchAngleDeg / -20.0f, 0.0f, 1.0f);
			estimatedExitVelocity *= angleScale;

			//打球方向を基準にしたローカル座標系を作る
			physx::PxVec3 forward = launchDirection;
			physx::PxVec3 worldUp(0.0, 1.0f, 0.0f);

			physx::PxVec3 rightAxis = worldUp.cross(forward);//右方向
			if(rightAxis.magnitude() <1e-4f) rightAxis = physx::PxVec3(1.0f, 0.0f, 0.0f);//forwardが上方向と同じ場合は右方向をX軸にする
			
			physx::PxVec3 upAxis = forward.cross(rightAxis);//上方向
			upAxis.normalize();

			// 回転計算
			physx::PxVec3 tangentialVelocity =
				relativeVelocity - contactNormal * relativeVelocityAlongNormal;
			float tangentialSpeed = tangentialVelocity.magnitude();
			float angularVelocityRadPerSec = (tangentialSpeed > 1e-3f)
				? (tangentialSpeed * combinedFriction) / BALL_RADIUS : 0.0f;

			//バックスピン成分
			float backSpinRatio = std::clamp(launchAngleDeg / 25.0f, 0.0f, 1.0f) * 0.8f + 0.2f;// 0度で0.2、25度以上で1.0

			//サイドスピン成分
			float launchHorizontalAngleDeg = std::atan2(
				horizDir.x, horizDir.z) * (180.0f / PI); // horizDir は launchDirection 計算時に使ったもの
			float sideSpinRatio = std::clamp(launchHorizontalAngleDeg / 45.0f, -1.0f, 1.0f);// 左方向が正、右方向が負


			physx::PxVec3 spinAxis = rightAxis * -backSpinRatio + upAxis * sideSpinRatio;
			if (spinAxis.magnitude() > 1e-4f) spinAxis.normalize();
			else spinAxis = rightAxis;
			
			
			constexpr float SOFT_LIMIT_THRESHOLD = 165.0f;// 165km/h以上は回転を抑制
			constexpr float SOFT_LIMIT_MAX = 195.0f;// 195km/h以上は回転を抑制
			constexpr float SOFT_LIMIT_KNEE = SOFT_LIMIT_MAX - SOFT_LIMIT_THRESHOLD;// 30km/hの範囲で抑制

			// 最終速度
			physx::PxVec3 newBallVelocity = launchDirection * estimatedExitVelocity;
			//newBallVelocity *= result.velocityScale; // 2D判定の倍率

			//ソフトリミットをかける
			float speed = newBallVelocity.magnitude() * 3.6f;
			if(speed > SOFT_LIMIT_THRESHOLD)
			{
				float excess = speed - SOFT_LIMIT_THRESHOLD;// 165km/hを超えた分
				//tanhで滑らかに抑制する
				float compressedSpeed = SOFT_LIMIT_THRESHOLD + SOFT_LIMIT_KNEE * std::tanh(excess / SOFT_LIMIT_KNEE);// 165km/hを超えた分をtanhで抑制
				newBallVelocity *= compressedSpeed / speed;// 速度を圧縮
			}

			// 打球方向判定
			float originalAngleDeg = std::atan2(newBallVelocity.x, newBallVelocity.z) * (180.0f / PI);
			SetBallOriginalDirection(originalAngleDeg);
			float hitDirectionAngleDeg = std::fabs(originalAngleDeg);
			const char* hitResult = u8"ファウル";
			if (hitDirectionAngleDeg <= 45.0f)
			{
				if (hitDirectionAngleDeg <= 15.0f) hitResult = u8"センター方向";//+15～-15
				else if (originalAngleDeg < 0.0f)       hitResult = u8"レフト方向";//-15～-45
				else                                     hitResult = u8"ライト方向";//+15～+45
			}

			

			float finalExitVelocityKmh = newBallVelocity.magnitude() * 3.6f;

			// バレルゾーン判定（打球速度が158km/h以上かつ打球角度が25～30以内）
			bool isBarrelZone = (finalExitVelocityKmh >= 158.0f) &&
				(launchAngleDeg >= 25.0f && launchAngleDeg <= 30.0f);
			if (isBarrelZone) hitResult = u8"バレルゾーン！";

			
			//打球方向が15度～30度の範囲内で打球速度が170キロ以上、打球角度が25度～35度の時は確信ホームランとして仮でログ出力
			//後で確信ホームラン用のカメラ演出に切り替える
			const char* homeRunResult = nullptr;
			if(hitDirectionAngleDeg >= 15.0f && hitDirectionAngleDeg <= 30.0f && launchAngleDeg >= 25.0f && launchAngleDeg <= 35.0f && finalExitVelocityKmh >= 170.0f)
			{
				homeRunResult = u8"確信ホームラン！";
				isHomeRun = true;
			}

			//センター方向は打球角度30度以上35ど以内かつ打球速度が180キロ以上でホームラン判定
			if(hitDirectionAngleDeg <= 15.0f && launchAngleDeg >= 30.0f && launchAngleDeg <= 35.0f && finalExitVelocityKmh >= 180.0f)
			{
				homeRunResult = u8"確信ホームラン！";
				isHomeRun = true;
			}

		
			// キューに登録
			float limitedBallSpeedKmh = ballVelocity.magnitude() * 3.6f;
			std::lock_guard<std::mutex> lock(queueMutex);
			std::vector<std::string>* logPtr = consoleLog;
			velocityUpdateQueue.push([ballCollider, newBallVelocity, spinAxis,
				angularVelocityRadPerSec, limitedBallSpeedKmh, batSpeed,
				launchAngleDeg, hitDirectionAngleDeg, hitResult, logPtr, homeRunResult]()
				{
					ballCollider->setLinearVelocity(newBallVelocity);
					ballCollider->setAngularVelocity(spinAxis * angularVelocityRadPerSec);
					ballCollider->setLinearDamping(0.0f);
					ballCollider->setAngularDamping(0.0f);
					Physics::Instance().ballWasHit = true;

					float exitVelocityKmh = newBallVelocity.magnitude() * 3.6f;
					float spinRpm = (angularVelocityRadPerSec * 60.0f) / (2.0f * 3.14159265359f);
					if (logPtr)
					{
						char logBuf[512];
						snprintf(logBuf, sizeof(logBuf),
							u8"[Hit] 初速:%.1fkm/h スイング:%.1fkm/h 打球:%.1fkm/h 角度:%.1f° 方向:%.1f°[%s] 回転:%.0frpm  [%s]",
							limitedBallSpeedKmh, batSpeed * 3.6f, exitVelocityKmh,
							launchAngleDeg, hitDirectionAngleDeg, hitResult, spinRpm, homeRunResult ? homeRunResult : "");
						logPtr->push_back(logBuf);
					}
				});

			outSpeed = newBallVelocity.magnitude() * 3.6f;
			outAngle = launchAngleDeg;
			outDirection = hitDirectionAngleDeg;
			outOriginalDirection = originalAngleDeg;

			//エフェクト再生

			float speedKmh = newBallVelocity.magnitude() * 3.6f;

			if (speedKmh >= 150.0f && speedKmh < 160.0f && launchAngleDeg >= 15.0f && launchAngleDeg <= 40.0f)
			{
				if (hitSmallEffect) hitSmallEffect->Play(DirectX::XMFLOAT3(ballCollider->getGlobalPose().p.x, ballCollider->getGlobalPose().p.y, ballCollider->getGlobalPose().p.z), 0.8f);
			}
			if(speedKmh >= 160.0f && speedKmh < 170.0f && launchAngleDeg >= 15.0f && launchAngleDeg <= 40.0f)
			{
				if (hitEffect) hitEffect->Play(DirectX::XMFLOAT3(ballCollider->getGlobalPose().p.x, ballCollider->getGlobalPose().p.y, ballCollider->getGlobalPose().p.z));
			}
			else if (speedKmh >= 170.0f && launchAngleDeg >= 15.0f && launchAngleDeg <= 40.0f)
			{
				if (hitBigEffect) hitBigEffect->Play(DirectX::XMFLOAT3(ballCollider->getGlobalPose().p.x, ballCollider->getGlobalPose().p.y, ballCollider->getGlobalPose().p.z));
			}
			
			
			//打球音再生
			if (speedKmh >= 170.0f && launchAngleDeg >= 15.0f)
			{
				// 170km/h以上 且つ 角度15度以上：本塁打音
				if (homeRunSound) homeRunSound->Play(false);
			}
			else if (speedKmh >= 150.0f)
			{
				// 150km/h以上（170km/h以上のゴロ含む）：長打音
				if (longHitSound) longHitSound->Play(false);
			}
			else if (speedKmh >= 100.0f)
			{
				// 100km/h〜150km/h未満：通常ヒット音
				if (hitSound) hitSound->Play(false);
			}
			else
			{
				// 上記以外のすべてのケース（100km/h未満）：詰まった音
				if (hitClogSound) hitClogSound->Play(false);
			}
			

		}


	}
}