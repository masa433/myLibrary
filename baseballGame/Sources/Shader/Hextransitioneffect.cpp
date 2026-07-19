#include "Hextransitioneffect.h"
#include "Graphics.h"
#include "misc.h"
#include <d3dcompiler.h>
#include <algorithm>
#include "shader.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace
{
	HRESULT CompileShaderFromFile(const wchar_t* filename, const char* entryPoint, const char* target, ID3DBlob** blobOut)
	{
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS; // 厳密なコンパイルを有効にする

#ifndef _DEBUG
		flags |= D3DCOMPILE_DEBUG; // デバッグ情報を含める
#endif

		ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, flags, 0, blobOut, errorBlob.GetAddressOf());

		if(FAILED(hr) && errorBlob)
		{
			OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
		}
		return hr;
	}
}

void HexTransitionEffect::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	HRESULT hr = S_OK;

	//頂点シェーダーのコンパイル
	ComPtr<ID3DBlob> vsBlob;
	//hr = CompileShaderFromFile(L".\\Shaders\\HextransitionVS.hlsl", "VSMain", "vs_5_0", vsBlob.GetAddressOf());
	hr = create_vs_from_cso(device, ".\\resources\\shader\\HextransitionVS.cso", vertexShader.GetAddressOf(), nullptr, nullptr, 0);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	

	//ピクセルシェーダーのコンパイル
	ComPtr<ID3DBlob> psBlob;
	//hr = CompileShaderFromFile(L".\\Shaders\\HextransitionPS.hlsl", "PSMain", "ps_5_0", psBlob.GetAddressOf());
	hr = create_ps_from_cso(device, ".\\resources\\shader\\HextransitionPS.cso", pixelShader.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//定数バッファの作成
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.ByteWidth = sizeof(TransitionConstants);
	bd.CPUAccessFlags = 0;
	hr = device->CreateBuffer(&bd, nullptr, constantBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//バックバッファと同じサイズ・フォーマットでコピー用のテクスチャを作成
	ComPtr<ID3D11Resource> backBufferResource;
	Graphics::Instance().GetRenderTargetView()->GetResource(backBufferResource.GetAddressOf());

	ComPtr<ID3D11Texture2D> backBufferTexture;
	backBufferResource.As(&backBufferTexture);// ID3D11Texture2Dにキャスト

	D3D11_TEXTURE2D_DESC desc{};
	backBufferTexture->GetDesc(&desc);// バックバッファの情報を取得
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // シェーダーリソースとして使用できるようにする
	desc.Usage = D3D11_USAGE_DEFAULT; // デフォルトの使用方法
	desc.CPUAccessFlags = 0; // CPUからのアクセスは不要
	desc.MiscFlags = 0; // 特殊な用途はなし

	hr = device->CreateTexture2D(&desc, nullptr, sceneCopyTexture.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	
	hr = device->CreateShaderResourceView(sceneCopyTexture.Get(), nullptr, sceneCopySRV.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void HexTransitionEffect::Start(float durationSeconds)
{
	playing = true;
	finished = false;
	progress = 0.0f;
	timer = 0.0f;
	duration = durationSeconds;
}

void HexTransitionEffect::Reset()
{
	playing = false;
	finished = false;
	progress = 0.0f;
	timer = 0.0f;
}

void HexTransitionEffect::Update(float elapsedTime)
{
	if (!playing) return;

	timer += elapsedTime;
	progress = (std::min)(timer / duration, 1.0f);

	if(progress >= 1.0f)
	{
		playing = false;
		finished = true;
	}
}

void HexTransitionEffect::Render()
{
	if (!playing && !finished) return;
	if (!constantBuffer || !vertexShader || !pixelShader) return;

	// 通常描画済みのバックバッファをテクスチャにキャプチャ
	CaptureScene();

	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	TransitionConstants cb{};
	cb.screen_size = { Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight() };
	cb.progress = progress;
	cb.hex_size = 50.0f; // 六角形のサイズ
	cb.direction = { 1.0f, 1.0f };    // 中央左から中央右へのワイプ方向（正規化ベクトル）
	cb.jitter = 0.35f;                 // 出現タイミングのランダム幅
	cb.edge_softness = 0.04f;                 // 出現境界のぼかし

	dc->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	dc->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());


	dc->PSSetShaderResources(0, 1, sceneCopySRV.GetAddressOf());
	ID3D11SamplerState* sampler = renderState->GetSamplerState(SamplerState::LinearClamp);
	dc->PSSetSamplers(0, 1, &sampler);

	// ブレンドステート
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	// デプスステンシルステート
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	// ラスタライザーステート
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 描画
	dc->IASetInputLayout(nullptr);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	dc->Draw(3, 0);// フルスクリーン三角形を描画
	
}

void HexTransitionEffect::CaptureScene()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	// バックバッファのコピーを作成
	ComPtr<ID3D11Resource> backBufferResource;
	Graphics::Instance().GetRenderTargetView()->GetResource(backBufferResource.GetAddressOf());

	dc->CopyResource(sceneCopyTexture.Get(), backBufferResource.Get());
}