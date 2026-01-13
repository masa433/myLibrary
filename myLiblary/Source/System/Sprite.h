#pragma once

#include "RenderContext.h"

// スプライト
class Sprite
{
public:
	Sprite();
	Sprite(const char* filename);


	// 頂点データ
	struct Vertex
	{
		DirectX::XMFLOAT3	position;
		DirectX::XMFLOAT4	color;
		DirectX::XMFLOAT2	texcoord;
	};

	// 描画実行
	void Render(const RenderContext& rc,
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float sx, float sy,					// 画像切り抜き位置
		float sw, float sh,					// 画像切り抜きサイズ
		float angle,						// 角度
		float r, float g, float b, float a	// 色
	) const;

	// 描画実行（テクスチャ切り抜き指定なし）
	void Render(const RenderContext& rc,
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float angle,						// 角度
		float r, float g, float b, float a	// 色
	) const;

	// 描画実行
	void Render(ID3D11DeviceContext* dc,
		float dx, float dy,
		float dw, float dh,
		float sx, float sy,
		float sw, float sh,
		float angle,
		float r, float g, float b, float a) const;

	void SkyRender(RenderContext& rc,
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float sx, float sy,					// 画像切り抜き位置
		float sw, float sh,					// 画像切り抜きサイズ
		float angle,						// 角度
		float r, float g, float b, float a // 色
	);

	// テクスチャ幅取得
	int GetTextureWidth() const { return textureWidth; }

	// テクスチャ高さ取得
	int GetTextureHeight() const { return textureHeight; }

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>			vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>			inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer>				vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11BlendState>			blendState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>		rasterizerState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		depthStencilState;

	Microsoft::WRL::ComPtr<ID3D11SamplerState>			samplerState;

	float textureWidth = 0;
	float textureHeight = 0;

	//スカイマップ
	struct skymap_constants
	{
		DirectX::XMFLOAT4X4 inverse_view_projection;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> skymap_constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> skymap_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> skymap_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> skymap_pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skymap_depth_stencil_state;
	D3D11_TEXTURE2D_DESC skymap_texture2d_desc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skymap_shader_resource_view;
	
	skymap_constants skymap_cb;
};
