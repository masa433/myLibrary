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


scene_game::scene_game()
{
    // ライト設定のみコンストラクタで行う
    DirectionalLight directionalLight;
    directionalLight.direction = { 0, -1, 0 };
    directionalLight.color = { 1, 1, 1 };
    light.SetDirectionalLight(directionalLight);


}

void scene_game::initialize()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    // カメラ設定をここに移動
    float screenWidth = Graphics::Instance().GetScreenWidth();
    float screenHeight = Graphics::Instance().GetScreenHeight();

    camera.SetPerspectiveFov(
        DirectX::XMConvertToRadians(45),
        screenWidth / screenHeight,
        camera_near_z,
        camera_far_z
    );
    camera.SetLookAt(
        { -0.02f, 3.5f, 51.5f },
        { -0.02f, 0.0f, 14.0f },
        { 0, 1, 0 }
    );
    cameraController.SyncCameraToController(camera);

    // シーン定数バッファの作成
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = sizeof(scene_constants);
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

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
}

void scene_game::update(float elapsed_time)
{
	elapsed_time *= timeScale;

    // カメラコントローラーの更新
    cameraController.Update();
    cameraController.SyncControllerToCamera(camera);

    // ステージの更新
    stage::Instance().update(elapsed_time);

    // プレイヤーの更新
    Player::Instance().Update(elapsed_time);

	// ピッチャーの更新
    Pitcher::Instance().Update(elapsed_time);

    // 物理システムの更新
    Physics::Instance().Update(elapsed_time);


#ifdef USE_IMGUI
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
        DirectionalLight dirLight = light.GetDirectionalLight();
        if (ImGui::SliderFloat3("Light Direction", &dirLight.direction.x, -1.0f, 1.0f))
        {
            light.SetDirectionalLight(dirLight);
        }
    }

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
    using namespace DirectX;

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();


    // 描画コンテキスト設定
    RenderContext rc;
    rc.context = dc;
    rc.renderState = renderState;
    rc.camera = &camera;
    rc.light = &light;

    

    // プレイヤーの描画
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
    

    

    // ピッチャーの描画
    Pitcher::Instance().Render(rc);

    Player::Instance().Render(rc);

    // ステージの描画
    stage::Instance().render(rc);

    // レンダーステート設定
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
	shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection(), light.GetDirectionalLight().direction);

    // サンプラーステートを設定
    ID3D11SamplerState* samplerStates[] = {
        renderState->GetSamplerState(SamplerState::LinearWrap),
        renderState->GetSamplerState(SamplerState::LinearClamp),
        renderState->GetSamplerState(SamplerState::LinearWrap)
    };
    dc->PSSetSamplers(0, 3, samplerStates);

    // シーン定数バッファの更新
    scene_constants scene_data;
    XMMATRIX V = XMLoadFloat4x4(&camera.GetView());
    XMMATRIX P = XMLoadFloat4x4(&camera.GetProjection());
    XMStoreFloat4x4(&scene_data.view_projection, V * P);

    DirectionalLight dirLight = light.GetDirectionalLight();
    scene_data.light_direction = XMFLOAT4(dirLight.direction.x, dirLight.direction.y, dirLight.direction.z, 0.0f);

    XMFLOAT3 eye = camera.GetEye();
    scene_data.camera_position = XMFLOAT4(eye.x, eye.y, eye.z, 1.0f);

    dc->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &scene_data, 0, 0);
    dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());

	//Physics::Instance().Render(camera.GetView(), camera.GetProjection(), light.GetDirectionalLight().direction);

    //// 2Dスプライトの描画（画面に重ねて表示）
    //if (showStrikeZoneImage && strikeZoneSprite)
    //{
    //    // 深度テストを無効化（2D描画用）
    //    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::WriteOnly), 0);

    //    // テクスチャのサイズを取得
    //    float textureWidth = static_cast<float>(strikeZoneSprite->texture2d_desc.Width);
    //    float textureHeight = static_cast<float>(strikeZoneSprite->texture2d_desc.Height);

    //    strikeZoneSprite->render(
    //        dc,
    //        spritePosition.x,                    // X座標
    //        spritePosition.y,                    // Y座標
    //        textureWidth * spriteScale.x,        // 幅
    //        textureHeight * spriteScale.y,       // 高さ
    //        spriteTint.x, spriteTint.y, spriteTint.z, spriteTint.w,  // 色
    //        0.0f                                 // 回転角度
    //    );

    //    // 深度テストを戻す
    //    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    //}
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