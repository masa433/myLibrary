#include "Graphics.h"
#include "input.h"
#include "scene_loading.h"
#include "sceneManager.h"

void scene_loading::initialize()
{
	// ロード画面の初期化処理

	//スレッド開始
	thread = std::make_unique<std::thread>(LoadingThread, this);
}

void scene_loading::uninitialize()
{
	// ロード画面の終了処理

	if (thread != nullptr)
	{
		thread->join(); // スレッドの終了を待機
		thread = nullptr;
	}
}

void scene_loading::update(float elapsed_time)
{
	// ロード画面の更新処理
	// ここでリソースのロードや初期化を行うことができます
	// 例: ロードが完了したら次のシーンに切り替える
	// sceneManager::Instance().ChangeScene(new scene_main());

	//次のシーンの準備が完了したらシーンを切り替える
	if (nextScene != nullptr && nextScene->IsReady())
	{
		sceneManager::Instance().ChangeScene(nextScene.release());
		nextScene = nullptr;
	}
}

void scene_loading::render(float elapsed_time)
{
	// ロード画面の描画処理
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	ID3D11RenderTargetView* backBufferRTV = Graphics::Instance().GetRenderTargetView();
	dc->ClearRenderTargetView(backBufferRTV, clear_color);
	dc->ClearDepthStencilView(Graphics::Instance().GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	dc->OMSetRenderTargets(1, &backBufferRTV, Graphics::Instance().GetDepthStencilView());
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