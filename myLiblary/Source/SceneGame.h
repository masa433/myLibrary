#pragma once
#include"Stage.h"
#include "Object.h"
#include"CameraController.h"
#include "Scene.h"
#include "System/AudioSource.h"
#include "System/Audio.h"
#include "System/Sprite.h"
#include "System/FreeCameraController.h"

// ゲームシーン
class SceneGame : public Scene
{
public:
	SceneGame() {};
	~SceneGame() override {};

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize()override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render() override;

	// GUI描画
	void DrawGUI() override;
	
	void ShadowMapInit();

	void RenderShadowMap();

private:
	std::unique_ptr<Stage> stage = nullptr;
	//Player* player = nullptr;
	Camera* camera = nullptr;
	CameraController* cameraController = nullptr;
	std::unique_ptr<FreeCameraController> freeCameraController = std::make_unique<FreeCameraController>();
	//AudioSource* gameBGM = nullptr;
	DirectX::XMFLOAT3 lightDirection = { 0.0f, -1.0f, -1.0f };
	DirectX::XMFLOAT3   cameraPosition = {};
	std::unique_ptr<Object> net = nullptr;
	float timeScale = 1.0f;

private:
	// シャドウマップ用定数バッファ
	struct shadowmap_constants
	{
		DirectX::XMFLOAT4X4 light_view_projection; // ライトの位置から見た射影行列
		DirectX::XMFLOAT3 shadow_color;            // 影色
		float shadow_bias;                         // 深度バイアス
	};

	// シャドウマップ用リソース
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11Buffer> shadowMap_Constant_Buffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowMap_Depth_Stencil_View;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowMap_Shader_Resource_View;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowMap_Sampler_State;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shadowMap_Caster_Vertex_Shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> shadowMap_Caster_Input_Layout;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> shadowContext;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cacheRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cacheDepthStencilView;
	DirectX::XMFLOAT4X4 light_view_projection;
	float shadow_bias{ 0.008f };
	DirectX::XMFLOAT3 shadow_color{ 0.3f, 0.3f, 0.3f };

	// 必要に応じて追加
	Microsoft::WRL::ComPtr<ID3D11PixelShader> shadowMap_Caster_Pixel_Shader; // SV_DEPTH返すPS
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowMap_Rasterizer_State; // カスタムラスタライザ
	Microsoft::WRL::ComPtr<ID3D11Buffer> shadowMap_Object_Constant_Buffer; // モデルごとのワールド行列用

	std::unique_ptr<Sprite> skymapSprite;

};
