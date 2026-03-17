#include "stage.h"
#include "imgui.h"
#include "Graphics.h"

//std::vector<physx::PxRigidStatic*> stage::boxColliders;

// 初期化
void stage::initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	model = std::make_unique<Model>(".\\resources\\field\\stadium2.mdl");
	// 位置、スケール、回転の初期化
	position = { 0.0f, 0.0f, 0.0f };
	scale = { 0.01f, 0.01f, 0.01f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f};


	//静的剛体の作成
	{
		// ボックスコライダーの生成
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxScene* pxScene = Physics::Instance().GetScene();
		physx::PxMaterial* pxMaterial = Physics::Instance().GetMaterial();

		//

		//// ボックスコライダーを複数作成
		//boxPositions = { {0.0f, 3.0f, 50.0f},{0.0f, 3.0f, 50.0f} }; // ボックスの位置

		//boxSizes = { {40.0f, 20.0f, 45.0f}, {50.0f, 20.0f, 30.0f} }; // ボックスのサイズ

		//for (size_t i = 0; i < boxPositions.size(); ++i)
		//{
		//	physx::PxTransform boxTransform(boxPositions[i]);
		//	physx::PxRigidStatic* boxCollider = pxPhysics->createRigidStatic(boxTransform);
		//	physx::PxShape* boxShape = pxPhysics->createShape(physx::PxBoxGeometry(boxSizes[i]), *pxMaterial);

		//	// フィルターデータを設定
		//	physx::PxFilterData filterData;
		//	filterData.word0 = 1 << 1; // ボックスコライダー用のグループ
		//	boxShape->setSimulationFilterData(filterData);

		//	boxCollider->attachShape(*boxShape);
		//	pxScene->addActor(*boxCollider);
		//	boxColliders.push_back(boxCollider);

		//	boxShape->release(); // 解放
		//}


		pxMaterial->setRestitution(0.0f);// 反発係数を設定
		pxMaterial->setDynamicFriction(1.0f);// 動的摩擦係数を設定
		pxMaterial->setStaticFriction(1.0f);// 静止摩擦係数を設定

		const ModelResource* resources = model->GetResource();

		DirectX::XMMATRIX Transform = DirectX::XMLoadFloat4x4(&transform);

		for (const ModelResource::Mesh& mesh : resources->GetMeshes()) 
		{
			// 三角形メッシュの作成
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

			//静的剛体の作成
			const Model::Node& node = model->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z); // スケール行列を作成
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z); // 回転行列を作成
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
			_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create rigid body");

			//静的剛体にメッシュ形状を関連付ける
			physx::PxMeshScale pxMeshScale(pxScale);
			physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh, pxMeshScale);
			physx::PxShape* pxShape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *pxMaterial);

			pxRigidBody->setName("Stage");

			//シーンに剛体を追加
			pxScene->addActor(*pxRigidBody);

			//削除用にポインタを保持
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

	renderer->Render(rc, transform, model.get(), ShaderId::ShadowMap);
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

