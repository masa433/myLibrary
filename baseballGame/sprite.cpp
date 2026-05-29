#include "sprite.h"
#include "misc.h"
#include <sstream>
//#include <WICTextureLoader.h>
#include "texture.h"
#include "shader.h"


//コンストラクタ
sprite::sprite(ID3D11Device* device, const wchar_t* filename)
{
	HRESULT hr{ S_OK };

	vertex vertices[]
	{
		{ { -1.0, +1.0, 0 }, { 1, 1, 1, 1 }, { 0, 0 } },
		{ { +1.0, +1.0, 0 }, { 1, 1, 1, 1 }, { 1, 0 } },
		{ { -1.0, -1.0, 0 }, { 1, 1, 1, 1 }, { 0, 1 } },
		{ { +1.0, -1.0, 0 }, { 1, 1, 1, 1 }, { 1, 1 } },
	};

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(vertices);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = vertices;
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;
	hr = device->CreateBuffer(&buffer_desc, &subresource_data, vertex_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// UNIT.05
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// UNIT.10
	create_vs_from_cso(device, "sprite_vs.cso", vertex_shader.GetAddressOf(), input_layout.GetAddressOf(), input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, "sprite_ps.cso", pixel_shader.GetAddressOf());

	// UNIT.10
	load_texture_from_file(device, filename, shader_resource_view.GetAddressOf(), &texture2d_desc);
}

void sprite::render(ID3D11DeviceContext* immediate_context,
	float dx, float dy,
	float dw, float dh,
	float r,float g,float b,float a,
	float angle)
{
	render(immediate_context, dx, dy, dw, dh, r, g, b, a, angle, 0.0f, 0.0f, static_cast<float>(texture2d_desc.Width), static_cast<float>(texture2d_desc.Height));
}

void sprite::render(ID3D11DeviceContext* immediate_context,
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
	float tex_width = static_cast<float>(texture2d_desc.Width);   // テクスチャの横幅
	float tex_height = static_cast<float>(texture2d_desc.Height); // テクスチャの縦幅

	float u0 = sx / tex_width;
	float v0 = sy / tex_height;
	float u1 = (sx + sw) / tex_width;
	float v1 = (sy + sh) / tex_height;

	// 頂点データ更新
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = immediate_context->Map(vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	vertex* vertices = reinterpret_cast<vertex*>(mapped.pData);
	if (vertices)
	{
		vertices[0].position = { x0, y0, 0 };
		vertices[1].position = { x1, y1, 0 };
		vertices[2].position = { x2, y2, 0 };
		vertices[3].position = { x3, y3, 0 };
		vertices[0].color = vertices[1].color = vertices[2].color = vertices[3].color = { r, g, b, a };

		vertices[0].texcoord = { u0, v0 };
		vertices[1].texcoord = { u1, v0 };
		vertices[2].texcoord = { u0, v1 };
		vertices[3].texcoord = { u1, v1 };
	}
	immediate_context->Unmap(vertex_buffer.Get(), 0);

	// 描画処理（バインドやDraw）
	immediate_context->PSSetShaderResources(0, 1, shader_resource_view.GetAddressOf());
	UINT stride = sizeof(vertex), offset = 0;
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	immediate_context->IASetInputLayout(input_layout.Get());
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	immediate_context->Draw(4, 0);
}

void sprite::render(ID3D11DeviceContext* immediate_context, float dx, float dy, float dw, float dh)
{
	render(immediate_context, dx, dy, dw, dh, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
		0.0f, 0.0f, static_cast<float>(texture2d_desc.Width), static_cast<float>(texture2d_desc.Height));
}


sprite::~sprite() 
{
	 /*vertex_buffer->Release();
	 vertex_shader->Release();
	pixel_shader->Release();
	input_layout->Release();
	shader_resource_view->Release();*/
}

void sprite::textout(ID3D11DeviceContext* immediate_context, std::string s,
	float x, float y, float w, float h, float r, float g, float b, float a)
{

	float sw = static_cast<float>(texture2d_desc.Width / 16);
	float sh = static_cast<float>(texture2d_desc.Height / 16);
	float carriage = 0;
	for (const char c : s)
	{

		render(immediate_context, x + carriage, y, w, h, r, g, b, a, 0,
			sw * (c & 0x0F), sh * (c >> 4), sw, sh);
		carriage += w;

	}
}

// Undo・Redo用の関数
void sprite::SaveState()
{
	undoStack.push_back(state);
	if(undoStack.size() > MAX_UNDO)
	{
		undoStack.erase(undoStack.begin());
	}
	redoStack.clear();
}

void sprite::Undo()
{
	if(undoStack.empty())
	{
		return;
	}
	redoStack.push_back(state);
	state = undoStack.back();
	undoStack.pop_back();
}

void sprite::Redo()
{
	if(redoStack.empty())
	{
		return;
	}
	undoStack.push_back(state);
	state = redoStack.back();
	redoStack.pop_back();
}

// stateを使ったシンプルなrender
void sprite::render(ID3D11DeviceContext* immediate_context)
{
	render(immediate_context,
		state.position.x, state.position.y,
		state.size.x, state.size.y,
		state.color.x, state.color.y, state.color.z, state.color.w,
		state.rotation);
}