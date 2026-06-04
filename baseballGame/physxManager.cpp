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
		pxSceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		pxSceneDesc.cpuDispatcher = pxDispatcher;
		pxSceneDesc.filterShader = SimulationFilterShader;
		pxSceneDesc.simulationEventCallback = this;

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
}

// 終了化
void Physics::Finalize()
{
	PxCloseExtensions();

	PX_RELEASE(pxControllerManager);
	PX_RELEASE(pxScene);
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

//--------------------------
// NOTE:⑧衝突検出フィルタリング
//--------------------------
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

	return physx::PxFilterFlag::eDEFAULT;
}

void Physics::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	//OutputDebugStringA("onContact called\n"); // ログを追加

	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& pair = pairs[i];

	
			// ボールとバットの衝突を検知
		if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1] == Player::Instance().GetBatCollider()) ||
			(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0] == Player::Instance().GetBatCollider()))
		{
			// 衝突が既に処理されている場合はスキップ
			if (Ball::Instance().GetHasCollided())
			{
			    continue; // または continue; ループ内なら
			}

			Ball::Instance().SetHasCollided(true);

			physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
			physx::PxRigidDynamic* batCollider = Player::Instance().GetBatCollider();

			if (ballCollider && batCollider)
			{
				// バット衝突時のボール位置を保存
				physx::PxVec3 hitPos = ballCollider->getGlobalPose().p;
				Ball::Instance().SetBallHitPosition({ hitPos.x, hitPos.y, hitPos.z });

				// ===== 物理定数 =====
				const float BALL_RADIUS = 0.037f;
				const float PI = 3.14159265359f;

				// ===== 1. 衝突前の情報取得 =====
				physx::PxVec3 ballVelocity = ballCollider->getLinearVelocity();
				physx::PxVec3 batVelocity = batCollider->getLinearVelocity();

				float ballSpeed = ballVelocity.magnitude();

				float batSpeed = batVelocity.magnitude();

				// ===== マテリアル取得 =====
				physx::PxMaterial* ballMaterial;
				physx::PxMaterial* batMaterial;
				{
					physx::PxShape* ballShape;
					ballCollider->getShapes(&ballShape, 1);
					ballShape->getMaterials(&ballMaterial, 1);

					physx::PxShape* batShape;
					batCollider->getShapes(&batShape, 1);
					batShape->getMaterials(&batMaterial, 1);
				}

				//反発係数を取得
				float combinedRestitution = (ballMaterial->getRestitution() + batMaterial->getRestitution()) * 0.5f;

				//摩擦係数を取得
				float combinedFriction = ((ballMaterial->getStaticFriction() + batMaterial->getStaticFriction()) * 0.5f);

				// ===== 2. 接触点・法線の取得 =====
				physx::PxVec3 collisionNormal(0.0f, 0.0f, 0.0f);
				{
					physx::PxContactPairPoint contactPoints[16];
					physx::PxU32 numContactPoints = pairs[i].extractContacts(contactPoints, 16);

					if (numContactPoints > 0)
					{
						collisionNormal = contactPoints[0].normal;
						collisionNormal.normalize();
					}
					else
					{
						collisionNormal = (ballCollider->getGlobalPose().p - batCollider->getGlobalPose().p);
						if (collisionNormal.normalize() < 1e-4f)
						{
							collisionNormal = physx::PxVec3(0.0f, 0.0f, 1.0f);
						}
					}
				}

				// ===== 3. 衝突計算 =====
				physx::PxVec3 relativeVelocity = batVelocity - ballVelocity;
				float relativeVelocityAlongNormal = relativeVelocity.dot(collisionNormal);

				//ボールの質量とバットの有効質量を設定
				const float BALL_MASS = 0.145f;
				const float EFFECTIVE_BAT_MASS = 0.3f;

				float impulseScalar = 0.0f;
				if (std::fabs(relativeVelocityAlongNormal) > 1e-4f)
				{
					impulseScalar = -(1.0f + combinedRestitution) * relativeVelocityAlongNormal;
					impulseScalar /= (1.0f / BALL_MASS + 1.0f / EFFECTIVE_BAT_MASS);
				}

				// ===== 打撃係数・打球速度 =====
				const float TARGET_Q = 0.2f;
				float massRatio = BALL_MASS / EFFECTIVE_BAT_MASS;
				float adjustedRestitution = TARGET_Q + (1.0f + TARGET_Q) * massRatio;
				adjustedRestitution = std::clamp(adjustedRestitution, 0.5f, 0.95f);
				float q = (adjustedRestitution - massRatio) / (1.0f + massRatio);

				float estimatedExitVelocity = (q * ballSpeed + (1.0f + q) * batSpeed);

				// ===== 打球角度による速度調整 =====
				float launchAngle = std::atan2(
					collisionNormal.y,
					std::sqrt(collisionNormal.x * collisionNormal.x + collisionNormal.z * collisionNormal.z));
				float launchAngleDeg = launchAngle * (180.0f / PI);

				float angleScale = 1.0f;
				if (launchAngleDeg <= -20.0f)
				{
					float t = std::clamp((launchAngleDeg + 20.0f) / -40.0f, 0.0f, 1.0f);
					angleScale = 0.85f - 0.1f * t;
				}
				else if (launchAngleDeg < 0.0f)
				{
					float t = std::clamp(launchAngleDeg / -20.0f, 0.0f, 1.0f);
					angleScale = 0.95f - 0.1f * t;
				}
				estimatedExitVelocity *= angleScale;

				// ===== 4. 回転計算 =====
				physx::PxVec3 tangentialVelocity = relativeVelocity - collisionNormal * relativeVelocityAlongNormal;
				float tangentialSpeed = tangentialVelocity.magnitude();

				float angularVelocityRadPerSec = 0.0f;
				if (tangentialSpeed > 1e-3f)
				{
					angularVelocityRadPerSec = (tangentialSpeed * combinedFriction) / BALL_RADIUS;
				}

				physx::PxVec3 spinAxis = collisionNormal.cross(tangentialVelocity);
				if (spinAxis.magnitude() > 1e-3f)
				{
					spinAxis.normalize();
				}
				else
				{
					spinAxis = physx::PxVec3(0.0f, 1.0f, 0.0f);
				}
				spinAxis.x = -spinAxis.x;
				spinAxis.y = -spinAxis.y;

				// ===== 5. 最終速度計算 =====
				physx::PxVec3 newBallVelocity = ballVelocity + collisionNormal * impulseScalar / BALL_MASS;
				float physSpeed = newBallVelocity.magnitude();

				if (physSpeed > 1e-3f)
				{
					float targetSpeed = (std::min)(estimatedExitVelocity, physSpeed * 1.0f);
					newBallVelocity *= (targetSpeed / physSpeed);
				}
				else
				{
					newBallVelocity = collisionNormal * estimatedExitVelocity;
				}

				// ===== 打球速度の上限設定（190 km/h） =====
				float finalSpeed = newBallVelocity.magnitude();
				if (finalSpeed > 52.78f)  // 190 km/h ≈ 52.78 m/s
				{
					newBallVelocity = (newBallVelocity / finalSpeed) * 52.78f;
				}

				/// ===== 6. 打球方向（左右の角度）の計算 =====
				// Z方向（バックスクリーン方向）を0度としたときの、打球速度ベクトル(XとZ) の角度を計算
				float originalAngleDeg = std::atan2(newBallVelocity.x, newBallVelocity.z) * (180.0f / PI);

				// 絶対値に変換
				float hitDirectionAngleDeg = std::fabs(originalAngleDeg);

				// 打球方向の判定（絶対値を使って判定する）
				const char* hitResult = "ファウル";
				if (hitDirectionAngleDeg <= 45.0f)
				{
					if (hitDirectionAngleDeg <= 15.0f)
					{
						hitResult = "センター方向";
					}
					else if (originalAngleDeg < 0.0f) // マイナスならレフト方向
					{
						hitResult = "レフト方向";
					}
					else // プラスならライト方向
					{
						hitResult = "ライト方向";
					}
				}


				//バレルゾーンの設定
				//打球速度が158キロ以上かつ打球角度が26度～30度の範囲ならバレルゾーンとする
				bool isBarrelZone = (estimatedExitVelocity >= 43.89f) && (launchAngleDeg >= 26.0f && launchAngleDeg <= 30.0f);
				if (isBarrelZone)
				{
					hitResult = "バレルゾーン！";
				}

				////空気抵抗・風・マグヌスを考慮した落下点予測
				//physx::PxVec3 predictedLandingPoint = hitPos;
				//{
				//	physx::PxVec3 pPos = hitPos;
				//	physx::PxVec3 pVel = newBallVelocity;
				//	physx::PxVec3 pSpin = spinAxis * angularVelocityRadPerSec;

				//	DirectX::XMFLOAT3 windDX = Wind::Instance().GetWindVector();
				//	physx::PxVec3 windVec(windDX.x, windDX.y, windDX.z);

				//	float dt = 0.01f; // シミュレーションの時間刻み
				//	float timeLimit = 10.0f; // 最大シミュレーション時間
				//	float elapsedTime = 0.0f;

				//	const float AIR_DENSITY = 1.225f; // kg/m^3
				//	const float BALL_DENSITY_RADIUS = 0.0365f; // 投影面積用の半径
				//	const float BALL_AREA = PI * BALL_DENSITY_RADIUS * BALL_DENSITY_RADIUS; // 投影面積
				//	const float DRAG_COEFF = 0.41f; // 野球ボールの標準抗力係数
				//	const float GRAVITY = -9.81f;

				//	physx::PxScene* scene = ballCollider->getScene();

				//	// 地面、またはスタンド等に当たるまでループ
				//	while (elapsedTime < timeLimit)
				//	{
				//		// --- A. 物理挙動シミュレーション（次の移動先を先に計算） ---
				//		physx::PxVec3 relativeVel = pVel - windVec;
				//		float relativeSpeed = relativeVel.magnitude();
				//		physx::PxVec3 acceleration(0.0f, GRAVITY, 0.0f);

				//		if (relativeSpeed > 0.0f)
				//		{
				//			float dragMag = 0.5f * AIR_DENSITY * relativeSpeed * relativeSpeed * DRAG_COEFF * BALL_AREA;
				//			physx::PxVec3 dragForce = -relativeVel.getNormalized() * dragMag;
				//			acceleration += dragForce / BALL_MASS;

				//			float angularSpeed = pSpin.magnitude();
				//			if (angularSpeed > 0.0f)
				//			{
				//				float spinParameter = (BALL_DENSITY_RADIUS * angularSpeed) / relativeSpeed;
				//				float liftCoeff = 1.5f * spinParameter;
				//				if (liftCoeff > 0.4f) liftCoeff = 0.4f;

				//				float magnusMag = 0.5f * AIR_DENSITY * relativeSpeed * relativeSpeed * liftCoeff * BALL_AREA;
				//				physx::PxVec3 magnusDir = pSpin.cross(relativeVel);
				//				if (magnusDir.magnitudeSquared() > 1e-4f)
				//				{
				//					magnusDir.normalize();
				//					acceleration += (magnusDir * magnusMag) / BALL_MASS;
				//				}
				//			}
				//		}

				//		// 次のステップの速度と位置を仮計算
				//		physx::PxVec3 nextVel = pVel + acceleration * dt;
				//		physx::PxVec3 nextPos = pPos + nextVel * dt;

				//		// ---  PhysXレイキャストによる本物のコライダー衝突判定 ---
				//		if (scene)
				//		{
				//			physx::PxVec3 rayDir = nextPos - pPos;
				//			float rayDistance = rayDir.magnitude();

				//			if (rayDistance > 1e-4f)
				//			{
				//				rayDir.normalize();
				//				physx::PxRaycastBuffer hitBuffer;

				//				// 現在地(pPos)から移動先(nextPos)の間に何かコライダーがあるか光線を飛ばす
				//				
				//				if (scene->raycast(pPos, rayDir, rayDistance, hitBuffer))
				//				{
				//					physx::PxActor* hitActor = hitBuffer.block.actor;
				//					if (hitActor && hitActor->getName())
				//					{
				//						std::string actorName = hitActor->getName();

				//						// 名前が "Ground" または "Stand" (フェンスやスタンド) ならそこで飛行終了
				//						// ※プログラムに合わせて "Wall" や "Fence" などを追加してください
				//						if (actorName == "Ground" || actorName == "Stand")
				//						{
				//							pPos = hitBuffer.block.position; // 衝突した正確な座標を代入
				//							break;
				//						}
				//					}
				//				}
				//			}
				//		}

				//		// 衝突がなければ、仮計算した次の状態を本採用してループを継続
				//		pVel = nextVel;
				//		pPos = nextPos;
				//		elapsedTime += dt;
				//	}

				//	// ループを抜けた最終座標を、落下点として保存
				//	predictedLandingPoint = pPos;

				//}



				// ===== 7. 速度をキューに登録 =====
				{
					// 制限後の初速を保存
					float limitedBallSpeedKmh = ballVelocity.magnitude() * 3.6f;

					std::lock_guard<std::mutex> lock(queueMutex);
					// キャプチャリストに predictedLandingPoint を追加
					velocityUpdateQueue.push([ballCollider, batCollider, newBallVelocity, spinAxis, angularVelocityRadPerSec,
						limitedBallSpeedKmh, batSpeed, launchAngleDeg, hitDirectionAngleDeg, hitResult]() {
							ballCollider->setLinearVelocity(newBallVelocity);
							ballCollider->setAngularVelocity(spinAxis * angularVelocityRadPerSec);
							ballCollider->setLinearDamping(0.0f);
							ballCollider->setAngularDamping(0.0f);

							// ===== バットの当たり判定自体を無効化 =====
							if (batCollider)
							{
								physx::PxShape* shape = nullptr;
								if (batCollider->getShapes(&shape, 1))
								{
									shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
								}
							}

							// ===== デバッグ出力（速度設定直後） =====
#ifdef _DEBUG
							float exitVelocityKmh = newBallVelocity.magnitude() * 3.6f;
							float spinRpm = (angularVelocityRadPerSec * 60.0f) / (2.0f * 3.14159265359f);

							// 打球の飛距離（初期位置からの水平距離）を計算
							DirectX::XMFLOAT3 hitPosPhysX = Ball::Instance().GetBallHitPosition();
							/*float diffX = predictedLandingPoint.x - hitPosPhysX.x;
							float diffZ = predictedLandingPoint.z - hitPosPhysX.z;
							float predictedDistance = std::sqrt(diffX * diffX + diffZ * diffZ);*/

							char debugMessage[512];
							snprintf(debugMessage, sizeof(debugMessage),
								"=== バット衝突 ===\nボール初速: %.1f km/h\nバット速度: %.1f km/h\n打球速度: %.1f km/h\n打ち出し角度(上下): %.1f°\n打球方向(左右): %.1f° [%s]\n回転: %.0f rpm\n",
								limitedBallSpeedKmh, batSpeed * 3.6f, exitVelocityKmh, launchAngleDeg, hitDirectionAngleDeg, hitResult, spinRpm);
							OutputDebugStringA(debugMessage);
#endif
						});
				}
			}
		}

		// ボールとグラウンドの衝突を検知
		if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Ground") ||
			(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Ground"))
		{
			Ball::Instance().SetHasCollided(true); // 衝突フラグを設定
			

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
					if (speed > 0.1f)
					{
						// フェンス衝突後かどうかで減衰率を変更
						float dampingFactor;
						if (Ball::Instance().GetHasCollidedWithFence())
						{
							// フェンス衝突後: 速度を大きく減速（1%に低下）
							dampingFactor = 0.97f;
						}
						else
						{
							// フェンス衝突なし: 通常の摩擦ベース減衰
							physx::PxMaterial* stageMaterial = Physics::Instance().GetMaterial();
							float friction = stageMaterial->getDynamicFriction();
							dampingFactor = 1.0f - (friction * 0.005f);
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

					physx::PxVec3 ballPosition = ballCollider->getGlobalPose().p;
					DirectX::XMFLOAT3 ballHitPos = Ball::Instance().GetBallHitPosition();

					float distanceX = ballPosition.x - ballHitPos.x;
					float distanceZ = ballPosition.z - ballHitPos.z;
					float horizontalDistance = sqrtf(distanceX * distanceX + distanceZ * distanceZ);

					// フェア/ファウル判定
						// ホームベース(z=0)からポール位置(±67, z=67)を結ぶ直線の傾き = 67/67 = 1.0
						// |x| <= z なら2本の直線の間（フェアゾーン）
					bool isFair = (ballPosition.z >= 0.0f) && (std::fabs(ballPosition.x) <= ballPosition.z);

					char debugMessage[256];
					snprintf(debugMessage, sizeof(debugMessage),
						"=== ボールが地面に着地 ===\n"
						"判定: %s\n"
						"水平飛距離: %.2f m\n",
						isFair ? "フェア" : "ファウル",
						horizontalDistance);
					OutputDebugStringA(debugMessage);
				}
			}
		}

		//ボールとスタンドの衝突を検知
		if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Stand") ||
			(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Stand"))
		{
			Ball::Instance().SetHasCollided(true);

			if (!Ball::Instance().GetHasCollidedWithFence())
			{
				Ball::Instance().SetHasCollidedWithFence(true);
				Ball::Instance().SetHasCollidedWithGround(true);

				physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
				if (ballCollider)
				{
					physx::PxVec3 ballPosition = ballCollider->getGlobalPose().p;

					// ===== ホームラン判定 =====
					if (ballPosition.z < 67.0f || !Ball::Instance().GetHasPassedHomeRunZone())
					{
						char debugMessage[256];
						snprintf(debugMessage, sizeof(debugMessage),
							"ファウル：x=%.2f y=%.2f z=%.2f\n",
							ballPosition.x, ballPosition.y, ballPosition.z);
						OutputDebugStringA(debugMessage);
					}
					else
					{
						if (ballPosition.y >= 4.0f)
						{
							char debugMessage[256];
							snprintf(debugMessage, sizeof(debugMessage),
								"ホームラン！：ボールの高さ %.2f m\n",
								ballPosition.y);
							OutputDebugStringA(debugMessage);
						}
						else
						{
							char debugMessage[256];
							snprintf(debugMessage, sizeof(debugMessage),
								"フェンスに当たったがホームランではない：ボールの高さ %.2f m\n",
								ballPosition.y);
							OutputDebugStringA(debugMessage);
						}
					}

					// ===== 飛距離計算 =====
					physx::PxVec3 ballFencePosition = ballCollider->getGlobalPose().p;
					DirectX::XMFLOAT3 ballHitPos = Ball::Instance().GetBallHitPosition();

					float distanceX = ballFencePosition.x - ballHitPos.x;
					float distanceY = ballFencePosition.y - ballHitPos.y;
					float distanceZ = ballFencePosition.z - ballHitPos.z;
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
						"=== ボールがフェンスに入った ===\n"
						"水平飛距離（実測）: %.2f m\n"
						"推定飛距離（スタンドなしでグラウンド着地）: %.2f m\n"
						"総飛距離: %.2f m\n",
						horizontalDistance,
						estimatedDistance,
						totalDistance);
					OutputDebugStringA(debugMessage);
				}
			}
		}
		

		//ボールとポールの衝突を検知
		if ((pairHeader.actors[0] == Ball::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Pole") ||
			(pairHeader.actors[1] == Ball::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Pole"))
		{
			Ball::Instance().SetHasCollided(true); // 衝突フラグを設定
			Ball::Instance().SetHasCollidedWithGround(true); // 地面衝突フラグを設定

			//ポールに当たったら無条件でホームラン判定
			// 初回のみ判定（スタンド衝突済みの場合はスキップ）
			if (!Ball::Instance().GetHasCollidedWithFence())
			{
				Ball::Instance().SetHasCollidedWithFence(true);
				char debugMessage[256];
				snprintf(debugMessage, sizeof(debugMessage),
					"ホームラン！：ポールに衝突");
				OutputDebugStringA(debugMessage);
			}
		}
	}
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

					
					{
						// トリガー通過でホームラン確定フラグをON
						Ball::Instance().SetHasPassedHomeRunZone(true);

						char debugMessage[256];
						snprintf(debugMessage, sizeof(debugMessage),
							"ホームランゾーン通過！",
							ballPos.y);
						OutputDebugStringA(debugMessage);
					}
					
				}
			}
		}
	}
}