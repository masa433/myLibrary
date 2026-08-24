#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <memory>
#include "RenderState.h"
#include "shapeRenderer.h"
#include "PrimitiveRenderer.h"
#include "Light.h"
#include "ModelRenderer.h"
#include <mutex>


class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;

public:
	// シングルトンインスタンスの取得
	static Graphics& Instance()
	{
		static Graphics instance;
		return instance;
	}

	// レンダーステートの初期化
	void Initialize(HWND hwnd);

	//クリア
	void Clear(float r, float g, float b, float a);

	//レンダーターゲットの設定
	void SetRenderTarget();

	//画面表示
	void Present(UINT syncInterval);

	//ウィンドウハンドル取得
	HWND GetHwnd() const { return hWnd; }

	// デバイスの取得
	ID3D11Device* GetDevice() const { return device.Get(); }

	// デバイスコンテキスト取得
	ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

	// スクリーン幅取得
	float GetScreenWidth() const { return screenWidth; }

	// スクリーン高さ取得
	float GetScreenHeight() const { return screenHeight; }

	// レンダーステート取得
	RenderState* GetRenderState() { return renderState.get(); }

	IDXGISwapChain* GetSwapChain() const { return swapchain.Get(); }

	// シェイプレンダラ取得
	ShapeRenderer* GetShapeRenderer() const { return shapeRenderer.get(); }

	// プリミティブレンダラ取得
	PrimitiveRenderer* GetPrimitiveRenderer() const { return primitiveRenderer.get(); }

	// ライトマネージャー取得
	Light& GetLightManager() { return lightManager; }

	// モデルレンダラ取得
	ModelRenderer* GetModelRenderer() const { return modelRenderer.get(); }

	//深度ステンシルビュー取得
	ID3D11DepthStencilView* GetDepthStencilView() const { return depthStencilView.Get(); }

	//レンダーターゲットビュー取得
	ID3D11RenderTargetView* GetRenderTargetView() const { return renderTargetView.Get(); }

	//レンダーターゲットビューのアドレスを取得
	ID3D11RenderTargetView** GetRenderTargetViewAddress() { return renderTargetView.GetAddressOf(); }

	//ミューテックス取得
	std::mutex& GetMutex() { return mutex; }
private:
	HWND											hWnd = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device>			device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		immediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	depthStencilView;
	D3D11_VIEWPORT									viewport;

	float	screenWidth = 0;
	float	screenHeight = 0;

	Light											lightManager;
	std::unique_ptr<RenderState>					renderState;
	std::unique_ptr<ShapeRenderer>					shapeRenderer;
	std::unique_ptr<PrimitiveRenderer>				primitiveRenderer;
	std::unique_ptr<ModelRenderer>					modelRenderer;

	std::mutex mutex; // スレッドセーフのためのミューテックス
};