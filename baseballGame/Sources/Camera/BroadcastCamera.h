#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <camera_controller.h>
#include "json.hpp"

using json = nlohmann::json;

//カメラの種類の列挙
enum class CameraType
{
	NormalCamera,  //通常カメラ
	HitCamera,    //ヒットカメラ
	HomeRunCamera, //ホームランカメラ
	ReplayCamera,  //リプレイカメラ
	EventCamera,   //イベントカメラ
};

class BroadcastCamera
{
public:

	//カメラの位置定義
	struct CameraPreset
	{
		std::string name;
		DirectX::XMFLOAT3 eye;
		DirectX::XMFLOAT3 focus;
		float fov = DirectX::XMConvertToRadians(45.0f);
		bool enableTrackingZoom = false;
		float fovNear = DirectX::XMConvertToRadians(5.0f);  // ボールが近いときのFOV
		float fovFar = DirectX::XMConvertToRadians(15.0f); // ボールが遠いときのFOV
		float zoomNearDist = 10.0f;  // この距離以下でfovNear
		float zoomFarDist = 100.0f;  // この距離以上でfovFar
		CameraType type = CameraType::HomeRunCamera; // カメラの種類
		int cameraId = -1; // カメラのID（必要に応じて使用）
		bool lockFocusY = false; // 追跡中に focus の Y 座標を固定するかどうか
	};

	//カメラ関連の関数
	int AddCameraPreset(const CameraPreset& preset);//カメラ追加関数
	int AddCameraPreset(const std::string& name, const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus);//カメラ追加関数
	void RemoveCameraPreset(int index);//カメラ削除関数
	void ApplyPresetToController(const CameraPreset& preset, CameraController& controller);//カメラプリセットをコントローラーに適用する関数
	void SetupDefaultCameras();//デフォルトカメラの設定関数

	void Update(float elapsed_time, bool ballHasCollidedWithBat);     // 数字キー切替+追跡ロジック
	void SyncToCamera(Camera& camera, float aspect, float nearZ, float farZ); // 実際にCameraへ反映
	void DrawGUI();                                        // 「中継カメラ」ヘッダーの中身

	void SaveToJson(json& j) const;
	void LoadFromJson(const json& j);

	bool IsTrackingBall() const;
	void StopAllTracking();

	//イベントカメラのfovを時間で狭める関数
	void StartEventCameraZoom(float targetFov, float duration);

	std::string GetActiveCameraName() const
	{
		if(cameraPresets.empty() || activeCameraIndex < 0 || activeCameraIndex >= static_cast<int>(cameraPresets.size()))
		{
			return ""; // デフォルトの名前を返す
		}
		return cameraPresets[activeCameraIndex].name;
	}
	int GetActiveIndex() const { return activeCameraIndex; }

	void SetActiveIndex(int i)
	{
		if (cameraPresets.empty()) { activeCameraIndex = 0; return; }
		activeCameraIndex = (std::max)(0, (std::min)(i, static_cast<int>(cameraPresets.size()) - 1));
	}

	std::vector<CameraPreset>& Presets() { return cameraPresets; }
	
	CameraType GetActiveCameraType() const
	{
		if(cameraPresets.empty() || activeCameraIndex < 0 || activeCameraIndex >= static_cast<int>(cameraPresets.size()))
		{
			return CameraType::NormalCamera; // デフォルトのカメラタイプを返す
		}
		return cameraPresets[activeCameraIndex].type;
	}

	int GetCameraIndexById(int cameraId) const
	{
		for (size_t i = 0; i < cameraPresets.size(); ++i)
		{
			if (cameraPresets[i].cameraId == cameraId)
			{
				return static_cast<int>(i);
			}
		}
		return activeCameraIndex; // 見つからなかったらデフォルトのカメラを返す
	}

	void ResetCameraToPreset(int index)
	{
		if (index < 0 || index >= static_cast<int>(cameraPresets.size()))
			return; // インデックスが範囲外の場合は何もしない
		ApplyPresetToController(cameraPresets[index], cameraControllers[index]);
	}

	std::string GetPresetNameById(int cameraId) const;

	bool prevHasCollidedWithBat = false; // 前フレームでボールがバットに当たったかどうかのフラグ
	bool prevHasShowTrackingData = false; // 前フレームで追跡データを表示していたかどうかのフラグ
	bool prevIsHomeRun = false; // 前フレームでホームランだったかどうかのフラグ
	bool forceLockFocusYThisPlay = false;
	bool hasTriggeredImpactZoom = false;//ホームラン時のズームを一度だけ発動させるためのフラグ
	float zoomStartDelay = 0.0f; // ズーム開始までの遅延時間（秒）
	float zoomStartTime = 0.2f; // ズーム開始までの時間（秒）
private:
	//カメラ配列
	std::vector<CameraPreset> cameraPresets;//カメラプリセットの配列
	std::vector<CameraController> cameraControllers;//カメラコントローラーの配列
	int activeCameraIndex = 0;


};