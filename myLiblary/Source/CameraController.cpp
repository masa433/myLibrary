#include "pch.h"
#include "System/Input.h"
#include "CameraController.h"
#include "Camera.h"
#include "Character.h"


// コンストラクタ
CameraController::CameraController()
	: target(0.0f, 0.0f, 0.0f), // 注視点の初期値
	angle(DirectX::XMConvertToRadians(30.0f), 0.0f, 0.0f), // 初期角度を設定 (見下ろし視点)
	range(50.0f) // カメラ距離
	
	
{
}


//更新処理
void CameraController::Update(float elapsedTime) 
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisRX();
	float ay = gamePad.GetAxisRY();
	//カメラの回転速度
	float speed = rollSpeed * elapsedTime;

	//スティックの入力値に合わせてX軸とY軸を回転
	angle.x += ay * speed;
	angle.y += ax * speed;

	////X軸のカメラ回転を制限
	//if (angle.x < minAngleX) 
	//{
	//	angle.x = minAngleX;
	//}
	//if (angle.x > maxAngleX) 
	//{
	//	angle.x = maxAngleX;
	//}

	//Y軸の回転値を-3.14～3.14に収まるようにする
	if (angle.y < -DirectX::XM_PI) 
	{
		angle.y += DirectX::XM_2PI;
	}
	if (angle.y > DirectX::XM_PI) 
	{
		angle.y -= DirectX::XM_2PI;
	}

	//カメラ回転値を回転行列に変換
	DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

	//回転行列から前方向ベクトルを取り出す
	DirectX::XMVECTOR Front = Transform.r[2];//行列の3行目のデータを取り出し
	DirectX::XMFLOAT3 front;
	DirectX::XMStoreFloat3(&front, Front);


	//注視点から後ろベクトル方向に一定距離離れたカメラ視点を求める
	DirectX::XMFLOAT3 eye;
	eye.x = target.x - front.x * range;
	eye.y = target.y - front.y * range;
	eye.z = target.z - front.z * range;

	//カメラの視点と注視点を設定
	Camera::Instance().SetLookAt(eye, target, DirectX::XMFLOAT3(0, 1, 0));
	//ターゲットを中心にカメラを回転
}

void CameraController::DrawGUI() 
{
    ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
    ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 200), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Camera", nullptr, ImGuiWindowFlags_None))
    {
        // トランスフォーム
        if (ImGui::CollapsingHeader("angle", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 回転 (角度に変換して表示)
			DirectX::XMFLOAT3 a;
			a.x = DirectX::XMConvertToDegrees(angle.x);
			a.y = DirectX::XMConvertToDegrees(angle.y);
			a.z = DirectX::XMConvertToDegrees(angle.z);
			ImGui::InputFloat3("Angle", &a.x);
			angle.x = DirectX::XMConvertToRadians(a.x);
			angle.y = DirectX::XMConvertToRadians(a.y);
			angle.z = DirectX::XMConvertToRadians(a.z);
        }
    }
    ImGui::End();
}
