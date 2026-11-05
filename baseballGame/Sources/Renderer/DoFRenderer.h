#pragma once
#include "sprite.h"
#include "Graphics.h"

class DoFRenderer
{
public:
    DoFRenderer() = default;
	~DoFRenderer() = default;
    

    // 初期化
    // device        : D3D11デバイス
    // sceneColorSRV : ブルームの元となるシーンカラーのシェーダーリソースビュー
    // screenWidth / screenHeight : 画面サイズ
    void Initialize(ID3D11Device* device, ID3D11ShaderResourceView* sceneColorSRV,
        UINT screenWidth, UINT screenHeight);
    void Uninitialize();

    //高輝度抽出からぼかしまでの処理を行う
    // view / projection / cameraPosition は現在のシーンカメラの情報
    void Extract(ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& cameraPosition,
        float near_clip_distance,
        float far_clip_distance);

    // ぼかした結果を現在セットされているレンダーターゲットに加算合成する
    void Composite(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* sceneColorSRV);

    // ImGuiでの設定UI
    void DrawGUI(float near_clip_distance, float far_clip_distance);

    // "Enable Bloom" チェックボックス（Performanceタブなど別の場所に置きたい場合用）
    void DrawEnableCheckbox();

    void SaveToJson(nlohmann::json& j) const;
    void LoadFromJson(const nlohmann::json& j);

    bool IsEnabled() const { return enable_dof; }
    void SetEnabled(bool enabled) { enable_dof = enabled; }

    //深度ステンシルビューのゲッター
	ID3D11DepthStencilView* GetDepthStencilView() const { return scene_depth_stencil_view.Get(); }

	
private:
    //被写界深度関連(のちに独自のレンダラークラスを作る予定)
    static constexpr int KernelMax = 25;

private:
    //定数バッファ構造体
    struct scene_constants
    {
        DirectX::XMFLOAT4X4  view_projection;
        DirectX::XMFLOAT4    camera_position;
        DirectX::XMFLOAT4    camera_right;
        DirectX::XMFLOAT4    camera_up;
        DirectX::XMFLOAT4    camera_clip_distance;
    };

    //シェーダー側への転送用定数バッファ
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
        int					kernel_size{ 2 };
        float				sigma{ 10.0f };
        DirectX::XMFLOAT2	texture_size{ Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight() };
    };

    //	シーン用定数バッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> scene_constant_buffer;

	gaussian_filter_datas gaussian_filter_data;
	Microsoft::WRL::ComPtr<ID3D11Buffer> gaussian_filter_constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_filter_pixel_shader;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> bokeh_scene_render_target_view;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bokeh_scene_shader_resource_view;
	std::unique_ptr<sprite> bokeh_scene_sprite;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  sprite_input_layout;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  sprite_pixel_shader;

    //ガウスフィルター
	void calculate_gaussian_filter_constant(gaussian_filter_constants& constant, const gaussian_filter_datas& data);
    //シーンのぼかし
	void generate_bokeh_scene(float elapsed_time,
        ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& cameraPosition,
        float near_clip_distance,
        float far_clip_distance);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> scene_depth_stencil_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scene_depth_shader_resource_view;

    struct depth_of_field_constants
    {
        float				focus_distance{ 25.0f };	//	焦点距離
        float				dof_range{ 20.0f };			//	焦点範囲
        DirectX::XMFLOAT2	dummy;
    };
    depth_of_field_constants depth_of_field_constant;
    Microsoft::WRL::ComPtr<ID3D11Buffer> depth_of_field_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> depth_of_field_pixel_shader;

    

	bool enable_dof{ true };

	UINT screen_width = Graphics::Instance().GetScreenWidth();
	UINT screen_height = Graphics::Instance().GetScreenHeight();
};