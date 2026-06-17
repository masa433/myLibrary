#include "camera_controller.h"
#include "imgui.h"
#include "Ball.h"
#include <cmath>

//ラープ関数
DirectX::XMFLOAT3 CameraController::Lerp3(const DirectX::XMFLOAT3& a,
	const DirectX::XMFLOAT3& b,
	float t)
{
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t,
	};
}

// Smoothstep イージング関数
float CameraController::Smoothstep(float t)
{
	t = (std::max)(0.0f, (std::min)(1.0f, t));// 0→1 を滑らかにするイージング関数
	return t * t * (3.0f - 2.0f * t);
}

// ボールの速度方向からカメラの理想 eye を計算する
DirectX::XMFLOAT3 CameraController::CalcIdealEye(const DirectX::XMFLOAT3& ballPos,
	const DirectX::XMFLOAT3& ballVel) const
{
	//速度ベクトルを正規化して後方オフセットを求める
	float vx = ballVel.x, vy = ballVel.y, vz = ballVel.z;
	float len = sqrtf(vx * vx + vy * vy + vz * vz);

	DirectX::XMFLOAT3 backDir = { 0.0f,0.0f,-1.0f };//デフォルト
	if(len>0.01f)
	{
		backDir = { -vx / len, -vy / len, -vz / len };//速度ベクトルの逆方向
	}

	return {
		ballPos.x + backDir.x * trackOffsetBack,
		ballPos.y + backDir.y * trackOffsetBack + trackOffsetUp,
		ballPos.z + backDir.z * trackOffsetBack,
	};//ボールの位置から後方オフセットと上方オフセットを加算して理想的なカメラ位置を計算
}


// コントローラーからカメラへパラメータを同期する
void CameraController::SyncControllerToCamera(Camera& camera)
{
	camera.SetLookAt(eye, focus, up);
	
}


// ボール追跡カメラを開始する
void CameraController::StartTrackingBall(const Ball* ball, float offsetTracking, float offsetUp)
{
	
	if (!ball) return;

	// 追跡前の位置を保存
	savedEye = eye;
	savedFocus = focus;

	trackedBall = ball;
	/*trackOffsetBack = offsetTracking;
	trackOffsetUp = offsetUp;*/

	//現在のカメラ位置を補間開始位置として保存
	//transitionStartEye = eye;
	transitionStartFocus = focus;

	//スムーズ追従の初期値も現在位置に保存
	//smoothEye = eye;
	smoothFocus = focus;

	zoomTime = 0.0f;

	defaultFov = currentFov;
	trackingBlendTime = 0.0f;

	transitionTime = 0.0f;
	trackingState = TrackState::Transition;
}

// ボール追跡カメラを停止する
void CameraController::StopTrackingBall()
{
	trackedBall = nullptr;
	trackingState = TrackState::None;

	eye = savedEye;
	focus = savedFocus;

	currentFov = defaultFov;
}

// 更新処理
void CameraController::Update(float elapsedTime)
{
	//追跡状態のカメラ
	if (trackingState != TrackState::None && trackedBall)
	{
		//ボールの位置と速度を保存
		const DirectX::XMFLOAT3& ballPos = trackedBall->GetWorldPosition();
		
		
		if (trackingState == TrackState::Transition)
		{
			//現在位置から理想の位置への補間
			transitionTime += elapsedTime;
			float t = Smoothstep(transitionTime / transitionDuration);

			
			focus = Lerp3(transitionStartFocus, ballPos, t);

			// スムーズ追従バッファも同期しておく
			
			smoothFocus = focus;

			//追跡開始までの時間を超えたら次のステートに遷移
			if (transitionTime >= transitionDuration)
			{
				trackingState = TrackState::Tracking;
			}
		}
		else// TrackingState::Tracking
		{
			// Tracking開始直後は追従速度を抑えて徐々に本速度へ
			trackingBlendTime += elapsedTime;
			float speedBlend = Smoothstep(trackingBlendTime / trackingBlendDuration);
			float blendedFocusSpeed = TrackFocusSpeed * speedBlend;

			float focusLerp = 1.0f - expf(-blendedFocusSpeed * elapsedTime);
			smoothFocus = Lerp3(smoothFocus, ballPos, focusLerp);
			focus = smoothFocus;

			
			// ズームアウト処理（カメラ3用）
			if (enableTrackingZoom)
			{
				//カメラからボールまでの距離を計算
				float dx = ballPos.x - eye.x;
				float dy = ballPos.y - eye.y;
				float dz = ballPos.z - eye.z;
				float dist = sqrtf(dx * dx + dy * dy + dz * dz);

				//距離に応じてズームアウトする
				float t = (dist - zoomNearDist) / (zoomFarDist - zoomNearDist);
				t = (std::max)(0.0f, (std::min)(1.0f, t));// 0→1 にクランプ

				//FOVをマッピング(遠いほど広角)
				float targetFov = fovNear + (fovFar - fovNear) * t;

				// 急変を防ぐため指数補間でスムーズに追従
				float fovLerp = 1.0f - expf(-fovSmoothSpeed * elapsedTime);
				currentFov += (targetFov - currentFov) * fovLerp;
			}
			
		}

		// up ベクトルは常にワールド Y 軸方向で固定
		up = { 0.0f, 1.0f, 0.0f };

		// カメラへ反映
		// （呼び出し元が SyncControllerToCamera を毎フレーム呼ぶ前提）

		return;
	}

	
}
