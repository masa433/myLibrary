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

CONST LONG SHADOWMAP_WIDTH{ 8192 };
CONST LONG SHADOWMAP_HEIGHT{ 8192 };
CONST float SHADOWMAP_DRAWRECT{ 100 };


void scene_game::initialize()
{
	HRESULT hr = S_OK;

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

		// シーン定数バッファの作成
        buffer_desc.ByteWidth = sizeof(scene_constants);
        HRESULT hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		// ライト定数バッファの作成
		buffer_desc.ByteWidth = sizeof(light_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, light_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		// 半球ライティング定数バッファの作成
		buffer_desc.ByteWidth = sizeof(hemisphere_light_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, hemisphere_light_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		// フォグ定数バッファの作成
        buffer_desc.ByteWidth = sizeof(fog_constants);
		hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, fog_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//シャドウマップの定数バッファの作成
		buffer_desc.ByteWidth = sizeof(shadowmap_constants);
		hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, shadowmap_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//高輝度抽出の定数バッファの作成
		buffer_desc.ByteWidth = sizeof(luminance_extract_constants);
		hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, luminance_extract_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //	ガウスフィルター用定数バッファ      
        buffer_desc.ByteWidth = sizeof(gaussian_filter_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, gaussian_filter_constant_buffer.GetAddressOf());
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

    // テクスチャマネージャーの初期化
    textureManager.Initialize(device, L"./resources/texture");

   
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

    // ライトから見たシーンの深度描画用バッファ
    {
        

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer{};
        D3D11_TEXTURE2D_DESC texture2d_desc{};
        texture2d_desc.Width = SHADOWMAP_WIDTH;
        texture2d_desc.Height = SHADOWMAP_HEIGHT;
        texture2d_desc.MipLevels = 1;
        texture2d_desc.ArraySize = 1;
        texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        texture2d_desc.SampleDesc.Count = 1;
        texture2d_desc.SampleDesc.Quality = 0;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0;
        texture2d_desc.MiscFlags = 0;
        HRESULT hr = device->CreateTexture2D(&texture2d_desc, NULL, depth_buffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //	深度ステンシルビュー生成
        D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
        depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
        depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depth_stencil_view_desc.Texture2D.MipSlice = 0;
        hr = device->CreateDepthStencilView(depth_buffer.Get(),
            &depth_stencil_view_desc,
            shadowmap_depth_stencil_view.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //	シェーダーリソースビュー生成
        D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
        shader_resource_view_desc.Format = DXGI_FORMAT_R32_FLOAT;
        shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
        shader_resource_view_desc.Texture2D.MipLevels = 1;
        hr = device->CreateShaderResourceView(depth_buffer.Get(),
            &shader_resource_view_desc,
            shadowmap_shader_resource_view.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // サンプラステートの生成
        {
            D3D11_SAMPLER_DESC sampler_desc{};
            sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
            sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
            sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
            sampler_desc.MipLODBias = 0;
            sampler_desc.MaxAnisotropy = 16;
            sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            sampler_desc.BorderColor[0] = FLT_MAX;
            sampler_desc.BorderColor[1] = FLT_MAX;
            sampler_desc.BorderColor[2] = FLT_MAX;
            sampler_desc.BorderColor[3] = FLT_MAX;
            sampler_desc.MinLOD = 0;
            sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
            hr = device->CreateSamplerState(&sampler_desc, shadowmap_sampler_state.GetAddressOf());
        }
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    }

    ////シーン描画用のバッファ生成
    Microsoft::WRL::ComPtr<ID3D11Texture2D> color_buffer{};
    D3D11_TEXTURE2D_DESC texture2d_desc{};
    texture2d_desc.Width = static_cast<UINT>(screenWidth);
    texture2d_desc.Height = static_cast<UINT>(screenHeight);
    texture2d_desc.MipLevels = 1;
    texture2d_desc.ArraySize = 1;
    texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture2d_desc.SampleDesc.Count = 1;
    texture2d_desc.SampleDesc.Quality = 0;
    texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
    texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture2d_desc.CPUAccessFlags = 0;
    texture2d_desc.MiscFlags = 0;
    hr = device->CreateTexture2D(&texture2d_desc, NULL, color_buffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    //	レンダーターゲットビュー生成
    hr = device->CreateRenderTargetView(color_buffer.Get(), NULL, scene_render_target_view.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    //	シェーダーリソースビュー生成
    hr = device->CreateShaderResourceView(color_buffer.Get(), NULL, scene_shader_resource_view.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    //シャドウマップ生成用シェーダー
    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "JOINTS", 0, DXGI_FORMAT_R16G16B16A16_UINT, 4, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "WEIGHTS", 0,DXGI_FORMAT_R32G32B32A32_FLOAT, 5, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        create_vs_from_cso(device,"shadowmap_caster_vs.cso", shadowmap_caster_vertex_shader.GetAddressOf(), shadowmap_caster_input_layout.GetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
    }

    //	スプライトシェーダー準備
    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        create_vs_from_cso(device, "sprite_vs.cso", sprite_vertex_shader.GetAddressOf(), sprite_input_layout.GetAddressOf(), input_element_desc, _countof(input_element_desc));
        create_ps_from_cso(device, "sprite_ps.cso", sprite_pixel_shader.GetAddressOf());

     
    }

    //高輝度抽出バッファ生成
    {
        D3D11_TEXTURE2D_DESC texture2d_desc{};
        texture2d_desc.Width = SCREEN_WIDTH;
        texture2d_desc.Height = SCREEN_HEIGHT;
        texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texture2d_desc.MipLevels = 1;
        texture2d_desc.ArraySize = 1;
        texture2d_desc.SampleDesc.Count = 1;
        texture2d_desc.SampleDesc.Quality = 0;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0;
        texture2d_desc.MiscFlags = 0;


        Microsoft::WRL::ComPtr<ID3D11Texture2D> color_buffer{};
		hr = device->CreateTexture2D(&texture2d_desc, NULL, color_buffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	レンダーターゲットビュー生成
        hr = device->CreateRenderTargetView(color_buffer.Get(), NULL, luminance_extract_render_target_view.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	シェーダーリソースビュー生成
        hr = device->CreateShaderResourceView(color_buffer.Get(), NULL, luminance_extract_shader_resource_view.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }

    //	高輝度抽出暈しバッファ生成
    {
        D3D11_TEXTURE2D_DESC texture2d_desc{};
        texture2d_desc.Width = SCREEN_WIDTH;
        texture2d_desc.Height = SCREEN_HEIGHT;
        texture2d_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texture2d_desc.MipLevels = 1;
        texture2d_desc.ArraySize = 1;
        texture2d_desc.SampleDesc.Count = 1;
        texture2d_desc.SampleDesc.Quality = 0;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0;
        texture2d_desc.MiscFlags = 0;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> color_buffer{};
        hr = device->CreateTexture2D(&texture2d_desc, NULL, color_buffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	レンダーターゲットビュー生成
        hr = device->CreateRenderTargetView(color_buffer.Get(), NULL, bokeh_luminance_extract_render_target_view.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	シェーダーリソースビュー生成
        hr = device->CreateShaderResourceView(color_buffer.Get(), NULL, bokeh_luminance_extract_shader_resource_view.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }


    //高輝度抽出用シェーダー
    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "JOINTS", 0, DXGI_FORMAT_R16G16B16A16_UINT, 4, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "WEIGHTS", 0,DXGI_FORMAT_R32G32B32A32_FLOAT, 5, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        create_ps_from_cso(device, "luminance_extract_ps.cso", luminance_extract_pixel_shader.GetAddressOf());
        luminance_extract_pass_sprite = std::make_unique<sprite>(device, scene_shader_resource_view);

        //	高輝度抽出バッファぼかし用
        create_ps_from_cso(device, "gaussian_filtering_ps.cso", gaussian_filter_pixel_shader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        bokeh_luminance_extract_pass_sprite = std::make_unique<sprite>(device, luminance_extract_shader_resource_view);


        //ぼかした結果を利用するスプライト
        add_luminance_extract_pass_sprite = std::make_unique<sprite>(device, bokeh_luminance_extract_shader_resource_view);
	}
}

void scene_game::update(float elapsed_time)
{
	elapsed_time *= timeScale;

    // カメラコントローラーの更新
	Camera& camera = Camera::Instance();
    cameraController.SyncControllerToCamera(camera);
    cameraController.Update();
	cameraPosition = camera.GetEye();

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

    if(ImGui::CollapsingHeader("Hemisphere Light & Fog"))
    {
        ImGui::ColorEdit3("Sky Color", &sky_color.x);
        ImGui::ColorEdit3("Ground Color", &ground_color.x);
        ImGui::SliderFloat("Hemisphere Weight", &hemisphere_weight, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::ColorEdit3("Fog Color", &fog_color.x);
        ImGui::SliderFloat("fog_near", &fog_range.x, 0.1f, +100.0f);
        ImGui::SliderFloat("fog_far", &fog_range.y, 0.1f, +100.0f);

    }

    if (ImGui::CollapsingHeader("Shadowmap"))
    {
        ImGui::SliderFloat("shadow_attenuation", &shadow_attenuation, 0.0f, 1.0f);
        ImGui::SliderFloat("shadow_bias", &shadow_bias, 0.0f, +0.01f);
        ImGui::Separator();

        ImGui::Text("scene_texture");
        ImGui::Image(ImTextureRef(scene_shader_resource_view.Get()), ImVec2(256, 144), ImVec2(0, 0), ImVec2(1, 1));
        ImGui::Text("shadow_map");
        ImGui::Image(ImTextureRef(shadowmap_shader_resource_view.Get()), ImVec2(256, 256), ImVec2(0, 0), ImVec2(1, 1));
    }

	ImGui::Checkbox("Show PhysX Debug", &showPhysxDebug);

    if (ImGui::CollapsingHeader("bloom"))
    {
        ImGui::Text("luminance_extract");
        ImGui::SliderFloat("threshold", &luminance_extract_constant.threshold, 0.0f, 2.0f);
        ImGui::SliderFloat("intensity", &luminance_extract_constant.intensity, 0.0f, 10.0f);
        ImGui::Image(ImTextureRef(luminance_extract_shader_resource_view.Get()), ImVec2(256, 256), ImVec2(0, 0), ImVec2(1, 1));

        ImGui::Text("bokeh_luminance_extract");
        ImGui::SliderInt("kernel", &gaussian_filter_data.kernel_size, 1, KernelMax);
        ImGui::SliderFloat("sigma", &gaussian_filter_data.sigma, 1.0f, 50.0f);
        ImGui::Image(ImTextureRef(bokeh_luminance_extract_shader_resource_view.Get()), ImVec2(256, 256), ImVec2(0, 0), ImVec2(1, 1));
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

void scene_game::renderShadowMap() 
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    // 描画コンテキスト設定
    RenderContext rc;
    rc.deviceContext = dc;
    rc.renderState = renderState;

	HRESULT hr = S_OK;
    //シャドウマップ生成処理
    {
        //シャドウマップ用の深度バッファに設定
		dc->ClearDepthStencilView(shadowmap_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        dc->OMSetRenderTargets(0, nullptr, shadowmap_depth_stencil_view.Get());
        //ビューポートの設定
        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(SHADOWMAP_WIDTH);
        viewport.Height = static_cast<float>(SHADOWMAP_HEIGHT);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        dc->RSSetViewports(1, &viewport);
		
        //ブレンドステートの設定
        dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
        //深度ステンシルステートの設定
        dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
        //ラスタライザーステートの設定
        dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
        //シェーダーの設定
        dc->IASetInputLayout(shadowmap_caster_input_layout.Get());
        dc->VSSetShader(shadowmap_caster_vertex_shader.Get(), nullptr, 0);
        dc->PSSetShader(nullptr, nullptr, 0);
        
        Camera& camera = Camera::Instance();

		//ライトのビュー射影行列の計算
		DirectX::XMVECTOR LightPosition = DirectX::XMLoadFloat4(&directional_light_direction);
		LightPosition = DirectX::XMVectorScale(LightPosition, -50);

        DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(LightPosition,
            DirectX::XMVectorSet(camera.GetFocus().x, camera.GetFocus().y, camera.GetFocus().z, 1.0f),
            DirectX::XMVectorSet(camera.GetUp().x, camera.GetUp().y, camera.GetUp().z, 0.0f));

        // シャドウマップに描画したい範囲の射影行列を生成
        DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicLH(SHADOWMAP_DRAWRECT, SHADOWMAP_DRAWRECT,
            0.1f, 200.0f);

        // ライトビュー行列を保存
       
        DirectX::XMStoreFloat4x4(&light_view_projection, V * P);

        //定数バッファの更新
        {
            //ライトから見たシーンのビュー射影行列を計算して定数バッファに転送
			scene_constants scene{};
			scene.camera_position.x = cameraPosition.x;
			scene.camera_position.y = cameraPosition.y;
			scene.camera_position.z = cameraPosition.z;
			//DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
			scene.view_projection = light_view_projection;
			dc->UpdateSubresource(constant_buffer.Get(), 0, 0, &scene, 0, 0);
			dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
			dc->PSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());

        }

        //モデルの描画
		stage::Instance().render(rc,modelRenderer);

        // プレイヤー・ピッチャーの描画(カリングなしで両面描画)
        //dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

        // ピッチャーの描画
        Pitcher::Instance().Render(rc, modelRenderer);

        // プレイヤーの描画
        Player::Instance().Render(rc, modelRenderer);


        // プレイヤー描画後、元のカリング状態に戻しておく
        //dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
    }
}

void scene_game::render(float elapsedTime)
{
   // renderShadowMap();

    using namespace DirectX;

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    RenderContext rc;
    rc.deviceContext = dc;
    rc.renderState = renderState;

    Camera& camera = Camera::Instance();

    
	//ポイントライトの描画
    for (int i = 0; i < 6; ++i)
    {
        //大きさは変わらない
        shapeRenderer->DrawPointLight(DirectX::XMFLOAT3(pointLights[i].position.x, pointLights[i].position.y, pointLights[i].position.z), 0.1, pointLights[i].color);
    }

	//スポットライトの描画
    for (int i = 0; i < 6; ++i)
    {
        shapeRenderer->DrawSpotLight(
            DirectX::XMFLOAT3(spotLights[i].position.x, spotLights[i].position.y, spotLights[i].position.z),
            DirectX::XMFLOAT3(spotLights[i].direction.x, spotLights[i].direction.y, spotLights[i].direction.z),
            spotLights[i].range,
            spotLights[i].innerCorn,
            spotLights[i].outerCorn,
            spotLights[i].color
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

        light_constants lightConstants{};
        lightConstants.ambient_color = ambient_color;
        lightConstants.directional_light_direction = directional_light_direction;
        lightConstants.directional_light_color = directional_light_color;
        memcpy_s(lightConstants.pointLights, sizeof(lightConstants.pointLights), pointLights, sizeof(pointLights));
        memcpy_s(lightConstants.spotLights, sizeof(lightConstants.spotLights), spotLights, sizeof(spotLights));
        dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lightConstants, 0, 0);
        dc->VSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());
        dc->PSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());

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

        shadowmap_constants shadowmapConstants{};
        shadowmapConstants.light_view_projection = light_view_projection;
        shadowmapConstants.shadow_attenuation = shadow_attenuation;
        shadowmapConstants.shadow_bias = shadow_bias;
        dc->UpdateSubresource(shadowmap_constant_buffer.Get(), 0, 0, &shadowmapConstants, 0, 0);
        dc->VSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());
        dc->PSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());
    }

    // サンプラーステート
    ID3D11SamplerState* samplerStates[] = {
        renderState->GetSamplerState(SamplerState::PointWrap),
        renderState->GetSamplerState(SamplerState::PointClamp),
        renderState->GetSamplerState(SamplerState::PointWrap)
    };
    dc->PSSetSamplers(0, 3, samplerStates);
    dc->PSSetShaderResources(10, 1, shadowmap_shader_resource_view.GetAddressOf());
    dc->PSSetSamplers(10, 1, shadowmap_sampler_state.GetAddressOf());

    // レンダーステート
   /* dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));*/

    stage::Instance().render(rc, modelRenderer);

    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
    Pitcher::Instance().Render(rc, modelRenderer);
    Player::Instance().Render(rc, modelRenderer);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

    if (showPhysxDebug)
        Physics::Instance().Render(camera.GetView(), camera.GetProjection(), rc.lightDirection);

    // ここで高輝度抽出とぼかしを実行してパスのSRVを更新する
    luminance_extract_pass(elapsedTime);
    bokeh_luminance_extract_pass(elapsedTime);


    // 使い終わったらシャドウマップをアンバインド
    ID3D11ShaderResourceView* null_srv[] = { nullptr };
    dc->PSSetShaderResources(10, 1, null_srv);

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

    textureManager.Render(dc);

    //	ぼかした結果を加算合成
    {
        dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
        dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
        dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

        //	シェーダー設定
        dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
        dc->PSSetShader(sprite_pixel_shader.Get(), nullptr, 0);
        dc->IASetInputLayout(sprite_input_layout.Get());

        add_luminance_extract_pass_sprite->render(dc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void scene_game::luminance_extract_pass(float elapsedTime)
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

    //バックバッファ指定
    {

        // 高輝度抽出用のレンダーターゲットをクリアしてセット
        float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        dc->ClearRenderTargetView(luminance_extract_render_target_view.Get(), clear_color);
        dc->OMSetRenderTargets(1, luminance_extract_render_target_view.GetAddressOf(), nullptr);

    }

    //ビューポートの設定
    {
        D3D11_VIEWPORT scene_viewport{};
        scene_viewport.TopLeftX = 0;
        scene_viewport.TopLeftY = 0;
        scene_viewport.Width = static_cast<float>(SCREEN_WIDTH);
        scene_viewport.Height = static_cast<float>(SCREEN_HEIGHT);
        scene_viewport.MinDepth = 0.0f;
        scene_viewport.MaxDepth = 1.0f;
        dc->RSSetViewports(1, &scene_viewport);
    }

    //リソース設定
    {
        //	定数バッファ設定
        static constexpr int SceneCBVIndex = 1;
		scene_constants scene{};
		scene.camera_position.x = cameraPosition.x;
		scene.camera_position.y = cameraPosition.y;
		scene.camera_position.z = cameraPosition.z;
		Camera& camera = Camera::Instance();
		DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&camera.GetView());
		DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&camera.GetProjection());
		DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
        dc->UpdateSubresource(constant_buffer.Get(), 0, 0, &scene, 0, 0);
        dc->PSSetConstantBuffers(SceneCBVIndex, 1, constant_buffer.GetAddressOf());

        //	サンプラステート設定
        static constexpr int SamplerStateIndex = 0;
        ID3D11SamplerState* sampler_states[] =
        {
            Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::LinearClamp),

        };

		dc->PSSetSamplers(SamplerStateIndex, ARRAYSIZE(sampler_states), sampler_states);
		dc->VSSetSamplers(SamplerStateIndex, ARRAYSIZE(sampler_states), sampler_states);

        //	高輝度抽出用情報設定
        static constexpr int LuminanceExtractCBVIndex = 2;
        dc->UpdateSubresource(luminance_extract_constant_buffer.Get(), 0, 0, &luminance_extract_constant, 0, 0);
		dc->PSSetConstantBuffers(LuminanceExtractCBVIndex, 1, luminance_extract_constant_buffer.GetAddressOf());
		
    }

    //描画
    {
        dc->OMSetBlendState(Graphics::Instance().GetRenderState()->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
        dc->OMSetDepthStencilState(Graphics::Instance().GetRenderState()->GetDepthStencilState(DepthState::TestAndWrite), 0);
        dc->RSSetState(Graphics::Instance().GetRenderState()->GetRasterizerState(RasterizerState::SolidCullNone));

        dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
        dc->PSSetShader(luminance_extract_pixel_shader.Get(), nullptr, 0);
        dc->IASetInputLayout(sprite_input_layout.Get());

        luminance_extract_pass_sprite->render(dc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    }

    //シェーダー登録解除
    {

        dc->VSSetShader(nullptr, nullptr, 0);
        dc->PSSetShader(nullptr, nullptr, 0);
        dc->IASetInputLayout(nullptr);
    }
}

void scene_game::calculate_gaussian_filter_constant(gaussian_filter_constants& constant, const gaussian_filter_datas& data)
{
    //偶数の場合は奇数に直す
	int kernel_size = data.kernel_size;
    if (kernel_size % 2 == 0)
    {
        kernel_size++;
    }
    constant.kernel_size = static_cast<float>(kernel_size);
    constant.texcel.x = 1.0f / data.texture_size.x;
    constant.texcel.y = 1.0f / data.texture_size.y;

    //重みを算出
	float sum = 0.0f;
	int id = 0;
    for(int y = -kernel_size / 2; y <= kernel_size / 2; y++)
    {
        for (int x = -kernel_size / 2; x <= kernel_size / 2; x++)
        {
            constant.weights[id].x = (float)x;
            constant.weights[id].y = (float)y;
            constant.weights[id].z = (float)exp(-(x * x + y * y) / (2.0f * data.sigma * data.sigma)) / (2.0f * DirectX::XM_PI * data.sigma);
            sum += constant.weights[id].z;
            id++;

        }
	}
    //平均化
    for(int i = 0; i < KernelMax * KernelMax; i++)
    {
        constant.weights[i].z /= sum;
	}
}

void scene_game::bokeh_luminance_extract_pass(float elapsedTime)
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    //バックバッファ指定
    {
       
        dc->OMSetRenderTargets(1, bokeh_luminance_extract_render_target_view.GetAddressOf(), nullptr);

        
        float color[4] = { 0, 0, 0, 1 }; // 黒
        dc->ClearRenderTargetView(
            bokeh_luminance_extract_render_target_view.Get(),
            color
        );

    }
    //ビューポートの設定
    {
        D3D11_VIEWPORT scene_viewport{};
        scene_viewport.TopLeftX = 0;
        scene_viewport.TopLeftY = 0;
        scene_viewport.Width = static_cast<float>(SCREEN_WIDTH);
        scene_viewport.Height = static_cast<float>(SCREEN_HEIGHT);
        scene_viewport.MinDepth = 0.0f;
        scene_viewport.MaxDepth = 1.0f;
        dc->RSSetViewports(1, &scene_viewport);
    }
    //リソース設定
    {
        //	定数バッファ設定
        static constexpr int SceneCBVIndex = 1;
        scene_constants scene{};
        scene.camera_position.x = cameraPosition.x;
        scene.camera_position.y = cameraPosition.y;
        scene.camera_position.z = cameraPosition.z;
        Camera& camera = Camera::Instance();
        DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&camera.GetView());
        DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&camera.GetProjection());
        DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
        dc->UpdateSubresource(constant_buffer.Get(), 0, 0, &scene, 0, 0);
        dc->PSSetConstantBuffers(SceneCBVIndex, 1, constant_buffer.GetAddressOf());
        //	サンプラステート設定
        static constexpr int SamplerStateIndex = 0;
        ID3D11SamplerState* sampler_states[] =
        {
            Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::LinearClamp),
        };
        dc->PSSetSamplers(SamplerStateIndex, ARRAYSIZE(sampler_states), sampler_states);
        
        //	ガウシアンフィルター情報設定
        {
            gaussian_filter_constants gaussian_filter_constant;
            calculate_gaussian_filter_constant(gaussian_filter_constant, gaussian_filter_data);

            //	定数バッファを設定
            static constexpr int GaussianFilterCBVIndex = 2;
            dc->UpdateSubresource(gaussian_filter_constant_buffer.Get(), 0, 0, &gaussian_filter_constant, 0, 0);
            dc->PSSetConstantBuffers(GaussianFilterCBVIndex, 1, gaussian_filter_constant_buffer.GetAddressOf());
        }

    }
    //描画
    {
        dc->OMSetBlendState(Graphics::Instance().GetRenderState()->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
        dc->OMSetDepthStencilState(Graphics::Instance().GetRenderState()->GetDepthStencilState(DepthState::TestAndWrite), 0);
        dc->RSSetState(Graphics::Instance().GetRenderState()->GetRasterizerState(RasterizerState::SolidCullNone));
        dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
        dc->PSSetShader(gaussian_filter_pixel_shader.Get(), nullptr, 0);
        dc->IASetInputLayout(sprite_input_layout.Get());
        bokeh_luminance_extract_pass_sprite->render(dc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    //シェーダー登録解除
    {

        dc->VSSetShader(nullptr, nullptr, 0);
        dc->PSSetShader(nullptr, nullptr, 0);
        dc->IASetInputLayout(nullptr);
    }

}

void scene_game::uninitialize()
{
    // 終了処理
    Player::Instance().Uninitialize();
    stage::Instance().uninitialize();
    Pitcher::Instance().Uninitialize();
    textureManager.Clear();
    Physics::Instance().Finalize();
}

void scene_game::DrawGUI()
{
    // プレイヤーのGUI描画
    Player::Instance().DrawGUI();

	// ピッチャーのGUI描画
	Pitcher::Instance().DrawGUI();

    textureManager.DrawGUI();

}