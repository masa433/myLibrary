#include "pch.h"
#include "SettingManager.h"
#include "shader.h"
#include "Graphics.h"
#include "input.h"

void SettingManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (!device || !context) return;
	// ここで各パネルを初期化し、panelsベクターに追加する
	panels.push_back(std::make_unique<AudioSettingPanel>());
	panels.push_back(std::make_unique<CursorSettingPanel>());
	panels.push_back(std::make_unique<OtherSettingPanel>());
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


	//テクスチャの読み込み
	settingBackgroundData = std::make_unique<SpriteData>();
	settingBackgroundData->texturePath = L".\\resources\\textures\\settingBoard.png";
	settingBackgroundData->position = { settingBackgroundPosition.x, settingBackgroundPosition.y };
	settingBackgroundData->size = { settingBackgroundSize.x, settingBackgroundSize.y };
	settingBackgroundData->rotation = 0.0f;
	settingBackgroundData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	settingBackgroundSprite = std::make_unique<sprite>(device, context, settingBackgroundData->texturePath.c_str());

	

	//タブボタンの初期化
	//オーディオタブボタン
	{
		TabButtonData audioTabButton;
		audioTabButton.tabButtonData = std::make_unique<SpriteData>();
		audioTabButton.tabButtonData->texturePath = L".\\resources\\textures\\audioTabButton.png";
		audioTabButton.tabButtonData->position = { 600.0f, 350.0f };
		audioTabButton.tabButtonData->size = { originalTabButtonSize.x, originalTabButtonSize.y };
		audioTabButton.tabButtonData->rotation = 0.0f;
		audioTabButton.tabButtonData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		audioTabButton.tabButtonSprite = std::make_unique<sprite>(device, context, audioTabButton.tabButtonData->texturePath.c_str());
		audioTabButton.panelIndex = 0; // オーディオパネルのインデックス

		//最初のタブボタンの位置を保存
		firstTabButtonPosition = audioTabButton.tabButtonData->position;

		tabButtons.push_back(std::move(audioTabButton));
	}

	//カーソルタブボタン
	{
		TabButtonData cursorTabButton;
		cursorTabButton.tabButtonData = std::make_unique<SpriteData>();
		cursorTabButton.tabButtonData->texturePath = L".\\resources\\textures\\cursorTabButton.png";
		cursorTabButton.tabButtonData->position = { 960.0f, 350.0f };
		cursorTabButton.tabButtonData->size = { originalTabButtonSize.x, originalTabButtonSize.y };
		cursorTabButton.tabButtonData->rotation = 0.0f;
		cursorTabButton.tabButtonData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		cursorTabButton.tabButtonSprite = std::make_unique<sprite>(device, context, cursorTabButton.tabButtonData->texturePath.c_str());
		cursorTabButton.panelIndex = 1; // カーソルパネルのインデックス
		tabButtons.push_back(std::move(cursorTabButton));
	}

	//その他タブボタン
	{
		TabButtonData otherTabButton;
		otherTabButton.tabButtonData = std::make_unique<SpriteData>();
		otherTabButton.tabButtonData->texturePath = L".\\resources\\textures\\otherTabButton.png";
		otherTabButton.tabButtonData->position = { 1320.0f, 350.0f };
		otherTabButton.tabButtonData->size = { originalTabButtonSize.x, originalTabButtonSize.y };
		otherTabButton.tabButtonData->rotation = 0.0f;
		otherTabButton.tabButtonData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		otherTabButton.tabButtonSprite = std::make_unique<sprite>(device, context, otherTabButton.tabButtonData->texturePath.c_str());
		otherTabButton.panelIndex = 2; // その他パネルのインデックス
		tabButtons.push_back(std::move(otherTabButton));
	}

	//アンダーバーの初期化
	underBarData = std::make_unique<SpriteData>();
	underBarData->texturePath = L".\\resources\\textures\\pitchTypeButton.png";
	//アンダーバーの初期位置は最初のタブボタンの位置に設定
	underBarData->position = { firstTabButtonPosition.x, firstTabButtonPosition.y + underBarOffsetY };
	underBarData->size = { 200.0f, 5.0f };
	underBarData->rotation = 0.0f;
	underBarData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	underBarSprite = std::make_unique<sprite>(device, context, underBarData->texturePath.c_str());



	hoveredTabButtonIndex = -1; // 初期化

	closeButton.Initialize();

}

void SettingManager::Uninitialize()
{
	for (auto& panel : panels)
	{
		panel->Uninitialize();
	}
	//panels.clear();
	currentActivePanel = nullptr;

	settingBackgroundData.reset();
	settingBackgroundSprite.reset();
	underBarData.reset();
	underBarSprite.reset();

	closeButton.Uninitialize();

	//シェーダーの解放
	spritePS.Reset();
	spriteVS.Reset();
	spriteInputLayout.Reset();

	//タブボタンの解放
	tabButtons.clear();
}

void SettingManager::UpdateSelectedTabButton()
{
	Input& input = Input::Instance();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	//マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY());
	bool clicked = input.GetMouse().GetButtonDown() & Mouse::BTN_LEFT;//左クリックが押されたかどうか

	int previousHoveredIndex = hoveredTabButtonIndex;// 前回のマウスオーバーしていたタブボタンのインデックスを保存
	hoveredTabButtonIndex = -1; // 初期化

	//タブボタンのアイコン範囲をチェック
	for(auto& tabButton : tabButtons)
	{
		DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(tabButton.tabButtonData->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(tabButton.tabButtonData->size);
		DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };

		float iconX = scaledCenterPos.x;
		float iconY = scaledCenterPos.y;
		float iconWidth = scaledSize.x;
		float iconHeight = scaledSize.y;

		bool isHovered = (mousePos.x >= iconX &&
			mousePos.x <= iconX + iconWidth &&
			mousePos.y >= iconY &&
			mousePos.y <= iconY + iconHeight);


		bool isPressed = isHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT);// 左クリックが押されているかどうか
		
		//押されている時のサイズとマウスオーバー時のサイズを変更
		tabButton.tabButtonData->size = isPressed ? originalTabButtonSize : (isHovered ? targetTabButtonSize : originalTabButtonSize); // ボタンのサイズを変更

		if (isHovered)
		{
			hoveredTabButtonIndex = tabButton.panelIndex; // マウスオーバーしているタブボタンのインデックスを保存
			if (clicked)
			{
				currentActivePanel = panels[tabButton.panelIndex].get(); // 対応するパネルをアクティブにする

				//アンダーバーの位置を更新
				underBarData->position = { tabButton.tabButtonData->position.x, tabButton.tabButtonData->position.y + underBarOffsetY };

			}
			break; // マウスオーバーしているボタンが見つかったらループを抜ける
		}
	}

}

void SettingManager::Update(float elapsedTime)
{
	UpdateSelectedTabButton();

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

	if (settingBackgroundData && settingBackgroundSprite)
	{
		// スケーリングを適用して描画
		DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(settingBackgroundPosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(settingBackgroundSize);
		DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };

		settingBackgroundSprite->render(dc,
			scaledCenterPos.x, scaledCenterPos.y,
			scaledSize.x, scaledSize.y,
			settingBackgroundData->color.x, settingBackgroundData->color.y, settingBackgroundData->color.z, settingBackgroundData->color.w,
			settingBackgroundData->rotation);
	}

	// タブボタンの描画
	for(auto& tabButton : tabButtons)
	{
		if (tabButton.tabButtonData && tabButton.tabButtonSprite)
		{
			DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(tabButton.tabButtonData->position);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(tabButton.tabButtonData->size);

			DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };
			tabButton.tabButtonSprite->render(dc,
				scaledCenterPos.x, scaledCenterPos.y,
				scaledSize.x, scaledSize.y,
				tabButton.tabButtonData->color.x, tabButton.tabButtonData->color.y, tabButton.tabButtonData->color.z, tabButton.tabButtonData->color.w,
				tabButton.tabButtonData->rotation);
		}
	}

	// アンダーバーの描画
	if (underBarData && underBarSprite)
	{
		DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(underBarData->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(underBarData->size);
		DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };
		underBarSprite->render(dc,
			scaledCenterPos.x, scaledCenterPos.y,
			scaledSize.x, scaledSize.y,
			underBarData->color.x, underBarData->color.y, underBarData->color.z, underBarData->color.w,
			underBarData->rotation);
	}


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
	if (currentActivePanel && isOpen)
	{
		currentActivePanel->DrawGUI();
	}

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