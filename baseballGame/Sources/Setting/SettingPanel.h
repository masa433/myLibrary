#pragma once
#include "sprite.h"
#include "UiEasing.h"
#include "input.h"
#include "FontRenderer.h"
#include "..\Sources\Audio\AudioSource.h"
#include "..\Sources\Audio\Audio.h"


class SettingPanel
{
public:
	SettingPanel() = default;
	virtual ~SettingPanel() = default;
	virtual void Initialize(ID3D11Device* device,ID3D11DeviceContext* context) = 0;
	virtual void Uninitialize() = 0;
	virtual void Update(float elapsedTime) = 0;
	virtual void Render() = 0;

	

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

private:
	
	FontData showOperationFontData = { { 960.0f, 540.0f }, 0.5f };//操作方法の表示
	FontData gameSpeedFontData = { { 960.0f, 540.0f }, 0.5f };//ゲームスピードの表示

};