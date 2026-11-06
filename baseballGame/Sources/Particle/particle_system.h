#pragma once
#include "texture.h"
#include "shader.h"

class particle_system
{
private:
	struct vertex
	{
		DirectX::XMFLOAT3 position;	//	位置
		DirectX::XMFLOAT2 texcoord;	//	UV
		DirectX::XMFLOAT4 color;	//	頂点色		
		DirectX::XMFLOAT4 param;	//	汎用パラメータ
	};

	struct particle_data
	{
		float x, y, z;	//	位置
		float w, h;	//	サイズ(幅、高さ)
		float aw, ah;	//	サイズ(幅、高さ)の変化量
		float vx, vy, vz;	//	速度
		float ax, ay, az;	//	加速度
		float alpha;	//	透明度
		float timer;	//	経過時間
		float anime_timer;	//	アニメーション用経過時間
		float type;	//	パーティクルの種類
		float r, g, b;	//	色
		bool anime;	//	アニメーションするかどうか
		float anime_speed;	//	アニメーション速度
	};

	//定数バッファのデータ定義
	struct constants
	{
		DirectX::XMFLOAT2 size;	//	スクリーンサイズ
		DirectX::XMFLOAT2 dummy;	//	ダミー
	};

public:
	particle_system(ID3D11Device* device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view, int komax = 1, int komay = 1, int num = 1000);

	~particle_system();
	void Update(float elapsed_time);

	void Render(ID3D11DeviceContext* immediate_context);

	void Set(
		int type,//	パーティクルの種類
		float timer,//	パーティクルの寿命
		DirectX::XMFLOAT3 p,//	パーティクルの位置
		DirectX::XMFLOAT3 v = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),//	パーティクルの速度
		DirectX::XMFLOAT3 f = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),//	パーティクルの加速度
		DirectX::XMFLOAT2 size = DirectX::XMFLOAT2(1.0f, 1.0f),//	パーティクルのサイズ
		bool anime = false,//	アニメーションするかどうか
		float anime_speed = 24.0f,// 	アニメーション速度
		float r = 1.0f, float g = 1.0f, float b = 1.0f//	色
	);

private:
	particle_data* data;	//	パーティクル情報
	vertex* vertices;		//	頂点バッファ書き込み情報
	int num_particles = 0;	//	パーティクル数
	int komax, komay;		//	Textureの縦横分割数

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometry_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;

};