#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>
#include "Player.h"
#include "Pitcher.h"
#include "stage.h"
#include "RenderContext.h"
#include "ModelRenderer.h"

class ShadowRenderer
{
	// シャドウマップのサイズ定数
	static constexpr UINT ShadowmapSize = 4096;
	static constexpr UINT SpotShadowmapSize = 4096;
	static constexpr float ShadowmapDrawRect = 60.0f;
	static constexpr int ShadowBufferSize = 4; // カスケードシャドウマップの数
	static constexpr int SpotShadowCount = 4; // スポットシャドウマップの数

public:
	//定数バッファ構造体
    struct scene_constants
    {
        DirectX::XMFLOAT4X4 view_projection;
        DirectX::XMFLOAT4 camera_position;
        DirectX::XMFLOAT4    camera_right;
        DirectX::XMFLOAT4    camera_up;
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

public:
	void Initialize();
	void Uninitialize();

	//シャドウマップの描画
    void RenderShadowMap(float elapsedTime);
	void RenderCascadeShadowMap(float elapsedTime);
	void RenderSpotShadowMap(float elapsedTime);

	void SetDirectionalLight(const DirectX::XMFLOAT4& direction, const DirectX::XMFLOAT4& color, float intensity);
	void SetCameraPosition(const DirectX::XMFLOAT3& position);

    //シェーダーにバインド・解除
	void BindShadowResources(ID3D11DeviceContext* dc) const;
	void UnbindShadowResources(ID3D11DeviceContext* dc) const;

    //ImGui/SaveSetting用のゲッター
    ID3D11ShaderResourceView* GetShadowmapSRV() const;
	ID3D11ShaderResourceView* GetCascadeShadowmapSRV(int index) const;

    //ライト配列へのアクセス
	std::vector<point_lights>& GetPointLights() { return pointLights; }
	std::vector<spot_lights>& GetSpotLights() { return spotLights; }

    // scene_game から直接触る公開パラメータ
    bool   use_cascade_shadow_map = true;
    float  shadow_bias = 0.008f;
    float  shadow_attenuation = 0.5f;
    int    spot_shadow_update_interval = 3;
    int    spot_shadow_frame_count = 0;
    cascade_shadowmap_constants cascade_shadow_constant;

    

private:
        //GPU リソース
        Microsoft::WRL::ComPtr<ID3D11VertexShader>        shadowmap_caster_vertex_shader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>         shadowmap_caster_input_layout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>              constant_buffer;   // b1書き込み用
        Microsoft::WRL::ComPtr<ID3D11Buffer>              shadowmap_constant_buffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>    shadowmap_depth_stencil_view;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  shadowmap_shader_resource_view;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>        shadowmap_sampler_state;
        DirectX::XMFLOAT4X4                              light_view_projection;

        Microsoft::WRL::ComPtr<ID3D11Buffer>              cascade_shadowmap_constant_buffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>    cascade_shadowmap_depth_stencil_views[ShadowBufferSize];
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  cascade_shadowmap_shader_resource_views[ShadowBufferSize];

        Microsoft::WRL::ComPtr<ID3D11Buffer>              spot_shadowmap_constant_buffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>    spot_shadowmap_depth_stencil_views[SpotShadowCount];
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  spot_shadowmap_shader_resource_views[SpotShadowCount];
        spot_shadowmap_constants                          spot_shadow_constant{};

        // ライトデータ
        DirectX::XMFLOAT4         directional_light_direction{ 0,-1,0,1 };
        DirectX::XMFLOAT4         directional_light_color{ 1, 1,1,1 };
        float                     directional_light_intensity = 0.7f;
        DirectX::XMFLOAT3         cameraPosition{};
        std::vector<point_lights> pointLights;
        std::vector<spot_lights>  spotLights;
};