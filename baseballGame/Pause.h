#pragma once
#include "sprite.h"

//ポーズ画面の管理クラス
class Pause
{
public:
	static Pause& Instance()
	{
		static Pause instance;
		return instance;
	}
	void Initialize(ID3D11Device* device,ID3D11DeviceContext* context);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();

	void OpenPauseScreen() { isPauseOpen = true; }
	void ClosePauseScreen() { isPauseOpen = false; }
	bool IsPauseScreenOpen() const { return isPauseOpen; }

private:
	bool isPauseOpen = false; // ポーズ画面が開いているかどうかのフラグ

	//テクスチャデータの構造体
	struct PauseData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;


	//ポーズ画面のスプライトとデータ
	std::unique_ptr<PauseData> pauseBackGroundData;
	std::unique_ptr<sprite> pauseBackGroundSprite;

	DirectX::XMFLOAT2 pauseBackGroundPosition = { 960.0f, 540.0f };
	DirectX::XMFLOAT2 pauseBackGroundSize = { 1000.0f, 700.0f };

};