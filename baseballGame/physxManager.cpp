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
#include "PitchingNet.h"

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

float GenerateRandomFloat2(float min, float max)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(min, max);
	return dis(gen);
}

void Physics::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	//OutputDebugStringA("onContact called\n"); // ログを追加

	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& pair = pairs[i];

		// 衝突ペアの情報を出力
		/*if (pairHeader.actors[0] && pairHeader.actors[0]->getName())
			OutputDebugStringA(pairHeader.actors[0]->getName());
		if (pairHeader.actors[1] && pairHeader.actors[1]->getName())
			OutputDebugStringA(pairHeader.actors[1]->getName());*/


			// ボールとバットの衝突を検知
			if ((pairHeader.actors[0] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[1] == Player::Instance().GetBatCollider()) ||
				(pairHeader.actors[1] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[0] == Player::Instance().GetBatCollider()))
			{
				Pitcher::Instance().SetHasCollided(true); // 衝突フラグを設定

				// ボールのコライダーを取得
				physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
				physx::PxRigidDynamic* batCollider = Player::Instance().GetBatCollider();

				if (ballCollider && batCollider)
				{
					// ===== 物理定数 =====
					const float AIR_DENSITY = 1.2f;                // 空気密度 (kg/m³)
					const float BALL_RADIUS = 0.037f;              // 野球ボールの半径 (m)
					const float MAGNUS_COEFFICIENT = 0.000035f;    // マグヌス係数
					const float PI = 3.14159265359f;

					// ===== 1. 衝突前の情報取得 =====
					physx::PxVec3 ballVelocity = ballCollider->getLinearVelocity();
					physx::PxVec3 batVelocity = batCollider->getLinearVelocity();

					float ballSpeed = ballVelocity.magnitude();
					float batSpeed = batVelocity.magnitude();

					// ボールとバットの質量を取得
					float ballMass = ballCollider->getMass();
					float batMass = batCollider->getMass();

					// マテリアルの取得
					physx::PxMaterial* ballMaterial;
					physx::PxShape* ballShape;
					ballCollider->getShapes(&ballShape, 1);
					ballShape->getMaterials(&ballMaterial, 1);

					physx::PxMaterial* batMaterial;
					physx::PxShape* batShape;
					batCollider->getShapes(&batShape, 1);
					batShape->getMaterials(&batMaterial, 1);

					// 反発係数と摩擦係数を取得
					float ballRestitution = ballMaterial->getRestitution();
					float batRestitution = batMaterial->getRestitution();
					float combinedRestitution = (ballRestitution + batRestitution) * 0.5f;

					float ballFriction = ballMaterial->getStaticFriction();
					float batFriction = batMaterial->getStaticFriction();
					float combinedFriction = (ballFriction + batFriction) * 0.5f;

					// ===== 2. 接触点の取得 =====
					physx::PxContactPairPoint contactPoints[16];
					physx::PxU32 numContactPoints = pairs[i].extractContacts(contactPoints, 16);

					// ===== 3. 衝突方向（法線方向）の取得 =====
					physx::PxVec3 collisionNormal(0.0f, 0.0f, 0.0f);
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
							// 法線が取得できない場合のフォールバック（前方とみなす）
							collisionNormal = physx::PxVec3(0.0f, 0.0f, 1.0f);
						}
					}

					// ===== 4. 相対速度の計算 =====
					physx::PxVec3 relativeVelocity = batVelocity - ballVelocity;
					float relativeVelocityAlongNormal = relativeVelocity.dot(collisionNormal);

					// ===== 5. 衝突後の速度計算（インパルスベース） =====
					float impulseScalar = 0.0f;
					if (std::fabs(relativeVelocityAlongNormal) > 1e-4f)
					{
						impulseScalar = -(1.0f + combinedRestitution) * relativeVelocityAlongNormal;
						impulseScalar /= (1.0f / ballMass + 1.0f / batMass);
					}

					// 打球速度の経験式（上限として使う）
					float estimatedExitVelocity = (batRestitution * ballSpeed + (1.0f + batRestitution) * batSpeed);

					// 打球角度（法線ベース）を計算
					float launchAngle = std::atan2(
						collisionNormal.y,
						std::sqrt(collisionNormal.x * collisionNormal.x + collisionNormal.z * collisionNormal.z));
					float launchAngleDeg = launchAngle * (180.0f / PI);

					// 角度に応じて速度スケールを調整（叩きつけるほど弱く）
					float angleScale = 1.0f;
					if (launchAngleDeg <= -20.0f)
					{
						// -20度〜-60度で 0.85 -> 0.75 まで落とす
						float t = std::clamp((launchAngleDeg + 20.0f) / -40.0f, 0.0f, 1.0f);
						angleScale = 0.85f - 0.1f * t;
					}
					else if (launchAngleDeg < 0.0f)
					{
						// 0〜-20度で 0.95 -> 0.85 まで落とす
						float t = std::clamp(launchAngleDeg / -20.0f, 0.0f, 1.0f);
						angleScale = 0.95f - 0.1f * t;
					}
					estimatedExitVelocity *= angleScale;

					// ===== 6. 回転の計算（摩擦による） =====
					physx::PxVec3 tangentialVelocity = relativeVelocity - collisionNormal * relativeVelocityAlongNormal;
					float tangentialSpeed = tangentialVelocity.magnitude();

					float angularVelocityRadPerSec = 0.0f;
					if (tangentialSpeed > 1e-3f)
					{
						angularVelocityRadPerSec = (tangentialSpeed * combinedFriction) / BALL_RADIUS;
					}

					// 回転軸（接線方向と法線方向の外積）
					physx::PxVec3 spinAxis = collisionNormal.cross(tangentialVelocity);
					if (spinAxis.magnitude() > 1e-3f)
					{
						spinAxis.normalize();
					}
					else
					{
						// 回転軸が取れない場合はとりあえず上方向でフォールバック
						spinAxis = physx::PxVec3(0.0f, 1.0f, 0.0f);
					}

					//回転軸をy軸のみ逆にする
					spinAxis.y = -spinAxis.y;

					physx::PxVec3 newAngularVelocity = spinAxis * angularVelocityRadPerSec;

					// ===== 7. 新しい線形速度を計算 =====
					physx::PxVec3 impulse = collisionNormal * impulseScalar;
					physx::PxVec3 newBallVelocity = ballVelocity + impulse / ballMass;

					float physSpeed = newBallVelocity.magnitude();
					if (physSpeed > 1e-3f)
					{
						// 物理ベースの速度に対して、経験式の速度を「上限」としてクランプする
						// かつ過剰なブーストにならないよう倍率を制限
						float targetSpeed = (std::min)(estimatedExitVelocity, physSpeed * 0.8f); // 物理ベースの速度を超えないようにしつつ、最大でも20%程度のブーストに留める
						float scale = targetSpeed / physSpeed;
						scale = std::clamp(scale, 0.0f, 1.0f);
						newBallVelocity *= scale;
					}
					else
					{
						// ほぼ静止していた場合は、法線方向に経験式速度を与える
						newBallVelocity = collisionNormal * estimatedExitVelocity;
					}

					// ===== 打球速度の上限設定（195 km/h） =====
					const float MAX_EXIT_VELOCITY_KMH = 195.0f;
					const float MAX_EXIT_VELOCITY_MS = MAX_EXIT_VELOCITY_KMH / 3.6f;  // m/s に変換
					float currentExitSpeed = newBallVelocity.magnitude();
					if (currentExitSpeed > MAX_EXIT_VELOCITY_MS)
					{
						// 速度を正規化してから上限値を掛ける
						newBallVelocity = (newBallVelocity / currentExitSpeed) * MAX_EXIT_VELOCITY_MS;
					}

					// ===== 8. マグヌス効果の初期計算（デバッグ用） =====
					float magnusForceMagnitude = 0.0f;
					physx::PxVec3 magnusForce(0.0f, 0.0f, 0.0f);

					if (estimatedExitVelocity > 1e-3f && angularVelocityRadPerSec > 1e-3f)
					{
						// F_magnus = Cm * ρ * v² * r * ω
						magnusForceMagnitude = MAGNUS_COEFFICIENT *
							AIR_DENSITY *
							estimatedExitVelocity * estimatedExitVelocity *
							BALL_RADIUS *
							angularVelocityRadPerSec;

						// マグヌス力の方向 = 角速度 × 速度（外積）
						physx::PxVec3 magnusDirection = newAngularVelocity.cross(newBallVelocity);
						if (magnusDirection.magnitude() > 1e-3f)
						{
							magnusDirection.normalize();
							magnusForce = magnusDirection * magnusForceMagnitude;
						}
					}

					// ===== 9. PhysXに速度を設定 =====
					{
						std::lock_guard<std::mutex> lock(queueMutex);
						velocityUpdateQueue.push([ballCollider, newBallVelocity, newAngularVelocity]() {
							ballCollider->setLinearVelocity(newBallVelocity);
							ballCollider->setAngularVelocity(newAngularVelocity);

							// 減衰は別途シーン側で制御するので、ここではゼロに固定
							ballCollider->setLinearDamping(0.0f);
							ballCollider->setAngularDamping(0.0f);
							});
					}

					// ===== 10. デバッグ情報の出力 =====
					float exitVelocityKmh = estimatedExitVelocity * 3.6f;
					float batSpeedKmh = batSpeed * 3.6f;
					float ballSpeedKmh = ballSpeed * 3.6f;
					float spinRpm = (angularVelocityRadPerSec * 60.0f) / (2.0f * PI);

					char debugMessage[768];
					snprintf(
						debugMessage,
						sizeof(debugMessage),
						"=== ボールとバットの衝突（マグヌス効果あり） ===\n"
						"初速（ボール）: %.1f km/h\n"
						"バット速度: %.1f km/h\n"
						"打球速度: %.1f km/h\n"
						"打球角度: %.1f 度\n"
						"回転数: %.0f rpm (%.1f rad/s)\n"
						"合成反発係数: %.3f\n"
						"合成摩擦係数: %.3f\n"
						"--- マグヌス効果 ---\n"
						"初期マグヌス力: %.3f N\n"
						"マグヌス方向: (%.3f, %.3f, %.3f)\n",
						ballSpeedKmh,
						batSpeedKmh,
						exitVelocityKmh,
						launchAngleDeg,
						spinRpm,
						angularVelocityRadPerSec,
						combinedRestitution,
						combinedFriction,
						magnusForceMagnitude,
						magnusForce.x,
						magnusForce.y,
						magnusForce.z);
					OutputDebugStringA(debugMessage);
				}
			}

		// ボールとステージの衝突を検知
		if ((pairHeader.actors[0] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Ground") ||
			(pairHeader.actors[1] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Ground"))
		{
			Pitcher::Instance().SetHasCollided(true); // 衝突フラグを設定

			// キューに速度変更リクエストを追加
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				velocityUpdateQueue.push([]() {
					physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
					physx::PxVec3 velocity = ballCollider->getLinearVelocity();

					// 速度の大きさをチェック
					float speed = velocity.magnitude();

					// 速度がある程度以上ある場合のみ減衰を適用
					if (speed > 0.1f)
					{
						// ステージのマテリアルから摩擦係数を取得
						physx::PxMaterial* stageMaterial = Physics::Instance().GetMaterial();
						float friction = stageMaterial->getDynamicFriction();

						// 摩擦係数から減衰率を計算
						// 摩擦係数が大きいほど減衰が強い
						float dampingFactor = 1.0f - (friction * 0.001f);  // 摩擦係数を減衰に反映
						//dampingFactor = std::clamp(dampingFactor, 0.3f, 0.999f);  // クランプして安定させる

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
			}

			// ボールのコライダーを取得
			physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
			if (ballCollider)
			{
				// ボールの形状を取得
				physx::PxShape* ballShape;
				ballCollider->getShapes(&ballShape, 1);

				// ボールのマテリアルを取得
				physx::PxMaterial* ballMaterial;
				ballShape->getMaterials(&ballMaterial, 1);

				ballMaterial->setRestitution(0.2f);
				
				
			}

			////何メートル飛んだかを表示(最初の着弾点のみ)
			//physx::PxRigidBody* ballCollider = Pitcher::Instance().GetBallCollider();
			//if (ballCollider)
			//{
			//	physx::PxVec3 ballPosition = ballCollider->getGlobalPose().p;
			//	float distance = sqrtf(ballPosition.x * ballPosition.x + ballPosition.z * ballPosition.z);
			//	char debugMessage[128];
			//	snprintf(debugMessage, sizeof(debugMessage), "Distance: %.2f m\n", distance);
			//	OutputDebugStringA(debugMessage);
			//}
			//// ボールのコライダーを取得
			//physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
			//if (ballCollider)
			//{
			//	// ボールの形状を取得
			//	physx::PxShape* ballShape;
			//	ballCollider->getShapes(&ballShape, 1);

			//	// ボールのマテリアルを取得
			//	physx::PxMaterial* ballMaterial;
			//	ballShape->getMaterials(&ballMaterial, 1);

			//	// ボールの反発係数を変更
			//	ballMaterial->setRestitution(0.2f);

			//}

		}

		//ボールとフェンスの衝突を検知
		if ((pairHeader.actors[0] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Stand") ||
			(pairHeader.actors[1] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Stand"))
		{
			Pitcher::Instance().SetHasCollided(true); // 衝突フラグを設定
			// ボールのコライダーを取得
			physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
			if (ballCollider)
			{
				// ボールの形状を取得
				physx::PxShape* ballShape;
				ballCollider->getShapes(&ballShape, 1);
				// ボールのマテリアルを取得
				physx::PxMaterial* ballMaterial;
				ballShape->getMaterials(&ballMaterial, 1);
				// ボールの反発係数を変更
				ballMaterial->setRestitution(0.0f);

				
			}
		}
		
		////ボールとピッチングネットの衝突を検知
		//if ((pairHeader.actors[0] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Net") ||
		//	(pairHeader.actors[1] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Net"))
		//	{
		//	// ボールのコライダーを取得
		//	physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
		//	if (ballCollider)
		//	{
		//		// ボールの形状を取得
		//		physx::PxShape* ballShape;
		//		ballCollider->getShapes(&ballShape, 1);

		//		// ボールのマテリアルを取得
		//		physx::PxMaterial* ballMaterial;
		//		ballShape->getMaterials(&ballMaterial, 1);

		//		// ボールの反発係数を変更
		//		ballMaterial->setRestitution(0.1f);

		//	}
		//}

		//// バットとの衝突時
		//if ((pairHeader.actors[0] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[1] == Player::Instance().GetBatCollider()) ||
		//	(pairHeader.actors[1] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[0] == Player::Instance().GetBatCollider()))
		//{
		//	physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
		//	if (ballCollider)
		//	{
		//		physx::PxShape* ballShape;
		//		ballCollider->getShapes(&ballShape, 1);

		//		physx::PxMaterial* ballMaterial;
		//		ballShape->getMaterials(&ballMaterial, 1);

		//		// バットとの衝突時に反発係数を元に戻す
		//		ballMaterial->setRestitution(0.5f);
		//	}
		//}
	}
}

//// ボックスコライダーかどうかを判定
//bool Physics::IsBoxCollider(physx::PxActor* actor)
//{
//	for (const auto& boxCollider : stage::Instance().GetBoxColliders())
//	{
//		if (actor == boxCollider)
//		{
//			return true;
//		}
//	}
//	return false;
//}
