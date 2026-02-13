#pragma once
#include "scene.h"
#include "camera.h"
#include "camera_controller.h"
#include "Light.h"
#include <DirectXMath.h>
#include <memory>
#include <wrl.h>
#include <d3d11.h>
#include <vector>
#include "gltf_model.h"
#include "RenderContext.h"
#include "sprite.h"

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

    Camera				camera;
    CameraController	cameraController;
    Light			    light;

    //カメラのZ座標の描画範囲
    float camera_near_z = 1.0f;
    float camera_far_z = 10000.0f;

public:
    scene_game();
    ~scene_game() override = default;

    void initialize() override;
    
    void update(float elapsed_time) override;
    void render(float elapsedTime) override;
    void uninitialize() override;
	// GUI描画処理
	void DrawGUI() override;

	void RenderStrikeZone();

    // 定数バッファ構造体
    struct scene_constants
    {
        DirectX::XMFLOAT4X4 view_projection;
        DirectX::XMFLOAT4 light_direction;
        DirectX::XMFLOAT4 camera_position;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;

    float timeScale = 1.0f;



    // ストライクゾーン表示用スプライト
    std::unique_ptr<sprite> strikeZoneSprite;
    bool showStrikeZoneImage = true;
    DirectX::XMFLOAT2 spritePosition = { 560.0f, 330.0f };
    DirectX::XMFLOAT2 spriteScale = { 0.2f, 0.25f };
    DirectX::XMFLOAT4 spriteTint = { 1.0f, 1.0f, 1.0f, 1.0f };

};