#include "sprite_batch.h"
#include "misc.h"
#include <sstream>
//#include <WICTextureLoader.h>
#include "texture.h"
#include "shader.h"

//コンストラクタ
sprite_batch::sprite_batch(ID3D11Device* device, const wchar_t* filename, size_t max_sprites)
	:max_vertices(max_sprites * 6)
{
	HRESULT hr{ S_OK };

	std::unique_ptr<vertex[]> vertices{ std::make_unique<vertex[]>(max_vertices) };


		//頂点バッファオブジェクトの生成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(vertex) * max_vertices);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(&buffer_desc, NULL, vertex_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// UNIT.10
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.GetAddressOf(), input_layout.GetAddressOf(), input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.GetAddressOf());

	// UNIT.10
	load_texture_from_file(device, filename, shader_resource_view.GetAddressOf(), &texture2d_desc);
}

void sprite_batch::render(ID3D11DeviceContext* immediate_context,
	// UNIT.03
	float dx, float dy, float dw, float dh,
	float r, float g, float b, float a,
	// UNIT.04
	float angle/*degree*/)
{
	// UNIT.06
	render(immediate_context, dx, dy, dw, dh, r, g, b, a, angle, 0.0f, 0.0f, static_cast<float>(texture2d_desc.Width), static_cast<float>(texture2d_desc.Height));
}

void sprite_batch::render(ID3D11DeviceContext* immediate_context,
	float dx, float dy,
	float dw, float dh,
	float r, float g, float b, float a,
	float angle,
	float sx, float sy, float sw, float sh)
{
	// ビューポート取得
	D3D11_VIEWPORT viewport{};
	UINT num_viewports{ 1 };
	immediate_context->RSGetViewports(&num_viewports, &viewport);

	// 矩形の頂点座標
	float x0{ dx }, y0{ dy };
	float x1{ dx + dw }, y1{ dy };
	float x2{ dx }, y2{ dy + dh };
	float x3{ dx + dw }, y3{ dy + dh };

	// 回転処理
	auto rotate = [](float& x, float& y, float cx, float cy, float angle)
		{
			x -= cx; y -= cy;
			float cosA = cosf(DirectX::XMConvertToRadians(angle));
			float sinA = sinf(DirectX::XMConvertToRadians(angle));
			float tx = x, ty = y;
			x = cosA * tx - sinA * ty;
			y = sinA * tx + cosA * ty;
			x += cx; y += cy;
		};

	float cx = dx + dw * 0.5f;
	float cy = dy + dh * 0.5f;
	rotate(x0, y0, cx, cy, angle);
	rotate(x1, y1, cx, cy, angle);
	rotate(x2, y2, cx, cy, angle);
	rotate(x3, y3, cx, cy, angle);

	// スクリーン座標 -> NDC
	x0 = 2.0f * x0 / viewport.Width - 1.0f;
	y0 = 1.0f - 2.0f * y0 / viewport.Height;
	x1 = 2.0f * x1 / viewport.Width - 1.0f;
	y1 = 1.0f - 2.0f * y1 / viewport.Height;
	x2 = 2.0f * x2 / viewport.Width - 1.0f;
	y2 = 1.0f - 2.0f * y2 / viewport.Height;
	x3 = 2.0f * x3 / viewport.Width - 1.0f;
	y3 = 1.0f - 2.0f * y3 / viewport.Height;

	// UV座標変換 (テクセル -> 正規化UV)
	float tex_width = texture2d_desc.Width;   // テクスチャの横幅
	float tex_height = texture2d_desc.Height; // テクスチャの縦幅

	float u0 = sx / tex_width;
	float v0 = sy / tex_height;
	float u1 = (sx + sw) / tex_width;
	float v1 = (sy + sh) / tex_height;

	vertices.push_back({ { x0, y0 , 0 }, { r, g, b, a }, { u0, v0 } });
	vertices.push_back({ { x1, y1 , 0 }, { r, g, b, a }, { u1, v0 } });
	vertices.push_back({ { x2, y2 , 0 }, { r, g, b, a }, { u0, v1 } });
	vertices.push_back({ { x2, y2 , 0 }, { r, g, b, a }, { u0, v1 } });
	vertices.push_back({ { x1, y1 , 0 }, { r, g, b, a }, { u1, v0 } });
	vertices.push_back({ { x3, y3 , 0 }, { r, g, b, a }, { u1, v1 } });

	
}

void sprite_batch::render(ID3D11DeviceContext* immediate_context, float dx, float dy, float dw, float dh)
{

	render(immediate_context,
		dx, dy,         // 描画位置
		dw, dh,         // 描画サイズ
		1.0f, 1.0f, 1.0f, 1.0f,  // 色
		0.0f,           // 回転角
		0.0f, 0.0f,     // テクスチャ左上 (sx, sy)
		static_cast<float>(texture2d_desc.Width),   // sw
		static_cast<float>(texture2d_desc.Height)); // sh

}

sprite_batch::~sprite_batch()
{
	/*vertex_buffer->Release();
	vertex_shader->Release();
	pixel_shader->Release();
	input_layout->Release();
	shader_resource_view->Release();*/
}

void sprite_batch::begin(ID3D11DeviceContext* immediate_context) 
{
	vertices.clear();
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	immediate_context->PSSetShaderResources(0, 1, shader_resource_view.GetAddressOf());
}

void sprite_batch::end(ID3D11DeviceContext* immediate_context) 
{
	HRESULT hr{ S_OK };
	D3D11_MAPPED_SUBRESOURCE mapped_subresource{};
	hr = immediate_context->Map(vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	size_t vertex_count = vertices.size();
	_ASSERT_EXPR(max_vertices >= vertex_count, "Buffer overflow");
	vertex* data{ reinterpret_cast<vertex*>(mapped_subresource.pData) };
	if (data != nullptr)
	{
		const vertex* p = vertices.data();
		memcpy_s(data, max_vertices * sizeof(vertex), p, vertex_count * sizeof(vertex));
	}
	immediate_context->Unmap(vertex_buffer.Get(), 0);

	UINT stride{ sizeof(vertex) };
	UINT offset{ 0 };
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	immediate_context->IASetInputLayout(input_layout.Get());

	immediate_context->Draw(static_cast<UINT>(vertex_count), 0);
}
