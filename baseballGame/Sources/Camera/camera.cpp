#include "Camera.h"

//指定方向を向く
void Camera::SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up)
{
	DirectX::XMFLOAT3 safeFocus = focus;
    //eyeとfocusが一致していると行列計算のアサートで落ちるため少しずらす
    if (eye.x == focus.x && eye.y == focus.y && eye.z == focus.z)
    {
        safeFocus.z += 0.0001f; // 微小なオフセットを加える
    }

    DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
    DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&safeFocus);
    DirectX::XMVECTOR Up = DirectX::XMLoadFloat3(&up);
    DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
    //LH=Left Hand(左手系用)
    DirectX::XMStoreFloat4x4(&view, View);

    // カメラの方向を取り出す
    this->right.x = view._11;
    this->right.y = view._21;
    this->right.z = view._31;

    this->up.x = view._12;
    this->up.y = view._22;
    this->up.z = view._32;

    this->front.x = view._13;
    this->front.y = view._23;
    this->front.z = view._33;

    // 視点、注視点を保存
    this->eye = eye;
    this->focus = focus;
}

//パースペクティブ設定
void Camera::SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ)
{
    this->fovY = fovY;
    //画角、画面比率、クリップ距離からプロジェクション行列を作成
    DirectX::XMMATRIX Projection = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);

    DirectX::XMStoreFloat4x4(&projection, Projection);
}

