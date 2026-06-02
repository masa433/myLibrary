#include "Pitcher.h"
#include "imgui.h"
#include <Windows.h>
#include "Graphics.h"
#include <algorithm>
#include "scene_game.h"
#include <random>
#include "PrimitiveRenderer.h"
#include "camera.h"

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

	// 風表現用の流線を生成
	windLines.clear();
	windLines.reserve(100);
	for (int i = 0; i < 100; ++i)
	{
		const float t = static_cast<float>(i);
		WindLine line{};
		line.position = {
			-30.0f + std::fmod(t * 7.3f, 60.0f),
			0.0f,
			-5.0f + std::fmod(t * 5.1f, 100.0f)
		};
		// Y軸の相対的な位置割合(0.0 ～ 1.0)を決定して保存する
		line.baseYOffset = std::fmod(t * 1.7f, 1.0f);

		line.speed = windStrength * (0.6f + std::fmod(t * 0.37f, 1.0f));
		line.length = 1.5f + std::fmod(t * 0.23f, 2.0f);
		line.phase = t * 0.4f;
		windLines.push_back(line);
	}

	windHeight = 20.0f; // 風の流線の高さ
	windThickness = 50.0f; // 風の流線の厚み

	//スプライトの初期化
	windDirectionSprite = std::make_unique<Sprite>();
	windDirectionSprite->texturePath = L".\\resources\\textures\\windDirection.png";
	windDirectionSprite->position = { 1150.0f, 100.0f };
	windDirectionSprite->size = { 50.0f, 70.0f };
	windDirectionSprite->rotation = 0.0f;
	windDirectionSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };

	windDirectionSpriteRenderer = std::make_unique<sprite>(device, windDirectionSprite->texturePath.c_str());

	windGroundSprite = std::make_unique<Sprite>();
	windGroundSprite->texturePath = L".\\resources\\textures\\ground.png";
	windGroundSprite->position = { 1100.0f, 100.0f };
	windGroundSprite->size = { 150.0f, 100.0f };
	windGroundSprite->rotation = 0.0f;
	windGroundSprite->color = { 1.0f, 1.0f, 1.0f, 0.9f };

	windGroundSpriteRenderer = std::make_unique<sprite>(device, windGroundSprite->texturePath.c_str());

	windStrengthFontRenderer = std::make_unique<sprite>(device, L".\\resources\\fonts\\font6.png");
}

void Pitcher::Uninitialize() 
{
	Ball::Instance().Uninitialize();
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

	// ボールの軌跡を記録
	if (isBallThrown)
	{
		trailRecordTimer += elapsedTime;
		if (trailRecordTimer >= TrailRecordInterval)
		{
			trailRecordTimer = 0.0f;
			ballTrail.push_back(Ball::Instance().GetWorldPosition());
			if (ballTrail.size() > MaxTrailLength)
			{
				ballTrail.pop_front();
			}
		}
	}
	else
	{
		// 投げられていない時は軌跡をクリア
		ballTrail.clear();
	}

	// ボールが地面に落ちたらリセット
	if (Ball::Instance().GetWorldPosition().y < 0.0f)
	{
		isBallThrown = false;
		hasBeenJudged = false; // 判定フラグをリセット
		hasCollided = false; // 衝突フラグをリセット
		hasCollidedWithFence = false; // フェンス衝突フラグをリセット
		ballTrail.clear(); // 軌跡をクリア
	}


	for (auto& line : windLines)
	{
		line.position.x += windDirection.x * line.speed * elapsedTime;
		line.baseYOffset += (windDirection.y * line.speed * elapsedTime) / (windThickness > 0.01f ? windThickness : 0.01f);
		line.position.z += windDirection.z * line.speed * elapsedTime;
		line.phase += elapsedTime * 4.0f;

		// 画面外に出たらループさせる (X軸とZ軸)
		if (line.position.x > 100.0f) line.position.x -= 200.0f;
		else if (line.position.x < -100.0f) line.position.x += 200.0f;

		if (line.position.z > 100.0f) line.position.z -= 105.0f;
		else if (line.position.z < -5.0f) line.position.z += 105.0f;

		// Y軸(上下)の相対範囲ループ (0.0 ～ 1.0)
		if (line.baseYOffset > 1.0f) line.baseYOffset -= 1.0f;
		else if (line.baseYOffset < 0.0f) line.baseYOffset += 1.0f;

		// 実際のY座標を計算して更新
		line.position.y = line.baseYOffset * windThickness;
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

bool Pitcher::IsBallInWindArea() const
{
	// 流線の描画範囲に合わせて風の有効範囲を定義
	if (Ball::Instance().GetWorldPosition().x < -100.0f || Ball::Instance().GetWorldPosition().x > 100.0f) return false;
	if (Ball::Instance().GetWorldPosition().y < windHeight || Ball::Instance().GetWorldPosition().y > windHeight + windThickness) return false;
	if (Ball::Instance().GetWorldPosition().z < -5.0f || Ball::Instance().GetWorldPosition().z > 95.0f) return false;

	return true;
}

// 描画
void Pitcher::Render(const RenderContext& rc, ModelRenderer* renderer) 
{
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	// ← 最初にモデル用の深度ステートを設定
	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);



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

	// トレイルの描画
	if (ballTrail.size() > 1)
	{
		

		// 現在アクティブなカメラインスタンスのView/Projection行列を取得して利用する
		Camera& camera = Camera::Instance(); // シングルトンなどから取得

		// 軌跡の色（赤から白へグラデーションなど）
		DirectX::XMFLOAT4 trailColor = { 1.0f, 0.5f, 0.0f, 1.0f };

		for (size_t i = 0; i < ballTrail.size() - 1; ++i)
		{
			// 古いほど薄くするアルファ値の計算
			float alpha = static_cast<float>(i) / ballTrail.size();
			DirectX::XMFLOAT4 color = { trailColor.x, trailColor.y, trailColor.z, alpha };

			primitiveRenderer->AddVertex(ballTrail[i], color);
			primitiveRenderer->AddVertex(ballTrail[i + 1], color);
		}

		// 線の描画を実行（ラインリスト指定）
		primitiveRenderer->Render(rc.deviceContext, camera.GetView(), camera.GetProjection(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}
	
	// 風の描画
	for (const auto& line : windLines)
	{
		DirectX::XMFLOAT3 start = line.position;
		start.y += windHeight; // 風の高さを加算
		DirectX::XMFLOAT3 end = {
			line.position.x - windDirection.x * line.length,
			(line.position.y + windHeight) - windDirection.y * line.length,
			line.position.z - windDirection.z * line.length
		};

		DirectX::XMFLOAT4 color = { 0.8f, 0.9f, 1.0f, 0.35f };

		primitiveRenderer->AddVertex(start, color);
		primitiveRenderer->AddVertex(end, color);
	}

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0); // 書き込みなし

	if (windGroundSprite && windGroundSpriteRenderer)
	{
		windGroundSpriteRenderer->render(rc.deviceContext, windGroundSprite->position.x, windGroundSprite->position.y,
			windGroundSprite->size.x, windGroundSprite->size.y,
			windGroundSprite->color.x, windGroundSprite->color.y, windGroundSprite->color.z, windGroundSprite->color.w,
			0.0f);
	}

	// 風向きスプライトの描画
	if (windDirectionSprite && windDirectionSpriteRenderer)
	{
		windDirectionSprite->rotation = atan2f(windDirection.x, windDirection.z); // 風向きに合わせて回転
		windDirectionSpriteRenderer->render(rc.deviceContext, windDirectionSprite->position.x, windDirectionSprite->position.y,
			windDirectionSprite->size.x, windDirectionSprite->size.y,
			windDirectionSprite->color.x, windDirectionSprite->color.y, windDirectionSprite->color.z, windDirectionSprite->color.w,
			DirectX::XMConvertToDegrees(windDirectionSprite->rotation));

		
	}

	if (windStrengthFontRenderer)
	{
		physx::PxVec3 windVec(windDirection.x * windStrength, windDirection.y * windStrength, windDirection.z * windStrength);
		float currentWindSpeed = windVec.magnitude();

		char speedText[64];
		snprintf(speedText, sizeof(speedText), "%.fm", currentWindSpeed);

		// アイコンの座標に基づいてテキスト位置を決定
		float textX = windDirectionSprite->position.x + 60.0f;
		float textY = windDirectionSprite->position.y + 15.0f;

		// 文字描画 (文字の幅と高さを適当なサイズで指定。例: 16x32 や 20x40 など適宜調整)
		windStrengthFontRenderer->textout(rc.deviceContext, speedText,
			textX, textY,
			16.0f, 32.0f,
			1.0f, 1.0f, 1.0f, 1.0f);
	}

	// 描画後に元に戻す
	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
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
			ImGui::DragFloat("Trail Width", &trailWidth, 0.01f, 0.01f, 1.0f, "%.2f");
			ImGui::DragFloat("MaxTrailLength", &MaxTrailLength, 0.01f, 0.01f, 1.0f, "%.2f");
		}
	}
	
	if (ImGui::CollapsingHeader("Wind Settings"))
	{
		// 風向の操作
		ImGui::DragFloat3("Wind Direction", &windDirection.x, 0.01f, -1.0f, 1.0f);
		if (ImGui::Button("Normalize Wind Direction"))
		{
			DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&windDirection);
			// ゼロベクトルの場合は正規化しない
			if (DirectX::XMVector3NotEqual(dir, DirectX::XMVectorZero()))
			{
				dir = DirectX::XMVector3Normalize(dir);
				DirectX::XMStoreFloat3(&windDirection, dir);
			}
		}

		// 風の強さの操作
		ImGui::DragFloat("Wind Strength", &windStrength, 0.1f, 0.0f, 50.0f);

		// 風の基本高さの操作
		ImGui::DragFloat("Wind Height", &windHeight, 0.1f, -10.0f, 50.0f);

		// 風の厚みの操作
		ImGui::DragFloat("Wind Thickness", &windThickness, 0.1f, 0.1f, 100.0f);

		// 流線の描画などに強さの変更を即時反映させるため、表示用に現在の風ベクトルも表示する
		ImGui::Text("Current Wind Velocity: (%.2f, %.2f, %.2f)",
			windDirection.x * windStrength,
			windDirection.y * windStrength,
			windDirection.z * windStrength);
	}

	//スプライトのデバッグ表示
	if(ImGui::CollapsingHeader("Sprite Debug"))
	{
		if (windDirectionSprite)
		{
			ImGui::DragFloat2("Wind Direction Sprite Position", &windDirectionSprite->position.x, 1.0f, 0.0f, 1280.0f);
			ImGui::DragFloat2("Wind Direction Sprite Size", &windDirectionSprite->size.x, 1.0f, 1.0f, 500.0f);
			ImGui::DragFloat("Wind Direction Sprite Rotation", &windDirectionSprite->rotation, 1.0f, 0.0f, 360.0f);
			ImGui::ColorEdit4("Wind Direction Sprite Color", &windDirectionSprite->color.x);
		}
		ImGui::Separator();
		if(windGroundSprite)
		{
			ImGui::DragFloat2("Wind Ground Sprite Position", &windGroundSprite->position.x, 1.0f, 0.0f, 1280.0f);
			ImGui::DragFloat2("Wind Ground Sprite Size", &windGroundSprite->size.x, 1.0f, 1.0f, 500.0f);
			ImGui::ColorEdit4("Wind Ground Sprite Color", &windGroundSprite->color.x);
		}
	}

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
	if (IsBallInWindArea())
	{
		windVec = physx::PxVec3(windDirection.x * windStrength, windDirection.y * windStrength, windDirection.z * windStrength);
	}

	Ball::Instance().ApplyPitchPhysics(selectedPitchType == PitchType::Knuckleball, windVec);
}
void Pitcher::SelectPitchType() 
{
	// 乱数生成
	float randomValue = GenerateRandomFloat(0.0f, 1.0f); // 0.0～1.0の乱数を生成
	selectedPitchType = PitchType::Slider; // デフォルトはストレート

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
