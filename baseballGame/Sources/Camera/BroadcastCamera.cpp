#include "BroadcastCamera.h"
#include "Ball.h"
#include "physxManager.h"
#include "TrackingData.h"
#include <imgui.h>

//カメラ管理
void BroadcastCamera::ApplyPresetToController(const CameraPreset& preset, CameraController& controller)
{
	controller.SetEyeAndFocus(preset.eye, preset.focus);
	controller.SetFov(preset.fov);
	controller.SetTrackingZoomOut(preset.enableTrackingZoom, preset.fovNear, preset.fovFar, preset.zoomNearDist, preset.zoomFarDist);
}

//カメラを1台追加する(カメラプリセットを追加してコントローラーに適用する)
int BroadcastCamera::AddCameraPreset(const CameraPreset& preset)
{
	cameraPresets.push_back(preset);
	cameraControllers.emplace_back();
	ApplyPresetToController(cameraPresets.back(), cameraControllers.back());
	return static_cast<int>(cameraPresets.size() - 1);// 追加したカメラのインデックスを返す
}

//カメラを1台追加する(名前と位置情報を指定して追加する)
int BroadcastCamera::AddCameraPreset(const std::string& name, const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus)
{
	CameraPreset preset;
	preset.name = name;
	preset.eye = eye;
	preset.focus = focus;
	return AddCameraPreset(preset);// 追加したカメラのインデックスを返す
}

//カメラプリセットを削除する
void BroadcastCamera::RemoveCameraPreset(int index)
{
	if (index < 0 || index >= static_cast<int>(cameraPresets.size()))
		return; // インデックスが範囲外の場合は何もしない

	//最低１台は残す
	if (cameraPresets.size() <= 1)
		return;

	cameraPresets.erase(cameraPresets.begin() + index);
	cameraControllers.erase(cameraControllers.begin() + index);
	// アクティブなカメラインデックスを調整
	if (activeCameraIndex >= static_cast<int>(cameraPresets.size()))
		activeCameraIndex = static_cast<int>(cameraPresets.size() - 1);
}

//デフォルトのカメラを設定する
void BroadcastCamera::SetupDefaultCameras()
{
	cameraPresets.clear();// 既存のカメラプリセットをクリア
	cameraControllers.clear();// 既存のカメラコントローラーをクリア


	{
		CameraPreset preset;
		preset.name = u8"デフォルトカメラ";
		preset.eye = { 0.0f, 1.0f, -3.5f };
		preset.focus = { 0.0f, 0.0f, 14.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::NormalCamera;
		preset.cameraId = 0; // デフォルトカメラのIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"バックネットカメラ";
		preset.eye = { 0.0f, 5.0f, -20.0f };
		preset.focus = { 0.0f, 0.0f, 14.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = true;
		preset.fovNear = DirectX::XMConvertToRadians(45.0f);// ボールが近いときのFOV
		preset.fovFar = DirectX::XMConvertToRadians(10.0f);// ボールが遠いときのFOV
		preset.zoomNearDist = 10.0f;// この距離以下でfovNear
		preset.zoomFarDist = 150.0f;// この距離以上でfovFar
		preset.type = CameraType::HitCamera;
		preset.cameraId = 1; // バックネットカメラのIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"1塁側カメラ";
		preset.eye = { 20.0f, 5.0f, -15.0f };
		preset.focus = { 0.0f, 1.0f, 5.0f };
		preset.fov = DirectX::XMConvertToRadians(30.0f);
		preset.enableTrackingZoom = true;
		preset.fovNear = DirectX::XMConvertToRadians(30.0f);// ボールが近いときのFOV
		preset.fovFar = DirectX::XMConvertToRadians(10.0f);// ボールが遠いときのFOV
		preset.zoomNearDist = 10.0f;// この距離以下でfovNear
		preset.zoomFarDist = 100.0f;// この距離以上でfovFar
		preset.type = CameraType::HitCamera;
		preset.cameraId = 2; // 1塁側カメラのIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"外野カメラ";
		preset.eye = { -10.0f, 9.0f,127.5f };
		preset.focus = { 0.0f, 0.5f, 5.0f };
		preset.fov = DirectX::XMConvertToRadians(2.0f);
		preset.enableTrackingZoom = true;
		preset.fovNear = DirectX::XMConvertToRadians(30.0f);// ボールが近いときのFOV
		preset.fovFar = DirectX::XMConvertToRadians(10.0f);// ボールが遠いときのFOV
		preset.zoomNearDist = 10.0f;// この距離以下でfovNear
		preset.zoomFarDist = 130.0f;// この距離以上でfovFar
		preset.type = CameraType::HitCamera;
		preset.cameraId = 3; // 外野カメラのIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"屋根カメラ";
		preset.eye = { 0.0f, 60.0f, -65.0f };
		preset.focus = { 0.0f, 0.0f, 30.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::EventCamera;
		preset.cameraId = 4; // 屋根カメラのIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"3塁側カメラ";
		preset.eye = { -20.0f, 5.0f, -15.0f };
		preset.focus = { 0.0f, 1.0f, 5.0f };
		preset.fov = DirectX::XMConvertToRadians(30.0f);
		preset.enableTrackingZoom = true;
		preset.fovNear = DirectX::XMConvertToRadians(30.0f);// ボールが近いときのFOV
		preset.fovFar = DirectX::XMConvertToRadians(10.0f);// ボールが遠いときのFOV
		preset.zoomNearDist = 10.0f;// この距離以下でfovNear
		preset.zoomFarDist = 100.0f;// この距離以上でfovFar
		preset.type = CameraType::HitCamera;
		preset.cameraId = 5; // 3塁側カメラのIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"確信ホームランカメラ1";
		preset.eye = { 0.0f, 0.7f, 2.0f };
		preset.focus = { 0.0f, 1.8f, -1.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::HomeRunCamera;
		preset.cameraId = 6; // 確信ホームランカメラ1のIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"確信ホームランカメラ2";
		preset.eye = { -4.0f, 1.0f, 2.0f };
		preset.focus = { 0.0f, 1.0f, 0.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::HomeRunCamera;
		preset.cameraId = 7; // 確信ホームランカメラ2のIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"確信ホームランカメラ3";
		preset.eye = { 4.0f, 1.0f, 2.0f };
		preset.focus = { 0.0f, 1.0f, 0.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::HomeRunCamera;
		preset.cameraId = 8; // 確信ホームランカメラ3のIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"確信ホームランカメラ4";
		preset.eye = { 0.0f, 0.4f, -3.5f };
		preset.focus = { 0.0f, 1.0f, -1.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::HomeRunCamera;
		preset.cameraId = 9; // 確信ホームランカメラ4のIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"確信ホームランカメラ5";
		preset.eye = { -2.0f, 1.0f, -3.0f };
		preset.focus = { 0.0f, 1.0f, 0.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::HomeRunCamera;
		preset.cameraId = 10; // 確信ホームランカメラ5のIDを設定
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"確信ホームランカメラ6";
		preset.eye = { 3.0f, 1.0f, -30.0f };
		preset.focus = { 0.0f, 1.0f, 0.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		preset.type = CameraType::HomeRunCamera;
		preset.cameraId = 11; // 確信ホームランカメラ6のIDを設定
		AddCameraPreset(preset);
	}
	activeCameraIndex = 0;// 最初のカメラをアクティブにする
}

void BroadcastCamera::Update(float elapsed_time, bool ballHasCollidedWithBat)
{
	
	if (cameraPresets.empty())
	{
		SetupDefaultCameras();
	}
	activeCameraIndex = (std::max)(0, (std::min)(activeCameraIndex, static_cast<int>(cameraPresets.size() - 1)));


	bool nowShowTrackingData = TrackingData::Instance().IsTrackingDataVisible();
	bool nowIsHomeRun = Physics::Instance().GetIsHomeRun();

	//打球方向によってアクティブにするカメラを変える
	float ballDirection = Physics::Instance().GetBallDirection();
	float originalDirection = Physics::Instance().GetBallOriginalDirection();

	//バットに当たった瞬間にHitCameraの追跡を開始する
	if(ballHasCollidedWithBat && !prevHasCollidedWithBat)
	{
		Physics::Instance().ClearBallWasHit();
		for (int i = 0; i < static_cast<int>(cameraPresets.size()); ++i)
		{
			if (cameraPresets[i].type == CameraType::HitCamera || cameraPresets[i].type == CameraType::ReplayCamera)
			{
				cameraControllers[i].StartTrackingBall(&Ball::Instance(), 3.0f, -30.0f);
			}
		}
	}

	//確信ホームランの立ち上がりでHomeRunCameraに切り替える
	if(nowIsHomeRun && !prevIsHomeRun)
	{
		
		CameraPreset& preset = cameraPresets[activeCameraIndex];
		if(preset.type != CameraType::HomeRunCamera)
		{
			std::vector<int> homeRunCameraIndices;
			for(int i = 0; i < static_cast<int>(cameraPresets.size()); ++i)
			{
				if(cameraPresets[i].type == CameraType::HomeRunCamera)
				{
					homeRunCameraIndices.push_back(i);// HomeRunCameraのインデックスを保存
				}
			}
			if(!homeRunCameraIndices.empty())
			{
				//打球角度によってカメラを切り替える
				if(originalDirection >=-45.0f && originalDirection <=-15.0f)
				{
					//確信ホームランカメラ1か3か6のどれかからランダム
					int randomIndex = rand() % 3;

					if(randomIndex == 0)
					{
						activeCameraIndex = 6;//確信ホームランカメラ1
					}
					else if(randomIndex == 1)
					{
						activeCameraIndex = 8;//確信ホームランカメラ3
					}
					else
					{
						activeCameraIndex = 11;//確信ホームランカメラ6
					}

				}
				else if(originalDirection >=15.0f && originalDirection <=45.0f)
				{
					//確信ホームランカメラ1か2か5のどれかからランダム
					int randomIndex = rand() % 3;

					if(randomIndex == 0)
					{
						activeCameraIndex = 6;//確信ホームランカメラ1
					}
					else if(randomIndex == 1)
					{
						activeCameraIndex = 7;//確信ホームランカメラ2
					}
					else
					{
						activeCameraIndex = 10;//確信ホームランカメラ5
					}
				}
				else
				{
					//確信ホームランカメラ1か4のどれかからランダム
					int randomIndex = rand() % 2;

					if(randomIndex == 0)
					{
						activeCameraIndex = 6;//確信ホームランカメラ1
					}
					else
					{
						activeCameraIndex = 9;//確信ホームランカメラ4
					}
				}

			}
		}
	}

	//確信ホームランの状態でトラッキングデータが表示された瞬間にhitCameraに切り替える
	if(nowIsHomeRun && nowShowTrackingData && !prevHasShowTrackingData)
	{
		CameraPreset& preset = cameraPresets[activeCameraIndex];
		if(preset.type != CameraType::HitCamera)
		{
			std::vector<int> hitCameraIndices;
			for(int i = 0; i < static_cast<int>(cameraPresets.size()); ++i)
			{
				if(cameraPresets[i].type == CameraType::HitCamera)
				{
					hitCameraIndices.push_back(i);// HitCameraのインデックスを保存
				}
			}
			if(!hitCameraIndices.empty())
			{
				//レフト方向のカメラを選択
				if (originalDirection >= -45.0f && originalDirection <= -15.0f)
				{
					//1塁側かバックネットのどちらかにする
					int randomIndex = rand() % 2;

					if (randomIndex == 0)
					{
						activeCameraIndex = 1;//バックネットカメラ
					}
					else
					{
						activeCameraIndex = 2;//1塁側カメラ
					}
				}
				else if (originalDirection >= 15.0f && originalDirection <= 45.0f)
				{
					//3塁側かバックネットのどちらかにする
					int randomIndex = rand() % 2;

					if (randomIndex == 0)
					{
						activeCameraIndex = 1;//バックネットカメラ
					}
					else
					{
						activeCameraIndex = 5;//3塁側カメラ
					}
				}
				else
				{
					//2つのヒットカメラからランダムに選択
					int randomIndex = rand() % 2;

					if (randomIndex == 0)
					{
						activeCameraIndex = 1;//バックネットカメラ
					}
					else
					{
						activeCameraIndex = 2;//1塁側カメラ
					}				
				}
			}
		}
	}

	//トラッキングデータが表示された瞬間にカメラをhitCameraに切り替える
	if (!nowIsHomeRun)
	{
		if (nowShowTrackingData && !prevHasShowTrackingData)
		{
			
			CameraPreset& preset = cameraPresets[activeCameraIndex];
			if (preset.type != CameraType::HitCamera)
			{
				std::vector<int> hitCameraIndices;
				for (int i = 0; i < static_cast<int>(cameraPresets.size()); ++i)
				{
					if (cameraPresets[i].type == CameraType::HitCamera)
					{
						hitCameraIndices.push_back(i);// HitCameraのインデックスを保存
					}
				}
				if (!hitCameraIndices.empty())
				{
					
					//打球方向によってカメラを切り替える
					
					//レフト方向のカメラを選択
					if(originalDirection >=-45.0f && originalDirection <=-15.0f)
					{
						//1塁側かバックネットのどちらかにする
						int randomIndex = rand() % 2;

						if(randomIndex == 0)
						{
							activeCameraIndex = 1;//バックネットカメラ
						}
						else
						{
							activeCameraIndex = 2;//1塁側カメラ
						}
					}
					else if(originalDirection >=15.0f && originalDirection <=45.0f)
					{
						//3塁側かバックネットのどちらかにする
						int randomIndex = rand() % 2;

						if(randomIndex == 0)
						{
							activeCameraIndex = 1;//バックネットカメラ
						}
						else
						{
							activeCameraIndex = 5;//3塁側カメラ
						}
					}
					else
					{
						//2つのヒットカメラからランダムに選択
						int randomIndex = rand() % 2;

						if(randomIndex == 0)
						{
							activeCameraIndex = 1;//バックネットカメラ
						}
						else
						{
							activeCameraIndex = 2;//1塁側カメラ
						}
						
					}
				}
			}
		}
	}

	//バットの接触状態が解除されたら、NormalCameraに戻す
	if(!ballHasCollidedWithBat && prevHasCollidedWithBat)
	{
		for (int i = 0; i < static_cast<int>(cameraPresets.size()); ++i)
		{
			if (cameraPresets[i].type == CameraType::NormalCamera)
			{
				activeCameraIndex = i;
				break;
			}
		}
		StopAllTracking();
	}

	prevHasCollidedWithBat = ballHasCollidedWithBat;// 前フレームの状態を更新
	prevHasShowTrackingData = nowShowTrackingData;// 前フレームの状態を更新
	prevIsHomeRun = nowIsHomeRun;// 前フレームの状態を更新

	//すべてのカメラを更新する
	for (auto& controller : cameraControllers)
	{
		controller.Update(elapsed_time);
	}
}

void BroadcastCamera::SyncToCamera(Camera& camera, float aspect, float nearZ, float farZ)
{
	cameraControllers[activeCameraIndex].SyncControllerToCamera(camera);
	camera.SetPerspectiveFov(cameraControllers[activeCameraIndex].GetCurrentFov(), aspect, nearZ, farZ);
}

bool BroadcastCamera::IsTrackingBall() const
{
	return cameraControllers[activeCameraIndex].IsTrackingBall();
}

void BroadcastCamera::StopAllTracking()
{
	for (auto& controller : cameraControllers)
	{
		controller.StopTrackingBall();
	}
	activeCameraIndex = 0; // デフォルトカメラに戻す
}

std::string BroadcastCamera::GetPresetNameById(int cameraId) const
{
	for (const auto& preset : cameraPresets)
	{
		if (preset.cameraId == cameraId)
		{
			return preset.name;
		}
	}
	return ""; // 該当するカメラIDが見つからなかった場合は空文字を返す
}

void BroadcastCamera::DrawGUI()
{
	//中継カメラの設定
	if (ImGui::CollapsingHeader(u8"中継カメラ"), ImGuiTreeNodeFlags_DefaultOpen)
	{
		Camera& camera = Camera::Instance();
		ImGui::TextDisabled(u8"数字キーでも切り替えできる");

		int deleteIndex = -1;

		for (int index = 0; index < static_cast<int>(cameraPresets.size()); ++index)
		{
			ImGui::PushID(index);
			CameraPreset& preset = cameraPresets[index];
			bool isActive = (index == activeCameraIndex);
			// カメラプリセットの選択
			//カメラ名前があればそれを表示、なければ "カメラX" と表示
			std::string label = preset.name.empty() ? (u8"カメラ" + std::to_string(index)) : preset.name;
			if (isActive) label += u8" (アクティブ)";

			if (ImGui::TreeNode(label.c_str()))
			{
				char nameBuffer[64];
				strncpy_s(nameBuffer, preset.name.c_str(), sizeof(nameBuffer) - 1);

				if (ImGui::InputText(u8"名前", nameBuffer, sizeof(nameBuffer)))
				{
					preset.name = nameBuffer;
				}

				bool changed = false;// カメラ位置・注視点の編集
				changed |= ImGui::DragFloat3(u8"eye", &preset.eye.x, 0.1f);
				changed |= ImGui::DragFloat3(u8"focus", &preset.focus.x, 0.1f);

				float fovDeg = DirectX::XMConvertToDegrees(preset.fov);// カメラのFOV編集
				if (ImGui::SliderFloat(u8"FOV", &fovDeg, 10.0f, 120.0f))
				{
					preset.fov = DirectX::XMConvertToRadians(fovDeg);// ラジアンに変換して保存
					changed = true;// FOVの変更はフリーカメラに反映させる
				}

				ImGui::Checkbox(u8"ズームアウト追跡を有効化", &preset.enableTrackingZoom);
				if (preset.enableTrackingZoom)
				{
					float fovNearDeg = DirectX::XMConvertToDegrees(preset.fovNear);// ズームアウト追跡の近距離FOV編集

					float fovFarDeg = DirectX::XMConvertToDegrees(preset.fovFar);// ズームアウト追跡の遠距離FOV編集
					if (ImGui::SliderFloat(u8"追跡近距離FOV", &fovNearDeg, 10.0f, 120.0f))
					{
						preset.fovNear = DirectX::XMConvertToRadians(fovNearDeg);
						changed = true;// 近距離FOVの変更はフリーカメラに反映させる
					}
					if (ImGui::SliderFloat(u8"追跡遠距離FOV", &fovFarDeg, 10.0f, 120.0f))
					{
						preset.fovFar = DirectX::XMConvertToRadians(fovFarDeg);
						changed = true;// 遠距離FOVの変更はフリーカメラに反映させる
					}

					changed |= ImGui::DragFloat(u8"近距離閾値", &preset.zoomNearDist, 0.5f, 0.0f, 500.0f);// ズームアウト追跡の距離範囲編集
					changed |= ImGui::DragFloat(u8"遠距離閾値", &preset.zoomFarDist, 0.5f, 0.0f, 500.0f);// ズームアウト追跡のFOV範囲編集
				}

				//カメラタイプを選択できるようにする
				const char* cameraTypeItems[] = { u8"通常カメラ", u8"ヒットカメラ", u8"ホームランカメラ", u8"リプレイカメラ", u8"イベントカメラ" };

				int currentTypeIndex = static_cast<int>(preset.type);

				if (ImGui::Combo(u8"カメラタイプ", &currentTypeIndex, cameraTypeItems, IM_ARRAYSIZE(cameraTypeItems)))
				{
					preset.type = static_cast<CameraType>(currentTypeIndex);
				}

				// カメラIDを編集できるようにする
				changed |= ImGui::InputInt(u8"カメラID", &preset.cameraId);

				if (changed)
				{
					// 編集したプリセットがアクティブでフリーカメラ使用中なら、即座に反映させる
					ApplyPresetToController(preset, cameraControllers[index]);
				}
				if (ImGui::Button(u8"選択"))
				{
					activeCameraIndex = index;
				}
				ImGui::SameLine();
				if (ImGui::Button(u8"ここに配置"))
				{
					preset.eye = camera.GetEye();
					preset.focus = camera.GetFocus();
					ApplyPresetToController(preset, cameraControllers[index]);
				}
				ImGui::SameLine();
				if (ImGui::Button(u8"削除"))
				{
					deleteIndex = index;
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		if (deleteIndex != -1)
		{
			RemoveCameraPreset(deleteIndex);
		}
		ImGui::Separator();
		if (ImGui::Button(u8"+ 新しい中継カメラを追加"))
		{
			// フリーカメラ中なら今いる位置に、そうでなければ現在のアクティブカメラの位置を初期値にして追加する。
			// 追加後はそのまま座標を手入力で微調整することもできる。
			std::string newName = u8"中継カメラ" + std::to_string(cameraPresets.size() + 1);
			activeCameraIndex = AddCameraPreset(newName, camera.GetEye(), camera.GetFocus());
		}
	}
}

void BroadcastCamera::SaveToJson(json& j) const
{
	for (size_t i = 0; i < cameraPresets.size(); ++i)
	{
		const CameraPreset& p = cameraPresets[i];
		j["relay_cameras"][i]["name"] = p.name;
		j["relay_cameras"][i]["eye"] = { p.eye.x, p.eye.y, p.eye.z };
		j["relay_cameras"][i]["focus"] = { p.focus.x, p.focus.y, p.focus.z };
		j["relay_cameras"][i]["fov"] = p.fov;
		j["relay_cameras"][i]["enable_tracking_zoom"] = p.enableTrackingZoom;
		j["relay_cameras"][i]["fov_near"] = p.fovNear;
		j["relay_cameras"][i]["fov_far"] = p.fovFar;
		j["relay_cameras"][i]["zoom_near_dist"] = p.zoomNearDist;
		j["relay_cameras"][i]["zoom_far_dist"] = p.zoomFarDist;
		j["relay_cameras"][i]["type"] = static_cast<int>(p.type);
		j["relay_cameras"][i]["cameraId"] = p.cameraId;
	}
	j["relay_cameras_active_index"] = activeCameraIndex;
}

void BroadcastCamera::LoadFromJson(const nlohmann::json& j)
{
	if (!j.contains("relay_cameras") || j["relay_cameras"].empty())
		return; // 呼び出し元で SetupDefaultCameras() 済みのデフォルトのまま

	cameraPresets.clear();
	cameraControllers.clear();

	for (auto& jc : j["relay_cameras"])
	{
		CameraPreset preset;
		preset.name = jc.value("name", std::string(u8"カメラ"));
		preset.eye = { jc["eye"][0], jc["eye"][1], jc["eye"][2] };
		preset.focus = { jc["focus"][0], jc["focus"][1], jc["focus"][2] };
		preset.fov = jc.value("fov", DirectX::XMConvertToRadians(45.0f));
		preset.enableTrackingZoom = jc.value("enable_tracking_zoom", false);
		preset.fovNear = jc.value("fov_near", DirectX::XMConvertToRadians(5.0f));
		preset.fovFar = jc.value("fov_far", DirectX::XMConvertToRadians(15.0f));
		preset.zoomNearDist = jc.value("zoom_near_dist", 10.0f);
		preset.zoomFarDist = jc.value("zoom_far_dist", 130.0f);
		preset.type = static_cast<CameraType>(jc.value("type", 0));
		preset.cameraId = jc.value("cameraId", -1);
		AddCameraPreset(preset);
	}
	activeCameraIndex = j.value("relay_cameras_active_index", 0);
	activeCameraIndex = (std::max)(0, (std::min)(activeCameraIndex, static_cast<int>(cameraPresets.size()) - 1));
}