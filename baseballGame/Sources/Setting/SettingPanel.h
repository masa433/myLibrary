#pragma once
#include "sprite.h"
#include "UiEasing.h"
#include "input.h"
#include "FontRenderer.h"
#include "..\Sources\Audio\AudioSource.h"
#include "..\Sources\Audio\Audio.h"

// 設定パネルの基底クラス
class SettingPanel
{
public:
	SettingPanel() = default;
	virtual ~SettingPanel() = default;
	virtual void Initialize(ID3D11Device* device,ID3D11DeviceContext* context) = 0;
	virtual void Uninitialize() = 0;
	virtual void Update(float elapsedTime) = 0;
	virtual void Render() = 0;
	virtual void DrawGUI() = 0;
	

protected:
	
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	struct SpriteData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	FontRenderer fontRenderer;

	struct FontData
	{
		DirectX::XMFLOAT2 position;
		float size;
	};

};

class AudioSettingPanel : public SettingPanel
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Uninitialize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

private:

	FontData masterVolumeFontData = { { 960.0f, 540.0f }, 0.5f };//マスターボリュームの表示
	FontData bgmVolumeFontData = { { 960.0f, 540.0f }, 0.5f };//bgmボリュームの表示
	FontData seVolumeFontData = { { 960.0f, 540.0f }, 0.5f };//seボリュームの表示

};

class CursorSettingPanel : public SettingPanel
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Uninitialize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

private:
	
	FontData cursorFontData = { { 960.0f, 540.0f }, 0.5f };//カーソル感度の表示

};

class OtherSettingPanel : public SettingPanel
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Uninitialize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

	//チェックボックス更新処理
	void UpdateShowOperationCheckbox();	

	//ゲームスピードの表示更新処理
	//void UpdateGameSpeedDisplay();

private:
	
	FontData showOperationFontData = { { 960.0f, 540.0f }, 0.5f };//操作方法の表示
	FontData gameSpeedFontData = { { 960.0f, 540.0f }, 0.5f };//ゲームスピードの表示

	std::unique_ptr<SpriteData> showOperationCheckboxData;
	std::unique_ptr<sprite> showOperationCheckboxSprite;

	DirectX::XMFLOAT2 checkboxPosition = { 960.0f, 540.0f };
	DirectX::XMFLOAT2 checkboxSize = { 80.0f, 80.0f };

	DirectX::XMFLOAT2 checkBoxOriginalSize = { checkboxSize.x, checkboxSize.y };
	DirectX::XMFLOAT2 checkBoxHoverSize = { checkBoxOriginalSize.x * 1.1f, checkBoxOriginalSize.y * 1.1f };

	std::unique_ptr<SpriteData> checkIconData;
	std::unique_ptr<sprite> checkIconSprite;

	DirectX::XMFLOAT2 checkIconPosition = { 970.0f, 535.0f };
	DirectX::XMFLOAT2 checkIconSize = { 80.0f, 80.0f };

	float offsetX = 500.0f;
	float offsetY = -30.0f;

	bool showCheckIcon = false;

};