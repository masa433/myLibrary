#pragma once


//カメラコントローラー
class CameraController 
{
public:
	CameraController();

	static CameraController& Instance() 
	{
		static CameraController instance;
		return instance;
	}
	//更新処理
	void Update(float elaspedTime);

	//ターゲット位置設定
	void SetTarget(const DirectX::XMFLOAT3& t) { this->target = t; }
	
	void DrawGUI();

private:
	DirectX::XMFLOAT3 target = { 0,0,0 };//注視点
	DirectX::XMFLOAT3 angle = { 0,0,0 };//回転角度
	float             rollSpeed = DirectX::XMConvertToRadians(90);//回転速度
	float			  range = 50.0f;//距離
	float			  maxAngleX = DirectX::XMConvertToRadians(45);
	float			  minAngleX = DirectX::XMConvertToRadians(0);
	
	
};