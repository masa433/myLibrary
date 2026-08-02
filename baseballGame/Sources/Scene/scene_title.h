#pragma once
#include "scene.h"
#include "camera_controller.h"
#include <DirectXMath.h>
#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <vector>
#include "RenderContext.h"
#include "sprite.h"
#include "ModelRenderer.h"
#include "SkyRenderer.h"
#include "ShadowRenderer.h"
#include "FreeCameraController.h"
#include "Hextransitioneffect.h"
#include "json.hpp"
#include "ButtonManager.h" 
#include "BloomRenderer.h"


using json = nlohmann::json;

class SceneTitle : public scene
{
public:
	SceneTitle() {}
	~SceneTitle() override {}
	void initialize() override;
	void update(float elapsed_time) override;
	void render(float elapsed_time) override;
	void uninitialize() override;
	void DrawGUI() override;

	void SaveSetting();
	void LoadSetting();

private:
	//	カメラのZ座標の描画範囲
	float camera_near_z = 1.0f;
	float camera_far_z = 1000.0f;

	//	タイトル用カメラ（1台だけでOK）
	CameraController cameraController;

	HexTransitionEffect hexTransitionEffect;
	bool isChangingScene = false;//	シーン切り替え中かどうか

	//空と太陽のレンダラー
	SkyRenderer skyRenderer;

	ShadowRenderer shadowRenderer;

	BloomRenderer bloomRenderer;

private:

	//	カスケードシャドウマップ数
	static constexpr int ShadowBufferSize = 4;

	// スポットライトシャドウマップ関連
	static constexpr int SpotShadowCount = 4; // light_max と一致させる

	//	シーン用定数バッファ構造体（stageのシェーダーが要求するレイアウトに合わせる）
	struct scene_constants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4 camera_position;
		DirectX::XMFLOAT4 camera_right;
		DirectX::XMFLOAT4 camera_up;
	};

	struct light_constants
	{
		static constexpr int light_max = 36;
		DirectX::XMFLOAT4 ambient_color;
		DirectX::XMFLOAT4 directional_light_direction;
		DirectX::XMFLOAT4 directional_light_color;
		float directional_light_intensity;
		DirectX::XMFLOAT3 dummy;
		DirectX::XMUINT4 light_count{ 0, 0, 0, 0 };
		ShadowRenderer::point_lights point_light[light_max];
		ShadowRenderer::spot_lights spot_light[6];
	};

	struct hemisphere_light_constants
	{
		DirectX::XMFLOAT4 sky_color;
		DirectX::XMFLOAT4 ground_color;
		DirectX::XMFLOAT4 hemisphere_weight;
	};

	struct fog_constants
	{
		DirectX::XMFLOAT4 fog_color;
		DirectX::XMFLOAT4 fog_range;
	};

	struct post_effect_constants
	{
		int   tone_mapping_mode = 0;
		float tone_mapping_exposure = 1.0f;
		float tone_mapping_white_point = 4.0f;
		int   pe_dummy0 = 0;

		int   toon_shading_enabled = 0;
		int   toon_diffuse_steps = 3;
		float toon_specular_threshold = 0.6f;
		float toon_specular_smoothness = 0.02f;

		float toon_rim_threshold = 0.7f;
		float toon_rim_smoothness = 0.05f;
		DirectX::XMFLOAT2 pe_dummy1 = {};

		DirectX::XMFLOAT4 toon_rim_color = { 1.0f, 1.0f, 1.0f, 0.5f };
	};

	struct shadow_quality_constants
	{
		int   soft_shadow_enabled = 0;	//	タイトルでは常に0でいい
		int   soft_shadow_samples = 9;
		float soft_shadow_radius = 1.5f;
		float shadow_map_texel_size = 1.0f / 4096.0f;
	};

private:
	//	定数バッファ本体
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> light_constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> hemisphere_light_constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> fog_constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> post_effect_constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> shadow_quality_constant_buffer;

	//	値
	DirectX::XMFLOAT3 cameraPosition = {};
	DirectX::XMFLOAT4 ambient_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 directional_light_direction{ 0.3f, -0.7f, 0.5f, 0.0f };
	DirectX::XMFLOAT4 directional_light_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	float directional_light_intensity = 1.0f;

	DirectX::XMFLOAT4 sky_color{ 0.6f, 0.7f, 0.9f, 1.0f };
	DirectX::XMFLOAT4 ground_color{ 0.3f, 0.3f, 0.3f, 1.0f };
	float hemisphere_weight = 0.5f;

	DirectX::XMFLOAT4 fog_color{ 0.5f, 0.5f, 0.5f, 1.0f };
	DirectX::XMFLOAT4 fog_range{ 100.0f, 1000.0f, 0.0f, 0.0f }; // 遠くにフォグ開始点を置いて実質無効化

	post_effect_constants post_effect_constant;
	shadow_quality_constants shadow_quality_constant;

private:
	ButtonManager buttonManager;

	struct SpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};
	std::unique_ptr<SpriteData> logoSpriteData;
	std::unique_ptr<sprite> logoSprite;

	DirectX::XMFLOAT2 logoPosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 logoSize = { 400.0f, 200.0f };
	DirectX::XMFLOAT4 logoColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	// シーン描画用のレンダーターゲットとシェーダーリソースビュー
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> scene_render_target_view;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scene_shader_resource_view;

	bool enableShadows = false;
};