#pragma once
#include "camera.h"

class FreeCameraController
{
public:
	// カメラからコントローラーへパラメータを同期する
	void SyncCameraToController(const Camera& camera);

	// コントローラーからカメラへパラメータを同期する
	void SyncControllerToCamera(Camera& camera);

	// 更新処理
	void Update(float elapsedTime);

public:
	bool isGameViewHovered = false;
	void SetIsGameViewHovered(bool hovered) { isGameViewHovered = hovered; }

private:
	DirectX::XMFLOAT3		eye;
	DirectX::XMFLOAT3		focus;
	DirectX::XMFLOAT3		up;
	DirectX::XMFLOAT3		right;
	float					distance;

	float					angleX;
	float					angleY;
};