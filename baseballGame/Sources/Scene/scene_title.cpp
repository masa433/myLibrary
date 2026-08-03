#include "scene_title.h"
#include "camera.h"
#include "Graphics.h"
#include "RenderContext.h"
#include "misc.h"
#include "stage.h"
#include "input.h"
#include "sceneTransition.h"
#include <fstream>
#include <string>
#include "devmidi.h"
#include <shader.h>


void scene_title::initialize()
{
	HRESULT hr = S_OK;
	ID3D11Device* device = Graphics::Instance().GetDevice();

    // カメラ設定をここに移動
    float screenWidth = Graphics::Instance().GetScreenWidth();
    float screenHeight = Graphics::Instance().GetScreenHeight();

    Camera& camera = Camera::Instance();
    camera.SetPerspectiveFov(
        DirectX::XMConvertToRadians(45.0f),
        screenWidth / screenHeight,
        camera_near_z,
        camera_far_z
    );

    cameraController.SetEyeAndFocus(
        { 0.0f, 5.0f, -15.0f },   // eye
        { 0.0f, 1.0f, 10.0f }     // focus（ステージのどこを見せたいか）
    );
    cameraController.SetFov(DirectX::XMConvertToRadians(45.0f));
    cameraController.SyncControllerToCamera(camera);

	//	定数バッファ作成
	{
		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		buffer_desc.ByteWidth = sizeof(scene_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = sizeof(light_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, light_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = sizeof(hemisphere_light_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, hemisphere_light_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = sizeof(fog_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, fog_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = sizeof(post_effect_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, post_effect_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = sizeof(shadow_quality_constants);
		hr = device->CreateBuffer(&buffer_desc, nullptr, shadow_quality_constant_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}

	//スカイレンダラーの初期化
	{
		skyRenderer.Initialize(device);

		skyRenderer.auto_advance_time = false;
		skyRenderer.time_of_day = 14.0f; // 初期の昼間設定（14時）
	}

	//シャドウレンダラーの初期化
	shadowRenderer.Initialize();

	//シーン描画用のバッファ生成
	{
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
		hr = device->CreateTexture2D(&texture2d_desc, nullptr, color_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//レンダーターゲットビュー生成
		hr = device->CreateRenderTargetView(color_buffer.Get(), nullptr, scene_render_target_view.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		//シェーダーリソースビュー生成
		hr = device->CreateShaderResourceView(color_buffer.Get(), nullptr, scene_shader_resource_view.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}

	//ブルームレンダラー初期化
	bloomRenderer.Initialize(device, scene_shader_resource_view.Get(), Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight());


	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	logoSpriteData = std::make_unique<SpriteData>();
	logoSpriteData->texturePath = L".\\resources\\textures\\titleLogo.png";
	logoSpriteData->position = { logoPosition.x, logoPosition.y };
	logoSpriteData->size = logoSize;
	logoSpriteData->rotation = 0.0f;
	logoSpriteData->color = logoColor;
	logoSprite = std::make_unique<sprite>(device, context, logoSpriteData->texturePath.c_str());

	//	ステージ初期化
	Physics::Instance().Initialize();
	stage::Instance().initialize();

	hexTransitionEffect.Initialize();
	isChangingScene = false;

	buttonManager.Initialize();

	devmidiInit();

	LoadSetting();
}

void scene_title::update(float elapsed_time)
{
	// Ctrl + S で設定保存
	ImGuiIO& io = ImGui::GetIO();
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		SaveSetting();
	}

	Camera& camera = Camera::Instance();
	float screenWidth = static_cast<float>(Graphics::Instance().GetScreenWidth());
	float screenHeight = static_cast<float>(Graphics::Instance().GetScreenHeight());

	cameraController.Update(elapsed_time);
	cameraController.SyncControllerToCamera(camera);
	camera.SetPerspectiveFov(
		cameraController.GetCurrentFov(),
		screenWidth / screenHeight,
		camera_near_z, camera_far_z
	);
	cameraPosition = camera.GetEye();

	Physics::Instance().Update(elapsed_time);

	stage::Instance().update(elapsed_time);

	//	スカイレンダラー更新
	skyRenderer.Update(elapsed_time);
	//	太陽光の方向を取得してライト定数バッファに反映
	directional_light_direction = skyRenderer.GetSunDirectionToLight();

	//	シャドウレンダラー更新
	shadowRenderer.SetDirectionalLight(directional_light_direction, directional_light_color, directional_light_intensity);


	buttonManager.Update(elapsed_time);


	if(!isChangingScene)
	{
		if (buttonManager.IsStartRequested())
		{
			isChangingScene = true;
			hexTransitionEffect.Start(1.0f);
			buttonManager.ResetStartRequest(false);
		}
	}
	else
	{
		hexTransitionEffect.Update(elapsed_time);

		if (hexTransitionEffect.IsFinished())
		{
			ChangeSceneTitleToBatterSelect();
		}
	}
	
	devmidiUpdate();
}

void scene_title::render(float elapsed_time)
{
	using namespace DirectX;

	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;

	Camera& camera = Camera::Instance();

	shadowRenderer.SetCameraPosition(cameraPosition);


	// 3本の描画呼び出し（関数名だけ変わる）
	if (enableShadows)
	{
		if (shadowRenderer.use_cascade_shadow_map)
			shadowRenderer.RenderCascadeShadowMap(elapsed_time);
		else
			shadowRenderer.RenderShadowMap(elapsed_time);

		/*shadowRenderer.spot_shadow_frame_count++;
		if (shadowRenderer.spot_shadow_frame_count >= shadowRenderer.spot_shadow_update_interval)
		{
			shadowRenderer.RenderSpotShadowMap(elapsed_time);
			shadowRenderer.spot_shadow_frame_count = 0;

		}*/
	}

	//	オフスクリーンに描画する
	float clear_color[4] = { 0.2f, 0.4f, 0.6f, 1.0f };
	
	dc->ClearRenderTargetView(scene_render_target_view.Get(), clear_color);
	dc->ClearDepthStencilView(Graphics::Instance().GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	dc->OMSetRenderTargets(1, scene_render_target_view.GetAddressOf(), Graphics::Instance().GetDepthStencilView());

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(Graphics::Instance().GetScreenWidth());
	viewport.Height = static_cast<float>(Graphics::Instance().GetScreenHeight());
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &viewport);

	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	//	scene_constants
	XMMATRIX V = XMLoadFloat4x4(&camera.GetView());
	XMMATRIX P = XMLoadFloat4x4(&camera.GetProjection());

	scene_constants scene{};
	scene.camera_position = { cameraPosition.x, cameraPosition.y, cameraPosition.z, 1.0f };
	XMStoreFloat4x4(&scene.view_projection, V * P);
	XMFLOAT3 right = camera.GetRight();
	XMFLOAT3 up = camera.GetUp();
	scene.camera_right = { right.x, right.y, right.z, 0.0f };
	scene.camera_up = { up.x, up.y, up.z, 0.0f };
	dc->UpdateSubresource(constant_buffer.Get(), 0, 0, &scene, 0, 0);
	dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
	dc->PSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());

	//	light_constants（ポイント・スポットは0件のまま）
	light_constants lightConstants{};
	lightConstants.ambient_color = ambient_color;
	lightConstants.directional_light_direction = directional_light_direction;
	lightConstants.directional_light_color = directional_light_color;
	lightConstants.directional_light_intensity = directional_light_intensity;
	dc->UpdateSubresource(light_constant_buffer.Get(), 0, 0, &lightConstants, 0, 0);
	dc->VSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());
	dc->PSSetConstantBuffers(3, 1, light_constant_buffer.GetAddressOf());

	//	hemisphere
	hemisphere_light_constants hemi{};
	hemi.sky_color = sky_color;
	hemi.ground_color = ground_color;
	hemi.hemisphere_weight.x = hemisphere_weight;
	dc->UpdateSubresource(hemisphere_light_constant_buffer.Get(), 0, 0, &hemi, 0, 0);
	dc->VSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());
	dc->PSSetConstantBuffers(4, 1, hemisphere_light_constant_buffer.GetAddressOf());

	//	fog（遠くに設定して実質見えなくしてある）
	fog_constants fog{};
	fog.fog_color = fog_color;
	fog.fog_range = fog_range;
	dc->UpdateSubresource(fog_constant_buffer.Get(), 0, 0, &fog, 0, 0);
	dc->VSSetConstantBuffers(5, 1, fog_constant_buffer.GetAddressOf());
	dc->PSSetConstantBuffers(5, 1, fog_constant_buffer.GetAddressOf());

	//	post effect / shadow quality（どちらも無効値のまま）
	dc->UpdateSubresource(post_effect_constant_buffer.Get(), 0, 0, &post_effect_constant, 0, 0);
	dc->VSSetConstantBuffers(10, 1, post_effect_constant_buffer.GetAddressOf());
	dc->PSSetConstantBuffers(10, 1, post_effect_constant_buffer.GetAddressOf());

	dc->UpdateSubresource(shadow_quality_constant_buffer.Get(), 0, 0, &shadow_quality_constant, 0, 0);
	dc->PSSetConstantBuffers(11, 1, shadow_quality_constant_buffer.GetAddressOf());

	shadowRenderer.BindShadowResources(dc);

	//	サンプラー
	ID3D11SamplerState* sampler_states[] =
	{
		renderState->GetSamplerState(SamplerState::PointClamp),
		renderState->GetSamplerState(SamplerState::LinearClamp),
		renderState->GetSamplerState(SamplerState::AnisotropicClamp),
	};
	dc->PSSetSamplers(0, ARRAYSIZE(sampler_states), sampler_states);
	dc->VSSetSamplers(0, ARRAYSIZE(sampler_states), sampler_states);

	skyRenderer.Render(dc,
		constant_buffer.Get(),
		renderState->GetDepthStencilState(DepthState::TestOnly),
		renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 深度・ラスタライザーを元に戻す
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	//	ステージ描画
	stage::Instance().render(rc, modelRenderer);

	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	buttonManager.Render();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	

	if (logoSpriteData && logoSprite)
	{
		logoSprite->render(dc,
			logoPosition.x, logoPosition.y,
			logoSize.x, logoSize.y,
			logoColor.x, logoColor.y, logoColor.z, logoColor.w,
			logoSpriteData->rotation);
	}

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

	// シャドウリソースのバインドを解除
	shadowRenderer.UnbindShadowResources(dc);

	// ここで高輝度抽出とぼかしを実行してパスのSRVを更新する
	bloomRenderer.Extract(dc, camera.GetView(), camera.GetProjection(), cameraPosition);

	//バックバッファに戻してコピー
	ID3D11RenderTargetView* backbufferRTV = Graphics::Instance().GetRenderTargetView();
	dc->OMSetRenderTargets(1, &backbufferRTV, nullptr);

	{
		ID3D11Resource* srcRes = nullptr;
		ID3D11Resource* dstRes = nullptr;
		scene_render_target_view->GetResource(&srcRes);
		backbufferRTV->GetResource(&dstRes);
		dc->CopyResource(dstRes, srcRes);
		srcRes->Release();
		dstRes->Release();
	}

	// ここでブルームの合成を行う
	bloomRenderer.Composite(dc);

	if (isChangingScene)
	{
		hexTransitionEffect.Render();
	}

	

}

void scene_title::uninitialize()
{
	stage::Instance().uninitialize();
	devmidiTerm();

	skyRenderer.Uninitialize();
	shadowRenderer.Uninitialize();
	bloomRenderer.Uninitialize();

	Physics::Instance().Finalize();

	// GPU コマンドをフラッシュしてからリソース破棄
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	if (dc) dc->Flush();

	constant_buffer.Reset();
	light_constant_buffer.Reset();
	hemisphere_light_constant_buffer.Reset();
	fog_constant_buffer.Reset();
	post_effect_constant_buffer.Reset();
	shadow_quality_constant_buffer.Reset();
	scene_render_target_view.Reset();
	scene_shader_resource_view.Reset();
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
}

void scene_title::DrawGUI()
{
#ifdef _DEBUG
#ifdef USE_IMGUI
	Camera& camera = Camera::Instance();
	DirectX::XMFLOAT3 eye = camera.GetEye();
	DirectX::XMFLOAT3 focus = camera.GetFocus();

	ImGui::Begin("Title Debug");
	ImGui::DragFloat3("Camera Eye", &eye.x, 0.1f);
	ImGui::DragFloat3("Camera Focus", &focus.x, 0.1f);
	ImGui::ColorEdit4("Ambient", &ambient_color.x);
	ImGui::SliderFloat("Dir Intensity", &directional_light_intensity, 0.0f, 5.0f);
	ImGui::End();

	ImGui::Begin("MIDI Keyboard");
	drawMidiKeyboard("main_kb");
	ImGui::End();

	buttonManager.DrawGUI();

	if(ImGui::CollapsingHeader("Logo Sprite"))
	{
		if(logoSpriteData)
		{
			ImGui::DragFloat2("Position", &logoPosition.x, 1.0f);
			ImGui::DragFloat2("Size", &logoSize.x, 1.0f);
			ImGui::ColorEdit4("Color", &logoColor.x);
		}
	}

	if (ImGui::CollapsingHeader("Sky & Time")) { skyRenderer.DrawGUI(); }
	if (ImGui::CollapsingHeader("Bloom"))
	{
		bloomRenderer.DrawGUI();
		bloomRenderer.DrawEnableCheckbox();
	}
	ImGui::Checkbox("Enable Shadows", &enableShadows);

#endif
#endif
}

void scene_title::SaveSetting()
{
	json j;
	buttonManager.SaveToJson(j);
	// JSONをファイルに保存する処理を追加

	//ロゴ関連の保存
	j["logo"] = {
		{"position", {logoPosition.x, logoPosition.y}},
		{"size", {logoSize.x, logoSize.y}},
		{"color", {logoColor.x, logoColor.y, logoColor.z, logoColor.w}},
	};

	bloomRenderer.SaveToJson(j);
	
	// ファイルに保存
	std::ofstream file("resources\\setting\\titleSettings.json");
	file << j.dump(4);
	
}

void scene_title::LoadSetting()
{
	std::ifstream file("resources\\setting\\titleSettings.json");
	

	json j;
	file >> j;
	// JSONファイルから読み込む処理を追加
	buttonManager.LoadFromJson(j);

	//ロゴ関連の読み込み
	if (j.contains("logo"))
	{
		auto& logo = j["logo"];
		if (logo.contains("position") && logo["position"].is_array() && logo["position"].size() == 2)
		{
			logoPosition.x = logo["position"][0].get<float>();
			logoPosition.y = logo["position"][1].get<float>();
		}
		if (logo.contains("size") && logo["size"].is_array() && logo["size"].size() == 2)
		{
			logoSize.x = logo["size"][0].get<float>();
			logoSize.y = logo["size"][1].get<float>();
		}
		if (logo.contains("color") && logo["color"].is_array() && logo["color"].size() == 4)
		{
			logoColor.x = logo["color"][0].get<float>();
			logoColor.y = logo["color"][1].get<float>();
			logoColor.z = logo["color"][2].get<float>();
			logoColor.w = logo["color"][3].get<float>();
		}
	}

	bloomRenderer.LoadFromJson(j);
}