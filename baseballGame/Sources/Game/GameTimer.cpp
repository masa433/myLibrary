#include "GameTimer.h"
#include "Graphics.h"
#include <imgui.h>

void GameTimer::Initialize(ID3D11Device* device)
{
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, "sprite_vs.cso", spriteVS.GetAddressOf(), spriteInputLayout.GetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, "sprite_ps.cso", spritePS.GetAddressOf());

	// スプライトの初期化
	timerSpriteData = std::make_unique<Sprite>();
	timerSpriteData->texturePath = L".\\resources\\textures\\timerBoard.png";
	timerSpriteData->position = { 10.0f, 10.0f };
	timerSpriteData->size = { 100.0f, 50.0f };
	timerSpriteData->rotation = 0.0f;
	timerSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	timerSprite = std::make_unique<sprite>(device, timerSpriteData->texturePath.c_str());

	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> timerCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789:."
	);
	timerFont.Initialize(device,
		L".\\resources\\fonts\\DSEG7Classic-Bold.ttf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&timerCodepoints);
}

void GameTimer::Uninitialize()
{
	timerFont.Uninitialize();
	timerSprite.reset();
	timerSpriteData.reset();
}

void GameTimer::Update(float elapsedTime)
{
	// カウントダウンの更新処理
	if (startCountdown > 0)
	{
		startCountdown -= static_cast<int>(elapsedTime);
		if (startCountdown < 0)
		{
			startCountdown = 0;
		}
		return; // カウントダウン中はタイマーの更新を行わない
	}

	// タイマーの更新処理
	remainingTime -= elapsedTime;
	if (remainingTime <= 0.0f)
	{
		remainingTime = 0.0f;
	}
}

void GameTimer::Render()
{
	// タイマーの描画処理
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// スプライトの描画(中心点)
	if (timerSprite && timerSpriteData)
	{
		timerSprite->render(dc,
			timerSpriteData->position.x - timerSpriteData->size.x / 2.0f, timerSpriteData->position.y - timerSpriteData->size.y / 2.0f,
			timerSpriteData->size.x, timerSpriteData->size.y,
			timerSpriteData->color.x, timerSpriteData->color.y,
			timerSpriteData->color.z, timerSpriteData->color.w,
			timerSpriteData->rotation);
	}

	//中央ぞろえにするヘルパー関数
	auto centerTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
	{
		float textWidth = 0.0f;
		float textHeight = 0.0f;
		timerFont.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
		return { x - textWidth / 2.0f, y - textHeight / 2.0f };
	};


	// フォントの描画
	DirectX::XMFLOAT2 fontPos = centerTextPosition("00:00", fontSize,
		fontPosition.x, fontPosition.y
	);


	// フォントの描画
	int minutes = static_cast<int>(remainingTime) / 60;
	int seconds = static_cast<int>(remainingTime) % 60;
	char buffer[6];
	sprintf_s(buffer, "%02d:%02d", minutes, seconds);

	//残り時間が30秒を切ったら黄色くする、10秒を切ったら赤くする
	if (remainingTime <= 30.0f && remainingTime > 10.0f)
	{
		fontColor = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
	}
	else if (remainingTime <= 10.0f)
	{
		fontColor = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤色
	}
	else
	{
		fontColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
	}

	timerFont.DrawTextW(dc, buffer, fontPosition.x, fontPosition.y, fontSize, fontColor.x, fontColor.y, fontColor.z, fontColor.w);

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void GameTimer::DrawGUI()
{
	if(ImGui::CollapsingHeader("Game Timer"))
	{
		//スプライトの位置、サイズ、色をGUIで編集

		if(ImGui::TreeNode("Sprite"))
		{
			ImGui::DragFloat2("Position", &timerSpriteData->position.x);
			ImGui::DragFloat2("Size", &timerSpriteData->size.x);
			ImGui::ColorEdit4("Color", &timerSpriteData->color.x);
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Font"))
		{
			ImGui::DragFloat2("Position", &fontPosition.x);
			ImGui::DragFloat("Font Size", &fontSize);
			ImGui::ColorEdit4("Font Color", &fontColor.x);
			ImGui::TreePop();
		}

		ImGui::DragInt("Start Countdown", &startCountdown, 1.0f, 0, 60);
	}
}

void GameTimer::SaveToJson(nlohmann::json& j)
{
	j["gameTimer"]["position"] = { timerSpriteData->position.x, timerSpriteData->position.y };
	j["gameTimer"]["size"] = { timerSpriteData->size.x, timerSpriteData->size.y };
	j["gameTimer"]["color"] = { timerSpriteData->color.x, timerSpriteData->color.y, timerSpriteData->color.z, timerSpriteData->color.w };
	j["gameTimer"]["fontSize"] = fontSize;
	j["gameTimer"]["fontColor"] = { fontColor.x, fontColor.y, fontColor.z, fontColor.w };
	j["gameTimer"]["fontPosition"] = { fontPosition.x, fontPosition.y };

}

void GameTimer::LoadFromJson(const nlohmann::json& j)
{
	if(j.contains("gameTimer"))
	{
		const auto& timerData = j["gameTimer"];
		if(timerData.contains("position"))
		{
			timerSpriteData->position.x = timerData["position"][0];
			timerSpriteData->position.y = timerData["position"][1];
		}
		if(timerData.contains("size"))
		{
			timerSpriteData->size.x = timerData["size"][0];
			timerSpriteData->size.y = timerData["size"][1];
		}
		if(timerData.contains("color"))
		{
			timerSpriteData->color.x = timerData["color"][0];
			timerSpriteData->color.y = timerData["color"][1];
			timerSpriteData->color.z = timerData["color"][2];
			timerSpriteData->color.w = timerData["color"][3];
		}
		if(timerData.contains("fontSize"))
		{
			fontSize = timerData["fontSize"];
		}
		if(timerData.contains("fontColor"))
		{
			fontColor.x = timerData["fontColor"][0];
			fontColor.y = timerData["fontColor"][1];
			fontColor.z = timerData["fontColor"][2];
			fontColor.w = timerData["fontColor"][3];
		}
		if(timerData.contains("fontPosition"))
		{
			fontPosition.x = timerData["fontPosition"][0];
			fontPosition.y = timerData["fontPosition"][1];
		}
	}
}