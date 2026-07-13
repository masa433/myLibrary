#include "FreeCameraController.h"
#include "imgui.h"
#include <cmath>

void FreeCameraController::SyncCameraToController(const Camera& camera)
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
void FreeCameraController::SyncControllerToCamera(Camera& camera)
{
	camera.SetLookAt(eye, focus, up);

}

void FreeCameraController::Update(float elapsedTime)
{
    if (!isGameViewHovered) return;

    ImGuiIO io = ImGui::GetIO();
    float moveX = io.MouseDelta.x * 0.02f;
    float moveY = io.MouseDelta.y * 0.02f;

    if (io.MouseDown[ImGuiMouseButton_Right])
    {
        angleY += moveX * 0.5f;
        if (angleY > DirectX::XM_PI) angleY -= DirectX::XM_2PI;
        if (angleY < -DirectX::XM_PI) angleY += DirectX::XM_2PI;

        angleX += moveY * 0.5f;
        if (angleX > DirectX::XM_PI) angleX -= DirectX::XM_2PI;
        if (angleX < -DirectX::XM_PI) angleX += DirectX::XM_2PI;
    }
    else if (io.MouseDown[ImGuiMouseButton_Middle])
    {
        float s = distance * 0.035f;
        focus.x += (-right.x * moveX + up.x * moveY) * s;
        focus.y += (-right.y * moveX + up.y * moveY) * s;
        focus.z += (-right.z * moveX + up.z * moveY) * s;
    }
    else if (io.MouseDown[ImGuiMouseButton_Left] && io.MouseDown[ImGuiMouseButton_Right])
    {
        distance += (-moveY - moveX) * distance * 0.1f;
    }
    else if (io.MouseWheel != 0)
    {
        distance -= io.MouseWheel * distance * 0.1f;
    }
    else if (io.MouseDown[ImGuiMouseButton_Left] && io.KeyCtrl)
    {
        float fx = focus.x - eye.x;
        float fy = focus.y - eye.y;
        float fz = focus.z - eye.z;
        float len = sqrtf(fx * fx + fy * fy + fz * fz);
        if (len > 0.0001f) { fx /= len; fy /= len; fz /= len; }

        float speed = distance * 0.01f;
        focus.x += fx * speed;
        focus.y += fy * speed;
        focus.z += fz * speed;

        angleY += moveX * 0.5f;
        if (angleY > DirectX::XM_PI) angleY -= DirectX::XM_2PI;
        if (angleY < -DirectX::XM_PI) angleY += DirectX::XM_2PI;
        angleX += moveY * 0.5f;
        if (angleX > DirectX::XM_PI) angleX -= DirectX::XM_2PI;
        if (angleX < -DirectX::XM_PI) angleX += DirectX::XM_2PI;
    }

    float sx = ::sinf(angleX), cx = ::cosf(angleX);
    float sy = ::sinf(angleY), cy = ::cosf(angleY);

    DirectX::XMVECTOR Front = DirectX::XMVectorSet(-cx * sy, -sx, -cx * cy, 0.0f);
    DirectX::XMVECTOR Right = DirectX::XMVectorSet(cy, 0, -sy, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVector3Cross(Right, Front);
    DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&focus);
    DirectX::XMVECTOR Dist = DirectX::XMVectorReplicate(distance);
    DirectX::XMVECTOR Eye = DirectX::XMVectorSubtract(Focus, DirectX::XMVectorMultiply(Front, Dist));

    DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
    DirectX::XMMATRIX World = DirectX::XMMatrixTranspose(View);
    Right = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1, 0, 0, 0), World);
    Up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0, 1, 0, 0), World);

    DirectX::XMStoreFloat3(&eye, Eye);
    DirectX::XMStoreFloat3(&up, Up);
    DirectX::XMStoreFloat3(&right, Right);
}