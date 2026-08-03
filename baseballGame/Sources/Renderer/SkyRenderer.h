#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "json.hpp"
using json = nlohmann::json;

class SkyRenderer
{
public:
	//空の定数バッファ構造体
	struct sky_constants
	{
		DirectX::XMFLOAT4 sun_direction;// 太陽光の方向
		DirectX::XMFLOAT4 sun_color;    // 太陽光の色
		DirectX::XMFLOAT4 sky_zenith_color;// 空の天頂の色
		DirectX::XMFLOAT4 sky_horizon_color;// 空の地平線の色
		DirectX::XMFLOAT4 sky_ground_color;// 空の地面の色（今回は使用しない）
		DirectX::XMFLOAT4 cloud_color; // 雲の色
		DirectX::XMFLOAT4 cloud_params; // 雲のパラメータ　x=被覆率、y=スケール、z=速度、w=柔らかさ
		float time_of_day; // 時刻（0.0f ～ 1.0f）
		float sun_size; // 太陽のサイズ
		float sun_bloom_size; // 太陽のブルームのサイズ
		float cloud_time; // 雲の時間（アニメーション用）
	};

	//パラメータ
	float time_of_day = 0.35f; // 時刻（0.0f ～ 1.0f）
	float time_speed = 0.01f; // 時刻の変化速度
	bool auto_advance_time = false; // 時刻の自動進行
	float sun_size = 0.025f; // 太陽のサイズ
	float sun_bloom_size = 0.12f; // 太陽のブルームのサイズ
	DirectX::XMFLOAT3 cloud_color = { 1.0f, 1.0f, 1.0f }; // 雲の色
	float cloud_coverage = 1.0f; // 雲の被覆率（0.0f～1.0f）
	float cloud_scale = 5.0f; // 雲のスケール
	float cloud_speed = 0.05f; // 雲の速度
	float cloud_softness = 1.0f; // 雲の柔らかさ（0.0f～1.0f）
	float cloud_time_accum = 0.0f; // 雲の時間の累積値（アニメーション用）

public:
	//関数
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(ID3D11DeviceContext* dc,
		ID3D11Buffer* scene_constant_buffer,  // シーン定数バッファ
		ID3D11DepthStencilState* depth_read_only,   // 深度ステンシルステート（読み取り専用）
		ID3D11RasterizerState* rasterizer_none);// ラスタライザーステート（カリングなし）

	DirectX::XMFLOAT4 GetSunDirectionToLight() const; // ライト空間での太陽光の方向を取得

	void DrawGUI();

	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	float NormalizedTimeOfDay() const;
	DirectX::XMFLOAT3 ComputeSunDirection() const; // 太陽光の方向を計算
	DirectX::XMFLOAT3 ComputeCloudColor() const; // 雲の色を計算

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer; // 定数バッファ
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader; // 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader; // ピクセルシェーダー

	void ComputeSkyColors(sky_constants& out) const; // 空の色を計算
	
};