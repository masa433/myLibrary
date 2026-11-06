#include "pch.h"
#include "particle_system.h"

particle_system::particle_system(ID3D11Device* device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view,
	int komax, int komay, int num)
	:komax(komax),
	komay(komay), 
	num_particles(num)
{
	HRESULT hr;

	if(!device || !shader_resource_view)
	{
		throw std::runtime_error("Invalid device pointer or shader resource view");
	}

	//パーティクル情報リスト
	data = new particle_data[num];
	ZeroMemory(data, sizeof(particle_data) * num);//	初期化

	//頂点情報リスト
	vertices = new vertex[num];
	ZeroMemory(vertices, sizeof(vertex) * num);//	初期化

	//	パーティクル情報の初期化
	for (int i = 0; i < num_particles; i++) { data[i].type = -1; }

	//パーティクル用画像ロード
	this->shader_resource_view = shader_resource_view;

	//頂点バッファ作成
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;//	書き込み不可
	//頂点数分のバッファ
	bd.ByteWidth = sizeof(vertex) * num_particles;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	hr = device->CreateBuffer(&bd, nullptr, vertex_buffer.GetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("Failed to create vertex buffer"); }

	//定数バッファ作成
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(constants);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;
	hr = device->CreateBuffer(&cbd, nullptr, constant_buffer.GetAddressOf());
	if (FAILED(hr)) { throw std::runtime_error("Failed to create constant buffer"); }

	//	頂点シェーダー
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "PARAMETER", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\geometry_particle_vs.cso", vertex_shader.ReleaseAndGetAddressOf(), input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
	create_gs_from_cso(device, ".\\resources\\shader\\geometry_particle_gs.cso", geometry_shader.ReleaseAndGetAddressOf());
	create_ps_from_cso(device, ".\\resources\\shader\\geometry_particle_ps.cso", pixel_shader.ReleaseAndGetAddressOf());

}

particle_system::~particle_system()
{
	delete[] data;
	delete[] vertices;
}

void particle_system::Update(float elapsed_time)
{
	for(int i =0; i < num_particles; i++)
	{
		if (data[i].type < 0) continue;//	パーティクルが存在しない場合はスキップ
		
		data[i].vx += data[i].ax * elapsed_time;//	速度に加速度を加算
		data[i].vy += data[i].ay * elapsed_time;
		data[i].vz += data[i].az * elapsed_time;

		data[i].x += data[i].vx * elapsed_time;//	位置に速度を加算
		data[i].y += data[i].vy * elapsed_time;
		data[i].z += data[i].vz * elapsed_time;

		data[i].timer -= elapsed_time;//	経過時間を減算
		data[i].alpha = sqrtf(data[i].timer);//	透明度を経過時間に応じて変化させる

		//アニメ
		if (data[i].anime)
		{
			data[i].type += elapsed_time * data[i].anime_speed;//	アニメーション用経過時間を加算
		}

		//終了判定
		if(data[i].timer <= 0.0f)
		{
			data[i].type = -1;//	パーティクルの種類を-1にすることで、パーティクルが存在しないことを示す
		}
	}
}

void particle_system::Render(ID3D11DeviceContext* immediate_context)
{
	//定数バッファの更新
	constants cb;
	cb.size = { 0.1f,0.1f };
	immediate_context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &cb, 0, 0);//	定数バッファの更新
	immediate_context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());//	定数バッファの設定
	immediate_context->GSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());//	定数バッファの設定
	immediate_context->PSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());//	定数バッファの設定

	//点描画設定
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);//	点描画

	//シェーダー設定
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->GSSetShader(geometry_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	//入力レイアウト設定
	immediate_context->IASetInputLayout(input_layout.Get());

	//テクスチャ設定
	immediate_context->PSSetShaderResources(0, 1, shader_resource_view.GetAddressOf());

	//パーティクル情報を頂点バッファに転送
	
	//パーティクル発生数
	int particle_count = 0;
	for(int i = 0; i < num_particles; i++)
	{
		if (data[i].type < 0) continue;//	パーティクルが存在しない場合はスキップ
		vertices[particle_count].position = { data[i].x, data[i].y, data[i].z };//	位置
		vertices[particle_count].texcoord = { data[i].w, data[i].h };//	UV
		vertices[particle_count].color = { data[i].r, data[i].g, data[i].b, data[i].alpha};//	頂点色

		vertices[particle_count].param.x = 0;//	汎用パラメータ
		vertices[particle_count].param.y = data[i].type;//	汎用パラメータ
		vertices[particle_count].param.z = (float)komax;//横コマ数
		vertices[particle_count].param.w = (float)komay;//縦コマ数
		++particle_count;
	}

	//頂点データ更新
	immediate_context->UpdateSubresource(vertex_buffer.Get(), 0, nullptr, vertices, 0, 0);

	//バーテックスバッファをセット
	UINT stride = sizeof(vertex);//	頂点のサイズ
	UINT offset = 0;//	オフセット
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	//パーティクル情報分描画
	immediate_context->Draw(particle_count, 0);

	//シェーダーの設定を解除
	immediate_context->VSSetShader(nullptr, nullptr, 0);
	immediate_context->GSSetShader(nullptr, nullptr, 0);
	immediate_context->PSSetShader(nullptr, nullptr, 0);
}

void particle_system::Set(int type, float timer, DirectX::XMFLOAT3 p, DirectX::XMFLOAT3 v, DirectX::XMFLOAT3 f, DirectX::XMFLOAT2 size, bool anime, float anime_speed, float r, float g, float b)
{
	for (int i = 0; i < num_particles; i++)
	{
		if (data[i].type >= 0) continue;
		data[i].type = (float)type;
		data[i].x = p.x;// 位置を設定
		data[i].y = p.y;
		data[i].z = p.z;
		data[i].vx = v.x;// 速度を設定	
		data[i].vy = v.y;
		data[i].vz = v.z;
		data[i].ax = f.x;// 加速度を設定
		data[i].ay = f.y;
		data[i].az = f.z;
		data[i].w = size.x;// サイズを設定
		data[i].h = size.y;
		data[i].r = r;// 色を設定
		data[i].g = g;
		data[i].b = b;
		data[i].alpha = 1.0f;
		data[i].timer = timer;// 寿命を設定
		data[i].anime = anime;// アニメーションするかどうかを設定
		data[i].anime_speed = anime_speed;// アニメーション速度を設定
		break;
	}
}
