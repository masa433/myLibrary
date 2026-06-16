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
#include <fstream>
#include <string>

using json = nlohmann::json;

//	シャドウマップサイズ
static constexpr UINT ShadowmapSize = 4096;
static constexpr UINT SpotShadowmapSize = 4096;
static constexpr float ShadowmapDrawRect = 60;



void scene_game::initialize()
{
	HRESULT hr = S_OK;

    ID3D11Device* device = Graphics::Instance().GetDevice();

	// コンソールログを物理システムに渡す
    Physics::Instance().SetConsoleLog(&consoleLog);
	Pitcher::Instance().SetConsoleLog(&consoleLog);

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
        
        //カスケードシャドウ用定数バッファ
        buffer_desc.ByteWidth = sizeof(cascade_shadowmap_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, cascade_shadowmap_constant_buffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//スポットシャドウ用定数バッファ
        buffer_desc.ByteWidth = sizeof(spot_shadowmap_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, spot_shadowmap_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }
    
	// スカイレンダラーの初期化
    {
        skyRenderer.Initialize(device);

        // 初期の昼間設定（正午）
        skyRenderer.time_of_day = 0.5f;
        skyRenderer.auto_advance_time = false;
        skyRenderer.time_speed = 0.02f;
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

    pointLights.resize(36);
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
    pointLights.clear();

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

                point_lights pl{};
                XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&pl.position), finalPos);
                pl.position.w = 1.f;
                pl.range = 50.f;
                pl.color = { 1.f, 0.98f, 0.9f, 1.f };
                pl.intensity = 2.0f;
                pointLights.push_back(pl);
            }
        }
    }

    spotLights.resize(6);

    // 位置
    spotLights[0].position = { 80.0f, 95.0f, -80.0f, 1.0f };
    spotLights[1].position = { -80.0f, 95.0f, -80.0f, 1.0f };
    spotLights[2].position = { -150.0f, 95.0f, 40.0f, 1.0f };
    spotLights[3].position = { 150.0f, 95.0f, 40.0f, 1.0f };
    spotLights[4].position = { 75.0f, 80.0f, 135.0f, 1.0f };
    spotLights[5].position = { -75.0f, 80.0f, 135.0f, 1.0f };

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
            DirectX::XMLoadFloat4(&spotLights[i].position);

        DirectX::XMVECTOR target =
            DirectX::XMVectorSet(
                targets[i].x,
                targets[i].y,
                targets[i].z,
                0.0f);

        DirectX::XMVECTOR dir =
            DirectX::XMVector3Normalize(target - pos);

        DirectX::XMStoreFloat4(&spotLights[i].direction, dir);

        spotLights[i].color = { 1,1,1,1 };
        spotLights[i].range = 250.0f;
        spotLights[i].intensity = 3.0f;
        spotLights[i].innerCorn = DirectX::XMConvertToRadians(50.0f);
        spotLights[i].outerCorn = DirectX::XMConvertToRadians(90.0f);
    }

    // ライトから見たシーンの深度描画用バッファ
    {

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer{};
        D3D11_TEXTURE2D_DESC texture2d_desc{};
        texture2d_desc.Width = ShadowmapSize;
        texture2d_desc.Height = ShadowmapSize;
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

    //カスケードシャドウマップ生成
    {
        D3D11_TEXTURE2D_DESC texture2d_desc{};
        texture2d_desc.Width = ShadowmapSize;
        texture2d_desc.Height = ShadowmapSize;
        texture2d_desc.MipLevels = 1;
        texture2d_desc.ArraySize = 1;
        texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        texture2d_desc.SampleDesc.Count = 1;
        texture2d_desc.SampleDesc.Quality = 0;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0;
        texture2d_desc.MiscFlags = 0;

        for (int index = 0; index < ShadowBufferSize; ++index)
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer{};
            hr = device->CreateTexture2D(&texture2d_desc, NULL, depth_buffer.GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

            //	深度ステンシルビュー生成
            D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
            depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
            depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            depth_stencil_view_desc.Texture2D.MipSlice = 0;
            hr = device->CreateDepthStencilView(depth_buffer.Get(),
                &depth_stencil_view_desc,
                cascade_shadowmap_depth_stencil_views[index].GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

            //	シェーダーリソースビュー生成
            D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
            shader_resource_view_desc.Format = DXGI_FORMAT_R32_FLOAT;
            shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
            shader_resource_view_desc.Texture2D.MipLevels = 1;
            hr = device->CreateShaderResourceView(depth_buffer.Get(),
                &shader_resource_view_desc,
                cascade_shadowmap_shader_resource_views[index].GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        }
    }

    //スポットシャドウマップ生成
    {
        D3D11_TEXTURE2D_DESC texture2d_desc{};
        texture2d_desc.Width = SpotShadowmapSize;
        texture2d_desc.Height = SpotShadowmapSize;
        texture2d_desc.MipLevels = 1;
        texture2d_desc.ArraySize = 1;
        texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        texture2d_desc.SampleDesc.Count = 1;
        texture2d_desc.SampleDesc.Quality = 0;
        texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
        texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        texture2d_desc.CPUAccessFlags = 0;
        texture2d_desc.MiscFlags = 0;

        for (int index = 0; index < SpotShadowCount; ++index)
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer{};
            hr = device->CreateTexture2D(&texture2d_desc, NULL, depth_buffer.GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
            dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
            dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Texture2D.MipSlice = 0;
            hr = device->CreateDepthStencilView(depth_buffer.Get(), &dsv_desc,
                spot_shadowmap_depth_stencil_views[index].GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
            srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
            srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MostDetailedMip = 0;
            srv_desc.Texture2D.MipLevels = 1;
            hr = device->CreateShaderResourceView(depth_buffer.Get(), &srv_desc,
                spot_shadowmap_shader_resource_views[index].GetAddressOf());
            _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        }

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
        texture2d_desc.Width = Graphics::Instance().GetScreenWidth();
        texture2d_desc.Height = Graphics::Instance().GetScreenHeight();
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
        texture2d_desc.Width = Graphics::Instance().GetScreenWidth();
        texture2d_desc.Height = Graphics::Instance().GetScreenHeight();
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

    //ドローコール表示用
	D3D11_QUERY_DESC query_desc{};
    query_desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
    query_desc.MiscFlags = 0;
    hr = device->CreateQuery(&query_desc, pipeline_stats_query.GetAddressOf());
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

	elapsed_time *= timeScale;

    // カメラ追跡の開始チェック
    if (Physics::Instance().GetBallWasHit())
    {
        Physics::Instance().ClearBallWasHit();
        cameraController.StartTrackingBall(&Ball::Instance(), 3.0f, 0.5f);
    }

    // カメラコントローラーの更新
	Camera& camera = Camera::Instance();
    cameraController.Update(elapsed_time);
    cameraController.SyncControllerToCamera(camera);
	cameraPosition = camera.GetEye();

    // 追跡終了条件（例：ボールが止まったら）
    if (cameraController.IsTrackingBall())
    {
        const auto& vel = Ball::Instance().GetVelocity();
        float speed = sqrtf(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
        if (speed < 0.5f)
        {
            cameraController.StopTrackingBall();
        }
    }

    // ステージの更新
    stage::Instance().update(elapsed_time);

    // プレイヤーの更新
    Player::Instance().Update(elapsed_time);

	// ピッチャーの更新
    Pitcher::Instance().Update(elapsed_time);

    // 物理システムの更新
    Physics::Instance().Update(elapsed_time);

    // スカイレンダラーの更新
    skyRenderer.Update(elapsed_time * timeScale);

    //太陽方向をライト方向と同期
	directional_light_direction = skyRenderer.GetSunDirectionToLight();

    //時刻が0.2以上0.7以下の時はポイントライトとスポットライトを消す
    if (skyRenderer.time_of_day >= 0.2f && skyRenderer.time_of_day <= 0.7f)
    {
        directional_light_intensity = 0.7f;
        ambient_color = {1.0f, 1.0f, 1.0f, 1.0f};
        for (auto& pl : pointLights)
        {
            pl.intensity = 0.0f;
        }
        for (auto& sl : spotLights)
        {
            sl.intensity = 0.0f;
        }
    }
    else
    {
		directional_light_intensity = 0.0f;
        ambient_color = { 0.7f, 0.7f, 0.7f, 1.0f };
        for (auto& pl : pointLights)
        {
            pl.intensity = 2.0f;
        }
        for (auto& sl : spotLights)
        {
            sl.intensity = 2.0f;
        }
	}


}

void scene_game::renderSpotShadowMap(float elapsedTime)
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    RenderContext rc;
    rc.deviceContext = dc;
    rc.renderState = renderState;

    // ビューポートはシャドウマップサイズに固定
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(SpotShadowmapSize);
    viewport.Height = static_cast<float>(SpotShadowmapSize);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    dc->RSSetViewports(1, &viewport);

    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
    dc->IASetInputLayout(shadowmap_caster_input_layout.Get());
    dc->VSSetShader(shadowmap_caster_vertex_shader.Get(), nullptr, 0);
    dc->PSSetShader(nullptr, nullptr, 0);

    const int count = static_cast<int>(spotLights.size());
    for (int i = 0; i < count && i < SpotShadowCount; ++i)
    {
        // DSVクリア & バインド
        dc->ClearDepthStencilView(spot_shadowmap_depth_stencil_views[i].Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        dc->OMSetRenderTargets(0, nullptr, spot_shadowmap_depth_stencil_views[i].Get());

        // ライトビュー行列（位置 → 照射方向）
        using namespace DirectX;

        XMFLOAT3 posF3 = {spotLights[i].position.x, spotLights[i].position.y, spotLights[i].position.z};
		XMFLOAT3 dirF3 = { spotLights[i].direction.x, spotLights[i].direction.y, spotLights[i].direction.z };

        XMVECTOR pos = XMLoadFloat3(&posF3);
        XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&dirF3));
        XMVECTOR up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        if (fabsf(XMVectorGetY(dir)) > 0.99f)
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        XMMATRIX V = XMMatrixLookToLH(pos, dir, up);

        // ライトプロジェクション行列（outerCorn の2倍をFovYに）
        float fovY = spotLights[i].outerCorn * 2.0f; // outerCorn はラジアン半角
        XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, 1.0f, 1.0f, spotLights[i].range);

        XMStoreFloat4x4(&spot_shadow_constant.light_view_projection[i], V * P);

        // 定数バッファを更新してVSにセット（b1のview_projectionを上書き）
        scene_constants scene{};
        scene.camera_position = { posF3.x,posF3.y,posF3.z, 1.0f };
        scene.view_projection = spot_shadow_constant.light_view_projection[i];
        dc->UpdateSubresource(constant_buffer.Get(), 0, 0, &scene, 0, 0);
        dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());

        stage::Instance().render(rc, modelRenderer);
        Pitcher::Instance().Render(rc, modelRenderer);
        Player::Instance().Render(rc, modelRenderer);
    }
}

void scene_game::renderShadowMap(float elapsedTime) 
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
        viewport.Width = static_cast<float>(ShadowmapSize);
        viewport.Height = static_cast<float>(ShadowmapSize);
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
        DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicLH(ShadowmapDrawRect, ShadowmapDrawRect,
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
		stage::Instance().render(rc, modelRenderer);

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
    if (use_cascade_shadow_map)
    {
        renderCascadeShadowMap(elapsedTime);
    }
    else
    {
        renderShadowMap(elapsedTime);
    }

    //renderSpotShadowMap(elapsedTime);
    spot_shadow_frame_count++;
    if(spot_shadow_frame_count >= spot_shadow_update_interval)
    {
        renderSpotShadowMap(elapsedTime);
        spot_shadow_frame_count = 0;
	}

    using namespace DirectX;

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    RenderContext rc;
    rc.deviceContext = dc;
    rc.renderState = renderState;

    Camera& camera = Camera::Instance();

    // 前フレームの結果を取得（ノンブロッキング）
    /*dc->GetData(pipeline_stats_query.Get(), &pipeline_stats,
        sizeof(pipeline_stats), D3D11_ASYNC_GETDATA_DONOTFLUSH);*/

    // 今フレームの計測開始
    //dc->Begin(pipeline_stats_query.Get());


	//ポイントライトの描画
    for (int i = 0; i < pointLights.size(); ++i)
    {
        //大きさは変わらない
        shapeRenderer->DrawPointLight(DirectX::XMFLOAT3(pointLights[i].position.x, pointLights[i].position.y, pointLights[i].position.z), 0.1, pointLights[i].color);
    }

	//スポットライトの描画
    
        for (int i = 0; i < spotLights.size(); ++i)
        {
            shapeRenderer->DrawSpotLight(
                DirectX::XMFLOAT3(spotLights[i].position.x, spotLights[i].position.y, spotLights[i].position.z),
                DirectX::XMFLOAT3(spotLights[i].direction.x, spotLights[i].direction.y, spotLights[i].direction.z),
                5.0f,
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
        
        for (auto& point_light : pointLights)
        {
			lightConstants.point_light[lightConstants.light_count.y] = point_light;
            if (++lightConstants.light_count.y == light_constants::light_max)
                break;
        }
        for (auto& spot_light : spotLights)
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

        shadowmap_constants shadowmapConstants{};
        shadowmapConstants.light_view_projection = light_view_projection;
        shadowmapConstants.shadow_attenuation = shadow_attenuation;
        shadowmapConstants.shadow_bias = shadow_bias;
		shadowmapConstants.use_cascade = use_cascade_shadow_map;
        dc->UpdateSubresource(shadowmap_constant_buffer.Get(), 0, 0, &shadowmapConstants, 0, 0);
        dc->VSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());
        dc->PSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());

        // スポットシャドウマップ定数バッファ更新 (b7)
        spot_shadow_constant.shadow_attenuation = shadow_attenuation; // 既存の値を流用
        spot_shadow_constant.shadow_bias = 0.005f;
        dc->UpdateSubresource(spot_shadowmap_constant_buffer.Get(), 0, 0, &spot_shadow_constant, 0, 0);
        dc->VSSetConstantBuffers(7, 1, spot_shadowmap_constant_buffer.GetAddressOf());
        dc->PSSetConstantBuffers(7, 1, spot_shadowmap_constant_buffer.GetAddressOf());
    

    // サンプラーステート
    ID3D11SamplerState* sampler_states[] =
    {
        Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::PointClamp),        // s0: POINT
        Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::LinearClamp),  // s1: LINEAR
        Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::AnisotropicClamp),  // s2: ANISOTROPIC
    };
    dc->PSSetSamplers(0, ARRAYSIZE(sampler_states), sampler_states);
    dc->VSSetSamplers(0, ARRAYSIZE(sampler_states), sampler_states);
    if (use_cascade_shadow_map)
    {
		
        dc->UpdateSubresource(cascade_shadowmap_constant_buffer.Get(), 0, 0,
            &cascade_shadow_constant, 0, 0);
        dc->VSSetConstantBuffers(8, 1, cascade_shadowmap_constant_buffer.GetAddressOf());
        dc->PSSetConstantBuffers(8, 1, cascade_shadowmap_constant_buffer.GetAddressOf());
        for (int i = 0; i < ShadowBufferSize; ++i)
        {
            dc->PSSetShaderResources(20 + i, 1,
                cascade_shadowmap_shader_resource_views[i].GetAddressOf());
        }
    }
    else
    {
        dc->PSSetShaderResources(10, 1, shadowmap_shader_resource_view.GetAddressOf());
    }

    // スポットシャドウマップSRVをt30～t35にバインド
    for (int i = 0; i < SpotShadowCount; ++i)
    {
        dc->PSSetShaderResources(30 + i, 1, spot_shadowmap_shader_resource_views[i].GetAddressOf());
    }

    dc->PSSetSamplers(10, 1, shadowmap_sampler_state.GetAddressOf());

    skyRenderer.Render(
        dc,
        constant_buffer.Get(),
        renderState->GetDepthStencilState(DepthState::TestOnly),   //深度テストのみ・書き込みなし
        renderState->GetRasterizerState(RasterizerState::SolidCullNone)
    );

    // 深度・ラスタライザーを元に戻す
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

    stage::Instance().render(rc, modelRenderer);

    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    // --- ambient を一時的に無効化 ---
    light_constants noAmbientConstants{};
    noAmbientConstants = lightConstants; // 直前に作った lightConstants をコピー
    // ※ lightConstants がスコープ外なら再構築が必要

    light_constants noAmbientLight = lightConstants;
    //時刻が0.2以上0.7以下の時に強くする
    if(skyRenderer.time_of_day >= 0.2f && skyRenderer.time_of_day <= 0.7f)
    {
        noAmbientLight.ambient_color = { 1.5f, 1.5f, 1.5f, 1.0f };
    }
	else
    noAmbientLight.ambient_color = { 1.2f, 1.2f, 1.2f, 1.0f };

    dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &noAmbientLight, 0, 0);
    dc->PSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());

    // 半球ライトも無効化
    hemisphere_light_constants noHemi{};
    noHemi.sky_color = { 0, 0, 0, 0 };
    noHemi.ground_color = { 0, 0, 0, 0 };
    noHemi.hemisphere_weight.x = 0.0f;
    dc->UpdateSubresource(hemisphere_light_constant_buffer.Get(), 0, 0, &noHemi, 0, 0);
    dc->PSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());

    Pitcher::Instance().Render(rc, modelRenderer);
	Player::Instance().RenderPlayer(rc, modelRenderer);

    // --- ambient を元に戻す ---
    dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lightConstants, 0, 0);
    dc->PSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());

    hemisphere_light_constants hemi{};
    hemi.sky_color = sky_color;
    hemi.ground_color = ground_color;
    hemi.hemisphere_weight.x = hemisphere_weight;
    dc->UpdateSubresource(hemisphere_light_constant_buffer.Get(), 0, 0, &hemi, 0, 0);
    dc->PSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());

    // バットだけ ambient を 0 にして描画
    {
        light_constants noAmbientLight = lightConstants;  //  lightConstantsをメンバ変数に昇格する必要あり
		//バットも時刻が0.2以上0.7以下の時に強くする
        if (skyRenderer.time_of_day >= 0.2f && skyRenderer.time_of_day <= 0.7f)
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


    

    // ShapeRenderer の描画実行

    if (showPhysxDebug)

    {
        shapeRenderer->Render(
            dc,
            camera.GetView(),
            camera.GetProjection(),
            rc.lightDirection
        );

        Physics::Instance().Render(camera.GetView(), camera.GetProjection(), rc.lightDirection);
    }

    // ここで高輝度抽出とぼかしを実行してパスのSRVを更新する
    luminance_extract_pass(elapsedTime);
    bokeh_luminance_extract_pass(elapsedTime);


    // 使い終わったらシャドウマップをアンバインド
    ID3D11ShaderResourceView* null_srv[] = { nullptr };
    dc->PSSetShaderResources(10, 1, null_srv);

	//カスケードシャドウマップをアンバインド
    if (use_cascade_shadow_map)
    {
        ID3D11ShaderResourceView* nullSRVs[ShadowBufferSize] = {};
        dc->PSSetShaderResources(20, ShadowBufferSize, nullSRVs);
	}

    // 既存の1スロットアンバインドを6スロットに拡張
    ID3D11ShaderResourceView* nullSpotSRVs[SpotShadowCount] = {};
    dc->PSSetShaderResources(30, SpotShadowCount, nullSpotSRVs);

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
        dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);
        dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
        dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

        //	シェーダー設定
        dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
        dc->PSSetShader(sprite_pixel_shader.Get(), nullptr, 0);
        dc->IASetInputLayout(sprite_input_layout.Get());

        add_luminance_extract_pass_sprite->render(dc, 0, 0, Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight());
    }

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

    //計測終了
	//dc->End(pipeline_stats_query.Get());
}

//カスケードシャドウマップ生成関数
void scene_game::renderCascadeShadowMap(float elapsedTime)
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

    RenderContext rc;
    rc.deviceContext = dc;
    rc.renderState = renderState;

    Camera& camera = Camera::Instance();

	//ビューポートの設定
    {
        D3D11_VIEWPORT scene_viewport{};
        scene_viewport.TopLeftX = 0;
        scene_viewport.TopLeftY = 0;
        scene_viewport.Width = static_cast<float>(ShadowmapSize);
        scene_viewport.Height = static_cast<float>(ShadowmapSize);
        scene_viewport.MinDepth = 0.0f;
        scene_viewport.MaxDepth = 1.0f;
        dc->RSSetViewports(1, &scene_viewport);
    }

	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    // カメラの右・上・前ベクトルを取得（camera.h に GetRight/GetUp/GetFront がある）
    DirectX::XMVECTOR CameraRight = DirectX::XMLoadFloat3(&camera.GetRight());
    DirectX::XMVECTOR CameraUp = DirectX::XMLoadFloat3(&camera.GetUp());
    DirectX::XMVECTOR CameraFront = DirectX::XMLoadFloat3(&camera.GetFront());
    DirectX::XMVECTOR CameraPosition = DirectX::XMLoadFloat3(&camera.GetEye());


    //ライトからの位置から見たビュー・プロジェクション行列
	DirectX::XMVECTOR LightPosition = DirectX::XMLoadFloat4(&directional_light_direction);
	LightPosition = DirectX::XMVectorScale(LightPosition, -50.0f);
    DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(LightPosition,
        DirectX::XMVectorSet(camera.GetFocus().x, camera.GetFocus().y, camera.GetFocus().z, 1.0f),
		DirectX::XMVectorSet(camera.GetUp().x, camera.GetUp().y, camera.GetUp().z, 0.0f));
    

    // カスケード分割距離テーブル
    static constexpr float SplitAreaTable[] = {
        0.1f,
        10.0f,   // 近景：カメラ極近距離
        30.0f,   // 中景1
        80.0f,   // 中景2
        200.0f,  // 遠景
    };

	static constexpr float fov_y = DirectX::XMConvertToRadians(45);
    float aspect_ratio = static_cast<float>(Graphics::Instance().GetScreenWidth()) / Graphics::Instance().GetScreenHeight();

    // SRVのバインドを事前に解除
    ID3D11ShaderResourceView* nullSRVs[ShadowBufferSize] = {};
    dc->PSSetShaderResources(21, ShadowBufferSize, nullSRVs);  // slot番号は実際に使っているものに合わせる

    for (int index = 0; index < ShadowBufferSize; ++index)
    {
        //シャドウマップ用の深度バッファに設定
		dc->ClearDepthStencilView(cascade_shadowmap_depth_stencil_views[index].Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		dc->OMSetRenderTargets(0, nullptr, cascade_shadowmap_depth_stencil_views[index].Get());

        float near_depth = SplitAreaTable[index];
        float far_depth = SplitAreaTable[index + 1];

        //エリアを内包する視推台の8頂点を算出する
		DirectX::XMVECTOR vertex[8];
        {
            //	エリアの近平面の中心からの上面までの距離を求める
            float	nearY = tanf(fov_y * 0.5f) * near_depth;

            //	エリアの近平面の中心からの右面までの距離を求める
            float	nearX = nearY * aspect_ratio;

            //	エリアの遠平面の中心からの上面までの距離を求める
            float	farY = tanf(fov_y * 0.5f) * far_depth;

            //	エリアの遠平面の中心からの右面までの距離を求める
            float	farX = farY * aspect_ratio;

            //	エリアの近平面の中心座標を求める
            DirectX::XMVECTOR	NearPosition = DirectX::XMVectorAdd(CameraPosition,
                DirectX::XMVectorScale(CameraFront, near_depth));

            //	エリアの遠平面の中心座標を求める
            DirectX::XMVECTOR	FarPosition = DirectX::XMVectorAdd(CameraPosition,
                DirectX::XMVectorScale(CameraFront, far_depth));

            //8頂点を求める
            {
				//	近平面の右上
                vertex[0] = DirectX::XMVectorAdd(NearPosition,
                    DirectX::XMVectorAdd(
                        DirectX::XMVectorScale(CameraUp, nearY),
                        DirectX::XMVectorScale(CameraRight, nearX)));

				//	近平面の左上
                vertex[1] = DirectX::XMVectorAdd(NearPosition,
                    DirectX::XMVectorAdd(
                        DirectX::XMVectorScale(CameraUp, nearY),
						DirectX::XMVectorScale(CameraRight, -nearX)));

                //	近平面の右下
                vertex[2] = DirectX::XMVectorAdd(NearPosition,
                    DirectX::XMVectorAdd(
						DirectX::XMVectorScale(CameraUp, -nearY),
						DirectX::XMVectorScale(CameraRight, nearX)));

                //	近平面の左下
                vertex[3] = DirectX::XMVectorAdd(NearPosition,
                    DirectX::XMVectorAdd(
						DirectX::XMVectorScale(CameraUp, -nearY),
						DirectX::XMVectorScale(CameraRight, -nearX)));

                //	遠平面の右上
                vertex[4] = DirectX::XMVectorAdd(FarPosition,
					DirectX::XMVectorAdd(
						DirectX::XMVectorScale(CameraUp, farY),
                        DirectX::XMVectorScale(CameraRight, farX)));
                //	遠平面の左上
                vertex[5] = DirectX::XMVectorAdd(FarPosition,
                    DirectX::XMVectorAdd(
                        DirectX::XMVectorScale(CameraUp, farY),
                        DirectX::XMVectorScale(CameraRight, -farX)));
                //	遠平面の右下
                vertex[6] = DirectX::XMVectorAdd(FarPosition,
                    DirectX::XMVectorAdd(
                        DirectX::XMVectorScale(CameraUp, -farY),
                        DirectX::XMVectorScale(CameraRight, farX)));
                //	遠平面の左下
                vertex[7] = DirectX::XMVectorAdd(FarPosition,
                    DirectX::XMVectorAdd(
                        DirectX::XMVectorScale(CameraUp, -farY),
						DirectX::XMVectorScale(CameraRight, -farX)));
            }
        }

		//8頂点をライトビュー空間に変換して、最大値・最小値を求める
        float lsMinZ = FLT_MAX, lsMaxZ = -FLT_MAX;
        for (auto& it : vertex)
        {
            DirectX::XMFLOAT3 p;
            DirectX::XMStoreFloat3(&p, DirectX::XMVector3TransformCoord(it, V));
            lsMinZ = min(p.z, lsMinZ);
            lsMaxZ = max(p.z, lsMaxZ);
        }
        lsMinZ = max(0.1f, lsMinZ - 50.0f); // 影キャスターが範囲外にいても拾えるよう手前に延長
        lsMaxZ += 50.0f;

        DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicLH(10000.0f, 10000.0f, lsMinZ, lsMaxZ);
        DirectX::XMMATRIX LVP = V * P;

		DirectX::XMFLOAT2 vertex_min(FLT_MAX, FLT_MAX),vertex_max(-FLT_MAX, -FLT_MAX);

        for (auto& it : vertex)
        {
            DirectX::XMFLOAT3	p;
            DirectX::XMStoreFloat3(&p, DirectX::XMVector3TransformCoord(it, LVP));

            vertex_min.x = min(p.x, vertex_min.x);
            vertex_min.y = min(p.y, vertex_min.y);
            vertex_max.x = max(p.x, vertex_max.x);
            vertex_max.y = max(p.y, vertex_max.y);

        }

        //クロップ行列を求める
		DirectX::XMMATRIX ClopMatrix = DirectX::XMMatrixIdentity();
        {
            float	xScale = 2.0f / (vertex_max.x - vertex_min.x);
            float	yScale = 2.0f / (vertex_max.y - vertex_min.y);
            float	xOffset = -0.5f * (vertex_max.x + vertex_min.x) * xScale;
            float	yOffset = -0.5f * (vertex_max.y + vertex_min.y) * yScale;
            DirectX::XMFLOAT4X4	clopMatrix;
            DirectX::XMStoreFloat4x4(&clopMatrix, ClopMatrix);
            clopMatrix._11 = xScale;
            clopMatrix._22 = yScale;
            clopMatrix._41 = xOffset;
            clopMatrix._42 = yOffset;
            ClopMatrix = DirectX::XMLoadFloat4x4(&clopMatrix);

        }

        //ライトビュープロジェクション行列にクロップ行列を乗算
        DirectX::XMFLOAT4X4 light_view_projection;
		DirectX::XMStoreFloat4x4(&light_view_projection, LVP* ClopMatrix);

        //カスケード用定数バッファに納入
		
		cascade_shadow_constant.light_view_projection[index] = light_view_projection;

        //定数バッファの更新
        {
			static constexpr int SceneCBVIndex = 1;
			scene_constants scene{};
			scene.camera_position.x = cameraPosition.x;
			scene.camera_position.y = cameraPosition.y;
			scene.camera_position.z = cameraPosition.z;
			
			scene.view_projection = light_view_projection;
			
			dc->UpdateSubresource(cascade_shadowmap_constant_buffer.Get(), 0, 0, &scene, 0, 0);
			dc->VSSetConstantBuffers(SceneCBVIndex, 1, cascade_shadowmap_constant_buffer.GetAddressOf());
			dc->PSSetConstantBuffers(SceneCBVIndex, 1, cascade_shadowmap_constant_buffer.GetAddressOf());
        }

		//モデルの描画
        stage::Instance().render(rc, modelRenderer);
        // プレイヤー・ピッチャーの描画(カリングなしで両面描画)
        //dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
        // ピッチャーの描画
        Pitcher::Instance().Render(rc, modelRenderer);
        // プレイヤーの描画
		Player::Instance().Render(rc, modelRenderer);
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
        scene_viewport.Width = static_cast<float>(Graphics::Instance().GetScreenWidth());
        scene_viewport.Height = static_cast<float>(Graphics::Instance().GetScreenHeight());
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

        luminance_extract_pass_sprite->render(dc, 0, 0, Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight());

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

        float color[4] = { 0, 0, 0, 1 }; // 黒
        dc->ClearRenderTargetView(bokeh_luminance_extract_render_target_view.Get(),color);
        dc->OMSetRenderTargets(1, bokeh_luminance_extract_render_target_view.GetAddressOf(), nullptr);

    }
    //ビューポートの設定
    {
        D3D11_VIEWPORT scene_viewport{};
        scene_viewport.TopLeftX = 0;
        scene_viewport.TopLeftY = 0;
        scene_viewport.Width = static_cast<float>(Graphics::Instance().GetScreenWidth());
        scene_viewport.Height = static_cast<float>(Graphics::Instance().GetScreenHeight());
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
        bokeh_luminance_extract_pass_sprite->render(dc, 0, 0, Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight());
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
#ifdef _DEBUG

#ifdef USE_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    const float W = io.DisplaySize.x;
    const float H = io.DisplaySize.y;

    // ── パネル幅・高さ定数 ──────────────────────────────
    const float LEFT_W = 300.0f;   // 左パネル（Player / Pitcher）
    const float RIGHT_W = 320.0f;   // 右パネル（Debug）
    const float BOTTOM_H = 250.0f;   // 下パネル（Console）
    const float CENTER_W = W - LEFT_W - RIGHT_W;
    const float CENTER_H = H - BOTTOM_H;

    // ウィンドウフラグ共通（移動・リサイズ・折りたたみ禁止）
    const ImGuiWindowFlags FIXED =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;
        

    // ════════════════════════════════════════════════════
    //  左パネル ── Player / Pitcher
    // ════════════════════════════════════════════════════
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(LEFT_W, H));
    ImGui::Begin("## Left", nullptr, FIXED | ImGuiWindowFlags_NoTitleBar);

    // ── Player ──
    if (ImGui::CollapsingHeader("Player"))
    {
        
        Player::Instance().DrawGUI();   // 既存の DrawGUI をそのまま流用
        
    }

    ImGui::Separator();

    // ── Pitcher ──
    if (ImGui::CollapsingHeader("Pitcher"))
    {
        
        Pitcher::Instance().DrawGUI();
       
    }

    ImGui::Separator();

	// ―― Stage ――
    if (ImGui::CollapsingHeader("Stage"))
    {
        stage::Instance().DrawGUI();
	}

	ImGui::Separator();

    // ── Sky ──
    if (ImGui::CollapsingHeader("Sky & Time"))
    {
        skyRenderer.DrawGUI();
    }

    ImGui::End();

    // ════════════════════════════════════════════════════
    //  中央上 ── Game View
    // ════════════════════════════════════════════════════
    ImGui::SetNextWindowPos(ImVec2(LEFT_W, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(CENTER_W, CENTER_H));
    ImGui::Begin("Game View", nullptr,
        FIXED |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);
    {
		// ImGui::IsWindowHovered() でマウスオーバーを検知して、カメラコントローラーに伝える
        cameraController.SetIsGameViewHovered(ImGui::IsWindowHovered());

        // タイトルバー分を除いたコンテンツ領域
        ImVec2 avail = ImGui::GetContentRegionAvail();

        // アスペクト比を保ってフィット（16:9 想定）
        const float aspect = Graphics::Instance().GetScreenWidth()
            / Graphics::Instance().GetScreenHeight();
        float dispW = avail.x;
        float dispH = avail.x / aspect;
        if (dispH > avail.y) { dispH = avail.y; dispW = avail.y * aspect; }

        // センタリング
        float offX = (avail.x - dispW) * 0.5f;
        float offY = (avail.y - dispH) * 0.5f;
        ImGui::SetCursorPos(ImVec2(
            ImGui::GetCursorPosX() + offX,
            ImGui::GetCursorPosY() + offY));

        // scene_shader_resource_view = シーンのカラーバッファ SRV
        ImGui::Image(
            ImTextureRef(scene_shader_resource_view.Get()),
            ImVec2(dispW, dispH));
    }
    ImGui::End();

    // ════════════════════════════════════════════════════
    //  右パネル ── Debug
    // ════════════════════════════════════════════════════
    ImGui::SetNextWindowPos(ImVec2(LEFT_W + CENTER_W, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(RIGHT_W, H));
    ImGui::Begin("Debug", nullptr, FIXED);

    // ── Camera ──
    if (ImGui::CollapsingHeader("Camera"))
    {
        Camera& camera = Camera::Instance();
        DirectX::XMFLOAT3 eye = camera.GetEye();
        DirectX::XMFLOAT3 focus = camera.GetFocus();

        if (ImGui::DragFloat3("Eye", &eye.x, 0.1f))
        {
            camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
            cameraController.SyncCameraToController(camera);
        }
        if (ImGui::DragFloat3("Focus", &focus.x, 0.1f))
        {
            camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
            cameraController.SyncCameraToController(camera);
        }
        ImGui::SliderFloat("Near Z", &camera_near_z, 0.1f, 100.0f);
        ImGui::SliderFloat("Far Z", &camera_far_z, 100.0f, 10000.0f);
        camera.SetPerspectiveFov(
            DirectX::XMConvertToRadians(45),
            Graphics::Instance().GetScreenWidth() / Graphics::Instance().GetScreenHeight(),
            camera_near_z, camera_far_z);
    }

    // ── Time Scale ──
    if (ImGui::CollapsingHeader("Time Control", ImGuiTreeNodeFlags_DefaultOpen))
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
            for (int i = 0; i < (int)pointLights.size(); ++i)
            {
                if (ImGui::TreeNode((std::string("point ") + std::to_string(i)).c_str()))
                {
                    ImGui::SliderFloat3("pos", &pointLights[i].position.x, -200.0f, 200.0f);
                    ImGui::ColorEdit3("color", &pointLights[i].color.x);
                    ImGui::SliderFloat("intensity", &pointLights[i].intensity, 0.0f, 100.0f);
                    ImGui::SliderFloat("range", &pointLights[i].range, 0.1f, 200.0f);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Spot Lights"))
        {
            for (int i = 0; i < (int)spotLights.size(); ++i)
            {
                if (ImGui::TreeNode((std::string("spot ") + std::to_string(i)).c_str()))
                {
                    ImGui::SliderFloat3("pos", &spotLights[i].position.x, -200.0f, 200.0f);
                    if (ImGui::SliderFloat3("dir", &spotLights[i].direction.x, -1.0f, 1.0f))
                    {
                        float len = sqrtf(
                            spotLights[i].direction.x * spotLights[i].direction.x +
                            spotLights[i].direction.y * spotLights[i].direction.y +
                            spotLights[i].direction.z * spotLights[i].direction.z);
                        if (len > 0) {
                            spotLights[i].direction.x /= len;
                            spotLights[i].direction.y /= len;
                            spotLights[i].direction.z /= len;
                        }
                    }
                    ImGui::ColorEdit3("color", &spotLights[i].color.x);
                    ImGui::SliderFloat("intensity", &spotLights[i].intensity, 0.0f, 100.0f);
                    ImGui::SliderFloat("range", &spotLights[i].range, 0.1f, 1000.0f);
                    float inner_deg = DirectX::XMConvertToDegrees(spotLights[i].innerCorn);
                    float outer_deg = DirectX::XMConvertToDegrees(spotLights[i].outerCorn);
                    if (ImGui::SliderFloat("inner", &inner_deg, 0.0f, 89.0f))
                        spotLights[i].innerCorn = DirectX::XMConvertToRadians(inner_deg);
                    if (ImGui::SliderFloat("outer", &outer_deg, 0.0f, 89.0f))
                        spotLights[i].outerCorn = DirectX::XMConvertToRadians(outer_deg);
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
        ImGui::Checkbox("Cascade Shadow", &use_cascade_shadow_map);
        if (use_cascade_shadow_map)
        {
            ImGui::Checkbox("Show Cascade Area", &cascade_shadow_constant.display_cascade_area);
            ImGui::SliderFloat("Attenuation", &cascade_shadow_constant.shadow_attenuation, 0.0f, 1.0f);
            ImGui::SliderFloat4("Bias", &cascade_shadow_constant.shadow_bias.x, 0.0f, 0.01f);
            for (int i = 0; i < ShadowBufferSize; ++i)
            {
                ImGui::Text("Cascade Map %d", i);
                ImGui::Image(ImTextureRef(cascade_shadowmap_shader_resource_views[i].Get()),
                    ImVec2(200, 200));
            }
        }
        else
        {
            ImGui::SliderFloat("Attenuation", &shadow_attenuation, 0.0f, 1.0f);
            ImGui::SliderFloat("Bias", &shadow_bias, 0.0f, 0.01f);
            ImGui::Text("Scene RT");
            ImGui::Image(ImTextureRef(scene_shader_resource_view.Get()), ImVec2(200, 112));
            ImGui::Text("Shadow Map");
            ImGui::Image(ImTextureRef(shadowmap_shader_resource_view.Get()), ImVec2(200, 200));
        }
    }

    // ── Bloom ──
    if (ImGui::CollapsingHeader("Bloom"))
    {
        ImGui::SliderFloat("Threshold", &luminance_extract_constant.threshold, 0.0f, 2.0f);
        ImGui::SliderFloat("Intensity", &luminance_extract_constant.intensity, 0.0f, 10.0f);
        ImGui::Image(ImTextureRef(luminance_extract_shader_resource_view.Get()), ImVec2(200, 200));
        ImGui::Text("Gaussian Blur");
        ImGui::SliderInt("Kernel", &gaussian_filter_data.kernel_size, 1, KernelMax);
        ImGui::SliderFloat("Sigma", &gaussian_filter_data.sigma, 1.0f, 50.0f);
        ImGui::Image(ImTextureRef(bokeh_luminance_extract_shader_resource_view.Get()), ImVec2(200, 200));
    }

    // ── PhysX ──
    if (ImGui::CollapsingHeader("Physics"))
    {
        ImGui::Checkbox("Show PhysX Debug", &showPhysxDebug);
    }

    // ── Texture Manager ──
    if (ImGui::CollapsingHeader("Textures"))
    {
        textureManager.DrawGUI();
    }

    // ── Performance ──
    if (ImGui::CollapsingHeader("Performance"))
    {
        ImGui::Text("FPS          : %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("VS Invokes   : %llu", pipeline_stats.VSInvocations);
        ImGui::Text("Primitives   : %llu", pipeline_stats.IAPrimitives);
    }

    ImGui::End();

    // ════════════════════════════════════════════════════
    //  下パネル ── Console
    // ════════════════════════════════════════════════════
    ImGui::SetNextWindowPos(ImVec2(LEFT_W, CENTER_H));
    ImGui::SetNextWindowSize(ImVec2(CENTER_W, BOTTOM_H));
    ImGui::Begin("Console", nullptr, FIXED);
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
            if(line.find("[Hit]") != std::string::npos)
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
            inputBuf[0] = '\0';
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    ImGui::End();

#endif // USE_IMGUI

#endif // _DEBUG
}

void scene_game::SaveSetting() 
{
    json j;

	//// カメラ設定の保存
	//Camera& camera = Camera::Instance();
	//DirectX::XMFLOAT3 eye = camera.GetEye();
	//DirectX::XMFLOAT3 focus = camera.GetFocus();
	//j["camera"]["eye"] = { eye.x, eye.y, eye.z };
	//j["camera"]["focus"] = { focus.x, focus.y, focus.z };
	//j["camera"]["near_z"] = camera_near_z;
	//j["camera"]["far_z"] = camera_far_z; 

	//タイムコントロールの保存
	j["time"]["time_scale"] = timeScale;

	// ライト設定の保存
    j["light"]["ambient"] = { ambient_color.x, ambient_color.y, ambient_color.z, ambient_color.w };
    j["light"]["dir_dir"] = { directional_light_direction.x, directional_light_direction.y, directional_light_direction.z };
    j["light"]["dir_color"] = { directional_light_color.x, directional_light_color.y, directional_light_color.z };
    j["light"]["dir_intensity"] = directional_light_intensity;

    //ポイントライトの保存
    for (int i = 0; i < (int)pointLights.size(); ++i)
    {
        j["point_lights"][i]["pos"] = { pointLights[i].position.x, pointLights[i].position.y, pointLights[i].position.z };
        j["point_lights"][i]["color"] = { pointLights[i].color.x, pointLights[i].color.y, pointLights[i].color.z };
        j["point_lights"][i]["intensity"] = pointLights[i].intensity;
        j["point_lights"][i]["range"] = pointLights[i].range;
    }

	//スポットライトの保存
    for (int i = 0; i < (int)spotLights.size(); ++i)
    {
        j["spot_lights"][i]["pos"] = { spotLights[i].position.x, spotLights[i].position.y, spotLights[i].position.z };
        j["spot_lights"][i]["dir"] = { spotLights[i].direction.x, spotLights[i].direction.y, spotLights[i].direction.z };
        j["spot_lights"][i]["color"] = { spotLights[i].color.x, spotLights[i].color.y, spotLights[i].color.z };
        j["spot_lights"][i]["intensity"] = spotLights[i].intensity;
        j["spot_lights"][i]["range"] = spotLights[i].range;
        j["spot_lights"][i]["innerCorn"] = spotLights[i].innerCorn;
        j["spot_lights"][i]["outerCorn"] = spotLights[i].outerCorn;
	}

	//ヘミスフィアライトとフォグの保存
    j["hemisphere"]["sky_color"] = { sky_color.x, sky_color.y, sky_color.z };
    j["hemisphere"]["ground_color"] = { ground_color.x, ground_color.y, ground_color.z };
    j["hemisphere"]["weight"] = hemisphere_weight;
    j["fog"]["color"] = { fog_color.x, fog_color.y, fog_color.z };
    j["fog"]["near"] = fog_range.x;
    j["fog"]["far"] = fog_range.y;

	//シャドウの保存
    j["shadow"]["use_cascade"] = use_cascade_shadow_map;
    j["shadow"]["cascade_attenuation"] = cascade_shadow_constant.shadow_attenuation;
    j["shadow"]["cascade_bias"] = { cascade_shadow_constant.shadow_bias.x, cascade_shadow_constant.shadow_bias.y, cascade_shadow_constant.shadow_bias.z, cascade_shadow_constant.shadow_bias.w };
	j["shadow"]["bias"] = shadow_bias;

    // ブルームの保存
    j["bloom"]["luminance_threshold"] = luminance_extract_constant.threshold;
    j["bloom"]["luminance_intensity"] = luminance_extract_constant.intensity;
    j["bloom"]["gaussian_kernel_size"] = gaussian_filter_data.kernel_size;
	j["bloom"]["gaussian_sigma"] = gaussian_filter_data.sigma;

	//physxの保存
	j["physx"]["show_debug"] = showPhysxDebug;

	//各クラスの保存処理
	Pitcher::Instance().SaveToJson(j["pitcher"]);
	Player::Instance().SaveToJson(j["player"]);
	Wind::Instance().SaveToJson(j["wind"]);
	Ball::Instance().SaveToJson(j["ball"]);
	skyRenderer.SaveToJson(j["sky"]);

    // ファイルに保存
    std::ofstream file("settings.json");
    file << j.dump(4);
    consoleLog.push_back("[Info] Settings saved.");
}

void scene_game::LoadSetting()
{
	std::ifstream file("settings.json");
    if(!file.is_open())
    {
        consoleLog.push_back("[Warn] No settings file found. Using defaults.");
        return;
	}

    json j;
	file >> j;

	//// カメラ設定の読み込み
 //   if(j.contains("camera"))
 //   {
 //       DirectX::XMFLOAT3 eye = { j["camera"]["eye"][0], j["camera"]["eye"][1], j["camera"]["eye"][2] };
 //       DirectX::XMFLOAT3 focus = { j["camera"]["focus"][0], j["camera"]["focus"][1], j["camera"]["focus"][2] };
 //       Camera& camera = Camera::Instance();
 //       camera.SetLookAt(eye, focus, { 0.0f, 1.0f, 0.0f });
 //       cameraController.SyncCameraToController(camera);
 //       camera_near_z = j["camera"]["near_z"];
 //       camera_far_z = j["camera"]["far_z"];
	//}

	//タイムコントロールの読み込み
    if(j.contains("time"))
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
        for (size_t i = 0; i < j["point_lights"].size() && i < pointLights.size(); ++i)
        {
            pointLights[i].position = { j["point_lights"][i]["pos"][0],   j["point_lights"][i]["pos"][1],   j["point_lights"][i]["pos"][2],   0.0f };
            pointLights[i].color = { j["point_lights"][i]["color"][0], j["point_lights"][i]["color"][1], j["point_lights"][i]["color"][2], 1.0f };
            pointLights[i].intensity = j["point_lights"][i]["intensity"];
            pointLights[i].range = j["point_lights"][i]["range"];
        }
    }

	//スポットライトの読み込み
    if (j.contains("spot_lights"))
    {
        for (size_t i = 0; i < j["spot_lights"].size() && i < spotLights.size(); ++i)
        {
            spotLights[i].position = { j["spot_lights"][i]["pos"][0], j["spot_lights"][i]["pos"][1], j["spot_lights"][i]["pos"][2], 0.0f };
            spotLights[i].direction = { j["spot_lights"][i]["dir"][0], j["spot_lights"][i]["dir"][1], j["spot_lights"][i]["dir"][2], 0.0f };
            spotLights[i].color = { j["spot_lights"][i]["color"][0], j["spot_lights"][i]["color"][1], j["spot_lights"][i]["color"][2], 1.0f };
            spotLights[i].intensity = j["spot_lights"][i]["intensity"];
            spotLights[i].range = j["spot_lights"][i]["range"];
            spotLights[i].innerCorn = j["spot_lights"][i]["innerCorn"];
            spotLights[i].outerCorn = j["spot_lights"][i]["outerCorn"];
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
        use_cascade_shadow_map = j["shadow"]["use_cascade"];
        cascade_shadow_constant.shadow_attenuation = j["shadow"]["cascade_attenuation"];
        cascade_shadow_constant.shadow_bias = { j["shadow"]["cascade_bias"][0], j["shadow"]["cascade_bias"][1], j["shadow"]["cascade_bias"][2], j["shadow"]["cascade_bias"][3] };
        shadow_bias = j["shadow"]["bias"];
	}

	// ブルームの読み込み
    if (j.contains("bloom"))
    {
        luminance_extract_constant.threshold = j["bloom"]["luminance_threshold"];
        luminance_extract_constant.intensity = j["bloom"]["luminance_intensity"];
        gaussian_filter_data.kernel_size = j["bloom"]["gaussian_kernel_size"];
        gaussian_filter_data.sigma = j["bloom"]["gaussian_sigma"];
	}

	//physxの読み込み
    if (j.contains("physx"))
    {
        showPhysxDebug = j["physx"]["show_debug"];
	}

	//各クラスの読み込み処理
	if (j.contains("pitcher")) Pitcher::Instance().LoadFromJson(j["pitcher"]);
	if (j.contains("player")) Player::Instance().LoadFromJson(j["player"]);
	if (j.contains("wind")) Wind::Instance().LoadFromJson(j["wind"]);
    if (j.contains("ball")) Ball::Instance().LoadFromJson(j["ball"]);
	if (j.contains("sky")) skyRenderer.LoadFromJson(j["sky"]);
	consoleLog.push_back("[Info] Settings loaded.");
}