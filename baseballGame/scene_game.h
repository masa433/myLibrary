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

class scene_game : public scene2
{
private:
    DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 angle{ 0.0f, 0.0f, 0.0f };



    // モデル
	std::unique_ptr<gltf_model> animated_model;
    // アニメーション関連のメンバ変数を追加
  
    float animation_time = 0.0f;
    std::vector<gltf_model::node> animated_nodes;
    int current_animation_index = 0;  // 現在再生中のアニメーションインデックス
    bool animation_playing = true;    // アニメーション再生中かどうか


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

	void RenderStrikeZone();

	void RenderShadowMap();

    // 定数バッファ構造体
    struct scene_constants
    {
        DirectX::XMFLOAT4X4 view_projection;

        DirectX::XMFLOAT4 camera_position;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;

    DirectX::XMFLOAT3   cameraPosition = {};

	// ライト関連の定数バッファ構造体
    struct light_constants
    {
        DirectX::XMFLOAT4 ambient_color;
        DirectX::XMFLOAT4 directional_light_direction;
        DirectX::XMFLOAT4 directional_light_color;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> light_constant_buffer;

    DirectX::XMFLOAT4 ambient_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_direction{ 0.0f, 1.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 directional_light_color{ 1.0f, 1.0f, 1.0f, 1.0f };


    float timeScale = 1.0f;

	

    // ストライクゾーン表示用スプライト
    std::unique_ptr<sprite> strikeZoneSprite;
    bool showStrikeZoneImage = true;
    DirectX::XMFLOAT2 spritePosition = { 560.0f, 330.0f };
    DirectX::XMFLOAT2 spriteScale = { 0.2f, 0.25f };
    DirectX::XMFLOAT4 spriteTint = { 1.0f, 1.0f, 1.0f, 1.0f };

	bool showPhysxDebug = true;

    //struct ShadowMapContext
    //{
    //    DirectX::XMFLOAT4X4 lightViewProjection;	// ライトの位置から見た射影行列
    //    DirectX::XMFLOAT3	shadowColor;			// 影色
    //    float				shadowBias;			// 深度バイアス
    //};
    //DirectX::XMFLOAT4X4     lightViewProjection;
    //float				    shadowBias = { 0.001f }; // (ToT)
    //DirectX::XMFLOAT3	    shadowColor = { 0.5f, 0.5f, 0.5f }; // (ToT)

    

    //float SHADOWMAP_DRAWRECT = { 30 };

    //Microsoft::WRL::ComPtr<ID3D11Device>			 device;
    //Microsoft::WRL::ComPtr<ID3D11Buffer>             shadowMapConstantBuffer;
    //Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   shadowMapDepthStencilView;
    //Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowMapShaderResourceView;
    //Microsoft::WRL::ComPtr<ID3D11SamplerState>       shadowMapSamplerState;
    //Microsoft::WRL::ComPtr<ID3D11VertexShader>       shadowMapCasterVertexShader;
    //Microsoft::WRL::ComPtr<ID3D11InputLayout>        shadowMapCasterInputLayout;
    //Microsoft::WRL::ComPtr<ID3D11DeviceContext> shadowContext;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> mesh_vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mesh_input_layout;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mesh_pixel_shader;
};