#pragma once
#include "Camera.h"

class Ball;

class CameraController
{
public:
	// カメラからコントローラーへパラメータを同期する
	void SyncCameraToController(const Camera& camera);

	// コントローラーからカメラへパラメータを同期する
	void SyncControllerToCamera(Camera& camera);

	// 更新処理
	void Update(float elapsedTime);

	//ボール追跡カメラ
	//バットにボールが当たった瞬間に呼び出される
	// ball        : 追跡対象のBallインスタンス
	// offsetBack  : ボール後方への距離（デフォルト 3.0m）
	// offsetUp    : ボール上方へのオフセット（デフォルト 0.5m）
	void StartTrackingBall(const Ball* ball, float offsetTracking = 3.0f, float offsetUp = 0.5f);

	//ボール追跡カメラを停止する
	void StopTrackingBall();

	//ボール追跡カメラが有効かどうか
	bool IsTrackingBall() const { return trackingState != TrackState::None; }

private:
	DirectX::XMFLOAT3		eye;
	DirectX::XMFLOAT3		focus;
	DirectX::XMFLOAT3		up;
	DirectX::XMFLOAT3		right;
	float					distance;

	float					angleX;
	float					angleY;

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

	// スムーズ追従用（現在の eye/focus を保持して lerp する）
	DirectX::XMFLOAT3   smoothEye = {};
	DirectX::XMFLOAT3   smoothFocus = {};

	float transitionTime = 0.0f;
	static constexpr float transitionDuration = 0.5f; // 追跡開始位置への補間時間（秒）

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

	float zoomTime = 0.0f;
	float zoomSpeed = 1.0f;

	float minEyeY = 0.5f; // カメラの最低高さ
	float maxEyeY = 10.0f; // カメラの最高高さ

public:
	bool isGameViewHovered = false;
	void SetIsGameViewHovered(bool hovered) { isGameViewHovered = hovered; }
};
