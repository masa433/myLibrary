#include "pch.h"
#include "ParticleRenderer.h"
#include "Graphics.h"
#include "camera.h"

void ParticleRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	int num_particles = 50000; // 最大のパーティクルの数

	D3D11_TEXTURE2D_DESC desc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;

	//パーティクル用のテクスチャのロード
	load_texture_from_file(device, context, L".\\resources\\textures\\particle256x256.png", shader_resource_view.ReleaseAndGetAddressOf(), &desc);

	//パーティクルシステム生成
	particles = std::make_unique<particle_system>(device, shader_resource_view, 4, 4, num_particles);

    particle_positions = {
            { 67.0f, 1.0f,  77.0f },
            { 45.0f, 1.0f, 100.0f },
            { 23.0f, 1.0f, 120.0f },
            { 13.0f, 7.0f, 125.0f },
            { 0.0f,  7.0f, -120.0f },
            { -13.0f,7.0f, 125.0f },
            { -29.0f, 7.0f, 120.0f },
            { -45.0f, 5.0f, 105.0f },
            { -55.0f, 1.0f, 90.0f },
            { -67.0f, 1.0f, 70.0f },// 10個のパーティクル位置を設定

    };

    fireworks_positions = {
        { 0.0f, 17.5f, 100.0f },
        { 30.0f, 20.0f, 90.0f },
        { -25.0f, 25.0f, 110.0f },
        { 10.0f, 22.0f, 115.0f },
        { -15.0f, 25.0f, 120.0f },

    };

    isFireworksActive = false;
    fireworksTimer = 0.0f;
    fireworksCount = 0;
    isLaunchingFireworks = false;
    loopCount = 0;
    maxLoopCount = 2;

	fireworksSound = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\FireWorks.wav", SoundType::SE);
}

void ParticleRenderer::Uninitialize()
{
    particles.reset();
	delete fireworksSound;
}

void ParticleRenderer::Update(float elapsed_time, bool isHomeRun)
{
    //パーティクル生成処理

    //	スパーク
    if (isHomeRun || ::GetAsyncKeyState('C') & 0x8000)
    {

        //1本当たりの毎秒のパーティクル数を設定
        const float rate = 1000.0f; // 1秒あたりのパーティクル数

        for (const auto& pos : particle_positions)
        {
            int count = (int)(rate * elapsed_time + (rand() % 10001) * 0.0004f);// パーティクルの数を範囲(0～4)でランダムに変化させる
            for (int i = 0; i < count; i++)
            {

                DirectX::XMFLOAT3 v = { 0,0,0 };//速度を初期化
                v.x = (rand() % 10001 - 5000) * 0.0003f;
                v.y = (rand() % 10001) * 0.0004f + 4.0f;//4.0～8.0の間でランダムに変化させる
                v.z = (rand() % 10001 - 5000) * 0.0003f;
                float t = (rand() % 10001) * 0.0001f * elapsed_time;//時間をランダムに変化させる
                DirectX::XMFLOAT3 p;
                p = { pos.x + v.x * t,pos.y + v.y * t,pos.z + v.z * t };//位置を時間に応じて変化させる

                DirectX::XMFLOAT3 f = { 0,-1.2f,0 };//重力を設定
                DirectX::XMFLOAT2 s = { 0.5f,0.5f };//サイズを設定

                float lifetime = 1.0f + (rand() % 10001) * 0.0001f * 0.5f; // ランダムな寿命を設定(1.0秒～1.5秒)

                //パーティクルの色(黄色っぽいオレンジ)
                float r = 1.0f;
                float g = 0.5f + (rand() % 10001) * 0.0001f * 0.5f; // 緑の成分をランダムに変化させる(0.5～1.0)
                float b = 0.0f;

                particles->Set(2, lifetime, p, v, f, s, false, 24.0f, r, g, b);
            }
        }
    }

    //花火
    if (isHomeRun || ::GetAsyncKeyState('X') & 0x8000)
    {
        if (!isFireworksActive)
        {
            isFireworksActive = true;
            fireworksTimer = 1.0f;//1発目をすぐに出すために1秒に設定
            fireworksCount = 0;

			//カメラのeyeの位置を取得
			const DirectX::XMFLOAT3 cameraEye = Camera::Instance().GetEye();
			//カメラの前方向のベクトルを取得
			const DirectX::XMFLOAT3 cameraFront = Camera::Instance().GetFront();

			//カメラのeyeの位置から前方向に40.0f進めた位置を花火の発射位置とする
			DirectX::XMFLOAT3 launchPosition =
            { 
                cameraEye.x + cameraFront.x * 40.0f,
                cameraEye.y + cameraFront.y * 40.0f,
                cameraEye.z + cameraFront.z * 40.0f 
            };

			//花火の発射位置をfireworks_positionsの要素に設定
			fireworks_positions.clear();//花火の発射位置をクリア
			fireworks_positions.push_back(launchPosition);//花火の発射位置を追加
			fireworks_positions.push_back({ launchPosition.x + 1.0f, launchPosition.y + 1.0f, launchPosition.z + 1.0f });
			fireworks_positions.push_back({ launchPosition.x - 1.0f, launchPosition.y - 1.0f, launchPosition.z - 1.0f });
            
           
        }

        if (isFireworksActive)
        {

            fireworksTimer += elapsed_time;

            if (fireworksTimer >= 0.5f)
            {

                fireworksTimer = 0.0f;
                fireworksCount++;
				if (fireworksSound) fireworksSound->PlayOneShot(); // 花火の音を再生

                if (!fireworks_positions.empty())
                {
                    const auto& pos = fireworks_positions[fireworksCount % fireworks_positions.size()];

                    int count = 500; // パーティクルの数を固定で500に設定
                    for (int i = 0; i < count; i++)
                    {
                        //ランダムな方向を生成するために球面座標系を使用
                        float theta = (rand() % 10001) * 0.0001f * DirectX::XM_PI * 2.0f;
                        float phi = (rand() % 10001) * 0.0001f * DirectX::XM_PI;

						float speed = (rand() % 10001) * 0.0002f + 2.0f;//速度をランダムに変化させる(2.0～4.0の間)


                        DirectX::XMFLOAT3 v = { 0,0,0 };//速度を初期化
                        v.x = sinf(phi) * cosf(theta) * speed;//球面座標系を使ってランダムな方向に速度を設定
                        v.y = cosf(phi) * speed;//-12.5～12.5の間でランダムに変化させる
                        v.z = sinf(phi) * sinf(theta) * speed;//-10.0～10.0の間でランダムに変化させる
                        float t = (rand() % 10001) * 0.0001f * elapsed_time;//時間をランダムに変化させる
                        DirectX::XMFLOAT3 p;
                        p = { pos.x + v.x * t,pos.y + v.y * t,pos.z + v.z * t };//位置を時間に応じて変化させる
                        DirectX::XMFLOAT3 f = { 0,-1.2f,0 };//重力を設定
                        DirectX::XMFLOAT2 s = { 0.1f,0.1f };//サイズを設定
                        float lifetime = (rand() % 10001) * 0.0001f + 1.0f; // ランダムな寿命を設定(1.0秒～2.0秒)
                        //パーティクルの色(赤っぽいオレンジ)
                        float r = 1.0f;
                        float g = (rand() % 10001) * 0.0001f * 0.5f; // 緑の成分をランダムに変化させる(0～0.5)
                        float b = (rand() % 10001) * 0.0001f * 0.5f; // 青の成分をランダムに変化させる(0～0.5)

                        //パーティクルを1発ずつ生成する
                        particles->Set(2, lifetime, p, v, f, s, false, 24.0f, r, g, b);

                        if (fireworksCount >= maxFireworksCount)
                        {
                            fireworksCount = 0;//花火の最大数に達したら最初の花火に戻す
                            loopCount++;//ループ回数を増やす
                        }
                        if (loopCount >= maxLoopCount)//ループ回数が最大数に達したら花火を終了する
                        {
                            isFireworksActive = false;
                            fireworksTimer = 0.0f;
                            loopCount = 0;
                        }

                    }
                }
            }
        }

    }
    else
    {
        isFireworksActive = false;
        fireworksTimer = 0.0f;
    }

    //	パーティクル更新
    particles->Update(elapsed_time);
}

void ParticleRenderer::Render(ID3D11DeviceContext* immediate_context)
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();

    if (!dc || !renderState) return;

    // パーティクル(加算合成・深度テストのみ・カリングなし)
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
    particles->Render(dc);
    // 元に戻す
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));
}

void ParticleRenderer::DrawGUI()
{
    if (ImGui::CollapsingHeader("Particle"))
    {
        for (auto& pos : particle_positions)
        {
            ImGui::DragFloat3(("Particle Position " + std::to_string(&pos - &particle_positions[0])).c_str(), &pos.x, 0.1f);
        }
        ImGui::Separator();
        for (auto& fireWorkPos : fireworks_positions)
        {
            ImGui::DragFloat3(("Firework Position " + std::to_string(&fireWorkPos - &fireworks_positions[0])).c_str(), &fireWorkPos.x, 0.1f);
        }
    }
}

void ParticleRenderer::SaveToJson(json& j)
{
    for (auto& pos : particle_positions)
    {
        j["particle_positions"].push_back({ pos.x, pos.y, pos.z });
    }
    for (auto& fireWorkPos : fireworks_positions)
    {
        j["fireworks_positions"].push_back({ fireWorkPos.x, fireWorkPos.y, fireWorkPos.z });
    }
}

void ParticleRenderer::LoadFromJson(const json& j)
{
    if (j.contains("particle_positions"))
    {
        particle_positions.clear();// 既存のパーティクル位置をクリア
        for (const auto& pos : j["particle_positions"])
        {
            particle_positions.emplace_back(pos[0], pos[1], pos[2]);
        }
    }
    if (j.contains("fireworks_positions"))
    {
        fireworks_positions.clear();// 既存の花火位置をクリア
        for (const auto& pos : j["fireworks_positions"])
        {
            fireworks_positions.emplace_back(pos[0], pos[1], pos[2]);
        }
    }
}