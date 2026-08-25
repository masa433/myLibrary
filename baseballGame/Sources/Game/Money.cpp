#include "Money.h"
#include "Graphics.h"
#include "shader.h"
#include <imgui.h>
#include "ballDistance.h"
#include "Ball.h"
#include "ballSprite.h"
#include <cstdio>
#include <string>

// 小数点以下の不要な 0 を削除する関数(小数第1位は消さない)
std::string FormatFloat(float value)
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

void Money::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	moneyData = std::make_unique<MoneyData>();
	moneyData->texturePath = L".\\resources\\textures\\moneyCountBack.png";
	moneyData->position = { moneyPosition.x, moneyPosition.y };
	moneyData->size = { moneySize.x, moneySize.y };
	moneyData->rotation = 0.0f;
	moneyData->color = { moneyColor.x, moneyColor.y, moneyColor.z, moneyColor.w };
	moneySprite = std::make_unique<sprite>(device, context, moneyData->texturePath.c_str());

	bonusItems.clear();
	std::vector<int> bonusCodepoints = FontRenderer::Utf8ToCodepoints(u8"0123456789.xG ");

	// ホームランボーナスアイテムの初期化
	BonusItem homeRunBonusItem;
	homeRunBonusItem.name = "HomeRun";
	homeRunBonusItem.info = { { 300.0f, 50.0f }, { 500.0f, 80.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	homeRunBonusItem.data = std::make_unique<MoneyData>();
	homeRunBonusItem.data->texturePath = L".\\resources\\textures\\homeRunBonusBoard.png";
	homeRunBonusItem.sprite = std::make_unique<sprite>(device, context, homeRunBonusItem.data->texturePath.c_str());
	homeRunBonusItem.fontRenderer = std::make_unique<FontRenderer>();
	homeRunBonusItem.fontRenderer->Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		static_cast<int>(Graphics::Instance().GetScreenWidth()),
		static_cast<int>(Graphics::Instance().GetScreenHeight()),
		512, 512,
		&bonusCodepoints);
	bonusItems.push_back(std::move(homeRunBonusItem));

	// ボールゾーンボーナスアイテムの初期化
	BonusItem ballZoneBonusItem;
	ballZoneBonusItem.name = "BallZone";
	ballZoneBonusItem.info = { { 300.0f, 120.0f }, { 500.0f, 80.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	ballZoneBonusItem.data = std::make_unique<MoneyData>();
	ballZoneBonusItem.data->texturePath = L".\\resources\\textures\\ballZoneBonusBoard.png";
	ballZoneBonusItem.sprite = std::make_unique<sprite>(device, context, ballZoneBonusItem.data->texturePath.c_str());
	ballZoneBonusItem.fontRenderer = std::make_unique<FontRenderer>();
	ballZoneBonusItem.fontRenderer->Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		static_cast<int>(Graphics::Instance().GetScreenWidth()),
		static_cast<int>(Graphics::Instance().GetScreenHeight()),
		512, 512,
		&bonusCodepoints);
	bonusItems.push_back(std::move(ballZoneBonusItem));

	// 変化球ボーナスアイテムの初期化
	BonusItem breakingBallBonusItem;
	breakingBallBonusItem.name = "BreakingBall";
	breakingBallBonusItem.info = { { 300.0f, 190.0f }, { 500.0f, 80.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	breakingBallBonusItem.data = std::make_unique<MoneyData>();
	breakingBallBonusItem.data->texturePath = L".\\resources\\textures\\breakingBallBonusBoard.png";
	breakingBallBonusItem.sprite = std::make_unique<sprite>(device, context, breakingBallBonusItem.data->texturePath.c_str());
	breakingBallBonusItem.fontRenderer = std::make_unique<FontRenderer>();
	breakingBallBonusItem.fontRenderer->Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		static_cast<int>(Graphics::Instance().GetScreenWidth()),
		static_cast<int>(Graphics::Instance().GetScreenHeight()),
		512, 512,
		&bonusCodepoints);
	bonusItems.push_back(std::move(breakingBallBonusItem));

	// コンボボーナスアイテムの初期化
	BonusItem comboBonusItem;
	comboBonusItem.name = "Combo";
	comboBonusItem.info = { { 300.0f, 190.0f }, { 500.0f, 80.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	comboBonusItem.data = std::make_unique<MoneyData>();
	comboBonusItem.data->texturePath = L".\\resources\\textures\\comboBonusBoard.png";
	comboBonusItem.sprite = std::make_unique<sprite>(device, context, comboBonusItem.data->texturePath.c_str());
	comboBonusItem.fontRenderer = std::make_unique<FontRenderer>();
	comboBonusItem.fontRenderer->Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		static_cast<int>(Graphics::Instance().GetScreenWidth()),
		static_cast<int>(Graphics::Instance().GetScreenHeight()),
		512, 512,
		&bonusCodepoints);
	bonusItems.push_back(std::move(comboBonusItem));

	BonusItem totalItem;
	totalItem.name = "Total";
	totalItem.info = { { 300.0f, 260.0f }, { 500.0f, 80.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } };
	totalItem.data = std::make_unique<MoneyData>();
	totalItem.data->texturePath = L".\\resources\\textures\\totalBoard.png";
	totalItem.sprite = std::make_unique<sprite>(device, context, totalItem.data->texturePath.c_str());
	totalItem.fontRenderer = std::make_unique<FontRenderer>();
	totalItem.fontRenderer->Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		static_cast<int>(Graphics::Instance().GetScreenWidth()),
		static_cast<int>(Graphics::Instance().GetScreenHeight()),
		512, 512,
		&bonusCodepoints);
	bonusItems.push_back(std::move(totalItem));

	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	std::vector<int> resultCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789");
	moneyFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&resultCodepoints);

	currentMoney = 0;
	targetMoney = 0;

}

void Money::Uninitialize()
{
	moneyFont.Uninitialize();
	moneySprite.reset();
	moneyData.reset();
	consoleLog = nullptr;
}

void Money::Update(float elapsedTime)
{
	bool isLocked = BallDistance::Instance().GetDistanceLock();

	//ファールの時は、お金が増えないようにする
	if (Ball::Instance().GetIsFoulConfirmed()) return;

	

	if (isLocked && !prevDistanceLocked)
	{
		float baseDistance = BallDistance::Instance().GetCurrentDistance();
		float totalMultiplier = 1.0f;

		bool isHomeRun = Ball::Instance().GetHasPassedHomeRunZone() || Ball::Instance().GetHasCollidedWithPole();

		TriggerBonusAnimation(isHomeRun, Pitcher::Instance().IsBreakingBallBonus());

		// ホームランボーナスを適用
		if (isHomeRun)
		{
			totalMultiplier *= homerunBonus;
			/*OutputDebugStringA("ホームランボーナスが適用されました。\n");*/
			if(consoleLog)
			{
				consoleLog->push_back(u8"[Info]ホームランボーナスが適用されました。");
				consoleLog->push_back(u8"[Info]現在のホームランボーナス倍率: " + FormatFloat(homerunBonus));
			}
		}

		// 変化球ボーナスを適用
		if(Pitcher::Instance().IsBreakingBallBonus() && isHomeRun)
		{
			
			// 変化球ボーナスの倍率を設定
			float breakingBonus = breakingBallBonus;
			totalMultiplier *= breakingBonus;
			if(consoleLog)
			{
				consoleLog->push_back(u8"[Info]変化球ボーナスが適用されました。");
				consoleLog->push_back(u8"[Info]現在の変化球ボーナス倍率: " + FormatFloat(breakingBonus));
			}
		}

		if(Combo::Instance().GetCurrentCombo() >= 2 && isHomeRun)
		{
			comboBonus = 1.0f + (Combo::Instance().GetCurrentCombo()) * comboBonusIncrement;
			totalMultiplier *= comboBonus;
			if(consoleLog)
			{
				consoleLog->push_back(u8"[Info]コンボボーナスが適用されました。");
				consoleLog->push_back(u8"[Info]現在のコンボボーナス倍率: " + FormatFloat(comboBonus));
			}
		}

		// 最終的な距離に倍率を適用して加算
		finalDistance = static_cast<int>(std::round(baseDistance * totalMultiplier));
		AddMoney(finalDistance);
		
	}

	if(isBonusAnimating)
	{
		bonusAnimTimer += elapsedTime;

		
		const float delayPerItem = 0.1f; // 各アイテムのアニメーション開始の遅延時間

		bool allItemsFinished = true;

		for(size_t i = 0; i < bonusItems.size(); ++i)
		{
			auto& item = bonusItems[i];
			if (!item.isActive) continue;

			//アイテムごとのアニメーション開始時間を計算
			float myTime = bonusAnimTimer - (i * delayPerItem);//アイテムごとの遅延を考慮

			if (myTime < 0.0f)
			{
				item.currentPos = item.startPos; // 遅延中はスタート位置に固定
				allItemsFinished = false; // まだ全てのアイテムが終了していない
				continue;
			}

			// アニメーションの進行度を計算
			float myProgress = myTime / BONUS_ANIM_DURATION;//	0.0fから1.0fの範囲に正規化

			if (myProgress < 1.0f)
			{
				allItemsFinished = false; // まだ全てのアイテムが終了していない

				if (myProgress < 0.2f)
				{
					float t = myProgress / 0.2f; // 0.0fから1.0fの範囲に正規化
					item.currentPos = UiEasing::Lerp(item.startPos, item.targetPos, t, UiEasing::EasingType::OutBack);
				}
				else if (myProgress > 0.8f)
				{
					float t = (myProgress - 0.8f) / 0.2f; // 0.0fから1.0fの範囲に正規化
					item.currentPos = UiEasing::Lerp(item.targetPos, item.startPos, t, UiEasing::EasingType::InBack);
				}
				else
				{
					item.currentPos = item.targetPos; // 中間の時間帯はターゲット位置に固定
				}
			}
			else
			{
				// 自分のアニメーション完了（最終位置に固定）
				item.currentPos = item.startPos;
			}
		}

		if (allItemsFinished)
		{
			isBonusAnimating = false;
		}

	}

	if (currentMoney < targetMoney)
	{
		currentMoney++;
	}

	// 現在の状態を保存
	prevDistanceLocked = isLocked;
}

void Money::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨

	if (moneySprite && moneyData)
	{
		moneySprite->render(dc,
			moneyPosition.x, moneyPosition.y,
			moneySize.x, moneySize.y,
			moneyColor.x, moneyColor.y, moneyColor.z, moneyColor.w,
			moneyData->rotation);
	}

	if (isBonusAnimating)
	{
		for (const auto& item : bonusItems)
		{
			if (item.isActive && item.sprite)
			{
				item.sprite->render(dc,
					item.currentPos.x, item.currentPos.y,
					item.info.size.x, item.info.size.y,
					item.info.color.x, item.info.color.y, item.info.color.z, item.info.color.w,
					0.0f);
			}
		}

		for (const auto& item : bonusItems)
		{
			if (item.fontRenderer && item.isActive)
			{
				std::string bonusText;
				if (item.name == "HomeRun")
				{
					bonusText = " x " + FormatFloat(homerunBonus);
				}
				else if (item.name == "BallZone")
				{
					bonusText = std::to_string(currentBallZoneBonusMoney) + " G";
				}
				else if (item.name == "BreakingBall")
				{
					bonusText = " x " + FormatFloat(breakingBallBonus);
				}
				else if (item.name == "Combo")
				{
					bonusText = " x " + FormatFloat(comboBonus);
				}
				else if (item.name == "Total")
				{
					bonusText = std::to_string(finalDistance) + " G";
				}
				float fontSize = 1.5f; // フォントサイズを適切に設定
				float textWidth = 0.0f;
				float textHeight = 100.0f;
				item.fontRenderer->MeasureText(bonusText.c_str(), fontSize, textWidth, textHeight);
				float textX = item.currentPos.x + (item.info.size.x - textWidth) / 2.0f; // 中央揃え
				float textY = item.currentPos.y + (item.info.size.y - textHeight) / 2.0f; // 中央揃え
				item.fontRenderer->DrawText(dc, bonusText.c_str(),
					textX + textXOffset, textY + textYOffset,
					fontSize,
					1.0f, 1.0f, 1.0f, 1.0f); // 白色で描画
			}
		}
		
	}

	if(moneyFont.IsValid())
	{
		std::string moneyText = std::to_string(currentMoney);

		auto centerTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
			{
				float textWidth = 0.0f;
				float textHeight = 0.0f;
				moneyFont.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
				return { x - textWidth / 2.0f, y - textHeight / 2.0f };
			};

		DirectX::XMFLOAT2 fontPos = centerTextPosition(moneyText, moneyTextScale, moneyTextPosition.x, moneyTextPosition.y);

		
		moneyFont.DrawTextW(dc, moneyText.c_str(),
			fontPos.x, fontPos.y,
			moneyTextScale,
			moneyTextColor.x, moneyTextColor.y, moneyTextColor.z, moneyTextColor.w);
	}

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void Money::DrawGUI()
{
	if(ImGui::CollapsingHeader("Money Sprite"))
	{
		ImGui::DragFloat2("Position", &moneyPosition.x);
		ImGui::DragFloat2("Size", &moneySize.x);
		ImGui::ColorEdit4("Color", &moneyColor.x);
	}
	if(ImGui::CollapsingHeader("Money Text"))
	{
		ImGui::DragFloat2("Text Position", &moneyTextPosition.x);
		ImGui::DragFloat("Text Scale", &moneyTextScale);
		ImGui::ColorEdit4("Text Color", &moneyTextColor.x);
	}

	if (ImGui::CollapsingHeader("BonusText"))
	{
		ImGui::DragFloat("Text X", &textXOffset);
		ImGui::DragFloat("Text Y", &textYOffset);
	}

	ImGui::DragInt("Current Money", &currentMoney);
}

void Money::SaveToJson(json& j)
{
	
	j["moneyPosition"] = { moneyPosition.x, moneyPosition.y };
	j["moneySize"] = { moneySize.x, moneySize.y };
	j["moneyColor"] = { moneyColor.x, moneyColor.y, moneyColor.z, moneyColor.w };
	j["moneyTextPosition"] = { moneyTextPosition.x, moneyTextPosition.y };
	j["moneyTextScale"] = moneyTextScale;
	j["moneyTextColor"] = { moneyTextColor.x, moneyTextColor.y, moneyTextColor.z, moneyTextColor.w };
	j["textXOffset"] = textXOffset; 
	j["textYOffset"] = textYOffset;
}

void Money::LoadFromJson(const json& j)
{
	if (j.contains("moneyPosition"))
	{
		moneyPosition.x = j["moneyPosition"][0].get<float>();
		moneyPosition.y = j["moneyPosition"][1].get<float>();
	}
	if (j.contains("moneySize"))
	{
		moneySize.x = j["moneySize"][0].get<float>();
		moneySize.y = j["moneySize"][1].get<float>();
	}
	if (j.contains("moneyColor"))
	{
		moneyColor.x = j["moneyColor"][0].get<float>();
		moneyColor.y = j["moneyColor"][1].get<float>();
		moneyColor.z = j["moneyColor"][2].get<float>();
		moneyColor.w = j["moneyColor"][3].get<float>();
	}
	if (j.contains("moneyTextPosition"))
	{
		moneyTextPosition.x = j["moneyTextPosition"][0].get<float>();
		moneyTextPosition.y = j["moneyTextPosition"][1].get<float>();
	}
	if (j.contains("moneyTextScale")) moneyTextScale = j["moneyTextScale"].get<float>();
	if (j.contains("moneyTextColor"))
	{
		moneyTextColor.x = j["moneyTextColor"][0].get<float>();
		moneyTextColor.y = j["moneyTextColor"][1].get<float>();
		moneyTextColor.z = j["moneyTextColor"][2].get<float>();
		moneyTextColor.w = j["moneyTextColor"][3].get<float>();
	}
	if (j.contains("textXOffset")) textXOffset = j["textXOffset"].get<float>();
	if (j.contains("textYOffset")) textYOffset = j["textYOffset"].get<float>();
}

void Money::TriggerBonusAnimation(bool isHomeRun, bool isBreaking)
{
	float startY = 310.0f; // 初期Y座標
	float spacingY = 90.0f; // ボーナスアイテム間の垂直間隔
	int activeCount = 0;

	for (auto& bonusItem : bonusItems)
	{
		bonusItem.isActive = false; // まず全てのボーナスアイテムを非アクティブにする

		if (bonusItem.name == "HomeRun" && isHomeRun)
		{
			bonusItem.isActive = true;
		}
		else if (bonusItem.name == "BreakingBall" && isHomeRun && isBreaking)
		{
			bonusItem.isActive = true;
		}
		else if(bonusItem.name == "Combo" && isHomeRun && Combo::Instance().GetCurrentCombo() >= 2)
		{
			bonusItem.isActive = true;
		}
		else if(bonusItem.name == "Total")
		{
			bonusItem.isActive = true; // Totalは常に表示
		}

		if (bonusItem.isActive)
		{
			float targetY = startY + (activeCount * spacingY);

			bonusItem.startPos = { 2000.0f, targetY };
			bonusItem.targetPos = { 1500.0f, targetY };
			bonusItem.currentPos = bonusItem.startPos;

			activeCount++;
		}
	}

	if (activeCount > 0)
	{
		isBonusAnimating = true;
		bonusAnimTimer = 0.0f;
	}
	else
	{
		isBonusAnimating = false;
	}
}

void Money::TriggerBallZoneBonusAnimation()
{
	float startY = 310.0f; // 初期Y座標
	
	for (auto& bonusItem : bonusItems)
	{
		if (bonusItem.name == "BallZone")
		{
			bonusItem.isActive = true;
			bonusItem.startPos = { 2000.0f, startY };
			bonusItem.targetPos = { 1500.0f, startY };
			bonusItem.currentPos = bonusItem.startPos;
		}
		else
		{
			bonusItem.isActive = false;
		}
	}
	
	isBonusAnimating = true;
	bonusAnimTimer = 0.0f;
}