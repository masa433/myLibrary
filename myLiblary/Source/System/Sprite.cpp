#include "../pch.h"
#include "System/Graphics.h"
#include "Sprite.h"
#include "Misc.h"
#include "GpuResourceUtils.h"

// コンストラクタ
Sprite::Sprite()
	: Sprite(nullptr)
{
}

// コンストラクタ
Sprite::Sprite(const char* filename)
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	HRESULT hr = S_OK;

	// 頂点バッファの生成
	{
		// 頂点バッファを作成するための設定オプション
		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = sizeof(Vertex) * 4;
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		// 頂点バッファオブジェクトの生成
		hr = device->CreateBuffer(&buffer_desc, nullptr, vertexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = sizeof(skymap_constants);
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.ByteWidth = sizeof(skymap_cb);
		D3D11_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pSysMem = &skymap_cb;
		subresourceData.SysMemPitch = 0;
		subresourceData.SysMemSlicePitch = 0;

		hr = device->CreateBuffer(&bufferDesc, nullptr, skymap_constant_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// 頂点シェーダー
	{
		// 入力レイアウト
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = GpuResourceUtils::LoadVertexShader(
			device,
			"Data/Shader/SpriteVS.cso",
			inputElementDesc,
			ARRAYSIZE(inputElementDesc),
			inputLayout.GetAddressOf(),
			vertexShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// スカイマップ
		hr = GpuResourceUtils::LoadVertexShader(
			device,
			"Data/Shader/skymapVS.cso",
			inputElementDesc,
			ARRAYSIZE(inputElementDesc),
			skymap_input_layout.GetAddressOf(),
			skymap_vertex_shader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	}

	// ピクセルシェーダー
	{
		hr = GpuResourceUtils::LoadPixelShader(
			device,
			"Data/Shader/SpritePS.cso",
			pixelShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// スカイマップ
		hr = GpuResourceUtils::LoadPixelShader(
			device,
			"Data/Shader/skymapPS.cso",
			skymap_pixel_shader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// サンプラーステート作成
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		HRESULT hr = device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}


	// テクスチャの生成	
	if (filename != nullptr)
	{
		// テクスチャファイル読み込み
		D3D11_TEXTURE2D_DESC desc;
		hr = GpuResourceUtils::LoadTexture(device, filename, shaderResourceView.GetAddressOf(), &desc);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		textureWidth = static_cast<float>(desc.Width);
		textureHeight = static_cast<float>(desc.Height);
	}
	else
	{
		// ダミーテクスチャ生成
		D3D11_TEXTURE2D_DESC desc;
		hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF, shaderResourceView.GetAddressOf(),
			&desc);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		textureWidth = static_cast<float>(desc.Width);
		textureHeight = static_cast<float>(desc.Height);
	}
}

// 描画実行
void Sprite::Render(const RenderContext& rc,
	float dx, float dy,					// 左上位置
	float dz,							// 奥行
	float dw, float dh,					// 幅、高さ
	float sx, float sy,					// 画像切り抜き位置
	float sw, float sh,					// 画像切り抜きサイズ
	float angle,						// 角度
	float r, float g, float b, float a	// 色
) const
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// 頂点座標
	DirectX::XMFLOAT2 positions[] = {
		DirectX::XMFLOAT2(dx,      dy),			// 左上
		DirectX::XMFLOAT2(dx + dw, dy),			// 右上
		DirectX::XMFLOAT2(dx,      dy + dh),	// 左下
		DirectX::XMFLOAT2(dx + dw, dy + dh),	// 右下
	};

	// テクスチャ座標
	DirectX::XMFLOAT2 texcoords[] = {
		DirectX::XMFLOAT2(sx,      sy),			// 左上
		DirectX::XMFLOAT2(sx + sw, sy),			// 右上
		DirectX::XMFLOAT2(sx,      sy + sh),	// 左下
		DirectX::XMFLOAT2(sx + sw, sy + sh),	// 右下
	};

	// スプライトの中心で回転させるために４頂点の中心位置が
	// 原点(0, 0)になるように一旦頂点を移動させる。
	float mx = dx + dw * 0.5f;
	float my = dy + dh * 0.5f;
	for (auto& p : positions)
	{
		p.x -= mx;
		p.y -= my;
	}

	// 頂点を回転させる
	float theta = DirectX::XMConvertToRadians(angle);
	float c = cosf(theta);
	float s = sinf(theta);
	for (auto& p : positions)
	{
		DirectX::XMFLOAT2 r = p;
		p.x = c * r.x + -s * r.y;
		p.y = s * r.x + c * r.y;
	}

	// 回転のために移動させた頂点を元の位置に戻す
	for (auto& p : positions)
	{
		p.x += mx;
		p.y += my;
	}

	// 現在設定されているビューポートからスクリーンサイズを取得する。
	D3D11_VIEWPORT viewport;
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &viewport);
	float screenWidth = viewport.Width;
	float screenHeight = viewport.Height;

	// スクリーン座標系からNDC座標系へ変換する。
	for (DirectX::XMFLOAT2& p : positions)
	{
		p.x = 2.0f * p.x / screenWidth - 1.0f;
		p.y = 1.0f - 2.0f * p.y / screenHeight;
	}

	// 頂点バッファの内容の編集を開始する。
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 頂点バッファの内容を編集
	Vertex* v = static_cast<Vertex*>(mappedSubresource.pData);
	for (int i = 0; i < 4; ++i)
	{
		v[i].position.x = positions[i].x;
		v[i].position.y = positions[i].y;
		v[i].position.z = dz;

		v[i].color.x = r;
		v[i].color.y = g;
		v[i].color.z = b;
		v[i].color.w = a;

		v[i].texcoord.x = texcoords[i].x / textureWidth;
		v[i].texcoord.y = texcoords[i].y / textureHeight;
	}

	// 頂点バッファの内容の編集を終了する。
	dc->Unmap(vertexBuffer.Get(), 0);

	// GPUに描画するためのデータを渡す
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);
	dc->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());
	dc->PSSetSamplers(0, 1, samplerState.GetAddressOf());
	// レンダーステート設定
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	// 描画
	dc->Draw(4, 0);
}

// 描画実行（テクスチャ切り抜き指定なし）
void Sprite::Render(const RenderContext& rc,
	float dx, float dy,					// 左上位置
	float dz,							// 奥行
	float dw, float dh,					// 幅、高さ
	float angle,						// 角度
	float r, float g, float b, float a	// 色
) const
{
	Render(rc, dx, dy, dz, dw, dh, 0, 0, textureWidth, textureHeight, angle, r, g, b, a);
}

// 描画実行
void Sprite::Render(ID3D11DeviceContext* immediate_context,
	float dx, float dy,
	float dw, float dh,
	float sx, float sy,
	float sw, float sh,
	float angle,
	float r, float g, float b, float a) const
{
	{
		// 現在設定されているビューポートからスクリーンサイズを取得する。
		D3D11_VIEWPORT viewport;
		UINT numViewports = 1;
		immediate_context->RSGetViewports(&numViewports, &viewport);
		float screen_width = viewport.Width;
		float screen_height = viewport.Height;

		// スプライトを構成する４頂点のスクリーン座標を計算する
		DirectX::XMFLOAT2 positions[] = {
			DirectX::XMFLOAT2(dx,      dy),			// 左上
			DirectX::XMFLOAT2(dx + dw, dy),			// 右上
			DirectX::XMFLOAT2(dx,      dy + dh),	// 左下
			DirectX::XMFLOAT2(dx + dw, dy + dh),	// 右下
		};

		// スプライトを構成する４頂点のテクスチャ座標を計算する
		DirectX::XMFLOAT2 texcoords[] = {
			DirectX::XMFLOAT2(sx,      sy),			// 左上
			DirectX::XMFLOAT2(sx + sw, sy),			// 右上
			DirectX::XMFLOAT2(sx,      sy + sh),	// 左下
			DirectX::XMFLOAT2(sx + sw, sy + sh),	// 右下
		};

		// スプライトの中心で回転させるために４頂点の中心位置が
		// 原点(0, 0)になるように一旦頂点を移動させる。
		float mx = dx + dw * 0.5f;
		float my = dy + dh * 0.5f;
		for (auto& p : positions)
		{
			p.x -= mx;
			p.y -= my;
		}

		// 頂点を回転させる
		const float PI = 3.141592653589793f;
		float theta = angle * (PI / 180.0f);	// 角度をラジアン(θ)に変換
		float c = cosf(theta);
		float s = sinf(theta);
		for (auto& p : positions)
		{
			DirectX::XMFLOAT2 r = p;
			p.x = c * r.x + -s * r.y;
			p.y = s * r.x + c * r.y;
		}

		// 回転のために移動させた頂点を元の位置に戻す
		for (auto& p : positions)
		{
			p.x += mx;
			p.y += my;
		}

		// スクリーン座標系からNDC座標系へ変換する。
		for (auto& p : positions)
		{
			p.x = 2.0f * p.x / screen_width - 1.0f;
			p.y = 1.0f - 2.0f * p.y / screen_height;
		}

		// 頂点バッファの内容の編集を開始する。
		D3D11_MAPPED_SUBRESOURCE mappedBuffer;
		HRESULT hr = immediate_context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// pDataを編集することで頂点データの内容を書き換えることができる。
		Vertex* v = static_cast<Vertex*>(mappedBuffer.pData);
		for (int i = 0; i < 4; ++i)
		{
			v[i].position.x = positions[i].x;
			v[i].position.y = positions[i].y;
			v[i].position.z = 0.0f;

			v[i].color.x = r;
			v[i].color.y = g;
			v[i].color.z = b;
			v[i].color.w = a;

			v[i].texcoord.x = texcoords[i].x / textureWidth;
			v[i].texcoord.y = texcoords[i].y / textureHeight;
		}

		// 頂点バッファの内容の編集を終了する。
		immediate_context->Unmap(vertexBuffer.Get(), 0);
	}

	{
		// パイプライン設定
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		immediate_context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
		immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		immediate_context->IASetInputLayout(inputLayout.Get());

		immediate_context->RSSetState(rasterizerState.Get());

		immediate_context->VSSetShader(vertexShader.Get(), nullptr, 0);
		immediate_context->PSSetShader(pixelShader.Get(), nullptr, 0);

		immediate_context->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());
		immediate_context->PSSetSamplers(0, 1, samplerState.GetAddressOf());

		// 描画
		immediate_context->Draw(4, 0);
	}
}

void Sprite::SkyRender(RenderContext& rc, float dx, float dy, float dz, float dw, float dh, float sx, float sy, float sw, float sh, float angle, float r, float g, float b, float a)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.view);
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.projection);
	DirectX::XMStoreFloat4x4(&skymap_cb.inverse_view_projection, DirectX::XMMatrixInverse(nullptr, V * P));

	dc->UpdateSubresource(skymap_constant_buffer.Get(), 0, 0, &skymap_cb, 0, 0);
	dc->VSSetConstantBuffers(5, 1, skymap_constant_buffer.GetAddressOf());
	dc->PSSetConstantBuffers(5, 1, skymap_constant_buffer.GetAddressOf());

	// 頂点座標
	DirectX::XMFLOAT2 positions[] = {
		DirectX::XMFLOAT2(dx,      dy),			// 左上
		DirectX::XMFLOAT2(dx + dw, dy),			// 右上
		DirectX::XMFLOAT2(dx,      dy + dh),	// 左下
		DirectX::XMFLOAT2(dx + dw, dy + dh),	// 右下
	};

	// テクスチャ座標
	DirectX::XMFLOAT2 texcoords[] = {
		DirectX::XMFLOAT2(sx,      sy),			// 左上
		DirectX::XMFLOAT2(sx + sw, sy),			// 右上
		DirectX::XMFLOAT2(sx,      sy + sh),	// 左下
		DirectX::XMFLOAT2(sx + sw, sy + sh),	// 右下
	};

#if 1
	// スプライトの中心で回転させるために４頂点の中心位置が
	// 原点(0, 0)になるように一旦頂点を移動させる。
	float mx = dx + dw * 0.5f;
	float my = dy + dh * 0.5f;
	for (auto& p : positions)
	{
		p.x -= mx;
		p.y -= my;
	}
#else
	float mx = dx + cx;
	float my = dy + cy;
	for (auto& p : positions)
	{
		p.x -= mx;
		p.y -= my;
	}

#endif

	// 頂点を回転させる
	float theta = DirectX::XMConvertToRadians(angle);
	float c = cosf(theta);
	float s = sinf(theta);
	for (auto& p : positions)
	{
		DirectX::XMFLOAT2 r = p;
		p.x = c * r.x + -s * r.y;
		p.y = s * r.x + c * r.y;
	}

	// 回転のために移動させた頂点を元の位置に戻す
	for (auto& p : positions)
	{
		p.x += mx;
		p.y += my;
	}

	// 現在設定されているビューポートからスクリーンサイズを取得する。
	D3D11_VIEWPORT viewport;
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &viewport);
	float screenWidth = viewport.Width;
	float screenHeight = viewport.Height;

	// スクリーン座標系からNDC座標系へ変換する。
	for (DirectX::XMFLOAT2& p : positions)
	{
		p.x = 2.0f * p.x / screenWidth - 1.0f;
		p.y = 1.0f - 2.0f * p.y / screenHeight;
	}

	// 頂点バッファの内容の編集を開始する。
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 頂点バッファの内容を編集
	Vertex* v = static_cast<Vertex*>(mappedSubresource.pData);
	for (int i = 0; i < 4; ++i)
	{
		v[i].position.x = positions[i].x;
		v[i].position.y = positions[i].y;
		v[i].position.z = dz;

		v[i].color.x = r;
		v[i].color.y = g;
		v[i].color.z = b;
		v[i].color.w = a;

		v[i].texcoord.x = texcoords[i].x / textureWidth;
		v[i].texcoord.y = texcoords[i].y / textureHeight;
	}

	// 頂点バッファの内容の編集を終了する。
	dc->Unmap(vertexBuffer.Get(), 0);

	// GPUに描画するためのデータを渡す
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	dc->VSSetShader(skymap_vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(skymap_pixel_shader.Get(), nullptr, 0);

	dc->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());

	// サンプラステート設定
	ID3D11SamplerState* samplerStates[] =
	{
		rc.renderState->GetSamplerState(SamplerState::LinearWrap)
	};
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// レンダーステート設定
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	// 描画
	dc->Draw(4, 0);
}