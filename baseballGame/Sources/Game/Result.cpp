#include "Result.h"
#include "Graphics.h"
#include "imgui.h"
#include "sceneTransition.h"
#include "Money.h"
#include "Combo.h"
#include "RoundManager.h"

void Result::Initialize(ID3D11Device* device)
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
	// スプライトの初期化
	resultSpriteData = std::make_unique<Sprite>();
	resultSpriteData->texturePath = L".\\resources\\textures\\scrollViewBack.png";
	resultSpriteData->position = { spritePosition.x, spritePosition.y };
	resultSpriteData->size = { spriteSize.x, spriteSize.y };
	resultSpriteData->rotation = 0.0f;
	resultSpriteData->color = { spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w };
	resultSprite = std::make_unique<sprite>(device, context, resultSpriteData->texturePath.c_str());
	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	std::vector<int> resultCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"総ホームラン数0123456789本"
		u8"最高飛距離m"
	u8"所持金G"
	u8"最大コンボ数"
	u8"到達ラウンド数");
	resultFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		4096.0f, 4096.0f,
		&resultCodepoints);

	buttonManager.Initialize();

	hexTransitionEffect.Initialize();

	isResultToTitle = false;
	isResultToBatterSelect = false;
	isResultToRetry = false;

	currentState = State::Result;

}

void Result::Uninitialize()
{
	resultFont.Uninitialize();
	resultSprite.reset();
	resultSpriteData.reset();
	hexTransitionEffect.Reset();
	buttonManager.Uninitialize();
}

void Result::Update(float elapsedTime)
{
	


	switch (currentState)
	{
		case State::Result:
		{
			buttonManager.Update(elapsedTime);

			//タイトルシーンに戻るボタンの更新処理
			if (!isResultToTitle && !isResultToRetry && !isResultToBatterSelect)
			{
				if (buttonManager.IsTitleRequested())
				{
					isResultToTitle = true;
					hexTransitionEffect.Start(1.0f);
					buttonManager.ResetTitleRequest(false);
					currentState = State::Transition;
				}
				if (buttonManager.IsRetryRequested())
				{
					isResultToRetry = true;
					hexTransitionEffect.Start(1.0f);
					buttonManager.ResetRetryRequest(false);
					currentState = State::Transition;
				}
				if (buttonManager.IsBatterSelectRequested())
				{
					isResultToBatterSelect = true;
					hexTransitionEffect.Start(1.0f);
					buttonManager.ResetBatterSelectRequest(false);
					currentState = State::Transition;
				}
			}

			break;
		}

		case State::Transition:
		{
			hexTransitionEffect.Update(elapsedTime);

			if (hexTransitionEffect.IsFinished())
			{
				if (isResultToTitle)
				{
					ChangeSceneGameToTitle();
				}
				else if (isResultToRetry)
				{
					ChangeSceneGameToGame();
				}
				else if (isResultToBatterSelect)
				{
					ChangeSceneGameToBatterSelect();
				}
			}
			break;
		}
	}


	
	
}

void Result::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨



	
	// スプライトの描画
	if (resultSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(spritePosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(spriteSize);

		resultSprite->render(dc,
			scaledPosition.x, scaledPosition.y,
			scaledSize.x, scaledSize.y,
			spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w,
			resultSpriteData->rotation);
	}
	// フォントの描画
	if (resultFont.IsValid())
	{

		float spacingY = 100.0f; // 各テキストの垂直間隔

		struct TextInfo
		{
			std::string label;
			std::string value;
			DirectX::XMFLOAT2 position;
			float fontSize;
			DirectX::XMFLOAT4 color;
		};

		auto scaledText = [&](const DirectX::XMFLOAT2& scaledPos, float scaledSize) -> std::pair<DirectX::XMFLOAT2, float>
		{
			return {
				screenScaler.Scale(scaledPos),
				scaledSize * screenScaler.GetUniformScale()
			};
		};

		

		TextInfo textInfos[] = {
			
			{ u8"総ホームラン数: ", std::to_string(HomeRunCount::Instance().GetTotalHomeRunCount()) + u8"本", homeRunFontPosition, homeRunFontSize, homeRunFontColor },
			{ u8"最高飛距離: ", std::to_string(static_cast<int>(BallDistance::Instance().GetMaxDistance())) + u8"m", distanceFontPosition, distanceFontSize, distanceFontColor },
			{ u8"最大コンボ数: ", std::to_string(Combo::Instance().GetMaxCombo()), comboFontPosition, comboFontSize, comboFontColor },
			{ u8"到達ラウンド数: ", std::to_string(RoundManager::Instance().GetCurrentRound()), roundFontPosition, roundFontSize, roundFontColor }
		};

		auto [scaledMoneyFontPosition, scaledMoneyFontSize] = scaledText(moneyFontPosition, moneyFontSize);

		std::string moneyText = u8"所持金: " + std::to_string(Money::Instance().GetCurrentMoney()) + u8"G";

		float moneyTextWidth, moneyTextHeight;
		resultFont.MeasureText(moneyText.c_str(), scaledMoneyFontSize, moneyTextWidth, moneyTextHeight);
		float drawX = scaledMoneyFontPosition.x - moneyTextWidth / 2.0f;
		float drawY = scaledMoneyFontPosition.y - moneyTextHeight / 2.0f;

		resultFont.DrawTextW(dc, moneyText.c_str(),
			drawX, drawY,
			scaledMoneyFontSize,
			moneyFontColor.x, moneyFontColor.y, moneyFontColor.z, moneyFontColor.w);


		for(const auto& textInfo : textInfos)
		{
			float textWidth, textHeight;

			auto [scaledPosition, scaledFontSize] = scaledText(textInfo.position, textInfo.fontSize);

			// テキストの幅と高さを計算して中央揃えの位置を決定
			resultFont.MeasureText(textInfo.label.c_str(), scaledFontSize, textWidth, textHeight);
			float labelX = scaledPosition.x - textWidth / 2.0f;
			float labelY = scaledPosition.y - textHeight / 2.0f;

			// テキストを描画
			resultFont.DrawTextW(dc,textInfo.label.c_str(),
				labelX, labelY, 
				scaledFontSize, 
				textInfo.color.x, textInfo.color.y, textInfo.color.z, textInfo.color.w);

			resultFont.MeasureText(textInfo.value.c_str(), scaledFontSize, textWidth, textHeight);
			float valueX = scaledPosition.x - textWidth / 2.0f; // ラベルの右側に配置
			float valueY = labelY + spacingY; // ラベルの下に配置

			resultFont.DrawTextW(dc,textInfo.value.c_str(),
				valueX, valueY, 
				scaledFontSize, 
				textInfo.color.x, textInfo.color.y, textInfo.color.z, textInfo.color.w);
		}
		

		
	}

	buttonManager.Render(1.0f, ButtonManager::ButtonType::Title);
	buttonManager.Render(1.0f, ButtonManager::ButtonType::Retry);
	buttonManager.Render(1.0f, ButtonManager::ButtonType::BatterSelect);


	if(isResultToTitle || isResultToRetry || isResultToBatterSelect)
	{
		hexTransitionEffect.Render();
	}

	// 描画後の状態をリセット
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

}

void Result::DrawGUI()
{
	if(ImGui::CollapsingHeader("Result Settings"))
	{
		ImGui::DragFloat2("Money Font Position", &moneyFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Money Font Size", &moneyFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Money Font Color", &moneyFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Home Run Font Position", &homeRunFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Home Run Font Size", &homeRunFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Home Run Font Color", &homeRunFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Distance Font Position", &distanceFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Distance Font Size", &distanceFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Distance Font Color", &distanceFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Combo Font Position", &comboFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Combo Font Size", &comboFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Combo Font Color", &comboFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Round Font Position", &roundFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Round Font Size", &roundFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Round Font Color", &roundFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Result Sprite Position", &spritePosition.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat2("Result Sprite Size", &spriteSize.x, 0.01f, 0.0f, 1.0f);
		ImGui::ColorEdit4("Result Sprite Color", &spriteColor.x);
	}

	buttonManager.DrawGUI();
}

void Result::SaveToJson(nlohmann::json& j)
{
	j["MoneyFontPosition"] = { moneyFontPosition.x, moneyFontPosition.y };
	j["MoneyFontSize"] = moneyFontSize;
	j["MoneyFontColor"] = { moneyFontColor.x, moneyFontColor.y, moneyFontColor.z, moneyFontColor.w };
	j["HomeRunFontPosition"] = { homeRunFontPosition.x, homeRunFontPosition.y };
	j["HomeRunFontSize"] = homeRunFontSize;
	j["HomeRunFontColor"] = { homeRunFontColor.x, homeRunFontColor.y, homeRunFontColor.z, homeRunFontColor.w };
	j["DistanceFontPosition"] = { distanceFontPosition.x, distanceFontPosition.y };
	j["DistanceFontSize"] = distanceFontSize;
	j["DistanceFontColor"] = { distanceFontColor.x, distanceFontColor.y, distanceFontColor.z, distanceFontColor.w };
	j["ResultSpritePosition"] = { spritePosition.x, spritePosition.y };
	j["ResultSpriteSize"] = { spriteSize.x, spriteSize.y };
	j["ResultSpriteColor"] = { spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w };
	j["ComboFontPosition"] = { comboFontPosition.x, comboFontPosition.y };
	j["ComboFontSize"] = comboFontSize;
	j["ComboFontColor"] = { comboFontColor.x, comboFontColor.y, comboFontColor.z, comboFontColor.w };
	j["RoundFontPosition"] = { roundFontPosition.x, roundFontPosition.y };
	j["RoundFontSize"] = roundFontSize;
	j["RoundFontColor"] = { roundFontColor.x, roundFontColor.y, roundFontColor.z, roundFontColor.w };

	buttonManager.SaveToJson(j);
}

void Result::LoadFromJson(const nlohmann::json& j)
{
	if(j.contains("MoneyFontPosition"))
	{
		moneyFontPosition.x = j["MoneyFontPosition"][0].get<float>();
		moneyFontPosition.y = j["MoneyFontPosition"][1].get<float>();
	}
	if (j.contains("MoneyFontSize"))
	{
		moneyFontSize = j["MoneyFontSize"].get<float>();
	}
	if(j.contains("MoneyFontColor"))
	{
		moneyFontColor.x = j["MoneyFontColor"][0].get<float>();
		moneyFontColor.y = j["MoneyFontColor"][1].get<float>();
		moneyFontColor.z = j["MoneyFontColor"][2].get<float>();
		moneyFontColor.w = j["MoneyFontColor"][3].get<float>();
	}
	if (j.contains("HomeRunFontPosition"))
	{
		homeRunFontPosition.x = j["HomeRunFontPosition"][0].get<float>();
		homeRunFontPosition.y = j["HomeRunFontPosition"][1].get<float>();
	}
	if (j.contains("HomeRunFontSize"))
	{
		homeRunFontSize = j["HomeRunFontSize"].get<float>();
	}
	if (j.contains("HomeRunFontColor"))
	{
		homeRunFontColor.x = j["HomeRunFontColor"][0].get<float>();
		homeRunFontColor.y = j["HomeRunFontColor"][1].get<float>();
		homeRunFontColor.z = j["HomeRunFontColor"][2].get<float>();
		homeRunFontColor.w = j["HomeRunFontColor"][3].get<float>();
	}
	if (j.contains("DistanceFontPosition"))
	{
		distanceFontPosition.x = j["DistanceFontPosition"][0].get<float>();
		distanceFontPosition.y = j["DistanceFontPosition"][1].get<float>();
	}
	if (j.contains("DistanceFontSize"))
	{
		distanceFontSize = j["DistanceFontSize"].get<float>();
	}
	if (j.contains("DistanceFontColor"))
	{
		distanceFontColor.x = j["DistanceFontColor"][0].get<float>();
		distanceFontColor.y = j["DistanceFontColor"][1].get<float>();
		distanceFontColor.z = j["DistanceFontColor"][2].get<float>();
		distanceFontColor.w = j["DistanceFontColor"][3].get<float>();
	}
	if(j.contains("ComboFontPosition"))
	{
		comboFontPosition.x = j["ComboFontPosition"][0].get<float>();
		comboFontPosition.y = j["ComboFontPosition"][1].get<float>();
	}
	if (j.contains("ComboFontSize"))
	{
		comboFontSize = j["ComboFontSize"].get<float>();
	}
	if (j.contains("ComboFontColor"))
	{
		comboFontColor.x = j["ComboFontColor"][0].get<float>();
		comboFontColor.y = j["ComboFontColor"][1].get<float>();
		comboFontColor.z = j["ComboFontColor"][2].get<float>();
		comboFontColor.w = j["ComboFontColor"][3].get<float>();
	}
	if (j.contains("RoundFontPosition"))
	{
		roundFontPosition.x = j["RoundFontPosition"][0].get<float>();
		roundFontPosition.y = j["RoundFontPosition"][1].get<float>();
	}
	if(j.contains("RoundFontSize"))
	{
		roundFontSize = j["RoundFontSize"].get<float>();
	}
	if(j.contains("RoundFontColor"))
	{
		roundFontColor.x = j["RoundFontColor"][0].get<float>();
		roundFontColor.y = j["RoundFontColor"][1].get<float>();
		roundFontColor.z = j["RoundFontColor"][2].get<float>();
		roundFontColor.w = j["RoundFontColor"][3].get<float>();
	}
	if (j.contains("ResultSpritePosition"))
	{
		spritePosition.x = j["ResultSpritePosition"][0].get<float>();
		spritePosition.y = j["ResultSpritePosition"][1].get<float>();
	}
	if (j.contains("ResultSpriteSize"))
	{
		spriteSize.x = j["ResultSpriteSize"][0].get<float>();
		spriteSize.y = j["ResultSpriteSize"][1].get<float>();
	}
	if (j.contains("ResultSpriteColor"))
	{
		spriteColor.x = j["ResultSpriteColor"][0].get<float>();
		spriteColor.y = j["ResultSpriteColor"][1].get<float>();
		spriteColor.z = j["ResultSpriteColor"][2].get<float>();
		spriteColor.w = j["ResultSpriteColor"][3].get<float>();
	}

	buttonManager.LoadFromJson(j);

}