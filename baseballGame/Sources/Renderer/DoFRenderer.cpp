#include "pch.h"
#include "DoFRenderer.h"
#include "shader.h"
#include "misc.h"

void DoFRenderer::Initialize(ID3D11Device* device, ID3D11ShaderResourceView* sceneColorSRV, UINT screenWidth, UINT screenHeight)
{
    if(!device || !sceneColorSRV)
    {
        throw std::invalid_argument("Invalid device or sceneColorSRV");
	}
	HRESULT hr = S_OK;
	screen_width = screenWidth;
	screen_height = screenHeight;

	//ガウスフィルター情報の初期化
	gaussian_filter_data.texture_size = { static_cast<float>(screenWidth), static_cast<float>(screenHeight) };  


    //定数バッファの生成
    {
        D3D11_BUFFER_DESC buffer_desc{};
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = 0;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;

		//シーン用定数バッファ
        buffer_desc.ByteWidth = sizeof(scene_constants);
        hr = device->CreateBuffer(&buffer_desc, nullptr, scene_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //ガウスフィルター用定数バッファ(被写界深度)    
        buffer_desc.ByteWidth = sizeof(gaussian_filter_constants);
        hr = device->CreateBuffer(&buffer_desc, nullptr, gaussian_filter_constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //被写界深度用の定数バッファ
        buffer_desc.ByteWidth = sizeof(depth_of_field_constants);
        hr = device->CreateBuffer(&buffer_desc, nullptr, depth_of_field_constant_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        
    }

    //シーンぼかし用バッファ生成(被写界深度)
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = Graphics::Instance().GetScreenWidth();
        desc.Height = Graphics::Instance().GetScreenHeight();
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> color_buffer{};
        hr = device->CreateTexture2D(&desc, nullptr, color_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // レンダーターゲットビューの作成
        hr = device->CreateRenderTargetView(color_buffer.Get(), nullptr, bokeh_scene_render_target_view.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // シェーダーリソースビューの作成
        hr = device->CreateShaderResourceView(color_buffer.Get(), nullptr, bokeh_scene_shader_resource_view.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

    }

    //シーン深度バッファ生成(被写界深度)
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = Graphics::Instance().GetScreenWidth();
        desc.Height = Graphics::Instance().GetScreenHeight();
        desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer{};
        hr = device->CreateTexture2D(&desc, nullptr, depth_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // 深度ステンシルビューの作成
        D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
        dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Texture2D.MipSlice = 0;
        hr = device->CreateDepthStencilView(depth_buffer.Get(), &dsv_desc, scene_depth_stencil_view.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // シェーダーリソースビューの作成
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.MipLevels = 1;
        hr = device->CreateShaderResourceView(depth_buffer.Get(), &srv_desc, scene_depth_shader_resource_view.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    }

    //スプライトシェーダーの生成
    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[]
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", sprite_vertex_shader.ReleaseAndGetAddressOf(), sprite_input_layout.ReleaseAndGetAddressOf(),
            input_element_desc, _countof(input_element_desc));
        create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", sprite_pixel_shader.ReleaseAndGetAddressOf());

        //ガウスフィルター用のシェーダー準備(被写界深度)
        create_ps_from_cso(device, ".\\resources\\shader\\gaussian_filtering_ps.cso", gaussian_filter_pixel_shader.ReleaseAndGetAddressOf());
        bokeh_scene_sprite = std::make_unique<sprite>(device, sceneColorSRV);

        create_ps_from_cso(device, ".\\resources\\shader\\depth_of_field_ps.cso", depth_of_field_pixel_shader.ReleaseAndGetAddressOf());
    }
}


//被写界深度
void DoFRenderer::calculate_gaussian_filter_constant(gaussian_filter_constants& constant, const gaussian_filter_datas& data)
{
    //偶数の場合は奇数に直す
    int kernel_size = data.kernel_size;
    if (kernel_size % 2 == 0)
    {
        kernel_size++;
    }

    constant.kernel_size = static_cast<float>(kernel_size);
    constant.texcel.x = 1.0f / data.texture_size.x;// テクスチャの幅の逆数
    constant.texcel.y = 1.0f / data.texture_size.y;// テクスチャの高さの逆数

    //重みを算出
    float sum = 0.0f;
    int id = 0;
    for (int y = -kernel_size / 2; y <= kernel_size / 2; y++)
    {
        for (int x = -kernel_size / 2; x <= kernel_size / 2; x++)
        {
            constant.weights[id].x = (float)x;
            constant.weights[id].y = (float)y;
            constant.weights[id].z = (float)exp(-(x * x + y * y) / (2.0f * data.sigma * data.sigma)) / (2.0f * DirectX::XM_PI * data.sigma);// ガウス関数の計算
            sum += constant.weights[id].z;// 重みの合計を計算
            id++;// インデックスを進める
        }
    }

    //平均化
    for (int i = 0; i < kernel_size * kernel_size; i++)
    {
        constant.weights[i].z /= sum;// 重みを正規化
    }

}

// 被写界深度の計算とぼかしの描画
void DoFRenderer::generate_bokeh_scene(
    float elapsedTime,
    ID3D11DeviceContext* dc,
    const DirectX::XMFLOAT4X4& view,
    const DirectX::XMFLOAT4X4& projection,
    const DirectX::XMFLOAT3& cameraPosition,
    float near_clip_distance,
    float far_clip_distance)
{

    RenderState* renderState = Graphics::Instance().GetRenderState();

    //バックバッファを指定
    {
        // 高輝度抽出用のレンダーターゲットをクリアしてセット
        float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        dc->ClearRenderTargetView(bokeh_scene_render_target_view.Get(), clear_color);
        dc->OMSetRenderTargets(1, bokeh_scene_render_target_view.GetAddressOf(), nullptr);
    }

    //ビューポートの設定
    {
        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(screen_width);
        viewport.Height = static_cast<float>(screen_height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        dc->RSSetViewports(1, &viewport);
    }

    //リソース
    {

        //定数バッファ設定
        //	定数バッファ設定
        static constexpr int SceneCBVIndex = 1;
        scene_constants scene{};
        scene.camera_position.x = cameraPosition.x;
        scene.camera_position.y = cameraPosition.y;
        scene.camera_position.z = cameraPosition.z;
		scene.camera_clip_distance.x = near_clip_distance;
		scene.camera_clip_distance.y = far_clip_distance;
		scene.camera_clip_distance.z = near_clip_distance * far_clip_distance;
		scene.camera_clip_distance.w = far_clip_distance - near_clip_distance;
        DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
        DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projection);
        DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
        dc->UpdateSubresource(scene_constant_buffer.Get(), 0, 0, &scene, 0, 0);
        dc->PSSetConstantBuffers(SceneCBVIndex, 1, scene_constant_buffer.GetAddressOf());

        //サンプラーステート設定
        static constexpr int SamplerIndex = 0;
        ID3D11SamplerState* sampler_states[] =
        {
            Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::PointClamp),        // s0: POINT
            Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::LinearClamp),  // s1: LINEAR
            Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::AnisotropicClamp),  // s2: ANISOTROPIC
        };
        dc->PSSetSamplers(SamplerIndex, ARRAYSIZE(sampler_states), sampler_states);

        //ガウシアンフィルター情報設定
        {
            gaussian_filter_constants gaussianFilterConstants{};
            calculate_gaussian_filter_constant(gaussianFilterConstants, gaussian_filter_data);

            //定数バッファ更新
            static constexpr int GaussianFilterCBVIndex = 2;
            dc->UpdateSubresource(gaussian_filter_constant_buffer.Get(), 0, 0, &gaussianFilterConstants, 0, 0);
            dc->PSSetConstantBuffers(GaussianFilterCBVIndex, 1, gaussian_filter_constant_buffer.GetAddressOf());
        }
    }

    //描画
    {
        dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
        dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
        dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

        //シェーダー設定
        dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
        dc->PSSetShader(gaussian_filter_pixel_shader.Get(), nullptr, 0);
        dc->IASetInputLayout(sprite_input_layout.Get());

        bokeh_scene_sprite->render(dc, 0, 0, static_cast<float>(screen_width), static_cast<float>(screen_height));
    }

    //描画後にリソースを解除
    {
        dc->VSSetShader(nullptr, nullptr, 0);
        dc->PSSetShader(nullptr, nullptr, 0);
        dc->IASetInputLayout(nullptr);

        ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr, nullptr };
        dc->PSSetShaderResources(0, ARRAYSIZE(nullSRV), nullSRV);
    }

 
}

void DoFRenderer::Extract(ID3D11DeviceContext* dc,
    const DirectX::XMFLOAT4X4& view,
    const DirectX::XMFLOAT4X4& projection,
    const DirectX::XMFLOAT3& cameraPosition,
    float near_clip_distance,
    float far_clip_distance)
{
    if (!enable_dof) return;

    generate_bokeh_scene(0.0f, dc, view, projection, cameraPosition, near_clip_distance, far_clip_distance);
}

void DoFRenderer::Composite(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* sceneColorSRV)
{
    if (!enable_dof) return;

    auto* renderState = Graphics::Instance().GetRenderState();

    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
   
    //シェーダー設定
    dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
    dc->PSSetShader(depth_of_field_pixel_shader.Get(), nullptr, 0);
    dc->IASetInputLayout(sprite_input_layout.Get());
    //定数バッファ設定
    static constexpr int DepthOfFieldCBVIndex = 2;
    dc->UpdateSubresource(depth_of_field_constant_buffer.Get(), 0, 0, &depth_of_field_constant, 0, 0);
    dc->PSSetConstantBuffers(DepthOfFieldCBVIndex, 1, depth_of_field_constant_buffer.GetAddressOf());
    //シェーダーリソースビュー設定
    ID3D11ShaderResourceView* srvs[] =
    {
		sceneColorSRV,
        bokeh_scene_shader_resource_view.Get(),
        scene_depth_shader_resource_view.Get()
    };
    dc->PSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
    //描画
    bokeh_scene_sprite->render(dc, 0, 0, static_cast<float>(screen_width), static_cast<float>(screen_height));
    //描画後にリソースを解除
    {
        ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr, nullptr };
        dc->PSSetShaderResources(0, ARRAYSIZE(nullSRV), nullSRV);
    }

	
}

void DoFRenderer::Uninitialize()
{
    bokeh_scene_sprite.reset();
    sprite_vertex_shader.Reset();
    sprite_pixel_shader.Reset();
    sprite_input_layout.Reset();
    gaussian_filter_pixel_shader.Reset();
    depth_of_field_pixel_shader.Reset();
    scene_constant_buffer.Reset();
    gaussian_filter_constant_buffer.Reset();
    depth_of_field_constant_buffer.Reset();
    bokeh_scene_render_target_view.Reset();
    bokeh_scene_shader_resource_view.Reset();
    scene_depth_stencil_view.Reset();
    scene_depth_shader_resource_view.Reset();
}

void DoFRenderer::DrawGUI(float near_clip_distance, float far_clip_distance)
{
    if (ImGui::CollapsingHeader("depth of field"))
    {
        ImGui::SliderInt("kernel", &gaussian_filter_data.kernel_size, 1, KernelMax);
        ImGui::SliderFloat("sigma", &gaussian_filter_data.sigma, 1.0f, 50.0f);

        ImGui::SliderFloat("focus_distance", &depth_of_field_constant.focus_distance, near_clip_distance, far_clip_distance);
        ImGui::SliderFloat("dof_range", &depth_of_field_constant.dof_range, 1.0f, far_clip_distance - near_clip_distance);

        ImGui::Image(ImTextureRef(bokeh_scene_shader_resource_view.Get()), ImVec2(256, 256));
        ImGui::Image(ImTextureRef(scene_depth_shader_resource_view.Get()), ImVec2(256, 256));
    }
}