#include "framework.h"
#include "shader.h"
#include "texture.h"
#include "input.h"


// 垂直同期間隔設定
static const int syncInterval = 1;

//#ifdef USE_IMGUI
//ImWchar glyphRangesJapanese[] = {
//	0x0020, 0x00FF, // Basic Latin + Latin Supplement
//	0x3000, 0x30FF, // CJK Symbols, Hiragana, Katakana
//	0x31F0, 0x31FF, // Katakana Phonetic Extensions
//	0xFF00, 0xFFEF, // Half-width / Full-width
//	0x4E00, 0x9FAF, // CJK Unified Ideographs (kanji)
//	0,
//};
//#endif

framework::framework(HWND hwnd) : hwnd(hwnd)
{
	hDC = GetDC(hwnd);

#ifndef _DEBUG
	// リリースビルド時にボーダーレスフルスクリーンへ変更
	// ※Graphics初期化前にウィンドウサイズとスタイルを変更し、内部解像度をネイティブに合わせる
	/*LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
	SetWindowLongPtr(hwnd, GWL_STYLE, style);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(hwnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_NOZORDER);*/
#endif

	//インプット初期化
	Input::Instance().Initialize(hwnd);

	//グラフィックス初期化
	Graphics::Instance().Initialize(hwnd);



	//framebuffers[0] = std::make_unique<framebuffer>(device.Get(), 1280, 720);
	//framebuffers[1] = std::make_unique<framebuffer>(device.Get(), 1280 / 2, 720 / 2);

	//bit_block_transfer = std::make_unique<fullscreen_quad>(device.Get());

	//create_ps_from_cso(device.Get(), "luminance_extraction_ps.cso", pixel_shaders[0].GetAddressOf());
	//create_ps_from_cso(device.Get(), "blur_ps.cso", pixel_shaders[1].GetAddressOf());




	sceneManager::Instance().ChangeScene(new SceneTitle());


}

bool framework::initialize()
{
	return true;
}

void framework::update(float elapsed_time/*Elapsed seconds from last frame*/)
{
	Input::Instance().Update();

#ifdef USE_IMGUI
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif



	sceneManager::Instance().Update(elapsed_time);

}
void framework::render(float elapsed_time/*Elapsed seconds from last frame*/)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	//Graphics::Instance().Clear(0.5f, 0.8f, 1.0f, 1.0f);
	Graphics::Instance().Clear(0.0f, 0.0f, 0.0f, 1.0f);

	//レンダーターゲット設定
	Graphics::Instance().SetRenderTarget();

	sceneManager::Instance().Render();

	sceneManager::Instance().DrawGUI();

#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	//画面表示
	Graphics::Instance().Present(syncInterval);

}

bool framework::uninitialize()
{

	return true;
}

framework::~framework()
{

	sceneManager::Instance().Clear();


	ReleaseDC(hwnd, hDC);
}