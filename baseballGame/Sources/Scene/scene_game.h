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
#include "ShadowRenderer.h"
#include "FreeCameraController.h"
#include "FrustumCulling.h"
#include "BroadcastCamera.h"
#include "json.hpp"

CONST LONG SCREEN_WIDTH{ 1920 };
CONST LONG SCREEN_HEIGHT{ 1080 };
CONST BOOL FULLSCREEN{ FALSE };

class scene_game : public scene
{
private:
   
   
    //カメラのZ座標の描画範囲
    float camera_near_z = 1.0f;
    float camera_far_z = 1000.0f;

private:
    //空と太陽のレンダラー
	SkyRenderer skyRenderer;

	ShadowRenderer shadowRenderer;

    FreeCameraController freeCameraController;
	bool useFreeCamera = false;//	フリーカメラを使用するかどうか

	FrustumCulling frustumCulling;

	BroadcastCamera broadcastCamera;

private:
    //	カスケードシャドウマップ数
    static constexpr int ShadowBufferSize = 4;

    //	ガウスフィルター
    static constexpr int KernelMax = 25;

    // スポットライトシャドウマップ関連
    static constexpr int SpotShadowCount = 4; // light_max と一致させる

    //定数バッファ構造体
    struct scene_constants
    {
        DirectX::XMFLOAT4X4 view_projection;
        DirectX::XMFLOAT4 camera_position;
        DirectX::XMFLOAT4    camera_right;
        DirectX::XMFLOAT4    camera_up;
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
        ShadowRenderer::point_lights point_light[light_max]; // 最大36のポイントライト
        ShadowRenderer::spot_lights spot_light[6]; // 最大6つのスポットライト

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

    //ポストエフェクト用定数バッファ構造体
    struct post_effect_constants
    {
        // --- トーンマッピング ---
        int   tone_mapping_mode = 0;     // 0:なし 1:Reinhard 2:ReinhardEx 3:Uncharted2 4:ACES 5:Lottes
        float tone_mapping_exposure = 1.0f;  // 露出（1.0 = 変化なし）
        float tone_mapping_white_point = 4.0f;  // ReinhardEx 用白点
        int   pe_dummy0 = 0;     // パディング

        // --- トゥーンシェーディング ---
        int   toon_shading_enabled = 0;    // 0 = 通常, 1 = トゥーン
        int   toon_diffuse_steps = 3;    // 拡散段数（2〜8）
        float toon_specular_threshold = 0.6f; // ハイライト閾値
        float toon_specular_smoothness = 0.02f;// ハイライト境界幅

        float             toon_rim_threshold = 0.7f;  // リム閾値
        float             toon_rim_smoothness = 0.05f; // リム境界幅
        DirectX::XMFLOAT2 pe_dummy1 = {};    // パディング

        DirectX::XMFLOAT4 toon_rim_color = { 1.0f, 1.0f, 1.0f, 0.5f }; // xyz=色, w=強度
	};

    struct shadow_quality_constants
    {
        // ソフトシャドウ
        int   soft_shadow_enabled = 0;    // 0=ハード, 1=PCFソフト
        int   soft_shadow_samples = 9;    // サンプル数(4/9/16/25)
        float soft_shadow_radius = 1.5f; // PCFカーネル半径(テクセル単位)
        float shadow_map_texel_size = 1.0f / 4096.0f; // シャドウマップテクセルサイズ

	};

    shadow_quality_constants shadow_quality_constant;
    Microsoft::WRL::ComPtr<ID3D11Buffer> shadow_quality_constant_buffer;

public:
    scene_game() {};
    ~scene_game() override = default;

    void initialize() override;
    
    void update(float elapsed_time) override;
    void render(float elapsedTime) override;
    void uninitialize() override;
	// GUI描画処理
	void DrawGUI() override;

	//保存・読み込み用の関数
    void SaveSetting();
	void LoadSetting();

   
   
private:
	// シーン描画用定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;

	// シーン描画用のレンダーターゲットとシェーダーリソースビュー
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> scene_render_target_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scene_shader_resource_view;

    DirectX::XMFLOAT3   cameraPosition = {};

	// ライト関連
    DirectX::XMFLOAT4 ambient_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_direction{ 0.0f, -1.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	float directional_light_intensity = 0.7f;

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

	// ポストエフェクト用定数バッファ
    post_effect_constants            post_effect_constant;
    Microsoft::WRL::ComPtr<ID3D11Buffer> post_effect_constant_buffer;

    float timeScale = 1.0f;

    TextureManager textureManager;

	bool showPhysxDebug = true;
	bool physxRenderSimpleShapesOnly = true;
	bool physxSkipSleepingActors = true;
	bool enableShadows = false;
	bool enableBloom = false;
	bool enableFrustumCulling = true;

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
    //ドローコール表示用
    Microsoft::WRL::ComPtr<ID3D11Query> pipeline_stats_query;
    D3D11_QUERY_DATA_PIPELINE_STATISTICS pipeline_stats = {};

    std::vector<std::string> consoleLog;

private:
    float trackingTime = 0.0f;
};