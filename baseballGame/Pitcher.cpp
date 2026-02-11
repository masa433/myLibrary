#include "Pitcher.h"
#include "imgui.h"
#include <Windows.h>
#include "Graphics.h"
#include <algorithm>
#include "scene_game.h"


// 初期化
void Pitcher::Initialize() 
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//モデルの読み込み
	pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\pitcher.glb");

	position = { 0.0f,1.4f,-1.7f };
	scale = { -0.03f,0.03f,0.03f };
	angle = { 0.0f, 0.0f, 0.0f };

	// アニメーション用のノードをコピー
	animated_nodes = pitcher->nodes;

	ball = std::make_unique<gltf_model>(device, ".\\resources\\ball\\ball.glb");
	ballPosition = { 0.0f,2.0f,5.5f };
	ballScale = { 100.0f,100.0f,100.0f };
	ballAngle = { 0.0f,DirectX::XMConvertToRadians(90.0f),0.0f };

	rotationSpeed = { 0.0f,0.0f,-150.0f };//バックスピン

}

void Pitcher::Uninitialize() 
{
}

// 更新
void Pitcher::Update(float elapsedTime) 
{
	
	UpdateAnimation(elapsedTime);

	// 位置更新
	UpdateTransform();

	AttachBallToHand(elapsedTime);


}

// 描画
void Pitcher::Render(RenderContext& rc) 
{
	
	pitcher->render(rc.context, transform, animated_nodes);
	if(isBallThrown)
	{
		ball->render(rc.context, ballWorldTransform, {});
	}
	else 
	{
		ball->render(rc.context, ballTransform, {});
	}

	// ShapeRenderer をボールに関連付けて描画
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	const DirectX::XMFLOAT3& ballPosition = Pitcher::Instance().GetBallPosition(); // ボールの位置を取得
	const DirectX::XMFLOAT3& ballScale = Pitcher::Instance().GetBallScale();       // ボールのスケールを取得

	// スケールを ImGui の値に基づいて変更
	reducedRadius = (ballScale.x / ballScale.x) * ballDebugRadius;

	// ShapeRenderer で描画
	shapeRenderer->DrawSphere(ballPosition, reducedRadius, { 1, 0, 0, 1 }); // スケールを適用
	shapeRenderer->Render(rc.context, rc.camera->GetView(), rc.camera->GetProjection());
	
}

void Pitcher::DrawGUI()
{
#ifdef USE_IMGUI
	if (ImGui::Begin(u8"ピッチャー"))
	{
		if (ImGui::CollapsingHeader("Pitcher Animation Control"))
		{
			ImGui::DragFloat3("Position", &position.x);
			ImGui::DragFloat3("Scale", &scale.x);
			ImGui::DragFloat3("Angle", &angle.x);
			ImGui::Checkbox("Play Animation", &animation_playing);
		}
		if (ImGui::CollapsingHeader("ball"))
		{
			ImGui::DragFloat3("ballPosition", &ballPosition.x);
			ImGui::DragFloat3("ballScale", &ballScale.x);
			ImGui::DragFloat3("ballAngle", &ballAngle.x);

			ImGui::Separator();
			ImGui::Text("Ball World Transform");
			ImGui::DragFloat3("BallWorldPosition", &ballWorldPosition.x);
			ImGui::DragFloat3("BallWorldAngle", &ballWorldAngle.x);
			ImGui::DragFloat3("BallWorldScale", &ballWorldScale.x);

			ImGui::Separator();
			ImGui::Text("Pitch Settings");
			ImGui::DragFloat("Throw Timing", &throwTiming, 0.01f, 0.0f, 1.0f);
			ImGui::SliderFloat("Ball Speed (km/h)", &ballSpeedKmh, 10.0f, 180.0f);
			ImGui::SliderFloat("Launch Angle (deg)", &launchAngleDegrees, -20.0f, 10.0f);

			ImGui::Separator();
			ImGui::Text("Throw Direction");
			ImGui::DragFloat3("Throw Direction", &throwDirection.x, -1.0f, 1.0f); // 投球方向を操作可能に

			ImGui::Separator();
			ImGui::Text("Rotation Settings");
			ImGui::SliderFloat(u8"X軸回転速度 (サイドスピン)", &rotationSpeed.x, -150.0f, 150.0f);
			ImGui::SliderFloat(u8"Y軸回転速度", &rotationSpeed.y, -150.0f, 150.0f);
			ImGui::SliderFloat(u8"Z軸回転速度 (バック/トップスピン)", &rotationSpeed.z, -150.0f, 150.0f);

			ImGui::Separator();
			ImGui::Text("Break Settings");
			ImGui::SliderFloat(u8"横方向の変化量", &horizontalBreak, -30.0f, 30.0f);
			ImGui::SliderFloat(u8"縦方向の変化量", &verticalBreak, -30.0f, 30.0f);
			ImGui::SliderFloat(u8"変化が始まる距離", &breakStartDistance, 0.0f, 20.0f);

			ImGui::Separator();
			// 球種プリセット
			if (ImGui::Button(u8"Fastball (ストレート)"))
			{
				horizontalBreak = 0.0f;
				verticalBreak = 0.0f;
				ballSpeedKmh = 150.0f; // 速い
				ballAngle = { 0.5f,DirectX::XMConvertToRadians(90.0f),0.0f };
				rotationSpeed = { 0.0f,0.0f,-100.0f };//バックスピン
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"Slider (スライダー)"))
			{
				horizontalBreak = -15.0f; // 右方向に曲がる
				verticalBreak = -5.0f;   // 少し落ちる
				ballSpeedKmh = 130.0f;  // 少し遅い
				ballAngle = { -0.2f, 0.0f, 0.0f };
				rotationSpeed = { 0.0f,0.0f,-100.0f };//サイドスピン
			}
			if (ImGui::Button(u8"Curveball (カーブ)"))
			{
				horizontalBreak = -10.0f; // 左方向に曲がる
				verticalBreak = -15.0f;   // 大きく落ちる
				ballSpeedKmh = 110.0f;  // 遅い
				ballAngle = { 0.5f,DirectX::XMConvertToRadians(90.0f),0.0f };
				rotationSpeed = { 0.0f,0.0f,150.0f };//トップスピン
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"Changeup (チェンジアップ)"))
			{
				horizontalBreak = 10.0f;
				verticalBreak = -12.0f; // 落ちる
				ballSpeedKmh = 120.0f;  // 遅い
				rotationSpeed = { 100.0f,0.0f,100.0f };//ミックス回転
			}
			if(ImGui::Button(u8"Forkball (フォーク)"))
			{
				horizontalBreak = 0.0f;
				verticalBreak = -20.0f; // 大きく落ちる
				ballAngle.y = 0.0f;
				rotationSpeed = { -40.0f,0.0f,10.0f };//回転は少なめ
				ballSpeedKmh = 130.0f;  // 遅い
				
			}
			ImGui::SameLine();
			if(ImGui::Button(u8"Two-seam(ツーシーム)"))
			{
				horizontalBreak = 10.0f;
				verticalBreak = -10.0f; // 少し落ちる
				ballAngle.y = 0.0f;
				ballAngle.x = 0.2f;
				rotationSpeed = { -100.0f,0.0f,0.0f };//回転は少なめ
				ballSpeedKmh = 140.0f;  // 遅い
			}

			ImGui::Separator();
			ImGui::Checkbox("Is Ball Thrown", &isBallThrown);

			ImGui::Separator();
			// 速度情報の表示
			float speedMs = ballSpeedKmh / 3.6f;
			ImGui::Text("Speed: %.2f m/s (%.0f km/h)", speedMs, ballSpeedKmh);
		}

	}
	// ボールのデバッグスケールを変更するスライダー
	if (ImGui::CollapsingHeader("Debug Settings"))
	{
		ImGui::SliderFloat("Ball Debug Radius", &ballDebugRadius, 0.1f, 5.0f, "%.2f");
	}
	ImGui::End();
#endif
}

//アタッチメント処理
void Pitcher::AttachBallToHand(float elapsedTime)
{
	// ボールが投げられていない場合は手に追従
	if (!isBallThrown)
	{
		const char* handName = "mixamorig:RightHandMiddle1";

		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(ballScale.x, ballScale.y, ballScale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(ballAngle.x, ballAngle.y, ballAngle.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(ballPosition.x, ballPosition.y, ballPosition.z);
		DirectX::XMMATRIX ballLocalMatrix = S * R * T;

		bool handFound = false;
		for (const gltf_model::node& node : animated_nodes)
		{
			if (node.name == handName)
			{
				DirectX::XMMATRIX rightHandMatrix = DirectX::XMLoadFloat4x4(&node.global_transform);
				DirectX::XMMATRIX pitcherWorldMatrix = DirectX::XMLoadFloat4x4(&transform);
				DirectX::XMMATRIX ballWorldMatrix = ballLocalMatrix * rightHandMatrix * pitcherWorldMatrix;

				DirectX::XMStoreFloat4x4(&ballTransform, ballWorldMatrix);

				// ボールのワールド座標を保存（投げる瞬間の位置）
				ballWorldPosition.x = ballTransform._41;
				ballWorldPosition.y = ballTransform._42;
				ballWorldPosition.z = ballTransform._43;

				// 投球開始位置を保存
				ballStartPosition = ballWorldPosition;

				handFound = true;
				break;
			}
		}

		// 手が見つからない場合のフォールバック
		if (!handFound)
		{
			DirectX::XMMATRIX pitcherWorldMatrix = DirectX::XMLoadFloat4x4(&transform);
			DirectX::XMMATRIX ballWorldMatrix = ballLocalMatrix * pitcherWorldMatrix;
			DirectX::XMStoreFloat4x4(&ballTransform, ballWorldMatrix);
		}
	}
	else
	{
		//投球開始位置からの距離を計算
		float distanceTravel = sqrtf(
			(ballWorldPosition.x - ballStartPosition.x) * (ballWorldPosition.x - ballStartPosition.x) +
			(ballWorldPosition.z - ballStartPosition.z) * (ballWorldPosition.z - ballStartPosition.z)
		);

		//変化が始まる距離を超えたら変化する
		float breakFactor = 0.0f;
		if (distanceTravel > breakStartDistance) 
		{
			breakFactor = (std::min)(1.0f, (distanceTravel - breakStartDistance) / 10.0f);
		}

		//重力を適用
		ballVelocity.y += gravity * elapsedTime;

		//変化の加速度
		float ballAccelerationX = horizontalBreak * breakFactor * elapsedTime;
		float ballAccelerationY = verticalBreak * breakFactor * elapsedTime;

		ballVelocity.x += ballAccelerationX;
		ballVelocity.y += ballAccelerationY;

		// 空気抵抗
		float airResistance = 0.99999f;
		ballVelocity.x *= airResistance;
		ballVelocity.z *= airResistance;

		// 位置を更新
		ballWorldPosition.x += ballVelocity.x * elapsedTime;
		ballWorldPosition.y += ballVelocity.y * elapsedTime;
		ballWorldPosition.z += ballVelocity.z * elapsedTime;

		// ボールの回転を更新
		ballWorldAngle.x += rotationSpeed.x * elapsedTime;
		ballWorldAngle.y += rotationSpeed.y * elapsedTime;
		ballWorldAngle.z += rotationSpeed.z * elapsedTime;
		

		// ボールのワールド行列を更新
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(ballWorldScale.x, ballWorldScale.y, ballWorldScale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(ballWorldAngle.x, ballWorldAngle.y, ballWorldAngle.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(ballWorldPosition.x, ballWorldPosition.y, ballWorldPosition.z);
		DirectX::XMMATRIX ballWorldMatrix = S * R * T;
		DirectX::XMStoreFloat4x4(&ballWorldTransform, ballWorldMatrix);

		// 地面に落ちたらリセット
		if (ballWorldPosition.y < 0.0f)
		{
			isBallThrown = false;
			animation_time = 0.0f;
			ballVelocity = { 0.0f, 0.0f, 0.0f };
		}
	}
}

// アニメーション更新
void Pitcher::UpdateAnimation(float elapsedTime)
{
	if (!pitcher || pitcher->animations.empty())
	{
		return;
	}

	if (animation_playing)
	{
		animation_time += elapsedTime;

		// インデックスの範囲チェック
		if (current_animation_index < 0 || current_animation_index >= static_cast<int>(pitcher->animations.size()))
		{
			current_animation_index = 0;
		}

		float animation_duration = pitcher->animations[current_animation_index].duration;

		// ボールを投げるタイミングの判定
		if (!isBallThrown && animation_time >= throwTiming * animation_duration)
		{
			// ボールを投げる
			isBallThrown = true;

			// km/hからm/sに変換
			float speedMs = ballSpeedKmh ;

			// 発射角度を適用
			float launchAngleRadians = DirectX::XMConvertToRadians(launchAngleDegrees);

			// 方向ベクトルを設定
			throwDirection.y = sinf(launchAngleRadians);
			throwDirection.z = cosf(launchAngleRadians);

			// 正規化
			DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&throwDirection);
			dir = DirectX::XMVector3Normalize(dir);
			DirectX::XMFLOAT3 normalizedDir;
			DirectX::XMStoreFloat3(&normalizedDir, dir);

			// 速度ベクトルを設定（m/s単位）
			ballVelocity.x = normalizedDir.x * speedMs;
			ballVelocity.y = normalizedDir.y * speedMs;
			ballVelocity.z = normalizedDir.z * speedMs;

			// 投げた瞬間のボールのスケールと角度を設定
			ballWorldScale = { 3.0f, 3.0f, 3.0f };
			ballWorldAngle = ballAngle;
		}

		// アニメーションのループ処理
		if (animation_time > animation_duration)
		{
			animation_time = fmodf(animation_time, animation_duration);
			isBallThrown = false;
		}

		pitcher->animate(current_animation_index, animation_time, animated_nodes);
	}
}