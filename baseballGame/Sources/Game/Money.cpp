#include "Money.h"
#include "Graphics.h"
#include "shader.h"
#include <imgui.h>
#include "ballDistance.h"
#include "Ball.h"

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

		// ホームランボーナスを適用
		if (isHomeRun)
		{
			totalMultiplier *= homerunBonus;
			/*OutputDebugStringA("ホームランボーナスが適用されました。\n");*/
			if(consoleLog)
			{
				consoleLog->push_back(u8"[Info]ホームランボーナスが適用されました。");
				consoleLog->push_back(u8"[Info]現在のホームランボーナス倍率: " + std::to_string(homerunBonus));
			}

			// ボールゾーンボーナスを適用
			totalMultiplier *= currentBallZoneBonus;
			if (consoleLog)
			{
				consoleLog->push_back(u8"[Info]ボールゾーンボーナスが適用されました。");
				consoleLog->push_back(u8"[Info]現在のボールゾーンボーナス倍率: " + std::to_string(currentBallZoneBonus));
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
				consoleLog->push_back(u8"[Info]現在の変化球ボーナス倍率: " + std::to_string(breakingBonus));
			}
		}

		

		// 最終的な距離に倍率を適用して加算
		int finalDistance = static_cast<int>(std::round(baseDistance * totalMultiplier));
		AddMoney(finalDistance);
		
		if (isHomeRun)
		{
			ResetBallZoneBonus();
			if (consoleLog)
			{
				consoleLog->push_back(u8"[Info] ボールゾーンボーナスがリセットされました。by Money");
			}
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

		////10万以上になったら、サイズを1.7倍にする
		//if (currentMoney >= 100000)
		//{
		//	moneyTextScale = 2.0f;
		//}
		//else
		//{
		//	moneyTextScale = 2.2f;
		//}

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
}