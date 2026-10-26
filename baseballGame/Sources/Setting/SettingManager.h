#pragma once
#include "SettingPanel.h"
#include "ButtonManager.h"

class SettingManager
{
public:

    //シングルトン
    static SettingManager& Instance()
    {
        static SettingManager instance;
        return instance;
	}

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Uninitialize();
    void Update(float elapsedTime);
    void Render();
	void DrawGUI();

	void SaveToJson(nlohmann::json& j);
	void LoadFromJson(const nlohmann::json& j);

    // 特定のパネルを開く関数など
    void OpenPanel(int index);

    void Open() { isOpen = true; }
    void Close() { isOpen = false; }
    bool IsOpen() const { return isOpen; }

private:
    bool isOpen = false;

    // すべてのパネルを基底クラスのポインタでまとめて管理
    std::vector<std::unique_ptr<SettingPanel>> panels;

    // 現在アクティブなパネル
    SettingPanel* currentActivePanel = nullptr;

    ButtonManager closeButton;

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

	std::unique_ptr<SpriteData> settingBackgroundData;
	std::unique_ptr<sprite> settingBackgroundSprite;

	DirectX::XMFLOAT2 settingBackgroundPosition = { 960.0f, 540.0f };
	DirectX::XMFLOAT2 settingBackgroundSize = { 1200.0f, 700.0f };


};