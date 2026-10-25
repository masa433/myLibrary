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

};