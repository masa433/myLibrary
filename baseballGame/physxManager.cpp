#include <algorithm>
#include "Misc.h"
#include "Graphics.h"
#include "physxManager.h"

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
		pxSceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

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

			DirectX::XMFLOAT4 color(0.0f, 1.0f, 0.0f, 0.3f);
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
