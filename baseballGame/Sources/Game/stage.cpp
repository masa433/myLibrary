#include "stage.h"
#include "imgui.h"
#include "Graphics.h"

//std::vector<physx::PxRigidStatic*> stage::boxColliders;

// 初期化
void stage::initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	stand = std::make_unique<Model>(".\\resources\\field\\stand.mdl");
	ground = std::make_unique<Model>(".\\resources\\field\\ground.mdl");
	stand2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\stand.glb");
	ground2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\ground.glb");
	pole = std::make_unique<Model>(".\\resources\\field\\pole.mdl");
	pole2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\pole.glb");

	stand2->build_static_batches(device);
	ground2->build_static_batches(device);
	pole2->build_static_batches(device);


	// 位置、スケール、回転の初期化
	position = { 0.0f, 0.0f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f };

	hrTriggerPos = { 0.0f, 55.0f, 67.5f }; // トリガーの初期位置
	hrTriggerHalfExtents = { 67.0f, 55.0f, 0.5f }; // トリガーの半分のサイズ(XYZ)

	//静的剛体の作成
	{
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxScene* pxScene = Physics::Instance().GetScene();

		// Ground用のマテリアル（よく跳ねる）
		physx::PxMaterial* groundMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.3f);

		// Stand用のマテリアル（ほぼ跳ねない）
		physx::PxMaterial* standMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.0f);

		//Pole用のマテリアル（あまり跳ねない）
		physx::PxMaterial* poleMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.2f);

		DirectX::XMMATRIX Transform = DirectX::XMLoadFloat4x4(&transform);

		// Stand モデルのメッシュを処理
		const ModelResource* standResources = stand->GetResource();
		for (const ModelResource::Mesh& mesh : standResources->GetMeshes())
		{
			physx::PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = static_cast<physx::PxU32>(mesh.vertices.size());
			meshDesc.points.data = mesh.vertices.data();
			meshDesc.points.stride = sizeof(ModelResource::Vertex);
			meshDesc.triangles.count = static_cast<physx::PxU32>(mesh.indices.size() / 3);
			meshDesc.triangles.data = mesh.indices.data();
			meshDesc.triangles.stride = sizeof(UINT) * 3;

			physx::PxTolerancesScale pxTolerances;
			const physx::PxCookingParams cookingParams(pxTolerances);
			physx::PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc);

			const Model::Node& node = stand->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * Transform;
			physx::PxVec3 pxScale(
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[0])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[1])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[2]))
			);
			NodeTransform.r[0] = DirectX::XMVector3Normalize(NodeTransform.r[0]);
			NodeTransform.r[1] = DirectX::XMVector3Normalize(NodeTransform.r[1]);
			NodeTransform.r[2] = DirectX::XMVector3Normalize(NodeTransform.r[2]);

			DirectX::XMFLOAT4X4 nodeTransform;
			DirectX::XMStoreFloat4x4(&nodeTransform, NodeTransform);
			physx::PxTransform pxTransform(physx::PxMat44(
				physx::PxVec3(nodeTransform._11, nodeTransform._12, nodeTransform._13),
				physx::PxVec3(nodeTransform._21, nodeTransform._22, nodeTransform._23),
				physx::PxVec3(nodeTransform._31, nodeTransform._32, nodeTransform._33),
				physx::PxVec3(nodeTransform._41, nodeTransform._42, nodeTransform._43)
			));
			physx::PxRigidStatic* pxRigidBody = pxPhysics->createRigidStatic(pxTransform);
			_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create stand rigid body");

			physx::PxMeshScale pxMeshScale(pxScale);
			physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh, pxMeshScale);
			physx::PxShape* pxShape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *standMaterial);

			pxRigidBody->setName("Stand");

			pxScene->addActor(*pxRigidBody);

			actors.emplace_back(pxRigidBody);
			triangle_meshes.emplace_back(pxTriangleMesh);
		}

		// Ground モデルのメッシュを処理
		const ModelResource* groundResources = ground->GetResource();
		for (const ModelResource::Mesh& mesh : groundResources->GetMeshes())
		{
			physx::PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = static_cast<physx::PxU32>(mesh.vertices.size());
			meshDesc.points.data = mesh.vertices.data();
			meshDesc.points.stride = sizeof(ModelResource::Vertex);
			meshDesc.triangles.count = static_cast<physx::PxU32>(mesh.indices.size() / 3);
			meshDesc.triangles.data = mesh.indices.data();
			meshDesc.triangles.stride = sizeof(UINT) * 3;

			physx::PxTolerancesScale pxTolerances;
			const physx::PxCookingParams cookingParams(pxTolerances);
			physx::PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc);

			const Model::Node& node = ground->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * Transform;
			physx::PxVec3 pxScale(
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[0])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[1])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[2]))
			);
			NodeTransform.r[0] = DirectX::XMVector3Normalize(NodeTransform.r[0]);
			NodeTransform.r[1] = DirectX::XMVector3Normalize(NodeTransform.r[1]);
			NodeTransform.r[2] = DirectX::XMVector3Normalize(NodeTransform.r[2]);

			DirectX::XMFLOAT4X4 nodeTransform;
			DirectX::XMStoreFloat4x4(&nodeTransform, NodeTransform);
			physx::PxTransform pxTransform(physx::PxMat44(
				physx::PxVec3(nodeTransform._11, nodeTransform._12, nodeTransform._13),
				physx::PxVec3(nodeTransform._21, nodeTransform._22, nodeTransform._23),
				physx::PxVec3(nodeTransform._31, nodeTransform._32, nodeTransform._33),
				physx::PxVec3(nodeTransform._41, nodeTransform._42, nodeTransform._43)
			));
			physx::PxRigidStatic* pxRigidBody = pxPhysics->createRigidStatic(pxTransform);
			_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create ground rigid body");

			physx::PxMeshScale pxMeshScale(pxScale);
			physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh, pxMeshScale);
			physx::PxShape* pxShape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *groundMaterial);

			pxRigidBody->setName("Ground");

			pxScene->addActor(*pxRigidBody);

			actors.emplace_back(pxRigidBody);
			triangle_meshes.emplace_back(pxTriangleMesh);
		}

		// Pole モデルのメッシュを処理
		const ModelResource* poleResources = pole->GetResource();
		for (const ModelResource::Mesh& mesh : poleResources->GetMeshes())
		{
			physx::PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = static_cast<physx::PxU32>(mesh.vertices.size());
			meshDesc.points.data = mesh.vertices.data();
			meshDesc.points.stride = sizeof(ModelResource::Vertex);
			meshDesc.triangles.count = static_cast<physx::PxU32>(mesh.indices.size() / 3);
			meshDesc.triangles.data = mesh.indices.data();
			meshDesc.triangles.stride = sizeof(UINT) * 3;

			physx::PxTolerancesScale pxTolerances;
			const physx::PxCookingParams cookingParams(pxTolerances);
			physx::PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc);

			const Model::Node& node = pole->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * Transform;
			physx::PxVec3 pxScale(
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[0])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[1])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[2]))
			);
			NodeTransform.r[0] = DirectX::XMVector3Normalize(NodeTransform.r[0]);
			NodeTransform.r[1] = DirectX::XMVector3Normalize(NodeTransform.r[1]);
			NodeTransform.r[2] = DirectX::XMVector3Normalize(NodeTransform.r[2]);

			DirectX::XMFLOAT4X4 nodeTransform;
			DirectX::XMStoreFloat4x4(&nodeTransform, NodeTransform);
			physx::PxTransform pxTransform(physx::PxMat44(
				physx::PxVec3(nodeTransform._11, nodeTransform._12, nodeTransform._13),
				physx::PxVec3(nodeTransform._21, nodeTransform._22, nodeTransform._23),
				physx::PxVec3(nodeTransform._31, nodeTransform._32, nodeTransform._33),
				physx::PxVec3(nodeTransform._41, nodeTransform._42, nodeTransform._43)
			));
			physx::PxRigidStatic* pxRigidBody = pxPhysics->createRigidStatic(pxTransform);
			_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create pole rigid body");

			physx::PxMeshScale pxMeshScale(pxScale);
			physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh, pxMeshScale);
			physx::PxShape* pxShape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *poleMaterial);
			pxRigidBody->setName("Pole");

			pxScene->addActor(*pxRigidBody);

			actors.emplace_back(pxRigidBody);
			triangle_meshes.emplace_back(pxTriangleMesh);
		}



		// ホームラン判定用トリガーの作成
		{
			physx::PxMaterial* triggerMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.5f);
			physx::PxTransform triggerTransform(physx::PxVec3(hrTriggerPos.x, hrTriggerPos.y, hrTriggerPos.z));
			homeRunTrigger = pxPhysics->createRigidStatic(triggerTransform);

			physx::PxBoxGeometry triggerGeometry(physx::PxVec3(hrTriggerHalfExtents.x, hrTriggerHalfExtents.y, hrTriggerHalfExtents.z));
			physx::PxShape* triggerShape = physx::PxRigidActorExt::createExclusiveShape(*homeRunTrigger, triggerGeometry, *triggerMaterial);

			// 物理的な衝突を無効にし、トリガー（重なり判定）として設定する
			triggerShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
			triggerShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);

			homeRunTrigger->setName("HomeRunTrigger");
			pxScene->addActor(*homeRunTrigger);
		}
	}
}


// 更新
void stage::update(float elapsedTime)
{
#ifdef  USE_IMGUI
	if (ImGui::CollapsingHeader("Stage"))
	{
		ImGui::DragFloat3("Position", &position.x);
		ImGui::DragFloat3("Scale", &scale.x);
		ImGui::DragFloat3("Angle", &angle.x);
	}

	if (ImGui::CollapsingHeader("Home Run Trigger"))
	{
		ImGui::DragFloat3("Trigger Position", &hrTriggerPos.x, 0.5f);
		ImGui::DragFloat3("Trigger Half Extents (Size)", &hrTriggerHalfExtents.x, 0.5f);

		if (homeRunTrigger)
		{
			// 位置の更新
			physx::PxTransform transform(physx::PxVec3(hrTriggerPos.x, hrTriggerPos.y, hrTriggerPos.z));
			homeRunTrigger->setGlobalPose(transform);

			// サイズの更新
			physx::PxShape* shape = nullptr;
			homeRunTrigger->getShapes(&shape, 1);
			if (shape)
			{
				shape->setGeometry(physx::PxBoxGeometry(hrTriggerHalfExtents.x, hrTriggerHalfExtents.y, hrTriggerHalfExtents.z));
			}
		}
	}


#endif //  USE_IMGUI

	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX world = S * R * T;
	DirectX::XMStoreFloat4x4(&transform, world);

	//ボックスの位置とサイズを更新

	UpdateTransform();
}

void stage::render(const RenderContext& rc, ModelRenderer* renderer)
{
	//renderer->Render(rc, transform, stand.get(), ShaderId::ShadowMap);
	//renderer->Render(rc, transform, ground.get(), ShaderId::ShadowMap);
	stand2->render_batched(rc.deviceContext, transform, {});
	ground2->render_batched(rc.deviceContext, transform, {});
	pole2->render_batched(rc.deviceContext, transform, {});
}

// 終了
void stage::uninitialize()
{
	for(physx::PxTriangleMesh* pxTriangleMesh : triangle_meshes)
	{
		pxTriangleMesh->release();
	}

	physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
	physx::PxScene* pxScene = Physics::Instance().GetScene();

	if (actors.size() > 0) 
	{
		pxScene->removeActors(actors.data(), static_cast<physx::PxU32>(actors.size()));
	}

	/*for (auto* boxCollider : boxColliders)
	{
		boxCollider->release();
	}
	boxColliders.clear();*/

	stand2.reset();
	ground2.reset();
	pole2.reset();
}

