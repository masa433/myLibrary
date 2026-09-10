#include "ScrollView.h"
#include "Graphics.h"
#include <shader.h>
#include <imgui.h>
#include <functional>

void ScrollView::MatchSelectedButtonAndBatter()
{
	//Player::batterToSpriteIndexTableを使う
	char buffer[256];
	if(selectedIndex >= 0 && selectedIndex < buttonCount)
	{
		selectedBatter = static_cast<Player::RealBatter>(selectedIndex + 1); // インデックスに対応するバッターを設定

		snprintf(buffer, sizeof(buffer), "Selected Index: %d, Selected Batter: %d", selectedIndex, static_cast<int>(selectedBatter));
		OutputDebugStringA(buffer);
	}
	else
	{
		selectedBatter = Player::RealBatter::None; // インデックスが範囲外の場合はNoneに設定

		snprintf(buffer, sizeof(buffer), "Selected Index: %d, Selected Batter: None", selectedIndex);
		OutputDebugStringA(buffer);
	}

	if(Player::Instance().IsRightBatter())
	{
		randomBatterImageIndex = 3 + rand() % 3; // 右打者の場合は3か4か5
	}
	else
	{
		randomBatterImageIndex = rand() % 3; // 左打者の場合は0か1か2
	}

	Player::Instance().SelectRealBatter(selectedBatter); // Playerクラスに選択されたバッターを設定
}

ScrollView::ScrollView(ID3D11Device* device, float topX, float topY, float width, float height)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	// スクロール背景スプライトの初期化
	scrollBackgroundSpriteData.push_back(ScrollBackData());
	scrollBackgroundSpriteData[0].texturePath = L".\\resources\\textures\\scrollViewBack.png"; // スクロール背景のテクスチャパス
	scrollBackgroundSpriteData[0].position = { topX, topY }; // 中心位置に設定
	scrollBackgroundSpriteData[0].size = { width, height }; // 幅と高さを設定
	scrollBackgroundSpriteData[0].rotation = 0.0f; // 回転なし
	scrollBackgroundSpriteData[0].color = { 1.0f, 1.0f, 1.0f, 0.7f }; // 白色
	scrollBackgroundSprite.push_back(std::make_unique<sprite>(device, context, scrollBackgroundSpriteData[0].texturePath.c_str()));

	float capHeight = 90.0f; // 上下のフタの高さ
	topCapData.texturePath = L".\\resources\\textures\\scrollViewBack.png";
	topCapData.position = { 550.0f, 260.0f };
	topCapData.size = { width, capHeight };
	topCapData.rotation = 0.0f;
	topCapData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	topCapSprite = std::make_unique<sprite>(device, context, topCapData.texturePath.c_str());

	bottomCapData.texturePath = L".\\resources\\textures\\scrollViewBack.png";
	bottomCapData.position = { 550.0f, 900.0f };
	bottomCapData.size = { width, capHeight };
	bottomCapData.rotation = 0.0f;
	bottomCapData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	bottomCapSprite = std::make_unique<sprite>(device, context, bottomCapData.texturePath.c_str());

	topArrowData.texturePath = L".\\resources\\textures\\Arrow.png";
	topArrowData.position = { 550.0f, 260.0f };
	topArrowData.size = { 100.0f, 50.0f };
	topArrowData.rotation = 0.0f;
	topArrowData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	topArrowSprite = std::make_unique<sprite>(device, context, topArrowData.texturePath.c_str());

	bottomArrowData.texturePath = L".\\resources\\textures\\Arrow.png";
	bottomArrowData.position = { 550.0f, 900.0f };
	bottomArrowData.size = { 100.0f, 50.0f };
	bottomArrowData.rotation = 180.0f; // 矢印を逆向きにする
	bottomArrowData.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	bottomArrowSprite = std::make_unique<sprite>(device, context, bottomArrowData.texturePath.c_str());

	playerButtonDataList.clear();
	playerButtonSprites.clear();

	for (int i = 0; i < buttonCount; ++i)
	{
		// Y座標を「1つ目のY位置 + i * (高さ + 隙間)」で下方向に計算
		
		PlayerButtonData data;
		data.texturePath = L".\\resources\\textures\\batterButton\\batterButton" + std::to_wstring(i + 1) + L".png";
		data.position = { startPosX, startPosY + i * (buttonHeight + buttonSpacing) };
		data.size = { buttonWidth, buttonHeight };
		data.rotation = 0.0f;
		data.color = { 1.0f, 1.0f, 1.0f, 1.0f };

		playerButtonDataList.push_back(data);

		// テクスチャ（スプライト）の生成（画像が全て同じなら同じテクスチャを使い回す設計にするとメモリに優しいです）
		playerButtonSprites.push_back(
			std::make_unique<sprite>(device, context, data.texturePath.c_str())
		);
	}

	for(int i = 0; i < ParamCount; ++i)
	{
		BatterParamData data;
		data.texturePath = L".\\resources\\textures\\batterParameter\\batterParameter" + std::to_wstring(i + 1) + L".png";
		data.position = { 1300.0f,500.0f };
		data.size = { 600.0f, 500.0f };
		data.rotation = 0.0f;
		data.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		batterParamDataList.push_back(data);
		batterParamSprites.push_back(
			std::make_unique<sprite>(device, context, data.texturePath.c_str())
		);
	}

	batterListData = std::make_unique<BatterListData>();
	batterListData->texturePath = L".\\resources\\textures\\batterList.png";
	batterListData->position = { Graphics::Instance().GetScreenWidth() / 2.0f, Graphics::Instance().GetScreenHeight() / 2.0f };
	batterListData->size = { 1700.0f, 900.0f };
	batterListData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	batterListData->rotation = 0.0f;
	batterListSprite = std::make_unique<sprite>(device, context, batterListData->texturePath.c_str());

	for (int i = 0; i < BATTER_IMAGE_COUNT; ++i)
	{
		imageDataArray[i] = std::make_unique<batterImageData>();
		imageDataArray[i]->texturePath = L".\\resources\\textures\\batterimage\\batterimage" + std::to_wstring(i + 1) + L".png";
		imageDataArray[i]->position = { 150.0f,100.0f };
		imageDataArray[i]->size = { 600.0f, 800.0f };
		imageDataArray[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		imageDataArray[i]->rotation = 0.0f;
		imageData[i] = std::make_unique<sprite>(device, context, imageDataArray[i]->texturePath.c_str());
	}

	for(int i = 0; i < BATTER_COUNT; ++i)
	{
		batterNameTagData[i] = std::make_unique<BatterParamData>();
		batterNameTagData[i]->texturePath = L".\\resources\\textures\\batterNameTag\\batterNameTag" + std::to_wstring(i + 1) + L".png";
		batterNameTagData[i]->position = { batterNameTagPosition.x, batterNameTagPosition.y };
		batterNameTagData[i]->size = { batterNameTagSize.x, batterNameTagSize.y };
		batterNameTagData[i]->rotation = 0.0f;
		batterNameTagData[i]->color = { batterNameTagColor.x, batterNameTagColor.y, batterNameTagColor.z, batterNameTagColor.w };
		batterNameTagSprite[i] = std::make_unique<sprite>(device, context, batterNameTagData[i]->texturePath.c_str());
	}

	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.ReleaseAndGetAddressOf(), input_layout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.ReleaseAndGetAddressOf());

	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789"
		u8"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	);

	// フォントレンダラーの初期化
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GarpSansNormalItalic.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	powerFontData.position = { 1300.0f, 400.0f }; // 画面内に配置
	powerFontData.scale = 1.0f;                    // スケールを1.0に設定
	powerFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 不透明な白色

	contactFontData.position = { 1300.0f, 500.0f };
	contactFontData.scale = 1.0f;
	contactFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	powerRankFontData.position = { 1500.0f, 400.0f };
	powerRankFontData.scale = 1.0f;
	powerRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	contactRankFontData.position = { 1500.0f, 500.0f };
	contactRankFontData.scale = 1.0f;
	contactRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	clickNameButton = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\ClickBatterName.wav");
	hoverArrow = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\HoverButton.wav");
}

ScrollView::~ScrollView()
{
	delete clickNameButton;
	delete hoverArrow;
}

void ScrollView::Render(float alpha)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if(batterListData && batterListSprite)
	{
		batterListSprite->render(dc,
			batterListData->position.x - batterListData->size.x / 2.0f,
			batterListData->position.y - batterListData->size.y / 2.0f,
			batterListData->size.x, batterListData->size.y,
			batterListData->color.x, batterListData->color.y, batterListData->color.z, batterListData->color.w * alpha,
			batterListData->rotation);
	}


	if (!scrollBackgroundSprite.empty() && scrollBackgroundSprite[0])
	{
		ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
		scrollBackgroundSprite[0]->render(context,
			scrollBackgroundSpriteData[0].position.x - scrollBackgroundSpriteData[0].size.x / 2.0f,
			scrollBackgroundSpriteData[0].position.y - scrollBackgroundSpriteData[0].size.y / 2.0f,
			scrollBackgroundSpriteData[0].size.x,
			scrollBackgroundSpriteData[0].size.y,
			scrollBackgroundSpriteData[0].color.x, scrollBackgroundSpriteData[0].color.y,
			scrollBackgroundSpriteData[0].color.z, scrollBackgroundSpriteData[0].color.w * alpha,
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
					playerButtonDataList[i].color.z, playerButtonDataList[i].color.w * alpha,
					playerButtonDataList[i].rotation);
			}
		}
	}

	// 選択中の選手のパラメータ画像だけを描画する
	if (selectedIndex >= 0 && selectedIndex < (int)batterParamDataList.size())
	{
		if (batterParamSprites[selectedIndex])
		{
			batterParamSprites[selectedIndex]->render(dc,
				batterParamDataList[selectedIndex].position.x - batterParamDataList[selectedIndex].size.x / 2.0f,
				batterParamDataList[selectedIndex].position.y - batterParamDataList[selectedIndex].size.y / 2.0f,
				batterParamDataList[selectedIndex].size.x, batterParamDataList[selectedIndex].size.y,
				batterParamDataList[selectedIndex].color.x, batterParamDataList[selectedIndex].color.y,
				batterParamDataList[selectedIndex].color.z, batterParamDataList[selectedIndex].color.w * alpha,
				batterParamDataList[selectedIndex].rotation);
		}

		
	}

	if (topCapSprite)
	{
		topCapSprite->render(dc,
			topCapData.position.x - topCapData.size.x / 2.0f,
			topCapData.position.y - topCapData.size.y / 2.0f,
			topCapData.size.x, topCapData.size.y,
			topCapData.color.x, topCapData.color.y, topCapData.color.z, topCapData.color.w * alpha,
			topCapData.rotation);
	}
	if (bottomCapSprite)
	{
		bottomCapSprite->render(dc,
			bottomCapData.position.x - bottomCapData.size.x / 2.0f,
			bottomCapData.position.y - bottomCapData.size.y / 2.0f,
			bottomCapData.size.x, bottomCapData.size.y,
			bottomCapData.color.x, bottomCapData.color.y, bottomCapData.color.z, bottomCapData.color.w * alpha,
			bottomCapData.rotation);
	}

	if(topArrowData.texturePath != L"")
	{
		if (topArrowSprite && showTopArrow)
		{
			topArrowSprite->render(dc,
				topArrowData.position.x - topArrowData.size.x / 2.0f,
				topArrowData.position.y - topArrowData.size.y / 2.0f,
				topArrowData.size.x, topArrowData.size.y,
				topArrowData.color.x, topArrowData.color.y, topArrowData.color.z, topArrowData.color.w * alpha,
				topArrowData.rotation);
		}
	}

	if(bottomArrowData.texturePath != L"")
	{
		if (bottomArrowSprite && showBottomArrow)
		{
			bottomArrowSprite->render(dc,
				bottomArrowData.position.x - bottomArrowData.size.x / 2.0f,
				bottomArrowData.position.y - bottomArrowData.size.y / 2.0f,
				bottomArrowData.size.x, bottomArrowData.size.y,
				bottomArrowData.color.x, bottomArrowData.color.y, bottomArrowData.color.z, bottomArrowData.color.w * alpha,
				bottomArrowData.rotation);
		}
	}

	
	//フォントの描画

	if (fontRenderer.IsValid())
	{
		/*fontRenderer.DrawTextW(dc, "55", 1300.0f, 200.0f, 1.5f, 1.0f, 1.0f, 1.0f, alpha);*/

		//選択中の選手のパワーとミートの値を取得
		if (selectedIndex >= 0 && selectedIndex < BATTER_COUNT)
		{
			int power = Player::Instance().GetSelectedRealBatterPower();
			int contact = Player::Instance().GetSelectedRealBatterContact();
			// 関数側が inline const RankData& GetPowerRank(int power) const のような場合
			Player::BatterPowerRank powerRank = Player::Instance().GetPowerRank(power);
			Player::BatterContactRank contactRank = Player::Instance().GetContactRank(contact);

			//ランクに応じて色を変える
			switch(powerRank)
			{
				case Player::BatterPowerRank::S:
					powerRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白
					break;
				case Player::BatterPowerRank::A:
					powerRankFontData.color = { 1.0f, 0.75f, 0.8f, 1.0f }; // 薄ピンク
					break;
				case Player::BatterPowerRank::B:
					powerRankFontData.color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤
					break;
				case Player::BatterPowerRank::C:
					powerRankFontData.color = { 1.0f, 0.5f, 0.0f, 1.0f }; // オレンジ
					break;
				case Player::BatterPowerRank::D:
					powerRankFontData.color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
					break;
				case Player::BatterPowerRank::E:
					powerRankFontData.color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑
					break;
				case Player::BatterPowerRank::F:
					powerRankFontData.color = { 0.5f, 0.5f, 0.5f, 1.0f }; // グレー
					break;
				default:
					powerRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルトはホワイト
					break;
			}

			switch (contactRank)
			{
				case Player::BatterContactRank::S:
					contactRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白
					break;
				case Player::BatterContactRank::A:
					contactRankFontData.color = { 1.0f, 0.75f, 0.8f, 1.0f }; // 薄ピンク
					break;
				case Player::BatterContactRank::B:
					contactRankFontData.color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤
					break;
				case Player::BatterContactRank::C:
					contactRankFontData.color = { 1.0f, 0.5f, 0.0f, 1.0f }; // オレンジ
					break;
				case Player::BatterContactRank::D:
					contactRankFontData.color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
					break;
				case Player::BatterContactRank::E:
					contactRankFontData.color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑
					break;
				case Player::BatterContactRank::F:
					contactRankFontData.color = { 0.5f, 0.5f, 0.5f, 1.0f }; // グレー
					break;
				default:
					contactRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルトはホワイト
					break;
			}

			//パワーとミートの値を描画
			fontRenderer.DrawTextW(dc, std::to_string(power).c_str(), 
				powerFontData.position.x, powerFontData.position.y, powerFontData.scale,
				powerFontData.color.x, powerFontData.color.y, powerFontData.color.z, powerFontData.color.w * alpha);

			fontRenderer.DrawTextW(dc, GetBatterPowerRankString(powerRank),
				powerRankFontData.position.x, powerRankFontData.position.y, powerRankFontData.scale,
				powerRankFontData.color.x, powerRankFontData.color.y, powerRankFontData.color.z, powerRankFontData.color.w* alpha);

			fontRenderer.DrawTextW(dc, std::to_string(contact).c_str(),
				contactFontData.position.x, contactFontData.position.y, contactFontData.scale, 
				contactFontData.color.x, contactFontData.color.y, contactFontData.color.z, contactFontData.color.w * alpha);

			fontRenderer.DrawTextW(dc, GetBatterContactRankString(contactRank),
				contactRankFontData.position.x, contactRankFontData.position.y, contactRankFontData.scale,
				contactRankFontData.color.x, contactRankFontData.color.y, contactRankFontData.color.z, contactRankFontData.color.w* alpha);
			
		}

	}


	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void ScrollView::RenderBatterImage(ID3D11DeviceContext* dc, float alpha)
{
	if (randomBatterImageIndex >= 0 && randomBatterImageIndex < BATTER_IMAGE_COUNT)
	{
		if (imageData[randomBatterImageIndex])
		{
			imageData[randomBatterImageIndex]->render(dc,
				imageDataArray[randomBatterImageIndex]->position.x,
				imageDataArray[randomBatterImageIndex]->position.y,
				imageDataArray[randomBatterImageIndex]->size.x, imageDataArray[randomBatterImageIndex]->size.y,
				imageDataArray[randomBatterImageIndex]->color.x, imageDataArray[randomBatterImageIndex]->color.y, 
				imageDataArray[randomBatterImageIndex]->color.z, imageDataArray[randomBatterImageIndex]->color.w * alpha,
				imageDataArray[randomBatterImageIndex]->rotation);
		}
	}

	if (selectedIndex >= 0 && selectedIndex < BATTER_COUNT)
	{
		if (batterNameTagSprite[selectedIndex])
		{
			batterNameTagSprite[selectedIndex]->render(dc,
				batterNameTagPosition.x, batterNameTagPosition.y,
				batterNameTagSize.x, batterNameTagSize.y,
				batterNameTagColor.x, batterNameTagColor.y,
				batterNameTagColor.z, batterNameTagColor.w * alpha,
				batterNameTagData[selectedIndex]->rotation);
		}
	}
}

void ScrollView::Update(float elapsedTime)
{

	// スクロールビューの更新処理
	//マウスカーソルの位置がスクロールビューの背景の範囲内にあるかどうか
	Input& input = Input::Instance();
	if (input.GetMouse().GetPositionX() >= scrollBackgroundSpriteData[0].position.x - scrollBackgroundSpriteData[0].size.x / 2.0f &&
		input.GetMouse().GetPositionX() <= scrollBackgroundSpriteData[0].position.x + scrollBackgroundSpriteData[0].size.x / 2.0f &&
		input.GetMouse().GetPositionY() >= scrollBackgroundSpriteData[0].position.y - scrollBackgroundSpriteData[0].size.y / 2.0f &&
		input.GetMouse().GetPositionY() <= scrollBackgroundSpriteData[0].position.y + scrollBackgroundSpriteData[0].size.y / 2.0f)
	{
		int wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0)
		{
			scrollOffsetY -= wheel * 110.0f; // スクロール処理
		}
	}

	// 2. スクロール範囲のクランプ（下限・上限の制御）
	float maxScroll = 2090.0f;
	if (maxScroll < 0.0f) maxScroll = 0.0f; // 要素数が少なくスクロール不要な場合の考慮

	if (scrollOffsetY < 0.0f)
	{
		scrollOffsetY = 0.0f; // 最上部
	}
	else if (scrollOffsetY > maxScroll)
	{
		scrollOffsetY = maxScroll; // 最下部
	}

	
	showTopArrow = (scrollOffsetY > 0.0f);
	showBottomArrow = (scrollOffsetY < maxScroll);

	bool topHovered =
		input.GetMouse().GetPositionX() >= topArrowData.position.x - originalArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionX() <= topArrowData.position.x + originalArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionY() >= topArrowData.position.y - originalArrowSize.y / 2.0f &&
		input.GetMouse().GetPositionY() <= topArrowData.position.y + originalArrowSize.y / 2.0f;

	bool bottomHovered =
		input.GetMouse().GetPositionX() >= bottomArrowData.position.x - originalArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionX() <= bottomArrowData.position.x + originalArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionY() >= bottomArrowData.position.y - originalArrowSize.y / 2.0f &&
		input.GetMouse().GetPositionY() <= bottomArrowData.position.y + originalArrowSize.y / 2.0f;


	bool topPressed = topHovered && input.GetMouse().GetButton(); // 押しっぱなし判定
	bool bottomPressed = bottomHovered && input.GetMouse().GetButton();

	if (showTopArrow)
	{
		topArrowData.size = topPressed ? originalArrowSize : (topHovered ? targetArrowSize : originalArrowSize);
		if (topHovered && !topPressed && hoverArrow && !hoverSoundPlayed)
		{
			hoverArrow->PlayOneShot();
			hoverSoundPlayed = true;
		}
	}

	if (showBottomArrow)
	{
		bottomArrowData.size = bottomPressed ? originalArrowSize : (bottomHovered ? targetArrowSize : originalArrowSize);
		if (bottomHovered && !bottomPressed && hoverArrow && !hoverSoundPlayed)
		{
			hoverArrow->PlayOneShot();
			hoverSoundPlayed = true;
		}
	}

	// マウスが矢印ボタンの上にない場合、hoverSoundPlayedをリセット
	if (!topHovered && !bottomHovered)
	{
		hoverSoundPlayed = false;
	}

	//矢印を押したときのスクロール処理
	//このときはボタンは押せないようにする
	bool arrowClicked = false;
	if(input.GetMouse().GetButtonDown() && showTopArrow)
	{
		bool isHovered = input.GetMouse().GetPositionX() >= topArrowData.position.x - topArrowData.size.x / 2.0f &&
			input.GetMouse().GetPositionX() <= topArrowData.position.x + topArrowData.size.x / 2.0f &&
			input.GetMouse().GetPositionY() >= topArrowData.position.y - topArrowData.size.y / 2.0f &&
			input.GetMouse().GetPositionY() <= topArrowData.position.y + topArrowData.size.y / 2.0f;
		if(isHovered)
		{
			scrollOffsetY -= 110.0f; // 上方向にスクロール
			arrowClicked = true;

			//効果音再生
			if (hoverArrow)
			{
				hoverArrow->PlayOneShot();
			}
		}
	}

	if(input.GetMouse().GetButtonDown() && showBottomArrow)
	{
		bool isHovered = input.GetMouse().GetPositionX() >= bottomArrowData.position.x - bottomArrowData.size.x / 2.0f &&
			input.GetMouse().GetPositionX() <= bottomArrowData.position.x + bottomArrowData.size.x / 2.0f &&
			input.GetMouse().GetPositionY() >= bottomArrowData.position.y - bottomArrowData.size.y / 2.0f &&
			input.GetMouse().GetPositionY() <= bottomArrowData.position.y + bottomArrowData.size.y / 2.0f;
		if(isHovered)
		{
			scrollOffsetY += 110.0f; // 下方向にスクロール
			arrowClicked = true;
			if(hoverArrow)
			{
				hoverArrow->PlayOneShot();
			}
		}
	}

	//capの範囲にあるボタンは押せないようにする
	bool isInCapArea = false;

	if(input.GetMouse().GetPositionX() >= topCapData.position.x - topCapData.size.x / 2.0f &&
		input.GetMouse().GetPositionX() <= topCapData.position.x + topCapData.size.x / 2.0f &&
		input.GetMouse().GetPositionY() >= topCapData.position.y - topCapData.size.y / 2.0f &&
		input.GetMouse().GetPositionY() <= topCapData.position.y + topCapData.size.y / 2.0f ||
		input.GetMouse().GetPositionX() >= bottomCapData.position.x - bottomCapData.size.x / 2.0f &&
		input.GetMouse().GetPositionX() <= bottomCapData.position.x + bottomCapData.size.x / 2.0f &&
		input.GetMouse().GetPositionY() >= bottomCapData.position.y - bottomCapData.size.y / 2.0f &&
		input.GetMouse().GetPositionY() <= bottomCapData.position.y + bottomCapData.size.y / 2.0f)
	{
		isInCapArea = true;
	}



	if (!arrowClicked && !isInCapArea)
	{
		//ボタンを押すことができる範囲の設定
		float visibleMinY = topCapData.position.y + topCapData.size.y / 2.0f;
		float visibleMaxY = bottomCapData.position.y - bottomCapData.size.y / 2.0f;

		//ボタンの押下処理
		//ボタンを押したら、テクスチャを黄色くする
		for (size_t i = 0; i < playerButtonDataList.size(); ++i)
		{
			float buttonTopY = (startPosY + i * (buttonHeight + buttonSpacing)) - scrollOffsetY - playerButtonDataList[i].size.y / 2.0f;
			float buttonBottomY = (startPosY + i * (buttonHeight + buttonSpacing)) - scrollOffsetY + playerButtonDataList[i].size.y / 2.0f;

			bool isVisible = (buttonBottomY >= visibleMinY) && (buttonTopY <= visibleMaxY);

			if (!isVisible)
			{
				continue;
			}
			

			bool hovered = input.GetMouse().GetPositionX() >= playerButtonDataList[i].position.x - playerButtonDataList[i].size.x / 2.0f &&
				input.GetMouse().GetPositionX() <= playerButtonDataList[i].position.x + playerButtonDataList[i].size.x / 2.0f &&
				input.GetMouse().GetPositionY() >= buttonTopY &&
				input.GetMouse().GetPositionY() <= buttonBottomY;

			

			//　ボタンが押されたときの処理(長押しはロックする)
			if (hovered && input.GetMouse().GetButtonDown())
			{
				playerButtonDataList[i].color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色に変更
				selectedIndex = (int)i; // 選択されたボタンのインデックスを更新
				MatchSelectedButtonAndBatter(); // 選択されたボタンとバッターを一致させる

				if (clickNameButton)
				{
					clickNameButton->PlayOneShot();
				}
			}

			//一番最初のボタンをデフォルトで選択状態にする
			if (selectedIndex == -1 && i == 0)
			{
				playerButtonDataList[i].color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色に変更
				selectedIndex = (int)i; // 選択されたボタンのインデックスを更新
				MatchSelectedButtonAndBatter(); // 選択されたボタンとバッターを一致させる
			}

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

		if(ImGui::CollapsingHeader("Arrow Settings"))
		{
			ImGui::DragFloat2("Top Arrow Position", &topArrowData.position.x, 1.0f);
			ImGui::DragFloat2("Top Arrow Size", &topArrowData.size.x, 1.0f);
			ImGui::DragFloat2("Bottom Arrow Position", &bottomArrowData.position.x, 1.0f);
			ImGui::DragFloat2("Bottom Arrow Size", &bottomArrowData.size.x, 1.0f);
		}

		if(ImGui::CollapsingHeader("Cap Settings"))
		{
			ImGui::DragFloat2("Top Cap Position", &topCapData.position.x, 1.0f);
			ImGui::DragFloat2("Top Cap Size", &topCapData.size.x, 1.0f);
			ImGui::DragFloat2("Bottom Cap Position", &bottomCapData.position.x, 1.0f);
			ImGui::DragFloat2("Bottom Cap Size", &bottomCapData.size.x, 1.0f);	
		}

		if(ImGui::CollapsingHeader("Scroll Offset"))
		{
			ImGui::DragFloat("Scroll Offset Y", &scrollOffsetY, 1.0f);
		}

		if (ImGui::CollapsingHeader("Batter Param"))
		{
			for(size_t i = 0; i < batterParamDataList.size(); ++i)
			{
				ImGui::Text("Batter Param %zu", i);
				ImGui::DragFloat2(("Position##" + std::to_string(i)).c_str(), &batterParamDataList[i].position.x, 1.0f);
				ImGui::DragFloat2(("Size##" + std::to_string(i)).c_str(), &batterParamDataList[i].size.x, 1.0f);
				ImGui::DragFloat(("Rotation##" + std::to_string(i)).c_str(), &batterParamDataList[i].rotation, 1.0f);
				ImGui::ColorEdit4(("Color##" + std::to_string(i)).c_str(), &batterParamDataList[i].color.x);
			}

		}

		if(ImGui::CollapsingHeader("Batter Image"))
		{
			for(size_t i = 0; i < BATTER_IMAGE_COUNT; ++i)
			{
				ImGui::Text("Batter Image %zu", i);
				ImGui::DragFloat2(("Position##" + std::to_string(i)).c_str(), &imageDataArray[i]->position.x, 1.0f);
				ImGui::DragFloat2(("Size##" + std::to_string(i)).c_str(), &imageDataArray[i]->size.x, 1.0f);
				ImGui::DragFloat(("Rotation##" + std::to_string(i)).c_str(), &imageDataArray[i]->rotation, 1.0f);
				ImGui::ColorEdit4(("Color##" + std::to_string(i)).c_str(), &imageDataArray[i]->color.x);
			}
		}

		if(ImGui::CollapsingHeader("Batter Name Tag"))
		{
			ImGui::DragFloat2("Batter Name Tag Position", &batterNameTagPosition.x, 1.0f);
			ImGui::DragFloat2("Batter Name Tag Size", &batterNameTagSize.x, 1.0f);
			ImGui::ColorEdit4("Batter Name Tag Color", &batterNameTagColor.x);
		}

		if (ImGui::CollapsingHeader("Param Font"))
		{
			//パワーの数値とランクの文字列
			if(ImGui::CollapsingHeader("Power Font & Rank"))
			{
				ImGui::DragFloat2("Power Font Position", &powerFontData.position.x, 1.0f);
				ImGui::DragFloat("Power Font Scale", &powerFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Power Font Color", &powerFontData.color.x);

				ImGui::DragFloat2("Power Rank Font Position", &powerRankFontData.position.x, 1.0f);
				ImGui::DragFloat("Power Rank Font Scale", &powerRankFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Power Rank Font Color", &powerRankFontData.color.x);
			}

			if(ImGui::CollapsingHeader("Contact Font & Rank"))
			{
				ImGui::DragFloat2("Contact Font Position", &contactFontData.position.x, 1.0f);
				ImGui::DragFloat("Contact Font Scale", &contactFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Contact Font Color", &contactFontData.color.x);
				ImGui::DragFloat2("Contact Rank Font Position", &contactRankFontData.position.x, 1.0f);
				ImGui::DragFloat("Contact Rank Font Scale", &contactRankFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Contact Rank Font Color", &contactRankFontData.color.x);
			}
			
			
		}
	}
#endif // _DEBUG
}

void ScrollView::SaveToJson(nlohmann::json& json)
{
	// スクロールビューの設定をJSONに保存
	json["ScrollView"] = {
		{"scrollOffsetY", scrollOffsetY},
		{"buttonCount", buttonCount},
		{"startPosX", startPosX},
		{"startPosY", startPosY},
		{"buttonWidth", buttonWidth},
		{"buttonHeight", buttonHeight},
		{"buttonSpacing", buttonSpacing}
	};
	// ボタンの位置とサイズを保存
	for (size_t i = 0; i < playerButtonDataList.size(); ++i)
	{
		json["PlayerButtons"][i] = {
			{"position", {playerButtonDataList[i].position.x, playerButtonDataList[i].position.y}},
			{"size", {playerButtonDataList[i].size.x, playerButtonDataList[i].size.y}},
			{"color", {playerButtonDataList[i].color.x, playerButtonDataList[i].color.y, playerButtonDataList[i].color.z, playerButtonDataList[i].color.w}}
		};
	}

	//バッター画像とネームタグの設定を保存
	for(size_t i = 0; i < BATTER_IMAGE_COUNT; ++i)
	{
		json["BatterImages"][i] = {
			{"position", {imageDataArray[i]->position.x, imageDataArray[i]->position.y}},
			{"size", {imageDataArray[i]->size.x, imageDataArray[i]->size.y}},
			{"rotation", imageDataArray[i]->rotation},
			{"color", {imageDataArray[i]->color.x, imageDataArray[i]->color.y, imageDataArray[i]->color.z, imageDataArray[i]->color.w}}
		};
	}

	json["BatterNameTags"] = nlohmann::json::array();
	for(size_t i = 0; i < BATTER_COUNT; ++i)
	{
		json["BatterNameTags"][i] = {
			{"position", {batterNameTagPosition.x, batterNameTagPosition.y}},
			{"size", {batterNameTagSize.x, batterNameTagSize.y}},
			{"rotation", batterNameTagData[i]->rotation},
			{"color", {batterNameTagColor.x, batterNameTagColor.y, batterNameTagColor.z, batterNameTagColor.w}}
		};
	}

	// フォントの設定を保存
	json["FontSettings"] = {
		{"powerFontPosition", {powerFontData.position.x, powerFontData.position.y}},
		{"powerFontScale", powerFontData.scale},
		{"powerFontColor", {powerFontData.color.x, powerFontData.color.y, powerFontData.color.z, powerFontData.color.w}},
		{"powerRankFontPosition", {powerRankFontData.position.x, powerRankFontData.position.y}},
		{"powerRankFontScale", powerRankFontData.scale},
		{"powerRankFontColor", {powerRankFontData.color.x, powerRankFontData.color.y, powerRankFontData.color.z, powerRankFontData.color.w}},
		{"contactFontPosition", {contactFontData.position.x, contactFontData.position.y}},
		{"contactFontScale", contactFontData.scale},
		{"contactFontColor", {contactFontData.color.x, contactFontData.color.y, contactFontData.color.z, contactFontData.color.w}},
		{"contactRankFontPosition", {contactRankFontData.position.x, contactRankFontData.position.y}},
		{"contactRankFontScale", contactRankFontData.scale},
		{"contactRankFontColor", {contactRankFontData.color.x, contactRankFontData.color.y, contactRankFontData.color.z, contactRankFontData.color.w}}
	};
}

void ScrollView::LoadFromJson(const nlohmann::json& json)
{
	// JSONからスクロールビューの設定を読み込む
	if (json.contains("ScrollView"))
	{
		const auto& scrollViewJson = json["ScrollView"];
		scrollOffsetY = scrollViewJson.value("scrollOffsetY", 0.0f);
		buttonCount = scrollViewJson.value("buttonCount", 10);
		startPosX = scrollViewJson.value("startPosX", 550.0f);
		startPosY = scrollViewJson.value("startPosY", 300.0f);
		buttonWidth = scrollViewJson.value("buttonWidth", 400.0f);
		buttonHeight = scrollViewJson.value("buttonHeight", 100.0f);
		buttonSpacing = scrollViewJson.value("buttonSpacing", 20.0f);
	}
	// ボタンの位置とサイズを読み込む
	if (json.contains("PlayerButtons"))
	{
		const auto& buttonsJson = json["PlayerButtons"];
		for (size_t i = 0; i < buttonsJson.size() && i < playerButtonDataList.size(); ++i)
		{
			const auto& buttonJson = buttonsJson[i];
			if (buttonJson.contains("position"))
			{
				playerButtonDataList[i].position.x = buttonJson["position"][0].get<float>();
				playerButtonDataList[i].position.y = buttonJson["position"][1].get<float>();
			}
			if (buttonJson.contains("size"))
			{
				playerButtonDataList[i].size.x = buttonJson["size"][0].get<float>();
				playerButtonDataList[i].size.y = buttonJson["size"][1].get<float>();
			}
			if (buttonJson.contains("color"))
			{
				playerButtonDataList[i].color.x = buttonJson["color"][0].get<float>();
				playerButtonDataList[i].color.y = buttonJson["color"][1].get<float>();
				playerButtonDataList[i].color.z = buttonJson["color"][2].get<float>();
				playerButtonDataList[i].color.w = buttonJson["color"][3].get<float>();
			}
		}
	}

	//バッター画像とネームタグの設定を読み込む

	if (json.contains("BatterImages"))
	{
		const auto& imagesJson = json["BatterImages"];
		for (size_t i = 0; i < imagesJson.size() && i < BATTER_IMAGE_COUNT; ++i)
		{
			const auto& imageJson = imagesJson[i];
			if (imageJson.contains("position"))
			{
				imageDataArray[i]->position.x = imageJson["position"][0].get<float>();
				imageDataArray[i]->position.y = imageJson["position"][1].get<float>();
			}
			if (imageJson.contains("size"))
			{
				imageDataArray[i]->size.x = imageJson["size"][0].get<float>();
				imageDataArray[i]->size.y = imageJson["size"][1].get<float>();
			}
			if (imageJson.contains("rotation"))
			{
				imageDataArray[i]->rotation = imageJson["rotation"].get<float>();
			}
			if (imageJson.contains("color"))
			{
				imageDataArray[i]->color.x = imageJson["color"][0].get<float>();
				imageDataArray[i]->color.y = imageJson["color"][1].get<float>();
				imageDataArray[i]->color.z = imageJson["color"][2].get<float>();
				imageDataArray[i]->color.w = imageJson["color"][3].get<float>();
			}
		}
	}

	if(json.contains("BatterNameTags"))
	{
		const auto& nameTagsJson = json["BatterNameTags"];
		for(size_t i = 0; i < nameTagsJson.size() && i < BATTER_COUNT; ++i)
		{
			const auto& nameTagJson = nameTagsJson[i];
			if(nameTagJson.contains("position"))
			{
				batterNameTagPosition.x = nameTagJson["position"][0].get<float>();
				batterNameTagPosition.y = nameTagJson["position"][1].get<float>();
			}
			if(nameTagJson.contains("size"))
			{
				batterNameTagSize.x = nameTagJson["size"][0].get<float>();
				batterNameTagSize.y = nameTagJson["size"][1].get<float>();
			}
			if(nameTagJson.contains("rotation"))
			{
				batterNameTagData[i]->rotation = nameTagJson["rotation"].get<float>();
			}
			if(nameTagJson.contains("color"))
			{
				batterNameTagColor.x = nameTagJson["color"][0].get<float>();
				batterNameTagColor.y = nameTagJson["color"][1].get<float>();
				batterNameTagColor.z = nameTagJson["color"][2].get<float>();
				batterNameTagColor.w = nameTagJson["color"][3].get<float>();
			}
		}
	}

	// フォントの設定を読み込む
	if (json.contains("FontSettings"))
	{
		const auto& fontSettingsJson = json["FontSettings"];
		if (fontSettingsJson.contains("powerFontPosition"))
		{
			powerFontData.position.x = fontSettingsJson["powerFontPosition"][0].get<float>();
			powerFontData.position.y = fontSettingsJson["powerFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("powerFontScale"))
		{
			powerFontData.scale = fontSettingsJson["powerFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("powerFontColor"))
		{
			powerFontData.color.x = fontSettingsJson["powerFontColor"][0].get<float>();
			powerFontData.color.y = fontSettingsJson["powerFontColor"][1].get<float>();
			powerFontData.color.z = fontSettingsJson["powerFontColor"][2].get<float>();
			powerFontData.color.w = fontSettingsJson["powerFontColor"][3].get<float>();
		}
		if(fontSettingsJson.contains("powerRankFontPosition"))
		{
			powerRankFontData.position.x = fontSettingsJson["powerRankFontPosition"][0].get<float>();
			powerRankFontData.position.y = fontSettingsJson["powerRankFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("powerRankFontScale"))
		{
			powerRankFontData.scale = fontSettingsJson["powerRankFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("powerRankFontColor"))
		{
			powerRankFontData.color.x = fontSettingsJson["powerRankFontColor"][0].get<float>();
			powerRankFontData.color.y = fontSettingsJson["powerRankFontColor"][1].get<float>();
			powerRankFontData.color.z = fontSettingsJson["powerRankFontColor"][2].get<float>();
			powerRankFontData.color.w = fontSettingsJson["powerRankFontColor"][3].get<float>();
		}
		if (fontSettingsJson.contains("contactFontPosition"))
		{
			contactFontData.position.x = fontSettingsJson["contactFontPosition"][0].get<float>();
			contactFontData.position.y = fontSettingsJson["contactFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("contactFontScale"))
		{
			contactFontData.scale = fontSettingsJson["contactFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("contactFontColor"))
		{
			contactFontData.color.x = fontSettingsJson["contactFontColor"][0].get<float>();
			contactFontData.color.y = fontSettingsJson["contactFontColor"][1].get<float>();
			contactFontData.color.z = fontSettingsJson["contactFontColor"][2].get<float>();
			contactFontData.color.w = fontSettingsJson["contactFontColor"][3].get<float>();
		}
		if (fontSettingsJson.contains("contactRankFontPosition"))
		{
			contactRankFontData.position.x = fontSettingsJson["contactRankFontPosition"][0].get<float>();
			contactRankFontData.position.y = fontSettingsJson["contactRankFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("contactRankFontScale"))
		{
			contactRankFontData.scale = fontSettingsJson["contactRankFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("contactRankFontColor"))
		{
			contactRankFontData.color.x = fontSettingsJson["contactRankFontColor"][0].get<float>();
			contactRankFontData.color.y = fontSettingsJson["contactRankFontColor"][1].get<float>();
			contactRankFontData.color.z = fontSettingsJson["contactRankFontColor"][2].get<float>();
			contactRankFontData.color.w = fontSettingsJson["contactRankFontColor"][3].get<float>();
		}
	}
}