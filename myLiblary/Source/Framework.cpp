#include "pch.h"
#include "Framework.h"
#include "System/Input.h"
#include "System/Graphics.h"
#include "System/ImGuiRenderer.h"
#include "SceneGame.h"
#include "SceneTitle.h"
#include "SceneManager.h"
#include "System/Audio.h"
#include "System/Audio3D.h"
#include "EffectManager.h"


// 垂直同期間隔設定
static const int syncInterval = 1;

//static SceneGame sceneGame;

// コンストラクタ
Framework::Framework(HWND hWnd)
	: hWnd(hWnd)
{
	//オーディオ初期化
	Audio::Instance().Initialize();

	Audio3DSystem::Instance().Initialize();

	hDC = GetDC(hWnd);

	// インプット初期化
	Input::Instance().Initialize(hWnd);

	// グラフィックス初期化
	Graphics::Instance().Initialize(hWnd);

	// フルスクリーン設定がTRUEの場合、初期化後にフルスクリーンに切り替え
	if (FULLSCREEN)
	{
		Graphics::Instance().StylizeWindow(true);
	}

	// IMGUI初期化
	ImGuiRenderer::Initialize(hWnd, Graphics::Instance().GetDevice(), Graphics::Instance().GetDeviceContext());

	//エフェクトマネージャー初期化
	EffectManager::Instance().Initialize();

	// シーン初期化
	//sceneGame.Initialize();
	SceneManager::Instance().ChangeScene(new SceneGame);
}

// デストラクタ
Framework::~Framework()
{
	// シーン終了化
	//sceneGame.Finalize();
	SceneManager::Instance().Clear();

	// IMGUI終了化
	ImGuiRenderer::Finalize();

	//エフェクトマネージャー終了処理
	EffectManager::Instance().Finalize();

	//オーディオ終了化
	Audio::Instance().Finalize();

	ReleaseDC(hWnd, hDC);
}

// 更新処理
void Framework::Update(float elapsedTime)
{
	// Alt+Enterでフルスクリーン切り替え
	if ((GetAsyncKeyState(VK_RETURN) & 1) && (GetAsyncKeyState(VK_MENU) & 0x8000))
	{
		Graphics::Instance().StylizeWindow(!Graphics::Instance().IsFullScreen());
	}


	// インプット更新処理
	Input::Instance().Update();

	// IMGUIフレーム開始処理	
	ImGuiRenderer::NewFrame();

	// シーン更新処理
	//sceneGame.Update(elapsedTime);
	SceneManager::Instance().Update(elapsedTime);

	// オーディオ更新処理

}

// 描画処理
void Framework::Render(float elapsedTime)
{
	//別スレッド中にデバイスコンテキストが使われていた時に同時アクセスしないように排他制御
	std::lock_guard<std::mutex> lock(Graphics::Instance().GetMutex());

	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// 画面クリア
	Graphics::Instance().Clear(0, 0, 1, 1);

	// レンダーターゲット設定
	Graphics::Instance().SetRenderTargets();

	// シーン描画処理
	//sceneGame.Render();
	SceneManager::Instance().Render();

	// シーンGUI描画処理
	//sceneGame.DrawGUI();
	SceneManager::Instance().DrawGUI();

#if 0
	// IMGUIデモウインドウ描画（IMGUI機能テスト用）
	ImGui::ShowDemoWindow();
#endif
	// IMGUI描画
	ImGuiRenderer::Render(dc);

	// 画面表示
	Graphics::Instance().Present(syncInterval);

}

// フレームレート計算
void Framework::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
	static int frames = 0;
	static float time_tlapsed = 0.0f;

	frames++;

	// Compute averages over one second period.
	if ((timer.TimeStamp() - time_tlapsed) >= 1.0f)
	{
		float fps = static_cast<float>(frames); // fps = frameCnt / 1
		float mspf = 1000.0f / fps;
		std::ostringstream outs;
		outs.precision(6);
		outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
		SetWindowTextA(hWnd, outs.str().c_str());

		// Reset for next average.
		frames = 0;
		time_tlapsed += 1.0f;
	}
}

// アプリケーションループ
int Framework::Run()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.Tick();
			CalculateFrameStats();

			float elapsedTime = syncInterval == 0
				? timer.TimeInterval()
				: syncInterval / static_cast<float>(GetDeviceCaps(hDC, VREFRESH))
				;
			Update(elapsedTime);
			Render(elapsedTime);
		}
	}
	return static_cast<int>(msg.wParam);
}

// メッセージハンドラ
LRESULT CALLBACK Framework::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	case WM_ENTERSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		timer.Stop();
		break;
	case WM_EXITSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
		timer.Start();
		break;
	case WM_SIZE:
	{
		RECT clientRect{};
		GetClientRect(hWnd, &clientRect);
		Graphics::Instance().OnSizeChanged(
			static_cast<UINT>(clientRect.right - clientRect.left),
			clientRect.bottom - clientRect.top);
		break;
	}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}