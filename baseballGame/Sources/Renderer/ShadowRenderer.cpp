#include "ShadowRenderer.h"
#include "misc.h"
#include "shader.h"
#include "Graphics.h"


void ShadowRenderer::Initialize()
{
    HRESULT hr = S_OK;

    ID3D11Device* device = Graphics::Instance().GetDevice();

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

        //シャドウマップの定数バッファの作成
        buffer_desc.ByteWidth = sizeof(shadowmap_constants);
        hr = Graphics::Instance().GetDevice()->CreateBuffer(&buffer_desc, nullptr, shadowmap_constant_buffer.GetAddressOf());
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

        create_vs_from_cso(device, ".\\resources\\shader\\shadowmap_caster_vs.cso", shadowmap_caster_vertex_shader.GetAddressOf(), shadowmap_caster_input_layout.GetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
    }
}

void ShadowRenderer::RenderSpotShadowMap(float elapsedTime)
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

        XMFLOAT3 posF3 = { spotLights[i].position.x, spotLights[i].position.y, spotLights[i].position.z };
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

void ShadowRenderer::RenderShadowMap(float elapsedTime)
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

//カスケードシャドウマップ生成関数
void ShadowRenderer::RenderCascadeShadowMap(float elapsedTime)
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

        DirectX::XMFLOAT2 vertex_min(FLT_MAX, FLT_MAX), vertex_max(-FLT_MAX, -FLT_MAX);

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
        DirectX::XMStoreFloat4x4(&light_view_projection, LVP * ClopMatrix);

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

void ShadowRenderer::SetDirectionalLight(
    const DirectX::XMFLOAT4& dir, const DirectX::XMFLOAT4& color, float intensity)
{
    directional_light_direction = dir;
    directional_light_color = color;
    directional_light_intensity = intensity;
}

void ShadowRenderer::SetCameraPosition(const DirectX::XMFLOAT3& pos)
{
    cameraPosition = pos;
}

void ShadowRenderer::BindShadowResources(ID3D11DeviceContext* dc) const
{
    shadowmap_constants sm{};
    sm.light_view_projection = light_view_projection;
    sm.shadow_attenuation = shadow_attenuation;
    sm.shadow_bias = shadow_bias;
    sm.use_cascade = use_cascade_shadow_map;
    dc->UpdateSubresource(shadowmap_constant_buffer.Get(), 0, 0, &sm, 0, 0);
    // b6: shadowmap_constants（通常シャドウのバイアス・attenuation・use_cascade フラグ）
    dc->VSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(6, 1, shadowmap_constant_buffer.GetAddressOf());

    // b7: spot_shadowmap_constants
    dc->UpdateSubresource(spot_shadowmap_constant_buffer.Get(), 0, 0, &spot_shadow_constant, 0, 0);
    dc->VSSetConstantBuffers(7, 1, spot_shadowmap_constant_buffer.GetAddressOf());
    dc->PSSetConstantBuffers(7, 1, spot_shadowmap_constant_buffer.GetAddressOf());


    // cascade or 通常シャドウ
    if (use_cascade_shadow_map)
    {
        dc->UpdateSubresource(cascade_shadowmap_constant_buffer.Get(), 0, 0,
            &cascade_shadow_constant, 0, 0);
        dc->PSSetConstantBuffers(8, 1, cascade_shadowmap_constant_buffer.GetAddressOf());
        for (int i = 0; i < ShadowBufferSize; ++i)
            dc->PSSetShaderResources(20 + i, 1, cascade_shadowmap_shader_resource_views[i].GetAddressOf());
    }
    else
    {
        dc->PSSetShaderResources(10, 1, shadowmap_shader_resource_view.GetAddressOf());
    }
    // スポットシャドウ
    for (int i = 0; i < SpotShadowCount; ++i)
        dc->PSSetShaderResources(30 + i, 1, spot_shadowmap_shader_resource_views[i].GetAddressOf());

    dc->PSSetSamplers(10, 1, shadowmap_sampler_state.GetAddressOf());
}

void ShadowRenderer::UnbindShadowResources(ID3D11DeviceContext* dc) const
{
    // cascade or 通常シャドウ
    if (use_cascade_shadow_map)
    {
        ID3D11ShaderResourceView* nullSRVs[ShadowBufferSize] = {};
        dc->PSSetShaderResources(20, ShadowBufferSize, nullSRVs);
    }
    else
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        dc->PSSetShaderResources(10, 1, &nullSRV);
    }
    // スポットシャドウ
    ID3D11ShaderResourceView* nullSRVs[SpotShadowCount] = {};
    dc->PSSetShaderResources(30, SpotShadowCount, nullSRVs);
}

ID3D11ShaderResourceView* ShadowRenderer::GetShadowmapSRV() const
{
    return shadowmap_shader_resource_view.Get();
}

ID3D11ShaderResourceView* ShadowRenderer::GetCascadeShadowmapSRV(int index) const
{
    return cascade_shadowmap_shader_resource_views[index].Get();
}