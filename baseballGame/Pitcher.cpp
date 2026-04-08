#include "Pitcher.h"
#include "imgui.h"
#include <Windows.h>
#include "Graphics.h"
#include <algorithm>
#include "scene_game.h"
#include <random>

// ランダムな浮動小数点数を生成する関数
float GenerateRandomFloat(float min, float max)
{
	std::random_device rd; // ランダムデバイス
	std::mt19937 gen(rd()); // メルセンヌ・ツイスタ
	std::uniform_real_distribution<float> dis(min, max); // 一様分布
	return dis(gen);
}

// 初期化
void Pitcher::Initialize() 
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//モデルの読み込み
	pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\pitcher.glb");

	position = { -0.1f,0.22f,18.15f };
	scale = { -0.01f,0.01f,0.01f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f};

	// アニメーション用のノードをコピー
	animated_nodes = pitcher->nodes;

	ball = std::make_unique<gltf_model>(device, ".\\resources\\ball\\ball.glb");
	ballPosition = { 0.0f,2.0f,5.5f };
	ballScale = { 100.0f,100.0f,100.0f };
	ballAngle = { 0.0f,DirectX::XMConvertToRadians(90.0f),0.0f };

	ballDebugRadius = 0.037f; // デバッグ用の半径

	//rotationSpeed = { 0.0f,0.0f,-150.0f };//バックスピン

	{
		// PhysXのボールコライダーを作成
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxMaterial* pxMaterial = Physics::Instance().GetMaterial();
		physx::PxScene* pxScene = Physics::Instance().GetScene();

		//pxMaterial->setRestitution(0.6f);// 反発係数を設定
		//pxMaterial->setDynamicFriction(0.4f);// 動摩擦係数を設定
		//pxMaterial->setStaticFriction(0.5f);// 静止摩擦係数を設定
		pxBallMaterial = pxPhysics->createMaterial(0.5f, 0.4f, 0.52f); // ボール専用のマテリアルとして保存
		//pxMaterial->setRestitutionCombineMode(physx::PxCombineMode::eAVERAGE);

		// ボールの球状コライダーを作成
		physx::PxSphereGeometry ballGeometry(ballDebugRadius);
		physx::PxTransform ballTransform(
			physx::PxVec3(ballWorldPosition.x, ballWorldPosition.y, ballWorldPosition.z)
		);

		ballCollider = pxPhysics->createRigidDynamic(ballTransform);
		_ASSERT_EXPR(ballCollider != nullptr, "Failed to create ball collider");

		// ボールのコライダーに形状を追加
		physx::PxShape* ballShape = pxPhysics->createShape(ballGeometry, *pxBallMaterial);
		ballCollider->attachShape(*ballShape);

		// ボールの質量を設定
		float originalMass = 0.145f; // 野球の質量は約145g
		float scaleFactor = ballScale.x / 100.0f; // モデルのスケールに基づく質量のスケーリング
		float scaledMass = originalMass * (scaleFactor * scaleFactor * scaleFactor); // 体積に比例して質量をスケーリング
		physx::PxRigidBodyExt::updateMassAndInertia(*ballCollider, 0.145f); // 質量をスケーリングに基づいて設定

		// 重力を有効化
		//ballCollider->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);

		// シーンに追加
		pxScene->addActor(*ballCollider);

		// 解放
		ballShape->release();
	}

}

void Pitcher::Uninitialize() 
{
	PX_RELEASE(ballCollider);
	PX_RELEASE(pxBallMaterial);
}

// 更新
void Pitcher::Update(float elapsedTime)
{


	// **バックスペースキーで強制的に投球開始**
	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
	{
		if (currentState == State::SelectingPitch)
		{
			currentState = State::Throwing;
			stateTime = 0.0f;
			SelectPitchType(); // 球種選択
			OutputDebugStringA("Forced Throw: Backspace pressed\n");
		}
	}

	// 状態に応じた処理
	switch (currentState)
	{
	case State::SelectingPitch:
		stateTime += elapsedTime;
		if (stateTime > 7.0f) // 3秒後に投球開始
		{
			currentState = State::Throwing;
			stateTime = 0.0f;
			SelectPitchType(); // 球種選択
		}
		break;

	case State::Throwing:
		// 投げるアニメーションを再生
		UpdateAnimation(elapsedTime);

		// アニメーションが終了したら球種選択状態に遷移
		if (animation_time >= pitcher->animations[current_animation_index].duration)
		{
			currentState = State::SelectingPitch; // 球種選択状態に戻る
			animation_time = 0.0f; // アニメーション時間をリセット
			hasBeenJudged = false; // 判定フラグをリセット
		}
		break;
	}

	// 共通の更新処理
	UpdateTransform();
	UpdateBallCollider();
	AttachBallToHand(elapsedTime);

	// ストライク/ボールの判定
	if (isBallThrown && !hasBeenJudged)
	{
		// ボールがストライクゾーン内に入ったかを確認
		if (IsBallInStrikeZone())
		{
			OutputDebugStringA("Strike!\n");
			hasBeenJudged = true; // 判定済みフラグを設定
		}
		else if (ballWorldPosition.z > strikeZonePosition.z + strikeZoneSize.z / 2.0f)
		{
			// ボールがストライクゾーン外を通過した場合
			OutputDebugStringA("Ball!\n");
			hasBeenJudged = true; // 判定済みフラグを設定
		}
	}

	if (isBallThrown)
	{
		throwCounter += elapsedTime; // 投球カウンターを更新

		if (!hasReachedZero && ballWorldPosition.z <= 0.0f)
		{
			hasReachedZero = true; // z = 0.0f に到達したことを記録
			char debugMessage[128];
			snprintf(debugMessage, sizeof(debugMessage), "Time to reach z=0.0f: %.2f seconds\n", throwCounter);
			OutputDebugStringA(debugMessage);
		}
	}

	// ボールが地面に落ちたらリセット
	if (ballWorldPosition.y < 0.0f)
	{
		isBallThrown = false;
		hasBeenJudged = false; // 判定フラグをリセット
		hasCollided = false; // 衝突フラグをリセット
		hasCollidedWithFence = false; // フェンス衝突フラグをリセット
	}
}

void Pitcher::UpdateBallCollider()
{
	if (!ballCollider) return;

	// ボールのスケールを更新
	physx::PxShape* ballShape;
	ballCollider->getShapes(&ballShape, 1);

	// コライダーのスケールを更新
	physx::PxSphereGeometry ballGeometry(ballDebugRadius);
	ballShape->setGeometry(ballGeometry);

}

bool Pitcher::IsBallInStrikeZone() const
{
	// ストライクゾーンの最小値と最大値を計算
	float tolerance = 0.1f; // 変化球の影響を考慮した許容範囲
	float strikeZoneMinX = strikeZonePosition.x - strikeZoneSize.x / 2.0f - 0.3f - tolerance;
	float strikeZoneMaxX = strikeZonePosition.x + strikeZoneSize.x / 2.0f + 0.3f + tolerance;
	float strikeZoneMinY = strikeZonePosition.y - strikeZoneSize.y / 2.0f - 0.3f - tolerance;
	float strikeZoneMaxY = strikeZonePosition.y + strikeZoneSize.y / 2.0f + 0.3f + tolerance;
	float strikeZoneMinZ = strikeZonePosition.z - strikeZoneSize.z / 2.0f - 0.3f - tolerance;
	float strikeZoneMaxZ = strikeZonePosition.z + strikeZoneSize.z / 2.0f + 0.3f + tolerance;

	// ボールがストライクゾーン内にあるかを判定
	return (ballWorldPosition.x >= strikeZoneMinX && ballWorldPosition.x <= strikeZoneMaxX) &&
		(ballWorldPosition.y >= strikeZoneMinY && ballWorldPosition.y <= strikeZoneMaxY) &&
		(ballWorldPosition.z >= strikeZoneMinZ && ballWorldPosition.z <= strikeZoneMaxZ);
}

// 描画
void Pitcher::Render(const RenderContext& rc, ModelRenderer* renderer) 
{
	
	pitcher->render(rc.deviceContext, transform, animated_nodes);
	if(isBallThrown)
	{
		ball->render(rc.deviceContext, ballWorldTransform, {});
	}
	else 
	{
		ball->render(rc.deviceContext, ballTransform, {});
	}

	// ShapeRenderer をボールに関連付けて描画
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	const DirectX::XMFLOAT3& ballPosition = Pitcher::Instance().GetBallPosition(); // ボールの位置を取得
	const DirectX::XMFLOAT3& ballScale = Pitcher::Instance().GetBallScale();       // ボールのスケールを取得
	float tolerance = 0.1f; // 変化球の影響を考慮した許容範囲
	// ストライクゾーンの範囲を描画
	DirectX::XMFLOAT3 strikeZoneMin = {
		strikeZonePosition.x - strikeZoneSize.x / 2.0f - 0.3f - tolerance,
		strikeZonePosition.y - strikeZoneSize.y / 2.0f - 0.3f - tolerance,
		strikeZonePosition.z - strikeZoneSize.z / 2.0f - 0.3f - tolerance
	};
	DirectX::XMFLOAT3 strikeZoneMax = {
		strikeZonePosition.x + strikeZoneSize.x / 2.0f + 0.3f + tolerance,
		strikeZonePosition.y + strikeZoneSize.y / 2.0f + 0.3f + tolerance,
		strikeZonePosition.z + strikeZoneSize.z / 2.0f + 0.3f + tolerance
	};

	// ストライクゾーンを描画（緑色の半透明ボックス）
	//shapeRenderer->DrawBox(strikeZonePosition, {}, strikeZoneSize, strikeZoneColor);

	//// strikeZoneMin を赤い球体で描画
	//shapeRenderer->DrawSphere(strikeZoneMin, 0.1f, { 1, 0, 0, 1 }); // 半径 0.1f の赤い球体

	//// strikeZoneMax を青い球体で描画
	//shapeRenderer->DrawSphere(strikeZoneMax, 0.1f, { 0, 0, 1, 1 }); // 半径 0.1f の青い球体

	//// スケールを ImGui の値に基づいて変更
	//reducedRadius = (ballScale.x / ballScale.x) * ballDebugRadius;

	//// ShapeRenderer で描画
	//shapeRenderer->DrawSphere(ballPosition, reducedRadius, { 1, 0, 0, 1 }); // スケールを適用
	//shapeRenderer->Render(rc.context, rc.camera->GetView(), rc.camera->GetProjection(), rc.light->GetDirectionalLight().direction);

	
	
}

void Pitcher::DrawGUI()
{
#ifdef USE_IMGUI
	if (ImGui::Begin(u8"ピッチャー"))
	{
		ImGui::Text("Current State: %s",
			currentState == State::SelectingPitch ? "Selecting Pitch" :
			currentState == State::Throwing ? "Throwing" : "Idle");
		ImGui::Text("State Time: %.2f seconds", stateTime);

		if (ImGui::CollapsingHeader(u8"ストライクゾーン"))
		{
			ImGui::DragFloat3("StrikeZone Position", &strikeZonePosition.x, 0.1f);
			ImGui::DragFloat3("StrikeZone Size", &strikeZoneSize.x, 0.1f);
			ImGui::Text("Adjust the strike zone to ensure proper height.");
		}
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
			ImGui::DragFloat("Ball Speed (km/h)", &ballSpeedKmh, 10.0f, 180.0f);
			ImGui::DragFloat("Launch Angle (deg)", &launchAngleDegrees, -20.0f, 10.0f);

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
			ImGui::DragFloat(u8"横方向の変化量", &horizontalBreak, -30.0f, 30.0f);
			ImGui::DragFloat(u8"縦方向の変化量", &verticalBreak, -30.0f, 30.0f);
			ImGui::DragFloat(u8"変化が始まる距離", &breakStartDistance, 0.0f, 20.0f);

			ImGui::Separator();
			// 球種プリセット
			if (ImGui::Button(u8"Fastball (ストレート)"))
			{
				horizontalBreak = 0.0f;
				verticalBreak = 20.0f;
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
				verticalBreak = -30.0f; // 大きく落ちる
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
			if (ImGui::Button(u8"Cutter (カットボール)"))
			{
				horizontalBreak = -10.0f; // 左方向に少し曲がる
				verticalBreak = -2.0f;    // 少し落ちる
				ballSpeedKmh = 140.0f;    // 速い
				rotationSpeed = { 0.0f, 0.0f, -80.0f }; // 回転速度
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"Sinker (シンカー)"))
			{
				horizontalBreak = 5.0f;   // 右方向に少し曲がる
				verticalBreak = -10.0f;   // 大きく落ちる
				ballSpeedKmh = 130.0f;    // 少し遅い
				rotationSpeed = { 0.0f, 0.0f, -120.0f }; // 回転速度
			}
			if (ImGui::Button(u8"Vertical Slider (縦スライダー)"))
			{
				horizontalBreak = 0.0f;   // 横方向の変化なし
				verticalBreak = -15.0f;   // 大きく落ちる
				ballSpeedKmh = 125.0f;    // 遅い
				rotationSpeed = { 0.0f, 0.0f, -100.0f }; // 回転速度
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"Splitter (スプリット)"))
			{
				horizontalBreak = 0.0f;   // 横方向の変化なし
				verticalBreak = -20.0f;   // 非常に大きく落ちる
				ballSpeedKmh = 135.0f;    // 少し遅い
				rotationSpeed = { 0.0f, 0.0f, -50.0f }; // 回転速度
			}
			if (ImGui::Button(u8"Slow Curve (スローカーブ)"))
			{
				horizontalBreak = -5.0f;  // 左方向に少し曲がる
				verticalBreak = -25.0f;   // 非常に大きく落ちる
				ballSpeedKmh = 80.0f;    // 非常に遅い
				rotationSpeed = { 0.0f, 0.0f, 150.0f }; // トップスピン
			}
			if(ImGui::Button(u8"Shooter (シューター)"))
			{
				horizontalBreak = 15.0f;  // 右方向に大きく曲がる
				verticalBreak = -5.0f;    // 少し落ちる
				ballSpeedKmh = 145.0f;    // 少し遅い
				rotationSpeed = { 0.0f, 0.0f, -150.0f }; // バックスピン
				ballAngle = { 0.5f,DirectX::XMConvertToRadians(90.0f),0.0f };
			}
			if(ImGui::Button(u8"Knuckleball (ナックルボール)"))
			{
				horizontalBreak = 0.0f; //変化なし
				verticalBreak = 0.0f;   //変化なし
				ballSpeedKmh = 90.0f;    // 非常に遅い
				ballAngle = { 0.0f, 0.0f, 0.0f };
				rotationSpeed = { 5.0f, 0.0f, 5.0f }; // 不規則な回転
			}

			ImGui::Separator();
			ImGui::Checkbox("Is Ball Thrown", &isBallThrown);

			ImGui::Separator();
			// 速度情報の表示
			float speedMs = ballSpeedKmh / 3.6f;
			ImGui::Text("Speed: %.2f m/s (%.0f km/h)", speedMs, ballSpeedKmh);

			float originalMass = 0.145f; // 野球の質量は約145g
			float scaleFactor = ballScale.x / 100.0f; // モデルのスケールに基づく質量のスケーリング
			float scaledMass = originalMass * (scaleFactor * scaleFactor * scaleFactor); // 体積に比例して質量をスケーリング
			ImGui::Text("Mass: %.4f kg (Scaled by %.2f)", scaledMass, scaleFactor);
		}

	}
	// ボールのデバッグスケールを変更するスライダー
	if (ImGui::CollapsingHeader("Debug Settings"))
	{
		ImGui::DragFloat("Ball Debug Radius", &ballDebugRadius, 0.05f, 0.05f, 5.0f, "%.2f");
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

				// ボールのワールド座標を保存
				ballWorldPosition.x = ballTransform._41;
				ballWorldPosition.y = ballTransform._42;
				ballWorldPosition.z = ballTransform._43;

				// コライダーをボールの位置に同期
				if (ballCollider)
				{
					physx::PxTransform ballPhysxTransform(
						physx::PxVec3(ballWorldPosition.x, ballWorldPosition.y, ballWorldPosition.z)
					);
					ballCollider->setGlobalPose(ballPhysxTransform);
					// 速度をゼロに設定（手に追従している間は動かない）
					ballCollider->setLinearVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
					ballCollider->setAngularVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
				}

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
		// 投球後はPhysXで動きを制御
		ApplyPhysicsToBall(elapsedTime);

		// PhysXから位置を取得してワールド行列を更新
		physx::PxTransform ballPhysxTransform = ballCollider->getGlobalPose();
		ballWorldPosition.x = ballPhysxTransform.p.x;
		ballWorldPosition.y = ballPhysxTransform.p.y;
		ballWorldPosition.z = ballPhysxTransform.p.z;

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
			hasCollided = false; // 衝突フラグをリセット
			animation_time = 0.0f;
			ballVelocity = { 0.0f, 0.0f, 0.0f };
		}
	}
}


// 投球開始時にタイマーをリセット
void Pitcher::UpdateAnimation(float elapsedTime)
{
	if (!pitcher || pitcher->animations.empty())
	{
		return;
	}

	if (animation_playing)
	{
		if (animation_time == 0.0f)
		{
			isBallThrown = false;
		}

		animation_time += elapsedTime;

		if (current_animation_index < 0 || current_animation_index >= static_cast<int>(pitcher->animations.size()))
		{
			current_animation_index = 0;
		}

		float animation_duration = pitcher->animations[current_animation_index].duration;

		if (!isBallThrown && animation_time >= throwTiming * animation_duration)
		{
			isBallThrown = true;
			throwCounter = 0.0f; // 投球カウンターをリセット
			hasReachedZero = false; // z = 0.0f に到達フラグをリセット
			SetHasCollided(false);
			SetHasCollidedWithFence(false);

			// ボール反発係数をリセット
			{
				physx::PxRigidDynamic* ballCollider = GetBallCollider();
				if (ballCollider)
				{
					physx::PxShape* ballShape;
					ballCollider->getShapes(&ballShape, 1);
					physx::PxMaterial* ballMaterial;
					ballShape->getMaterials(&ballMaterial, 1);
					ballMaterial->setRestitution(0.52f);  // 初期値に設定
				}
			}

			float speedMs = ballSpeedKmh / 3.6f;
			float launchAngleRadians = DirectX::XMConvertToRadians(launchAngleDegrees);

			throwDirection.y = sinf(launchAngleRadians);
			throwDirection.z = -cosf(launchAngleRadians);

			DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&throwDirection);
			dir = DirectX::XMVector3Normalize(dir);
			DirectX::XMFLOAT3 normalizedDir;
			DirectX::XMStoreFloat3(&normalizedDir, dir);

			physx::PxVec3 initialVelocity(normalizedDir.x * speedMs, normalizedDir.y * speedMs, normalizedDir.z * speedMs);
			ballCollider->setLinearVelocity(initialVelocity);

			ballWorldScale = { 1.0f, 1.0f, 1.0f };
			ballWorldAngle = ballAngle;

			physx::PxRigidDynamic* ballCollider = GetBallCollider();
			if (ballCollider)
			{
				ballCollider->setLinearVelocity(initialVelocity);
				float throwSpeed = initialVelocity.magnitude();

				char debugMessage[128];
				snprintf(debugMessage, sizeof(debugMessage), "Throw Speed: %.2f km/h\n", throwSpeed * 3.6f);
				OutputDebugStringA(debugMessage);
			}
		}

		pitcher->animate(current_animation_index, animation_time, animated_nodes);
	}
}



void Pitcher::ApplyPhysicsToBall(float elapsedTime)
{
	if (!ballCollider) return;

	// 投球開始位置からの距離を計算
	float distanceTravel = sqrtf(
		(ballWorldPosition.x - ballStartPosition.x) * (ballWorldPosition.x - ballStartPosition.x) +
		(ballWorldPosition.z - ballStartPosition.z) * (ballWorldPosition.z - ballStartPosition.z)
	);

	// ナックルボールの特性: ランダムな横方向の揺れを加える
	if (selectedPitchType == PitchType::Knuckleball)
	{
		float randomLateralForce = GenerateRandomFloat(-0.01f, 0.01f); // ランダムな横方向の力
		physx::PxVec3 lateralForce(randomLateralForce, 0.0f, 0.0f);
		ballCollider->addForce(lateralForce, physx::PxForceMode::eFORCE);
	}

	// 変化球の力を加える
	if (!hasCollided && distanceTravel > breakStartDistance)
	{
		float breakFactor = (std::min)(1.0f, (distanceTravel - breakStartDistance) / 10.0f);
		float smoothBreakFactor = sinf(breakFactor * DirectX::XM_PIDIV2);
		float forceMultiplier = 0.0005f; // 半径に基づいてスケーリング

		// 横方向の力を加える
		physx::PxVec3 lateralForce(horizontalBreak * smoothBreakFactor * forceMultiplier, 0.0f, 0.0f);
		ballCollider->addForce(lateralForce, physx::PxForceMode::eFORCE);

		// 縦方向の力を加える
		physx::PxVec3 verticalForce(0.0f, verticalBreak * smoothBreakFactor * forceMultiplier, 0.0f);
		ballCollider->addForce(verticalForce, physx::PxForceMode::eFORCE);
	}

	// 空気抵抗を適用
	physx::PxVec3 velocity = ballCollider->getLinearVelocity();
	float speed = velocity.magnitude();
	float dragCoefficient = 0.005f; // 空気抵抗をスケールに基づいて調整
	float airResistance = 1.0f - (dragCoefficient * speed * elapsedTime);
	airResistance = (std::max)(0.99f, airResistance); // 最小値を設定
	velocity *= airResistance;
	ballCollider->setLinearVelocity(velocity);

	//// マグヌス効果を追加
	//physx::PxVec3 angularVelocity = ballCollider->getAngularVelocity();
	//float magnusCoefficient = 0.00000008f; // マグヌス効果を調整
	//physx::PxVec3 magnusForce = angularVelocity.cross(velocity) * magnusCoefficient;
	//ballCollider->addForce(magnusForce, physx::PxForceMode::eFORCE);

}


void Pitcher::SelectPitchType() 
{
	// 乱数生成
	float randomValue = GenerateRandomFloat(0.0f, 1.0f); // 0.0～1.0の乱数を生成

	if (randomValue <= 1.0f) // 50%の確率でストレート
	{
		selectedPitchType = PitchType::Fastball;
	}
	else // 残り50%の確率で他の球種をランダムに選択
	{
		// SlowCurve を含む他の球種をランダムに選択
		int randomPitchType = static_cast<int>(GenerateRandomFloat(0.0f, static_cast<float>(PitchType::Knuckleball)));
		selectedPitchType = static_cast<PitchType>(randomPitchType);
	}

	// 球種ごとの挙動を設定
	switch (selectedPitchType)
	{
	case PitchType::Fastball: // ストレート
		horizontalBreak = 0.0f;
		verticalBreak = 0.0f;//ややホップするような感じ
		ballSpeedKmh = 166.0f; // 速い
		ballAngle = { 0.2f, DirectX::XMConvertToRadians(90.0f), 0.0f};
		rotationSpeed = { 0.0f, 0.0f, -150.0f }; // バックスピン
		OutputDebugStringA("Pitch Type: Fastball\n");
		break;

	case PitchType::Slider: // スライダー
		horizontalBreak = 15.0f; // 左方向に曲がる
		verticalBreak = -5.0f;    // 少し落ちる
		ballSpeedKmh = 130.0f;    // 少し遅い
		ballAngle = { -0.2f, 0.0f, 0.0f };
		rotationSpeed = { 0.0f, 0.0f, -100.0f }; // サイドスピン
		OutputDebugStringA("Pitch Type: Slider\n");
		break;

	case PitchType::Curveball: // カーブ
		horizontalBreak = 10.0f; // 左方向に曲がる
		verticalBreak = -20.0f;   // 大きく落ちる
		ballSpeedKmh = 110.0f;    // 遅い
		ballAngle = { 0.5f, DirectX::XMConvertToRadians(90.0f), 0.0f };
		rotationSpeed = { 0.0f, 0.0f, 150.0f }; // トップスピン
		OutputDebugStringA("Pitch Type: Curveball\n");
		break;

	case PitchType::Changeup: // チェンジアップ
		horizontalBreak = -5.0f;  // 右方向に少し曲がる
		verticalBreak = -5.0f;   // 落ちる
		ballSpeedKmh = 120.0f;    // 遅い
		rotationSpeed = { 100.0f, 0.0f, 100.0f }; // ミックス回転
		OutputDebugStringA("Pitch Type: Changeup\n");
		break;

	case PitchType::Forkball: // フォーク
		horizontalBreak = 0.0f;
		verticalBreak = -30.0f;   // 非常に大きく落ちる
		ballSpeedKmh = 130.0f;    // 少し遅い
		ballAngle.y = 0.0f;
		rotationSpeed = { -40.0f, 0.0f, 10.0f }; // 回転は少なめ
		OutputDebugStringA("Pitch Type: Forkball\n");
		break;

	case PitchType::TwoSeam: // ツーシーム
		horizontalBreak = -10.0f;  // 右方向に少し曲がる
		verticalBreak = -7.0f;   // 少し落ちる
		ballSpeedKmh = 140.0f;    // 少し速い
		ballAngle.y = 0.0f;
		ballAngle.x = 0.2f;
		rotationSpeed = { -100.0f, 0.0f, 0.0f }; // 回転は少なめ
		OutputDebugStringA("Pitch Type: TwoSeam\n");
		break;

	case PitchType::Cutter: // カットボール
		horizontalBreak = 8.0f; // 左方向に少し曲がる
		verticalBreak = -2.0f;    // 少し落ちる
		ballSpeedKmh = 140.0f;    // 速い
		ballAngle = { -0.2f, 0.0f, 0.0f };
		rotationSpeed = { 0.0f, 0.0f, -80.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: Cutter\n");
		break;

	case PitchType::Sinker: // シンカー
		horizontalBreak = -15.0f;   // 右方向に少し曲がる
		verticalBreak = -15.0f;   // 大きく落ちる
		ballSpeedKmh = 130.0f;    // 少し遅い
		rotationSpeed = { 120.0f, 0.0f, 120.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: Sinker\n");
		break;

	case PitchType::VerticalSlider: // 縦スライダー
		horizontalBreak = 5.0f;   // 少し横に移動
		verticalBreak = -15.0f;   // 大きく落ちる
		ballSpeedKmh = 125.0f;    // 遅い
		ballAngle = { -0.2f, 0.0f, 0.0f };
		rotationSpeed = { 0.0f, 0.0f, -100.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: VerticalSlider\n");
		break;

	case PitchType::Splitter: // スプリット
		horizontalBreak = 0.0f;   // 横方向の変化なし
		verticalBreak = -15.0f;   // 非常に大きく落ちる
		ballSpeedKmh = 135.0f;    // 少し遅い
		ballAngle.y = 0.0f;
		rotationSpeed = { -40.0f, 0.0f, 10.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: Splitter\n");
		break;

	case PitchType::SlowCurve: // スローカーブ
		horizontalBreak = 15.0f;  // 左方向に少し曲がる
		verticalBreak = -20.0f;   // 非常に大きく落ちる
		ballSpeedKmh = 80.0f;     // 非常に遅い
		ballAngle = { 0.5f, DirectX::XMConvertToRadians(90.0f), 0.0f };
		rotationSpeed = { 0.0f, 0.0f, 150.0f }; // トップスピン
		OutputDebugStringA("Pitch Type: SlowCurve\n");
		break;

	case PitchType::Shooter: // シュート
		horizontalBreak = -15.0f;  // 大きく右に曲がる
		verticalBreak = -5.0f;   // 少し落ちる
		ballSpeedKmh = 145.0f;    // 遅い
		ballAngle = { -0.2f, 0.0f, 0.0f };
		rotationSpeed = { 0.0f, 0.0f, -150.0f }; // 強いサイドスピン
		OutputDebugStringA("Pitch Type: Shooter\n");
		break;

	case PitchType::Knuckleball: // ナックルボール
		horizontalBreak = 0.0f;   // 横方向の変化なし
		verticalBreak = 0.0f;     // 縦方向の変化なし
		ballSpeedKmh = 90.0f;     // 非常に遅い
		ballAngle = { 0.0f, 0.0f, 0.0f };
		rotationSpeed = { 5.0f, 0.0f, 5.0f }; // 不規則な回転
		OutputDebugStringA("Pitch Type: Knuckleball\n");
		break;

	default:
		break;
	}
	// ランダムな投球方向を設定
	throwDirection.x = GenerateRandomFloat(0.02f, 0.04f); // 左右方向のランダム値
	throwDirection.y = 0.2f; // 上下方向のランダム値
	throwDirection.z = -1.0f; // 前方向固定

	// ランダムな発射角度を設定
	launchAngleDegrees = GenerateRandomFloat(0.0f, 1.0f); // -4度から-2度の範囲でランダム
}