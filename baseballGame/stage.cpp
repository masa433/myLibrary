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
	// 位置、スケール、回転の初期化
	position = { 0.0f, 0.0f, 0.0f };
	scale = { 0.01f, 0.01f, 0.01f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f };

	//静的剛体の作成
	{
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxScene* pxScene = Physics::Instance().GetScene();

		// Ground用のマテリアル（よく跳ねる）
		physx::PxMaterial* groundMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.2f);

		// Stand用のマテリアル（ほぼ跳ねない）
		physx::PxMaterial* standMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.0f);

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

	//if (ImGui::CollapsingHeader("Box Colliders"))
	//{
	//	for (size_t i = 0; i < boxColliders.size(); ++i)
	//	{
	//		physx::PxRigidStatic* boxCollider = boxColliders[i];
	//		physx::PxTransform transform = boxCollider->getGlobalPose();

	//		// ボックスの位置を操作
	//		ImGui::DragFloat3(("Box Position " + std::to_string(i)).c_str(), &boxPositions[i].x, 0.1f);

	//		// ボックスのサイズを操作
	//		ImGui::DragFloat3(("Box Size " + std::to_string(i)).c_str(), &boxSizes[i].x, 0.1f);

	//		// 位置を更新
	//		transform.p = boxPositions[i];
	//		boxCollider->setGlobalPose(transform);

	//		// サイズを更新
	//		physx::PxShape* shape = nullptr;
	//		boxCollider->getShapes(&shape, 1);
	//		if (shape)
	//		{
	//			shape->setGeometry(physx::PxBoxGeometry(boxSizes[i]));
	//		}
	//	}
	//}

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
	renderer->Render(rc, transform, stand.get(), ShaderId::Phong);
	renderer->Render(rc, transform, ground.get(), ShaderId::ShadowMap);
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

	model.reset();
}

