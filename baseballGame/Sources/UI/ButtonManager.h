#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <string>
#include <memory>
#include "imgui.h"
#include "json.hpp"
#include "FontRenderer.h"
#include "sprite.h"
#include "sceneManager.h"

using json = nlohmann::json;

class ButtonManager
{
public:

	ButtonManager() {};
	~ButtonManager() {};
	void Initialize();
	void Update(float elapsedTime, float alpha = 1.0f);
	void Render(float alpha = 1.0f);
	void DrawGUI();
	void SaveToJson(nlohmann::json& j);
	void LoadFromJson(const nlohmann::json& j);
	void ResetStartRequest() { isStartRequested = false; }
	bool IsStartRequested() const { return isStartRequested; }
	//マウスカーソルがボタン上にあるかどうかを判定する関数
	bool IsMouseOverButton(const DirectX::XMFLOAT2& mousePos, const DirectX::XMFLOAT2& buttonPos, const DirectX::XMFLOAT2& buttonSize);


	void ResetOKRequest(bool requested) { isOKRequested = requested; }
	bool IsOKRequested() const { return isOKRequested; }

	//ボタンの種類
	enum class ButtonType
	{
		None,
		Start,//スタートボタン
		Settings,//設定ボタン
		Quit,//終了ボタン
		Pose,//ポーズボタン
		Return,//戻るボタン
		OK,//決定ボタン
		Count
	};

	

private:
	//ボタンのスプライトデータ
	struct ButtonSprite
	{
		std::wstring texturePath;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV;
		std::unique_ptr<sprite> spriteObj;
		DirectX::XMFLOAT2 position = {100.0f, 100.0f};
		DirectX::XMFLOAT2 size = {100.0f, 100.0f};
		float rotation = 0.0f;
		DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		std::string label;
		char labelBuffer[256] = "Button";
		ButtonType buttonType = ButtonType::None;
	};
	
	std::unique_ptr<std::vector<ButtonSprite>> buttonSpriteData;
	std::unique_ptr<FontRenderer> fontRenderer;

	DirectX::XMFLOAT2 fontPosition = { 0.0f, 0.0f };
	float fontSize = 1.0f;
	DirectX::XMFLOAT4 fontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	// シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	// ImGui編集用に追加
	std::wstring OpenTextureFileDialog();
	void LoadButtonTexture(ButtonSprite& button, const std::wstring& path);
	static std::string WideToUtf8(const std::wstring& wide);

	bool isStartRequested = false;
	bool isOKRequested = false;
};