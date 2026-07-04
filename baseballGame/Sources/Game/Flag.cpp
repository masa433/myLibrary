#include "Flag.h"
#include "Graphics.h"
#include "imgui.h"
#include <Windows.h>

void Flag::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	flagModel = std::make_unique<gltf_model>(device, ".\\resources\\flags\\japanFlag.glb");
	position = { 0.0f, 70.0f, 140.0f };
	scale = { 2.0f, 2.0f, 2.0f };
	angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f };

	flagModel->build_static_batches(device);
}

void Flag::UnInitialize()
{
	flagModel.reset();
}

void Flag::Update(float elapsedTime)
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX world = S * R * T;
	DirectX::XMStoreFloat4x4(&transform, world);

	//ボックスの位置とサイズを更新

	UpdateTransform();
}

void Flag::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	if (flagModel)
	{
		flagModel->render_batched(rc.deviceContext, transform, {});
	}
}

void Flag::DrawGUI()
{
	ImGui::Begin("Flag Settings");
	ImGui::SliderFloat3("Position", &position.x, -20.0f, 20.0f);
	ImGui::SliderFloat3("Scale", &scale.x, 0.1f, 5.0f);
	ImGui::SliderFloat3("Angle", &angle.x, -DirectX::XM_PI, DirectX::XM_PI);
	ImGui::End();
}