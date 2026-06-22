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
#include "Ball.h"

// ランダムな浮動小数点数を生成する関数
float GenerateRandomFloat(float min, float max)
{
	static std::random_device rd; // ランダムデバイス
	static std::mt19937 gen(rd()); // メルセンヌ・ツイスタ
	std::uniform_real_distribution<float> dis(min, max); // 一様分布
	return dis(gen);
}

// 初期化
void Pitcher::Initialize() 
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//モデルの読み込み
	if(IsRightPitcher())
	{
		pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\rightPitcher.glb");
	}
	else
	{
		pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\leftPitcher.glb");
	}

	position = { -0.1f,0.22f,18.15f };
	scale = { 1.0f,1.0f,1.0f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f};

	// アニメーション用のノードをコピー
	animated_nodes = pitcher->nodes;

	Ball::Instance().Initialize();

	Wind::Instance().Initialize();

	boxPosition = { 0.0f, 0.8f, 0.0f }; // ストライクゾーンの位置を設定
	boxSize = { 0.43f, 0.6f, 0.2f }; // ストライクゾーンのサイズを設定

	pitcher->build_static_batches(device);

	// ホームラン判定用トリガーの作成
	{
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxScene* pxScene = Physics::Instance().GetScene();

		physx::PxMaterial* triggerMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.5f);
		physx::PxTransform triggerTransform(physx::PxVec3(boxPosition.x, boxPosition.y, boxPosition.z));
		strikeZoneTrigger = pxPhysics->createRigidStatic(triggerTransform);

		physx::PxBoxGeometry triggerGeometry(physx::PxVec3(boxSize.x / 2.0f, boxSize.y / 2.0f, boxSize.z / 2.0f));
		physx::PxShape* triggerShape = physx::PxRigidActorExt::createExclusiveShape(*strikeZoneTrigger, triggerGeometry, *triggerMaterial);

		// 物理的な衝突を無効にし、トリガー（重なり判定）として設定する
		triggerShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		triggerShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);

		strikeZoneTrigger->setName("StrikeZoneTrigger");
		pxScene->addActor(*strikeZoneTrigger);

		
	}
	
	InitializePitchSettings();
	SelectPitchType();

}

//ピッチセッティングの初期化
void Pitcher::InitializePitchSettings()
{
	pitchParameters.resize(PITCH_TYPE_COUNT); // 球種の数に合わせてリサイズ
	// 各球種のパラメーターを設定
	//左から投球速度(km/h), 発射角度(度), 投球方向, 回転軸, 回転数(rpm), 見た目の回転速度(度/秒), 見た目の角度(度)
	pitchParameters[static_cast<int>(PitchType::Fastball)] = { 150.0f, -2.5f, { 0.0f, 0.0f, 0.0f }, { 0.02f, 0.2f, -1.0f }, 2500.0f, { 2500.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Slider)] = { 140.0f, -3.0f, { 0.0f, 200.0f, 0.0f }, { -0.02f, 0.2f, -1.0f }, 2200.0f, { 0.0f, -2200.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Curveball)] = { 130.0f, -5.0f, { 300.0f, 0.0f, 0.0f }, { 0.02f, 0.1f, -1.0f }, 1800.0f, { -1800.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Changeup)] = { 120.0f, -2.5f, { 100.0f, 100.0f, 100.0f }, { 0.02f, 0.2f, -1.0f }, 1500.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Forkball)] = { 110.0f, -6.5f, { 400.0f, 100.0f, 100.0f }, { -0.02f, 0.1f, -1.0f }, 1200.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::TwoSeam)] = { 145.0f, -2.5f, { 200.0f, 50.0f, 50.0f }, { -0.02f, 0.2f, -1.02f }, 2300.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Cutter)] = { 135.0f, -2.5f, { 0.0f, 300.0f, 0.0f }, { 0.02f, 0.2f, -1.02f }, 2000.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Sinker)] = { 140.0f, -4.0f, { 300.0f, 100.0f, 100.0f }, { -0.02f, 0.1f, -1.02f }, 2200.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::VerticalSlider)] = { 130.0f, -3.5f, { 200.0f, 0.0f, 0.0f }, { 0.02f, 0.1f, -1.0f }, 1800.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Splitter)] = { 120.0f, -6.0f, { 400.0f, 200.0f, 100.0f }, { -0.02f, 0.1f, -1.02f }, 1200.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::SlowCurve)] = { 100.0f, -8.0f, { 500.0f, 0.0f, 0.0f }, { 0.02f, 0.05f, -1.0f }, 800.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Shooter)] = { 130.0f, -2.5f, { 0.0f, 0.0f, 300.0f }, { 0.02f, 0.2f, -1.02f }, 1800.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::Knuckleball)] = { 90.0f, -2.5f, { 0.0f, 0.0f, 0.0f }, { 0.02f, 0.2f, -1.0f }, 500.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	pitchParameters[static_cast<int>(PitchType::SlowBall)] = { 70.0f, 4.0f, { 0.0f, 0.0f, 0.0f }, { 0.02f, 0.2f, -1.0f }, 300.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
}

void Pitcher::Uninitialize() 
{
	Ball::Instance().Uninitialize();
	Wind::Instance().Uninitialize();
	if (strikeZoneTrigger)
	{
		physx::PxScene* pxScene = Physics::Instance().GetScene();
		pxScene->removeActor(*strikeZoneTrigger);
		strikeZoneTrigger->release();
		strikeZoneTrigger = nullptr;
	}
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
			SelectPitchTypeByAI(); // 球種選択
			OutputDebugStringA("Forced Throw: Backspace pressed\n");

			if(consoleLog)
			{
				char debugMessage[256];
				snprintf(debugMessage, sizeof(debugMessage), "[Info] Forced Throw: Backspace pressed\n");
				consoleLog->push_back(debugMessage);
			}
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
			hasReachedZero = false;
			throwCounter = 0.0f;
			SelectPitchTypeByAI(); // 球種選択
			Ball::Instance().SetHasBeenJudged(false); // 判定フラグをリセット
			Ball::Instance().SetHasCollided(false); // 衝突フラグをリセット
			Ball::Instance().SetHasCollidedWithFence(false); // フェンス衝突フラグをリセット
			Ball::Instance().SetHasPassedHomeRunZone(false); // ホームランゾーン通過フラグをリセット
			Ball::Instance().SetHasCollidedWithGround(false); // 地面衝突フラグをリセット
			Ball::Instance().SetHasPassedFairFoulTrigger(false); // フェア/ファウル判定トリガー通過フラグをリセット
			Ball::Instance().SetFoulLogged(false); // ファウルログフラグをリセット
			Ball::Instance().SetThroughStrikeZone(false); // ストライクゾーン通過フラグをリセット
			OutputDebugStringA("Judgment reset\n");
			if(consoleLog)
			{
				 char debugMessage[256];
				 snprintf(debugMessage, sizeof(debugMessage), "[Info] Judgment reset\n");
				 consoleLog->push_back(debugMessage);
			}
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
			//Ball::Instance().SetHasBeenJudged(false); // 判定フラグをリセット
		}
		break;
	}

	// 共通の更新処理
	UpdateTransform();
	UpdateBallCollider();
	AttachBallToHand(elapsedTime);

	
	//ストライク・ボール判定用のz=0.0f到達監視
	if (isBallThrown)
	{
		throwCounter += elapsedTime;

		if (!hasReachedZero && Ball::Instance().GetWorldPosition().z <= 0.0f)
		{
			physx::PxRigidDynamic* ballCollider = Ball::Instance().GetBallCollider();
			if (ballCollider)
			{
				hasReachedZero = true;

				char debugMessage[256];
				if (Ball::Instance().GetThroughStrikeZone())
				{
					snprintf(debugMessage, sizeof(debugMessage), u8"[Info] ストライク！\n");
				}
				else
				{
					snprintf(debugMessage, sizeof(debugMessage), u8"[Info] ボール！\n");
				}
				if (consoleLog)
				{
					consoleLog->push_back(debugMessage);
				}

				char timeMessage[128];
				snprintf(timeMessage, sizeof(timeMessage),
					"Time to reach z=0.0f: %.2f seconds\n", throwCounter);
				OutputDebugStringA(timeMessage);

				if (consoleLog)
				{
					char timeMessage[128];
					snprintf(timeMessage, sizeof(timeMessage),
						u8"[Info] Time to reach z=0.0f: %.2f seconds\n", throwCounter);
					consoleLog->push_back(timeMessage);
				}
			}
		}
	}

	// ボールが地面に落ちたらリセット
	if (Ball::Instance().GetWorldPosition().y < 0.0f)
	{
		isBallThrown = false;
		Ball::Instance().SetHasBeenJudged(false); // 判定フラグをリセット
		Ball::Instance().SetHasCollided(false); // 衝突フラグをリセット
		Ball::Instance().SetHasCollidedWithFence(false); // フェンス衝突フラグをリセット
		Ball::Instance().SetHasPassedHomeRunZone(false); // ホームランゾーン通過フラグをリセット
		Ball::Instance().SetHasCollidedWithGround(false); // 地面衝突フラグをリセット
		Ball::Instance().SetHasPassedFairFoulTrigger(false); // フェア/ファウル判定トリガー通過フラグをリセット
		Ball::Instance().SetFoulLogged(false); // ファウルログフラグをリセット
		Ball::Instance().SetThroughStrikeZone(false); // ストライクゾーン通過フラグをリセット
	}

	Wind::Instance().Update(elapsedTime);

	// ボールが転がり中（グラウンド着地済み・まだ判定前）のみ監視
	if (Ball::Instance().GetHasCollidedWithGround() &&
		!Ball::Instance().GetHasCollidedWithFence() && !Ball::Instance().GetHasBeenJudged() && !Ball::Instance().GetFoulLogged())
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
					Ball::Instance().SetHasBeenJudged(true);

					Ball::Instance().SetFoulLogged(true); // ファウルログフラグを設定

					char debugMessage[256];
					snprintf(debugMessage, sizeof(debugMessage),
						u8"ファウル：転がってファウルラインを越えた x=%.2f z=%.2f\n",
						ballPos.x, ballPos.z);
					OutputDebugStringA(debugMessage);
					if (consoleLog)
					{
						consoleLog->push_back(debugMessage);
					}
				}
			}
		}
	}
}

void Pitcher::UpdateBallCollider()
{
	Ball::Instance().UpdateCollider();
}



// 描画
void Pitcher::Render(const RenderContext& rc, ModelRenderer* renderer) 
{
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	pitcher->render_batched(rc.deviceContext, transform, animated_nodes);
	Ball::Instance().Render(rc, renderer, isBallThrown);

	
	Wind::Instance().Render(rc);
	
}

void Pitcher::DrawGUI()
{
#ifdef USE_IMGUI
	
		ImGui::Text("Current State: %s",
			currentState == State::SelectingPitch ? "Selecting Pitch" :
			currentState == State::Throwing ? "Throwing" : "Idle");
		ImGui::Text("State Time: %.2f seconds", stateTime);

		if (ImGui::CollapsingHeader(u8"ストライクゾーン"))
		{
			ImGui::DragFloat3("StrikeZone Position", &boxPosition.x, 0.1f);
			ImGui::DragFloat3("StrikeZone Size", &boxSize.x, 0.1f);

			if (strikeZoneTrigger)
			{
				// 位置の更新
				physx::PxTransform transform(physx::PxVec3(boxPosition.x, boxPosition.y, boxPosition.z));
				strikeZoneTrigger->setGlobalPose(transform);
				// サイズの更新
				physx::PxShape* shape = nullptr;
				strikeZoneTrigger->getShapes(&shape, 1);
				if (shape)
				{
					shape->setGeometry(physx::PxBoxGeometry(boxSize.x / 2.0f, boxSize.y / 2.0f, boxSize.z / 2.0f));
				}
			}
		}
		if (ImGui::CollapsingHeader("Pitcher Animation Control"))
		{
			ImGui::DragFloat3("Position", &position.x);
			ImGui::DragFloat3("Scale", &scale.x);
			ImGui::DragFloat3("Angle", &angle.x);
			ImGui::Checkbox("Play Animation", &animation_playing);
		}
		Ball::Instance().DrawGUI();

		if (ImGui::CollapsingHeader(u8"配球AI"))
		{
			ImGui::Checkbox(u8"AI配球を使う", &usePitchAI);
			ImGui::SliderFloat(u8"ストライク率", &aiStrikeRate, 0.0f, 1.0f, "%.2f");
			ImGui::DragFloat(u8"少し外す幅", &aiNearBallMargin, 0.005f, 0.0f, 0.20f, "%.3f");
			ImGui::Text(u8"現在: %s %.1f km/h", GetPitchTypeName(selectedPitchType), ballSpeedKmh);
		}

		if (ImGui::CollapsingHeader(u8"球種エディター"))
		{
			const char* pitchTypeNames[] = {
				u8"ストレート", u8"スライダー", u8"カーブ", u8"チェンジアップ", u8"フォーク",
				u8"ツーシーム", u8"カットボール", IsRightPitcher() ? u8"シンカー" : u8"スクリュー", u8"縦スライダー", u8"スプリット",
				u8"スローカーブ", u8"シュート", u8"ナックルボール", u8"スローボール"
			};

			//現在選択されている球種を基準に編集
			int editingPitchIndex = static_cast<int>(selectedPitchType);
			if (ImGui::Combo(u8"編集する球種", &editingPitchIndex, pitchTypeNames, IM_ARRAYSIZE(pitchTypeNames)))// 選択された球種を編集するためのコンボボックス
			{
				selectedPitchType = static_cast<PitchType>(editingPitchIndex);// 選択された球種を更新
				SelectPitchType(); // 球種選択
			}
			ImGui::Separator();
			ImGui::Spacing();

			if (editingPitchIndex >= 0 && editingPitchIndex < static_cast<int>(pitchParameters.size()))

			{

				PitchParameter& p = pitchParameters[editingPitchIndex];

				// 各種パラメータのスライダー
				ImGui::DragFloat(u8"球速 (km/h)", &p.ballSpeedKmh, 0.5f, 60.0f, 180.0f, "%.1f km/h");
				ImGui::DragFloat(u8"リリース角度 (度)", &p.launchAngleDegrees, 0.1f, -10.0f, 10.0f, "%.1f deg");
				ImGui::DragFloat3(u8"投球方向微調整", &p.throwDirection.x, 0.005f, -1.0f, 1.0f);
				ImGui::DragFloat(u8"回転数 (RPM)", &p.rpm, 10.0f, 0.0f, 3500.0f, "%.0f RPM");
				ImGui::Text(u8"【モデル回転速度（見た目専用、deg/s）】");
				ImGui::DragFloat(u8"Visual Rot X (バックスピン)", &p.visualRotationSpeed.x, 10.0f, -3600.0f, 3600.0f, "%.0f");
				ImGui::DragFloat(u8"Visual Rot Y (サイドスピン)", &p.visualRotationSpeed.y, 10.0f, -3600.0f, 3600.0f, "%.0f");
				ImGui::DragFloat(u8"Visual Rot Z (ジャイロ)", &p.visualRotationSpeed.z, 10.0f, -3600.0f, 3600.0f, "%.0f");
				ImGui::DragFloat3(u8"Visual Angle (deg)", &p.visualAngle.x, 1.0f, -360.0f, 360.0f);

				ImGui::Spacing();
				ImGui::Text(u8"【回転軸の設定】");

				// 軸の各成分をスライダーで調整
				bool axisChanged = false;
				axisChanged |= ImGui::SliderFloat(u8"Axis X (ホップ/ドロップ)", &p.spinAxis.x, -1.0f, 1.0f, "%.2f");
				axisChanged |= ImGui::SliderFloat(u8"Axis Y (シュート/スライダー)", &p.spinAxis.y, -1.0f, 1.0f, "%.2f");
				axisChanged |= ImGui::SliderFloat(u8"Axis Z (ジャイロ成分)", &p.spinAxis.z, -1.0f, 1.0f, "%.2f");

				// 軸が変更されたら常に正規化（長さを1にする）して方向を維持する
				if (axisChanged) {
					DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&p.spinAxis);
					if (DirectX::XMVector3Length(v).m128_f32[0] > 0.001f) {
						v = DirectX::XMVector3Normalize(v);
						DirectX::XMStoreFloat3(&p.spinAxis, v);
					}
				}

				ImGui::Spacing();
				ImGui::Text(u8"回転軸の3D立体視覚化");

				//ImGuiのDrawListを使って3Dグラフィック表示
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				ImVec2 center = ImGui::GetCursorScreenPos();
				center.x += 70.0f, center.y += 70.0f;// 中心位置を調整
				float radius = 55.0f; // 半径

				//立体球体の背景
				drawList->AddCircleFilled(center,radius,IM_COL32(50,50,50,255));

				//投影結果をまとめて返すための構造体
				struct Proj3D { ImVec2 pos; float depth; };

				//3D空間から2D画面への簡易投影ラムダ関数
				// 視角を少し斜め上（X軸を約25度、Y軸を約30度回転）に傾けて立体感を出す
				auto Project3DTo2D = [&](float x, float y, float z) -> Proj3D
				{
					//3D回転の簡易適用
						const float cosP = 0.906f, sinP = 0.422f; // X軸回転（約25度）
						const float cosY = -0.866f, sinY = 0.5f;   // Y軸回転（約30度）

						//Y軸回転
						float x1 = x * cosY + z * sinY;// Y軸回転
						float z1 = -x * sinY + z * cosY;// Y軸回転

						//X軸回転
						float y2 = y * cosP - z1 * sinP;
						float z2 = y * sinP + z1 * cosP;

						// 2D投影（簡易的にX軸とY軸をそのまま使用）
						return Proj3D{ ImVec2(center.x + x1, center.y - y2), z2 /*深度情報としてZ軸回転後の値を使用*/ };
				};

				//立体感を出すワイヤーフレームの描画
				const int segments = 32;
				ImVec2 prevPtH, prevPtV;
				for (int i = 0; i <= segments; ++i)
				{
					float theta = (i * 2.0f * 3.14159265f) / segments;// 0～2π

					//横方向の輪郭
					ImVec2 ptH = Project3DTo2D(cosf(theta) * radius, 0.0f, sinf(theta) * radius).pos;

					//縦方向の輪郭
					ImVec2 ptV = Project3DTo2D(0.0f, cosf(theta) * radius, sinf(theta) * radius).pos;

					if (i > 0) {
						drawList->AddLine(prevPtH, ptH, IM_COL32(110, 110, 120, 255), 1.0f);// 横方向の輪郭線
						drawList->AddLine(prevPtV, ptV, IM_COL32(110, 110, 120, 255), 1.0f);// 縦方向の輪郭線
					}
					prevPtH = ptH;// 前の点を更新
					prevPtV = ptV;// 前の点を更新
				}

				//外枠の輪郭
				drawList->AddCircle(center, radius, IM_COL32(240, 240, 240, 255), 0, 2.0f);// 外枠の輪郭線

				//3D回転軸ベクトルの計算と描画
				float axisLen = sqrtf(p.spinAxis.x * p.spinAxis.x + p.spinAxis.y * p.spinAxis.y + p.spinAxis.z * p.spinAxis.z);

				if (axisLen > 0.001f)
				{
					// 回転軸の方向を正規化
					float axisX = p.spinAxis.x / axisLen;
					float axisY = p.spinAxis.y / axisLen;
					float axisZ = p.spinAxis.z / axisLen;
					
					//球体を突き抜けるように回転軸を描画
					float arrowLength = radius * 1.4f; // 矢印の長さ
					ImVec2 axisStart = Project3DTo2D(-axisX * arrowLength, -axisY * arrowLength, -axisZ * arrowLength).pos;
					ImVec2 axisEnd = Project3DTo2D(axisX * arrowLength, axisY * arrowLength, axisZ * arrowLength).pos;

					//回転軸を太線で描画
					drawList->AddLine(axisStart, axisEnd, IM_COL32(255, 60, 60, 255), 3.0f);

					//矢印の先端に黄色いピンヘッドを配置
					drawList->AddCircleFilled(axisEnd, 5.0f, IM_COL32(255, 255, 0, 255));

					//軸の後端に少し小さなピンを配置して前後をわかりやすく
					drawList->AddCircleFilled(axisStart, 3.0f, IM_COL32(200, 50, 50, 255));

					//ボールの周りを矢印が回る処理

					//軸に垂直な平面の正規直交基底を計算
					DirectX::XMVECTOR axisVec = DirectX::XMVectorSet(axisX, axisY, axisZ, 0.0f);

					// 回転軸がほぼY軸に近い場合はX軸を、そうでない場合はY軸を補助ベクトルとして使用して垂直な平面を定義
					DirectX::XMVECTOR helper = (fabsf(axisY) < 0.95f) ? DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) : DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
					DirectX::XMVECTOR uVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(helper, axisVec)); // 軸に垂直なベクトルU
					DirectX::XMVECTOR vVec = DirectX::XMVector3Cross(axisVec, uVec); // 軸に垂直なベクトルV

					DirectX::XMFLOAT3 u, v;
					DirectX::XMStoreFloat3(&u, uVec);
					DirectX::XMStoreFloat3(&v, vVec);

					const float orbitRadius = radius * 1.1f;// ボールの周りを回る矢印の半径
					const float angularSpeed = 2.2f;// 回転速度（ラジアン/秒）
					const float t = (float)ImGui::GetTime();// 経過時間を取得

					//軌跡上の点を計算するヘルパーラムダ関数
					auto OrbitPoint = [&](float angle) -> Proj3D
					{
							float px = (cosf(-angle) * u.x + sinf(-angle) * v.x) * orbitRadius;
							float py = (cosf(-angle) * u.y + sinf(-angle) * v.y) * orbitRadius;
							float pz = (cosf(-angle) * u.z + sinf(-angle) * v.z) * orbitRadius;
							return Project3DTo2D(px, py, pz);
					};

					//軌跡の描画
					const int trailCount = 10;// 軌跡の点の数
					for (int i = trailCount; i >= 1; --i)
					{
						float trailAngle = t * angularSpeed - (i * 0.18f);// 過去の点を少しずつ遅らせる
						Proj3D trailPt = OrbitPoint(trailAngle);// 軌跡の点の位置を計算

						bool isFront = trailPt.depth >= 0.0f; // 視点から見て前か後ろかを判定
						float fade = 1.0f - (float)i / trailCount; // 後ろの点ほど透明にする
						int alpha = (int)(fade * (isFront ? 220.0f : 80.0f)); // 裏側はさらに薄く＝奥行き感

						float dotSize = 2.0f + fade * 2.5f; // 後ろの点ほど小さくする

						drawList->AddCircleFilled(trailPt.pos, dotSize, IM_COL32(80, 210, 255, alpha));
					}

					//矢印の先端を描画
					float headAngle = t * angularSpeed;
					Proj3D headPt = OrbitPoint(headAngle);

					//進行方向を求める
					Proj3D ahead = OrbitPoint(headAngle + 0.12f);// 少し先の点を計算
					ImVec2 dir = ImVec2(ahead.pos.x - headPt.pos.x, ahead.pos.y - headPt.pos.y);

					//矢印の向きを正規化
					float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
					if (dirLen > 0.0001f)
					{
						dir.x /= dirLen;
						dir.y /= dirLen;
					}

					//矢印の先端を描画
					bool headIsFront = headPt.depth >= 0.0f;
					ImU32 arrowColor = IM_COL32(80, 220, 255, headIsFront ? 255 : 130);

					// 進行方向を向いた三角形（矢印）を描く
					ImVec2 tip = ImVec2(headPt.pos.x + dir.x * 8.0f, headPt.pos.y + dir.y * 8.0f);
					ImVec2 left = ImVec2(headPt.pos.x - dir.x * 6.0f - dir.y * 6.0f, headPt.pos.y - dir.y * 6.0f + dir.x * 6.0f);
					ImVec2 right = ImVec2(headPt.pos.x - dir.x * 6.0f + dir.y * 6.0f, headPt.pos.y - dir.y * 6.0f - dir.x * 6.0f);
					drawList->AddTriangleFilled(tip, left, right, arrowColor);

					/// 投手目線に合わせた補助ガイドテキスト表示
					ImGui::SetCursorScreenPos(ImVec2(center.x + radius + 20.0f, center.y - 30.0f));
					ImGui::Text(u8"→奥 (キャッチャー方向)");
					ImGui::SetCursorScreenPos(ImVec2(center.x + radius + 20.0f, center.y - 10.0f));
					ImGui::Text(u8"↑上 (ホップ成分)");
				}

				//描画位置の下側にUを復帰させるためのダミー領域の確保
				ImGui::SetCursorScreenPos(ImVec2(center.x - 70.0f, center.y + radius + 30.0f));
				ImGui::Dummy(ImVec2(150.0f, 10.0f));

				ImGui::Text(u8"赤線：回転軸（黄点が回転のベクトルの向き＝右ねじの法則）");
				ImGui::Text(u8"水色の矢印：軸の周りを実際に回っているスピンの方向");
				ImGui::Text(u8"・X軸(横): バックスピン / Y軸(縦): サイドスピン / Z軸(前後): ジャイロ");

				ImGui::Spacing();
				if (ImGui::Button(u8"このパラメータでテスト投球開始", ImVec2(240, 30))) {
					SelectPitchType(); // 選択パラメータを現在の投球に適用
					currentState = State::Throwing;
					stateTime = 0.0f;
				}
				ImGui::Spacing();

				ImGui::Separator();
				ImGui::Checkbox("Is Ball Thrown", &isBallThrown);

				float speedMs = ballSpeedKmh / 3.6f;
				ImGui::Text("Speed: %.2f m/s (%.0f km/h)", speedMs, ballSpeedKmh);
				
			}
		}

		// 変更後
		bool prev = isRightPitcher;
		if (ImGui::Checkbox("Right Handed", &isRightPitcher))
		{
			if (prev != isRightPitcher)
			{
				// モデル・位置を再初期化
				ID3D11Device* device = Graphics::Instance().GetDevice();
				if (IsRightPitcher())
				{
					pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\rightPitcher.glb");
					position = { -0.1f,0.22f,18.15f };
					Ball::Instance().SetBallPosition({ 0.0f, 0.0f, 0.05f });
					Ball::Instance().SetBallAngle({ 0.0f, 0.0f, -1.6f });
				}
				else
				{
					pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\leftPitcher.glb");
					position = { 0.1f,0.22f,18.15f };
					Ball::Instance().SetBallPosition({ 0.0f, 0.0f, 0.05f });
					Ball::Instance().SetBallAngle({ 0.0f, 0.0f, 1.6f });
				}
				pitcher->build_static_batches(device);
				animated_nodes = pitcher->nodes;
				animation_time = 0.0f;
				
			}
		}
	Wind::Instance().DrawGUI();

#endif
}

//アタッチメント処理
void Pitcher::AttachBallToHand(float elapsedTime)
{
	if (!isBallThrown)
	{
		// 変更前
		// Ball::Instance().AttachToHand(animated_nodes, transform, "mixamorig:RightHandMiddle1");

		// 変更後
		const char* handName = IsRightPitcher()
			? "mixamorig:LeftHandMiddle1"
			: "mixamorig:RightHandMiddle1";
		Ball::Instance().AttachToHand(animated_nodes, transform, handName);

		ballStartPosition = Ball::Instance().GetStartPosition();
	}
	else
	{
		ApplyPhysicsToBall(elapsedTime);
		Ball::Instance().UpdateFromPhysics(elapsedTime);

		if (Ball::Instance().GetWorldPosition().y < 0.0f)
		{
			isBallThrown = false;
			Ball::Instance().SetHasCollided(false);
			animation_time = 0.0f;
			Ball::Instance().SetHasCollidedWithFence(false);
			Ball::Instance().SetHasCollidedWithGround(false);
			Ball::Instance().SetHasPassedFairFoulTrigger(false);
			Ball::Instance().SetHasPassedHomeRunZone(false);
			Ball::Instance().SetHasBeenJudged(false);
			Ball::Instance().SetFoulLogged(false);
			Ball::Instance().SetThroughStrikeZone(false); // ストライクゾーン通過フラグをリセット
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
			Ball::Instance().SetHasCollided(false);
			Ball::Instance().SetHasCollidedWithFence(false);
			Ball::Instance().SetHasPassedHomeRunZone(false);
			Ball::Instance().SetHasCollidedWithGround(false);
			Ball::Instance().SetHasPassedFairFoulTrigger(false);
			Ball::Instance().SetHasBeenJudged(false);
			Ball::Instance().SetFoulLogged(false);
			Ball::Instance().SetThroughStrikeZone(false); // ストライクゾーン通過フラグをリセット

			float speedMs = ballSpeedKmh / 3.6f;
			float launchAngleRadians = DirectX::XMConvertToRadians(launchAngleDegrees);

			throwDirection.y = sinf(launchAngleRadians);
			throwDirection.z = -cosf(launchAngleRadians);

			DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&throwDirection);
			dir = DirectX::XMVector3Normalize(dir);
			DirectX::XMFLOAT3 normalizedDir;
			DirectX::XMStoreFloat3(&normalizedDir, dir);

			physx::PxVec3 initialVelocity(normalizedDir.x * speedMs, normalizedDir.y * speedMs, normalizedDir.z * speedMs);
			const auto& param = pitchParameters[static_cast<int>(selectedPitchType)];
			Ball::Instance().Throw(initialVelocity, GetSpinAxisFromPitchType(), param.visualRotationSpeed, param.visualAngle);

			char debugMessage[128];
			snprintf(debugMessage, sizeof(debugMessage), u8"Throw Speed: %.2f km/h\n", initialVelocity.magnitude() * 3.6f);
			if (consoleLog)
			{
				consoleLog->push_back(debugMessage);
			}
			OutputDebugStringA(debugMessage);
		}

		pitcher->animate(current_animation_index, animation_time, animated_nodes);
	}
}

// ===== 新規追加: 球種から角速度を計算 =====
physx::PxVec3 Pitcher::GetSpinAxisFromPitchType() const
{
	const float RPM_TO_RAD_PER_SEC = 2.0f * 3.14159265f / 60.0f;
	const float side = IsRightPitcher() ? 1.0f : -1.0f; // 左投手はY軸反転

	int index = static_cast<int>(selectedPitchType);
	const auto& params = pitchParameters[index];

	//回転数から角速度の大きさを計算
	float angularSpeed = params.rpm * RPM_TO_RAD_PER_SEC;

	//回転軸の反転処理(左右で反転させる)
	physx::PxVec3 axis(params.spinAxis.x, params.spinAxis.y * side, params.spinAxis.z);

	//回転軸を正規化してから角速度ベクトルを計算
	axis.normalize();
	return axis * angularSpeed;
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

void Pitcher::SelectPitchTypeByAI()
{
	if (!usePitchAI)
	{
		SelectPitchType();
		return;
	}

	selectedPitchType = ChooseAIPitchType();
	SelectPitchType();

	const float speedVariance = GetSpeedVarianceKmh(selectedPitchType);
	ballSpeedKmh += GenerateRandomFloat(-speedVariance, speedVariance);
	ballSpeedKmh = (std::max)(60.0f, (std::min)(ballSpeedKmh, 180.0f));

	//ApplyAIGridTargetToPitch();

	const float side = IsRightPitcher() ? 1.0f : -1.0f;
	throwDirection.x = GenerateRandomFloat(0.01f, 0.03f) * side;

	//launchAngleDegrees = GenerateRandomFloat(-1.00f, 0.2f);

	if (consoleLog)
	{
		char msg[192];
		snprintf(msg, sizeof(msg), u8"[Info] AI Pitch: %s %.1f km/h angle %.2f dirX %.3f\n",
			GetPitchTypeName(selectedPitchType), ballSpeedKmh, launchAngleDegrees, throwDirection.x);
		consoleLog->push_back(msg);
	}
}

Pitcher::PitchType Pitcher::ChooseAIPitchType() const
{
	struct WeightedPitch
	{
		PitchType type;
		float weight;
	};

	const WeightedPitch weights[] =
	{
		{ PitchType::Fastball, 34.0f },
		{ PitchType::TwoSeam, 12.0f },
		{ PitchType::Cutter, 10.0f },
		{ PitchType::Slider, 10.0f },
		{ PitchType::Changeup, 8.0f },
		{ PitchType::Sinker, 7.0f },
		{ PitchType::Curveball, 5.0f },
		{ PitchType::Forkball, 4.0f },
		{ PitchType::VerticalSlider, 3.0f },
		{ PitchType::Splitter, 3.0f },
		{ PitchType::Shooter, 2.0f },
		{ PitchType::SlowCurve, 1.0f },
		{ PitchType::Knuckleball, 0.7f },
		{ PitchType::SlowBall, 99.0f },
	};

	float totalWeight = 0.0f;
	for (const WeightedPitch& pitch : weights)
	{
		totalWeight += pitch.weight;
	}

	float roll = GenerateRandomFloat(0.0f, totalWeight);
	for (const WeightedPitch& pitch : weights)
	{
		roll -= pitch.weight;
		if (roll <= 0.0f)
		{
			return pitch.type;
		}
	}

	return PitchType::Fastball;
}

void Pitcher::ApplyAIGridTargetToPitch()
{
	
	/*const int column = aiTargetZoneIndex % 3;
	const int row = aiTargetZoneIndex / 3;
	const float cellWidth = boxSize.x / 3.0f;
	const float cellHeight = boxSize.y / 3.0f;
	const float left = boxPosition.x - boxSize.x * 0.5f;
	const float top = boxPosition.y + boxSize.y * 0.5f;

	float targetX = left + cellWidth * (static_cast<float>(column) + 0.5f);
	float targetY = top - cellHeight * (static_cast<float>(row) + 0.5f);

	targetX += GenerateRandomFloat(-cellWidth * 0.35f, cellWidth * 0.35f);
	targetY += GenerateRandomFloat(-cellHeight * 0.35f, cellHeight * 0.35f);

	if (GenerateRandomFloat(0.0f, 1.0f) > aiStrikeRate)
	{
		const float miss = GenerateRandomFloat(0.02f, (std::max)(0.02f, aiNearBallMargin));
		if (column == 0)
		{
			targetX = left - miss;
		}
		else if (column == 2)
		{
			targetX = left + boxSize.x + miss;
		}
		else if (row == 0)
		{
			targetY = top + miss;
		}
		else if (row == 2)
		{
			targetY = top - boxSize.y - miss;
		}
		else if (GenerateRandomFloat(0.0f, 1.0f) < 0.5f)
		{
			targetX = (GenerateRandomFloat(0.0f, 1.0f) < 0.5f) ? left - miss : left + boxSize.x + miss;
		}
		else
		{
			targetY = (GenerateRandomFloat(0.0f, 1.0f) < 0.5f) ? top + miss : top - boxSize.y - miss;
		}
	}

	const float targetZ = boxPosition.z;
	float distanceZ = ballStartPosition.z - targetZ;
	if (distanceZ < 1.0f)
	{
		distanceZ = 18.0f;
	}

	throwDirection.x = (targetX - ballStartPosition.x) / distanceZ;
	throwDirection.x += GenerateRandomFloat(-0.004f, 0.004f);

	launchAngleDegrees = DirectX::XMConvertToDegrees(atan2f(targetY - ballStartPosition.y, distanceZ));
	launchAngleDegrees += GenerateRandomFloat(-0.20f, 2.0f);*/
}

float Pitcher::GetSpeedVarianceKmh(PitchType pitchType) const
{
	switch (pitchType)
	{
	case PitchType::Fastball: return 4.0f;
	case PitchType::TwoSeam: return 3.5f;
	case PitchType::Cutter: return 3.0f;
	case PitchType::Slider: return 4.0f;
	case PitchType::Curveball: return 5.0f;
	case PitchType::Changeup: return 5.0f;
	case PitchType::Forkball: return 4.0f;
	case PitchType::Sinker: return 3.5f;
	case PitchType::VerticalSlider: return 4.0f;
	case PitchType::Splitter: return 4.0f;
	case PitchType::SlowCurve: return 6.0f;
	case PitchType::Shooter: return 3.5f;
	case PitchType::Knuckleball: return 7.0f;
	case PitchType::SlowBall: return 8.0f;
	default: return 3.0f;
	}
}

const char* Pitcher::GetPitchTypeName(PitchType pitchType) const
{
	switch (pitchType)
	{
	case PitchType::Fastball: return u8"ストレート";
	case PitchType::Slider: return u8"スライダー";
	case PitchType::Curveball: return u8"カーブ";
	case PitchType::Changeup: return u8"チェンジアップ";
	case PitchType::Forkball: return u8"フォーク";
	case PitchType::TwoSeam: return u8"ツーシーム";
	case PitchType::Cutter: return u8"カットボール";
	case PitchType::Sinker: return IsRightPitcher() ? u8"シンカー" : u8"スクリュー";
	case PitchType::VerticalSlider: return u8"縦スライダー";
	case PitchType::Splitter: return u8"スプリット";
	case PitchType::SlowCurve: return u8"スローカーブ";
	case PitchType::Shooter: return u8"シュート";
	case PitchType::Knuckleball: return u8"ナックルボール";
	case PitchType::SlowBall: return u8"スローボール";
	default: return u8"不明";
	}
}

void Pitcher::SelectPitchType() 
{
	

	int index =  static_cast<int>(selectedPitchType);

	// 安全ガード：万が一範囲外を指していたら 0番目（Fastball）にする
	if (index < 0 || index >= static_cast<int>(pitchParameters.size())) {
		index = 0;
		selectedPitchType = static_cast<PitchType>(PitchType::Fastball);
	}

	// vector から現在の変数へパラメータを適用
	const auto& param = pitchParameters[index];
	ballSpeedKmh = param.ballSpeedKmh;
	launchAngleDegrees = param.launchAngleDegrees;

	const float side = IsRightPitcher() ? 1.0f : -1.0f;
	throwDirection = param.throwDirection;
	throwDirection.x *= side; // 横方向（X軸）のベクトルを反転ection;

	// rotationSpeedの計算 (物理エンジン側で使う場合)
	// RPM（1分間の回転数）を度/秒に変換して rotationSpeed ベクトルを作る例
	// spinAxis(方向) * rpm * 変換係数
	float rpmToDegPerSec = (param.rpm * 360.0f) / 60.0f;
	rotationSpeed.x = param.spinAxis.x * rpmToDegPerSec;
	rotationSpeed.y = param.spinAxis.y * rpmToDegPerSec;
	rotationSpeed.z = param.spinAxis.z * rpmToDegPerSec;

	// 見た目専用の回転速度と見た目角度を Ball に適用
	// visualRotationSpeed は deg/s をそのまま渡す（Ball 側で deg/s を deg->rad に変換して使っている実装なら合わせてください）
	Ball::Instance().SetModelRotationSpeed(param.visualRotationSpeed);

	// visualAngle は Pitcher 側で度 -> ラジアン変換して Ball に渡す（Ball::SetModelAngle はラジアン想定）
	DirectX::XMFLOAT3 visualAngleRad = {
		DirectX::XMConvertToRadians(param.visualAngle.x),
		DirectX::XMConvertToRadians(param.visualAngle.y),
		DirectX::XMConvertToRadians(param.visualAngle.z)
	};
	Ball::Instance().SetModelAngle(visualAngleRad);

	if (consoleLog) {
		char msg[64];
		snprintf(msg, sizeof(msg), "Selected Pitch Index: %d\n", index);
		consoleLog->push_back(msg);
	}
}


void Pitcher::SaveToJson(json& j)
{
	j["position"] = { position.x, position.y, position.z };
	j["scale"] = { scale.x, scale.y, scale.z };
	j["angle"] = { angle.x, angle.y, angle.z };
	j["box_position"] = { boxPosition.x, boxPosition.y, boxPosition.z };
	j["box_size"] = { boxSize.x, boxSize.y, boxSize.z };
	j["ball_speed_kmh"] = ballSpeedKmh;
	j["launch_angle_deg"] = launchAngleDegrees;
	j["throw_timing"] = throwTiming;
	j["throw_direction"] = { throwDirection.x, throwDirection.y, throwDirection.z };
	j["rotation_speed"] = { rotationSpeed.x, rotationSpeed.y, rotationSpeed.z };
	j["is_right_pitcher"] = isRightPitcher;
	j["use_pitch_ai"] = usePitchAI;
	j["ai_strike_rate"] = aiStrikeRate;
	j["ai_near_ball_margin"] = aiNearBallMargin;

	// 球種設定を配列として保存
	json pitchArray = json::array();
	for (int i = 0; i < PITCH_TYPE_COUNT; ++i) {
		json p;
		p["speed"] = pitchParameters[i].ballSpeedKmh;
		p["angle"] = pitchParameters[i].launchAngleDegrees;
		p["dir"] = { pitchParameters[i].throwDirection.x, pitchParameters[i].throwDirection.y, pitchParameters[i].throwDirection.z };
		p["axis"] = { pitchParameters[i].spinAxis.x, pitchParameters[i].spinAxis.y, pitchParameters[i].spinAxis.z };
		p["rpm"] = pitchParameters[i].rpm;
		pitchArray.push_back(p);
	}
	j["pitch_settings"] = pitchArray;
}

void Pitcher::LoadFromJson(const json& j)
{
	if (j.contains("position")) position = { j["position"][0], j["position"][1], j["position"][2] };
	if (j.contains("scale")) scale = { j["scale"][0], j["scale"][1], j["scale"][2] };
	if (j.contains("angle")) angle = { j["angle"][0], j["angle"][1], j["angle"][2] };
	if (j.contains("box_position")) boxPosition = { j["box_position"][0], j["box_position"][1], j["box_position"][2] };
	if (j.contains("box_size")) boxSize = { j["box_size"][0], j["box_size"][1], j["box_size"][2] };
	if (j.contains("ball_speed_kmh")) ballSpeedKmh = j["ball_speed_kmh"];
	if (j.contains("launch_angle_deg")) launchAngleDegrees = j["launch_angle_deg"];
	if (j.contains("throw_timing")) throwTiming = j["throw_timing"];
	if (j.contains("throw_direction")) throwDirection = { j["throw_direction"][0], j["throw_direction"][1], j["throw_direction"][2] };
	if (j.contains("rotation_speed")) rotationSpeed = { j["rotation_speed"][0], j["rotation_speed"][1], j["rotation_speed"][2] };
	if (j.contains("use_pitch_ai")) usePitchAI = j["use_pitch_ai"];
	if (j.contains("ai_strike_rate")) aiStrikeRate = (std::max)(0.0f, (std::min)(1.0f, static_cast<float>(j["ai_strike_rate"])));
	if (j.contains("ai_near_ball_margin")) aiNearBallMargin = (std::max)(0.0f, static_cast<float>(j["ai_near_ball_margin"]));

	// 投手の左右設定を反映
	if(j.contains("is_right_pitcher") && (bool)j["is_right_pitcher"] != isRightPitcher)
	{
		isRightPitcher = j["is_right_pitcher"];
		ID3D11Device* device = Graphics::Instance().GetDevice();
		if (isRightPitcher)
		{
			pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\rightPitcher.glb");
			position = { -0.1f, 0.22f, 18.15f };
		}
		else
		{
			pitcher = std::make_unique<gltf_model>(device, ".\\resources\\pitcher\\leftPitcher.glb");
			position = { 0.1f, 0.22f, 18.15f };
		}
		pitcher->build_static_batches(device);
		animated_nodes = pitcher->nodes;
		animation_time = 0.0f;
	}

	// ストライクゾーンのPhysX更新
	if (strikeZoneTrigger)
	{
		strikeZoneTrigger->setGlobalPose(physx::PxTransform(physx::PxVec3(boxPosition.x, boxPosition.y, boxPosition.z)));
		physx::PxShape* shape = nullptr;
		strikeZoneTrigger->getShapes(&shape, 1);
		if (shape) shape->setGeometry(physx::PxBoxGeometry(boxSize.x / 2.0f, boxSize.y / 2.0f, boxSize.z / 2.0f));
	}

	// 球種設定の読み込み
	if (j.contains("pitch_settings") && j["pitch_settings"].is_array()) {
		const auto& pitchArray = j["pitch_settings"];
		for (size_t i = 0; i < pitchArray.size() && i < PITCH_TYPE_COUNT; ++i) {
			const auto& p = pitchArray[i];
			if (p.contains("speed")) pitchParameters[i].ballSpeedKmh = p["speed"];
			if (p.contains("angle")) pitchParameters[i].launchAngleDegrees = p["angle"];
			if (p.contains("rpm")) pitchParameters[i].rpm = p["rpm"];
			if (p.contains("dir")) pitchParameters[i].throwDirection = { p["dir"][0], p["dir"][1], p["dir"][2] };
			if (p.contains("axis")) pitchParameters[i].spinAxis = { p["axis"][0], p["axis"][1], p["axis"][2] };
		}
	}

	// 現在選択中のパラメータを再適用
	SelectPitchType();
}
