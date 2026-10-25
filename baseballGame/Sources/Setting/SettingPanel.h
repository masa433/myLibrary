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
};

class AudioSettingPanel : public SettingPanel
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Uninitialize() override;
	void Update(float elapsedTime) override;
	void Render() override;

private:
	std::unique_ptr<SpriteData> audioPanelBackgroundData;
	std::unique_ptr<sprite> audioPanelBackgroundSprite;

};

class CursorSettingPanel : public SettingPanel
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Uninitialize() override;
	void Update(float elapsedTime) override;
	void Render() override;
};

class GameSpeedSettingPanel : public SettingPanel
{
public:
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
	void Uninitialize() override;
	void Update(float elapsedTime) override;
	void Render() override;
};