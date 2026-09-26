#pragma once
#include "Camera.h"

class Ball;

class CameraController
{
public:
	

	// コントローラーからカメラへパラメータを同期する
	void SyncControllerToCamera(Camera& camera);

	// 更新処理
	void Update(float elapsedTime);

	// リプレイ等で位置座標を直接セットする用
	void SetTrackedPosition(const DirectX::XMFLOAT3& pos) { trackedPosition = pos; }

	//リプレイカメラ用の追跡カメラ
	void StartTrackingReplayCamera(const DirectX::XMFLOAT3& ballPos, float offsetBack = 3.0f, float offsetUp = 0.5f, bool lockY = false);

	//リプレイカメラの追跡カメラを停止する
	void StopTrackingReplayCamera();

	//ボール追跡カメラ
	//バットにボールが当たった瞬間に呼び出される
	// ball        : 追跡対象のBallインスタンス
	// offsetBack  : ボール後方への距離（デフォルト 3.0m）
	// offsetUp    : ボール上方へのオフセット（デフォルト 0.5m）
	// lockFocusY : 追跡中に focus の Y 座標を固定するかどうか（デフォルト false）
	void StartTrackingBall(const Ball* ball, float offsetTracking = 3.0f, float offsetUp = 0.5f, bool lockY = false);

	//ボール追跡カメラを停止する
	void StopTrackingBall();

	//ボール追跡カメラが有効かどうか
	bool IsTrackingBall() const { return trackingState != TrackState::None; }

	float GetCurrentFov() const { return currentFov; }
	void SetFov(float fov) { currentFov = fov; }

	void SetEyeAndFocus(const DirectX::XMFLOAT3& e, const DirectX::XMFLOAT3& f)
	{
		eye = e;
		focus = f;
		up = { 0.0f, 1.0f, 0.0f };
	}

	void DrawGUI();

private:
	DirectX::XMFLOAT3		eye;
	DirectX::XMFLOAT3		focus;
	DirectX::XMFLOAT3		up;
	DirectX::XMFLOAT3		right;
	float					distance;


	// ボール追跡カメラの状態
	enum class TrackState
	{
		None,       // 追跡なし
		Transition,  // 現在位置 → 追跡開始位置へ補間中
		Tracking,   // 追跡中
	};
	TrackState trackingState = TrackState::None;
	const Ball* trackedBall = nullptr;
	float trackOffsetBack = 3.0f;
	float trackOffsetUp = 0.5f;

	// 追跡開始位置への補間用
	DirectX::XMFLOAT3 transitionStartEye = {};
	DirectX::XMFLOAT3 transitionStartFocus = {};
	bool lockFocusY = false; // 追跡中に focus の Y 座標を固定するかどうか
	float trackedFocusY = 0.0f; // 追跡中固定するfocusのY座標

	// スムーズ追従用（現在の eye/focus を保持して lerp する）
	DirectX::XMFLOAT3   smoothEye = {};
	DirectX::XMFLOAT3   smoothFocus = {};

	float transitionTime = 0.0f;
	static constexpr float transitionDuration = 1.0f; // 追跡開始位置への補間時間（秒）

	//追跡中の追従速度
	static constexpr float TrackEyeSpeed = 5.0f;
	static constexpr float TrackFocusSpeed = 8.0f;

	// ボールの速度方向からカメラの理想 eye を計算する
	DirectX::XMFLOAT3 CalcIdealEye(const DirectX::XMFLOAT3& ballPos,
		const DirectX::XMFLOAT3& ballVel) const;

	static DirectX::XMFLOAT3 Lerp3(const DirectX::XMFLOAT3& a,
		const DirectX::XMFLOAT3& b,
		float t);

	// Smoothstep イージング（0→1 を滑らかに）
	static float Smoothstep(float t);

	

private:
	// 追跡開始前のカメラ位置を保存
	DirectX::XMFLOAT3 savedEye = {};
	DirectX::XMFLOAT3 savedFocus = {};

	float currentFov = DirectX::XMConvertToRadians(45.0f);
	float defaultFov = DirectX::XMConvertToRadians(45.0f);
	float zoomedFov = DirectX::XMConvertToRadians(10.0f); // 追跡中のズーム画角

	float zoomTime = 0.0f;
	float zoomSpeed = 5.0f;

	float minEyeY = 0.5f; // カメラの最低高さ
	float maxEyeY = 10.0f; // カメラの最高高さ

	DirectX::XMFLOAT3 trackedPosition = {}; // リプレイカメラ用の追跡位置

public:
	void SetTrackingZoomOut(bool enable,
		float fovAtNear = DirectX::XMConvertToRadians(5.0f),
		float fovAtFar = DirectX::XMConvertToRadians(15.0f),
		float nearDist = 10.0f,
		float farDist = 130.0f)
	{
		enableTrackingZoom = enable;
		fovNear = fovAtNear;
		fovFar = fovAtFar;
		zoomNearDist = nearDist;
		zoomFarDist = farDist;
	}
private:
	bool  enableTrackingZoom = false;
	float fovNear = DirectX::XMConvertToRadians(5.0f);  // ボールが近いときのFOV
	float fovFar = DirectX::XMConvertToRadians(15.0f); // ボールが遠いときのFOV
	float zoomNearDist = 10.0f;  // この距離以下でfovNear
	float zoomFarDist = 130.0f; // この距離以上でfovFar
	float fovSmoothSpeed = 3.0f; // FOV補間速度
private:
		float trackingBlendTime = 0.0f;                    // Tracking開始からの経過時間
		static constexpr float trackingBlendDuration = 0.5f; // この秒数かけて本速度に移行

public:

	//ホームラン着弾時の球ズーム演出
	void TriggerImpactZoom(float impactFov = DirectX::XMConvertToRadians(5.0f), float duration = 5.0f);

	//ボールに依存しないカメラ演出用のズーム演出
	// impactFov : ズーム後の画角（ラジアン）
	// duration  : ズーム演出の持続時間（秒）
	void StartEventZoom(float impactFov, float duration);

	void StartEventFucusYShift(float targetFocusY, float duration);

	void StartEventFocusZShift(float targetFocusZ, float duration);

	//カメラのeyeXとeyeZを減少させる関数
	void StartEventEyeXZShift(float targetEyeX, float targetEyeZ, float centerTargetEyeZ, float duration);

private:
	bool impactZoomActive = false;
	float impactZoomFov = 0.0f;
	float impactZoomDuration = 5.0f;

	bool eventCameraZoomActive = false;
	float eventCameraTargetFov = 0.0f;
	float eventCameraStartFov = 0.0f;
	float eventCameraZoomDuration = 10.0f;
	float eventCameraZoomTime = 0.0f;

	bool eventFocusYShiftActive = false;
	float eventFocusYShiftTarget = 0.0f;
	float eventFocusYShiftStart = 0.0f;
	float eventFocusYShiftDuration = 0.0f;
	float eventFocusYShiftTime = 0.0f;

	bool eventFocusZShiftActive = false;
	float eventFocusZShiftTarget = 0.0f;
	float eventFocusZShiftStart = 0.0f;
	float eventFocusZShiftDuration = 0.0f;
	float eventFocusZShiftTime = 0.0f;

	bool eventEyeXZShiftActive = false;
	float eventEyeXZShiftTargetX = 0.0f;
	float eventEyeXZShiftTargetZ = 0.0f;
	float eventEyeXZShiftStartX = 0.0f;
	float eventEyeXZShiftStartZ = 0.0f;
	float eventEyeXZShiftDuration = 0.0f;
	float eventEyeXZShiftTime = 0.0f;
	float eventEyeCenterTargetZ = 0.0f;
};
