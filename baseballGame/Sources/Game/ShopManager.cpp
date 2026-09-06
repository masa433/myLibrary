#include "ShopManager.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include "UiEasing.h"
#include "Money.h"
#include "RoundManager.h"


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

				//ラムダ式
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