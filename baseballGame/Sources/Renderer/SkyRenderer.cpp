#include "SkyRenderer.h"
#include "shader.h"
#include "misc.h"
#include "imgui.h"
#include <cmath>

using namespace DirectX;

//カラーテーブル

// 時刻に応じた空の色を計算
// テーブル上の t は 0.0f～1.0f の正規化時刻（0.0fが夜、0.5fが昼、1.0fが再び夜）
// ※ time_of_day（0～24時）からはNormalizedTimeOfDay()で変換してから参照する
struct SkyColorKey
{
	float t;// 時刻（正規化 0.0f～1.0f）
	XMFLOAT3 zenith;// 天頂の色
	XMFLOAT3 horizon;// 地平線の色
	XMFLOAT3 ground;// 地面の色（今回は使用しない）
	XMFLOAT3 sun_color;// 太陽の色
	float sun_intensity;// 太陽の強度
};

// 時刻と対応する空の色のキー
static const SkyColorKey kColorTable[] =
{
	{ 0.00f, {0.00f,0.00f,0.05f},  {0.02f,0.02f,0.08f},  {0.02f,0.01f,0.00f},  {0.0f ,0.0f ,0.0f }, 0.0f },  // midnight
	{ 0.18f, {0.01f,0.01f,0.10f},  {0.05f,0.05f,0.12f},  {0.03f,0.02f,0.01f},  {0.0f ,0.0f ,0.0f }, 0.0f },  // pre-dawn
	{ 0.22f, {0.05f,0.05f,0.20f},  {0.40f,0.20f,0.10f},  {0.10f,0.06f,0.03f},  {1.0f ,0.5f ,0.1f }, 0.6f },  // sunrise
	{ 0.30f, {0.15f,0.30f,0.65f},  {0.70f,0.60f,0.40f},  {0.12f,0.10f,0.07f},  {1.0f ,0.8f ,0.5f }, 1.0f },  // morning
	{ 0.50f, {0.10f,0.30f,0.75f},  {0.55f,0.65f,0.80f},  {0.15f,0.12f,0.10f},  {1.0f ,0.97f,0.9f }, 1.4f },  // noon
	{ 0.70f, {0.12f,0.28f,0.70f},  {0.65f,0.55f,0.35f},  {0.12f,0.10f,0.07f},  {1.0f ,0.8f ,0.5f }, 1.0f },  // afternoon
	{ 0.78f, {0.05f,0.05f,0.20f},  {0.55f,0.25f,0.10f},  {0.10f,0.06f,0.03f},  {1.0f ,0.5f ,0.1f }, 0.6f },  // sunset (18:43頃): 赤橙色のピーク
	{ 0.83f, {0.01f,0.01f,0.12f},  {0.08f,0.05f,0.18f},  {0.03f,0.02f,0.01f},  {0.2f ,0.05f,0.0f }, 0.1f },  // dusk (19:55頃): 赤みを一気に消して深い紫/藍色に沈める
	{ 0.87f, {0.00f,0.00f,0.06f},  {0.03f,0.03f,0.09f},  {0.02f,0.01f,0.00f},  {0.0f ,0.0f ,0.0f }, 0.0f },  // nightfall (20:52頃): すでに完全な夜色へ到達
	{ 1.00f, {0.00f,0.00f,0.05f},  {0.02f,0.02f,0.08f},  {0.02f,0.01f,0.00f},  {0.0f ,0.0f ,0.0f }, 0.0f },  // midnight again
};

// 線形補間関数
static XMFLOAT3 LerpFloat3(const XMFLOAT3& a, const XMFLOAT3& b, float t)
{
	return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

// time_of_day（0～24時）を 0.0f～1.0f の正規化時刻に変換
// 内部の色テーブルや太陽方向の計算は従来通り0～1の値を前提にしているため、
// 外部からは24時間表記で扱いつつ、参照する直前だけここで変換する
float SkyRenderer::NormalizedTimeOfDay() const
{
	constexpr float kHoursPerDay = 24.0f;

	float t = fmodf(time_of_day, kHoursPerDay) / kHoursPerDay;
	if (t < 0.0f) t += 1.0f; // 負の値の場合は正の範囲に変換
	return t;
}

// 時刻に応じた太陽の方向を計算
XMFLOAT3 SkyRenderer::ComputeSunDirection() const
{
	float normalized = NormalizedTimeOfDay();
	float angle = (normalized * 2.0f - 0.5f) * XM_PI;// 時刻を角度に変換（0.0fが夜、0.5fが昼、1.0fが再び夜）

	// 太陽の方向を計算（z軸を中心に回転）
	float sun_x = cosf(angle);
	float sun_y = sinf(angle);
	float sun_z = 0.0f; // 水平に回転させるだけなのでzは常に0

	XMFLOAT3 direction = { sun_x, sun_y, sun_z };
	XMVECTOR v = XMVector3Normalize(XMLoadFloat3(&direction));
	XMStoreFloat3(&direction, v);
	return direction;
}

// 時刻に応じた空の色を計算
XMFLOAT4 SkyRenderer::GetSunDirectionToLight() const
{
	XMFLOAT3 sun_dir = ComputeSunDirection();
	return XMFLOAT4(-sun_dir.x, -sun_dir.y, -sun_dir.z, 0.0f); // ライト空間では太陽光の方向は逆になる
}



// 時刻に応じた空の色を計算
void SkyRenderer::ComputeSkyColors(sky_constants& out) const
{
	float t = NormalizedTimeOfDay(); // 0.0f～1.0fの範囲に正規化された時刻

	int count = (int)(sizeof(kColorTable) / sizeof(kColorTable[0]));// カラーテーブルのエントリ数

	// 時刻に応じた2つのキーを見つける
	int lo = 0, hi = 1;
	for (int i = 0; i < count - 1; ++i)
	{
		// 時刻tがkColorTable[i]とkColorTable[i + 1]の間にあるか確認
		if (t >= kColorTable[i].t && t <= kColorTable[i + 1].t)
		{
			lo = i;
			hi = i + 1;
			break;
		}
	}

	float span = kColorTable[hi].t - kColorTable[lo].t; // 2つのキーの時刻の差
	float alpha = (span > 0.0001f) ? (t - kColorTable[lo].t) / span : 0.0f; // 補間係数

	const SkyColorKey& a = kColorTable[lo];// 2つのキーを線形補間して空の色を計算
	const SkyColorKey& b = kColorTable[hi];// 補間して結果を出力

	auto ToFloat4 = [](const XMFLOAT3& v, float w) -> XMFLOAT4 {
		return { v.x, v.y, v.z, w };
		};

	out.sky_zenith_color = ToFloat4(LerpFloat3(a.zenith, b.zenith, alpha), 1.0f);
	out.sky_horizon_color = ToFloat4(LerpFloat3(a.horizon, b.horizon, alpha), 1.0f);
	out.sky_ground_color = ToFloat4(LerpFloat3(a.ground, b.ground, alpha), 1.0f);

	XMFLOAT3 sun_rgb = LerpFloat3(a.sun_color, b.sun_color, alpha);
	float sun_intensity = a.sun_intensity + (b.sun_intensity - a.sun_intensity) * alpha;
	out.sun_color = { sun_rgb.x * sun_intensity, sun_rgb.y * sun_intensity, sun_rgb.z * sun_intensity, 1.0f };

	XMFLOAT3 sun_dir = ComputeSunDirection();
	out.sun_direction = { sun_dir.x, sun_dir.y, sun_dir.z, 0.0f };
	out.time_of_day = t; // シェーダー側は従来通り0.0f～1.0fの正規化時刻を受け取る
	out.sun_size = sun_size;
	out.sun_bloom_size = sun_bloom_size;
	out.cloud_color = { cloud_color.x, cloud_color.y, cloud_color.z, 1.0f };
	out.cloud_params = { cloud_coverage, cloud_scale, cloud_speed, cloud_softness };// 雲のパラメータを設定
	out.cloud_time = cloud_time_accum; // 雲の時間を設定（アニメーション用）
}

//初期化
void SkyRenderer::Initialize(ID3D11Device* device)
{
	HRESULT hr;

	// Load shaders (adjust paths to match your project layout)
	hr = create_vs_from_cso(device, ".\\resources\\shader\\sky_vs.cso", vertex_shader.ReleaseAndGetAddressOf(), nullptr, nullptr, 0);
	if (FAILED(hr)) return;

	hr = create_ps_from_cso(device, ".\\resources\\shader\\sky_ps.cso", pixel_shader.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return;

	// Constant buffer
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = sizeof(sky_constants);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = device->CreateBuffer(&bd, nullptr, constant_buffer.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return;

}

//更新
void SkyRenderer::Update(float elapsedTime)
{
	constexpr float kHoursPerDay = 24.0f;

	if (auto_advance_time)
	{
		// time_speed は「1秒あたり何時間進めるか」を表す
		time_of_day = fmodf(time_of_day + elapsedTime * time_speed, kHoursPerDay);
		if (time_of_day < 0.0f) time_of_day += kHoursPerDay;
	}

	cloud_time_accum += elapsedTime;// 雲の時間を累積（アニメーション用）

	//時刻が6時から18時までは雲の色を白、それ以外はグレーにする
	if(time_of_day >= 6.0f && time_of_day <= 18.0f)
	{
		cloud_color = { 1.0f, 1.0f, 1.0f }; // 昼間は白い雲
	}
	else
	{
		cloud_color = { 0.3f, 0.3f, 0.3f }; // 夜間はグレーの雲
	}
	
}

//描画
void SkyRenderer::Render(ID3D11DeviceContext* dc, ID3D11Buffer* scene_constant_buffer, ID3D11DepthStencilState* depth_read_only, ID3D11RasterizerState* rasterizer_none)
{
	//定数バッファの更新
	sky_constants constants;
	ComputeSkyColors(constants);// 空の色を計算
	dc->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &constants, 0, 0);

	//バインド
	dc->PSSetConstantBuffers(9, 1, constant_buffer.GetAddressOf()); // ピクセルシェーダーの定数バッファスロット1にバインド
	dc->PSSetConstantBuffers(1, 1, &scene_constant_buffer); // シーン定数バッファをピクセルシェーダーの定数バッファスロット2にバインド

	//シェーダー設定
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);

	dc->IASetInputLayout(nullptr); // 頂点レイアウトは不要（頂点シェーダーでSV_VertexIDを使用しているため）
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トポロジーは三角形リスト

	//深度ステンシルステートとラスタライザーステートを設定
	dc->OMSetDepthStencilState(depth_read_only, 0);
	dc->RSSetState(rasterizer_none);

	//描画
	dc->Draw(3, 0); // フルスクリーン三角形を描画
}

void SkyRenderer::DrawGUI()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("Sky & Sun"))
	{
		// time_of_day は 0.0f(0時)～24.0f(24時) で表示・編集する
		ImGui::SliderFloat("Time of Day (h)", &time_of_day, 0.0f, 24.0f, "%.2f h");
		ImGui::Checkbox("Auto Advance", &auto_advance_time);
		if (auto_advance_time)
			ImGui::SliderFloat("Time Speed (h/sec)", &time_speed, 0.001f, 5.0f);

		// Show computed sun direction
		XMFLOAT3 d = ComputeSunDirection();
		ImGui::Text("Sun Dir: (%.2f, %.2f, %.2f)", d.x, d.y, d.z);

		// Time labels（判定は従来通り正規化時刻 0～1 ベースで行う）
		float t = NormalizedTimeOfDay();
		const char* label =
			(t < 0.15f || t > 0.90f) ? "Night" :
			(t < 0.23f) ? "Pre-Dawn" :
			(t < 0.28f) ? "Sunrise" :
			(t < 0.45f) ? "Morning" :
			(t < 0.55f) ? "Noon" :
			(t < 0.72f) ? "Afternoon" :
			(t < 0.80f) ? "Sunset" : "Dusk";
		ImGui::Text("Phase: %s  (%02d:%02d)", label, (int)time_of_day, (int)((time_of_day - (int)time_of_day) * 60.0f));

		//デーゲームとナイターをボタンで設定
		if (ImGui::Button("Set Daytime (14:00)"))
		{
			time_of_day = 14.0f;
			auto_advance_time = false;
		}
		if (ImGui::Button("Set Nighttime (21:00)"))
		{
			time_of_day = 21.0f;
			auto_advance_time = false;
		}

		if (ImGui::CollapsingHeader("Cloud"))
		{
			ImGui::ColorEdit3("Cloud Color", &cloud_color.x);
			ImGui::SliderFloat("Cloud Coverage", &cloud_coverage, 0.0f, 1.0f);
			ImGui::SliderFloat("Cloud Scale", &cloud_scale, 0.1f, 5.0f);
			ImGui::SliderFloat("Cloud Speed", &cloud_speed, 0.0f, 1.0f);
			ImGui::SliderFloat("Cloud Softness", &cloud_softness, 0.0f, 1.0f);
		}
	}
#endif
}

void SkyRenderer::SaveToJson(json& j)
{
	j["time_of_day"] = time_of_day; // 0.0f～24.0fの時刻として保存
	j["time_speed"] = time_speed;
	j["auto_advance_time"] = auto_advance_time;
	j["sun_size"] = sun_size;
	j["sun_bloom_size"] = sun_bloom_size;
}

void SkyRenderer::LoadFromJson(const json& j)
{
	if (j.contains("time_of_day")) time_of_day = j["time_of_day"].get<float>();
	if (j.contains("time_speed")) time_speed = j["time_speed"].get<float>();
	if (j.contains("auto_advance_time")) auto_advance_time = j["auto_advance_time"].get<bool>();
	if (j.contains("sun_size")) sun_size = j["sun_size"].get<float>();
	if (j.contains("sun_bloom_size")) sun_bloom_size = j["sun_bloom_size"].get<float>();
}

void SkyRenderer::Uninitialize()
{
	constant_buffer.Reset();
	vertex_shader.Reset();
	pixel_shader.Reset();
}