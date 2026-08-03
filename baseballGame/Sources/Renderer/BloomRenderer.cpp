#include "BloomRenderer.h"
#include "shader.h"
#include "misc.h"
#include <imgui.h>

void BloomRenderer::Initialize(ID3D11Device* device, ID3D11ShaderResourceView* sceneColorSRV,
    UINT screenWidth, UINT screenHeight)
{
	HRESULT hr = S_OK;

	screen_width = screenWidth;
	screen_height = screenHeight;

	gaussian_filter_data.texture_size = { static_cast<float>(screen_width), static_cast<float>(screen_height) };


	//定数バッファの作成
	{
		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		//シーン定数バッファの作成
		buffer_desc.ByteWidth = sizeof(scene_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, scene_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//高輝度抽出用定数バッファの作成
		buffer_desc.ByteWidth = sizeof(luminance_extract_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, luminance_extract_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//ガウスフィルター用定数バッファの作成
		buffer_desc.ByteWidth = sizeof(gaussian_filter_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, gaussian_filter_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}

	//スプライトシェーダー準備
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
        hr = device->CreateTexture2D(&texture2d_desc, NULL, color_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	レンダーターゲットビュー生成
        hr = device->CreateRenderTargetView(color_buffer.Get(), NULL, luminance_extract_render_target_view.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	シェーダーリソースビュー生成
        hr = device->CreateShaderResourceView(color_buffer.Get(), NULL, luminance_extract_shader_resource_view.ReleaseAndGetAddressOf());
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
        hr = device->CreateTexture2D(&texture2d_desc, NULL, color_buffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	レンダーターゲットビュー生成
        hr = device->CreateRenderTargetView(color_buffer.Get(), NULL, bokeh_luminance_extract_render_target_view.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        //	シェーダーリソースビュー生成
        hr = device->CreateShaderResourceView(color_buffer.Get(), NULL, bokeh_luminance_extract_shader_resource_view.ReleaseAndGetAddressOf());
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
        create_ps_from_cso(device, ".\\resources\\shader\\luminance_extract_ps.cso", luminance_extract_pixel_shader.ReleaseAndGetAddressOf());
        luminance_extract_pass_sprite = std::make_unique<sprite>(device, sceneColorSRV);

        //	高輝度抽出バッファぼかし用
        create_ps_from_cso(device, ".\\resources\\shader\\gaussian_filtering_ps.cso", gaussian_filter_pixel_shader.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
        bokeh_luminance_extract_pass_sprite = std::make_unique<sprite>(device, luminance_extract_shader_resource_view);


        //ぼかした結果を利用するスプライト
        add_luminance_extract_pass_sprite = std::make_unique<sprite>(device, bokeh_luminance_extract_shader_resource_view);
    }
}


// ガウスフィルターの定数を計算する
void BloomRenderer::calculate_gaussian_filter_constant(gaussian_filter_constants& constant, const gaussian_filter_datas& data)
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
    for (int y = -kernel_size / 2; y <= kernel_size / 2; y++)
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
    for (int i = 0; i < KernelMax * KernelMax; i++)
    {
        constant.weights[i].z /= sum;
    }
}

// 高輝度抽出バッファをぼかす
void BloomRenderer::luminance_extract_pass(ID3D11DeviceContext* dc,
    const DirectX::XMFLOAT4X4& view,
    const DirectX::XMFLOAT4X4& projection,
    const DirectX::XMFLOAT3& cameraPosition)
{
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
        scene_viewport.Width = static_cast<float>(screen_width);
        scene_viewport.Height = static_cast<float>(screen_height);
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
        DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
        DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projection);
        DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
        dc->UpdateSubresource(scene_constant_buffer.Get(), 0, 0, &scene, 0, 0);
        dc->PSSetConstantBuffers(SceneCBVIndex, 1, scene_constant_buffer.GetAddressOf());
 
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
 
        luminance_extract_pass_sprite->render(dc, 0, 0, static_cast<float>(screen_width), static_cast<float>(screen_height));
    }
 
    //シェーダー登録解除
    {
        dc->VSSetShader(nullptr, nullptr, 0);
        dc->PSSetShader(nullptr, nullptr, 0);
        dc->IASetInputLayout(nullptr);
    }
}

// 高輝度抽出バッファをぼかす
void BloomRenderer::bokeh_luminance_extract_pass(ID3D11DeviceContext* dc,
    const DirectX::XMFLOAT4X4& view,
    const DirectX::XMFLOAT4X4& projection,
    const DirectX::XMFLOAT3& cameraPosition)
{
    //バックバッファ指定
    {

        // 高輝度抽出用のレンダーターゲットをクリアしてセット
        float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        dc->ClearRenderTargetView(bokeh_luminance_extract_render_target_view.Get(), clear_color);
        dc->OMSetRenderTargets(1, bokeh_luminance_extract_render_target_view.GetAddressOf(), nullptr);

    }

    //ビューポートの設定
    {
        D3D11_VIEWPORT scene_viewport{};
        scene_viewport.TopLeftX = 0;
        scene_viewport.TopLeftY = 0;
        scene_viewport.Width = static_cast<float>(screen_width);
        scene_viewport.Height = static_cast<float>(screen_height);
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
        
        DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
        DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projection);
        DirectX::XMStoreFloat4x4(&scene.view_projection, V * P);
        dc->UpdateSubresource(scene_constant_buffer.Get(), 0, 0, &scene, 0, 0);
        dc->PSSetConstantBuffers(SceneCBVIndex, 1, scene_constant_buffer.GetAddressOf());

        //	サンプラステート設定
        static constexpr int SamplerStateIndex = 0;
        ID3D11SamplerState* sampler_states[] =
        {
            Graphics::Instance().GetRenderState()->GetSamplerState(SamplerState::LinearClamp),

        };

        dc->PSSetSamplers(SamplerStateIndex, ARRAYSIZE(sampler_states), sampler_states);
        dc->VSSetSamplers(SamplerStateIndex, ARRAYSIZE(sampler_states), sampler_states);

        // ガウシアンフィルター情報設定
        gaussian_filter_constants gaussian_constant;
        calculate_gaussian_filter_constant(gaussian_constant, gaussian_filter_data);

        //	高輝度抽出用情報設定
        static constexpr int LuminanceExtractCBVIndex = 2;
        dc->UpdateSubresource(gaussian_filter_constant_buffer.Get(), 0, 0, &gaussian_constant, 0, 0);
        dc->PSSetConstantBuffers(LuminanceExtractCBVIndex, 1, gaussian_filter_constant_buffer.GetAddressOf());

    }

    //描画
    {
        dc->OMSetBlendState(Graphics::Instance().GetRenderState()->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
        dc->OMSetDepthStencilState(Graphics::Instance().GetRenderState()->GetDepthStencilState(DepthState::TestAndWrite), 0);
        dc->RSSetState(Graphics::Instance().GetRenderState()->GetRasterizerState(RasterizerState::SolidCullNone));

        dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
        dc->PSSetShader(gaussian_filter_pixel_shader.Get(), nullptr, 0);
        dc->IASetInputLayout(sprite_input_layout.Get());

        bokeh_luminance_extract_pass_sprite->render(dc, 0, 0, static_cast<float>(screen_width), static_cast<float>(screen_height));

    }

    //シェーダー登録解除
    {

        dc->VSSetShader(nullptr, nullptr, 0);
        dc->PSSetShader(nullptr, nullptr, 0);
        dc->IASetInputLayout(nullptr);
    }
}

// 高輝度抽出からぼかしまでの処理を行う
void BloomRenderer::Extract(ID3D11DeviceContext* dc,
    const DirectX::XMFLOAT4X4& view,
    const DirectX::XMFLOAT4X4& projection,
    const DirectX::XMFLOAT3& cameraPosition)
{
    if (!enable_bloom) return;

    luminance_extract_pass(dc, view, projection, cameraPosition);
    bokeh_luminance_extract_pass(dc, view, projection, cameraPosition);
}

// 高輝度抽出バッファを合成する
void BloomRenderer::Composite(ID3D11DeviceContext* dc)
{
    if (!enable_bloom) return;

	auto* renderState = Graphics::Instance().GetRenderState();

	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    //シェーダー設定
	dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(sprite_pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(sprite_input_layout.Get());

	add_luminance_extract_pass_sprite->render(dc, 0, 0, static_cast<float>(screen_width), static_cast<float>(screen_height));

    //シェーダー登録解除
    {
        dc->VSSetShader(nullptr, nullptr, 0);
        dc->PSSetShader(nullptr, nullptr, 0);
        dc->IASetInputLayout(nullptr);
	}
}

void BloomRenderer::DrawGUI()
{
#ifdef _DEBUG
    if (ImGui::CollapsingHeader("Bloom Setting"))
    {
        ImGui::SliderFloat("Threshold", &luminance_extract_constant.threshold, 0.0f, 2.0f);
        ImGui::SliderFloat("Intensity", &luminance_extract_constant.intensity, 0.0f, 10.0f);
        ImGui::Image(ImTextureRef(luminance_extract_shader_resource_view.Get()), ImVec2(200, 200));
        ImGui::Text("Gaussian Blur");
        ImGui::SliderInt("Kernel", &gaussian_filter_data.kernel_size, 1, KernelMax);
        ImGui::SliderFloat("Sigma", &gaussian_filter_data.sigma, 1.0f, 50.0f);
        ImGui::Image(ImTextureRef(bokeh_luminance_extract_shader_resource_view.Get()), ImVec2(200, 200));
    }
#endif
}

void BloomRenderer::DrawEnableCheckbox()
{
#ifdef _DEBUG
    ImGui::Checkbox("Enable Bloom", &enable_bloom);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Disable for better FPS");
#endif
}

void BloomRenderer::SaveToJson(nlohmann::json& j) const
{
    j["bloom"]["luminance_threshold"] = luminance_extract_constant.threshold;
    j["bloom"]["luminance_intensity"] = luminance_extract_constant.intensity;
    j["bloom"]["gaussian_kernel_size"] = gaussian_filter_data.kernel_size;
    j["bloom"]["gaussian_sigma"] = gaussian_filter_data.sigma;
    j["performance"]["enable_bloom"] = enable_bloom;
}

void BloomRenderer::LoadFromJson(const nlohmann::json& j)
{
    if (j.contains("bloom"))
    {
        luminance_extract_constant.threshold = j["bloom"]["luminance_threshold"];
        luminance_extract_constant.intensity = j["bloom"]["luminance_intensity"];
        gaussian_filter_data.kernel_size = j["bloom"]["gaussian_kernel_size"];
        gaussian_filter_data.sigma = j["bloom"]["gaussian_sigma"];
    }
    if (j.contains("performance"))
    {
        enable_bloom = j["performance"].value("enable_bloom", true);
    }
}

void BloomRenderer::Uninitialize()
{
    sprite_vertex_shader.Reset();
    sprite_pixel_shader.Reset();
    sprite_input_layout.Reset();
    luminance_extract_pixel_shader.Reset();
    luminance_extract_pass_sprite.reset();
    gaussian_filter_pixel_shader.Reset();
    bokeh_luminance_extract_pass_sprite.reset();
    add_luminance_extract_pass_sprite.reset();
    scene_constant_buffer.Reset();
    luminance_extract_constant_buffer.Reset();
    gaussian_filter_constant_buffer.Reset();
    luminance_extract_render_target_view.Reset();
    luminance_extract_shader_resource_view.Reset();
    bokeh_luminance_extract_render_target_view.Reset();
    bokeh_luminance_extract_shader_resource_view.Reset();
}