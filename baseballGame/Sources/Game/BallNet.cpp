#include "BallNet.h"
#include "Graphics.h"
#include "imgui.h"

void BallNet::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	// ネットのモデルを読み込む
	for (int i = 0; i < NET_COUNT; ++i)
	{
		netModels2[i] = std::make_unique<gltf_model>(device, ".\\resources\\field\\net.glb");
		netModels2[i]->build_static_batches(device);
	}
	
	// ホームラン判定用トリガーの作成
	{

		physx::PxPhysics* physics = Physics::Instance().GetPhysics();
		physx::PxScene* scene = Physics::Instance().GetScene();

		netColliderMaterial = physics->createMaterial(0.5f, 0.5f, 0.3f); // 摩擦係数と反発係数を設定

		for(int i = 0; i < NET_COUNT; ++i)
		{
			physx::PxQuat rotation =
				physx::PxQuat(netColliderAngles[i].x, physx::PxVec3(1, 0, 0)) *
				physx::PxQuat(netColliderAngles[i].y, physx::PxVec3(0, 1, 0)) *
				physx::PxQuat(netColliderAngles[i].z, physx::PxVec3(0, 0, 1));

			physx::PxTransform transform(physx::PxVec3(netColliderPositions[i].x, netColliderPositions[i].y, netColliderPositions[i].z), rotation);
			netCollider[i] = physics->createRigidStatic(transform);
			physx::PxBoxGeometry boxGeometry(netColliderScales[i].x / 2.0f, netColliderScales[i].y / 2.0f, netColliderScales[i].z / 2.0f);
			physx::PxShape* shape = physics->createShape(boxGeometry, *netColliderMaterial);
			//コライダー名を設定
			netColliderNames[i] = "NetCollider" + std::to_string(i);
			netCollider[i]->setName(netColliderNames[i].c_str());
			
			netCollider[i]->attachShape(*shape);
			shape->release(); // shapeは不要になったので解放
			scene->addActor(*netCollider[i]);
		}
	}

}

void BallNet::Uninitialize()
{
	// ネットのモデルを解放
	for (int i = 0; i < NET_COUNT; ++i)
	{
		netModels2[i].reset();

		netCollider[i]->release();
		netCollider[i] = nullptr;
	}
	// ネットの物理マテリアルを解放
	if (netColliderMaterial)
	{
		netColliderMaterial->release();
		netColliderMaterial = nullptr;
	}
	
}

void BallNet::Update(float elapsedTime)
{
	
}

void BallNet::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	std::vector<DirectX::XMFLOAT4X4> visibleNetTransforms;
	visibleNetTransforms.reserve(NET_COUNT);// 4箇所のネットの変換行列を格納するベクターを確保

	// ネットを4箇所配置
	for (int i = 0; i < NET_COUNT; i++)
	{
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(NetScales[i].x, NetScales[i].y, NetScales[i].z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(NetAngles[i].x, NetAngles[i].y, NetAngles[i].z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			NetPositions[i].x, NetPositions[i].y, NetPositions[i].z);
		
		DirectX::XMStoreFloat4x4(&netTransform, S * R * T);
		
		{
			//netModels2[i]->render_batched(rc.deviceContext, netTransform, {});
			visibleNetTransforms.push_back(netTransform); // ネットの変換行列を格納
		}
	}

	if (!visibleNetTransforms.empty() && isInstancingEnabled)
	{
		netModels2[0]->render_batched_instanced(rc.deviceContext, visibleNetTransforms);
	}
	else if (!isInstancingEnabled)
	{
		for (const auto& netTransform : visibleNetTransforms)
		{
			netModels2[0]->render_batched(rc.deviceContext, netTransform, {});
		}
	}

}

void BallNet::DrawGUI()
{
	if(ImGui::CollapsingHeader("BallNetInfo"))
	{
		ImGui::Checkbox("Enable Instancing", &isInstancingEnabled);

		//位置やサイズの設定
		for (int i = 0; i < NET_COUNT; ++i)
		{
			
			ImGui::PushID(i);
			ImGui::Text("Net %d", i + 1);
			ImGui::DragFloat3("Position", &NetPositions[i].x, 0.1f);
			ImGui::DragFloat3("Scale", &NetScales[i].x, 0.1f);
			ImGui::DragFloat3("Angle", &NetAngles[i].x, 0.1f);
			ImGui::PopID();
		}
	}

	if(ImGui::CollapsingHeader("NetColliderInfo"))
	{
		for (int i = 0; i < NET_COUNT; ++i)
		{
			ImGui::PushID(i);
			ImGui::Text("Net Collider %d", i + 1);
			ImGui::DragFloat3("Collider Position", &netColliderPositions[i].x, 0.1f);
			ImGui::DragFloat3("Collider Scale", &netColliderScales[i].x, 0.1f);
			ImGui::DragFloat3("Collider Angle", &netColliderAngles[i].x, 0.1f);
			ImGui::PopID();

			if (netCollider[i])
			{
				// 位置の更新
				physx::PxTransform transform(physx::PxVec3(netColliderPositions[i].x, netColliderPositions[i].y, netColliderPositions[i].z), 
					physx::PxQuat(netColliderAngles[i].x, physx::PxVec3(1, 0, 0)) *
					physx::PxQuat(netColliderAngles[i].y, physx::PxVec3(0, 1, 0)) *
					physx::PxQuat(netColliderAngles[i].z, physx::PxVec3(0, 0, 1)));
				netCollider[i]->setGlobalPose(transform);
				// サイズの更新
				physx::PxShape* shape = nullptr;
				netCollider[i]->getShapes(&shape, 1);
				if (shape)
				{
					shape->setGeometry(physx::PxBoxGeometry(netColliderScales[i].x / 2.0f, netColliderScales[i].y / 2.0f, netColliderScales[i].z / 2.0f));
				}
			}
		}
	}
}

void BallNet::SaveToJson(json& j)
{
	for (int i = 0; i < NET_COUNT; ++i)
	{
		j["nets"][i]["position"] = { NetPositions[i].x, NetPositions[i].y, NetPositions[i].z };
		j["nets"][i]["scale"] = { NetScales[i].x, NetScales[i].y, NetScales[i].z };
		j["nets"][i]["angle"] = { NetAngles[i].x, NetAngles[i].y, NetAngles[i].z };


		// ネットのコライダー情報も保存
		j["nets"][i]["colliderPosition"] = { netColliderPositions[i].x, netColliderPositions[i].y, netColliderPositions[i].z };
		j["nets"][i]["colliderScale"] = { netColliderScales[i].x, netColliderScales[i].y, netColliderScales[i].z };
		j["nets"][i]["colliderAngle"] = { netColliderAngles[i].x, netColliderAngles[i].y, netColliderAngles[i].z };
	}
	j["instancingEnabled"] = isInstancingEnabled;
}

void BallNet::LoadFromJson(const json& j)
{
	for (int i = 0; i < NET_COUNT; ++i)
	{
		if (j.contains("nets") && j["nets"].is_array())
		{
			const auto& netData = j["nets"][i];
			if (netData.contains("position"))
			{
				NetPositions[i] = DirectX::XMFLOAT3(netData["position"][0], netData["position"][1], netData["position"][2]);
			}
			if (netData.contains("scale"))
			{
				NetScales[i] = DirectX::XMFLOAT3(netData["scale"][0], netData["scale"][1], netData["scale"][2]);
			}
			if (netData.contains("angle"))
			{
				NetAngles[i] = DirectX::XMFLOAT3(netData["angle"][0], netData["angle"][1], netData["angle"][2]);
			}

			// ネットのコライダー情報も読み込む
			if (netData.contains("colliderPosition"))
			{
				netColliderPositions[i] = DirectX::XMFLOAT3(netData["colliderPosition"][0], netData["colliderPosition"][1], netData["colliderPosition"][2]);
			}
			if (netData.contains("colliderScale"))
			{
				netColliderScales[i] = DirectX::XMFLOAT3(netData["colliderScale"][0], netData["colliderScale"][1], netData["colliderScale"][2]);
			}
			if (netData.contains("colliderAngle"))
			{
				netColliderAngles[i] = DirectX::XMFLOAT3(netData["colliderAngle"][0], netData["colliderAngle"][1], netData["colliderAngle"][2]);
			}

			//Physxのコライダーの位置やサイズを更新
			if (netCollider[i])
			{
				physx::PxTransform transform(physx::PxVec3(netColliderPositions[i].x, netColliderPositions[i].y, netColliderPositions[i].z), 
					physx::PxQuat(netColliderAngles[i].x, physx::PxVec3(1, 0, 0)) *
					physx::PxQuat(netColliderAngles[i].y, physx::PxVec3(0, 1, 0)) *
					physx::PxQuat(netColliderAngles[i].z, physx::PxVec3(0, 0, 1)));
				netCollider[i]->setGlobalPose(transform);
				physx::PxShape* shape = nullptr;
				netCollider[i]->getShapes(&shape, 1);
				if (shape)
				{
					shape->setGeometry(physx::PxBoxGeometry(netColliderScales[i].x / 2.0f, netColliderScales[i].y / 2.0f, netColliderScales[i].z / 2.0f));
				}
			}
		}
	}
	if (j.contains("instancingEnabled"))
	{
		isInstancingEnabled = j["instancingEnabled"];
	}
}