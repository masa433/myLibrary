#include "pch.h"
#include "Misc.h"
#include "Graphics.h"

// 初期化
bool Graphics::Initialize(HWND hWnd)
{
	auto reportFailure = [](const wchar_t* stage, HRESULT result)
	{
		wchar_t message[256]{};
		swprintf_s(message, L"グラフィックスの初期化に失敗しました (%s)\nHRESULT: 0x%08X", stage, static_cast<unsigned int>(result));
		MessageBoxW(nullptr, message, L"BASEBALL 起動エラー", MB_OK | MB_ICONERROR);
	};

	this->hWnd = hWnd;
	// 画面のサイズを取得する。
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT screenWidth = rc.right - rc.left;
	UINT screenHeight = rc.bottom - rc.top;

	this->screenWidth = static_cast<float>(screenWidth);
	this->screenHeight = static_cast<float>(screenHeight);

	// 画面スケーリングの初期化
	screenScaler.UpdateScale(GetScreenWidth(), GetScreenHeight());

	HRESULT hr = S_OK;

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

		// スワップチェーンを作成するための設定オプション
		DXGI_SWAP_CHAIN_DESC swapchainDesc;
		{
			swapchainDesc.BufferDesc.Width = screenWidth;
			swapchainDesc.BufferDesc.Height = screenHeight;
			swapchainDesc.BufferDesc.RefreshRate.Numerator = 144;
			swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
			swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			swapchainDesc.SampleDesc.Count = 1;
			swapchainDesc.SampleDesc.Quality = 0;
			swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapchainDesc.BufferCount = 2;
			swapchainDesc.OutputWindow = hWnd;
			swapchainDesc.Windowed = TRUE;
			swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapchainDesc.Flags = 0;
		}

		D3D_FEATURE_LEVEL featureLevel;

		// デバイス＆スワップチェーンの生成
		hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&swapchainDesc,
			swapchain.GetAddressOf(),
			device.GetAddressOf(),
			&featureLevel,
			immediateContext.GetAddressOf()
		);
		if (FAILED(hr) || !swapchain || !device || !immediateContext)
		{
			reportFailure(L"D3D11 デバイス作成", hr);
			return false;
		}
	}

#if defined(DEBUG) || defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D11InfoQueue> infoQueue;
	if (SUCCEEDED(device.As(&infoQueue)))
	{
		// 無視したい警告メッセージのIDリスト
		D3D11_MESSAGE_ID hideMessages[] =
		{
			D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET, // 意図的にレンダーターゲットを外した時の警告を無視
			D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET,          // サンプラーが未セットの警告を無視 (今回追加)
		};

		D3D11_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hideMessages);
		filter.DenyList.pIDList = hideMessages;

		// フィルターを適用
		infoQueue->AddStorageFilterEntries(&filter);
	}
#endif

	Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;
	if (SUCCEEDED(device.As(&multithread)))
	{
		multithread->SetMultithreadProtected(TRUE);
	}

	// レンダーターゲットビューの生成
	{
		// スワップチェーンからバックバッファテクスチャを取得する。
		// ※スワップチェーンに内包されているバックバッファテクスチャは'色'を書き込むテクスチャ。
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = swapchain->GetBuffer(
			0,
			__uuidof(ID3D11Texture2D),
			reinterpret_cast<void**>(texture2d.GetAddressOf()));
		if (FAILED(hr) || !texture2d)
		{
			reportFailure(L"バックバッファ取得", hr);
			return false;
		}

		// バックバッファテクスチャへの書き込みの窓口となるレンダーターゲットビューを生成する。
		hr = device->CreateRenderTargetView(texture2d.Get(), nullptr, renderTargetView.GetAddressOf());
		if (FAILED(hr) || !renderTargetView)
		{
			reportFailure(L"レンダーターゲット作成", hr);
			return false;
		}
	}

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
		if (FAILED(hr) || !texture2d)
		{
			reportFailure(L"深度テクスチャ作成", hr);
			return false;
		}

		// 深度ステンシルテクスチャへの書き込みに窓口になる深度ステンシルビューを作成する。
		hr = device->CreateDepthStencilView(texture2d.Get(), nullptr, depthStencilView.GetAddressOf());
		if (FAILED(hr) || !depthStencilView)
		{
			reportFailure(L"深度ステンシルビュー作成", hr);
			return false;
		}
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
	if (!renderState || !renderState->IsValid())
	{
		reportFailure(L"レンダーステート作成", E_FAIL);
		return false;
	}

	// レンダラ生成
	shapeRenderer = std::make_unique<ShapeRenderer>(device.Get());
	primitiveRenderer = std::make_unique<PrimitiveRenderer>(device.Get());
	modelRenderer = std::make_unique<ModelRenderer>(device.Get());

	return true;
}

// クリア
void Graphics::Clear(float r, float g, float b, float a)
{
	if (!immediateContext || !renderTargetView || !depthStencilView) return;
	float color[4]{ r, g, b, a };
	immediateContext->ClearRenderTargetView(renderTargetView.Get(), color);
	immediateContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

// レンダーターゲット設定
void Graphics::SetRenderTarget()
{
	if (!immediateContext || !renderTargetView || !depthStencilView) return;
	immediateContext->RSSetViewports(1, &viewport);
	immediateContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
}

// 画面表示
void Graphics::Present(UINT syncInterval)
{
	if (!swapchain) return;
	swapchain->Present(syncInterval, 0);
}

// 解像度変更時にスケーラーを更新
void Graphics::OnScreenResolutionChanged(float newWidth, float newHeight)
{
	if (!immediateContext || newWidth <= 0.0f || newHeight <= 0.0f) return;
	screenWidth = newWidth;
	screenHeight = newHeight;
	screenScaler.UpdateScale(newWidth, newHeight);

	// ビューポートの更新
	viewport.Width = newWidth;
	viewport.Height = newHeight;
	immediateContext->RSSetViewports(1, &viewport);
}

//void Graphics::ReportLive(const char* tag)
//{
//#if defined(_DEBUG)
//	OutputDebugStringA("\n=== "); OutputDebugStringA(tag); OutputDebugStringA(" ===\n");
//	if (immediateContext) { immediateContext->ClearState(); immediateContext->Flush(); }
//	Microsoft::WRL::ComPtr<ID3D11Debug> dbg;
//	if (device && SUCCEEDED(device.As(&dbg)))
//		dbg->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
//#endif
//}
