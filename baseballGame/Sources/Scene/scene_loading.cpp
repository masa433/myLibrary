#include "Graphics.h"
#include "input.h"
#include "scene_loading.h"
#include "sceneManager.h"
#include "LoadingTips.h"

void scene_loading::initialize()
{
	// ロード画面の初期化処理

	//スレッド開始
	thread = std::make_unique<std::thread>(LoadingThread, this);

	ID3D11Device* device = Graphics::Instance().GetDevice();
	ID3D11DeviceContext* deviceContext = Graphics::Instance().GetDeviceContext();

	//シェーダー
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	alpha = 0.0f; // 初期透明度を設定
	alphaSpeed = 1.0f; // 透明度の変化速度を設定

	// ロード画面のスプライトデータを作成
	loadingBackSpriteData = std::make_unique<SpriteData>();
	loadingBackSpriteData->texturePath = L".\\resources\\textures\\loadingBack.png";
	loadingBackSpriteData->position = spritePosition;
	loadingBackSpriteData->size = spriteSize;
	loadingBackSpriteData->rotation = 0.0f;
	loadingBackSpriteData->color = { spriteColor.x, spriteColor.y, spriteColor.z, alpha };
	loadingBackSprite = std::make_unique<sprite>(device, deviceContext, loadingBackSpriteData->texturePath.c_str());

	LoadingTips::Instance().Initialize(device);

}

void scene_loading::uninitialize()
{
	// ロード画面の終了処理

	if (thread != nullptr)
	{
		thread->join(); // スレッドの終了を待機
		thread = nullptr;
	}

	loadingBackSpriteData.reset();
	loadingBackSprite.reset();

	LoadingTips::Instance().Uninitialize();
}

void scene_loading::update(float elapsed_time)
{
	Input& input = Input::Instance();
	
	if (nextScene != nullptr && nextScene->IsReady() && !isFadingOut)
	{
		isFadingOut = true; // フェードアウトを開始
	}

	if(isFadingOut)
	{
		alpha -= (alphaSpeed * 5.0f) * elapsed_time; // 透明度を減少させる
		if (alpha <= 0.0f)
		{
			alpha = 0.0f;
			// シーン切り替え
			sceneManager::Instance().ChangeScene(nextScene.release());
			nextScene = nullptr;
			return;// シーン切り替え後はupdate処理を終了
		}
	}
	else
	{
		fadeIndelayTime -= elapsed_time; // フェードインの遅延時間を減少させる

		if (fadeIndelayTime <= 0.0f)
		{
			fadeIndelayTime = 0.0f; // 遅延時間が0以下にならないようにする

			alpha += alphaSpeed * elapsed_time; // 透明度を増加させる
			if (alpha > 1.0f)
			{
				alpha = 1.0f;
			}
		}
	}

	LoadingTips::Instance().Update(elapsed_time);
	
}

void scene_loading::render(float elapsed_time)
{
	// ロード画面の描画処理
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	ID3D11RenderTargetView* backBufferRTV = Graphics::Instance().GetRenderTargetView();
	dc->ClearRenderTargetView(backBufferRTV, clear_color);
	dc->ClearDepthStencilView(Graphics::Instance().GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	dc->OMSetRenderTargets(1, &backBufferRTV, Graphics::Instance().GetDepthStencilView());


	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);

	if(loadingBackSprite && loadingBackSpriteData)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(spritePosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(spriteSize);

		loadingBackSprite->render(dc,
			scaledPosition.x,
			scaledPosition.y,
			scaledSize.x, scaledSize.y,
			spriteColor.x, spriteColor.y, spriteColor.z, alpha,
			loadingBackSpriteData->rotation);
	}

	LoadingTips::Instance().Render(alpha);

	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);

	// 描画後の状態をリセット
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

}

void scene_loading::DrawGUI()
{
	// ロード画面のGUI描画処理
}

void scene_loading::LoadingThread(scene_loading* scene)
{
	//COM関連の初期化でスレッド毎に呼ぶ必要がある
	CoInitialize(nullptr);

	//次のシーンの初期化
	scene->nextScene->initialize();

	//スレッドが終わる前にCOM関連の終了処理
	CoUninitialize();

	//次のシーンの準備完了設定
	scene->nextScene->SetReady();
}