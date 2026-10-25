#include "pch.h"
#include "SettingManager.h"
#include "shader.h"
#include "Graphics.h"

void SettingManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// ここで各パネルを初期化し、panelsベクターに追加する
	panels.push_back(std::make_unique<AudioSettingPanel>());
	panels.push_back(std::make_unique<CursorSettingPanel>());
	panels.push_back(std::make_unique<GameSpeedSettingPanel>());
	for (auto& panel : panels)
	{
		panel->Initialize(device, context);
	}

	isOpen = false;

	// デフォルトで最初のパネルをアクティブにする
	if (!panels.empty())
	{
		currentActivePanel = panels[0].get();
	}

	//シェーダー作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	closeButton.Initialize();

}

void SettingManager::Uninitialize()
{
	for (auto& panel : panels)
	{
		panel->Uninitialize();
	}
	panels.clear();
	currentActivePanel = nullptr;

	closeButton.Uninitialize();
}

void SettingManager::Update(float elapsedTime)
{
	if (currentActivePanel && isOpen)
	{
		currentActivePanel->Update(elapsedTime);
	}

	if(isOpen)
	{
		closeButton.Update(elapsedTime);
		if (closeButton.IsCloseRequested())
		{
			Close();
			closeButton.ResetCloseRequest(false);
		}
	}
	
}

void SettingManager::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);


	if (currentActivePanel && isOpen)
	{
		currentActivePanel->Render();
	}

	closeButton.Render();

	//シェーダーのリセット
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void SettingManager::DrawGUI()
{
	/*if (currentActivePanel && isOpen)
	{
		currentActivePanel->DrawGUI();
	}*/

	closeButton.DrawGUI();
}

void SettingManager::SaveToJson(nlohmann::json& j)
{
	/*for (size_t i = 0; i < panels.size(); ++i)
	{
		nlohmann::json panelJson;
		panels[i]->SaveToJson(panelJson);
		j["panels"][i] = panelJson;
	}*/
	closeButton.SaveToJson(j["closeButton"]);
}

void SettingManager::LoadFromJson(const nlohmann::json& j)
{
	/*for (size_t i = 0; i < panels.size(); ++i)
	{
		if (j.contains("panels") && j["panels"].contains(i))
		{
			panels[i]->LoadFromJson(j["panels"][i]);
		}
	}*/
	if (j.contains("closeButton"))
	{
		closeButton.LoadFromJson(j["closeButton"]);
	}
}