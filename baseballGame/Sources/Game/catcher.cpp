#include "catcher.h"
#include "Graphics.h"

void Catcher::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//キャッチャーのモデルを読み込む
	catcherModel = std::make_shared<gltf_model>(device, ".\\resources\\catcher\\catcher.glb");
	catcherPosition = { 0.0f, 0.0f, -2.0f };
	catcherAngle = { 0.0f, 0.0f, 0.0f };
	catcherScale = { 1.0f, 1.0f, 1.0f };

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
	// キャッチャーの位置、角度、スケールを更新
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(catcherScale.x, catcherScale.y, catcherScale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(catcherAngle.x, catcherAngle.y, catcherAngle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(catcherPosition.x, catcherPosition.y, catcherPosition.z);

	DirectX::XMMATRIX transformMatrix = S * R * T;
	DirectX::XMStoreFloat4x4(&catcherTransform, transformMatrix);

	
	AttachMittToHand();
	UpdateTransform();
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
			DirectX::XMMATRIX catcherWorldMatrix = DirectX::XMLoadFloat4x4(&catcherTransform);

			// キャッチャーミットのワールド行列を計算
			DirectX::XMMATRIX mittWorldMatrix = mittTransformMatrix * rightHandMatrix * catcherWorldMatrix;

			// キャッチャーミットの行列を保存（mittTransform に保存）
			DirectX::XMStoreFloat4x4(&mittTransform, mittWorldMatrix);
			break;
		}
	}
	

}


void Catcher::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	if (catcherModel)
	{
		// キャッチャーのモデルをレンダリングする処理をここに追加
		catcherModel->render_batched(rc.deviceContext, catcherTransform, animatedNodes);
	}
	if(catcherMitt)
	{
		// キャッチャーミットのモデルをレンダリングする処理をここに追加
		catcherMitt->render_batched(rc.deviceContext, mittTransform, {});
	}
}

void Catcher::DrawGUI()
{
	// キャッチャーの位置や角度をGUIで調整する処理をここに追加
	if(ImGui::CollapsingHeader("Catcher Settings"))
	{
		ImGui::DragFloat3("Position", &catcherPosition.x);
		ImGui::DragFloat3("Angle", &catcherAngle.x);
		ImGui::DragFloat3("Scale", &catcherScale.x);
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
	j["catcherPosition"] = { catcherPosition.x, catcherPosition.y, catcherPosition.z };
	j["catcherAngle"] = { catcherAngle.x, catcherAngle.y, catcherAngle.z };
	j["catcherScale"] = { catcherScale.x, catcherScale.y, catcherScale.z };

	j["mittPosition"] = { mittPosition.x, mittPosition.y, mittPosition.z };
	j["mittAngle"] = { mittAngle.x, mittAngle.y, mittAngle.z };
	j["mittScale"] = { mittScale.x, mittScale.y, mittScale.z };
}

void Catcher::LoadFromJson(const json& j)
{
	if (j.contains("catcherPosition"))
	{
		auto pos = j["catcherPosition"];
		catcherPosition = { pos[0], pos[1], pos[2] };
	}
	if (j.contains("catcherAngle"))
	{
		auto ang = j["catcherAngle"];
		catcherAngle = { ang[0], ang[1], ang[2] };
	}
	if (j.contains("catcherScale"))
	{
		auto scl = j["catcherScale"];
		catcherScale = { scl[0], scl[1], scl[2] };
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