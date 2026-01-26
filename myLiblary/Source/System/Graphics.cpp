#include "pch.h"
#include "Misc.h"
#include "Graphics.h"

void Graphics::AcquireHighPerformanceAdapter(IDXGIFactory6* dxgiFactory6, IDXGIAdapter3** dxgiAdapter3)
{
	HRESULT hr{ S_OK };

	Microsoft::WRL::ComPtr<IDXGIAdapter3> enumerated_adapter;
	for (UINT adapter_index = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory6->EnumAdapterByGpuPreference(adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(enumerated_adapter.ReleaseAndGetAddressOf())); ++adapter_index) 
	{
		DXGI_ADAPTER_DESC1 adapter_desc;
		hr = enumerated_adapter->GetDesc1(&adapter_desc);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		if (adapter_desc.VendorId == 0x1002/*AMD*/ || adapter_desc.VendorId == 0x10DE/*NVIDIA*/)
		{
			*dxgiAdapter3 = enumerated_adapter.Detach();
			break;
		}
	}
}

// 初期化
void Graphics::Initialize(HWND hWnd)
{
	this->hWnd = hWnd;
	// 画面のサイズを取得する。
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT screenWidth = rc.right - rc.left;
	UINT screenHeight = rc.bottom - rc.top;

	this->screenWidth = static_cast<float>(screenWidth);
	this->screenHeight = static_cast<float>(screenHeight);

	HRESULT hr = S_OK;

	//DXGIファクトリーの生成
	UINT createFactoryFlags = 0;
	
#if defined(DEBUG) || defined(_DEBUG)
	createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(dxgiFactory6.GetAddressOf()));
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 高性能アダプタを取得
	AcquireHighPerformanceAdapter(dxgiFactory6.Get(), adapter.GetAddressOf());

	// デバイス＆スワップチェーンの生成
	{
		UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1,
		};


		D3D_FEATURE_LEVEL featureLevel;

		// デバイスを作成
		hr = D3D11CreateDevice(
			adapter.Get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createDeviceFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			device.GetAddressOf(),
			&featureLevel,
			dc.GetAddressOf()
		);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// スワップチェーンの作成
	CreateSwapChain();

	
	// 深度ステンシルビューの生成
	{
		// 深度ステンシル情報を書き込むためのテクスチャを作成する。
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		D3D11_TEXTURE2D_DESC texture2dDesc;
		texture2dDesc.Width = screenWidth;
		texture2dDesc.Height = screenHeight;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2dDesc, nullptr, texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// 深度ステンシルテクスチャへの書き込みに窓口になる深度ステンシルビューを作成する。
		hr = device->CreateDepthStencilView(texture2d.Get(), nullptr, depthStencilView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ビューポート
	{
		viewport.Width = static_cast<float>(screenWidth);
		viewport.Height = static_cast<float>(screenHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
	}

	// レンダーステート生成
	renderState = std::make_unique<RenderState>(device.Get());

	// レンダラ生成
	shapeRenderer = std::make_unique<ShapeRenderer>(device.Get());
	modelRenderer = std::make_unique<ModelRenderer>(device.Get());

}

// スワップチェーンの作成
void Graphics::CreateSwapChain()
{
	HRESULT hr{ S_OK };

	if (swapchain)
	{
		// 既存のレンダーターゲットビューをリセット
		renderTargetView.Reset();

		// スワップチェーンのリサイズ
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapchain->GetDesc(&swapChainDesc);
		hr = swapchain->ResizeBuffers(
			swapChainDesc.BufferCount,
			static_cast<UINT>(screenWidth),
			static_cast<UINT>(screenHeight),
			swapChainDesc.BufferDesc.Format,
			swapChainDesc.Flags);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// レンダーターゲットビューの再作成
		Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTargetBuffer;
		hr = swapchain->GetBuffer(0, IID_PPV_ARGS(renderTargetBuffer.GetAddressOf()));
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		hr = device->CreateRenderTargetView(renderTargetBuffer.Get(), nullptr, renderTargetView.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
	else
	{
		// Tearing対応チェック
		BOOL allowTearing = FALSE;
		hr = dxgiFactory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		tearingSupported = SUCCEEDED(hr) && allowTearing;

		// スワップチェーンの作成
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc1{};
		swapChainDesc1.Width = static_cast<UINT>(screenWidth);
		swapChainDesc1.Height = static_cast<UINT>(screenHeight);
		swapChainDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc1.Stereo = FALSE;
		swapChainDesc1.SampleDesc.Count = 1;
		swapChainDesc1.SampleDesc.Quality = 0;
		swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc1.BufferCount = 2;
		swapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc1.Flags = tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		hr = dxgiFactory6->CreateSwapChainForHwnd(
			device.Get(),
			hWnd,
			&swapChainDesc1,
			nullptr,
			nullptr,
			swapchain.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// Alt+Enterによるフルスクリーン切り替えを無効化
		hr = dxgiFactory6->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// レンダーターゲットビューの作成
		Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTargetBuffer;
		hr = swapchain->GetBuffer(0, IID_PPV_ARGS(renderTargetBuffer.GetAddressOf()));
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		hr = device->CreateRenderTargetView(renderTargetBuffer.Get(), nullptr, renderTargetView.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ビューポートの更新
	viewport.Width = screenWidth;
	viewport.Height = screenHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	dc->RSSetViewports(1, &viewport);
}

// フルスクリーン/ウィンドウモード切り替え
void Graphics::StylizeWindow(bool fullscreen)
{
	fullscreenMode = fullscreen;

	if (fullscreen)
	{
		// ウィンドウモード時の位置を記録
		GetWindowRect(hWnd, &windowedRect);

		// フルスクリーン用のウィンドウスタイルに変更
		DWORD fullscreenStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME);
		SetWindowLongPtrA(hWnd, GWL_STYLE, fullscreenStyle);

		RECT fullscreenWindowRect;
		HRESULT hr{ E_FAIL };

		// スワップチェーンから出力デバイスの解像度を取得
		if (swapchain)
		{
			Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
			hr = swapchain->GetContainingOutput(&dxgiOutput);
			if (hr == S_OK)
			{
				DXGI_OUTPUT_DESC outputDesc;
				hr = dxgiOutput->GetDesc(&outputDesc);
				if (hr == S_OK)
				{
					fullscreenWindowRect = outputDesc.DesktopCoordinates;
				}
			}
		}

		// スワップチェーンからの取得に失敗した場合はディスプレイ設定から取得
		if (hr != S_OK)
		{
			DEVMODE devmode = {};
			devmode.dmSize = sizeof(DEVMODE);
			EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devmode);

			fullscreenWindowRect = {
				devmode.dmPosition.x,
				devmode.dmPosition.y,
				devmode.dmPosition.x + static_cast<LONG>(devmode.dmPelsWidth),
				devmode.dmPosition.y + static_cast<LONG>(devmode.dmPelsHeight)
			};
		}

		// フルスクリーンのウィンドウ位置とサイズを設定
		SetWindowPos(
			hWnd,
			nullptr,
			fullscreenWindowRect.left,
			fullscreenWindowRect.top,
			fullscreenWindowRect.right - fullscreenWindowRect.left,
			fullscreenWindowRect.bottom - fullscreenWindowRect.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		ShowWindow(hWnd, SW_MAXIMIZE);
	}
	else
	{
		// ウィンドウモードのスタイルに変更
		DWORD windowedStyle = WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME | WS_VISIBLE;
		SetWindowLongPtrA(hWnd, GWL_STYLE, windowedStyle);

		SetWindowPos(
			hWnd,
			HWND_NOTOPMOST,
			windowedRect.left,
			windowedRect.top,
			windowedRect.right - windowedRect.left,
			windowedRect.bottom - windowedRect.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		ShowWindow(hWnd, SW_NORMAL);
	}
}

// サイズ変更時の処理
void Graphics::OnSizeChanged(UINT width, UINT height)
{
	if (width > 0 && height > 0 && (width != static_cast<UINT>(screenWidth) || height != static_cast<UINT>(screenHeight)))
	{
		// スクリーンサイズを更新
		screenWidth = static_cast<float>(width);
		screenHeight = static_cast<float>(height);

		// GPUコマンドの完了を待つ
		dc->Flush();
		dc->ClearState();

		// 深度ステンシルビューの再作成
		depthStencilView.Reset();
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		D3D11_TEXTURE2D_DESC texture2dDesc;
		texture2dDesc.Width = width;
		texture2dDesc.Height = height;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;

		HRESULT hr = device->CreateTexture2D(&texture2dDesc, nullptr, texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		hr = device->CreateDepthStencilView(texture2d.Get(), nullptr, depthStencilView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// スワップチェーンの再作成
		CreateSwapChain();
	}
}


// クリア
void Graphics::Clear(float r, float g, float b, float a)
{
	float color[4]{ r, g, b, a };
	dc->ClearRenderTargetView(renderTargetView.Get(), color);
	dc->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

// レンダーターゲット設定
void Graphics::SetRenderTargets()
{
	dc->RSSetViewports(1, &viewport);
	dc->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
}

// 画面表示
void Graphics::Present(UINT syncInterval)
{
	swapchain->Present(syncInterval, 0);
}

