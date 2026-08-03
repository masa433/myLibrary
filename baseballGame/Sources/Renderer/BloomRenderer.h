#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "json.hpp"
#include "Graphics.h"

using json = nlohmann::json;

class BloomRenderer
{
public:
    //	ガウスフィルターの最大カーネルサイズ
    static constexpr int KernelMax = 25;

    //	高輝度抽出用の定数バッファ構造体
    struct luminance_extract_constants
    {
        float               threshold{ 0.7f };  //	高輝度抽出のための閾値
        float               intensity{ 2.0f };  //	ブルームの強度
        DirectX::XMFLOAT2   dummy;
    };

    //	シェーダー側への転送用ガウスフィルター定数バッファ
    struct gaussian_filter_constants
    {
        DirectX::XMFLOAT4   weights[KernelMax * KernelMax];
        float               kernel_size;
        DirectX::XMFLOAT2   texcel;
        float               dummy;
    };

    //	ガウスフィルター処理用情報（GUIで編集する側のデータ）
    struct gaussian_filter_datas
    {
        int                 kernel_size{ 9 };
        float               sigma{ 10.0f };
        DirectX::XMFLOAT2   texture_size{ Graphics::Instance().GetScreenWidth(), Graphics::Instance().GetScreenHeight() };
    };

private:
    //	シーン描画用定数バッファ（高輝度抽出・ぼかしパスの頂点変換に使用）
    struct scene_constants
    {
        DirectX::XMFLOAT4X4 view_projection;
        DirectX::XMFLOAT4   camera_position;
        DirectX::XMFLOAT4   camera_right;
        DirectX::XMFLOAT4   camera_up;
    };

public:
    BloomRenderer() = default;
    ~BloomRenderer() = default;

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
        const DirectX::XMFLOAT3& cameraPosition);

    // ぼかした結果を現在セットされているレンダーターゲットに加算合成する
    void Composite(ID3D11DeviceContext* dc);

    // ImGuiでの設定UI
    void DrawGUI();

    // "Enable Bloom" チェックボックス（Performanceタブなど別の場所に置きたい場合用）
    void DrawEnableCheckbox();

    void SaveToJson(nlohmann::json& j) const;
    void LoadFromJson(const nlohmann::json& j);

    bool IsEnabled() const { return enable_bloom; }
    void SetEnabled(bool enabled) { enable_bloom = enabled; }

    // GUIでプレビュー表示する際などに使用
    ID3D11ShaderResourceView* GetLuminanceExtractSRV() const { return luminance_extract_shader_resource_view.Get(); }
    ID3D11ShaderResourceView* GetBokehSRV() const { return bokeh_luminance_extract_shader_resource_view.Get(); }

private:
    void calculate_gaussian_filter_constant(gaussian_filter_constants& constant, const gaussian_filter_datas& data);

    // 高輝度抽出を行うパス
    void luminance_extract_pass(ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& cameraPosition);

    // 高輝度抽出バッファをぼかす
    void bokeh_luminance_extract_pass(ID3D11DeviceContext* dc,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& projection,
        const DirectX::XMFLOAT3& cameraPosition);

private:
    UINT screen_width = Graphics::Instance().GetScreenWidth();
    UINT screen_height = Graphics::Instance().GetScreenHeight();

    bool enable_bloom = true;

    //	シーン用定数バッファ（高輝度抽出・ぼかしパスの頂点変換に使用）
    Microsoft::WRL::ComPtr<ID3D11Buffer> scene_constant_buffer;

    //	2D描画関係（加算合成・各パスの描画に使用するスプライト用シェーダー）
    Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  sprite_input_layout;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  sprite_pixel_shader;

    //	高輝度抽出関係
    luminance_extract_constants luminance_extract_constant;
    Microsoft::WRL::ComPtr<ID3D11Buffer>              luminance_extract_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>    luminance_extract_render_target_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  luminance_extract_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>         luminance_extract_pixel_shader;
    std::unique_ptr<sprite>                           luminance_extract_pass_sprite;

    //	ガウスフィルター関係
    gaussian_filter_datas gaussian_filter_data;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       gaussian_filter_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  gaussian_filter_pixel_shader;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   bokeh_luminance_extract_render_target_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bokeh_luminance_extract_shader_resource_view;
    std::unique_ptr<sprite>                          bokeh_luminance_extract_pass_sprite;

    //	ぼかした結果を加算合成時に描画するスプライト
    std::unique_ptr<sprite> add_luminance_extract_pass_sprite;
};