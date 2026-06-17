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

// カメラからコントローラーへパラメータを同期する
void CameraController::SyncCameraToController(const Camera& camera)
{
	eye = camera.GetEye();
	focus = camera.GetFocus();
	up = camera.GetUp();
	right = camera.GetRight();

	// 視点から注視点までの距離を算出
	DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&focus);
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Focus, Eye);
	DirectX::XMVECTOR Distance = DirectX::XMVector3Length(Vec);
	DirectX::XMStoreFloat(&distance, Distance);

	// 回転角度を算出
	const DirectX::XMFLOAT3& front = camera.GetFront();
	angleX = ::asinf(-front.y);
	if (up.y < 0)
	{
		if (front.y > 0)
		{
			angleX = -DirectX::XM_PI - angleX;
		}
		else
		{
			angleX = DirectX::XM_PI - angleX;
		}
		angleY = ::atan2f(front.x, front.z);
	}
	else
	{
		angleY = ::atan2f(-front.x, -front.z);
	}

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

	zoomTime = 0.0f;
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
			//ボールを指数補間で追う
			//   lerpFactor = 1 - exp(-speed * dt)  で dt に依存しない追従速度になる
			float focusLerp = 1.0f - expf(-TrackFocusSpeed * elapsedTime);

			smoothFocus = Lerp3(smoothFocus, ballPos, focusLerp);

			focus = smoothFocus;

			
			zoomTime += elapsedTime;
			if (zoomTime > 1.0f) 
			{
				float fovLerp = 1.0f - expf(-zoomSpeed * elapsedTime);
				currentFov += (zoomedFov - currentFov) * fovLerp;
			}

			/*DirectX::XMFLOAT3 dir =
			{
				focus.x - eye.x,
				focus.y - eye.y,
				focus.z - eye.z,
			};
			float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
			if (len > 30.0f)
			{
				float zoomLerp = 1.0f - expf(-zoomSpeed * elapsedTime);
				eye.x += dir.x / len * len * zoomLerp;
				eye.y += dir.y / len * len * zoomLerp;
				eye.z += dir.z / len * len * zoomLerp;
			}*/
			
		}

		// up ベクトルは常にワールド Y 軸方向で固定
		up = { 0.0f, 1.0f, 0.0f };

		// カメラへ反映
		// （呼び出し元が SyncControllerToCamera を毎フレーム呼ぶ前提）

		//yのフォーカス点に高さ制限を設ける
		if (focus.y < 0.5f)
		{
			focus.y = 0.5f;
		}

		return;
	}

	// Game View にマウスがない場合は処理しない
	if (!isGameViewHovered)
	{
		return;
	}

	// IMGUIのマウス入力値を使ってカメラ操作する
	ImGuiIO io = ImGui::GetIO();

	// マウスカーソルの移動量を求める
	float moveX = io.MouseDelta.x * 0.02f;
	float moveY = io.MouseDelta.y * 0.02f;

	// マウス右ボタン押下中
	if (io.MouseDown[ImGuiMouseButton_Right])
	{
		// Y軸回転
		angleY += moveX * 0.5f;
		if (angleY > DirectX::XM_PI)
		{
			angleY -= DirectX::XM_2PI;
		}
		else if (angleY < -DirectX::XM_PI)
		{
			angleY += DirectX::XM_2PI;
		}
		// X軸回転
		angleX += moveY * 0.5f;
		if (angleX > DirectX::XM_PI)
		{
			angleX -= DirectX::XM_2PI;
		}
		else if (angleX < -DirectX::XM_PI)
		{
			angleX += DirectX::XM_2PI;
		}
	}
	// マウス中ボタン押下中
	else if (io.MouseDown[ImGuiMouseButton_Middle])
	{
		// 平行移動
		float s = distance * 0.035f;
		float x = moveX * s;
		float y = moveY * s;

		focus.x -= right.x * x;
		focus.y -= right.y * x;
		focus.z -= right.z * x;

		focus.x += up.x * y;
		focus.y += up.y * y;
		focus.z += up.z * y;
	}
	// マウス右ボタン押下中
	else if (io.MouseDown[ImGuiMouseButton_Left] && io.MouseDown[ImGuiMouseButton_Right])
	{
		// ズーム
		distance += (-moveY - moveX) * distance * 0.1f;
	}
	// マウスホイール
	else if (io.MouseWheel != 0)
	{
		// ズーム
		distance -= io.MouseWheel * distance * 0.1f;
	}
	// Ctrlキー + 左クリック長押しで前進
	else if (io.MouseDown[ImGuiMouseButton_Left] && io.KeyCtrl)
	{
		// カメラの向いている方向（フォーカスからカメラへの逆ベクトル）
		// frontベクトルを計算（カメラが向いている前方向）
		float frontX = focus.x - eye.x;
		float frontY = focus.y - eye.y;
		float frontZ = focus.z - eye.z;

		// 正規化
		float len = sqrtf(frontX * frontX + frontY * frontY + frontZ * frontZ);
		if (len > 0.0001f)
		{
			frontX /= len;
			frontY /= len;
			frontZ /= len;
		}

		// 移動速度（distanceに比例させると遠いほど速く移動）
		float speed = distance * 0.01f;

		// フォーカス点とカメラ位置を同時に移動（カメラの向きを維持）
		focus.x += frontX * speed;
		focus.y += frontY * speed;
		focus.z += frontZ * speed;

		//カーソル移動で視点回転を追加
		angleY += moveX * 0.5f;
		if (angleY > DirectX::XM_PI)       angleY -= DirectX::XM_2PI;
		else if (angleY < -DirectX::XM_PI) angleY += DirectX::XM_2PI;

		angleX += moveY * 0.5f;
		if (angleX > DirectX::XM_PI)       angleX -= DirectX::XM_2PI;
		else if (angleX < -DirectX::XM_PI) angleX += DirectX::XM_2PI;
	}

	float sx = ::sinf(angleX);
	float cx = ::cosf(angleX);
	float sy = ::sinf(angleY);
	float cy = ::cosf(angleY);

	// カメラの方向を算出
	DirectX::XMVECTOR Front = DirectX::XMVectorSet(-cx * sy, -sx, -cx * cy, 0.0f);
	DirectX::XMVECTOR Right = DirectX::XMVectorSet(cy, 0, -sy, 0.0f);
	DirectX::XMVECTOR Up = DirectX::XMVector3Cross(Right, Front);
	// カメラの視点＆注視点を算出
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&focus);
	DirectX::XMVECTOR Distance = DirectX::XMVectorSet(distance, distance, distance, 0.0f);
	DirectX::XMVECTOR Eye = DirectX::XMVectorSubtract(Focus, DirectX::XMVectorMultiply(Front, Distance));
	// ビュー行列からワールド行列を算出
	DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
	DirectX::XMMATRIX World = DirectX::XMMatrixTranspose(View);
	// ワールド行列から方向を算出
	Right = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1, 0, 0, 0), World);
	Up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0, 1, 0, 0), World);
	// 結果を格納
	DirectX::XMStoreFloat3(&eye, Eye);
	DirectX::XMStoreFloat3(&up, Up);
	DirectX::XMStoreFloat3(&right, Right);
}
