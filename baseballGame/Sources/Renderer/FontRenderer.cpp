// FontRenderer.cpp
#define STB_TRUETYPE_IMPLEMENTATION
#include "FontRenderer.h"

#include <d3dcompiler.h>
#include <fstream>
#include <vector>
#include <algorithm>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
	// 頂点シェーダ: CPU側で計算済みのNDC座標をそのまま使う簡易版(行列なし)
	const char* kFontVS = R"(
struct VS_IN
{
	float3 position : POSITION;
	float4 color    : COLOR;
	float2 texcoord : TEXCOORD;
};
struct VS_OUT
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
	float2 texcoord : TEXCOORD;
};
VS_OUT main(VS_IN vin)
{
	VS_OUT vout;
	vout.position = float4(vin.position, 1.0f);
	vout.color = vin.color;
	vout.texcoord = vin.texcoord;
	return vout;
}
)";

	// ピクセルシェーダ: フォントアトラスはR8(アルファのみ)。
	// サンプリングした値をアルファとして使い、頂点カラーで着色する。
	const char* kFontPS = R"(
Texture2D    fontTexture : register(t0);
SamplerState fontSampler : register(s0);

struct PS_IN
{
	float4 position : SV_POSITION;
	float4 color    : COLOR;
	float2 texcoord : TEXCOORD;
};

float4 main(PS_IN pin) : SV_TARGET
{
	float a = fontTexture.Sample(fontSampler, pin.texcoord).r;
	float4 outColor = pin.color;
	outColor.a *= a;
	if (outColor.a <= 0.001f) discard;
	return outColor;
}
)";

	bool ReadFileBytes(const wchar_t* path, std::vector<unsigned char>& out)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			wchar_t cwd[MAX_PATH] = {};
			GetCurrentDirectoryW(MAX_PATH, cwd);
			wchar_t msg[1024];
			swprintf_s(msg, L"[FontRenderer] Failed to open font: \"%s\"\n  CurrentDirectory: \"%s\"\n",
				path, cwd);
			OutputDebugStringW(msg);
			return false;
		}
		const std::streamsize size = file.tellg();
		if (size <= 0) return false;
		file.seekg(0, std::ios::beg);
		out.resize(static_cast<size_t>(size));
		return static_cast<bool>(file.read(reinterpret_cast<char*>(out.data()), size));
	}
}

// ===== UTF-8 デコード =====

std::vector<int> FontRenderer::Utf8ToCodepoints(const char* utf8Text)
{
	std::vector<int> result;
	if (!utf8Text) return result;

	const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8Text);
	while (*p)
	{
		unsigned char c = *p;
		int codepoint = 0;
		int extraBytes = 0;

		if ((c & 0x80) == 0x00) { codepoint = c; extraBytes = 0; }          // 0xxxxxxx (1byte)
		else if ((c & 0xE0) == 0xC0) { codepoint = c & 0x1F; extraBytes = 1; } // 110xxxxx (2byte)
		else if ((c & 0xF0) == 0xE0) { codepoint = c & 0x0F; extraBytes = 2; } // 1110xxxx (3byte, 日本語の大半はここ)
		else if ((c & 0xF8) == 0xF0) { codepoint = c & 0x07; extraBytes = 3; } // 11110xxx (4byte, 絵文字など)
		else
		{
			// 不正なバイト列は1バイト読み飛ばす
			++p;
			continue;
		}

		bool valid = true;
		const unsigned char* q = p + 1;
		for (int i = 0; i < extraBytes; ++i)
		{
			if (q[i] == '\0' || (q[i] & 0xC0) != 0x80) { valid = false; break; }
			codepoint = (codepoint << 6) | (q[i] & 0x3F);
		}

		if (!valid)
		{
			++p;
			continue;
		}

		result.push_back(codepoint);
		p += (1 + extraBytes);
	}
	return result;
}

bool FontRenderer::Initialize(
	ID3D11Device* device,
	const wchar_t* fontPath,
	float pixelHeight,
	int screenWidth,
	int screenHeight,
	int atlasWidth,
	int atlasHeight,
	const std::vector<int>* codepoints,
	int fontIndex)
{
	valid_ = false;
	pixelHeight_ = pixelHeight;
	screenWidth_ = screenWidth;
	screenHeight_ = screenHeight;
	atlasWidth_ = atlasWidth;
	atlasHeight_ = atlasHeight;
	glyphs_.clear();

	// TTF/OTFファイルをバイト列として読み込む
	std::vector<unsigned char> ttfBuffer;
	if (!ReadFileBytes(fontPath, ttfBuffer))
		return false;

	// ベイクするコードポイント一覧を確定する(指定が無ければASCII 32-126)
	std::vector<int> codepointList;
	if (codepoints && !codepoints->empty())
	{
		codepointList = *codepoints;
		std::sort(codepointList.begin(), codepointList.end());
		codepointList.erase(std::unique(codepointList.begin(), codepointList.end()), codepointList.end());
	}
	else
	{
		for (int c = 32; c < 127; ++c) codepointList.push_back(c);
	}

	// アトラスビットマップ(8bit, 1チャンネル)を確保してPack APIでベイクする
	std::vector<unsigned char> bitmap(static_cast<size_t>(atlasWidth_) * atlasHeight_, 0);
	std::vector<stbtt_packedchar> packedChars(codepointList.size());

	stbtt_pack_context packContext{};
	if (!stbtt_PackBegin(&packContext, bitmap.data(), atlasWidth_, atlasHeight_, 0, 1, nullptr))
		return false;

	// 小さい文字が潰れないよう軽くオーバーサンプリング
	stbtt_PackSetOversampling(&packContext, 2, 2);

	const int fontOffset = stbtt_GetFontOffsetForIndex(ttfBuffer.data(), fontIndex);
	if (fontOffset < 0)
	{
		stbtt_PackEnd(&packContext);
		OutputDebugStringW(L"[FontRenderer] Invalid font index / font collection.\n");
		return false;
	}

	stbtt_pack_range range{};
	range.font_size = pixelHeight_;
	range.array_of_unicode_codepoints = codepointList.data();
	range.num_chars = static_cast<int>(codepointList.size());
	range.chardata_for_range = packedChars.data();

	const int packResult = stbtt_PackFontRanges(&packContext, ttfBuffer.data(), fontOffset, &range, 1);
	stbtt_PackEnd(&packContext);

	if (packResult == 0)
	{
		// 0の場合: アトラスに全文字が入りきらなかった可能性が高い(atlasWidth/Heightを大きくする)
		OutputDebugStringW(L"[FontRenderer] WARNING: not all glyphs fit in the atlas. "
			L"Increase atlasWidth/atlasHeight or reduce pixelHeight/number of codepoints.\n");
		// 入らなかった文字はx1=x0=0などになるが、それでも描画は継続できるようそのまま進める
	}

	// コードポイント -> グリフ情報のマップを構築
	for (size_t i = 0; i < codepointList.size(); ++i)
	{
		glyphs_[codepointList[i]] = packedChars[i];
	}

	// D3D11テクスチャ/シェーダリソースビューを作成
	if (!CreateAtlasTexture(device, bitmap.data(), atlasWidth_, atlasHeight_))
		return false;

	// シェーダ/入力レイアウトを作成
	if (!CreateShaders(device))
		return false;

	// サンプラステート
	{
		D3D11_SAMPLER_DESC desc{};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		if (FAILED(device->CreateSamplerState(&desc, samplerState_.GetAddressOf())))
			return false;
	}

	// ブレンドステート(アルファブレンド)
	{
		D3D11_BLEND_DESC desc{};
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (FAILED(device->CreateBlendState(&desc, blendState_.GetAddressOf())))
			return false;
	}

	// ラスタライザステート(カリングなし)
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		if (FAILED(device->CreateRasterizerState(&desc, rasterizerState_.GetAddressOf())))
			return false;
	}

	valid_ = true;
	return true;
}

void FontRenderer::Uninitialize()
{
	vertexBuffer_.Reset();
	rasterizerState_.Reset();
	blendState_.Reset();
	samplerState_.Reset();
	atlasSRV_.Reset();
	atlasTexture_.Reset();
	inputLayout_.Reset();
	pixelShader_.Reset();
	vertexShader_.Reset();
	glyphs_.clear();
	vertexBufferCapacity_ = 0;
	valid_ = false;
}

bool FontRenderer::CreateShaders(ID3D11Device* device)
{
	ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;

	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompile(
		kFontVS, strlen(kFontVS), nullptr, nullptr, nullptr,
		"main", "vs_5_0", compileFlags, 0,
		vsBlob.GetAddressOf(), errBlob.GetAddressOf());
	if (FAILED(hr))
	{
		if (errBlob) OutputDebugStringA(static_cast<const char*>(errBlob->GetBufferPointer()));
		return false;
	}

	hr = D3DCompile(
		kFontPS, strlen(kFontPS), nullptr, nullptr, nullptr,
		"main", "ps_5_0", compileFlags, 0,
		psBlob.GetAddressOf(), errBlob.GetAddressOf());
	if (FAILED(hr))
	{
		if (errBlob) OutputDebugStringA(static_cast<const char*>(errBlob->GetBufferPointer()));
		return false;
	}

	if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vertexShader_.GetAddressOf())))
		return false;
	if (FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, pixelShader_.GetAddressOf())))
		return false;

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	if (FAILED(device->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inputLayout_.GetAddressOf())))
		return false;

	return true;
}

bool FontRenderer::CreateAtlasTexture(ID3D11Device* device, const unsigned char* bitmap, int width, int height)
{
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = bitmap;
	initData.SysMemPitch = width;

	if (FAILED(device->CreateTexture2D(&desc, &initData, atlasTexture_.GetAddressOf())))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	if (FAILED(device->CreateShaderResourceView(atlasTexture_.Get(), &srvDesc, atlasSRV_.GetAddressOf())))
		return false;

	return true;
}

bool FontRenderer::EnsureVertexBuffer(ID3D11Device* device, size_t requiredVertexCount)
{
	if (vertexBuffer_ && vertexBufferCapacity_ >= requiredVertexCount)
		return true;

	size_t newCapacity = std::max<size_t>(requiredVertexCount, vertexBufferCapacity_ * 2);
	newCapacity = std::max<size_t>(newCapacity, 256);

	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * newCapacity);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	vertexBuffer_.Reset();
	if (FAILED(device->CreateBuffer(&desc, nullptr, vertexBuffer_.GetAddressOf())))
		return false;

	vertexBufferCapacity_ = newCapacity;
	return true;
}

void FontRenderer::MeasureText(const char* utf8Text, float scale, float& outWidth, float& outHeight) const
{
	outWidth = 0.0f;
	outHeight = pixelHeight_ * scale;

	const std::vector<int> codepoints = Utf8ToCodepoints(utf8Text);
	float penX = 0.0f;
	for (int cp : codepoints)
	{
		auto it = glyphs_.find(cp);
		if (it == glyphs_.end())
		{
			penX += pixelHeight_ * 0.3f * scale; // 未対応文字は半角スペース分だけ進める
			continue;
		}
		penX += it->second.xadvance * scale;
	}
	outWidth = penX;
}

void FontRenderer::DrawText(
	ID3D11DeviceContext* dc,
	const char* utf8Text,
	float x, float y,
	float scale,
	float r, float g, float b, float a)
{
	if (!valid_ || !utf8Text || !*utf8Text) return;

	const std::vector<int> codepoints = Utf8ToCodepoints(utf8Text);
	std::vector<Vertex> vertices;
	vertices.reserve(codepoints.size() * 6);

	float penX = x;
	float penY = y;

	const float lineHeight = pixelHeight_ * scale * 1.2f;

	const float invW = 2.0f / static_cast<float>(screenWidth_);
	const float invH = 2.0f / static_cast<float>(screenHeight_);

	auto toNDC = [&](float px, float py) -> XMFLOAT2
		{
			return XMFLOAT2(px * invW - 1.0f, 1.0f - py * invH);
		};

	const XMFLOAT4 color(r, g, b, a);

	for (int cp : codepoints)
	{
		if (cp == '\n')
		{
			penX = x;
			penY += lineHeight;
			continue;
		}

		auto it = glyphs_.find(cp);
		if (it == glyphs_.end())
		{
			// ベイクされていない文字(改行・未対応漢字など)は半角スペース分だけ進めて無視
			penX += pixelHeight_ * 0.3f * scale;
			continue;
		}

		const stbtt_packedchar& bc = it->second;

		const float x0 = penX + bc.xoff * scale;
		const float y0 = penY + bc.yoff * scale;
		const float x1 = penX + bc.xoff2 * scale;
		const float y1 = penY + bc.yoff2 * scale;

		const float s0 = static_cast<float>(bc.x0) / atlasWidth_;
		const float t0 = static_cast<float>(bc.y0) / atlasHeight_;
		const float s1 = static_cast<float>(bc.x1) / atlasWidth_;
		const float t1 = static_cast<float>(bc.y1) / atlasHeight_;

		XMFLOAT2 p0 = toNDC(x0, y0); // 左上
		XMFLOAT2 p1 = toNDC(x1, y0); // 右上
		XMFLOAT2 p2 = toNDC(x0, y1); // 左下
		XMFLOAT2 p3 = toNDC(x1, y1); // 右下

		Vertex v0{ XMFLOAT3(p0.x, p0.y, 0.0f), color, XMFLOAT2(s0, t0) };
		Vertex v1{ XMFLOAT3(p1.x, p1.y, 0.0f), color, XMFLOAT2(s1, t0) };
		Vertex v2{ XMFLOAT3(p2.x, p2.y, 0.0f), color, XMFLOAT2(s0, t1) };
		Vertex v3{ XMFLOAT3(p3.x, p3.y, 0.0f), color, XMFLOAT2(s1, t1) };

		vertices.push_back(v0);
		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v1);
		vertices.push_back(v3);
		vertices.push_back(v2);

		penX += bc.xadvance * scale;
	}

	if (vertices.empty()) return;

	ID3D11Device* rawDevice = nullptr;
	dc->GetDevice(&rawDevice); // GetDeviceは参照カウントを+1するのでComPtrでAttachして管理する
	ComPtr<ID3D11Device> devicePtr;
	devicePtr.Attach(rawDevice);

	if (!EnsureVertexBuffer(devicePtr.Get(), vertices.size()))
		return;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(dc->Map(vertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return;
	memcpy(mapped.pData, vertices.data(), sizeof(Vertex) * vertices.size());
	dc->Unmap(vertexBuffer_.Get(), 0);

	const UINT stride = sizeof(Vertex);
	const UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout_.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dc->VSSetShader(vertexShader_.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader_.Get(), nullptr, 0);
	dc->PSSetShaderResources(0, 1, atlasSRV_.GetAddressOf());
	dc->PSSetSamplers(0, 1, samplerState_.GetAddressOf());

	const float blendFactor[4] = { 0,0,0,0 };
	dc->OMSetBlendState(blendState_.Get(), blendFactor, 0xFFFFFFFF);
	dc->RSSetState(rasterizerState_.Get());

	dc->Draw(static_cast<UINT>(vertices.size()), 0);
}