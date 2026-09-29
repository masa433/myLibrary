#include "catcher.h"
#include "Ball.h"
#include "Graphics.h"


void Catcher::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//キャッチャーのモデルを読み込む
	catcherModel = std::make_shared<gltf_model>(device, ".\\resources\\catcher\\catcher.glb");
	position = { 0.0f, 0.0f, -2.0f };
	angle = { 0.0f, 0.0f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };

	animatedNodes = catcherModel->nodes;

	//キャッチャーミットのモデルを読み込む
	catcherMitt = std::make_unique<gltf_model>(device, ".\\resources\\object\\glove.glb");
	mittPosition = { 0.0f, 0.05f, 0.0f };
	mittAngle = { 0.5f, -1.0f, -0.3f };
	mittScale = { 1.0f, 1.0f, 1.0f };
	
}

void Catcher::Uninitialize()
{
	catcherModel.reset();
}

void Catcher::Update(float elapsedTime)
{

	UpdateTransform();
	AttachMittToHand();

	catcherModel->animate(0, elapsedTime, animatedNodes);

	const DirectX::XMFLOAT3& ballPosition = Ball::Instance().GetWorldPosition();
	UpdateLookAt(ballPosition);

}

void Catcher::UpdateLookAt(const DirectX::XMFLOAT3& targetPosition)
{
	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) };
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z) };
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(position.x, position.y, position.z) };
	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, S * R * T);

	// 首ノードのインデックスを取得
	int neck_joint_index = catcherModel->GetNodeIndex("mixamorig:Head");
	if (neck_joint_index < 0) return; // 首ノードが存在しない場合は処理しない

	gltf_model::node& node = animatedNodes.at(neck_joint_index);

	// 首ノードのグローバル空間での位置を取得
	DirectX::XMFLOAT4 joint_position = { node.global_transform._41, node.global_transform._42, node.global_transform._43, 1.0f }; // global space

	// ターゲット位置を取得（ボールの位置）
	DirectX::XMFLOAT4 target_position = { targetPosition.x, targetPosition.y, targetPosition.z, 1.0f }; // world space

	// ターゲット位置をグローバル空間に変換
	DirectX::XMStoreFloat4(&target_position, DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&target_position), DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&world))));

	// ターゲットまでのベクトルを計算（グローバル空間）
	DirectX::XMFLOAT3 to_target = {
		target_position.x - joint_position.x,
		target_position.y - joint_position.y,
		target_position.z - joint_position.z
	};

	// グローバル空間での前方向
	DirectX::XMFLOAT3 forward = { 0, 0, 1 }; // global space

	// グローバル空間からボーン空間に変換
	DirectX::XMMATRIX inverse_global_transform = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&node.global_transform));
	DirectX::XMStoreFloat3(&to_target, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&to_target), inverse_global_transform));
	DirectX::XMStoreFloat3(&forward, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&forward), inverse_global_transform));

	
	// 回転軸と回転角を計算
	DirectX::XMVECTOR axis = DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&forward), DirectX::XMLoadFloat3(&to_target));
	float angle = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenVectors(DirectX::XMLoadFloat3(&forward), DirectX::XMLoadFloat3(&to_target)));
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(DirectX::XMVector3Normalize(axis), angle);
	
	DirectX::XMStoreFloat4x4(&node.global_transform, rotation * DirectX::XMLoadFloat4x4(&node.global_transform));

	
	// 子ノードのグローバル行列を再帰的に更新
	std::function<void(int, int)> traverse = [&](int parent_index, int node_index)
		{
			gltf_model::node& node = animatedNodes.at(node_index);
			if (parent_index > -1)
			{
				DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
				DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
				DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);
				DirectX::XMStoreFloat4x4(&node.global_transform, S * R * T * DirectX::XMLoadFloat4x4(&animatedNodes.at(parent_index).global_transform));
			}
			for (int child_index : node.children)
			{
				traverse(node_index, child_index);
			}
		};
	traverse(-1, neck_joint_index);
}

void Catcher::AttachMittToHand()
{
	//右手ボーン名を取得
	const char* rightHandBoneName = "mixamorig:RightHand";

	//キャッチャーミットのローカル行列を計算
	// キャッチャーミットの位置、角度、スケールを更新
	DirectX::XMMATRIX mittS = DirectX::XMMatrixScaling(mittScale.x, mittScale.y, mittScale.z);
	DirectX::XMMATRIX mittR = DirectX::XMMatrixRotationRollPitchYaw(mittAngle.x, mittAngle.y, mittAngle.z);
	DirectX::XMMATRIX mittT = DirectX::XMMatrixTranslation(mittPosition.x, mittPosition.y, mittPosition.z);

	DirectX::XMMATRIX mittTransformMatrix = mittS * mittR * mittT;

	//キャラクターモデルから左手ボーンのインデックスを取得
	for(const gltf_model::node& node : animatedNodes)
	{
		if(node.name == rightHandBoneName)
		{
			// 右手ノードの行列を取得
			DirectX::XMMATRIX rightHandMatrix = DirectX::XMLoadFloat4x4(&node.global_transform);

			// キャッチャーのワールド行列を取得
			DirectX::XMMATRIX catcherWorldMatrix = DirectX::XMLoadFloat4x4(&transform);

			// キャッチャーミットのワールド行列を計算
			DirectX::XMMATRIX mittWorldMatrix = mittTransformMatrix * rightHandMatrix * catcherWorldMatrix;

			// キャッチャーミットの行列を保存（mittTransform に保存）
			DirectX::XMStoreFloat4x4(&mittTransform, mittWorldMatrix);
			break;
		}
	}
	

}


void Catcher::Render(const RenderContext& rc, ModelRenderer* renderer, FrustumCulling* frustumCulling)
{
	
	if (catcherModel)
	{
		bool isVisible = true;

		if (frustumCulling)
		{
			const auto& sphere = catcherModel->GetBoundingSphere();
			isVisible = frustumCulling->IsTransformedSphereVisible(sphere.center, sphere.radius, transform);
		}

		if (isVisible)
		{
			// キャッチャーのモデルをレンダリングする処理をここに追加
			catcherModel->render_batched(rc.deviceContext, transform, animatedNodes);
		}
	}
	if(catcherMitt)
	{
		bool isVisible = true;

		if (frustumCulling)
		{
			const auto& sphere = catcherMitt->GetBoundingSphere();
			isVisible = frustumCulling->IsTransformedSphereVisible(sphere.center, sphere.radius, mittTransform);
		}

		if (isVisible)
		{
			// キャッチャーミットのモデルをレンダリングする処理をここに追加
			catcherMitt->render_batched(rc.deviceContext, mittTransform, {});
		}
	}
}

void Catcher::DrawGUI()
{
	// キャッチャーの位置や角度をGUIで調整する処理をここに追加
	if(ImGui::CollapsingHeader("Catcher Settings"))
	{
		ImGui::DragFloat3("Position", &position.x);
		ImGui::DragFloat3("Angle", &angle.x);
		ImGui::DragFloat3("Scale", &scale.x);
	}

	if(ImGui::CollapsingHeader("Mitt Settings"))
	{
		ImGui::DragFloat3("Mitt Position", &mittPosition.x);
		ImGui::DragFloat3("Mitt Angle", &mittAngle.x);
		ImGui::DragFloat3("Mitt Scale", &mittScale.x);
	}
}

void Catcher::SaveToJson(json& j)
{
	j["catcherPosition"] = { position.x, position.y, position.z };
	j["catcherAngle"] = { angle.x, angle.y, angle.z };
	j["catcherScale"] = { scale.x, scale.y, scale.z };

	j["mittPosition"] = { mittPosition.x, mittPosition.y, mittPosition.z };
	j["mittAngle"] = { mittAngle.x, mittAngle.y, mittAngle.z };
	j["mittScale"] = { mittScale.x, mittScale.y, mittScale.z };
}

void Catcher::LoadFromJson(const json& j)
{
	if (j.contains("catcherPosition"))
	{
		auto pos = j["catcherPosition"];
		position = { pos[0], pos[1], pos[2] };
	}
	if (j.contains("catcherAngle"))
	{
		auto ang = j["catcherAngle"];
		angle = { ang[0], ang[1], ang[2] };
	}
	if (j.contains("catcherScale"))
	{
		auto scl = j["catcherScale"];
		scale = { scl[0], scl[1], scl[2] };
	}
	
	if(j.contains("mittPosition"))
	{
		auto pos = j["mittPosition"];
		mittPosition = { pos[0], pos[1], pos[2] };
	}
	if(j.contains("mittAngle"))
	{
		auto ang = j["mittAngle"];
		mittAngle = { ang[0], ang[1], ang[2] };
	}
	if(j.contains("mittScale"))
	{
		auto scl = j["mittScale"];
		mittScale = { scl[0], scl[1], scl[2] };
	}
}