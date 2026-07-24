#include "ScrollView.h"
#include "Graphics.h"
#include <shader.h>
#include <imgui.h>
#include "input.h"

ScrollView::ScrollView(ID3D11Device* device, float topX, float topY, float width, float height)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	// スクロール背景スプライトの初期化
	scrollBackgroundSpriteData.push_back(ScrollBackData());
	scrollBackgroundSpriteData[0].texturePath = L".\\resources\\textures\\scrollViewBack.png"; // スクロール背景のテクスチャパス
	scrollBackgroundSpriteData[0].position = { topX, topY }; // 中心位置に設定
	scrollBackgroundSpriteData[0].size = { width, height }; // 幅と高さを設定
	scrollBackgroundSpriteData[0].rotation = 0.0f; // 回転なし
	scrollBackgroundSpriteData[0].color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
	scrollBackgroundSprite.push_back(std::make_unique<sprite>(device, context, scrollBackgroundSpriteData[0].texturePath.c_str()));

	float capHeight = 90.0f; // 上下のフタの高さ
	topCapData.texturePath = L".\\resources\\textures\\scrollViewBack.png";
	topCapData.position = { topX, topY - height / 2.0f + capHeight / 7.0f };
	topCapData.size = { width, capHeight };
	topCapData.rotation = 0.0f;
	topCapData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	topCapSprite = std::make_unique<sprite>(device, context, topCapData.texturePath.c_str());

	bottomCapData.texturePath = L".\\resources\\textures\\scrollViewBack.png";
	bottomCapData.position = { topX, topY + height / 2.0f - capHeight / 7.0f };
	bottomCapData.size = { width, capHeight };
	bottomCapData.rotation = 0.0f;
	bottomCapData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	bottomCapSprite = std::make_unique<sprite>(device, context, bottomCapData.texturePath.c_str());

	playerButtonDataList.clear();
	playerButtonSprites.clear();

	for (int i = 0; i < buttonCount; ++i)
	{
		// Y座標を「1つ目のY位置 + i * (高さ + 隙間)」で下方向に計算
		float currentY = startPosY + i * (buttonHeight + buttonSpacing);

		PlayerButtonData data;
		data.texturePath = L".\\resources\\textures\\batterButton\\batterButton" + std::to_wstring(i + 1) + L".png";
		data.position = { startPosX, currentY };
		data.size = { buttonWidth, buttonHeight };
		data.rotation = 0.0f;
		data.color = { 1.0f, 1.0f, 1.0f, 1.0f };

		playerButtonDataList.push_back(data);

		// テクスチャ（スプライト）の生成（画像が全て同じなら同じテクスチャを使い回す設計にするとメモリに優しいです）
		playerButtonSprites.push_back(
			std::make_unique<sprite>(device, context, data.texturePath.c_str())
		);
	}
	// シェーダーの作成

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.GetAddressOf(), input_layout.GetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.GetAddressOf());
}


void ScrollView::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if (!scrollBackgroundSprite.empty() && scrollBackgroundSprite[0])
	{
		ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
		scrollBackgroundSprite[0]->render(context,
			scrollBackgroundSpriteData[0].position.x - scrollBackgroundSpriteData[0].size.x / 2.0f,
			scrollBackgroundSpriteData[0].position.y - scrollBackgroundSpriteData[0].size.y / 2.0f,
			scrollBackgroundSpriteData[0].size.x,
			scrollBackgroundSpriteData[0].size.y,
			scrollBackgroundSpriteData[0].color.x, scrollBackgroundSpriteData[0].color.y,
			scrollBackgroundSpriteData[0].color.z, scrollBackgroundSpriteData[0].color.w,
			scrollBackgroundSpriteData[0].rotation);
	}

	if(playerButtonDataList.size() > 0)
	{

		for (size_t i = 0; i < playerButtonDataList.size(); ++i)
		{
			//ボタンが上端と下端のフタの間にある場合のみ描画する
			if(playerButtonDataList[i].position.y - scrollOffsetY + playerButtonDataList[i].size.y / 2.0f < topCapData.position.y + topCapData.size.y / 2.0f ||
			   playerButtonDataList[i].position.y - scrollOffsetY - playerButtonDataList[i].size.y / 2.0f > bottomCapData.position.y - bottomCapData.size.y / 2.0f)
			{
				continue; // 描画しない
			}


			if (playerButtonSprites[i])
			{
				ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

				float drawY = (startPosY + i * (buttonHeight + buttonSpacing)) - scrollOffsetY;

				playerButtonSprites[i]->render(context,
					playerButtonDataList[i].position.x - playerButtonDataList[i].size.x / 2.0f,
					drawY - playerButtonDataList[i].size.y / 2.0f,
					playerButtonDataList[i].size.x,
					playerButtonDataList[i].size.y,
					playerButtonDataList[i].color.x, playerButtonDataList[i].color.y,
					playerButtonDataList[i].color.z, playerButtonDataList[i].color.w,
					playerButtonDataList[i].rotation);
			}
		}
	}

	if (topCapSprite)
	{
		topCapSprite->render(dc,
			topCapData.position.x - topCapData.size.x / 2.0f,
			topCapData.position.y - topCapData.size.y / 2.0f,
			topCapData.size.x, topCapData.size.y,
			topCapData.color.x, topCapData.color.y, topCapData.color.z, topCapData.color.w,
			topCapData.rotation);
	}
	if (bottomCapSprite)
	{
		bottomCapSprite->render(dc,
			bottomCapData.position.x - bottomCapData.size.x / 2.0f,
			bottomCapData.position.y - bottomCapData.size.y / 2.0f,
			bottomCapData.size.x, bottomCapData.size.y,
			bottomCapData.color.x, bottomCapData.color.y, bottomCapData.color.z, bottomCapData.color.w,
			bottomCapData.rotation);
	}

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void ScrollView::Update(float elapsedTime)
{
	// スクロールビューの更新処理
	//マウスカーソルの位置がスクロールビューの背景の範囲内にあるかどうか
	Input& input = Input::Instance();
	if(input.GetMouse().GetPositionX() >= scrollBackgroundSpriteData[0].position.x - scrollBackgroundSpriteData[0].size.x / 2.0f &&
	   input.GetMouse().GetPositionX() <= scrollBackgroundSpriteData[0].position.x + scrollBackgroundSpriteData[0].size.x / 2.0f &&
	   input.GetMouse().GetPositionY() >= scrollBackgroundSpriteData[0].position.y - scrollBackgroundSpriteData[0].size.y / 2.0f &&
	   input.GetMouse().GetPositionY() <= scrollBackgroundSpriteData[0].position.y + scrollBackgroundSpriteData[0].size.y / 2.0f)
	{
		//マウスホイールを動かすとボタンがスクロールする
		//下にスクロールするとボタンは下にスクロール、上にスクロールするとボタンは上にスクロールする
		int wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0)
		{
			scrollOffsetY -= wheel * 50.0f; // スクロール量を調整
			if (scrollOffsetY < 0.0f) scrollOffsetY = 0.0f; // 下限チェック
			float maxScroll = buttonCount * buttonHeight - buttonHeight * 8.0f; // 最大スクロール量
			if (scrollOffsetY > maxScroll) scrollOffsetY = maxScroll; // 上限チェック
		}
	}

	//ボタンを押したら、テクスチャを黄色くする
	for (size_t i = 0; i < playerButtonDataList.size(); ++i)
	{
		float buttonTopY = (startPosY + i * (buttonHeight + buttonSpacing)) - scrollOffsetY - playerButtonDataList[i].size.y / 2.0f;
		float buttonBottomY = (startPosY + i * (buttonHeight + buttonSpacing)) - scrollOffsetY + playerButtonDataList[i].size.y / 2.0f;

		bool hovered = input.GetMouse().GetPositionX() >= playerButtonDataList[i].position.x - playerButtonDataList[i].size.x / 2.0f &&
			input.GetMouse().GetPositionX() <= playerButtonDataList[i].position.x + playerButtonDataList[i].size.x / 2.0f &&
			input.GetMouse().GetPositionY() >= buttonTopY &&
			input.GetMouse().GetPositionY() <= buttonBottomY;

		//　ボタンが押されたときの処理(長押しはロックする)
		if (hovered && input.GetMouse().GetButtonDown())
		{
			playerButtonDataList[i].color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色に変更
			selectedIndex = (int)i; // 選択されたボタンのインデックスを更新
		}

		//一番最初のボタンをデフォルトで選択状態にする
		if (selectedIndex == -1 && i == 0)
		{
			playerButtonDataList[i].color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色に変更
			selectedIndex = (int)i; // 選択されたボタンのインデックスを更新
		}
			
	}
	
	
	//選択されたボタン以外は白色に戻す
	for (size_t i = 0; i < playerButtonDataList.size(); ++i)
	{
		if ((int)i != selectedIndex)
		{
			playerButtonDataList[i].color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色に戻す
		}
	}

}

void ScrollView::DrawGUI()
{
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("ScrollView Settings"))
	{
		// 1. スクロール背景（黒の背景）の操作
		if (!scrollBackgroundSpriteData.empty())
		{
			ImGui::Text("--- Background ---");
			ImGui::DragFloat2("BG Position", &scrollBackgroundSpriteData[0].position.x, 1.0f);
			ImGui::DragFloat2("BG Size", &scrollBackgroundSpriteData[0].size.x, 1.0f);
			ImGui::ColorEdit4("BG Color", &scrollBackgroundSpriteData[0].color.x);
		}

		// 2. ボタン（PlayerButton）の操作
		if (ImGui::CollapsingHeader("Button Layout"))
		{
			bool isChanged = false;

			// パラメータの操作 UI
			isChanged |= ImGui::DragFloat2("Start Pos", &startPosX, 1.0f);
			isChanged |= ImGui::DragFloat2("Button Size", &buttonWidth, 1.0f);
			isChanged |= ImGui::DragFloat("Spacing (Y)", &buttonSpacing, 1.0f);

			if (ImGui::InputInt("Button Count", &buttonCount))
			{
				if (buttonCount < 0) buttonCount = 0;
				// 個数が変わったらスプライト生成をやり直すか再構築フラグを立てる
				isChanged = true;
			}

			// 値が変更されたら座標をすべて再計算
			if (isChanged)
			{
				for (size_t i = 0; i < playerButtonDataList.size(); ++i)
				{
					playerButtonDataList[i].position.x = startPosX;
					playerButtonDataList[i].position.y = startPosY + i * (buttonHeight + buttonSpacing);
					playerButtonDataList[i].size = { buttonWidth, buttonHeight };
				}
			}
		}
	}
#endif // _DEBUG
}