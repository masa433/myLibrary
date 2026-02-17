#include "stage.h"
#include "imgui.h"
#include "Graphics.h"

// 初期化
void stage::initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	model = std::make_unique<gltf_model>(device, ".\\resources\\field\\stadium.gltf");
	// 位置、スケール、回転の初期化
	position = { 0.0f, 0.0f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };
	angle = { 0.0f, 0.0f, 0.0f };

	
}

// 更新
void stage::update(float elapsedTime)
{
#ifdef  USE_IMGUI
	if (ImGui::CollapsingHeader("Stage"))
	{
		ImGui::DragFloat3("Position", &position.x);
		ImGui::DragFloat3("Scale", &scale.x);
		ImGui::DragFloat3("Angle", &angle.x);
	}
#endif //  USE_IMGUI



	UpdateTransform();
}

void stage::render(RenderContext& rc)
{

	// ワールド変換行列の計算
	DirectX::XMMATRIX matScale = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX matRotX = DirectX::XMMatrixRotationX(angle.x);
	DirectX::XMMATRIX matRotY = DirectX::XMMatrixRotationY(angle.y);
	DirectX::XMMATRIX matRotZ = DirectX::XMMatrixRotationZ(angle.z);
	DirectX::XMMATRIX matTranslation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX worldMatrix = matScale * matRotZ * matRotY * matRotX * matTranslation;
	DirectX::XMStoreFloat4x4(&transform, worldMatrix);
	// モデルのレンダリング
	model->render(rc.context, transform, {});
}

// 終了
void stage::uninitialize()
{
	model.reset();
}

