#pragma once

#include <mutex>
#include "RenderState.h"
#include "ShapeRenderer.h"
#include "ModelRenderer.h"
#include <dxgi1_6.h>

// グラフィックス
class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;

public:
	// インスタンス取得
	static Graphics& Instance()
	{
		static Graphics instance;
		return instance;
	}

	// 初期化
	void Initialize(HWND hWnd);

	// クリア
	void Clear(float r, float g, float b, float a);

	// レンダーターゲット設定
	void SetRenderTargets();

	// 画面表示
	void Present(UINT syncInterval);

	// ウインドウハンドル取得
	HWND GetWindowHandle() { return hWnd; }

	// デバイス取得
	ID3D11Device* GetDevice() { return device.Get(); }

	// デバイスコンテキスト取得
	ID3D11DeviceContext* GetDeviceContext() { return dc.Get(); }

	// スクリーン幅取得
	float GetScreenWidth() const { return screenWidth; }

	// スクリーン高さ取得
	float GetScreenHeight() const { return screenHeight; }

	// レンダーステート取得
	RenderState* GetRenderState() { return renderState.get(); }

	// シェイプレンダラ取得
	ShapeRenderer* GetShapeRenderer() const { return shapeRenderer.get(); }

	// モデルレンダラ取得
	ModelRenderer* GetModelRenderer() const { return modelRenderer.get(); }

	//ミューテックス取得(排他制御用オブジェクト)
	std::mutex& GetMutex() { return mutex; }

	// フルスクリーン関連
	bool IsFullScreen() const { return fullscreenMode; }
	void StylizeWindow(bool fullscreen);
	void OnSizeChanged(UINT width, UINT height);

private:
	void CreateSwapChain();
	void AcquireHighPerformanceAdapter(IDXGIFactory6* dxgiFactory6, IDXGIAdapter3** dxgiAdapter3);


public:
	HWND											hWnd = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device>			device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		dc;
	Microsoft::WRL::ComPtr<IDXGISwapChain1>			swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	depthStencilView;
	D3D11_VIEWPORT									viewport;

	float	screenWidth = 0;
	float	screenHeight = 0;

	std::unique_ptr<RenderState>					renderState;
	std::unique_ptr<ShapeRenderer>					shapeRenderer;
	std::unique_ptr<ModelRenderer>					modelRenderer;

	std::mutex mutex;

	// フルスクリーン関連
	bool fullscreenMode = false;
	bool tearingSupported = false;
	RECT windowedRect = {};
	Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory6;
	Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter;
};
