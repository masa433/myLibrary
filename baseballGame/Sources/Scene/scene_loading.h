#pragma once
#include "scene.h"
#include <thread>
#include <memory>
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "sprite.h"
#include "shader.h"

class scene_loading : public scene
{
public:


	scene_loading(scene* nextScene) : nextScene(nextScene) {}
	~scene_loading() override {}

	void initialize() override;
	void uninitialize() override;
	void update(float elapsed_time) override;
	void render(float elapsed_time) override;
	void DrawGUI() override;

private:
	//ローディングスレッド
	static void LoadingThread(scene_loading* scene);

private:

	std::unique_ptr<scene> nextScene;
	std::unique_ptr<std::thread> thread;

private:

	//スプライトデータ
	struct SpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};
	std::unique_ptr<SpriteData> loadingBackSpriteData;
	std::unique_ptr<sprite> loadingBackSprite;
	DirectX::XMFLOAT2 spritePosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 spriteSize = { 1920.0f, 1080.0f };
	DirectX::XMFLOAT4 spriteColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	//透明度アニメーション
	float alpha = 0.0f;
	float alphaSpeed = 1.0f; // 透明度の変化速度
	float fadeIndelayTime = 1.0f; // フェードインの遅延時間
	
	bool isFadingOut = false; // フェードアウト中かどうかのフラグ

};