#include "pch.h"
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Misc.h"
#include "GpuResourceUtils.h"

// 頂点シェーダー読み込み
HRESULT GpuResourceUtils::LoadVertexShader(
	ID3D11Device* device,
	const char* filename,
	const D3D11_INPUT_ELEMENT_DESC inputElementDescs[],
	UINT inputElementCount,
	ID3D11InputLayout** inputLayout,
	ID3D11VertexShader** vertexShader)
{
	if (!device || !filename || !vertexShader || (inputLayout && (!inputElementDescs || inputElementCount == 0))) return E_INVALIDARG;
	// ファイルを開く
	FILE* fp = nullptr;
	fopen_s(&fp, filename, "rb");
	if (!fp) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

	// ファイルのサイズを求める
	if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return E_FAIL; }
	long size = ftell(fp);
	if (size <= 0 || fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return E_FAIL; }

	// メモリ上に頂点シェーダーデータを格納する領域を用意する
	std::unique_ptr<u_char[]> data = std::make_unique<u_char[]>(size);
	const bool readOk = fread(data.get(), static_cast<size_t>(size), 1, fp) == 1;
	fclose(fp);
	if (!readOk) return E_FAIL;

	// 頂点シェーダー生成
	HRESULT hr = device->CreateVertexShader(data.get(), size, nullptr, vertexShader);
	if (FAILED(hr) || !*vertexShader) return FAILED(hr) ? hr : E_FAIL;

	// 入力レイアウト
	if (inputLayout != nullptr)
	{
		hr = device->CreateInputLayout(inputElementDescs, inputElementCount, data.get(), size, inputLayout);
		if (FAILED(hr) || !*inputLayout) return FAILED(hr) ? hr : E_FAIL;
	}

	return hr;
}

// ピクセルシェーダー読み込み
HRESULT GpuResourceUtils::LoadPixelShader(
	ID3D11Device* device,
	const char* filename,
	ID3D11PixelShader** pixelShader)
{
	if (!device || !filename || !pixelShader) return E_INVALIDARG;
	// ファイルを開く
	FILE* fp = nullptr;
	fopen_s(&fp, filename, "rb");
	if (!fp) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

	// ファイルのサイズを求める
	if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return E_FAIL; }
	long size = ftell(fp);
	if (size <= 0 || fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return E_FAIL; }

	// メモリ上に頂点シェーダーデータを格納する領域を用意する
	std::unique_ptr<u_char[]> data = std::make_unique<u_char[]>(size);
	const bool readOk = fread(data.get(), static_cast<size_t>(size), 1, fp) == 1;
	fclose(fp);
	if (!readOk) return E_FAIL;

	// ピクセルシェーダー生成
	HRESULT hr = device->CreatePixelShader(data.get(), size, nullptr, pixelShader);
	if (FAILED(hr) || !*pixelShader) return FAILED(hr) ? hr : E_FAIL;

	return hr;
}

// テクスチャ読み込み
HRESULT GpuResourceUtils::LoadTexture(
	ID3D11Device* device,
	const char* filename,
	ID3D11ShaderResourceView** shaderResourceView,
	D3D11_TEXTURE2D_DESC* texture2dDesc)
{
	if (!device || !filename || !shaderResourceView) return E_INVALIDARG;
	*shaderResourceView = nullptr;
	int width, height, bpp;
	unsigned char* pixels = stbi_load(filename, &width, &height, &bpp, STBI_rgb_alpha);
	if (pixels == nullptr)
	{
		return E_INVALIDARG;
	}

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA data;
	::memset(&data, 0, sizeof(data));
	data.pSysMem = pixels;
	data.SysMemPitch = width * 4;

	Microsoft::WRL::ComPtr<ID3D11Texture2D>	texture;
	HRESULT hr = device->CreateTexture2D(&desc, &data, texture.GetAddressOf());
	if (FAILED(hr) || !texture) { stbi_image_free(pixels); return FAILED(hr) ? hr : E_FAIL; }

	// 後始末
	stbi_image_free(pixels);

	// シェーダーリソースビュー作成
	hr = device->CreateShaderResourceView(texture.Get(), nullptr, shaderResourceView);
	if (FAILED(hr) || !*shaderResourceView) return FAILED(hr) ? hr : E_FAIL;

	// テクスチャ情報取得
	if (texture2dDesc != nullptr)
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;
		if (!*shaderResourceView) return E_FAIL;
		(*shaderResourceView)->GetResource(resource.GetAddressOf());
		if (!resource) return E_FAIL;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = resource->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());
		if (FAILED(hr) || !texture2d) return FAILED(hr) ? hr : E_FAIL;

		texture2d->GetDesc(texture2dDesc);
	}
	return hr;
}

// ダミーテクスチャ作成
HRESULT GpuResourceUtils::CreateDummyTexture(
	ID3D11Device* device,
	UINT color,
	ID3D11ShaderResourceView** shaderResourceView,
	D3D11_TEXTURE2D_DESC* texture2dDesc)
{
	if (!device || !shaderResourceView) return E_INVALIDARG;
	*shaderResourceView = nullptr;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA data{};
	data.pSysMem = &color;
	data.SysMemPitch = desc.Width;

	Microsoft::WRL::ComPtr<ID3D11Texture2D>	texture;
	HRESULT hr = device->CreateTexture2D(&desc, &data, texture.GetAddressOf());
	if (FAILED(hr) || !texture) return FAILED(hr) ? hr : E_FAIL;

	hr = device->CreateShaderResourceView(texture.Get(), nullptr, shaderResourceView);
	if (FAILED(hr) || !*shaderResourceView) return FAILED(hr) ? hr : E_FAIL;

	// テクスチャ情報取得
	if (texture2dDesc != nullptr)
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;
		if (!*shaderResourceView) return E_FAIL;
		(*shaderResourceView)->GetResource(resource.GetAddressOf());
		if (!resource) return E_FAIL;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = resource->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());
		if (FAILED(hr) || !texture2d) return FAILED(hr) ? hr : E_FAIL;
		texture2d->GetDesc(texture2dDesc);
	}

	return hr;
}

// 定数バッファ作成
HRESULT GpuResourceUtils::CreateConstantBuffer(
	ID3D11Device* device,
	UINT bufferSize,
	ID3D11Buffer** constantBuffer)
{
	if (!device || !constantBuffer || bufferSize == 0) return E_INVALIDARG;
	D3D11_BUFFER_DESC desc{};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.ByteWidth = bufferSize;
	desc.StructureByteStride = 0;

	HRESULT hr = device->CreateBuffer(&desc, 0, constantBuffer);
	if (FAILED(hr) || !*constantBuffer) return FAILED(hr) ? hr : E_FAIL;

	return hr;
}
