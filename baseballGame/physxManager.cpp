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
			// 衝突が既に処理されている場合はスキップ
			if (Pitcher::Instance().GetHasCollided())
			{
			    continue; // または continue; ループ内なら
			}

			Pitcher::Instance().SetHasCollided(true);

			physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
			physx::PxRigidDynamic* batCollider = Player::Instance().GetBatCollider();

			if (ballCollider && batCollider)
			{
				// バット衝突時のボール位置を保存
				physx::PxVec3 hitPos = ballCollider->getGlobalPose().p;
				Pitcher::Instance().SetBallHitPosition({ hitPos.x, hitPos.y, hitPos.z });

				// ===== 物理定数 =====
				const float BALL_RADIUS = 0.037f;
				const float PI = 3.14159265359f;

				// ===== 1. 衝突前の情報取得 =====
				physx::PxVec3 ballVelocity = ballCollider->getLinearVelocity();
				physx::PxVec3 batVelocity = batCollider->getLinearVelocity();

				// ===== ボール初速の上限設定（180 km/h） =====
				float ballSpeed = ballVelocity.magnitude();
				if (ballSpeed > 50.0f)  // 180 km/h ≈ 50.0 m/s
				{
					ballVelocity = (ballVelocity / ballSpeed) * 50.0f;
					ballSpeed = 50.0f;
				}

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
					float targetSpeed = (std::min)(estimatedExitVelocity, physSpeed * 0.8f);
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

				// ===== 6. 速度をキューに登録 =====
				{
					// 制限後の初速を保存
					float limitedBallSpeedKmh = ballVelocity.magnitude() * 3.6f;

					std::lock_guard<std::mutex> lock(queueMutex);
					velocityUpdateQueue.push([ballCollider, newBallVelocity, spinAxis, angularVelocityRadPerSec,
						limitedBallSpeedKmh, batSpeed, launchAngleDeg]() {
							ballCollider->setLinearVelocity(newBallVelocity);
							ballCollider->setAngularVelocity(spinAxis * angularVelocityRadPerSec);
							ballCollider->setLinearDamping(0.0f);
							ballCollider->setAngularDamping(0.0f);

							// ===== デバッグ出力（速度設定直後） =====
#ifdef _DEBUG
							float exitVelocityKmh = newBallVelocity.magnitude() * 3.6f;
							float spinRpm = (angularVelocityRadPerSec * 60.0f) / (2.0f * 3.14159265359f);

							char debugMessage[512];
							snprintf(debugMessage, sizeof(debugMessage),
								"=== バット衝突 ===\nボール初速: %.1f km/h\nバット速度: %.1f km/h\n打球速度: %.1f km/h\n打球角度: %.1f°\n回転: %.0f rpm\n",
								limitedBallSpeedKmh, batSpeed * 3.6f, exitVelocityKmh, launchAngleDeg, spinRpm);
							OutputDebugStringA(debugMessage);
#endif
						});
				}

				
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
							// フェンス衝突後かどうかで減衰率を変更
							float dampingFactor;
							if (Pitcher::Instance().GetHasCollidedWithFence())
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
				}
			}

		//ボールとフェンスの衝突を検知
		if ((pairHeader.actors[0] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[1]->getName() == "Stand") ||
			(pairHeader.actors[1] == Pitcher::Instance().GetBallCollider() && pairHeader.actors[0]->getName() == "Stand"))
		{
			Pitcher::Instance().SetHasCollided(true); // 衝突フラグを設定

			// フェンスとの衝突が初回のみ飛距離を出力
			if (!Pitcher::Instance().GetHasCollidedWithFence())
			{
				Pitcher::Instance().SetHasCollidedWithFence(true);

				// ボールのコライダーを取得
				physx::PxRigidDynamic* ballCollider = Pitcher::Instance().GetBallCollider();
				if (ballCollider)
				{
					// ボールの現在位置を取得（フェンス衝突位置）
					physx::PxVec3 ballFencePosition = ballCollider->getGlobalPose().p;

					// バット衝突時のボール位置を取得
					DirectX::XMFLOAT3 ballHitPos = Pitcher::Instance().GetBallHitPosition();

					// 実測飛距離を計算（3次元）
					float distanceX = ballFencePosition.x - ballHitPos.x;
					float distanceY = ballFencePosition.y - ballHitPos.y;
					float distanceZ = ballFencePosition.z - ballHitPos.z;
					float measuredDistance = sqrtf(distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ);
					float horizontalDistance = sqrtf(distanceX * distanceX + distanceZ * distanceZ);

					// ===== 推定飛距離の計算（スタンドがなくグラウンドに着地していたら何メートルか） =====
					physx::PxVec3 ballVelocity = ballCollider->getLinearVelocity();
					float exitVelocity = ballVelocity.magnitude();

					// 打球角度を計算（速度ベクトルから）
					float launchAngle = std::atan2(ballVelocity.y,
						sqrtf(ballVelocity.x * ballVelocity.x + ballVelocity.z * ballVelocity.z));

					// 投射体運動の計算
					// グラウンドレベル（y = 0）に着地するまでの水平距離を計算
					const float GRAVITY = 9.81f;
					const float GROUND_LEVEL = 0.0f;
					float estimatedDistance = 0.0f;

					if (exitVelocity > 0.1f)
					{
						// 初期高さ（ボール衝突時のY座標）
						float initialHeight = ballHitPos.y;

						// 着地時間を計算: y = y0 + v_y*t - 0.5*g*t^2
						// 0 = initialHeight + (exitVelocity * sin(launchAngle)) * t - 0.5 * GRAVITY * t^2
						// 整理すると: 0.5 * g * t^2 - v_y * t - y0 = 0
						float v_y = exitVelocity * sinf(launchAngle);
						float a = 0.5f * GRAVITY;
						float b = -v_y;
						float c = -initialHeight;

						// 二次方程式の解
						float discriminant = b * b - 4.0f * a * c;
						if (discriminant >= 0.0f)
						{
							float t = (-b + sqrtf(discriminant)) / (2.0f * a); // 正の解を取得

							if (t > 0.0f)
							{
								// 水平速度を計算
								float v_horizontal = exitVelocity * cosf(launchAngle);

								// 着地までの水平距離を計算
								estimatedDistance = v_horizontal * t;
							}
						}
					}

					// 総飛距離 = 水平飛距離 + 推定飛距離
					// （推定飛距離はスタンドがなかった場合にグラウンドに着地するまでの距離）
					float totalDistance = horizontalDistance + estimatedDistance;


					// 飛距離を出力
					char debugMessage[768];
					snprintf(
						debugMessage,
						sizeof(debugMessage),
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
		

	}
}

