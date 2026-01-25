#include "pch.h"
#include "Pitcher.h"

void Pitcher::Initialize()
{
	pitcher = std::make_unique<Model>("Data/Model/pitcher/pitcher.mdl");

	scale.x = scale.y = scale.z = 0.05f;
	position = { 20.5f, 0.0f, 0.0f };
	angle.y = DirectX::XMConvertToRadians(-90.0f);

	ball = std::make_unique<Model>("Data/Model/ball/ball.gltf");
	ballScale = { 0.05f,0.05f,0.05f };
	ballPosition = { 0.0f, -3.0f,5.0f };
	ballAngle = { 0.0f, 2.0f, 0.0f };

	SetPitchingState();
}

void Pitcher::Finalize()
{
	
}

void Pitcher::Update(float elapsedTime)
{
	// 更新処理
	switch (state)
	{
	case Pitcher::State::Throwing:
		UpdatePitchingState(elapsedTime);
		break;
	default:
		break;
	}

	// モデル行列更新
	pitcher->UpdateTransform();

	ball->UpdateTransform();

	// ピッチング状態の更新処理
	pitcher->UpdateAnimation(elapsedTime);

	const char* handName = "mixamorig:RightHandMiddle1";

	// バットのローカル行列を計算（バット専用の変数を使用）
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(ballScale.x, ballScale.y, ballScale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(ballAngle.x, ballAngle.y, ballAngle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(ballPosition.x, ballPosition.y, ballPosition.z);
	DirectX::XMMATRIX ballLocalMatrix = S * R * T;

	for (const Model::Node& node : pitcher->GetNodes())
	{
		if (strcmp(node.name, handName) == 0)
		{
			// 右手ノードの行列を取得
			DirectX::XMMATRIX rightHandMatrix = DirectX::XMLoadFloat4x4(&node.globalTransform);

			// プレイヤーのワールド行列を取得
			DirectX::XMMATRIX playerWorldMatrix = DirectX::XMLoadFloat4x4(&transform);

			// ボールのワールド行列を計算
			DirectX::XMMATRIX ballWorldMatrix = ballLocalMatrix * rightHandMatrix * playerWorldMatrix;

			// ボールの行列を保存（ballTransform に保存）
			DirectX::XMStoreFloat4x4(&ballTransform, ballWorldMatrix);
			// ボーンが見つかったらループを抜ける
			break;
		}
	}

	UpdateVelocity(elapsedTime);

	// オブジェクト行列を更新
	UpdateTransform();

	
}

void Pitcher::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	// 描画処理
	renderer->Render(rc, transform, pitcher.get(), ShaderId::Lambert);
	renderer->Render(rc, ballTransform, ball.get(), ShaderId::Lambert);
}

void Pitcher::DrawImGui()
{
	

	if(ImGui::Begin("Pitcher Transform"))
	{
		ImGui::DragFloat3("Position", &position.x, 0.1f);
		ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat3("Rotation", &angle.x, 1.0f, 0.0f, 360.0f);
	
		ImGui::Separator();
		ImGui::Text("Ball Transform");
		ImGui::DragFloat3("BallPosition", &ballPosition.x);
		ImGui::DragFloat3("BallAngle", &ballAngle.x);
		ImGui::DragFloat3("BallScale", &ballScale.x);
		ImGui::End();
	}
}
void Pitcher::SetPitchingState()
{
	state = State::Throwing;
	pitcher->PlayAnimation(Animation::Pitching, true);
}

void Pitcher::UpdatePitchingState(float elapsedTime)
{
	
}
