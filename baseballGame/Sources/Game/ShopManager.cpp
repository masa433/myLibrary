#include "ShopManager.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include "UiEasing.h"
#include "Money.h"
#include "RoundManager.h"
#include "input.h"
#include "ballCount.h"
#include "batSprite.h"

bool ShopManager::AbilityIsOwned() const
{
	return SpecialAbility::Instance().IsOwned();
}

void ShopManager::ApplyPowerUp(int power)
{
	Player::Instance().IncreaseBaseBatterPower(power);
}

void ShopManager::ApplyContactUp(int contact)
{
	Player::Instance().IncreaseBaseBatterContact(contact);
}

void ShopManager::IncreaseBallCount(int count)
{
	ballCount::Instance().IncreaseInitialBalls(count);
}

void ShopManager::IncreaseHomeRunMultiplier(float multiplier)
{
	Money::Instance().IncreaseHomerunBonus(multiplier);
}

void ShopManager::IncreaseBreakingBallMultiplier(float multiplier)
{
	Money::Instance().IncreaseBreakingBallBonus(multiplier);
}

void ShopManager::PitcherPowerRankDown(int penalty)
{
	shopPowerRankDown += penalty;
	ballSprite::Instance().ApplyShopPowerRankDown(shopPowerRankDown);
}

void ShopManager::PitcherBreakBallRankDown(int penalty)
{
	shopBreakRankDown += penalty;
	ballSprite::Instance().ApplyShopBreakRankDown(shopBreakRankDown);
}

void ShopManager::EnableMeetAssist()
{
	BatSprite::Instance().SetMeetAssistEnabled(true);
}

void ShopManager::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.ReleaseAndGetAddressOf());
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.ReleaseAndGetAddressOf(), input_layout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));

	shopBackSpriteData = std::make_unique<ShopSprite>();
	shopBackSpriteData->texturePath = L".\\resources\\textures\\shopBoard.png";
	shopBackSpriteData->position = { 960.0f, 540.0f }; // 適切な位置に配置
	shopBackSpriteData->size = { 1800.0f, 1000.0f }; // 適切なサイズに設定
	shopBackSpriteData->rotation = 0.0f;
	shopBackSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	shopBackSprite = std::make_unique<sprite>(device, context, shopBackSpriteData->texturePath.c_str());

	currentOffsetY = startOffsetY;

	for(int i = 0; i < BATTER_COUNT; ++i)
	{
		batterSpriteData[i] = std::make_unique<ShopSprite>();
		batterSpriteData[i]->texturePath = L".\\resources\\textures\\batterParameter\\batterParameter" + std::to_wstring(i + 1) + L".png";
		batterSpriteData[i]->position = { batterParamBackPosition.x, batterParamBackPosition.y }; // 適切な位置に配置
		batterSpriteData[i]->size = { batterParamBackSize.x, batterParamBackSize.y }; // 適切なサイズに設定
		batterSpriteData[i]->rotation = 0.0f;
		batterSpriteData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		batterSprites[i] = std::make_unique<sprite>(device, context, batterSpriteData[i]->texturePath.c_str());
	}

	static std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	moneyFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		50.0f,
		screenWidth,
		screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GarpSansNormalItalic.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	powerFontData.position = { 1275.0f, 590.0f }; // 画面内に配置
	powerFontData.scale = 1.5f;                    // スケールを1.0に設定
	powerFontData.color = { 0.0f, 0.0f, 0.0f, 1.0f }; // 不透明な白色

	contactFontData.position = { 1275.0f, 730.0f };
	contactFontData.scale = 1.5f;
	contactFontData.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	powerRankFontData.position = { 1550.0f, 590.0f };
	powerRankFontData.scale = 1.5f;
	powerRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	contactRankFontData.position = { 1550.0f, 730.0f };
	contactRankFontData.scale = 1.5f;
	contactRankFontData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	InitializeShopButtonSprites(device, context);

}

void ShopManager::InitializeShopButtonSprites(ID3D11Device* device, ID3D11DeviceContext* context)
{
	BuildShopItem();

	for (int i = 0; i < SHOP_ITEM_COUNT; ++i)
	{
		shopItemSprites[i] = std::make_unique<ShopSprite>();
		shopItemSprites[i]->texturePath = shopItems[i].texturePath;
		shopItemSprites[i]->size = shopItemSize;
		shopItemSprites[i]->rotation = 0.0f;
		shopItemSprites[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		shopItemSpriteObjects[i] = std::make_unique<sprite>(device, context, shopItemSprites[i]->texturePath.c_str());
	}
}

void ShopManager::BuildShopItem()
{
	auto& a = shopItems;
	int index = 0;

	// ここでshopItemsにアイテムを追加する処理を行う
	//パワーアップレベル1
	a[index].id = ShopItemID::PowerUp;
	a[index].name = u8"パワーアップLv1";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\powerLevel1.png";
	a[index].price = 300;
	a[index].appearanceRate = 10.0f;// 10%の確率で出現
	a[index].level = 1;
	a[index].powerUp = 1;
	a[index].onButtonPressed = [this, power = a[index].powerUp]() { this->ApplyPowerUp(power); }; // ボタンが押されたときの処理を設定
	++index;

	//パワーアップレベル2
	a[index].id = ShopItemID::PowerUp;
	a[index].name = u8"パワーアップLv2";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\powerLevel2.png";
	a[index].price = 600;
	a[index].appearanceRate = 5.0f;// 5%の確率で出現
	a[index].level = 2;
	a[index].powerUp = 3;
	a[index].onButtonPressed = [this, power = a[index].powerUp]() { this->ApplyPowerUp(power); }; // ボタンが押されたときの処理を設定
	++index;
	
	//パワーアップレベル3
	a[index].id = ShopItemID::PowerUp;
	a[index].name = u8"パワーアップLv3";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\powerLevel3.png";
	a[index].price = 1000;
	a[index].appearanceRate = 2.0f;// 2%の確率で出現
	a[index].level = 3;
	a[index].powerUp = 5;
	a[index].onButtonPressed = [this, power = a[index].powerUp]() { this->ApplyPowerUp(power); }; // ボタンが押されたときの処理を設定
	++index;

	//ミートアップレベル1
	a[index].id = ShopItemID::ContactUp;
	a[index].name = u8"ミートアップLv1";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\meetLevel1.png";
	a[index].price = 300;
	a[index].appearanceRate = 10.0f;// 10%の確率で出現
	a[index].level = 1;
	a[index].contactUp = 1;
	a[index].onButtonPressed = [this, contact = a[index].contactUp]() { this->ApplyContactUp(contact); }; // ボタンが押されたときの処理を設定
	++index;

	//ミートアップレベル2
	a[index].id = ShopItemID::ContactUp;
	a[index].name = u8"ミートアップLv2";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\meetLevel2.png";
	a[index].price = 600;
	a[index].appearanceRate = 5.0f;// 5%の確率で出現
	a[index].level = 2;
	a[index].contactUp = 3;
	a[index].onButtonPressed = [this, contact = a[index].contactUp]() { this->ApplyContactUp(contact); }; // ボタンが押されたときの処理を設定
	++index;

	//ミートアップレベル3
	a[index].id = ShopItemID::ContactUp;
	a[index].name = u8"ミートアップLv3";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\meetLevel3.png";
	a[index].price = 1000;
	a[index].appearanceRate = 2.0f;// 2%の確率で出現
	a[index].level = 3;
	a[index].contactUp = 5;
	a[index].onButtonPressed = [this, contact = a[index].contactUp]() { this->ApplyContactUp(contact); }; // ボタンが押されたときの処理を設定
	++index;

	//ミートアシスト
	a[index].id = ShopItemID::ContactAsist;
	a[index].name = u8"ミートアシスト";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\meetAssist.png";
	a[index].price = 1000;
	a[index].appearanceRate = 100.0f;// 5%の確率で出現
	a[index].level = 1;
	a[index].onButtonPressed = [this]() { this->EnableMeetAssist(); }; // ボタンが押されたときの処理を設定
	++index;


	//球数増加
	a[index].id = ShopItemID::BallIncrease;
	a[index].name = u8"球数増加";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\ballIncrease.png";
	a[index].price = 500;
	a[index].appearanceRate = 8.0f;// 8%の確率で出現
	a[index].level = 1;
	a[index].increaseBallCount = 1;
	a[index].onButtonPressed = [this, count = a[index].increaseBallCount]() { this->IncreaseBallCount(count); }; // ボタンが押されたときの処理を設定
	++index;

	//風無効
	a[index].id = ShopItemID::WindDisable;
	a[index].name = u8"風無効";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\wind.png";
	a[index].price = 500;
	a[index].appearanceRate = 8.0f;// 8%の確率で出現
	a[index].level = 1;
	++index;

	//球威ワンランクダウン
	a[index].id = ShopItemID::PitchPowerDown;
	a[index].name = u8"球威ワンランクダウン";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\pitcherPowerDown.png";
	a[index].price = 1000;
	a[index].appearanceRate = 5.0f;// 5%の確率で出現
	a[index].level = 1;
	a[index].pitcherPowerPenalty = 1; 
	a[index].onButtonPressed = [this, penalty = a[index].pitcherPowerPenalty]() { this->PitcherPowerRankDown(penalty); }; // ボタンが押されたときの処理を設定
	++index;

	//変化量ワンランクダウン
	a[index].id = ShopItemID::PitchBreakDown;
	a[index].name = u8"変化量ワンランクダウン";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\pitcherBreakDown.png";
	a[index].price = 1000;
	a[index].appearanceRate = 5.0f;// 5%の確率で出現
	a[index].level = 1;
	a[index].pitcherBreakBallPenalty = 1;
	a[index].onButtonPressed = [this, penalty = a[index].pitcherBreakBallPenalty]() { this->PitcherBreakBallRankDown(penalty); }; // ボタンが押されたときの処理を設定
	++index;

	//球種減少
	a[index].id = ShopItemID::PitchTypeDecrease;
	a[index].name = u8"球種減少";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\pitchTypeDecrease.png";
	a[index].price = 1500;
	a[index].appearanceRate = 5.0f;// 5%の確率で出現
	a[index].level = 1;
	++index;

	//ホームラン倍率アップ
	a[index].id = ShopItemID::HomeRunMultiplier;
	a[index].name = u8"ホームラン倍率アップ";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\homerunMultiplyUp.png";
	a[index].price = 800;
	a[index].appearanceRate = 7.0f;// 7%の確率で出現
	a[index].level = 1;
	a[index].homerunMultiplierUp = 0.1f; // ホームラン倍率を10%増加
	a[index].onButtonPressed = [this, multiplier = a[index].homerunMultiplierUp]() { this->IncreaseHomeRunMultiplier(multiplier); }; // ボタンが押されたときの処理を設定
	++index;

	//変化球倍率アップ
	a[index].id = ShopItemID::BreakingBallMultiplier;
	a[index].name = u8"変化球倍率アップ";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\breakingBallMultiplyUp.png";
	a[index].price = 800;
	a[index].appearanceRate = 7.0f;// 7%の確率で出現
	a[index].level = 1;
	a[index].breakingBallMultiplierUp = 0.1f; // 変化球倍率を10%増加
	a[index].onButtonPressed = [this, multiplier = a[index].breakingBallMultiplierUp]() { this->IncreaseBreakingBallMultiplier(multiplier); }; // ボタンが押されたときの処理を設定
	++index;

	//重力変化
	a[index].id = ShopItemID::GravityChange;
	a[index].name = u8"重力変化";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\gravityChange.png";
	a[index].price = 1000;
	a[index].appearanceRate = 7.0f;// 7%の確率で出現
	a[index].level = 1;
	a[index].gravityChange = 0.8f; // 重力を80%に変更
	++index;

	//特殊能力発動率アップ
	a[index].id = ShopItemID::SpecialAbilityActiveRateUp;
	a[index].name = u8"特殊能力発動率アップ";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\specialAbilityActiveRateUp.png";
	a[index].price = 1200;
	a[index].appearanceRate = 7.0f;// 7%の確率で出現
	a[index].level = 1;
	a[index].specialAbilityActiveRateUp = 5.0f; // 特殊能力発動率を5%増加
	a[index].isButtonVisible = [this]() { return AbilityIsOwned(); };// 特殊能力を所有している場合のみ表示
	++index;

	//ネット減少
	a[index].id = ShopItemID::NetDecrease;
	a[index].name = u8"ネット減少";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\netDecrease.png";
	a[index].price = 2000;
	a[index].appearanceRate = 5.0f;// 5%の確率で出現
	a[index].level = 1;
	a[index].netDecrease = 1; //ネットを減らす数
	++index;

	//半額
	a[index].id = ShopItemID::HalfPrice;
	a[index].name = u8"半額";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\halfPrice.png";
	a[index].price = 3000;
	a[index].appearanceRate = 5.0f;// 5%の確率で出現
	a[index].level = 1;
	a[index].targetHomerun = 1;
	++index;
	
	//無料
	a[index].id = ShopItemID::FreePrice;
	a[index].name = u8"無料";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\free.png";
	a[index].price = 5000;
	a[index].appearanceRate = 3.0f;// 3%の確率で出現
	a[index].level = 1;
	a[index].targetHomerun = 3;
	++index;

	//リロール
	a[index].id = ShopItemID::Reroll;
	a[index].name = u8"リロール";
	a[index].texturePath = L".\\resources\\textures\\shopIcon\\reroll.png";
	a[index].price = 100;
	a[index].appearanceRate = 100.0f;// 100%の確率で出現
	a[index].level = 1;
	a[index].onButtonPressed = [this]() { this->RerollShopItems(); }; // ボタンが押されたときの処理を設定
	++index;
}

void ShopManager::Uninitialize()
{
	shopBackSprite.reset();
	shopBackSpriteData.reset();
	pixel_shader.Reset();
	vertex_shader.Reset();
	input_layout.Reset();
}

void ShopManager::Update(float elapsedTime)
{
	// 右シフトキーが「押された瞬間」だけパワーを1増やす
	static bool isRShiftPressed = false;
	if (GetAsyncKeyState(VK_RSHIFT) & 0x8000)
	{
		if (!isRShiftPressed)
		{
			Player::Instance().IncreaseBaseBatterPower(1);
			isRShiftPressed = true;
		}
	}
	else
	{
		isRShiftPressed = false;
	}

	// 右コントロールキーが「押された瞬間」だけミートを1増やす
	static bool isRControlPressed = false;
	if (GetAsyncKeyState(VK_RCONTROL) & 0x8000)
	{
		if (!isRControlPressed)
		{
			Player::Instance().IncreaseBaseBatterContact(1);
			isRControlPressed = true;
		}
	}
	else
	{
		isRControlPressed = false;
	}

	UpdateShopItem();

	if(!isAnimating)
	{
		return; // アニメーション中でない場合は更新しない
	}

	if(isShopOpen)
	{
		// タイマーを加算
		easingTimer += elapsedTime;

		// 0.0f ～ 1.0f の範囲にクランプ
		float t = easingTimer / easingDuration;
		if (t > 1.0f) t = 1.0f;

		// イージングで現在位置を計算
		currentOffsetY = UiEasing::Lerp(startOffsetY, targetOffsetY, t, UiEasing::EasingType::OutBack);
		if (t >= 1.0f)
		{
			isAnimating = false; // アニメーション終了
		}
	}
	else if(isShopClosed)
	{
		// タイマーを加算
		easingTimer += elapsedTime;
		// 0.0f ～ 1.0f の範囲にクランプ
		//倍速で再生する
		float t = easingTimer / (easingDuration * 0.5f);
		if (t > 1.0f) t = 1.0f;
		// イージングで現在位置を計算
		currentOffsetY = UiEasing::Lerp(startOffsetY, targetOffsetY, t, UiEasing::EasingType::InCubic);
		if (t >= 1.0f)
		{
			isAnimating = false; // アニメーション終了
			isShopClosed = false; // ショップが閉じた状態にする
		}
	}

	
}

void ShopManager::UpdateShopItem()
{
	Input& input = Input::Instance();

	//マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY());
	bool clicked = input.GetMouse().GetButtonDown() & Mouse::BTN_LEFT;

	for(int i = 0; i < currentShopItemIndices.size(); ++i)
	{
		ShopData& item = shopItems[currentShopItemIndices[i]];
		float itemX = shopItemPositions[i].x - shopItemSize.x / 2.0f;
		float itemY = (shopItemPositions[i].y + currentOffsetY) - shopItemSize.y / 2.0f;
		if(mousePos.x >= itemX && mousePos.x <= itemX + shopItemSize.x &&
		   mousePos.y >= itemY && mousePos.y <= itemY + shopItemSize.y && !item.isPurchased)
		{
			item.isHover = true;

			if(clicked)
			{
				if(Money::Instance().GetCurrentMoney() >= item.price)
				{
					Money::Instance().DecreaseMoney(item.price);

					if (item.id != ShopItemID::Reroll)
					{
						item.isPurchased = true;
					}

					if(item.onButtonPressed)
					{
						item.onButtonPressed();
					}
				}
			}
		}
		else
		{
			item.isHover = false;
		}
	}
}

void ShopManager::Render()
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	context->IASetInputLayout(input_layout.Get());

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);


	if (shopBackSprite)
	{
		float drawX = baseShopBackPos.x - shopBackSpriteData->size.x / 2.0f;
		float drawY = (baseShopBackPos.y + currentOffsetY) - shopBackSpriteData->size.y / 2.0f;

		shopBackSprite->render(context,
			drawX,
			drawY,
			shopBackSpriteData->size.x,
			shopBackSpriteData->size.y,
			shopBackSpriteData->color.x,
			shopBackSpriteData->color.y,
			shopBackSpriteData->color.z,
			shopBackSpriteData->color.w,
			shopBackSpriteData->rotation);
	}

	float moneyWidth, moneyHeight;

	moneyFont.MeasureText(std::to_string(Money::Instance().GetCurrentMoney()).c_str(), moneyFontScale, moneyWidth, moneyHeight);

	float adjustedX = baseMoneyFontPos.x - moneyWidth / 2.0f; // 中央揃えのためにX座標を調整
	float adjustedY = (baseMoneyFontPos.y + currentOffsetY) - moneyHeight / 2.0f; // 中央揃えのためにY座標を調整

	moneyFont.DrawTextW(context, std::to_string(Money::Instance().GetCurrentMoney()).c_str(),
		adjustedX, adjustedY,
		moneyFontScale,
		moneyFontColor.x, moneyFontColor.y, moneyFontColor.z, moneyFontColor.w);

	context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	context->IASetInputLayout(input_layout.Get());

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	//現在選択されているバッターを取得
	Player::RealBatter currentBatterIndex = Player::Instance().GetSelectedRealBatter();
	selectedBatterIndex = static_cast<int>(currentBatterIndex) - 1;

	if (Pitcher::Instance().GetCurrentState() == Pitcher::State::SelectingPitch && RoundManager::Instance().IsShopState())
	{
		if (selectedBatterIndex >= 0 && selectedBatterIndex < BATTER_COUNT)
		{
			float drawX = batterParamBackPosition.x - batterParamBackSize.x / 2.0f;
			float drawY = (baseBatterParamPos.y + currentOffsetY) - batterParamBackSize.y / 2.0f;

			//選択されているバッターのアイコンを描画
			batterSprites[selectedBatterIndex]->render(context,
				drawX,
				drawY,
				batterParamBackSize.x, batterParamBackSize.y,
				batterSpriteData[selectedBatterIndex]->color.x, batterSpriteData[selectedBatterIndex]->color.y, batterSpriteData[selectedBatterIndex]->color.z, batterSpriteData[selectedBatterIndex]->color.w,
				batterSpriteData[selectedBatterIndex]->rotation);
		}


		if (fontRenderer.IsValid())
		{
			/*fontRenderer.DrawTextW(dc, "55", 1300.0f, 200.0f, 1.5f, 1.0f, 1.0f, 1.0f, alpha);*/

			//選択中の選手のパワーとミートの値を取得
			if (selectedBatterIndex >= 0 && selectedBatterIndex < BATTER_COUNT)
			{
				int power = Player::Instance().GetSelectedRealBatterPower();
				int contact = Player::Instance().GetSelectedRealBatterContact();
				// 関数側が inline const RankData& GetPowerRank(int power) const のような場合
				Player::BatterPowerRank powerRank = Player::Instance().GetPowerRank(power);
				Player::BatterContactRank contactRank = Player::Instance().GetContactRank(contact);

				//ランクに応じて色を変える
				switch (powerRank)
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

				// ラムダ関数を使って、パワーとミートのY座標を計算
				auto GetPositionForPowerAndContact = [this](float& outPowerY, float& outContactY,float& outPowerRankY,float& outContactRankY)
				{
					
					outPowerY = powerFontData.position.y + currentOffsetY;
					outContactY = contactFontData.position.y + currentOffsetY;
					outPowerRankY = powerRankFontData.position.y + currentOffsetY;
					outContactRankY = contactRankFontData.position.y + currentOffsetY;
				};

				float drawPowerY, drawContactY, drawPowerRankY, drawContactRankY;
				GetPositionForPowerAndContact(drawPowerY, drawContactY, drawPowerRankY, drawContactRankY);
				

				fontRenderer.DrawTextW(context, std::to_string(power).c_str(),
					powerFontData.position.x, drawPowerY, powerFontData.scale,
					powerFontData.color.x, powerFontData.color.y, powerFontData.color.z, powerFontData.color.w );

				fontRenderer.DrawTextW(context, GetBatterPowerRankString(powerRank),
					powerRankFontData.position.x, drawPowerRankY, powerRankFontData.scale,
					powerRankFontData.color.x, powerRankFontData.color.y, powerRankFontData.color.z, powerRankFontData.color.w );

				fontRenderer.DrawTextW(context, std::to_string(contact).c_str(),
					contactFontData.position.x, drawContactY, contactFontData.scale,
					contactFontData.color.x, contactFontData.color.y, contactFontData.color.z, contactFontData.color.w );
				fontRenderer.DrawTextW(context, GetBatterContactRankString(contactRank),
					contactRankFontData.position.x, drawContactRankY, contactRankFontData.scale,
					contactRankFontData.color.x, contactRankFontData.color.y, contactRankFontData.color.z, contactRankFontData.color.w );

			}

		}


	}


	context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	context->IASetInputLayout(input_layout.Get());

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	for(int slotIndex = 0; slotIndex < SHOP_ITEM_DISPLAY_COUNT; ++slotIndex)
	{
		if (slotIndex >= static_cast<int>(currentShopItemIndices.size())) break;

		int itemIndex = currentShopItemIndices[slotIndex];
		const ShopData& item = shopItems[itemIndex];
		
		if (!shopItemSpriteObjects[itemIndex]) continue;

		float drawX = shopItemPositions[slotIndex].x - shopItemSize.x / 2.0f;
		float drawY = (shopItemPositions[slotIndex].y + currentOffsetY) - shopItemSize.y / 2.0f;

		//ホバー時か購入できない状態の時か購入済みに色を変える
		bool canPurchase = Money::Instance().GetCurrentMoney() >= item.price && !item.isPurchased;

		float colorRGB = item.isHover ? 0.7f : (canPurchase ? 1.0f : 0.7f);

		shopItemSpriteObjects[itemIndex]->render(context,
			drawX,
			drawY,
			shopItemSize.x, shopItemSize.y,
			colorRGB, colorRGB, colorRGB, 1.0f,
			0.0f);
	}

	//シェーダーの設定を解除
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	context->IASetInputLayout(nullptr);

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void ShopManager::DrawGUI()
{
	if(ImGui::CollapsingHeader("ShopManager"))
	{
		ImGui::DragFloat2("fontPosition", &moneyFontPosition.x, 1.0f, 0.0f);
		ImGui::DragFloat("fontScale", &moneyFontScale, 0.01f, 0.1f, 10.0f);

		ImGui::DragFloat2("batterParamBackPosition", &batterParamBackPosition.x, 1.0f, 0.0f);
		ImGui::DragFloat2("batterParamBackSize", &batterParamBackSize.x, 1.0f, 0.0f);
		ImGui::Separator();
		ImGui::DragFloat2("powerFontPosition", &powerFontData.position.x, 1.0f, 0.0f);
		ImGui::DragFloat("powerFontScale", &powerFontData.scale, 0.01f, 0.1f, 10.0f);
		ImGui::ColorEdit4("powerFontColor", &powerFontData.color.x);
		ImGui::DragFloat2("contactFontPosition", &contactFontData.position.x, 1.0f, 0.0f);
		ImGui::DragFloat("contactFontScale", &contactFontData.scale, 0.01f, 0.1f, 10.0f);
		ImGui::ColorEdit4("contactFontColor", &contactFontData.color.x);
		ImGui::DragFloat2("powerRankFontPosition", &powerRankFontData.position.x, 1.0f, 0.0f);
		ImGui::DragFloat("powerRankFontScale", &powerRankFontData.scale, 0.01f, 0.1f, 10.0f);
		ImGui::DragFloat2("contactRankFontPosition", &contactRankFontData.position.x, 1.0f, 0.0f);
		ImGui::DragFloat("contactRankFontScale", &contactRankFontData.scale, 0.01f, 0.1f, 10.0f);
	}
}

void ShopManager::SaveToJson(json& j)
{
	j["moneyFontPosition"] = { moneyFontPosition.x, moneyFontPosition.y };
	j["moneyFontScale"] = moneyFontScale;
	j["batterParamBackPosition"] = { batterParamBackPosition.x, batterParamBackPosition.y };
	j["batterParamBackSize"] = { batterParamBackSize.x, batterParamBackSize.y };
	j["powerFontData"] = { powerFontData.position.x, powerFontData.position.y, powerFontData.scale, powerFontData.color.x, powerFontData.color.y, powerFontData.color.z, powerFontData.color.w };
	j["contactFontData"] = { contactFontData.position.x, contactFontData.position.y, contactFontData.scale, contactFontData.color.x, contactFontData.color.y, contactFontData.color.z, contactFontData.color.w };
	j["powerRankFontData"] = { powerRankFontData.position.x, powerRankFontData.position.y, powerRankFontData.scale, powerRankFontData.color.x, powerRankFontData.color.y, powerRankFontData.color.z, powerRankFontData.color.w };
	j["contactRankFontData"] = { contactRankFontData.position.x, contactRankFontData.position.y, contactRankFontData.scale, contactRankFontData.color.x, contactRankFontData.color.y, contactRankFontData.color.z, contactRankFontData.color.w };
}

void ShopManager::LoadFromJson(const json& j)
{
	if (j.contains("moneyFontPosition") && j["moneyFontPosition"].is_array() && j["moneyFontPosition"].size() == 2)
	{
		moneyFontPosition.x = j["moneyFontPosition"][0].get<float>();
		moneyFontPosition.y = j["moneyFontPosition"][1].get<float>();
	}
	if (j.contains("moneyFontScale"))
	{
		moneyFontScale = j["moneyFontScale"].get<float>();
	}
	if (j.contains("batterParamBackPosition") && j["batterParamBackPosition"].is_array() && j["batterParamBackPosition"].size() == 2)
	{
		batterParamBackPosition.x = j["batterParamBackPosition"][0].get<float>();
		batterParamBackPosition.y = j["batterParamBackPosition"][1].get<float>();
	}
	if (j.contains("batterParamBackSize") && j["batterParamBackSize"].is_array() && j["batterParamBackSize"].size() == 2)
	{
		batterParamBackSize.x = j["batterParamBackSize"][0].get<float>();
		batterParamBackSize.y = j["batterParamBackSize"][1].get<float>();
	}
	if (j.contains("powerFontData") && j["powerFontData"].is_array() && j["powerFontData"].size() == 7)
	{
		powerFontData.position.x = j["powerFontData"][0].get<float>();
		powerFontData.position.y = j["powerFontData"][1].get<float>();
		powerFontData.scale = j["powerFontData"][2].get<float>();
		powerFontData.color.x = j["powerFontData"][3].get<float>();
		powerFontData.color.y = j["powerFontData"][4].get<float>();
		powerFontData.color.z = j["powerFontData"][5].get<float>();
		powerFontData.color.w = j["powerFontData"][6].get<float>();
	}
	if (j.contains("contactFontData") && j["contactFontData"].is_array() && j["contactFontData"].size() == 7)
	{
		contactFontData.position.x = j["contactFontData"][0].get<float>();
		contactFontData.position.y = j["contactFontData"][1].get<float>();
		contactFontData.scale = j["contactFontData"][2].get<float>();
		contactFontData.color.x = j["contactFontData"][3].get<float>();
		contactFontData.color.y = j["contactFontData"][4].get<float>();
		contactFontData.color.z = j["contactFontData"][5].get<float>();
		contactFontData.color.w = j["contactFontData"][6].get<float>();
	}
	if (j.contains("powerRankFontData") && j["powerRankFontData"].is_array() && j["powerRankFontData"].size() == 7)
	{
		powerRankFontData.position.x = j["powerRankFontData"][0].get<float>();
		powerRankFontData.position.y = j["powerRankFontData"][1].get<float>();
		powerRankFontData.scale = j["powerRankFontData"][2].get<float>();
		powerRankFontData.color.x = j["powerRankFontData"][3].get<float>();
		powerRankFontData.color.y = j["powerRankFontData"][4].get<float>();
		powerRankFontData.color.z = j["powerRankFontData"][5].get<float>();
		powerRankFontData.color.w = j["powerRankFontData"][6].get<float>();
	}
	if (j.contains("contactRankFontData") && j["contactRankFontData"].is_array() && j["contactRankFontData"].size() == 7)
	{
		contactRankFontData.position.x = j["contactRankFontData"][0].get<float>();
		contactRankFontData.position.y = j["contactRankFontData"][1].get<float>();
		contactRankFontData.scale = j["contactRankFontData"][2].get<float>();
		contactRankFontData.color.x = j["contactRankFontData"][3].get<float>();
		contactRankFontData.color.y = j["contactRankFontData"][4].get<float>();
		contactRankFontData.color.z = j["contactRankFontData"][5].get<float>();
		contactRankFontData.color.w = j["contactRankFontData"][6].get<float>();
	}

}