#include "pch.h"
#include "Pitcher.h"

void Pitcher::Initialize()
{
	pitcher = std::make_unique<Model>("Data/Model/pitcher/pitcher.mdl");

	scale.x = scale.y = scale.z = 0.05f;
	position = { 20.5f, 0.0f, 0.0f };
	angle.y = DirectX::XMConvertToRadians(-90.0f);

	ball = std::make_unique<Model>("Data/Model/ball/ball.gltf");
	ballScale = { 0.05f, 0.05f, 0.05f };
	ballPosition = { 0.0f, -3.0f, 5.0f };
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

	// ボールが投げられていない場合は手に追従
	if (!isBallThrown)
	{
		const char* handName = "mixamorig:RightHandMiddle1";

		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(ballScale.x, ballScale.y, ballScale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(ballAngle.x, ballAngle.y, ballAngle.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(ballPosition.x, ballPosition.y, ballPosition.z);
		DirectX::XMMATRIX ballLocalMatrix = S * R * T;

		for (const Model::Node& node : pitcher->GetNodes())
		{
			if (strcmp(node.name, handName) == 0)
			{
				DirectX::XMMATRIX rightHandMatrix = DirectX::XMLoadFloat4x4(&node.globalTransform);
				DirectX::XMMATRIX playerWorldMatrix = DirectX::XMLoadFloat4x4(&transform);
				DirectX::XMMATRIX ballWorldMatrix = ballLocalMatrix * rightHandMatrix * playerWorldMatrix;

				DirectX::XMStoreFloat4x4(&ballTransform, ballWorldMatrix);

				// ボールのワールド座標を保存
				ballWorldPosition.x = ballTransform._41;
				ballWorldPosition.y = ballTransform._42;
				ballWorldPosition.z = ballTransform._43;

				break;
			}
		}
	}
	else
	{
		// ボールが投げられた後は物理演算で移動
		ballVelocity.y += gravity * elapsedTime; // 重力を適用

		ballWorldPosition.x += ballVelocity.x * elapsedTime;
		ballWorldPosition.y += ballVelocity.y * elapsedTime;
		ballWorldPosition.z += ballVelocity.z * elapsedTime;

		// ボールの回転を更新（追加）
		float rotationSpeed = -80.0f; // 回転速度（ラジアン/秒）
		ballWorldAngle.y += rotationSpeed * elapsedTime; // X軸回転
		//ballWorldAngle.z += rotationSpeed * 0.5f * elapsedTime; // Z軸回転（少し追加）

		// ballScale の再設定を削除（これが原因で大きさが変わっていた）
		// ballScale = { 0.05f, 0.05f, 0.05f }; // ← この行を削除

		// ボールのワールド行列を更新
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(ballWorldScale.x, ballWorldScale.y, ballWorldScale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(ballWorldAngle.x, ballWorldAngle.y, ballWorldAngle.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(ballWorldPosition.x, ballWorldPosition.y, ballWorldPosition.z);
		DirectX::XMMATRIX ballWorldMatrix = S * R * T;
		DirectX::XMStoreFloat4x4(&ballWorldTransform, ballWorldMatrix);

		// 地面に落ちたらリセット
		if (ballWorldPosition.y < -5.7f)
		{
			isBallThrown = false;
			SetPitchingState(); // アニメーションをリセット
		}
	}

	UpdateVelocity(elapsedTime);
	UpdateTransform();
	pitcher->UpdateTransform();
	ball->UpdateTransform();
	pitcher->UpdateAnimation(elapsedTime);
}

void Pitcher::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, pitcher.get(), ShaderId::Lambert);
	// ボールが投げられたかどうかで使用する行列を切り替え
	if (isBallThrown)
	{
		renderer->Render(rc, ballWorldTransform, ball.get(), ShaderId::Lambert); // 投げた後
	}
	else
	{
		renderer->Render(rc, ballTransform, ball.get(), ShaderId::Lambert); // 手に持っているとき
	}
}

void Pitcher::DrawImGui()
{
	if (ImGui::Begin("Pitcher Transform"))
	{
		ImGui::DragFloat3("Position", &position.x, 0.1f);
		ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat3("Rotation", &angle.x, 1.0f, 0.0f, 360.0f);

		ImGui::Separator();
		ImGui::Text("Ball Transform");
		ImGui::DragFloat3("BallPosition", &ballPosition.x);
		ImGui::DragFloat3("BallAngle", &ballAngle.x);
		ImGui::DragFloat3("BallScale", &ballScale.x);

		ImGui::Separator();
		ImGui::Text("Ball World Transform");
		ImGui::DragFloat3("BallWorldPosition", &ballWorldPosition.x);
		ImGui::DragFloat3("BallWorldAngle", &ballWorldAngle.x);
		ImGui::DragFloat3("BallWorldScale", &ballWorldScale.x);

		ImGui::Separator();
		ImGui::DragFloat("Throw Timing", &throwTiming, 0.01f, 0.0f, 1.0f);
		ImGui::Checkbox("Is Ball Thrown", &isBallThrown);

		ImGui::End();
	}
}

void Pitcher::SetPitchingState()
{
	state = State::Throwing;
	isBallThrown = false;
	pitcher->PlayAnimation(Animation::Pitching, true);
}

void Pitcher::UpdatePitchingState(float elapsedTime)
{
	// アニメーションの進行度を取得
	float animationTime = pitcher->GetCurrentAnimationSeconds();
	const ModelResource::Animation& anim = pitcher->GetResource()->GetAnimations()[Animation::Pitching];
	float animationProgress = animationTime / anim.secondsLength;

	// 特定のタイミングでボールを投げる
	if (!isBallThrown && animationProgress >= throwTiming)
	{
		isBallThrown = true;

		// 投げる方向（前方向）を計算
		DirectX::XMFLOAT3 forward;
		forward.x = sinf(angle.y);
		forward.y = 0.0f;
		forward.z = cosf(angle.y);

		// 左右方向のランダムなオフセットを追加（追加）
		DirectX::XMFLOAT3 right;
		right.x = cosf(angle.y);
		right.y = 0.0f;
		right.z = -sinf(angle.y);

		// ランダムな初速を設定
		float throwSpeed = 100.0f + static_cast<float>(rand() % 251); // 100～350
		float ySpeed = -20.0f + static_cast<float>(rand() % 11); // -20～-10
		float xOffset = -10.0f + static_cast<float>(rand() % 21); // -10～10（左右のブレ）

		ballVelocity.x = forward.x * throwSpeed + right.x * xOffset; // X方向にランダム性を追加
		ballVelocity.y = ySpeed; // ランダムなY方向の初速
		ballVelocity.z = forward.z * throwSpeed + right.z * xOffset; // Z方向にもランダム性を追加

		// ボールの回転初期化
		ballWorldAngle.x = 0.0f;
		ballWorldAngle.y = angle.y;
		ballWorldAngle.z = 0.0f;

		// スケールを手に持っているときと同じにする
		ballWorldScale.x = ballScale.x * 0.1f;
		ballWorldScale.y = ballScale.y * 0.1f;
		ballWorldScale.z = ballScale.z * 0.1f;
	}
}