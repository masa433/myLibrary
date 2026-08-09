#include "HomeRunCount.h"
#include "Graphics.h"
#include "imgui.h"

void HomeRunCount::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

	// シェーダーの読み込み
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
	homeRunCountSpriteData = std::make_unique<Sprite>();
	homeRunCountSpriteData->texturePath = L".\\resources\\textures\\MissionBoard.png";
	homeRunCountSpriteData->position = { 10.0f, 10.0f };
	homeRunCountSpriteData->size = { 200.0f, 50.0f };
	homeRunCountSpriteData->rotation = 0.0f;
	homeRunCountSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	homeRunCountSprite = std::make_unique<sprite>(device, context, homeRunCountSpriteData->texturePath.c_str());
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	// ホームラン数表示に必要な文字だけをベイクする
	std::vector<int> homeRunCountCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789HOMERUN");
	// フォントレンダラーの初期化
	homeRunCountFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&homeRunCountCodepoints);

	ResetCount(); // ホームラン数を初期化
}

void HomeRunCount::Uninitialize()
{
	homeRunCountFont.Uninitialize();
	homeRunCountSprite.reset();
	homeRunCountSpriteData.reset();
}

void HomeRunCount::Update(float elapsedTime)
{
	// ホームラン数が増えたら、表示を一旦大きくしてからアニメーションで戻す
	if (homeRunCount != previousHomeRunCount)
	{
		previousHomeRunCount = homeRunCount;
		numberDisplayScale = numberScale * numberPopScaleMultiplier; // 大きい状態から開始
	}

	// 現在のスケールを基準サイズへ滑らかに近づける
	if (numberDisplayScale > numberScale)
	{
		numberDisplayScale -= numberScaleAnimSpeed * elapsedTime;
		if (numberDisplayScale < numberScale)
		{
			numberDisplayScale = numberScale;
		}
	}
}

void HomeRunCount::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// ホームラン数の描画
	homeRunCountSprite->render(Graphics::Instance().GetDeviceContext(),
		homeRunCountSpriteData->position.x, homeRunCountSpriteData->position.y,
		homeRunCountSpriteData->size.x, homeRunCountSpriteData->size.y,
		homeRunCountSpriteData->color.x, homeRunCountSpriteData->color.y,
		homeRunCountSpriteData->color.z, homeRunCountSpriteData->color.w,
		homeRunCountSpriteData->rotation);
	
	// "HOMERUN" ラベルをそのまま描画
	//homeRunCountFont.DrawTextW(dc,
	//	"HOMERUN",
	//	labelPositionX,
	//	labelPositionY,
	//	labelScale,
	//	1.0f, 1.0f, 1.0f, 1.0f); // 白色

	// 数字だけ大きく、ラベルの下に描画
	char numberBuffer[16];
	sprintf_s(numberBuffer, sizeof(numberBuffer), "%d", homeRunCount);

	// 中央寄せしたい場合は幅を測ってから位置を調整
	float numberWidth = 0.0f, numberHeight = 0.0f;
	homeRunCountFont.MeasureText(numberBuffer, numberDisplayScale, numberWidth, numberHeight);
	float numberX = homeRunCountSpriteData->position.x
		+ homeRunCountSpriteData->size.x * 0.5f - numberWidth * 0.5f;

	homeRunCountFont.DrawTextW(dc,
		numberBuffer,
		numberX,
		numberPositionY,
		numberDisplayScale,
		numberColor.x, numberColor.y, numberColor.z, numberColor.w); // 金色にして目立たせる例

	// 後始末（Wind と同じ）
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void HomeRunCount::DrawGUI()
{
	if (ImGui::CollapsingHeader("Sprite"))
	{
		ImGui::DragFloat2("Position", &homeRunCountSpriteData->position.x);
		ImGui::DragFloat2("Size", &homeRunCountSpriteData->size.x);
		ImGui::DragFloat("Rotation", &homeRunCountSpriteData->rotation);
		ImGui::ColorEdit4("Color", &homeRunCountSpriteData->color.x);
	}
	if(ImGui::CollapsingHeader("Font"))
	{
		ImGui::DragFloat2("Label Position", &labelPositionX);
		ImGui::DragFloat("Label Scale", &labelScale);
		ImGui::DragFloat2("Number Position", &numberPositionX);
		ImGui::DragFloat("Number Scale", &numberScale);
		ImGui::ColorEdit4("Number Color", &numberColor.x);

		ImGui::Separator();
		ImGui::DragFloat("Pop Scale Multiplier", &numberPopScaleMultiplier, 0.05f, 1.0f, 5.0f);
		ImGui::DragFloat("Pop Anim Speed", &numberScaleAnimSpeed, 0.1f, 0.5f, 20.0f);
	}
}

void HomeRunCount::SaveToJson(nlohmann::json& j)
{
	j["homeRunCount"] = homeRunCount;
	j["sprite"] = {
		{"texturePath", homeRunCountSpriteData->texturePath},
		{"position", {homeRunCountSpriteData->position.x, homeRunCountSpriteData->position.y}},
		{"size", {homeRunCountSpriteData->size.x, homeRunCountSpriteData->size.y}},
		{"rotation", homeRunCountSpriteData->rotation},
		{"color", {homeRunCountSpriteData->color.x, homeRunCountSpriteData->color.y, homeRunCountSpriteData->color.z, homeRunCountSpriteData->color.w}}
	};
	j["font"] = {
		{"labelPosition", {labelPositionX, labelPositionY}},
		{"labelScale", labelScale},
		{"numberPosition", {numberPositionX, numberPositionY}},
		{"numberScale", numberScale},
		{"numberColor", {numberColor.x, numberColor.y, numberColor.z, numberColor.w}}
	};
}

void HomeRunCount::LoadFromJson(const nlohmann::json& j)
{
	if (j.contains("homeRunCount"))
	{
		homeRunCount = j["homeRunCount"].get<int>();
	}
	if (j.contains("sprite"))
	{
		const auto& spriteJson = j["sprite"];
		if (spriteJson.contains("texturePath"))
			homeRunCountSpriteData->texturePath = spriteJson["texturePath"].get<std::wstring>();
		if (spriteJson.contains("position"))
		{
			const auto& pos = spriteJson["position"];
			if (pos.is_array() && pos.size() == 2)
			{
				homeRunCountSpriteData->position.x = pos[0].get<float>();
				homeRunCountSpriteData->position.y = pos[1].get<float>();
			}
		}
		if (spriteJson.contains("size"))
		{
			const auto& size = spriteJson["size"];
			if (size.is_array() && size.size() == 2)
			{
				homeRunCountSpriteData->size.x = size[0].get<float>();
				homeRunCountSpriteData->size.y = size[1].get<float>();
			}
		}
		if (spriteJson.contains("rotation"))
			homeRunCountSpriteData->rotation = spriteJson["rotation"].get<float>();
		if (spriteJson.contains("color"))
		{
			const auto& color = spriteJson["color"];
			if (color.is_array() && color.size() == 4)
			{
				homeRunCountSpriteData->color.x = color[0].get<float>();
				homeRunCountSpriteData->color.y = color[1].get<float>();
				homeRunCountSpriteData->color.z = color[2].get<float>();
				homeRunCountSpriteData->color.w = color[3].get<float>();
			}
		}
	}
	if(j.contains("font"))
	{
		const auto& fontJson = j["font"];
		if (fontJson.contains("labelPosition"))
		{
			const auto& pos = fontJson["labelPosition"];
			if (pos.is_array() && pos.size() == 2)
			{
				labelPositionX = pos[0].get<float>();
				labelPositionY = pos[1].get<float>();
			}
		}
		if (fontJson.contains("labelScale"))
			labelScale = fontJson["labelScale"].get<float>();
		if (fontJson.contains("numberPosition"))
		{
			const auto& pos = fontJson["numberPosition"];
			if (pos.is_array() && pos.size() == 2)
			{
				numberPositionX = pos[0].get<float>();
				numberPositionY = pos[1].get<float>();
			}
		}
		if (fontJson.contains("numberScale"))
			numberScale = fontJson["numberScale"].get<float>();
		if (fontJson.contains("numberColor"))
		{
			const auto& color = fontJson["numberColor"];
			if (color.is_array() && color.size() == 4)
			{
				numberColor.x = color[0].get<float>();
				numberColor.y = color[1].get<float>();
				numberColor.z = color[2].get<float>();
				numberColor.w = color[3].get<float>();
			}
		}
	}

	ResetCount();
}