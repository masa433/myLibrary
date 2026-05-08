#include "scene_game.h"
#include "gltf_model.h"
#include "camera.h"
#include <DirectXMath.h>
#include "imgui.h"
#include "stage.h"
#include "player.h"
#include "Pitcher.h"
#include "Graphics.h"
#include "RenderContext.h"
#include "misc.h"
#include "physxManager.h"
#include "GpuResourceUtils.h"
#include "shader.h"

void scene_game::initialize()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    // カメラ設定をここに移動
    float screenWidth = Graphics::Instance().GetScreenWidth();
    float screenHeight = Graphics::Instance().GetScreenHeight();

    Camera& camera = Camera::Instance();
    camera.SetPerspectiveFov(
        DirectX::XMConvertToRadians(45),
        screenWidth / screenHeight,
        camera_near_z,
        camera_far_z
    );
    camera.SetLookAt(
        { 0.0f, 1.2f, -3.5f },
        { 0.0f, 0.0f, 14.0f },
        { 0, 1, 0 }
    );
    cameraController.SyncCameraToController(camera);

    //定数バッファの作成
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;

        buffer_desc.ByteWidth = sizeof(scene_constants);
        HRESULT hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = sizeof(light_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, light_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }
    

    //物理システムの初期化
	Physics::Instance().Initialize();

    // ステージの初期化
    stage::Instance().initialize();

    // プレイヤーの初期化
    Player::Instance().Initialize();

	// ピッチャーの初期化
    Pitcher::Instance().Initialize();

    //ストライクゾーンの初期化
    strikeZoneSprite = std::make_unique<sprite>(device, L"./resources/sprite/strikeZone.png");

   
    // ポイントライト・スポットライトの初期位置設定
    {
        pointLights[0].position.x = 10;
        pointLights[0].position.y = 1;
        pointLights[0].intensity = 10;
        pointLights[0].color = { 1, 0, 0, 1 };
        pointLights[1].position.x = -10;
        pointLights[1].position.y = 1;
        pointLights[1].intensity = 10;
        pointLights[1].color = { 0, 1, 0, 1 };
        pointLights[2].position.y = 1;
        pointLights[2].position.z = 10;
        pointLights[2].intensity = 10;
        pointLights[2].position.y = 1;
        pointLights[2].color = { 0, 0, 1, 1 };
        pointLights[3].position.y = 1;
        pointLights[3].position.z = -10;
        pointLights[3].intensity = 10;
        pointLights[3].color = { 1, 1, 1, 1 };
        pointLights[4].intensity = 10;
        pointLights[4].color = { 1, 1, 1, 1 };
        ZeroMemory(&pointLights[5], sizeof(point_lights) * 3);
        spotLights[0].position = { 15, 3, 15, 0 };
        spotLights[0].direction = { -1, -1, -1, 0 };
        spotLights[0].range = 100;
        spotLights[0].color = { 1, 0, 0, 1 };
        spotLights[1].position = { -15, 3, 15, 0 };
        spotLights[1].direction = { +1, -1, -1, 0 };
        spotLights[1].range = 100;
        spotLights[1].color = { 0, 1, 0, 1 };
        spotLights[2].position = { 15, 3, -15, 0 };
        spotLights[2].direction = { -1, -1, +1, 0 };
        spotLights[2].range = 100;
        spotLights[2].color = { 0, 0, 1, 1 };
        spotLights[3].position = { -15, 3, -15, 0 };
        spotLights[3].direction = { +1, -1, +1, 0 };
        spotLights[3].range = 100;
        spotLights[3].color = { 1, 1, 1, 1 };
        ZeroMemory(&spotLights[4], sizeof(spot_lights) * 2);
    }


}

void scene_game::update(float elapsed_time)
{
	elapsed_time *= timeScale;

    // カメラコントローラーの更新
	Camera& camera = Camera::Instance();
    cameraController.SyncControllerToCamera(camera);
    cameraController.Update();

    // ステージの更新
    stage::Instance().update(elapsed_time);

    // プレイヤーの更新
    Player::Instance().Update(elapsed_time);

	// ピッチャーの更新
    Pitcher::Instance().Update(elapsed_time);

    // 物理システムの更新
    Physics::Instance().Update(elapsed_time);


#ifdef USE_IMGUI
    RenderContext rc;
    //Camera& camera = Camera::Instance();

    ImGui::Separator();
  
    if (ImGui::CollapsingHeader("Camera"))
    {
        DirectX::XMFLOAT3 eye = camera.GetEye();
        DirectX::XMFLOAT3 focus = camera.GetFocus();

        if (ImGui::DragFloat3("Camera Position", &eye.x, -50.0f, 100.0f))
        {
            camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
            cameraController.SyncCameraToController(camera);
        }

        if (ImGui::DragFloat3("Camera Focus", &focus.x, -50.0f, 100.0f))
        {
            camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
            cameraController.SyncCameraToController(camera);
        }

        ImGui::SliderFloat("Near Z", &camera_near_z, 0.1f, 100.0f);
        ImGui::SliderFloat("Far Z", &camera_far_z, 100.0f, 10000.0f);

        camera.SetPerspectiveFov(
            DirectX::XMConvertToRadians(45),
            Graphics::Instance().GetScreenWidth() / Graphics::Instance().GetScreenHeight(),
            camera_near_z,
            camera_far_z
		);
        
    }

    if (ImGui::CollapsingHeader("Light"))
    {
        ImGui::ColorEdit3("ambient_color", &ambient_color.x);
        ImGui::SliderFloat3("directional_light_direction", &directional_light_direction.x, -1.0f, +1.0f);
        ImGui::ColorEdit3("directional_light_color", &directional_light_color.x);
    
        if (ImGui::TreeNode("points"))
        {
            for (int i = 0; i < 6; ++i)
            {
                std::string p = std::string("position") + std::to_string(i);
                ImGui::SliderFloat3(p.c_str(), &pointLights[i].position.x, -10.0f, +10.0f);
                std::string c = std::string("color") + std::to_string(i);
                ImGui::ColorEdit3(c.c_str(), &pointLights[i].color.x);
                std::string it = std::string("intensity") + std::to_string(i);
                ImGui::SliderFloat(it.c_str(), &pointLights[i].intensity, 0.0f, +100.0f);
				std::string r = std::string("range") + std::to_string(i);  
				ImGui::SliderFloat(r.c_str(), &pointLights[i].range, 0.0f, +1000.0f);
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("spots"))
        {
            for (int i = 0; i < 6; ++i)
            {
                std::string p = std::string("position") + std::to_string(i);
                ImGui::SliderFloat3(p.c_str(), &spotLights[i].position.x, -10.0f, +10.0f);
                std::string d = std::string("direction") + std::to_string(i);
                ImGui::SliderFloat3(d.c_str(), &spotLights[i].direction.x, 0.0f, +1.0f);
                std::string c = std::string("color") + std::to_string(i);
                ImGui::ColorEdit3(c.c_str(), &spotLights[i].color.x);
                std::string r = std::string("range") + std::to_string(i);
                ImGui::SliderFloat(r.c_str(), &spotLights[i].range, 0.0f, +1000.0f);
                std::string ic = std::string("inner") + std::to_string(i);
                ImGui::SliderFloat(ic.c_str(), &spotLights[i].innerCorn, -1.0f, +1.0f);
                std::string oc = std::string("outer") + std::to_string(i);
                ImGui::SliderFloat(oc.c_str(), &spotLights[i].outerCorn, -1.0f, +1.0f);
				std::string it = std::string("intensity") + std::to_string(i);
				ImGui::SliderFloat(it.c_str(), &spotLights[i].intensity, 0.0f, +1000.0f);
            }
            ImGui::TreePop();
        }
    }

	ImGui::Checkbox("Show PhysX Debug", &showPhysxDebug);

    // ストライクゾーン画像の制御
    if (ImGui::CollapsingHeader("Strike Zone Image"))
    {
        ImGui::Checkbox("Show Image", &showStrikeZoneImage);
        ImGui::DragFloat2("Screen Position", &spritePosition.x, 1.0f, 0.0f, 2000.0f);
        ImGui::DragFloat2("Scale", &spriteScale.x, 0.01f, 0.1f, 5.0f);
        ImGui::ColorEdit4("Tint", &spriteTint.x);

        

    }

    // タイムスケール制御
    if (ImGui::Begin("Time Control", nullptr, ImGuiWindowFlags_None))
    {
        ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 2.0f);
        if (ImGui::Button(u8"一時停止 (0.0)"))
        {
            timeScale = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"通常速度 (1.0)"))
        {
            timeScale = 1.0f;
        }
    }
    ImGui::End();
#endif
}

void scene_game::render(float elapsedTime)
{
	//RenderShadowMap();

    using namespace DirectX;

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    // 描画コンテキスト設定
    RenderContext rc;
    rc.deviceContext = dc;
    rc.renderState = renderState;
    //rc.camera = &camera;
    /*rc.lightDirection = lightDirection;
	rc.lightColor = lightColor;
	rc.ambientColor = ambientColor;*/

    //カメラパラメータ設定
    Camera& camera = Camera::Instance();
    rc.view = camera.GetView();
    rc.projection = camera.GetProjection();

    // 定数バッファの更新
    {
        XMMATRIX V = XMLoadFloat4x4(&camera.GetView());
        XMMATRIX P = XMLoadFloat4x4(&camera.GetProjection());

        scene_constants scene{};
        scene.camera_position.x = cameraPosition.x;
        scene.camera_position.y = cameraPosition.y;
        scene.camera_position.z = cameraPosition.z;
        DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
        dc->UpdateSubresource(constant_buffer.Get(), 0, 0, &scene, 0, 0);
        dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
        dc->PSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());

        // ライト定数バッファの更新（スロット b2 に統一）
        light_constants lightConstants{};
        lightConstants.ambient_color = ambient_color;
        lightConstants.directional_light_direction = directional_light_direction;
        lightConstants.directional_light_color = directional_light_color;
		memcpy_s(lightConstants.pointLights, sizeof(lightConstants.pointLights), pointLights, sizeof(pointLights));
		memcpy_s(lightConstants.spotLights, sizeof(lightConstants.spotLights), spotLights, sizeof(spotLights));
        dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lightConstants, 0, 0);

        // スロット b4 のみ設定
        dc->VSSetConstantBuffers(4, 1, light_constant_buffer.GetAddressOf());
        dc->PSSetConstantBuffers(4, 1, light_constant_buffer.GetAddressOf());
    }
    
    dc->IASetInputLayout(mesh_input_layout.Get());
    dc->VSSetShader(mesh_vertex_shader.Get(), nullptr, 0);
    dc->PSSetShader(mesh_pixel_shader.Get(), nullptr, 0);


    //	モデルクラスでのラスタライザーステート設定をきったからここで設定する
    rc.deviceContext->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

    // プレイヤーの描画
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));



    // ステージの描画
    stage::Instance().render(rc, modelRenderer);

    // ピッチャーの描画
    Pitcher::Instance().Render(rc, modelRenderer);

    Player::Instance().Render(rc, modelRenderer);

    // レンダーステート設定
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
    //shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection(), rc.lightDirection);

    // サンプラーステートを設定
    ID3D11SamplerState* samplerStates[] = {
        renderState->GetSamplerState(SamplerState::LinearWrap),
        renderState->GetSamplerState(SamplerState::LinearClamp),
        renderState->GetSamplerState(SamplerState::LinearWrap)
    };
    dc->PSSetSamplers(0, 3, samplerStates);

    if(showPhysxDebug)
	Physics::Instance().Render(camera.GetView(), camera.GetProjection(), rc.lightDirection);

}

void scene_game::uninitialize()
{
    // 終了処理
    Player::Instance().Uninitialize();
    stage::Instance().uninitialize();
    Pitcher::Instance().Uninitialize();
    Physics::Instance().Finalize();
}

void scene_game::DrawGUI()
{
    // プレイヤーのGUI描画
    Player::Instance().DrawGUI();

	// ピッチャーのGUI描画
	Pitcher::Instance().DrawGUI();

}