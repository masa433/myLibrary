#include "scene_game.h"
#include "gltf_model.h"
#include "camera.h"
#include <DirectXMath.h>
#include "imgui.h"
#include "stage.h"
#include "player.h"
#include "Pitcher.h"
#include "PitchingNet.h"
#include "Graphics.h"
#include "RenderContext.h"
#include "misc.h"
#include "physxManager.h"
#include "GpuResourceUtils.h"

CONST LONG SHADOWMAP_WIDTH = { 8192 };
CONST LONG SHADOWMAP_HEIGHT = { 8192 };

//scene_game::scene_game()
//{
//    // ライト設定のみコンストラクタで行う
//    DirectionalLight directionalLight;
//    directionalLight.direction = { 0, -1, 0 };
//    directionalLight.color = { 1, 1, 1 };
//    light.SetDirectionalLight(directionalLight);
//
//}

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

    //ネットの初期化
    //PitchingNet::Instance().Initialize();

    //ストライクゾーンの初期化
    strikeZoneSprite = std::make_unique<sprite>(device, L"./resources/sprite/strikeZone.png");

    // シャドウマップ
    {
        HRESULT hr = S_OK;

        device = Graphics::Instance().GetDevice();

        // バッファ生成
        GpuResourceUtils::CreateConstantBuffer(device, sizeof(ShadowMapContext), shadowMapConstantBuffer.GetAddressOf());

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer{};
        D3D11_TEXTURE2D_DESC texture2dDesc{};
        texture2dDesc.Width = SHADOWMAP_WIDTH;
        texture2dDesc.Height = SHADOWMAP_HEIGHT;
        texture2dDesc.MipLevels = 1;
        texture2dDesc.ArraySize = 1;
        texture2dDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        texture2dDesc.SampleDesc.Count = 1;
        texture2dDesc.SampleDesc.Quality = 0;
        texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
        texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        texture2dDesc.CPUAccessFlags = 0;
        texture2dDesc.MiscFlags = 0;
        hr = device->CreateTexture2D(&texture2dDesc, NULL, depthBuffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //	深度ステンシルビュー生成
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
        depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;
        hr = device->CreateDepthStencilView(depthBuffer.Get(),
            &depthStencilViewDesc,
            shadowMapDepthStencilView.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        //	シェーダーリソースビュー生成
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
        shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;
        hr = device->CreateShaderResourceView(depthBuffer.Get(),
            &shaderResourceViewDesc,
            shadowMapShaderResourceView.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        // サンプラーステートの生成
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.BorderColor[0] = FLT_MAX;
        samplerDesc.BorderColor[1] = FLT_MAX;
        samplerDesc.BorderColor[2] = FLT_MAX;
        samplerDesc.BorderColor[3] = FLT_MAX;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = device->CreateSamplerState(&samplerDesc, shadowMapSamplerState.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
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

	// ネットの更新
	//PitchingNet::Instance().Update(elapsed_time);

    // 物理システムの更新
    Physics::Instance().Update(elapsed_time);


#ifdef USE_IMGUI
    RenderContext rc;
    //Camera& camera = Camera::Instance();

    ImGui::Separator();

    // (ToT)
    ImGui::SliderFloat3("lightDirection", reinterpret_cast<float*>(&lightDirection), -1.0f, +1.0f);
    ImGui::DragFloat("shadowMapDrawRect", &SHADOWMAP_DRAWRECT, 0.1f);
    ImGui::DragFloat("shadowBias", &shadowBias, 0.0001f, 0, 1, "%.6f");
    ImGui::ColorEdit3("shadowColor", reinterpret_cast<float*>(&shadowColor));


    if (ImGui::TreeNode("texture"))
    {
        ImGui::Text("shadow_map");
        ImGui::Image(shadowMapShaderResourceView.Get(), { 256, 256 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });

        ImGui::TreePop();
    }

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
        
        if (ImGui::SliderFloat3("Light Direction", &lightDirection.x, -1.0f, 1.0f))
        {
            //light.SetDirectionalLight(dirLight);
        }
        if (ImGui::ColorEdit3("Light Color", &lightColor.x))
        {
            //light.SetDirectionalLight(dirLight);
		}
		ImGui::ColorEdit4("Ambient Color", &ambientColor.x);
    }
    // ポイントライト制御
    if (ImGui::CollapsingHeader("Point Light"))
    {
        ImGui::DragFloat3("Point Light Position", &pointLightPosition.x, 0.1f);
        ImGui::DragFloat("Point Light Range", &pointLightRange, 0.1f, 0.0f, 100.0f);
        ImGui::ColorEdit3("Point Light Color", &pointLightColor.x);
    }

    // スポットライト制御
    if (ImGui::CollapsingHeader("Spot Light"))
    {
        ImGui::DragFloat3("Spot Light Position", &spotLightPosition.x, 0.1f);
        ImGui::DragFloat("Spot Light Range", &spotLightRange, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat3("Spot Light Direction", &spotLightDirection.x, 0.1f);
        ImGui::ColorEdit3("Spot Light Color", &spotLightColor.x);
        ImGui::SliderFloat("Spot Light Inner Angle (cos)", &spotLightInnerAngle, 0.0f, 1.0f);
        ImGui::SliderFloat("Spot Light Outer Angle (cos)", &spotLightOuterAngle, 0.0f, 1.0f);
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

void scene_game::RenderShadowMap()
{
    Graphics& graphics = Graphics::Instance();
    ID3D11DeviceContext* dc = graphics.GetDeviceContext();
    ShapeRenderer* shapeRenderer = graphics.GetShapeRenderer();
    ModelRenderer* modelRenderer = graphics.GetModelRenderer();

    // 描画準備
    RenderContext rc;
    rc.deviceContext = dc;
    rc.lightDirection = lightDirection;	// ライト方向（下方向）
    rc.renderState = graphics.GetRenderState();

    // (ToT)←なにこれ
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	cacheRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   cacheDepthStencilView;
    dc->OMGetRenderTargets(1, cacheRenderTargetView.GetAddressOf(), cacheDepthStencilView.GetAddressOf());

    // シャドウマップ
    dc->ClearDepthStencilView(shadowMapDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    dc->OMSetRenderTargets(0, nullptr, shadowMapDepthStencilView.Get());
	dc->PSSetShader(nullptr, nullptr, 0);

    // ビューポートの設定
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(SHADOWMAP_WIDTH);
    viewport.Height = static_cast<float>(SHADOWMAP_HEIGHT);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    dc->RSSetViewports(1, &viewport);

    {
        Camera& camera = Camera::Instance();


        // ライトの位置から見た視線行列を生成
        DirectX::XMVECTOR LightPosition = DirectX::XMLoadFloat3(&lightDirection); // (ToT)
        LightPosition = DirectX::XMVectorScale(LightPosition, -50);
        DirectX::XMFLOAT3 focus = Player::Instance().GetPosition();
        DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(
            LightPosition,
            DirectX::XMVectorSet(focus.x, focus.y, focus.z, 1.0f),
            DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        // シャドウマップに描画したい範囲の射影行列を生成
        DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicLH(SHADOWMAP_DRAWRECT, SHADOWMAP_DRAWRECT,
            0.1f, 200.0f);

        // (ToT)
        DirectX::XMStoreFloat4x4(&rc.view, V);
        DirectX::XMStoreFloat4x4(&rc.projection, P);


        // 定数バッファの更新
        {
            // (ToT)
            ShadowMapContext shadowMapContext;
            DirectX::XMStoreFloat4x4(&shadowMapContext.lightViewProjection, V * P);
            shadowMapContext.shadowColor = shadowColor;
            shadowMapContext.shadowBias = shadowBias;
            dc->UpdateSubresource(shadowMapConstantBuffer.Get(), 0, 0, &shadowMapContext, 0, 0);
            dc->VSSetConstantBuffers(8/*TODO*/, 1, shadowMapConstantBuffer.GetAddressOf());
            dc->PSSetConstantBuffers(8/*TODO*/, 1, shadowMapConstantBuffer.GetAddressOf());
        }

        //{
        //    // カメラ情報・scene_constantsの更新
        //    Camera& camera = Camera::Instance();
        //    scene_constants scene_data;
        //    DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&camera.GetView());
        //    DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&camera.GetProjection());
        //    DirectX::XMStoreFloat4x4(&scene_data.view_projection, V * P);
        //    scene_data.light_direction = DirectX::XMFLOAT4(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
        //    DirectX::XMFLOAT3 eye = camera.GetEye();
        //    scene_data.camera_position = DirectX::XMFLOAT4(eye.x, eye.y, eye.z, 1.0f);

        //    dc->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &scene_data, 0, 0);
        //    dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
        //    dc->PSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
        //}
    }

    // 3Dモデル描画
    {
        //ステージ描画
        stage::Instance().render(rc, modelRenderer);//RenderContextを通じてカメラの情報を渡す

        //プレイヤー描画
        Player::Instance().Render(rc, modelRenderer);

		//ピッチャー描画
		Pitcher::Instance().Render(rc, modelRenderer);

		//ネット描画
		//PitchingNet::Instance().Render(rc, modelRenderer);
    }


    // (ToT)
    dc->OMSetRenderTargets(1, cacheRenderTargetView.GetAddressOf(), cacheDepthStencilView.Get());

    // ビューポートをスクリーンサイズにリセット
    D3D11_VIEWPORT screenViewport{};
    screenViewport.TopLeftX = 0;
    screenViewport.TopLeftY = 0;
    screenViewport.Width = Graphics::Instance().GetScreenWidth();
    screenViewport.Height = Graphics::Instance().GetScreenHeight();
    screenViewport.MinDepth = 0.0f;
    screenViewport.MaxDepth = 1.0f;
    dc->RSSetViewports(1, &screenViewport);
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
    rc.lightDirection = lightDirection;
	rc.lightColor = lightColor;
	rc.ambientColor = ambientColor;

    //// ポイントライト設定
    //rc.pointLightPosition = pointLightPosition;
    //rc.pointLightRange = pointLightRange;
    //rc.pointLightColor = pointLightColor;

    //// スポットライト設定
    //rc.spotLightPosition = spotLightPosition;
    //rc.spotLightRange = spotLightRange;
    //rc.spotLightDirection = spotLightDirection;
    //rc.spotLightInnerAngle = spotLightInnerAngle;
    //rc.spotLightColor = spotLightColor;
    //rc.spotLightOuterAngle = spotLightOuterAngle;

    //カメラパラメータ設定
    Camera& camera = Camera::Instance();
    rc.view = camera.GetView();
    rc.projection = camera.GetProjection();

    cameraPosition = camera.GetEye();
    rc.cameraPosition.x = cameraPosition.x;
    rc.cameraPosition.y = cameraPosition.y;
    rc.cameraPosition.z = cameraPosition.z;

    dc->PSSetShaderResources(8, 1, shadowMapShaderResourceView.GetAddressOf());
    dc->PSSetSamplers(8, 1, shadowMapSamplerState.GetAddressOf());

    //	モデルクラスでのラスタライザーステート設定をきったからここで設定する
    rc.deviceContext->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

    // プレイヤーの描画
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
    

    

    // ピッチャーの描画
    Pitcher::Instance().Render(rc,modelRenderer);

    Player::Instance().Render(rc, modelRenderer);

	//PitchingNet::Instance().Render(rc, modelRenderer);

    // ステージの描画
    stage::Instance().render(rc, modelRenderer);

    // レンダーステート設定
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
	shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection(), rc.lightDirection);

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

    /*DirectionalLight dirLight = light.GetDirectionalLight();
    scene_data.light_direction = XMFLOAT4(dirLight.direction.x, dirLight.direction.y, dirLight.direction.z, 0.0f);*/

    scene_data.light_direction = DirectX::XMFLOAT4(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);

    XMFLOAT3 eye = camera.GetEye();
    scene_data.camera_position = XMFLOAT4(eye.x, eye.y, eye.z, 1.0f);

    dc->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &scene_data, 0, 0);
    dc->VSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(1, 1, constant_buffer.GetAddressOf());

    if(showPhysxDebug)
	Physics::Instance().Render(camera.GetView(), camera.GetProjection(), rc.lightDirection);

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

    ID3D11ShaderResourceView* clearShaderResourceView[] = { nullptr };
    dc->PSSetShaderResources(8, 1, clearShaderResourceView);
    dc->PSSetSamplers(8, 1, shadowMapSamplerState.GetAddressOf());
}

void scene_game::uninitialize()
{
    // 終了処理
    Player::Instance().Uninitialize();
    stage::Instance().uninitialize();
    Pitcher::Instance().Uninitialize();
	//PitchingNet::Instance().Uninitialize();
    Physics::Instance().Finalize();
}

void scene_game::DrawGUI()
{
    // プレイヤーのGUI描画
    Player::Instance().DrawGUI();

	// ピッチャーのGUI描画
	Pitcher::Instance().DrawGUI();

	// ピッチングネットのGUI描画
	//PitchingNet::Instance().DrawGUI();
}