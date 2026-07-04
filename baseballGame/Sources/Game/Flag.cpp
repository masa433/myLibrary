#include "Flag.h"
#include "Graphics.h"
#include "Wind.h"
#include "imgui.h"
#include <Windows.h>

const char* Flag::GetModelPath(FlagColor color)
{
    switch (color)
    {
    case FlagColor::Blue:   return ".\\resources\\flags\\blueFlag.glb";
    case FlagColor::Green:  return ".\\resources\\flags\\greenFlag.glb";
    case FlagColor::Japan:  return ".\\resources\\flags\\japanFlag.glb";
    case FlagColor::Red:    return ".\\resources\\flags\\redFlag.glb";
    case FlagColor::Yellow: return ".\\resources\\flags\\yellowFlag.glb";
    }
    return ".\\resources\\flags\\japanFlag.glb";
}

void Flag::Initialize(int index, FlagColor color, float spacing,
    const DirectX::XMFLOAT3& basePosition, float baseYAngleDeg)
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    flagColor = color;
    flagModel = std::make_unique<gltf_model>(device, GetModelPath(color));
    flagModel->build_static_batches(device);

    // 5本を中央揃えで等間隔に並べる
    const int totalFlags = 5;
    const float offset = (index - (totalFlags - 1) * 0.5f) * spacing;

    position = { basePosition.x + offset, basePosition.y, basePosition.z };
    scale = { 2.0f, 2.0f, 2.0f };
    angle = { 0.0f, DirectX::XMConvertToRadians(baseYAngleDeg), 0.0f };

    UpdateTransform();
}

void Flag::UnInitialize()
{
   
    flagModel.reset();
}

void Flag::Update(float elapsedTime)
{
    windTime += elapsedTime;

 
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    DirectX::XMMATRIX world = S * R * T;
    DirectX::XMStoreFloat4x4(&transform, world);

    UpdateTransform();
}

void Flag::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    if (!flagModel)
    {
        return;
    }


    flagModel->render_batched(rc.deviceContext, transform, {});
}

void Flag::DrawGUI(int index)
{
    static const char* colorNames[] = { "Blue", "Green", "Japan", "Red", "Yellow" };

    ImGui::PushID(index);
    if (ImGui::TreeNode("", "Flag[%d] : %s", index, colorNames[static_cast<int>(flagColor)]))
    {
        ImGui::SliderFloat3("Position", &position.x, -20.0f, 20.0f);
        ImGui::SliderFloat3("Scale", &scale.x, 0.1f, 5.0f);
        ImGui::SliderFloat3("Angle", &angle.x, -DirectX::XM_PI, DirectX::XM_PI);
        ImGui::Checkbox("Use Physics Simulation", &usePhysicsSimulation);
      
    }
    ImGui::PopID();
}

