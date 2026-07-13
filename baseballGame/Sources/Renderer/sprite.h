#pragma once
#include <d3d11.h>
#include <directXmath.h>
#include <string>
#include <wrl.h>

class sprite 
{
public:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;
	D3D11_TEXTURE2D_DESC texture2d_desc;
	bool isLoadFile = false;

	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,//矩形の左上の座標
		float dw, float dh,//矩形のサイズ
		float r,float g,float b,float a,
		float angle/*degree*/);

	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,//矩形の左上の座標
		float dw, float dh,//矩形のサイズ
		float r, float g, float b, float a,
		float angle/*degree*/,
		float sx,float sy,float sw, float sh);

	void render(ID3D11DeviceContext* immediate_context, float dx, float dy, float dw, float dh);

	sprite(ID3D11Device* device, const wchar_t* filename);
	sprite(ID3D11Device* device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view);
	~sprite();

	void textout(ID3D11DeviceContext* immediate_context, std::string s,
		float x, float y, float w, float h, float r, float g, float b, float a);

	//頂点フォーマット
	struct vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texcoord;
	};
};