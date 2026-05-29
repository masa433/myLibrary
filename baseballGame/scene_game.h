#pragma once
#include "scene.h"
#include "camera_controller.h"
#include <DirectXMath.h>
#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <vector>
#include "gltf_model.h"
#include "RenderContext.h"
#include "sprite.h"
#include "ModelRenderer.h"
#include "TextureManager.h"

CONST LONG SCREEN_WIDTH{ 1280 };
CONST LONG SCREEN_HEIGHT{ 720 };
CONST BOOL FULLSCREEN{ FALSE };

class scene_game : public scene2
{
private:
   
    CameraController	cameraController;


    //カメラのZ座標の描画範囲
    float camera_near_z = 1.0f;
    float camera_far_z = 1000000.0f;

public:
    scene_game() {};
    ~scene_game() override = default;

    void initialize() override;
    
    void update(float elapsed_time) override;
    void render(float elapsedTime) override;
    void uninitialize() override;
	// GUI描画処理
	void DrawGUI() override;

    void renderShadowMap();

    // 定数バッファ構造体
    struct scene_constants
    {
        DirectX::XMFLOAT4X4 view_projection;

        DirectX::XMFLOAT4 camera_position;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;

    DirectX::XMFLOAT3   cameraPosition = {};

	// ライト関連の定数バッファ構造体

    //ポイントライトの構造体
    struct point_lights
    {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT4 color;
        float intensity;
		float range;
		DirectX::XMFLOAT2 dummy; // 4の倍数にするためのダミー
	};
    
	//スポットライトの構造体
    struct spot_lights
    {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT4 direction;
        DirectX::XMFLOAT4 color;
        float range;
        float innerCorn;
		float outerCorn;
		float intensity;
		
    };

    struct light_constants
    {
        DirectX::XMFLOAT4 ambient_color;
        DirectX::XMFLOAT4 directional_light_direction;
        DirectX::XMFLOAT4 directional_light_color;
		point_lights pointLights[6]; // 最大6つのポイントライト
		spot_lights spotLights[6]; // 最大6つのスポットライト
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> light_constant_buffer;

    DirectX::XMFLOAT4 ambient_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_direction{ 0.0f, 1.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	point_lights pointLights[6];
	spot_lights spotLights[6];

    //半球ライティング
    struct hemisphere_light_constants
    {
        DirectX::XMFLOAT4 sky_color;
        DirectX::XMFLOAT4 ground_color;
		DirectX::XMFLOAT4 hemisphere_weight; // x:skyの重み、y,z,wは未使用
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> hemisphere_light_constant_buffer;
	DirectX::XMFLOAT4 sky_color{ 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT4 ground_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	float hemisphere_weight = 0.5f; // 0.0fで完全に地面の色、1.0fで完全に空の色

    //フォグ
    struct fog_constants
    {
        DirectX::XMFLOAT4 fog_color;
        DirectX::XMFLOAT4 fog_range; // x:開始距離、y:終了距離、z,wは未使用
    };
	Microsoft::WRL::ComPtr<ID3D11Buffer> fog_constant_buffer;
	DirectX::XMFLOAT4 fog_color{ 0.5f, 0.5f, 0.5f, 1.0f };
	DirectX::XMFLOAT4 fog_range{ 0.1f, 1000.0f, 0.0f, 0.0f };

    float timeScale = 1.0f;

    TextureManager textureManager;

	bool showPhysxDebug = true;

   //シャドウマップ
    struct shadowmap_constants
    {
		DirectX::XMFLOAT4X4 light_view_projection; // ライトのビュー射影行列
        float				shadow_attenuation{ 0.5f };
        float				shadow_bias{ 0.0001f };
        DirectX::XMFLOAT2	shadow_dummy;
    };

    Microsoft::WRL::ComPtr<ID3D11Buffer> shadowmap_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowmap_depth_stencil_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowmap_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowmap_sampler_state;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowmap_caster_vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> shadowmap_caster_input_layout;

    DirectX::XMFLOAT4X4 light_view_projection;
    float				shadow_bias{ 0.008f };
	float shadow_attenuation{ 0.5f };


    Microsoft::WRL::ComPtr<ID3D11VertexShader> mesh_vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mesh_input_layout;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mesh_pixel_shader;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> scene_render_target_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scene_shader_resource_view;

};