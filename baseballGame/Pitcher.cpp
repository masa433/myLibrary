#include "Pitcher.h"
#include "imgui.h"
#include <Windows.h>
#include "Graphics.h"
#include <algorithm>
#include "scene_game.h"
#include <random>
#include "PrimitiveRenderer.h"
#include "camera.h"
#include "Wind.h"

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
	scale = { 1.0f,1.0f,1.0f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f};

	// アニメーション用のノードをコピー
	animated_nodes = pitcher->nodes;

	Ball::Instance().Initialize();

	Wind::Instance().Initialize();
	
}

void Pitcher::Uninitialize() 
{
	Ball::Instance().Uninitialize();
	Wind::Instance().Uninitialize();
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
		else if (Ball::Instance().GetWorldPosition().z < strikeZonePosition.z + strikeZoneSize.z / 2.0f)
		{
			// ボールがストライクゾーン外を通過した場合
			OutputDebugStringA("Ball!\n");
			hasBeenJudged = true; // 判定済みフラグを設定
		}
	}

	if (isBallThrown)
	{
		throwCounter += elapsedTime; // 投球カウンターを更新

		if (!hasReachedZero && Ball::Instance().GetWorldPosition().z <= 0.0f)
		{
			hasReachedZero = true; // z = 0.0f に到達したことを記録
			char debugMessage[128];
			snprintf(debugMessage, sizeof(debugMessage), "Time to reach z=0.0f: %.2f seconds\n", throwCounter);
			OutputDebugStringA(debugMessage);
		}
	}

	

	// ボールが地面に落ちたらリセット
	if (Ball::Instance().GetWorldPosition().y < 0.0f)
	{
		isBallThrown = false;
		hasBeenJudged = false; // 判定フラグをリセット
		hasCollided = false; // 衝突フラグをリセット
		hasCollidedWithFence = false; // フェンス衝突フラグをリセット
		m_hasPassedHomeRunZone = false; // ホームランゾーン通過フラグをリセット
		hasCollidedWithGround = false; // 地面衝突フラグをリセット
		m_hasPassedFairFoulTrigger = false; // フェア/ファウル判定トリガー通過フラグをリセット
	}

	Wind::Instance().Update(elapsedTime);

	// ボールが転がり中（グラウンド着地済み・まだ判定前）のみ監視
	if (GetHasCollidedWithGround() &&
		!GetHasCollidedWithFence() && !GetHasBeenJudged())
	{
		physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
		if (ballCollider)
		{
			physx::PxVec3 ballPos = ballCollider->getGlobalPose().p;

			// z=19.5未満の間だけ監視（超えたらもうフェア確定ゾーン）
			if (ballPos.z < 19.5f)
			{
				bool isFair = (ballPos.z >= 0.0f) &&
					(std::fabs(ballPos.x) <= ballPos.z);

				if (!isFair)
				{
					// フェア範囲外に出た → ファウル確定
					// 二重判定防止のため地面衝突フラグで流用
					SetHasBeenJudged(true);

					char debugMessage[256];
					snprintf(debugMessage, sizeof(debugMessage),
						"ファウル：転がってファウルラインを越えた x=%.2f z=%.2f\n",
						ballPos.x, ballPos.z);
					OutputDebugStringA(debugMessage);
				}
			}
		}
	}

}

void Pitcher::UpdateBallCollider()
{
	Ball::Instance().UpdateCollider();
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
	return (Ball::Instance().GetWorldPosition().x >= strikeZoneMinX && Ball::Instance().GetWorldPosition().x <= strikeZoneMaxX) &&
		(Ball::Instance().GetWorldPosition().y >= strikeZoneMinY && Ball::Instance().GetWorldPosition().y <= strikeZoneMaxY) &&
		(Ball::Instance().GetWorldPosition().z >= strikeZoneMinZ && Ball::Instance().GetWorldPosition().z <= strikeZoneMaxZ);
}

// 描画
void Pitcher::Render(const RenderContext& rc, ModelRenderer* renderer) 
{
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	pitcher->render(rc.deviceContext, transform, animated_nodes);
	Ball::Instance().Render(rc, renderer, isBallThrown);

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

	
	Wind::Instance().Render(rc);
	
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
		Ball::Instance().DrawGUI();

		if (ImGui::CollapsingHeader("Pitch Settings"))
		{
			ImGui::DragFloat("Throw Timing", &throwTiming, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Ball Speed (km/h)", &ballSpeedKmh, 10.0f, 180.0f);
			ImGui::DragFloat("Launch Angle (deg)", &launchAngleDegrees, -20.0f, 10.0f);

			ImGui::Separator();
			ImGui::Text("Throw Direction");
			ImGui::DragFloat3("Throw Direction", &throwDirection.x, -1.0f, 1.0f);

			ImGui::Separator();
			ImGui::Text("Rotation Settings");
			ImGui::SliderFloat(u8"X軸回転速度 (サイドスピン)", &rotationSpeed.x, -150.0f, 150.0f);
			ImGui::SliderFloat(u8"Y軸回転速度", &rotationSpeed.y, -150.0f, 150.0f);
			ImGui::SliderFloat(u8"Z軸回転速度 (バック/トップスピン)", &rotationSpeed.z, -150.0f, 150.0f);

			ImGui::Separator();
			ImGui::Checkbox("Is Ball Thrown", &isBallThrown);

			float speedMs = ballSpeedKmh / 3.6f;
			ImGui::Text("Speed: %.2f m/s (%.0f km/h)", speedMs, ballSpeedKmh);
			
		}
	}
	Wind::Instance().DrawGUI();

	ImGui::End();
#endif
}

//アタッチメント処理
void Pitcher::AttachBallToHand(float elapsedTime)
{
	if (!isBallThrown)
	{
		Ball::Instance().AttachToHand(animated_nodes, transform, "mixamorig:RightHandMiddle1");
		ballStartPosition = Ball::Instance().GetStartPosition();
	}
	else
	{
		ApplyPhysicsToBall(elapsedTime);
		Ball::Instance().UpdateFromPhysics(elapsedTime, rotationSpeed);

		if (Ball::Instance().GetWorldPosition().y < 0.0f)
		{
			isBallThrown = false;
			hasCollided = false;
			animation_time = 0.0f;
			hasCollidedWithFence = false;
			hasCollidedWithGround = false;
			m_hasPassedFairFoulTrigger = false;
			m_hasPassedHomeRunZone = false;
			Ball::Instance().ResetMotion();
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
			throwCounter = 0.0f;
			hasReachedZero = false;
			SetHasCollided(false);
			SetHasCollidedWithFence(false);
			m_hasPassedHomeRunZone = false;
			hasCollidedWithGround = false;
			m_hasPassedFairFoulTrigger = false;

			float speedMs = ballSpeedKmh / 3.6f;
			float launchAngleRadians = DirectX::XMConvertToRadians(launchAngleDegrees);

			throwDirection.y = sinf(launchAngleRadians);
			throwDirection.z = -cosf(launchAngleRadians);

			DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&throwDirection);
			dir = DirectX::XMVector3Normalize(dir);
			DirectX::XMFLOAT3 normalizedDir;
			DirectX::XMStoreFloat3(&normalizedDir, dir);

			physx::PxVec3 initialVelocity(normalizedDir.x * speedMs, normalizedDir.y * speedMs, normalizedDir.z * speedMs);
			Ball::Instance().Throw(initialVelocity, GetSpinAxisFromPitchType());

			char debugMessage[128];
			snprintf(debugMessage, sizeof(debugMessage), "Throw Speed: %.2f km/h\n", initialVelocity.magnitude() * 3.6f);
			OutputDebugStringA(debugMessage);
		}

		pitcher->animate(current_animation_index, animation_time, animated_nodes);
	}
}
// ===== 新規追加: 球種から角速度を計算 =====
physx::PxVec3 Pitcher::GetSpinAxisFromPitchType() const
{
	const float RPM_TO_RAD_PER_SEC = 2.0f * 3.14159265f / 60.0f;

	switch (selectedPitchType)
	{
	case PitchType::Fastball:  // バックスピン
		return physx::PxVec3(2500.0f * RPM_TO_RAD_PER_SEC, 0.0f, 0.0f);

	case PitchType::Slider:  // サイドスピン＋少しバック
		return physx::PxVec3(0.0f, -2400.0f * RPM_TO_RAD_PER_SEC, 0.0f);

	case PitchType::Curveball:  // サイドスピン＋トップスピン
		return physx::PxVec3(-2500.0f * RPM_TO_RAD_PER_SEC, -1500.0f * RPM_TO_RAD_PER_SEC, 0.0f);

	case PitchType::Changeup:  // ミックススピン（弱い）
		return physx::PxVec3(1000.0f * RPM_TO_RAD_PER_SEC, 0.0f, 0.0f);

	case PitchType::Forkball:  // ほぼ回転なし
		return physx::PxVec3(100.0f * RPM_TO_RAD_PER_SEC, -100.0f * RPM_TO_RAD_PER_SEC, -100.0f * RPM_TO_RAD_PER_SEC);

	case PitchType::TwoSeam:  // バックスピン＋弱いサイド
		return physx::PxVec3(1500.0f * RPM_TO_RAD_PER_SEC, 1500.0f * RPM_TO_RAD_PER_SEC, 2000.0f * RPM_TO_RAD_PER_SEC);

	case PitchType::Cutter:  // サイドスピン強め
		return physx::PxVec3(500.0f * RPM_TO_RAD_PER_SEC, -2000.0f * RPM_TO_RAD_PER_SEC, 0.0f);

	case PitchType::Sinker:  // サイドスピン＋トップスピン
		return physx::PxVec3(100.0f * RPM_TO_RAD_PER_SEC, 2000.0f * RPM_TO_RAD_PER_SEC, -2000.0f * RPM_TO_RAD_PER_SEC);

	case PitchType::VerticalSlider:  // 純粋なサイドスピン
		return physx::PxVec3(0.0f, -1000.0f * RPM_TO_RAD_PER_SEC, -2000.0f * RPM_TO_RAD_PER_SEC);

	case PitchType::Splitter:  // 回転が少ない
		return physx::PxVec3(200.0f * RPM_TO_RAD_PER_SEC, -200.0f * RPM_TO_RAD_PER_SEC, -200.0f * RPM_TO_RAD_PER_SEC);

	case PitchType::SlowCurve:  // トップスピン強め
		return physx::PxVec3(0.0f, -1200.0f * RPM_TO_RAD_PER_SEC, 2000.0f * RPM_TO_RAD_PER_SEC);

	case PitchType::Shooter:  // サイドスピン最強
		return physx::PxVec3(500.0f * RPM_TO_RAD_PER_SEC, 1000.0f * RPM_TO_RAD_PER_SEC, -100.0f * RPM_TO_RAD_PER_SEC);

	case PitchType::Knuckleball:  // ほぼ回転なし
		return physx::PxVec3(50.0f * RPM_TO_RAD_PER_SEC, 50.0f * RPM_TO_RAD_PER_SEC, 50.0f * RPM_TO_RAD_PER_SEC);

	default:
		return physx::PxVec3(0.0f, 0.0f, 0.0f);
	}
}

void Pitcher::ApplyPhysicsToBall(float elapsedTime)
{
	physx::PxVec3 windVec(0.0f, 0.0f, 0.0f);
	if (Wind::Instance().IsBallInWindArea())
	{
		windVec = physx::PxVec3(Wind::Instance().GetWindVector().x, Wind::Instance().GetWindVector().y, Wind::Instance().GetWindVector().z);
	}

	Ball::Instance().ApplyPitchPhysics(selectedPitchType == PitchType::Knuckleball, windVec);
}
void Pitcher::SelectPitchType() 
{
	// 乱数生成
	float randomValue = GenerateRandomFloat(0.0f, 1.0f); // 0.0～1.0の乱数を生成
	selectedPitchType = PitchType::Fastball; // デフォルトはストレート

	// 球種ごとの挙動を設定
	switch (selectedPitchType)
	{
	case PitchType::Fastball: // ストレート
		
		ballSpeedKmh = 150.0f; // 速い
		Ball::Instance().GetBallAngle() = { 0.2f, DirectX::XMConvertToRadians(90.0f), 0.0f};
		rotationSpeed = { 0.0f, 0.0f, 150.0f }; // バックスピン
		throwDirection.x = 0.03f;
		launchAngleDegrees = -1.5f;
		OutputDebugStringA("Pitch Type: Fastball\n");
		break;

	case PitchType::Slider: // スライダー
		
		ballSpeedKmh = 130.0f;    // 少し遅い
		Ball::Instance().GetBallAngle() = { -0.2f, 0.0f, 0.0f };
		rotationSpeed = { 0.0f, 0.0f, 100.0f }; // サイドスピン
		throwDirection.x = 0.0f;
		launchAngleDegrees = 0.5f;
		OutputDebugStringA("Pitch Type: Slider\n");
		break;

	case PitchType::Curveball: // カーブ
		
		ballSpeedKmh = 110.0f;    // 遅い
		Ball::Instance().GetBallAngle() = { 0.5f, DirectX::XMConvertToRadians(90.0f), 0.0f };
		rotationSpeed = { 0.0f, 0.0f, -150.0f }; // トップスピン
		throwDirection.x = 0.01f;
		launchAngleDegrees = 4.0f; // カーブはやや下向きに投げる
		OutputDebugStringA("Pitch Type: Curveball\n");
		break;

	case PitchType::Changeup: // チェンジアップ
		
		ballSpeedKmh = 120.0f;    // 遅い
		rotationSpeed = { 0.0f, 0.0f, 100.0f }; // ミックス回転
		throwDirection.x = 0.03f;
		launchAngleDegrees = 0.0f; // カーブはやや下向きに投げる
		OutputDebugStringA("Pitch Type: Changeup\n");
		break;

	case PitchType::Forkball: // フォーク
		
		ballSpeedKmh = 130.0f;    // 少し遅い
		Ball::Instance().GetBallAngle().y = 0.0f;
		throwDirection.x = 0.03f;
		launchAngleDegrees = -0.5f; // カーブはやや下向きに投げる
		rotationSpeed = { 40.0f, 0.0f, -10.0f }; // 回転は少なめ
		OutputDebugStringA("Pitch Type: Forkball\n");
		break;

	case PitchType::TwoSeam: // ツーシーム
		
		ballSpeedKmh = 145.0f;    // 少し速い
		Ball::Instance().GetBallAngle().y = 0.0f;
		Ball::Instance().GetBallAngle().x = 0.2f;
		throwDirection.x = 0.03f;
		launchAngleDegrees = -0.5f; // カーブはやや下向きに投げる
		rotationSpeed = { 100.0f, 0.0f, 0.0f }; // 回転は少なめ
		OutputDebugStringA("Pitch Type: TwoSeam\n");
		break;

	case PitchType::Cutter: // カットボール
		
		ballSpeedKmh = 140.0f;    // 速い
		Ball::Instance().GetBallAngle() = { -0.2f, 0.0f, 0.0f };
		throwDirection.x = 0.0f;
		launchAngleDegrees = -0.5f;
		rotationSpeed = { 0.0f, 0.0f, 80.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: Cutter\n");
		break;

	case PitchType::Sinker: // シンカー
		
		ballSpeedKmh = 130.0f;    // 少し遅い
		throwDirection.x = 0.05f;
		launchAngleDegrees = 1.0f;
		rotationSpeed = { -120.0f, 0.0f, -120.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: Sinker\n");
		break;

	case PitchType::VerticalSlider: // 縦スライダー
		
		//ballSpeedKmh = 125.0f;    // 遅い
		Ball::Instance().GetBallAngle() = { -0.2f, 0.0f, 0.0f };
		rotationSpeed = { 0.0f, 0.0f, 100.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: VerticalSlider\n");
		break;

	case PitchType::Splitter: // スプリット
		
		ballSpeedKmh = 140.0f;    // 少し遅い
		Ball::Instance().GetBallAngle().y = 0.0f;
		throwDirection.x = 0.03f;
		launchAngleDegrees = -0.5f; // カーブはやや下向きに投げる
		rotationSpeed = { 40.0f, 0.0f, -10.0f }; // 回転速度
		OutputDebugStringA("Pitch Type: Splitter\n");
		break;

	case PitchType::SlowCurve: // スローカーブ
		
		//ballSpeedKmh = 80.0f;     // 非常に遅い
		Ball::Instance().GetBallAngle() = { 0.5f, DirectX::XMConvertToRadians(90.0f), 0.0f };
		rotationSpeed = { 0.0f, 0.0f, -150.0f }; // トップスピン
		OutputDebugStringA("Pitch Type: SlowCurve\n");
		break;

	case PitchType::Shooter: // シュート
		
		ballSpeedKmh = 145.0f;    // 遅い
		throwDirection.x = 0.05f;
		launchAngleDegrees = -1.0f;
		Ball::Instance().GetBallAngle() = { -0.2f, 0.0f, 0.0f };
		rotationSpeed = { 0.0f, 0.0f, 150.0f }; // 強いサイドスピン
		OutputDebugStringA("Pitch Type: Shooter\n");
		break;

	case PitchType::Knuckleball: // ナックルボール
		
		//ballSpeedKmh = 90.0f;     // 非常に遅い
		Ball::Instance().GetBallAngle() = { 0.0f, 0.0f, 0.0f };
		rotationSpeed = { -5.0f, 0.0f, -5.0f }; // 不規則な回転
		OutputDebugStringA("Pitch Type: Knuckleball\n");
		break;

	default:
		break;
	}
	// ランダムな投球方向を設定
	//throwDirection.x = GenerateRandomFloat(0.02f, 0.04f); // 左右方向のランダム値
	//throwDirection.x = 0.01f; // 左右方向のランダム値
	throwDirection.y = 0.2f; // 上下方向のランダム値
	throwDirection.z = -1.0f; // 前方向固定

	// ランダムな発射角度を設定
	//launchAngleDegrees = GenerateRandomFloat(-2.0f, 0.0f); // -4度から-2度の範囲でランダム
}
