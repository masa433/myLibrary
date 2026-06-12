#pragma once
#include "scene.h"
#include "camera_controller.h"
#include <DirectXMath.h>
#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <vector>
#include "../Model/gltf_model.h"
#include "RenderContext.h"
#include "sprite.h"
#include "ModelRenderer.h"
#include "TextureManager.h"
#include "SkyRenderer.h"

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

private:
    //空と太陽のレンダラー
	SkyRenderer skyRenderer;

private:
    //	カスケードシャドウマップ数
    static constexpr int ShadowBufferSize = 4;

    //	ガウスフィルター
    static constexpr int KernelMax = 25;

    // スポットライトシャドウマップ関連
    static constexpr int SpotShadowCount = 4; // light_max と一致させる

     // 定数バッファ構造体
    struct scene_constants
    {
        DirectX::XMFLOAT4X4 view_projection;
        DirectX::XMFLOAT4 camera_position;
        DirectX::XMFLOAT4    camera_right;
        DirectX::XMFLOAT4    camera_up;   
    };

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
        static constexpr int light_max = 36;
        DirectX::XMFLOAT4 ambient_color;
        DirectX::XMFLOAT4 directional_light_direction;
        DirectX::XMFLOAT4 directional_light_color;
		float directional_light_intensity;
		DirectX::XMFLOAT3 dummy; // 4の倍数にするためのダミー
        DirectX::XMUINT4	light_count{ 0, 0, 0, 0 };	//	x : 空き, y : ポイントライト数, z : スポットライト数, w : 空き。
        point_lights point_light[light_max]; // 最大36のポイントライト
        spot_lights spot_light[6]; // 最大6つのスポットライト

    };

    //シャドウマップ
    struct shadowmap_constants
    {
        DirectX::XMFLOAT4X4 light_view_projection; // ライトのビュー射影行列
        float				shadow_attenuation{ 0.5f };
        float				shadow_bias{ 0.0001f };
        bool 				use_cascade;
        float	            shadow_dummy;
    };


    //	カスケードシャドウマップ用定数バッファ
    struct cascade_shadowmap_constants
    {
        DirectX::XMFLOAT4X4 light_view_projection[ShadowBufferSize];		//	ライトの位置から見た射影行列
        DirectX::XMFLOAT4	shadow_bias{ 0.001f, 0.002f, 0.003f, 0.004f };	//	深度比較用のオフセット値
        float				shadow_attenuation{ 0.5f };	//	影色
        bool				display_cascade_area;
        DirectX::XMFLOAT2	shadow_dummy;
    };

	//スポットシャドウマップ用定数バッファ
    struct spot_shadowmap_constants
    {
        DirectX::XMFLOAT4X4 light_view_projection[SpotShadowCount];
        float shadow_attenuation{ 0.5f };
        float shadow_bias{ 0.005f };
        DirectX::XMFLOAT2 dummy;
    };

    //半球ライティング
    struct hemisphere_light_constants
    {
        DirectX::XMFLOAT4 sky_color;
        DirectX::XMFLOAT4 ground_color;
        DirectX::XMFLOAT4 hemisphere_weight; // x:skyの重み、y,z,wは未使用
    };

    //フォグ
    struct fog_constants
    {
        DirectX::XMFLOAT4 fog_color;
        DirectX::XMFLOAT4 fog_range; // x:開始距離、y:終了距離、z,wは未使用
    };

    //ブルーム
    struct luminance_extract_constants
    {
        float				threshold{ 0.7f };	//	高輝度抽出のための閾値
        float				intensity{ 2.0f };	//	ブルームの強度
        DirectX::XMFLOAT2	dummy;

    };

    //	シェーダー側への転送用定数バッファ
    struct gaussian_filter_constants
    {
        DirectX::XMFLOAT4	weights[KernelMax * KernelMax];
        float				kernel_size;
        DirectX::XMFLOAT2	texcel;
        float				dummy;
    };

    //	ガウスフィルター処理用情報
    struct gaussian_filter_datas
    {
        int					kernel_size{ 9 };
        float				sigma{ 10.0f };
        DirectX::XMFLOAT2	texture_size{ SCREEN_WIDTH, SCREEN_HEIGHT };
    };

public:
    scene_game() {};
    ~scene_game() override = default;

    void initialize() override;
    
    void update(float elapsed_time) override;
    void render(float elapsedTime) override;
    void uninitialize() override;
	// GUI描画処理
	void DrawGUI() override;

    void renderShadowMap(float elapsedTime);

   
private:
	// シーン描画用定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;

	// シーン描画用のレンダーターゲットとシェーダーリソースビュー
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> scene_render_target_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scene_shader_resource_view;

    DirectX::XMFLOAT3   cameraPosition = {};

 
    cascade_shadowmap_constants cascade_shadow_constant;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cascade_shadowmap_constant_buffer;

	// ライト関連
    DirectX::XMFLOAT4 ambient_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_direction{ 0.0f, -1.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	float directional_light_intensity = 0.7f;
    std::vector<point_lights> pointLights;
    std::vector<spot_lights> spotLights;
    Microsoft::WRL::ComPtr<ID3D11Buffer> light_constant_buffer;

   

	// 半球ライティング用定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> hemisphere_light_constant_buffer;
	DirectX::XMFLOAT4 sky_color{ 0.0f, 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT4 ground_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	float hemisphere_weight = 0.5f; // 0.0fで完全に地面の色、1.0fで完全に空の色

	// フォグ用定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> fog_constant_buffer;
	DirectX::XMFLOAT4 fog_color{ 0.5f, 0.5f, 0.5f, 1.0f };
	DirectX::XMFLOAT4 fog_range{ 0.1f, 1000.0f, 0.0f, 0.0f };

    float timeScale = 1.0f;

    TextureManager textureManager;

	bool showPhysxDebug = true;

	// シャドウマップ関連
    Microsoft::WRL::ComPtr<ID3D11Buffer> shadowmap_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowmap_depth_stencil_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowmap_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowmap_sampler_state;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowmap_caster_vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> shadowmap_caster_input_layout;

    DirectX::XMFLOAT4X4 light_view_projection;
    float				shadow_bias{ 0.008f };
	float shadow_attenuation{ 0.5f };


private:	
    //	2D描画関係
    Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> sprite_input_layout;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> sprite_pixel_shader;

    
	//	高輝度抽出関係
	luminance_extract_constants luminance_extract_constant;

    Microsoft::WRL::ComPtr<ID3D11Buffer> luminance_extract_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> luminance_extract_render_target_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> luminance_extract_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> luminance_extract_pixel_shader;
    std::unique_ptr<sprite>	luminance_extract_pass_sprite;

    //	高輝度抽出を行うパス
    void luminance_extract_pass(float elapsed_time);


   
	//	ガウスフィルター関係
    gaussian_filter_datas gaussian_filter_data;
    Microsoft::WRL::ComPtr<ID3D11Buffer> gaussian_filter_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_filter_pixel_shader;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> bokeh_luminance_extract_render_target_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bokeh_luminance_extract_shader_resource_view;
    std::unique_ptr<sprite>	bokeh_luminance_extract_pass_sprite;

    //	ガウスフィルター
    void calculate_gaussian_filter_constant(gaussian_filter_constants& constant, const gaussian_filter_datas& data);

    //	高輝度抽出バッファをぼかす
    void bokeh_luminance_extract_pass(float elapsed_time);

    //	ぼかした結果を書き込む
    std::unique_ptr<sprite>	add_luminance_extract_pass_sprite;


private:
	//カスケードシャドウマップ
   
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cascade_shadowmap_depth_stencil_views[ShadowBufferSize];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cascade_shadowmap_shader_resource_views[ShadowBufferSize];

    void renderCascadeShadowMap(float elapsed_time);

    bool	use_cascade_shadow_map = true;

private:
	//スポットシャドウマップ
    Microsoft::WRL::ComPtr<ID3D11Buffer>             spot_shadowmap_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   spot_shadowmap_depth_stencil_views[SpotShadowCount];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> spot_shadowmap_shader_resource_views[SpotShadowCount];
    spot_shadowmap_constants spot_shadow_constant;

	void renderSpotShadowMap(float elapsedTime);

    // 影の更新頻度を下げる
	int spot_shadow_update_interval = 3; // 影の更新間隔（秒）
	int spot_shadow_frame_count = 0; // フレームカウンタ

private:
    //ドローコール表示用
    Microsoft::WRL::ComPtr<ID3D11Query> pipeline_stats_query;
    D3D11_QUERY_DATA_PIPELINE_STATISTICS pipeline_stats = {};


};