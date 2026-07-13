#include "BroadcastCamera.h"
#include "Ball.h"
#include "physxManager.h"
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
		preset.eye = { 0.0f, 1.2f, -3.5f };
		preset.focus = { 0.0f, 0.0f, 14.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"俯瞰カメラ";
		preset.eye = { 0.0f, 5.0f, -15.0f };
		preset.focus = { 0.0f, 0.0f, 14.0f };
		preset.fov = DirectX::XMConvertToRadians(45.0f);
		preset.enableTrackingZoom = false;
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"1塁側カメラ";
		preset.eye = { 27.0f, 11.0f, -12.5f };
		preset.focus = { 0.0f, 1.0f, 15.0f };
		preset.fov = DirectX::XMConvertToRadians(30.0f);
		preset.enableTrackingZoom = true;
		preset.fovNear = DirectX::XMConvertToRadians(30.0f);// ボールが近いときのFOV
		preset.fovFar = DirectX::XMConvertToRadians(10.0f);// ボールが遠いときのFOV
		preset.zoomNearDist = 10.0f;// この距離以下でfovNear
		preset.zoomFarDist = 100.0f;// この距離以上でfovFar
		AddCameraPreset(preset);
	}

	{
		CameraPreset preset;
		preset.name = u8"外野カメラ";
		preset.eye = { -5.0f, 10.0f,120.0f };
		preset.focus = { 0.0f, 0.5f, 5.0f };
		preset.fov = DirectX::XMConvertToRadians(2.0f);
		preset.enableTrackingZoom = true;
		preset.fovNear = DirectX::XMConvertToRadians(45.0f);// ボールが近いときのFOV
		preset.fovFar = DirectX::XMConvertToRadians(10.0f);// ボールが遠いときのFOV
		preset.zoomNearDist = 10.0f;// この距離以下でfovNear
		preset.zoomFarDist = 100.0f;// この距離以上でfovFar
		AddCameraPreset(preset);
	}

	activeCameraIndex = 0;// 最初のカメラをアクティブにする
}

void BroadcastCamera::Update(float elapsed_time, bool ballHasCollidedWithBat)
{
	// 数字キーで切り替え
	for (int i = 0; i < static_cast<int>(cameraPresets.size()) && i < 9; ++i)
	{
		if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i)))
		{
			activeCameraIndex = i;
		}
	}

	if (cameraPresets.empty())
	{
		SetupDefaultCameras();
	}
	activeCameraIndex = (std::max)(0, (std::min)(activeCameraIndex, static_cast<int>(cameraPresets.size() - 1)));

	CameraPreset& preset = cameraPresets[activeCameraIndex];
	if (ballHasCollidedWithBat
		&& preset.enableTrackingZoom)
	{
		Physics::Instance().ClearBallWasHit();
		for (auto& controller : cameraControllers)
			controller.StartTrackingBall(&Ball::Instance(), 3.0f, -30.0f);
	}

	cameraControllers[activeCameraIndex].Update(elapsed_time); // 仮の経過時間を渡す
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
		AddCameraPreset(preset);
	}
	activeCameraIndex = j.value("relay_cameras_active_index", 0);
	activeCameraIndex = (std::max)(0, (std::min)(activeCameraIndex, static_cast<int>(cameraPresets.size()) - 1));
}