#include "pch.h"
#include "ScrollView.h"
#include "Graphics.h"
#include <shader.h>
#include "Money.h"

std::string FormatBonusFloat(float value)
{

	char buf[32];
	std::snprintf(buf, sizeof(buf), "%.2f", value);
	std::string str(buf);

	// 末尾が '0' かつ 小数点から2文字以上後ろにある場合のみ '0' を削る
	// (＝ "1.00" の場合は "1.0" になるが、"1.0" の末尾の0は削られない)
	size_t dotPos = str.find('.');
	if (dotPos != std::string::npos)
	{
		while (str.back() == '0' && str.length() > dotPos + 2)
		{
			str.pop_back();
		}
	}

	return str;
}

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

	Player::Instance().SelectRealBatter(selectedBatter); // Playerクラスに選択されたバッターを設定

	if(Player::Instance().IsRightBatter())
	{
		randomBatterImageIndex = 3 + rand() % 3; // 右打者の場合は3か4か5
	}
	else
	{
		randomBatterImageIndex = rand() % 3; // 左打者の場合は0か1か2
	}

}

ScrollView::ScrollView(ID3D11Device* device, float topX, float topY, float width, float height)
{
	initialized = false;
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	if (!device || !context) return;
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
	batterListData->position = { batterListPos.x, batterListPos.y };
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

	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789×.:"
		u8"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		u8"ランク"
		u8"初期ホームランボーナス初期変化球ボーナス"
		u8"初期配布能力なし"
		u8"はランク2以下のバッターにのみ適用"
	);

	// フォントレンダラーの初期化
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GarpSansNormalItalic.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	paramFontRenderer.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		2048, 2048,
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

	rankFontData.position = { 1300.0f, 600.0f };
	rankFontData.scale = 0.5f;
	rankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	homerunBonusFontData.position = { 1300.0f, 700.0f };
	homerunBonusFontData.scale = 0.5f;
	homerunBonusFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	breakingBallBonusFontData.position = { 1300.0f, 800.0f };
	breakingBallBonusFontData.scale = 0.5f;
	breakingBallBonusFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	passiveFontData.position = { 1300.0f, 900.0f };
	passiveFontData.scale = 0.5f;
	passiveFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	passiveParamFontData.position = { 1300.0f, 1000.0f };
	passiveParamFontData.scale = 0.5f;
	passiveParamFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	notPassiveFontData.position = { 1350.0f, 700.0f };
	notPassiveFontData.scale = 0.5f;
	notPassiveFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	clickNameButton = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\ClickBatterName.wav",SoundType::SE);
	hoverArrow = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\HoverButton.wav",SoundType::SE);
	initialized = vertex_shader && pixel_shader && input_layout;
}

ScrollView::~ScrollView()
{
	delete clickNameButton;
	delete hoverArrow;
}

void ScrollView::Render(float alpha)
{
	if (!initialized) return;
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	if (!dc || !renderState || !vertex_shader || !pixel_shader || !input_layout) return;
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if(batterListData && batterListSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(batterListPos);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(batterListData->size);

		DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };

		batterListSprite->render(dc,
			scaledCenterPosition.x,
			scaledCenterPosition.y,
			scaledSize.x, scaledSize.y,
			batterListData->color.x, batterListData->color.y, batterListData->color.z, batterListData->color.w * alpha,
			batterListData->rotation);
	}


	if (!scrollBackgroundSprite.empty() && !scrollBackgroundSpriteData.empty() && scrollBackgroundSprite[0])
	{
		ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(scrollBackgroundSpriteData[0].position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(scrollBackgroundSpriteData[0].size);
		DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };
		scrollBackgroundSprite[0]->render(context,
			scaledCenterPosition.x,
			scaledCenterPosition.y,
			scaledSize.x,
			scaledSize.y,
			scrollBackgroundSpriteData[0].color.x, scrollBackgroundSpriteData[0].color.y,
			scrollBackgroundSpriteData[0].color.z, scrollBackgroundSpriteData[0].color.w * alpha,
			scrollBackgroundSpriteData[0].rotation);
	}

	if (!playerButtonDataList.empty())
	{
		const size_t drawableButtonCount = (std::min)(playerButtonDataList.size(), playerButtonSprites.size());
		for (size_t i = 0; i < drawableButtonCount; ++i)
		{
		
			float drawY = (startPosY + i * (buttonHeight + buttonSpacing)) - scrollOffsetY;

			//ボタンが上端と下端のフタの間にある場合のみ描画する
			if (drawY + playerButtonDataList[i].size.y / 2.0f < topCapData.position.y + topCapData.size.y / 2.0f ||
				drawY - playerButtonDataList[i].size.y / 2.0f > bottomCapData.position.y - bottomCapData.size.y / 2.0f)
			{
				continue; // 描画しない
			}


			if (playerButtonSprites[i])
			{
				ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

				

				DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale({ playerButtonDataList[i].position.x, drawY });
				DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(playerButtonDataList[i].size);
				DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };

				playerButtonSprites[i]->render(context,
					scaledCenterPosition.x,
					scaledCenterPosition.y,
					scaledSize.x,
					scaledSize.y,
					playerButtonDataList[i].color.x, playerButtonDataList[i].color.y,
					playerButtonDataList[i].color.z, playerButtonDataList[i].color.w * alpha,
					playerButtonDataList[i].rotation);
			}
		}
	}

	// 選択中の選手のパラメータ画像だけを描画する
	if (selectedIndex >= 0 && selectedIndex < static_cast<int>((std::min)(batterParamDataList.size(), batterParamSprites.size())))
	{
		if (batterParamSprites[selectedIndex])
		{
			DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(batterParamDataList[selectedIndex].position);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(batterParamDataList[selectedIndex].size);
			DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };

			batterParamSprites[selectedIndex]->render(dc,
				scaledCenterPosition.x,
				scaledCenterPosition.y,
				scaledSize.x, scaledSize.y,
				batterParamDataList[selectedIndex].color.x, batterParamDataList[selectedIndex].color.y,
				batterParamDataList[selectedIndex].color.z, batterParamDataList[selectedIndex].color.w * alpha,
				batterParamDataList[selectedIndex].rotation);
		}

		
	}

	if (topCapSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(topCapData.position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(topCapData.size);
		DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };
		topCapSprite->render(dc,
			scaledCenterPosition.x,
			scaledCenterPosition.y,
			scaledSize.x, scaledSize.y,
			topCapData.color.x, topCapData.color.y, topCapData.color.z, topCapData.color.w * alpha,
			topCapData.rotation);
	}
	if (bottomCapSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(bottomCapData.position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(bottomCapData.size);
		DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };
		bottomCapSprite->render(dc,
			scaledCenterPosition.x,
			scaledCenterPosition.y,
			scaledSize.x, scaledSize.y,
			bottomCapData.color.x, bottomCapData.color.y, bottomCapData.color.z, bottomCapData.color.w * alpha,
			bottomCapData.rotation);
	}

	if(topArrowData.texturePath != L"")
	{
		if (topArrowSprite && showTopArrow)
		{
			DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(topArrowData.position);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(topArrowData.size);
			DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };
			topArrowSprite->render(dc,
				scaledCenterPosition.x,
				scaledCenterPosition.y,
				scaledSize.x, scaledSize.y,
				topArrowData.color.x, topArrowData.color.y, topArrowData.color.z, topArrowData.color.w * alpha,
				topArrowData.rotation);
		}
	}

	if(bottomArrowData.texturePath != L"")
	{
		if (bottomArrowSprite && showBottomArrow)
		{
			DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(bottomArrowData.position);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(bottomArrowData.size);
			DirectX::XMFLOAT2 scaledCenterPosition = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };
			bottomArrowSprite->render(dc,
				scaledCenterPosition.x,
				scaledCenterPosition.y,
				scaledSize.x, scaledSize.y,
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

			
			switch (Player::Instance().GetSelectedRealBatterRank())
			{
			case BatterRank::Rank0:
				homerunBonus = 1.5f;
				breakingBallBonus = 1.5f;
				break;
			case BatterRank::Rank1:
				homerunBonus = 1.5f;
				breakingBallBonus = 1.5f;
				break;
			case BatterRank::Rank2:
				homerunBonus = 1.3f;
				breakingBallBonus = 1.3f;
				break;
			case BatterRank::Rank3:
				homerunBonus = 1.2f;
				breakingBallBonus = 1.2f;
				break;
			default:
				homerunBonus = 1.0f;
				breakingBallBonus = 1.0f;
				break;
			}

			auto scaledText = [&](const DirectX::XMFLOAT2& scaledPos, float scaledSize) -> std::pair<DirectX::XMFLOAT2, float>
				{
					return {
						screenScaler.Scale(scaledPos),
						scaledSize * screenScaler.GetUniformScale()
					};
				};

			auto [scaledPowerPos, scaledPowerSize] = scaledText(powerFontData.position, powerFontData.scale);
			auto [scaledContactPos, scaledContactSize] = scaledText(contactFontData.position, contactFontData.scale);
			auto [scaledPowerRankPos, scaledPowerRankSize] = scaledText(powerRankFontData.position, powerRankFontData.scale);
			auto [scaledContactRankPos, scaledContactRankSize] = scaledText(contactRankFontData.position, contactRankFontData.scale);

			//パワーとミートの値を描画
			fontRenderer.DrawTextW(dc, std::to_string(power).c_str(), 
				scaledPowerPos.x, scaledPowerPos.y, scaledPowerSize,
				powerFontData.color.x, powerFontData.color.y, powerFontData.color.z, powerFontData.color.w * alpha);

			fontRenderer.DrawTextW(dc, GetBatterPowerRankString(powerRank),
				scaledPowerRankPos.x, scaledPowerRankPos.y, scaledPowerRankSize,
				powerRankFontData.color.x, powerRankFontData.color.y, powerRankFontData.color.z, powerRankFontData.color.w* alpha);

			fontRenderer.DrawTextW(dc, std::to_string(contact).c_str(),
				scaledContactPos.x, scaledContactPos.y, scaledContactSize, 
				contactFontData.color.x, contactFontData.color.y, contactFontData.color.z, contactFontData.color.w * alpha);

			fontRenderer.DrawTextW(dc, GetBatterContactRankString(contactRank),
				scaledContactRankPos.x, scaledContactRankPos.y, scaledContactRankSize,
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
	if (!initialized || !dc) return;
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	if(currentState == ShowState::ShowBatterName)
	{
		if (randomBatterImageIndex >= 0 && randomBatterImageIndex < BATTER_IMAGE_COUNT)
		{
			if (imageData[randomBatterImageIndex] && imageDataArray[randomBatterImageIndex])
			{
				DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(imageDataArray[randomBatterImageIndex]->position);
				DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(imageDataArray[randomBatterImageIndex]->size);

				imageData[randomBatterImageIndex]->render(dc,
					scaledPosition.x,
					scaledPosition.y,
					scaledSize.x, scaledSize.y,
					imageDataArray[randomBatterImageIndex]->color.x, imageDataArray[randomBatterImageIndex]->color.y,
					imageDataArray[randomBatterImageIndex]->color.z, imageDataArray[randomBatterImageIndex]->color.w * alpha,
					imageDataArray[randomBatterImageIndex]->rotation);
			}
		}

		if (selectedIndex >= 0 && selectedIndex < BATTER_COUNT)
		{
			if (batterNameTagSprite[selectedIndex] && batterNameTagData[selectedIndex])
			{
				DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(batterNameTagPosition);
				DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(batterNameTagSize);

				batterNameTagSprite[selectedIndex]->render(dc,
					scaledPosition.x, scaledPosition.y,
					scaledSize.x, scaledSize.y,
					batterNameTagColor.x, batterNameTagColor.y,
					batterNameTagColor.z, batterNameTagColor.w * alpha,
					batterNameTagData[selectedIndex]->rotation);
			}
		}
	}
	else if(currentState == ShowState::ShowBatterParam)
	{
		if (randomBatterImageIndex >= 0 && randomBatterImageIndex < BATTER_IMAGE_COUNT)
		{
			if (imageData[randomBatterImageIndex] && imageDataArray[randomBatterImageIndex])
			{
				DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(showBatterParamPosition);
				DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(showBatterParamSize);

				imageData[randomBatterImageIndex]->render(dc,
					scaledPosition.x,
					scaledPosition.y,
					scaledSize.x, scaledSize.y,
					imageDataArray[randomBatterImageIndex]->color.x, imageDataArray[randomBatterImageIndex]->color.y,
					imageDataArray[randomBatterImageIndex]->color.z, imageDataArray[randomBatterImageIndex]->color.w * alpha,
					imageDataArray[randomBatterImageIndex]->rotation);
			}
		}

		//ランク・ボーナス・パッシブ能力のフォント描画
		auto scaledText = [&](const DirectX::XMFLOAT2& scaledPos, float scaledSize) -> std::pair<DirectX::XMFLOAT2, float>
			{
				return {
					screenScaler.Scale(scaledPos),
					scaledSize * screenScaler.GetUniformScale()
				};
			};

		auto [scaledRankPos, scaledRankSize] = scaledText(rankFontData.position, rankFontData.scale);
		auto [scaledHomerunBonusPos, scaledHomerunBonusSize] = scaledText(homerunBonusFontData.position, homerunBonusFontData.scale);
		auto [scaledBreakingBallPos, scaledBreakingBallSize] = scaledText(breakingBallBonusFontData.position, breakingBallBonusFontData.scale);
		auto [scaledPassivePos, scaledPassiveSize] = scaledText(passiveFontData.position, passiveFontData.scale);
		auto [scaledPassiveParamPos, scaledPassiveParamSize] = scaledText(passiveParamFontData.position, passiveParamFontData.scale);
		auto [scaledNotPassivePos, scaledNotPassiveSize] = scaledText(notPassiveFontData.position, notPassiveFontData.scale);

		//左ぞろえにするヘルパー関数
		auto leftAlignTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
			{
				float textWidth = 0.0f;
				float textHeight = 0.0f;
				paramFontRenderer.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
				return { x, y - textHeight / 2.0f };
			};

		const std::string rankString = u8"ランク:  " + std::to_string(Player::Instance().GetSelectedRealBatterRank());
		DirectX::XMFLOAT2 centeredRankPos = leftAlignTextPosition(rankString, scaledRankSize, scaledRankPos.x, scaledRankPos.y);
		paramFontRenderer.DrawTextW(dc, rankString.c_str(),
			centeredRankPos.x, centeredRankPos.y, scaledRankSize,
			rankFontData.color.x, rankFontData.color.y, rankFontData.color.z, rankFontData.color.w * alpha);


		const std::string homerunBonusString = u8"初期ホームランボーナス:  " + FormatBonusFloat(homerunBonus);
		DirectX::XMFLOAT2 centeredHomerunBonusPos = leftAlignTextPosition(homerunBonusString, scaledHomerunBonusSize, scaledHomerunBonusPos.x, scaledHomerunBonusPos.y);
		paramFontRenderer.DrawTextW(dc, homerunBonusString.c_str(),
			centeredHomerunBonusPos.x, centeredHomerunBonusPos.y, scaledHomerunBonusSize,
			homerunBonusFontData.color.x, homerunBonusFontData.color.y, homerunBonusFontData.color.z, homerunBonusFontData.color.w * alpha);

		const std::string breakingBallBonusString = u8"初期変化球ボーナス:  " + FormatBonusFloat(breakingBallBonus);
		DirectX::XMFLOAT2 centeredBreakingBallBonusPos = leftAlignTextPosition(breakingBallBonusString, scaledBreakingBallSize, scaledBreakingBallPos.x, scaledBreakingBallPos.y);
		paramFontRenderer.DrawTextW(dc, breakingBallBonusString.c_str(),
			centeredBreakingBallBonusPos.x, centeredBreakingBallBonusPos.y, scaledBreakingBallSize,
			breakingBallBonusFontData.color.x, breakingBallBonusFontData.color.y, breakingBallBonusFontData.color.z, breakingBallBonusFontData.color.w * alpha);

		const std::string passiveBonusString = u8"初期配布能力 　";
		DirectX::XMFLOAT2 centeredPassiveBonusPos = leftAlignTextPosition(passiveBonusString, scaledPassiveSize, scaledPassivePos.x, scaledPassivePos.y);
		paramFontRenderer.DrawTextW(dc, passiveBonusString.c_str(),
			centeredPassiveBonusPos.x, centeredPassiveBonusPos.y, scaledPassiveSize,
			passiveFontData.color.x, passiveFontData.color.y, passiveFontData.color.z, passiveFontData.color.w * alpha);

		if(Player::Instance().GetSelectedRealBatterRank() >= 3)
		{
			const std::string notPassiveBonusString = u8"なし";
			DirectX::XMFLOAT2 centeredNotPassiveBonusPos = leftAlignTextPosition(notPassiveBonusString, scaledNotPassiveSize, scaledNotPassivePos.x, scaledNotPassivePos.y);
			paramFontRenderer.DrawTextW(dc, notPassiveBonusString.c_str(),
				centeredNotPassiveBonusPos.x, centeredNotPassiveBonusPos.y, scaledNotPassiveSize,
				notPassiveFontData.color.x, notPassiveFontData.color.y, notPassiveFontData.color.z, notPassiveFontData.color.w * alpha);

			const std::string passiveParamString = u8"初期能力はランク2以下のバッターのみ配布";
			DirectX::XMFLOAT2 centeredPassiveParamPos = leftAlignTextPosition(passiveParamString, scaledPassiveParamSize, scaledPassiveParamPos.x, scaledPassiveParamPos.y);
			paramFontRenderer.DrawTextW(dc, passiveParamString.c_str(),
				centeredPassiveParamPos.x, centeredPassiveParamPos.y, scaledPassiveParamSize,
				passiveParamFontData.color.x, passiveParamFontData.color.y, passiveParamFontData.color.z, passiveParamFontData.color.w * alpha);
		}
	}
}

void ScrollView::Update(float elapsedTime)
{
	if (!initialized) return;
	//効果音更新処理
	if (hoverArrow) hoverArrow->Update();
	if (clickNameButton) clickNameButton->Update();


	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();
	DirectX::XMFLOAT2 scaledScrollBackgroundPos = screenScaler.Scale(scrollBackgroundSpriteData[0].position);
	DirectX::XMFLOAT2 scaledScrollBackgroundSize = screenScaler.ScaleSize(scrollBackgroundSpriteData[0].size);
	

	// スクロールビューの更新処理
	//マウスカーソルの位置がスクロールビューの背景の範囲内にあるかどうか
	Input& input = Input::Instance();
	if (input.GetMouse().GetPositionX() >= scaledScrollBackgroundPos.x - scaledScrollBackgroundSize.x / 2.0f &&
		input.GetMouse().GetPositionX() <= scaledScrollBackgroundPos.x + scaledScrollBackgroundSize.x / 2.0f &&
		input.GetMouse().GetPositionY() >= scaledScrollBackgroundPos.y - scaledScrollBackgroundSize.y / 2.0f &&
		input.GetMouse().GetPositionY() <= scaledScrollBackgroundPos.y + scaledScrollBackgroundSize.y / 2.0f)
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

	DirectX::XMFLOAT2 scaledTopCapPos = screenScaler.Scale(topCapData.position);
	DirectX::XMFLOAT2 scaledTopCapSize = screenScaler.ScaleSize(topCapData.size);
	DirectX::XMFLOAT2 scaledBottomCapPos = screenScaler.Scale(bottomCapData.position);
	DirectX::XMFLOAT2 scaledBottomCapSize = screenScaler.ScaleSize(bottomCapData.size);

	DirectX::XMFLOAT2 scaledTopArrowPos = screenScaler.Scale(topArrowData.position);
	DirectX::XMFLOAT2 scaledTopArrowSize = screenScaler.ScaleSize(originalArrowSize);
	DirectX::XMFLOAT2 scaledBottomArrowPos = screenScaler.Scale(bottomArrowData.position);
	DirectX::XMFLOAT2 scaledBottomArrowSize = screenScaler.ScaleSize(originalArrowSize);

	bool topHovered =
		input.GetMouse().GetPositionX() >= scaledTopArrowPos.x - scaledTopArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionX() <= scaledTopArrowPos.x + scaledTopArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionY() >= scaledTopArrowPos.y - scaledTopArrowSize.y / 2.0f &&
		input.GetMouse().GetPositionY() <= scaledTopArrowPos.y + scaledTopArrowSize.y / 2.0f;

	bool bottomHovered =
		input.GetMouse().GetPositionX() >= scaledBottomArrowPos.x - scaledBottomArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionX() <= scaledBottomArrowPos.x + scaledBottomArrowSize.x / 2.0f &&
		input.GetMouse().GetPositionY() >= scaledBottomArrowPos.y - scaledBottomArrowSize.y / 2.0f &&
		input.GetMouse().GetPositionY() <= scaledBottomArrowPos.y + scaledBottomArrowSize.y / 2.0f;

	bool topPressed = topHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT); // 押しっぱなし判定
	bool bottomPressed = bottomHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT);

	if (showTopArrow)
	{
		topArrowData.size = topPressed ? originalArrowSize : (topHovered ? targetArrowSize : originalArrowSize);
		if (topHovered && !topPressed && hoverArrow && !hoverSoundPlayed)
		{
			hoverArrow->Update();
			hoverArrow->PlayOneShot();
			hoverSoundPlayed = true;
		}
	}

	if (showBottomArrow)
	{
		bottomArrowData.size = bottomPressed ? originalArrowSize : (bottomHovered ? targetArrowSize : originalArrowSize);
		if (bottomHovered && !bottomPressed && hoverArrow && !hoverSoundPlayed)
		{
			hoverArrow->Update();
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
	if(input.GetMouse().GetButtonDown() & input.GetMouse().BTN_LEFT && showTopArrow)
	{
		bool isHovered = input.GetMouse().GetPositionX() >= scaledTopArrowPos.x - scaledTopArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionX() <= scaledTopArrowPos.x + scaledTopArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionY() >= scaledTopArrowPos.y - scaledTopArrowSize.y / 2.0f &&
			input.GetMouse().GetPositionY() <= scaledTopArrowPos.y + scaledTopArrowSize.y / 2.0f;
		if(isHovered)
		{
			scrollOffsetY -= 110.0f; // 上方向にスクロール
			arrowClicked = true;

			//効果音再生
			if (hoverArrow)
			{
				hoverArrow->Update();
				hoverArrow->PlayOneShot();
			}
		}
	}

	if(input.GetMouse().GetButtonDown() & input.GetMouse().BTN_LEFT && showBottomArrow)
	{
		bool isHovered = input.GetMouse().GetPositionX() >= scaledBottomArrowPos.x - scaledBottomArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionX() <= scaledBottomArrowPos.x + scaledBottomArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionY() >= scaledBottomArrowPos.y - scaledBottomArrowSize.y / 2.0f &&
			input.GetMouse().GetPositionY() <= scaledBottomArrowPos.y + scaledBottomArrowSize.y / 2.0f;
		if(isHovered)
		{
			scrollOffsetY += 110.0f; // 下方向にスクロール
			arrowClicked = true;
			if(hoverArrow)
			{
				hoverArrow->Update();
				hoverArrow->PlayOneShot();
			}
		}
	}

	//capの範囲にあるボタンは押せないようにする
	bool isInCapArea = false;

	if(input.GetMouse().GetPositionX() >= scaledTopCapPos.x - scaledTopCapSize.x / 2.0f &&
		input.GetMouse().GetPositionX() <= scaledTopCapPos.x + scaledTopCapSize.x / 2.0f &&
		input.GetMouse().GetPositionY() >= scaledTopCapPos.y - scaledTopCapSize.y / 2.0f &&
		input.GetMouse().GetPositionY() <= scaledTopCapPos.y + scaledTopCapSize.y / 2.0f ||
		input.GetMouse().GetPositionX() >= scaledBottomCapPos.x - scaledBottomCapSize.x / 2.0f &&
		input.GetMouse().GetPositionX() <= scaledBottomCapPos.x + scaledBottomCapSize.x / 2.0f &&
		input.GetMouse().GetPositionY() >= scaledBottomCapPos.y - scaledBottomCapSize.y / 2.0f &&
		input.GetMouse().GetPositionY() <= scaledBottomCapPos.y + scaledBottomCapSize.y / 2.0f)
	{
		isInCapArea = true;
	}



	if (!arrowClicked && !isInCapArea)
	{
		//ボタンを押すことができる範囲の設定
		float visibleMinY = scaledTopCapPos.y + scaledTopCapSize.y / 2.0f;
		float visibleMaxY = scaledBottomCapPos.y - scaledBottomCapSize.y / 2.0f;

		//ボタンの押下処理
		//ボタンを押したら、テクスチャを黄色くする
		for (size_t i = 0; i < playerButtonDataList.size(); ++i)
		{
			float rawTopY = (startPosY + i * (buttonHeight + buttonSpacing)) - scrollOffsetY;
			DirectX::XMFLOAT2 scaledButtonPos = screenScaler.Scale({ playerButtonDataList[i].position.x, rawTopY });
			DirectX::XMFLOAT2 scaledButtonSize = screenScaler.ScaleSize(playerButtonDataList[i].size);

			float buttonTopY = scaledButtonPos.y - scaledButtonSize.y / 2.0f;
			float buttonBottomY = scaledButtonPos.y + scaledButtonSize.y / 2.0f;

			bool isVisible = (buttonBottomY >= visibleMinY) && (buttonTopY <= visibleMaxY);

			if (!isVisible)
			{
				continue;
			}
			

			bool hovered = input.GetMouse().GetPositionX() >= scaledButtonPos.x - scaledButtonSize.x / 2.0f &&
				input.GetMouse().GetPositionX() <= scaledButtonPos.x + scaledButtonSize.x / 2.0f &&
				input.GetMouse().GetPositionY() >= buttonTopY &&
				input.GetMouse().GetPositionY() <= buttonBottomY;

			

			//　ボタンが押されたときの処理(長押しはロックする)
			if (hovered && (input.GetMouse().GetButtonDown() & input.GetMouse().BTN_LEFT))
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

	if (ImGui::CollapsingHeader("ScrollView Settings"))
	{
		if (ImGui::CollapsingHeader("batterList"))
		{
			ImGui::DragFloat2("Position", &batterListPos.x, 1.0f);
			ImGui::DragFloat2("Size", &batterListData->size.x, 1.0f);
			ImGui::ColorEdit4("Color", &batterListData->color.x);
		}

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
			
			if(ImGui::CollapsingHeader("Rank Font"))
			{
				ImGui::DragFloat2("Rank Font Position", &rankFontData.position.x, 1.0f);
				ImGui::DragFloat("Rank Font Scale", &rankFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Rank Font Color", &rankFontData.color.x);
			}
			if(ImGui::CollapsingHeader("Homerun Bonus Font"))
			{
				ImGui::DragFloat2("Homerun Bonus Font Position", &homerunBonusFontData.position.x, 1.0f);
				ImGui::DragFloat("Homerun Bonus Font Scale", &homerunBonusFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Homerun Bonus Font Color", &homerunBonusFontData.color.x);
			}
			if(ImGui::CollapsingHeader("Breaking Ball Bonus Font"))
			{
				ImGui::DragFloat2("Breaking Ball Bonus Font Position", &breakingBallBonusFontData.position.x, 1.0f);
				ImGui::DragFloat("Breaking Ball Bonus Font Scale", &breakingBallBonusFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Breaking Ball Bonus Font Color", &breakingBallBonusFontData.color.x);
			}
			if(ImGui::CollapsingHeader("Passive Font"))
			{
				ImGui::DragFloat2("Passive Font Position", &passiveFontData.position.x, 1.0f);
				ImGui::DragFloat("Passive Font Scale", &passiveFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Passive Font Color", &passiveFontData.color.x);
			}
			if(ImGui::CollapsingHeader("Passive Param Font"))
			{
				ImGui::DragFloat2("Passive Param Font Position", &passiveParamFontData.position.x, 1.0f);
				ImGui::DragFloat("Passive Param Font Scale", &passiveParamFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Passive Param Font Color", &passiveParamFontData.color.x);
			}
			if(ImGui::CollapsingHeader("Not Passive Font"))
			{
				ImGui::DragFloat2("Not Passive Font Position", &notPassiveFontData.position.x, 1.0f);
				ImGui::DragFloat("Not Passive Font Scale", &notPassiveFontData.scale, 0.01f, 0.1f, 10.0f);
				ImGui::ColorEdit4("Not Passive Font Color", &notPassiveFontData.color.x);
			}
			
		}

		//バッターパラメータの位置など調整
		if(ImGui::CollapsingHeader("Batter Param Adjustments"))
		{
			ImGui::DragFloat2("Show Batter Param Position", &showBatterParamPosition.x, 1.0f);
			ImGui::DragFloat2("Show Batter Param Size", &showBatterParamSize.x, 1.0f);
		}
	}

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
		{"contactRankFontColor", {contactRankFontData.color.x, contactRankFontData.color.y, contactRankFontData.color.z, contactRankFontData.color.w}},
		{"rankFontPosition", {rankFontData.position.x, rankFontData.position.y}},
		{"rankFontScale", rankFontData.scale},
		{"rankFontColor", {rankFontData.color.x, rankFontData.color.y, rankFontData.color.z, rankFontData.color.w}},
		{"homerunBonusFontPosition", {homerunBonusFontData.position.x, homerunBonusFontData.position.y}},
		{"homerunBonusFontScale", homerunBonusFontData.scale},
		{"homerunBonusFontColor", {homerunBonusFontData.color.x, homerunBonusFontData.color.y, homerunBonusFontData.color.z, homerunBonusFontData.color.w}},
		{"breakingBallBonusFontPosition", {breakingBallBonusFontData.position.x, breakingBallBonusFontData.position.y}},
		{"breakingBallBonusFontScale", breakingBallBonusFontData.scale},
		{"breakingBallBonusFontColor", {breakingBallBonusFontData.color.x, breakingBallBonusFontData.color.y, breakingBallBonusFontData.color.z, breakingBallBonusFontData.color.w}},
		{"passiveFontPosition", {passiveFontData.position.x, passiveFontData.position.y}},
		{"passiveFontScale", passiveFontData.scale},
		{"passiveFontColor", {passiveFontData.color.x, passiveFontData.color.y, passiveFontData.color.z, passiveFontData.color.w}},
		{"passiveParamFontPosition", {passiveParamFontData.position.x, passiveParamFontData.position.y}},
		{"passiveParamFontScale", passiveParamFontData.scale},
		{"passiveParamFontColor", {passiveParamFontData.color.x, passiveParamFontData.color.y, passiveParamFontData.color.z, passiveParamFontData.color.w}}
	};
}

void ScrollView::LoadFromJson(const nlohmann::json& json)
{
	if (!json.is_object()) return;
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
		if(fontSettingsJson.contains("rankFontPosition"))
		{
			rankFontData.position.x = fontSettingsJson["rankFontPosition"][0].get<float>();
			rankFontData.position.y = fontSettingsJson["rankFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("rankFontScale"))
		{
			rankFontData.scale = fontSettingsJson["rankFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("rankFontColor"))
		{
			rankFontData.color.x = fontSettingsJson["rankFontColor"][0].get<float>();
			rankFontData.color.y = fontSettingsJson["rankFontColor"][1].get<float>();
			rankFontData.color.z = fontSettingsJson["rankFontColor"][2].get<float>();
			rankFontData.color.w = fontSettingsJson["rankFontColor"][3].get<float>();
		}
		if(fontSettingsJson.contains("homerunBonusFontPosition"))
		{
			homerunBonusFontData.position.x = fontSettingsJson["homerunBonusFontPosition"][0].get<float>();
			homerunBonusFontData.position.y = fontSettingsJson["homerunBonusFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("homerunBonusFontScale"))
		{
			homerunBonusFontData.scale = fontSettingsJson["homerunBonusFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("homerunBonusFontColor"))
		{
			homerunBonusFontData.color.x = fontSettingsJson["homerunBonusFontColor"][0].get<float>();
			homerunBonusFontData.color.y = fontSettingsJson["homerunBonusFontColor"][1].get<float>();
			homerunBonusFontData.color.z = fontSettingsJson["homerunBonusFontColor"][2].get<float>();
			homerunBonusFontData.color.w = fontSettingsJson["homerunBonusFontColor"][3].get<float>();
		}
		if(fontSettingsJson.contains("breakingBallBonusFontPosition"))
		{
			breakingBallBonusFontData.position.x = fontSettingsJson["breakingBallBonusFontPosition"][0].get<float>();
			breakingBallBonusFontData.position.y = fontSettingsJson["breakingBallBonusFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("breakingBallBonusFontScale"))
		{
			breakingBallBonusFontData.scale = fontSettingsJson["breakingBallBonusFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("breakingBallBonusFontColor"))
		{
			breakingBallBonusFontData.color.x = fontSettingsJson["breakingBallBonusFontColor"][0].get<float>();
			breakingBallBonusFontData.color.y = fontSettingsJson["breakingBallBonusFontColor"][1].get<float>();
			breakingBallBonusFontData.color.z = fontSettingsJson["breakingBallBonusFontColor"][2].get<float>();
			breakingBallBonusFontData.color.w = fontSettingsJson["breakingBallBonusFontColor"][3].get<float>();
		}
		if(fontSettingsJson.contains("passiveFontPosition"))
		{
			passiveFontData.position.x = fontSettingsJson["passiveFontPosition"][0].get<float>();
			passiveFontData.position.y = fontSettingsJson["passiveFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("passiveFontScale"))
		{
			passiveFontData.scale = fontSettingsJson["passiveFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("passiveFontColor"))
		{
			passiveFontData.color.x = fontSettingsJson["passiveFontColor"][0].get<float>();
			passiveFontData.color.y = fontSettingsJson["passiveFontColor"][1].get<float>();
			passiveFontData.color.z = fontSettingsJson["passiveFontColor"][2].get<float>();
			passiveFontData.color.w = fontSettingsJson["passiveFontColor"][3].get<float>();
		}
		if(fontSettingsJson.contains("passiveParamFontPosition"))
		{
			passiveParamFontData.position.x = fontSettingsJson["passiveParamFontPosition"][0].get<float>();
			passiveParamFontData.position.y = fontSettingsJson["passiveParamFontPosition"][1].get<float>();
		}
		if (fontSettingsJson.contains("passiveParamFontScale"))
		{
			passiveParamFontData.scale = fontSettingsJson["passiveParamFontScale"].get<float>();
		}
		if (fontSettingsJson.contains("passiveParamFontColor"))
		{
			passiveParamFontData.color.x = fontSettingsJson["passiveParamFontColor"][0].get<float>();
			passiveParamFontData.color.y = fontSettingsJson["passiveParamFontColor"][1].get<float>();
			passiveParamFontData.color.z = fontSettingsJson["passiveParamFontColor"][2].get<float>();
			passiveParamFontData.color.w = fontSettingsJson["passiveParamFontColor"][3].get<float>();
		}
	}
}