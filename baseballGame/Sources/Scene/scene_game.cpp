#include "scene_game.h"
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
#include "texture.h"
#include "sprite.h"
#include "json.hpp"
#include "Wind.h"
#include "ballSprite.h"
#include "batSprite.h"
#include "GameTimer.h"
#include "HomeRunCount.h"
#include "FoulSprite.h"
#include "catcher.h"
#include "ballDistance.h"
#include "Result.h"
#include "Money.h"
#include <fstream>
#include <string>
#include <random>
#include <ballCount.h>


using json = nlohmann::json;



void scene_game::initialize()
{
    HRESULT hr = S_OK;

    ID3D11Device* device = Graphics::Instance().GetDevice();

    // コンソールログを物理システムに渡す
    Physics::Instance().SetConsoleLog(&consoleLog);
    Pitcher::Instance().SetConsoleLog(&consoleLog);
    Player::Instance().SetConsoleLog(&consoleLog);
    ballSprite::Instance().SetConsoleLog(&consoleLog);

    // カメラ設定をここに移動
    float screenWidth = Graphics::Instance().GetScreenWidth();
    float screenHeight = Graphics::Instance().GetScreenHeight();

    Camera& camera = Camera::Instance();
    camera.SetPerspectiveFov(
        camera.GetFov(),
        screenWidth / screenHeight,
        camera_near_z,
        camera_far_z
    );

    freeCameraController.SyncCameraToController(camera);

    // デフォルトのカメラプリセットを設定
    broadcastCamera.SetupDefaultCameras();

    //定数バッファの作成
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;

        // シーン定数バッファの作成
        buffer_desc.ByteWidth = sizeof(scene_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // ライト定数バッファの作成
        buffer_desc.ByteWidth = sizeof(light_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, light_constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // 半球ライティング定数バッファの作成
        buffer_desc.ByteWidth = sizeof(hemisphere_light_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, hemisphere_light_constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // フォグ定数バッファの作成
        buffer_desc.ByteWidth = sizeof(fog_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, fog_constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //ポストエフェクト用の定数バッファの作成
        buffer_desc.ByteWidth = sizeof(post_effect_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, post_effect_constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //シャドウクオリティの設定
        buffer_desc.ByteWidth = sizeof(shadow_quality_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, shadow_quality_constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }

    // スカイレンダラーの初期化
    {
        skyRenderer.Initialize(device);

        // 初期の昼間設定（正午）
        //skyRenderer.time_of_day = 12.0f;
        skyRenderer.auto_advance_time = false;
        skyRenderer.time_speed = 0.02f;

		//初期時間を昼なら14時、夜なら21時に設定
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 1);

		skyRenderer.time_of_day = (dist(rng) == 0) ? 14.0f : 21.0f;
    }

  
    // ステージの初期化

    Physics::Instance().Initialize();

    stage::Instance().initialize();

    // プレイヤーの初期化
    Player::Instance().Initialize();

    // ピッチャーの初期化
    Pitcher::Instance().Initialize();

    ballSprite::Instance().Initialize(device);

    BatSprite::Instance().Initialize(device);

    //GameTimer::Instance().Initialize(device);

    HomeRunCount::Instance().Initialize(device);

    Catcher::Instance().Initialize();

    // ボール距離の初期化
    BallDistance::Instance().Initialize(device);

	Result::Instance().Initialize(device);

    BallNet::Instance().Initialize();

	Money::Instance().Initialize(device);

    shadowRenderer.Initialize();

    shadowRenderer.GetPointLights().resize(36);
    using namespace DirectX;

    // フィールド中心（大まかな照射ターゲット）
    XMVECTOR fieldCenter = XMVectorSet(0.f, 0.f, 20.f, 0.f);

    struct LightPanel {
        XMFLOAT3 center;
        XMFLOAT3 right;  // パネル横方向（ライトが横に並ぶ方向）
        XMFLOAT3 up;     // パネル縦方向（ライトが縦に並ぶ方向）
        float halfW;
        float halfH;
    };

    // 各塔の中心位置
    XMFLOAT3 towerPositions[6] = {
        {  80.f, 95.f, -80.f },
        { -80.f, 95.f, -80.f },
        {-150.f, 95.f,  40.f },
        { 150.f, 95.f,  40.f },
        {  75.f,  80.f, 135.f },
        { -75.f,  80.f, 135.f },
    };

    LightPanel panels[6];
    for (int i = 0; i < 6; ++i)
    {
        XMVECTOR pos = XMLoadFloat3(&towerPositions[i]);
        // ライト塔 → フィールド中心への方向（水平成分のみ）
        XMVECTOR toField = XMVectorSet(
            fieldCenter.m128_f32[0] - towerPositions[i].x,
            0.f,
            fieldCenter.m128_f32[2] - towerPositions[i].z,
            0.f);
        toField = XMVector3Normalize(toField);

        XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

        // パネル横方向 = フィールド方向 × 上  （ライト塔を正面から見て左右）
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, toField));
        // パネル縦方向 = 常に上向き
        XMVECTOR up = worldUp;

        XMStoreFloat3(&panels[i].center, pos);
        XMStoreFloat3(&panels[i].right, right);
        XMStoreFloat3(&panels[i].up, up);
        panels[i].halfW = 12.f;  // 横方向の半幅
        panels[i].halfH = 4.f;  // 縦方向の半高
    }

    // 格子状に配置（横3×縦2 = 6個/塔 × 6塔 = 36個）
    const int gridX = 3, gridY = 2;
    shadowRenderer.GetPointLights().clear();

    for (auto& panel : panels)
    {
        for (int gy = 0; gy < gridY; ++gy)
        {
            for (int gx = 0; gx < gridX; ++gx)
            {
                // -1.0 〜 +1.0 に正規化してパネル面上に均等配置
                float tx = (gridX > 1) ? (gx / float(gridX - 1) * 2.f - 1.f) : 0.f;
                float ty = (gridY > 1) ? (gy / float(gridY - 1) * 2.f - 1.f) : 0.f;

                XMVECTOR c = XMLoadFloat3(&panel.center);
                XMVECTOR r = XMLoadFloat3(&panel.right);
                XMVECTOR u = XMLoadFloat3(&panel.up);
                XMVECTOR offset = r * (tx * panel.halfW) + u * (ty * panel.halfH);
                XMVECTOR finalPos = c + offset;

                ShadowRenderer::point_lights pl{};
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&pl.position), finalPos);
                pl.position.w = 1.f;
                pl.range = 50.f;
                pl.color = { 1.f, 0.98f, 0.9f, 1.f };
                pl.intensity = 2.0f;
                shadowRenderer.GetPointLights().push_back(pl);
            }
        }
    }

    shadowRenderer.GetSpotLights().resize(6);

    // 位置
    shadowRenderer.GetSpotLights()[0].position = { 80.0f, 95.0f, -80.0f, 1.0f };
    shadowRenderer.GetSpotLights()[1].position = { -80.0f, 95.0f, -80.0f, 1.0f };
    shadowRenderer.GetSpotLights()[2].position = { -150.0f, 95.0f, 40.0f, 1.0f };
    shadowRenderer.GetSpotLights()[3].position = { 150.0f, 95.0f, 40.0f, 1.0f };
    shadowRenderer.GetSpotLights()[4].position = { 75.0f, 80.0f, 135.0f, 1.0f };
    shadowRenderer.GetSpotLights()[5].position = { -75.0f, 80.0f, 135.0f, 1.0f };

    // 狙う場所
    DirectX::XMFLOAT3 targets[6] =
    {
        {  5.0f, 0.0f,  5.0f },
        { -5.0f, 0.0f,  5.0f },
        {  5.0f, 0.0f, 37.5f },
        { -5.0f, 0.0f, 37.5f },
        { 5.0f, 0.0f, 10.0f },
        {-5.0f, 0.0f, 10.0f }
    };

    for (int i = 0; i < 6; ++i)
    {
        DirectX::XMVECTOR pos =
            DirectX::XMLoadFloat4(&shadowRenderer.GetSpotLights()[i].position);

        DirectX::XMVECTOR target =
            DirectX::XMVectorSet(
                targets[i].x,
                targets[i].y,
                targets[i].z,
                0.0f);

        DirectX::XMVECTOR dir =
            DirectX::XMVector3Normalize(target - pos);

        DirectX::XMStoreFloat4(&shadowRenderer.GetSpotLights()[i].direction, dir);

        shadowRenderer.GetSpotLights()[i].color = { 1,1,1,1 };
        shadowRenderer.GetSpotLights()[i].range = 250.0f;
        shadowRenderer.GetSpotLights()[i].intensity = 3.0f;
        shadowRenderer.GetSpotLights()[i].innerCorn = DirectX::XMConvertToRadians(50.0f);
        shadowRenderer.GetSpotLights()[i].outerCorn = DirectX::XMConvertToRadians(55.0f);
    }

    ////シーン描画用のバッファ生成
    Microsoft::WRL::ComPtr<ID3D11Texture2D> color_buffer{};
    D3D11_TEXTURE2D_DESC texture2d_desc{};
    texture2d_desc.Width = static_cast<UINT>(Graphics::Instance().GetScreenWidth());
    texture2d_desc.Height = static_cast<UINT>(Graphics::Instance().GetScreenHeight());
    texture2d_desc.MipLevels = 1;
    texture2d_desc.ArraySize = 1;
    texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture2d_desc.SampleDesc.Count = 1;
    texture2d_desc.SampleDesc.Quality = 0;
    texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
    texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture2d_desc.CPUAccessFlags = 0;
    texture2d_desc.MiscFlags = 0;
    hr = device->CreateTexture2D(&texture2d_desc, NULL, color_buffer.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    //	レンダーターゲットビュー生成
    hr = device->CreateRenderTargetView(color_buffer.Get(), NULL, scene_render_target_view.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    //	シェーダーリソースビュー生成
    hr = device->CreateShaderResourceView(color_buffer.Get(), NULL, scene_shader_resource_view.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


    //	スプライトシェーダー準備
    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", sprite_vertex_shader.ReleaseAndGetAddressOf(), sprite_input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
        create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", sprite_pixel_shader.ReleaseAndGetAddressOf());


    }

    bloomRenderer.Initialize(device, scene_shader_resource_view.Get(), Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight());

    //ドローコール表示用
    D3D11_QUERY_DESC query_desc{};
    query_desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
    query_desc.MiscFlags = 0;
    hr = device->CreateQuery(&query_desc, pipeline_stats_query.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    // 設定のロード
    LoadSetting();
}

void scene_game::update(float elapsed_time)
{
    // Ctrl + S で設定保存
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
    {
        SaveSetting();
    }

    //HOMEキーを押したらIMGUIの表示・非表示を切り替える
	static bool prevHomePressed = false;
	bool homePressed = (GetKeyState(VK_HOME) & 0x8000) != 0;
	if (homePressed && !prevHomePressed)
	{
		showGUI = !showGUI;
	}
	prevHomePressed = homePressed;

    elapsed_time *= timeScale;

    broadcastCamera.Update(elapsed_time, Ball::Instance().GetHasCollidedWithBat());

    float screenWidth = static_cast<float>(Graphics::Instance().GetScreenWidth());
    float screenHeight = static_cast<float>(Graphics::Instance().GetScreenHeight());

    // カメラコントローラーの更新
    Camera& camera = Camera::Instance();
    cameraPosition = camera.GetEye();

    // 追跡終了条件（例：ボールが止まったら）
    if (useFreeCamera)
    {
        freeCameraController.Update(elapsed_time);
        freeCameraController.SyncControllerToCamera(camera);
        camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45.0f), screenWidth / screenHeight, camera_near_z, camera_far_z);
    }
    else
    {
        broadcastCamera.SyncToCamera(camera, screenWidth / screenHeight, camera_near_z, camera_far_z);
        enableShadows = true;
    }
    cameraPosition = camera.GetEye();

    if (broadcastCamera.IsTrackingBall())
    {
        enableShadows = false;
        /*const auto& vel = Ball::Instance().GetVelocity();
        float speed = sqrtf(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
        if (speed < 0.1f)
        {
            trackingTime += elapsed_time;
            if (trackingTime > 1.0f)
            {
                broadcastCamera.StopAllTracking();
                trackingTime = 0.0f;
                enableShadows = true;
            }
        }*/
        if (ImGui::IsKeyPressed(ImGuiKey_LeftShift))
        {
            broadcastCamera.StopAllTracking();
            trackingTime = 0.0f;
            enableShadows = true;
        }
    }

    ballSprite::Instance().Update(elapsed_time);

    BatSprite::Instance().Update(elapsed_time);

    // ステージの更新
    stage::Instance().update(elapsed_time);

    // プレイヤーの更新
    Player::Instance().Update(elapsed_time);

    // ピッチャーの更新
    Pitcher::Instance().Update(elapsed_time);
   
    // 物理システムの更新
    Physics::Instance().Update(elapsed_time);

    //GameTimer::Instance().Update(elapsed_time);

    HomeRunCount::Instance().Update(elapsed_time);

    Catcher::Instance().Update(elapsed_time);

	BallDistance::Instance().Update(elapsed_time);

	Result::Instance().Update(elapsed_time);

	BallNet::Instance().Update(elapsed_time);

	Money::Instance().Update(elapsed_time);

    // スカイレンダラーの更新
    skyRenderer.Update(elapsed_time * timeScale);


    //太陽方向をライト方向と同期
    directional_light_direction = skyRenderer.GetSunDirectionToLight();

    //時刻が6時から16時の時はポイントライトとスポットライトを消す
    if (skyRenderer.time_of_day >= 6.0f && skyRenderer.time_of_day <= 17.0f)
    {
        directional_light_intensity = 2.0f;
        ambient_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        for (auto& pl : shadowRenderer.GetPointLights())
        {
            pl.intensity = 0.0f;
        }
        for (auto& sl : shadowRenderer.GetSpotLights())
        {
            sl.intensity = 0.0f;
        }
    }
    else
    {
        directional_light_intensity = 0.0f;
        ambient_color = { 0.7f, 0.7f, 0.7f, 1.0f };
        for (auto& pl : shadowRenderer.GetPointLights())
        {
            pl.intensity = 30.0f;
        }
        for (auto& sl : shadowRenderer.GetSpotLights())
        {
            sl.intensity = 5.0f;
        }
    }

    shadowRenderer.SetDirectionalLight(
        directional_light_direction,
        directional_light_color,
        directional_light_intensity
    );
}



void scene_game::render(float elapsedTime)
{


    using namespace DirectX;

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    RenderContext rc;
    rc.deviceContext = dc;
    rc.renderState = renderState;

    Camera& camera = Camera::Instance();

    if (enableFrustumCulling)
    {
        frustumCulling.Construct(camera.GetView(), camera.GetProjection());
    }

    // 前フレームの結果を取得（ノンブロッキング）
    /*dc->GetData(pipeline_stats_query.Get(), &pipeline_stats,
        sizeof(pipeline_stats), D3D11_ASYNC_GETDATA_DONOTFLUSH);*/

        // 今フレームの計測開始
        //dc->Begin(pipeline_stats_query.Get());

        // shadowRenderer にカメラ位置を渡す
    shadowRenderer.SetCameraPosition(cameraPosition);

	
    // 3本の描画呼び出し（関数名だけ変わる）
    if (enableShadows)
    {
        if (shadowRenderer.use_cascade_shadow_map)
            shadowRenderer.RenderCascadeShadowMap(elapsedTime);
        else
            shadowRenderer.RenderShadowMap(elapsedTime);

        shadowRenderer.spot_shadow_frame_count++;
        if (shadowRenderer.spot_shadow_frame_count >= shadowRenderer.spot_shadow_update_interval)
        {
            shadowRenderer.RenderSpotShadowMap(elapsedTime);
            shadowRenderer.spot_shadow_frame_count = 0;

        }
    }

    //ポイントライトの描画
    for (int i = 0; i < shadowRenderer.GetPointLights().size(); ++i)
    {
        //大きさは変わらない
        shapeRenderer->DrawPointLight(DirectX::XMFLOAT3(shadowRenderer.GetPointLights()[i].position.x, shadowRenderer.GetPointLights()[i].position.y, shadowRenderer.GetPointLights()[i].position.z), 0.1, shadowRenderer.GetPointLights()[i].color);
    }

    //スポットライトの描画

    for (int i = 0; i < shadowRenderer.GetSpotLights().size(); ++i)
    {
        shapeRenderer->DrawSpotLight(
            DirectX::XMFLOAT3(shadowRenderer.GetSpotLights()[i].position.x, shadowRenderer.GetSpotLights()[i].position.y, shadowRenderer.GetSpotLights()[i].position.z),
            DirectX::XMFLOAT3(shadowRenderer.GetSpotLights()[i].direction.x, shadowRenderer.GetSpotLights()[i].direction.y, shadowRenderer.GetSpotLights()[i].direction.z),
            5.0f,
            shadowRenderer.GetSpotLights()[i].innerCorn,
            shadowRenderer.GetSpotLights()[i].outerCorn,
            shadowRenderer.GetSpotLights()[i].color
        );
    }

    // バックバッファに直接描画

    float clear_color[4] = { 0.2f, 0.4f, 0.6f, 1.0f };
    dc->ClearRenderTargetView(scene_render_target_view.Get(), clear_color);
    dc->ClearDepthStencilView(Graphics::Instance().GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    dc->OMSetRenderTargets(1, scene_render_target_view.GetAddressOf(), Graphics::Instance().GetDepthStencilView());

    // ビューポートの設定
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(Graphics::Instance().GetScreenWidth());
    viewport.Height = static_cast<float>(Graphics::Instance().GetScreenHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    dc->RSSetViewports(1, &viewport);

    //ブレンドステートの設定
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    //深度ステンシルステートの設定
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    //ラスタライザーステートの設定
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

    // 定数バッファの更新

    XMMATRIX V = XMLoadFloat4x4(&camera.GetView());
    XMMATRIX P = XMLoadFloat4x4(&camera.GetProjection());

    scene_constants scene{};
    scene.camera_position.x = cameraPosition.x;
    scene.camera_position.y = cameraPosition.y;
    scene.camera_position.z = cameraPosition.z;
    DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
    XMFLOAT3 right = camera.GetRight();
    XMFLOAT3 up = camera.GetUp();
    scene.camera_right = { right.x, right.y, right.z, 0.0f };
    scene.camera_up = { up.x, up.y, up.z, 0.0f };
    dc->UpdateSubresource(constant_buffer.Get(), 0, 0, &scene, 0, 0);
    dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());

    static constexpr int LightCBVIndex = 3;
    light_constants lightConstants{};
    lightConstants.ambient_color = ambient_color;
    lightConstants.directional_light_direction = directional_light_direction;
    lightConstants.directional_light_color = directional_light_color;
    lightConstants.directional_light_intensity = directional_light_intensity;

    for (auto& point_light : shadowRenderer.GetPointLights())
    {
        lightConstants.point_light[lightConstants.light_count.y] = point_light;
        if (++lightConstants.light_count.y == light_constants::light_max)
            break;
    }
    for (auto& spot_light : shadowRenderer.GetSpotLights())
    {
        lightConstants.spot_light[lightConstants.light_count.z] = spot_light;
        lightConstants.spot_light[lightConstants.light_count.z].innerCorn = cosf(spot_light.innerCorn);
        lightConstants.spot_light[lightConstants.light_count.z].outerCorn = cosf(spot_light.outerCorn);
        if (++lightConstants.light_count.z == light_constants::light_max)
            break;
    }

    dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lightConstants, 0, 0);
    dc->VSSetConstantBuffers(LightCBVIndex, 1, light_constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(LightCBVIndex, 1, light_constant_buffer.GetAddressOf());

    hemisphere_light_constants hemisphereLightConstants{};
    hemisphereLightConstants.sky_color = sky_color;
    hemisphereLightConstants.ground_color = ground_color;
    hemisphereLightConstants.hemisphere_weight.x = hemisphere_weight;
    dc->UpdateSubresource(hemisphere_light_constant_buffer.Get(), 0, 0, &hemisphereLightConstants, 0, 0);
    dc->VSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());

    fog_constants fogConstants{};
    fogConstants.fog_color = fog_color;
    fogConstants.fog_range = fog_range;
    dc->UpdateSubresource(fog_constant_buffer.Get(), 0, 0, &fogConstants, 0, 0);
    dc->VSSetConstantBuffers(5, 1, fog_constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(5, 1, fog_constant_buffer.GetAddressOf());

    // ポストエフェクト用定数バッファの更新
    dc->UpdateSubresource(post_effect_constant_buffer.Get(), 0, 0, &post_effect_constant, 0, 0);
    dc->VSSetConstantBuffers(10, 1, post_effect_constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(10, 1, post_effect_constant_buffer.GetAddressOf());

    dc->UpdateSubresource(shadow_quality_constant_buffer.Get(), 0, 0,
        &shadow_quality_constant, 0, 0);
    dc->PSSetConstantBuffers(11, 1, shadow_quality_constant_buffer.GetAddressOf());

    shadowRenderer.BindShadowResources(dc);

    // サンプラーステート
    ID3D11SamplerState* sampler_states[] =
    {
        Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::PointClamp),        // s0: POINT
        Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::LinearClamp),  // s1: LINEAR
        Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::AnisotropicClamp),  // s2: ANISOTROPIC
    };
    dc->PSSetSamplers(0, ARRAYSIZE(sampler_states), sampler_states);
    dc->VSSetSamplers(0, ARRAYSIZE(sampler_states), sampler_states);

    skyRenderer.Render(
        dc,
        constant_buffer.Get(),
        renderState->GetDepthStencilState(DepthState::TestOnly),   //深度テストのみ・書き込みなし
        renderState->GetRasterizerState(RasterizerState::SolidCullNone)
    );

    // 深度・ラスタライザーを元に戻す
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

    // ステージの描画
    //フラスタムカリングを有効にする場合は frustumCulling を渡す、無効にする場合は nullptr を渡す
    stage::Instance().render(rc, modelRenderer, enableFrustumCulling ? &frustumCulling : nullptr);

    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    // ピッチャー・バッター(プレイヤー本体)・キャッチャーは、ステージと同じ
    // 通常の ambient / 半球ライト（lightConstants・hemisphereLightConstants、
    // どちらもこの時点でスロット3・4にバインド済み）の影響を受けたまま描画する。
    // バットだけは別途この下のブロックで専用のライティングに切り替える。
	BallNet::Instance().Render(rc, modelRenderer);
    Pitcher::Instance().Render(rc, modelRenderer);
    Player::Instance().RenderPlayer(rc, modelRenderer);

    //アクティブカメラが通常カメラか確信ホームランカメラ4の時は描画しない
    if (broadcastCamera.GetActiveCameraType() != CameraType::NormalCamera && broadcastCamera.GetActiveCameraName() != "確信ホームランカメラ4")
    {
        Catcher::Instance().Render(rc, modelRenderer, enableFrustumCulling ? &frustumCulling : nullptr);
    }
    
    // バットだけ ambient を 0 にして描画
    {
        light_constants noAmbientLight = lightConstants;  //  lightConstantsをメンバ変数に昇格する必要あり
        //バットも時刻が6時以上16時以下の時に強くする
        if (skyRenderer.time_of_day >= 6.0f && skyRenderer.time_of_day <= 17.0f)
        {
            noAmbientLight.ambient_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        }
        else
            noAmbientLight.ambient_color = { 0.7f, 0.7f, 0.7f, 1.0f };
        dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &noAmbientLight, 0, 0);
        dc->PSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());

        hemisphere_light_constants noHemi{};
        noHemi.sky_color = { 0, 0, 0, 0 };
        noHemi.ground_color = { 0, 0, 0, 0 };
        noHemi.hemisphere_weight.x = 0.0f;
        dc->UpdateSubresource(hemisphere_light_constant_buffer.Get(), 0, 0, &noHemi, 0, 0);
        dc->PSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());

        Player::Instance().RenderBat(rc, modelRenderer);

        // 元に戻す
        dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lightConstants, 0, 0);
        dc->PSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());

        hemisphere_light_constants hemi{};
        hemi.sky_color = sky_color;
        hemi.ground_color = ground_color;
        hemi.hemisphere_weight.x = hemisphere_weight;
        dc->UpdateSubresource(hemisphere_light_constant_buffer.Get(), 0, 0, &hemi, 0, 0);
        dc->PSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());
    }

    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

    if(!GameTimer::Instance().IsFinished())BatSprite::Instance().Render();
    ballSprite::Instance().Render();

    //GameTimer::Instance().Render();

    HomeRunCount::Instance().Render();

    FoulSprite::Instance().Render();

	BallDistance::Instance().Render();

	Money::Instance().Render();

    // ShapeRenderer の描画実行

    if (showPhysxDebug)

    {
        shapeRenderer->Render(
            dc,
            camera.GetView(),
            camera.GetProjection(),
            rc.lightDirection
        );

        Physics::Instance().SetRenderSimpleShapesOnly(physxRenderSimpleShapesOnly);
        Physics::Instance().SetSkipSleepingActors(physxSkipSleepingActors);
        Physics::Instance().Render(camera.GetView(), camera.GetProjection(), rc.lightDirection);
    }

    // ここで高輝度抽出とぼかしを実行してパスのSRVを更新する
	bloomRenderer.Extract(dc, camera.GetView(), camera.GetProjection(), cameraPosition);

    // ... 描画後に ...
    shadowRenderer.UnbindShadowResources(dc);

    // バックバッファに戻してコピー
    ID3D11RenderTargetView* backBufferRTV = Graphics::Instance().GetRenderTargetView();
    dc->OMSetRenderTargets(1, &backBufferRTV, nullptr);

    ID3D11Resource* srcRes = nullptr;
    ID3D11Resource* dstRes = nullptr;
    scene_render_target_view->GetResource(&srcRes);
    backBufferRTV->GetResource(&dstRes);
    dc->CopyResource(dstRes, srcRes);
    srcRes->Release();
    dstRes->Release();

    //	ぼかした結果を加算合成
    bloomRenderer.Composite(dc);

    //bloom合成済みの最終画面をscene_render_target_viewへコピー
    {
        ID3D11Resource* srcRes = nullptr;
        ID3D11Resource* dstRes = nullptr;
        backBufferRTV->GetResource(&srcRes);          // バックバッファ（Bloom済み）
        scene_render_target_view->GetResource(&dstRes); // sceneテクスチャへ書き戻し
        dc->CopyResource(dstRes, srcRes);
        srcRes->Release();
        dstRes->Release();
    }

    if (ballCount::Instance().GetRemainingBalls() <= 0 && Pitcher::Instance().GetCurrentState() == Pitcher::State::SelectingPitch)
    {
        //リザルトを表示する
        Result::Instance().Render();
    }

    //計測終了
    //dc->End(pipeline_stats_query.Get());
}

void scene_game::uninitialize()
{
    // オブジェクト側の終了
    Player::Instance().Uninitialize();
    stage::Instance().uninitialize();
    Pitcher::Instance().Uninitialize();
    BallDistance::Instance().Uninitialize();
    Result::Instance().Uninitialize();
	BallNet::Instance().Uninitialize();
	Money::Instance().Uninitialize();

    skyRenderer.Uninitialize();
	shadowRenderer.Uninitialize();
	bloomRenderer.Uninitialize();

    // 最後に物理システムなどを終了
    Physics::Instance().Finalize();

    // GPU コマンドをフラッシュしてからリソース破棄
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    if (dc) dc->Flush();

    // scene_game が持つ GPU リソースを明示的にリセットして参照カウントを下げる
    constant_buffer.Reset();
    light_constant_buffer.Reset();
    hemisphere_light_constant_buffer.Reset();
    fog_constant_buffer.Reset();
    post_effect_constant_buffer.Reset();
    scene_render_target_view.Reset();
    scene_shader_resource_view.Reset();
    sprite_vertex_shader.Reset();
    sprite_input_layout.Reset();
    sprite_pixel_shader.Reset();
    pipeline_stats_query.Reset();
    
   
}

void scene_game::DrawGUI()
{

	

#ifdef _DEBUG

#ifdef USE_IMGUI
    if(showGUI)
    {
        ImGuiIO& io = ImGui::GetIO();
        const float W = io.DisplaySize.x;
        const float H = io.DisplaySize.y;

        // ── パネル幅・高さ定数 ──────────────────────────────
        const float LEFT_W = 320.0f;   // 左パネル（Player / Pitcher）
        const float RIGHT_W = 320.0f;   // 右パネル（Debug）
        const float BOTTOM_H = 250.0f;   // 下パネル（Console）

        const float PANEL_ALPHA = 0.9f;

        //移動とリサイズを許可するウィンドウフラグ
        const ImGuiWindowFlags FLOAT_FLAGS =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize;


        // ════════════════════════════════════════════════════
        //  左パネル ── Player / Pitcher
        // ════════════════════════════════════════════════════
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(LEFT_W, H * 0.8f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(PANEL_ALPHA);
        ImGui::Begin("## Left", nullptr, FLOAT_FLAGS);

        if (ImGui::CollapsingHeader("Player")) { Player::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Pitcher")) { Pitcher::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Stage")) { stage::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Sky & Time")) { skyRenderer.DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Ball Sprite")) { ballSprite::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Bat Sprite")) { BatSprite::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Timer")) { GameTimer::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Home Run Count")) { HomeRunCount::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Catcher")) { Catcher::Instance().DrawGUI(); }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Ball Distance")) { BallDistance::Instance().DrawGUI(); }
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Result")) { Result::Instance().DrawGUI(); }
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Ball Net")) { BallNet::Instance().DrawGUI(); }
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Money")) { Money::Instance().DrawGUI(); }

        ImGui::End();

        // ════════════════════════════════════════════════════
        //  中央上 ── Game View
        // ════════════════════════════════════════════════════

        {

            // ── フェンスライン編集ツールの更新 ──
            Camera& camera = Camera::Instance();
            DirectX::XMFLOAT4X4 view = camera.GetView();          // ※名称が違う場合は合わせてください
            DirectX::XMFLOAT4X4 proj = camera.GetProjection();    // ※同上

            stage::Instance().UpdateLineEditor(
                stage::Instance().homerunLineEditor,
                view, proj,
                0.0f, 0.0f,
                W, H);
            stage::Instance().UpdateLineEditor(
                stage::Instance().foulLineEditor,
                view, proj,
                0.0f, 0.0f,
                W, H);

            // ── 打った点をその場でつないで見せる(Rebuildする前のプレビュー) ──
            // ホームランフェンス：赤系
            //stage::Instance().DrawLineOverlay(
            //    stage::Instance().homerunLineEditor,
            //    view, proj,
            //    0.0f, 0.0f,
            //    W, H, IM_COL32(255, 60, 60, 255), IM_COL32(255, 255, 0, 255)); // 赤線・黄点

            //// ファウルライン：青系
            //stage::Instance().DrawLineOverlay(
            //    stage::Instance().foulLineEditor,
            //    view, proj,
            //    0.0f, 0.0f,
            //    W, H, IM_COL32(60, 60, 255, 255), IM_COL32(0, 255, 255, 255)); // 青線・水色点
        }

        // ════════════════════════════════════════════════════
        //  右パネル ── Debug
        // ════════════════════════════════════════════════════
        ImGui::SetNextWindowPos(ImVec2(W - RIGHT_W, 0.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(RIGHT_W, H * 0.8f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(PANEL_ALPHA);
        ImGui::Begin("Debug", nullptr, FLOAT_FLAGS);
        // ── Camera ──
        if (ImGui::CollapsingHeader("Camera"))
        {
            Camera& camera = Camera::Instance();
            DirectX::XMFLOAT3 eye = camera.GetEye();
            DirectX::XMFLOAT3 focus = camera.GetFocus();

            if (ImGui::DragFloat3("Eye", &eye.x, 0.1f))
            {
                camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
                freeCameraController.SyncCameraToController(camera);
            }
            if (ImGui::DragFloat3("Focus", &focus.x, 0.1f))
            {
                camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
                freeCameraController.SyncCameraToController(camera);
            }
            ImGui::SliderFloat("Near Z", &camera_near_z, 0.1f, 100.0f);
            ImGui::SliderFloat("Far Z", &camera_far_z, 100.0f, 1000.0f);
            camera.SetPerspectiveFov(
                camera.GetFov(),
                Graphics::Instance().GetScreenWidth() / Graphics::Instance().GetScreenHeight(),
                camera_near_z, camera_far_z);

            ImGui::Checkbox(u8"フリーカメラ", &useFreeCamera);

            static bool prevFreeCamera = false;
            if (useFreeCamera && !prevFreeCamera)
            {
                freeCameraController.SyncCameraToController(camera);
            }
            prevFreeCamera = useFreeCamera;

            broadcastCamera.DrawGUI();


        }

        // ── Time Scale ──
        if (ImGui::CollapsingHeader("Time Control"))
        {
            ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 2.0f);
            if (ImGui::Button(u8"一時停止"))  timeScale = 0.0f;
            ImGui::SameLine();
            if (ImGui::Button(u8"通常速度"))  timeScale = 1.0f;
        }

        // ── Light ──
        if (ImGui::CollapsingHeader("Light"))
        {
            ImGui::ColorEdit4("Ambient", &ambient_color.x);
            ImGui::SliderFloat3("Dir Light Dir", &directional_light_direction.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Dir Light Color", &directional_light_color.x);
            ImGui::SliderFloat("Dir Intensity", &directional_light_intensity, 0.0f, 100.0f);

            if (ImGui::TreeNode("Point Lights"))
            {
                for (int i = 0; i < (int)shadowRenderer.GetPointLights().size(); ++i)
                {
                    if (ImGui::TreeNode((std::string("point ") + std::to_string(i)).c_str()))
                    {
                        ImGui::SliderFloat3("pos", &shadowRenderer.GetPointLights()[i].position.x, -200.0f, 200.0f);
                        ImGui::ColorEdit3("color", &shadowRenderer.GetPointLights()[i].color.x);
                        ImGui::SliderFloat("intensity", &shadowRenderer.GetPointLights()[i].intensity, 0.0f, 100.0f);
                        ImGui::SliderFloat("range", &shadowRenderer.GetPointLights()[i].range, 0.1f, 200.0f);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Spot Lights"))
            {
                for (int i = 0; i < (int)shadowRenderer.GetSpotLights().size(); ++i)
                {
                    if (ImGui::TreeNode((std::string("spot ") + std::to_string(i)).c_str()))
                    {
                        ImGui::SliderFloat3("pos", &shadowRenderer.GetSpotLights()[i].position.x, -200.0f, 200.0f);
                        if (ImGui::SliderFloat3("dir", &shadowRenderer.GetSpotLights()[i].direction.x, -1.0f, 1.0f))
                        {
                            float len = sqrtf(
                                shadowRenderer.GetSpotLights()[i].direction.x * shadowRenderer.GetSpotLights()[i].direction.x +
                                shadowRenderer.GetSpotLights()[i].direction.y * shadowRenderer.GetSpotLights()[i].direction.y +
                                shadowRenderer.GetSpotLights()[i].direction.z * shadowRenderer.GetSpotLights()[i].direction.z);
                            if (len > 0) {
                                shadowRenderer.GetSpotLights()[i].direction.x /= len;
                                shadowRenderer.GetSpotLights()[i].direction.y /= len;
                                shadowRenderer.GetSpotLights()[i].direction.z /= len;
                            }
                        }
                        ImGui::ColorEdit3("color", &shadowRenderer.GetSpotLights()[i].color.x);
                        ImGui::SliderFloat("intensity", &shadowRenderer.GetSpotLights()[i].intensity, 0.0f, 100.0f);
                        ImGui::SliderFloat("range", &shadowRenderer.GetSpotLights()[i].range, 0.1f, 1000.0f);
                        float inner_deg = DirectX::XMConvertToDegrees(shadowRenderer.GetSpotLights()[i].innerCorn);
                        float outer_deg = DirectX::XMConvertToDegrees(shadowRenderer.GetSpotLights()[i].outerCorn);
                        if (ImGui::SliderFloat("inner", &inner_deg, 0.0f, 89.0f))
                            shadowRenderer.GetSpotLights()[i].innerCorn = DirectX::XMConvertToRadians(inner_deg);
                        if (ImGui::SliderFloat("outer", &outer_deg, 0.0f, 89.0f))
                            shadowRenderer.GetSpotLights()[i].outerCorn = DirectX::XMConvertToRadians(outer_deg);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }

        // ── Hemisphere Light & Fog ──
        if (ImGui::CollapsingHeader("Hemisphere / Fog"))
        {
            ImGui::ColorEdit3("Sky Color", &sky_color.x);
            ImGui::ColorEdit3("Ground Color", &ground_color.x);
            ImGui::SliderFloat("Hemi Weight", &hemisphere_weight, 0.0f, 1.0f);
            ImGui::Separator();
            ImGui::ColorEdit3("Fog Color", &fog_color.x);
            ImGui::SliderFloat("Fog Near", &fog_range.x, 0.1f, 100.0f);
            ImGui::SliderFloat("Fog Far", &fog_range.y, 0.1f, 100.0f);
        }

        // ── Shadow ──
        if (ImGui::CollapsingHeader("Shadow"))
        {
            ImGui::Checkbox("Cascade Shadow", &shadowRenderer.use_cascade_shadow_map);
            if (shadowRenderer.use_cascade_shadow_map)
            {
                ImGui::Checkbox("Show Cascade Area", &shadowRenderer.cascade_shadow_constant.display_cascade_area);
                ImGui::SliderFloat("Attenuation", &shadowRenderer.cascade_shadow_constant.shadow_attenuation, 0.0f, 1.0f);
                ImGui::SliderFloat4("Bias", &shadowRenderer.cascade_shadow_constant.shadow_bias.x, 0.0f, 0.01f);
                for (int i = 0; i < ShadowBufferSize; ++i)
                {
                    ImGui::Text("Cascade Map %d", i);
                    ImGui::Image(ImTextureRef(shadowRenderer.GetCascadeShadowmapSRV(i)), ImVec2(200, 200));
                }
            }
            else
            {
                ImGui::SliderFloat("Attenuation", &shadowRenderer.shadow_attenuation, 0.0f, 1.0f);
                ImGui::SliderFloat("Bias", &shadowRenderer.shadow_bias, 0.0f, 0.01f);
                ImGui::Text("Scene RT");
                ImGui::Image(ImTextureRef(scene_shader_resource_view.Get()), ImVec2(200, 112));
                ImGui::Text("Shadow Map");
                ImGui::Image(ImTextureRef(shadowRenderer.GetShadowmapSRV()), ImVec2(200, 200));
            }

            ImGui::Separator();
            ImGui::Text("--- Soft Shadow ---");

            bool soft_on = (shadow_quality_constant.soft_shadow_enabled != 0);
            if (ImGui::Checkbox("Soft Shadow (PCF)", &soft_on))
                shadow_quality_constant.soft_shadow_enabled = soft_on ? 1 : 0;

            if (soft_on)
            {
                static const char* sample_items[] = { "4", "9", "16", "25" };
                static const int   sample_vals[] = { 4,   9,  16,   25 };
                static int sample_idx = 1; // デフォルト 9
                if (ImGui::Combo("PCF Samples", &sample_idx, sample_items, 4))
                    shadow_quality_constant.soft_shadow_samples = sample_vals[sample_idx];
                ImGui::SliderFloat("PCF Radius", &shadow_quality_constant.soft_shadow_radius,
                    0.5f, 5.0f);
            }

        }

        // ── Bloom ──
        if (ImGui::CollapsingHeader("Bloom"))
        {
			bloomRenderer.DrawGUI();
			
        }

        // ── Tone Mapping ──
        if (ImGui::CollapsingHeader("Tone Mapping"))
        {
            static const char* tone_mode_names[] = {
                "None (Pass-through)",
                "Reinhard",
                "Reinhard Extended",
                "Uncharted2 / Filmic",
                "ACES",
                "Lottes",
            };
            ImGui::Combo("Mode", &post_effect_constant.tone_mapping_mode,
                tone_mode_names, IM_ARRAYSIZE(tone_mode_names));
            ImGui::SliderFloat("Exposure", &post_effect_constant.tone_mapping_exposure, 0.1f, 10.0f);
            if (post_effect_constant.tone_mapping_mode == 2)
                ImGui::SliderFloat("White Point", &post_effect_constant.tone_mapping_white_point, 1.0f, 20.0f);
        }

        // ── Toon Shading ──
        if (ImGui::CollapsingHeader("Toon Shading"))
        {
            bool toon_enabled = (post_effect_constant.toon_shading_enabled != 0);
            if (ImGui::Checkbox("Enable Toon Shading", &toon_enabled))
                post_effect_constant.toon_shading_enabled = toon_enabled ? 1 : 0;

            if (toon_enabled)
            {
                ImGui::SliderInt("Diffuse Steps", &post_effect_constant.toon_diffuse_steps, 2, 8);
                ImGui::SliderFloat("Specular Threshold", &post_effect_constant.toon_specular_threshold, 0.0f, 1.0f);
                ImGui::SliderFloat("Specular Smoothness", &post_effect_constant.toon_specular_smoothness, 0.0f, 0.2f);
                ImGui::Separator();
                ImGui::SliderFloat("Rim Threshold", &post_effect_constant.toon_rim_threshold, 0.0f, 1.0f);
                ImGui::SliderFloat("Rim Smoothness", &post_effect_constant.toon_rim_smoothness, 0.0f, 0.2f);
                ImGui::ColorEdit3("Rim Color", reinterpret_cast<float*>(&post_effect_constant.toon_rim_color));
                ImGui::SliderFloat("Rim Intensity", &post_effect_constant.toon_rim_color.w, 0.0f, 2.0f);
            }
        }

        // ── PhysX ──
        if (ImGui::CollapsingHeader("Physics"))
        {
            ImGui::Checkbox("Show PhysX Debug", &showPhysxDebug);
            ImGui::Checkbox("Render Simple Shapes Only (High Performance)", &physxRenderSimpleShapesOnly);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Skip ConvexMesh/TriangleMesh rendering for better FPS");
            ImGui::Checkbox("Skip Sleeping Actors", &physxSkipSleepingActors);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Skip rendering of sleeping physics actors");
        }

        // ── Culling ──
        if (ImGui::CollapsingHeader("Culling"))
        {
            ImGui::Checkbox("Enable Frustum Culling", &enableFrustumCulling);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Cull objects outside the camera's view frustum");
        }

        // ── Performance Options ──
        if (ImGui::CollapsingHeader("Performance"))
        {
            ImGui::Checkbox("Enable Shadows", &enableShadows);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Disable for better FPS");
            bloomRenderer.DrawEnableCheckbox();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Disable for better FPS");
        }


        ImGui::End();

        // ════════════════════════════════════════════════════
        //  下パネル ── Console
        // ════════════════════════════════════════════════════
        ImGui::SetNextWindowPos(ImVec2(LEFT_W, H - BOTTOM_H), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(W - LEFT_W - RIGHT_W, BOTTOM_H), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(PANEL_ALPHA);
        ImGui::Begin("Console", nullptr, FLOAT_FLAGS);
        {
            // ── ログ表示エリア ──────────────────────────────
            // consoleLog は scene_game のメンバーとして追加推奨:
            //   std::vector<std::string> consoleLog;
            // ログ追加はゲームコード中で:
            //   consoleLog.push_back("[Info] ...");
            //
            // ここでは現状のシンプル表示にとどめる
            ImGui::BeginChild("##log", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);

            // サンプルログ（実際は consoleLog を iterate する）
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[Info]  Scene running...");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "[Info]  PhysX : %s",
                showPhysxDebug ? "Visible" : "Hidden");
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "[Info]  TimeScale : %.2f", timeScale);

            for (const auto& line : consoleLog)
            {
                if (line.find("[Hit]") != std::string::npos)
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", line.c_str());
                else if (line.find("[Warn]") != std::string::npos)
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", line.c_str());
                else
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", line.c_str());
            }

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);   // 常に末尾へ自動スクロール

            ImGui::EndChild();

            // ── 入力フィールド（コマンド入力用・将来拡張） ──
            static char inputBuf[256] = {};
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##cmd", inputBuf, sizeof(inputBuf),
                ImGuiInputTextFlags_EnterReturnsTrue))
            {
                // consoleLog.push_back(std::string("> ") + inputBuf);
                //LogResetと書いたらログをクリアする例
                if (std::string(inputBuf) == "LogReset")
                {
                    consoleLog.clear();
                    consoleLog.push_back("[Info] Console log cleared.");
                }

                //不要なメモリを削除するコマンド
                if (std::string(inputBuf) == "ClearMemory")
                {
                    // メモリ解放処理をここに追加
                    consoleLog.clear();
                    consoleLog.push_back("[Info] Unused memory cleared.");
                }


                inputBuf[0] = '\0';
                ImGui::SetKeyboardFocusHere(-1);
            }
        }
        ImGui::End();
    }

#endif // USE_IMGUI

#endif // _DEBUG
}

void scene_game::SaveSetting()
{
    json j;

    //// カメラ設定の保存
    Camera& camera = Camera::Instance();
    DirectX::XMFLOAT3 eye = camera.GetEye();
    DirectX::XMFLOAT3 focus = camera.GetFocus();
    j["camera"]["eye"] = { eye.x, eye.y, eye.z };
    j["camera"]["focus"] = { focus.x, focus.y, focus.z };
    j["camera"]["near_z"] = camera_near_z;
    j["camera"]["far_z"] = camera_far_z;

    broadcastCamera.SaveToJson(j["broadcast_camera"]);

    //タイムコントロールの保存
    j["time"]["time_scale"] = timeScale;

    // ライト設定の保存
    j["light"]["ambient"] = { ambient_color.x, ambient_color.y, ambient_color.z, ambient_color.w };
    j["light"]["dir_dir"] = { directional_light_direction.x, directional_light_direction.y, directional_light_direction.z };
    j["light"]["dir_color"] = { directional_light_color.x, directional_light_color.y, directional_light_color.z };
    j["light"]["dir_intensity"] = directional_light_intensity;

    //ポイントライトの保存
    for (int i = 0; i < (int)shadowRenderer.GetPointLights().size(); ++i)
    {
        j["point_lights"][i]["pos"] = { shadowRenderer.GetPointLights()[i].position.x, shadowRenderer.GetPointLights()[i].position.y, shadowRenderer.GetPointLights()[i].position.z };
        j["point_lights"][i]["color"] = { shadowRenderer.GetPointLights()[i].color.x, shadowRenderer.GetPointLights()[i].color.y, shadowRenderer.GetPointLights()[i].color.z };
        j["point_lights"][i]["intensity"] = shadowRenderer.GetPointLights()[i].intensity;
        j["point_lights"][i]["range"] = shadowRenderer.GetPointLights()[i].range;
    }

    //スポットライトの保存
    for (int i = 0; i < (int)shadowRenderer.GetSpotLights().size(); ++i)
    {
        j["spot_lights"][i]["pos"] = { shadowRenderer.GetSpotLights()[i].position.x, shadowRenderer.GetSpotLights()[i].position.y, shadowRenderer.GetSpotLights()[i].position.z };
        j["spot_lights"][i]["dir"] = { shadowRenderer.GetSpotLights()[i].direction.x, shadowRenderer.GetSpotLights()[i].direction.y, shadowRenderer.GetSpotLights()[i].direction.z };
        j["spot_lights"][i]["color"] = { shadowRenderer.GetSpotLights()[i].color.x, shadowRenderer.GetSpotLights()[i].color.y, shadowRenderer.GetSpotLights()[i].color.z };
        j["spot_lights"][i]["intensity"] = shadowRenderer.GetSpotLights()[i].intensity;
        j["spot_lights"][i]["range"] = shadowRenderer.GetSpotLights()[i].range;
        j["spot_lights"][i]["innerCorn"] = shadowRenderer.GetSpotLights()[i].innerCorn;
        j["spot_lights"][i]["outerCorn"] = shadowRenderer.GetSpotLights()[i].outerCorn;
    }

    //ヘミスフィアライトとフォグの保存
    j["hemisphere"]["sky_color"] = { sky_color.x, sky_color.y, sky_color.z };
    j["hemisphere"]["ground_color"] = { ground_color.x, ground_color.y, ground_color.z };
    j["hemisphere"]["weight"] = hemisphere_weight;
    j["fog"]["color"] = { fog_color.x, fog_color.y, fog_color.z };
    j["fog"]["near"] = fog_range.x;
    j["fog"]["far"] = fog_range.y;

    //シャドウの保存
    j["shadow"]["use_cascade"] = shadowRenderer.use_cascade_shadow_map;
    j["shadow"]["cascade_attenuation"] = shadowRenderer.cascade_shadow_constant.shadow_attenuation;
    j["shadow"]["cascade_bias"] = { shadowRenderer.cascade_shadow_constant.shadow_bias.x, shadowRenderer.cascade_shadow_constant.shadow_bias.y, shadowRenderer.cascade_shadow_constant.shadow_bias.z, shadowRenderer.cascade_shadow_constant.shadow_bias.w };
    j["shadow"]["bias"] = shadowRenderer.shadow_bias;

    j["shadow"]["soft_enabled"] = shadow_quality_constant.soft_shadow_enabled;
    j["shadow"]["soft_samples"] = shadow_quality_constant.soft_shadow_samples;
    j["shadow"]["soft_radius"] = shadow_quality_constant.soft_shadow_radius;

    // ブルームの保存
	bloomRenderer.SaveToJson(j["bloom"]);

    // トーンマッピング設定の保存
    j["tone_mapping"]["mode"] = post_effect_constant.tone_mapping_mode;
    j["tone_mapping"]["exposure"] = post_effect_constant.tone_mapping_exposure;
    j["tone_mapping"]["white_point"] = post_effect_constant.tone_mapping_white_point;

    // トゥーンシェーディング設定の保存
    j["toon"]["enabled"] = (post_effect_constant.toon_shading_enabled != 0);
    j["toon"]["diffuse_steps"] = post_effect_constant.toon_diffuse_steps;
    j["toon"]["spec_threshold"] = post_effect_constant.toon_specular_threshold;
    j["toon"]["spec_smoothness"] = post_effect_constant.toon_specular_smoothness;
    j["toon"]["rim_threshold"] = post_effect_constant.toon_rim_threshold;
    j["toon"]["rim_smoothness"] = post_effect_constant.toon_rim_smoothness;
    j["toon"]["rim_color"] = { post_effect_constant.toon_rim_color.x,
                                     post_effect_constant.toon_rim_color.y,
                                     post_effect_constant.toon_rim_color.z };
    j["toon"]["rim_intensity"] = post_effect_constant.toon_rim_color.w;

    //physxの保存
    j["physx"]["show_debug"] = showPhysxDebug;

    //カリングの保存
    j["culling"]["enable_frustum_culling"] = enableFrustumCulling;

    // ── Performance Options の保存
    j["performance"]["enable_shadows"] = enableShadows;
    j["performance"]["enable_bloom"] = enableBloom;

    //各クラスの保存処理
    Pitcher::Instance().SaveToJson(j["pitcher"]);
    Player::Instance().SaveToJson(j["player"]);
    Wind::Instance().SaveToJson(j["wind"]);
    Ball::Instance().SaveToJson(j["ball"]);
    skyRenderer.SaveToJson(j["sky"]);
    ballSprite::Instance().SaveToJson(j["ball_sprite"]);
    stage::Instance().SaveToJson(j["stage"]);
    //GameTimer::Instance().SaveToJson(j["gameTimer"]);
    HomeRunCount::Instance().SaveToJson(j["homeRunCount"]);
    Catcher::Instance().SaveToJson(j["catcher"]);
	BallDistance::Instance().SaveToJson(j["ballDistance"]);
	Result::Instance().SaveToJson(j["result"]);
	BallNet::Instance().SaveToJson(j["ballNet"]);
	Money::Instance().SaveToJson(j["money"]);

    // ファイルに保存
    std::ofstream file("resources\\setting\\settings.json");
    file << j.dump(4);
    consoleLog.push_back("[Info] Settings saved.");
}

void scene_game::LoadSetting()
{
    std::ifstream file("resources\\setting\\settings.json");
    if (!file.is_open())
    {
        consoleLog.push_back("[Warn] No settings file found. Using defaults.");
        return;
    }

    json j;
    file >> j;

    // カメラ設定の読み込み
    if (j.contains("camera"))
    {
        DirectX::XMFLOAT3 eye = { j["camera"]["eye"][0], j["camera"]["eye"][1], j["camera"]["eye"][2] };
        DirectX::XMFLOAT3 focus = { j["camera"]["focus"][0], j["camera"]["focus"][1], j["camera"]["focus"][2] };
        Camera& camera = Camera::Instance();
        camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
        freeCameraController.SyncCameraToController(camera);
        camera_near_z = j["camera"]["near_z"];
        camera_far_z = j["camera"]["far_z"];
    }

    // 中継カメラの読み込み（保存されていれば、デフォルト4台を全て置き換える）
    broadcastCamera.LoadFromJson(j["broadcast_camera"]);
    // 保存データが無ければ initialize() の SetupDefaultCameras() で設定済みのデフォルト4台のまま

    //タイムコントロールの読み込み
    if (j.contains("time"))
    {
        timeScale = j["time"]["time_scale"];
    }

    // ライト設定の読み込み
    if (j.contains("light"))
    {
        ambient_color = { j["light"]["ambient"][0],   j["light"]["ambient"][1],   j["light"]["ambient"][2],   j["light"]["ambient"][3] };
        directional_light_direction = { j["light"]["dir_dir"][0],   j["light"]["dir_dir"][1],   j["light"]["dir_dir"][2],   0.0f };
        directional_light_color = { j["light"]["dir_color"][0], j["light"]["dir_color"][1], j["light"]["dir_color"][2], 1.0f };
        directional_light_intensity = j["light"]["dir_intensity"];
    }

    //ポイントライトの読み込み
    if (j.contains("point_lights"))
    {
        for (size_t i = 0; i < j["point_lights"].size() && i < shadowRenderer.GetPointLights().size(); ++i)
        {
            shadowRenderer.GetPointLights()[i].position = { j["point_lights"][i]["pos"][0],   j["point_lights"][i]["pos"][1],   j["point_lights"][i]["pos"][2],   0.0f };
            shadowRenderer.GetPointLights()[i].color = { j["point_lights"][i]["color"][0], j["point_lights"][i]["color"][1], j["point_lights"][i]["color"][2], 1.0f };
            shadowRenderer.GetPointLights()[i].intensity = j["point_lights"][i]["intensity"];
            shadowRenderer.GetPointLights()[i].range = j["point_lights"][i]["range"];
        }
    }

    //スポットライトの読み込み
    if (j.contains("spot_lights"))
    {
        for (size_t i = 0; i < j["spot_lights"].size() && i < shadowRenderer.GetSpotLights().size(); ++i)
        {
            shadowRenderer.GetSpotLights()[i].position = { j["spot_lights"][i]["pos"][0], j["spot_lights"][i]["pos"][1], j["spot_lights"][i]["pos"][2], 0.0f };
            shadowRenderer.GetSpotLights()[i].direction = { j["spot_lights"][i]["dir"][0], j["spot_lights"][i]["dir"][1], j["spot_lights"][i]["dir"][2], 0.0f };
            shadowRenderer.GetSpotLights()[i].color = { j["spot_lights"][i]["color"][0], j["spot_lights"][i]["color"][1], j["spot_lights"][i]["color"][2], 1.0f };
            shadowRenderer.GetSpotLights()[i].intensity = j["spot_lights"][i]["intensity"];
            shadowRenderer.GetSpotLights()[i].range = j["spot_lights"][i]["range"];
            shadowRenderer.GetSpotLights()[i].innerCorn = j["spot_lights"][i]["innerCorn"];
            shadowRenderer.GetSpotLights()[i].outerCorn = j["spot_lights"][i]["outerCorn"];
        }
    }

    //ヘミスフィアライトとフォグの読み込み
    if (j.contains("hemisphere"))
    {
        sky_color = { j["hemisphere"]["sky_color"][0], j["hemisphere"]["sky_color"][1], j["hemisphere"]["sky_color"][2],1.0f };
        ground_color = { j["hemisphere"]["ground_color"][0], j["hemisphere"]["ground_color"][1], j["hemisphere"]["ground_color"][2], 1.0f };
        hemisphere_weight = j["hemisphere"]["weight"];
    }
    if (j.contains("fog"))
    {
        fog_color = { j["fog"]["color"][0], j["fog"]["color"][1], j["fog"]["color"][2], 1.0f };
        fog_range.x = j["fog"]["near"];
        fog_range.y = j["fog"]["far"];
    }

    //シャドウの読み込み
    if (j.contains("shadow"))
    {
        shadowRenderer.use_cascade_shadow_map = j["shadow"]["use_cascade"];
        shadowRenderer.cascade_shadow_constant.shadow_attenuation = j["shadow"]["cascade_attenuation"];
        shadowRenderer.cascade_shadow_constant.shadow_bias = { j["shadow"]["cascade_bias"][0], j["shadow"]["cascade_bias"][1], j["shadow"]["cascade_bias"][2], j["shadow"]["cascade_bias"][3] };
        shadowRenderer.shadow_bias = j["shadow"]["bias"];

        shadow_quality_constant.soft_shadow_enabled = j["shadow"].value("soft_enabled", 0);
        shadow_quality_constant.soft_shadow_samples = j["shadow"].value("soft_samples", 9);
        shadow_quality_constant.soft_shadow_radius = j["shadow"].value("soft_radius", 1.5f);

    }

    // ブルームの読み込み
    if (j.contains("bloom"))
    {
		bloomRenderer.LoadFromJson(j["bloom"]);
    }

    // トーンマッピング設定の読み込み
    if (j.contains("tone_mapping"))
    {
        post_effect_constant.tone_mapping_mode = j["tone_mapping"].value("mode", 0);
        post_effect_constant.tone_mapping_exposure = j["tone_mapping"].value("exposure", 1.0f);
        post_effect_constant.tone_mapping_white_point = j["tone_mapping"].value("white_point", 4.0f);
    }

    // トゥーンシェーディング設定の読み込み
    if (j.contains("toon"))
    {
        post_effect_constant.toon_shading_enabled = j["toon"].value("enabled", false) ? 1 : 0;
        post_effect_constant.toon_diffuse_steps = j["toon"].value("diffuse_steps", 3);
        post_effect_constant.toon_specular_threshold = j["toon"].value("spec_threshold", 0.6f);
        post_effect_constant.toon_specular_smoothness = j["toon"].value("spec_smoothness", 0.02f);
        post_effect_constant.toon_rim_threshold = j["toon"].value("rim_threshold", 0.7f);
        post_effect_constant.toon_rim_smoothness = j["toon"].value("rim_smoothness", 0.05f);
        if (j["toon"].contains("rim_color"))
        {
            post_effect_constant.toon_rim_color.x = j["toon"]["rim_color"][0];
            post_effect_constant.toon_rim_color.y = j["toon"]["rim_color"][1];
            post_effect_constant.toon_rim_color.z = j["toon"]["rim_color"][2];
        }
        post_effect_constant.toon_rim_color.w = j["toon"].value("rim_intensity", 0.5f);
    }

    //physxの読み込み
    if (j.contains("physx"))
    {
        showPhysxDebug = j["physx"]["show_debug"];
    }

    //カリングの読み込み
    if (j.contains("culling"))
    {
        enableFrustumCulling = j["culling"].value("enable_frustum_culling", true);
    }

    // ── Performance Options の読み込み
    if (j.contains("performance"))
    {
        enableShadows = j["performance"].value("enable_shadows", true);
        enableBloom = j["performance"].value("enable_bloom", true);
    }

    //各クラスの読み込み処理
    if (j.contains("pitcher")) Pitcher::Instance().LoadFromJson(j["pitcher"]);
    if (j.contains("player")) Player::Instance().LoadFromJson(j["player"]);
    if (j.contains("wind")) Wind::Instance().LoadFromJson(j["wind"]);
    if (j.contains("ball")) Ball::Instance().LoadFromJson(j["ball"]);
    //if (j.contains("sky")) skyRenderer.LoadFromJson(j["sky"]);
    if (j.contains("ball_sprite")) ballSprite::Instance().LoadFromJson(j["ball_sprite"]);
    if (j.contains("stage")) stage::Instance().LoadFromJson(j["stage"]);
    //if (j.contains("gameTimer")) GameTimer::Instance().LoadFromJson(j["gameTimer"]);
    if (j.contains("homeRunCount")) HomeRunCount::Instance().LoadFromJson(j["homeRunCount"]);
    if (j.contains("catcher")) Catcher::Instance().LoadFromJson(j["catcher"]);
	if (j.contains("ballDistance")) BallDistance::Instance().LoadFromJson(j["ballDistance"]);
	if (j.contains("result")) Result::Instance().LoadFromJson(j["result"]);
	if (j.contains("ballNet")) BallNet::Instance().LoadFromJson(j["ballNet"]);
	if (j.contains("money")) Money::Instance().LoadFromJson(j["money"]);
    consoleLog.push_back("[Info] Settings loaded.");
}