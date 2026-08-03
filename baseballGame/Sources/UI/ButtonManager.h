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
		Close,//閉じるボタン
		Reroll,//振り直しボタン
		Title, //タイトルボタン
		BatterSelect,//打者選択ボタン
		Retry,//リトライボタン
		Count
	};


	ButtonManager() {};
	~ButtonManager() {};
	void Initialize();
	void Update(float elapsedTime);
	void Render(float alpha = 1.0f, ButtonType buttonType = ButtonType::None);
	void DrawGUI();
	void SaveToJson(nlohmann::json& j);
	void LoadFromJson(const nlohmann::json& j);
	
	//マウスカーソルがボタン上にあるかどうかを判定する関数
	bool IsMouseOverButton(const DirectX::XMFLOAT2& mousePos, const DirectX::XMFLOAT2& buttonPos, const DirectX::XMFLOAT2& buttonSize);

	void ResetStartRequest(bool requested) { isStartRequested = requested; }
	bool IsStartRequested() const { return isStartRequested; }

	void ResetOKRequest(bool requested) { isOKRequested = requested; }
	bool IsOKRequested() const { return isOKRequested; }

	void ResetReturnRequest(bool requested) { isReturnRequested = requested; }
	bool IsReturnRequested() const { return isReturnRequested; }

	void ResetCloseRequest(bool requested) { isCloseRequested = requested; }
	bool IsCloseRequested() const { return isCloseRequested; }

	void ResetRerollRequest(bool requested) { isRerollRequested = requested; }
	bool IsRerollRequested() const { return isRerollRequested; }

	void ResetTitleRequest(bool requested) { isTitleRequested = requested; }
	bool IsTitleRequested() const { return isTitleRequested; }

	void ResetRetryRequest(bool requested) { isRetryRequested = requested; }
	bool IsRetryRequested() const { return isRetryRequested; }

	void ResetBatterSelectRequest(bool requested) { isBatterSelectRequested = requested; }
	bool IsBatterSelectRequested() const { return isBatterSelectRequested; }

	//ボタンの色を変える関数
	void ChangeColor(DirectX::XMFLOAT4 color, ButtonType buttonType = ButtonType::None);

private:
	//ボタンのスプライトデータ
	struct ButtonSprite
	{
		std::wstring texturePath;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV;
		std::unique_ptr<sprite> spriteObj;
		DirectX::XMFLOAT2 position = {100.0f, 100.0f};
		DirectX::XMFLOAT2 size = {100.0f, 100.0f};
		DirectX::XMFLOAT2 originalSize = { 100.0f, 100.0f };
		float rotation = 0.0f;
		DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		std::string label;
		char labelBuffer[256] = "Button";
		ButtonType buttonType = ButtonType::None;
		float currentAlpha = 0.0f;
		bool isPressing = false;
		//押されているボタンの記録用
		
	};
	bool prevMouseDown = false;      // 前フレームでマウスが押されていたか
	ButtonSprite* pressedButton = nullptr; // 現在押し始めているボタン

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
	bool isReturnRequested = false;
	bool isCloseRequested = false;
	bool isRerollRequested = false;
	bool isTitleRequested = false;
	bool isRetryRequested = false;
	bool isBatterSelectRequested = false;
};