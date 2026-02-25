#include "framework.h"
#include "shader.h"
#include "texture.h"
#include "input.h"


// 垂直同期間隔設定
static const int syncInterval = 1;

framework::framework(HWND hwnd) : hwnd(hwnd)
{
	hDC = GetDC(hwnd);

	//インプット初期化
	Input::Instance().Initialize(hwnd);

	//グラフィックス初期化
	Graphics::Instance().Initialize(hwnd);



	//framebuffers[0] = std::make_unique<framebuffer>(device.Get(), 1280, 720);
	//framebuffers[1] = std::make_unique<framebuffer>(device.Get(), 1280 / 2, 720 / 2);

	//bit_block_transfer = std::make_unique<fullscreen_quad>(device.Get());

	//create_ps_from_cso(device.Get(), "luminance_extraction_ps.cso", pixel_shaders[0].GetAddressOf());
	//create_ps_from_cso(device.Get(), "blur_ps.cso", pixel_shaders[1].GetAddressOf());



	
	sceneGame.initialize();


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



	sceneGame.update(elapsed_time);

}
void framework::render(float elapsed_time/*Elapsed seconds from last frame*/)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	Graphics::Instance().Clear(0.5f, 0.8f, 1.0f, 1.0f);

	//レンダーターゲット設定
	Graphics::Instance().SetRenderTarget();

	sceneGame.render(elapsed_time);

	sceneGame.DrawGUI();

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
	
	sceneGame.uninitialize();


	ReleaseDC(hwnd, hDC);
}