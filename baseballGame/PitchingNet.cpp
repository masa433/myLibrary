#include "PitchingNet.h"
#include "Graphics.h"
#include "imgui.h"
#include "ModelRenderer.h"

void PitchingNet::Initialize()
{
	net = std::make_unique<Model>(".\\resources\\net\\net.mdl");
	position = { 0.0f, 0.0f, 15.10f };
	scale = { 0.012f,0.012f,0.01f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f};

	//静的剛体の作成
	{
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxScene* pxScene = Physics::Instance().GetScene();
		physx::PxMaterial* pxMaterial = Physics::Instance().GetMaterial();

		pxPhysics->createMaterial(
			1.0f,// 静止摩擦係数
			1.0f,// 動摩擦係数
			0.000000001f);// 反発係数

		const ModelResource* resources = net->GetResource();

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
			const Model::Node& node = net->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z); // スケール行列を作成
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z); // 回転行列を作成
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z); // 平行移動行列を作成
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * T * Transform;
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

			//シーンに剛体を追加
			pxScene->addActor(*pxRigidBody);

			//削除用にポインタを保持
			actors.emplace_back(pxRigidBody);
			triangle_meshes.emplace_back(pxTriangleMesh);
		}
	}
}

void PitchingNet::Uninitialize()
{
	for (physx::PxTriangleMesh* pxTriangleMesh : triangle_meshes)
	{
		pxTriangleMesh->release();
	}

	physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
	physx::PxScene* pxScene = Physics::Instance().GetScene();

	if (actors.size() > 0)
	{
		pxScene->removeActors(actors.data(), static_cast<physx::PxU32>(actors.size()));
	}

	net.reset();
}

void PitchingNet::Update(float elapsedTime)
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX W = S * R * T;
	DirectX::XMStoreFloat4x4(&transform, W);


	UpdateTransform();
}

void PitchingNet::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, net.get(), ShaderId::Lambert);
}

void PitchingNet::DrawGUI()
{
	if (ImGui::Begin(u8"ピッチングネット"))
	{
		ImGui::DragFloat3("Position", &position.x);
		ImGui::DragFloat3("Scale", &scale.x);
		ImGui::DragFloat3("Angle", &angle.x);
	}
	ImGui::End();
}