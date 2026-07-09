#include "TrackingData.h"
#include "shader.h"
#include <imgui.h>
#include <Ball.h>

void TrackingData::Initialize(ID3D11Device* device)
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
	
	trackingDataSpriteData = std::make_unique<Sprite>();
	trackingDataSpriteData->texturePath = L".\\resources\\textures\\TrackingDataBoard.png";
	trackingDataSpriteData->position = { 10.0f, 10.0f };
	trackingDataSpriteData->size = { 300.0f, 100.0f };
	trackingDataSpriteData->rotation = 0.0f;
	trackingDataSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	trackingDataSprite = std::make_unique<sprite>(device, trackingDataSpriteData->texturePath.c_str());

	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789.km/h-"
		u8"角度速度方向"
		u8"0123456789.度"
		u8"Tracking Data"
	);

	// フォントレンダラーの初期化
	trackingDataFont.Initialize(device,
		L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&trackingDataCodepoints);
}

void TrackingData::Uninitialize()
{
	trackingDataFont.Uninitialize();
	trackingDataSprite.reset();
	trackingDataSpriteData.reset();
}

void TrackingData::Update(float elapsedTime)
{
	if (!Ball::Instance().GetHasCollidedWithBat())
	{
		return; // まだヒットしていないのでタイマーを進めない
	}

	showTrackingDelay += elapsedTime;

	if (showTrackingDelay > 1.0f)
	{
		showTrackingDelay = 1.0f;
		showTrackingData = true;
	}
}

void TrackingData::Reset()
{
	showTrackingData = false;
	showTrackingDelay = 0.0f;
	Physics::Instance().SetIsHomeRun(false);
}

void TrackingData::Render()
{
	if (!showTrackingData || showTrackingDelay < 1.0f) return;
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if (trackingDataSprite && trackingDataSpriteData)
	{
		trackingDataSprite->render(dc,
			trackingDataSpriteData->position.x,
			trackingDataSpriteData->position.y,
			trackingDataSpriteData->size.x,
			trackingDataSpriteData->size.y,
			trackingDataSpriteData->color.x,
			trackingDataSpriteData->color.y,
			trackingDataSpriteData->color.z,
			trackingDataSpriteData->color.w,
			trackingDataSpriteData->rotation);
	}

	const float baseX = trackingDataSpriteData->position.x + 10.0f;
	float lineY = trackingDataSpriteData->position.y + 10.0f;
	const float lineHeight = trackingDataValueFontScale * 50.0f;
	const float valuePadding = 8.0f;
	const float TrackingDataLabelOffsetY = 20.0f; // "Tracking Data"ラベルのYオフセット

	const float valueColumnCenterX = trackingDataSpriteData->position.x + trackingDataSpriteData->size.x * 0.7f;

	auto FormatRoundedValue = [](float value) -> float
		{
			float rounded = std::round(value);
			if (rounded == 0.0f)
			{
				return 0.0f; // -0.xの場合に-0度と表示されるのを防ぐ
			}
			return rounded;
		};

	char angleLabel[16];     snprintf(angleLabel, sizeof(angleLabel), u8"角度　");
	char angleValue[32];     snprintf(angleValue, sizeof(angleValue), u8"%.f度", FormatRoundedValue(Physics::Instance().GetBallAngle()));

	char speedLabel[16];     snprintf(speedLabel, sizeof(speedLabel), u8"速度　");
	char speedValue[32];     snprintf(speedValue, sizeof(speedValue), u8"%.fkm/h", FormatRoundedValue(Physics::Instance().GetBallSpeed()));

	//画像の左上付近にTrackingDataのラベルを表示する
	char trackingDataLabel[32]; snprintf(trackingDataLabel, sizeof(trackingDataLabel), "Tracking Data");

	
	/*char directionLabel[16]; snprintf(directionLabel, sizeof(directionLabel), u8"方向　");
	char directionValue[32]; snprintf(directionValue, sizeof(directionValue), u8"%.f度", FormatRoundedValue(Physics::Instance().GetBallDirection()));*/

	// ラベル＋数値をペアで描画するヘルパー（オフセット付き）
	auto DrawLabelAndValue = [&](const char* label, const char* value, float y,
		const DirectX::XMFLOAT2& labelOffset, const DirectX::XMFLOAT2& valueOffset)
		{
			//打球速度が150キロ以上かつ打球角度が25度から35度の範囲内の場合、両者を金色で表示
			if(Physics::Instance().GetIsHomeRun())
			{
				trackingDataFont.DrawTextW(dc, label,
					baseX + labelOffset.x, y + labelOffset.y,
					trackingDataFontScale, 1.0f, 1.0f, 1.0f, 1.0f); // 白色
				// 数値は幅を測って中央ぞろえ
				float valueWidth = 0.0f, valueHeight = 0.0f;
				trackingDataFont.MeasureText(value, trackingDataValueFontScale, valueWidth, valueHeight);
				float valueX = valueColumnCenterX - valueWidth * 0.5f;
				trackingDataFont.DrawTextW(dc, value,
					valueX + valueOffset.x, y + valueOffset.y,
					trackingDataValueFontScale, 1.0f, 0.843f, 0.0f, 1.0f); // 金色
			}
			else
			{
				// ラベルはこれまで通り左揃え
				trackingDataFont.DrawTextW(dc, label,
					baseX + labelOffset.x, y + labelOffset.y,
					trackingDataFontScale, 1.0f, 1.0f, 1.0f, 1.0f);
				// 数値は幅を測って中央ぞろえ
				float valueWidth = 0.0f, valueHeight = 0.0f;
				trackingDataFont.MeasureText(value, trackingDataValueFontScale, valueWidth, valueHeight);
				float valueX = valueColumnCenterX - valueWidth * 0.5f;
				trackingDataFont.DrawTextW(dc, value,
					valueX + valueOffset.x, y + valueOffset.y,
					trackingDataValueFontScale, 1.0f, 1.0f, 1.0f, 1.0f);
			}			
		};

	DrawLabelAndValue(trackingDataLabel, "", lineY + TrackingDataLabelOffsetY, { 0.0f, 0.0f }, { 0.0f, 0.0f });
	DrawLabelAndValue(angleLabel, angleValue, lineY, angleLabelOffset, angleValueOffset);
	DrawLabelAndValue(speedLabel, speedValue, lineY + lineHeight, speedLabelOffset, speedValueOffset);
	//DrawLabelAndValue(directionLabel, directionValue, lineY + 2 * lineHeight, directionLabelOffset, directionValueOffset);

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void TrackingData::DrawGUI()
{
	if (ImGui::CollapsingHeader("Tracking Data"))
	{
		ImGui::Checkbox("Show Tracking Data", &showTrackingData);
		ImGui::DragFloat2("Position", &trackingDataSpriteData->position.x, 1.0f, 0.0f, Graphics::Instance().GetScreenWidth());
		ImGui::DragFloat2("Size", &trackingDataSpriteData->size.x, 1.0f, 0.0f, Graphics::Instance().GetScreenWidth());
		ImGui::ColorEdit4("Color", &trackingDataSpriteData->color.x);
		ImGui::SliderFloat("Label Font Scale", &trackingDataFontScale, 0.5f, 3.0f);
		ImGui::SliderFloat("Value Font Scale", &trackingDataValueFontScale, 0.5f, 4.0f);

		if (ImGui::TreeNode("Text Offsets"))
		{
			ImGui::Text(u8"角度");
			ImGui::DragFloat2("Angle Label Offset", &angleLabelOffset.x, 0.5f, -200.0f, 200.0f);
			ImGui::DragFloat2("Angle Value Offset", &angleValueOffset.x, 0.5f, -200.0f, 200.0f);

			ImGui::Text(u8"速度");
			ImGui::DragFloat2("Speed Label Offset", &speedLabelOffset.x, 0.5f, -200.0f, 200.0f);
			ImGui::DragFloat2("Speed Value Offset", &speedValueOffset.x, 0.5f, -200.0f, 200.0f);

			/*ImGui::Text(u8"方向");
			ImGui::DragFloat2("Direction Label Offset", &directionLabelOffset.x, 0.5f, -200.0f, 200.0f);
			ImGui::DragFloat2("Direction Value Offset", &directionValueOffset.x, 0.5f, -200.0f, 200.0f);*/

			ImGui::TreePop();
		}
	}
}

void TrackingData::SaveToJson(json& j)
{
	j["trackingData"]["showTrackingData"] = showTrackingData;
	j["trackingData"]["position"] = { trackingDataSpriteData->position.x, trackingDataSpriteData->position.y };
	j["trackingData"]["size"] = { trackingDataSpriteData->size.x, trackingDataSpriteData->size.y };
	j["trackingData"]["color"] = { trackingDataSpriteData->color.x, trackingDataSpriteData->color.y, trackingDataSpriteData->color.z, trackingDataSpriteData->color.w };
	j["trackingData"]["fontScale"] = trackingDataFontScale;
	j["trackingData"]["valueFontScale"] = trackingDataValueFontScale;

	j["trackingData"]["angleLabelOffset"] = { angleLabelOffset.x, angleLabelOffset.y };
	j["trackingData"]["angleValueOffset"] = { angleValueOffset.x, angleValueOffset.y };
	j["trackingData"]["speedLabelOffset"] = { speedLabelOffset.x, speedLabelOffset.y };
	j["trackingData"]["speedValueOffset"] = { speedValueOffset.x, speedValueOffset.y };
	j["trackingData"]["directionLabelOffset"] = { directionLabelOffset.x, directionLabelOffset.y };
	j["trackingData"]["directionValueOffset"] = { directionValueOffset.x, directionValueOffset.y };
}

void TrackingData::LoadFromJson(const json& j)
{
	if (j.contains("trackingData"))
	{
		const auto& t = j["trackingData"];
		showTrackingData = t.value("showTrackingData", true);

		if (t.contains("position") && t["position"].is_array() && t["position"].size() == 2)
		{
			trackingDataSpriteData->position.x = t["position"][0].get<float>();
			trackingDataSpriteData->position.y = t["position"][1].get<float>();
		}
		if (t.contains("size") && t["size"].is_array() && t["size"].size() == 2)
		{
			trackingDataSpriteData->size.x = t["size"][0].get<float>();
			trackingDataSpriteData->size.y = t["size"][1].get<float>();
		}
		if (t.contains("color") && t["color"].is_array() && t["color"].size() == 4)
		{
			trackingDataSpriteData->color.x = t["color"][0].get<float>();
			trackingDataSpriteData->color.y = t["color"][1].get<float>();
			trackingDataSpriteData->color.z = t["color"][2].get<float>();
			trackingDataSpriteData->color.w = t["color"][3].get<float>();
		}
		trackingDataFontScale = t.value("fontScale", 1.0f);
		trackingDataValueFontScale = t.value("valueFontScale", 1.5f);

		auto loadOffset = [&](const char* key, DirectX::XMFLOAT2& offset)
			{
				if (t.contains(key) && t[key].is_array() && t[key].size() == 2)
				{
					offset.x = t[key][0].get<float>();
					offset.y = t[key][1].get<float>();
				}
			};
		loadOffset("angleLabelOffset", angleLabelOffset);
		loadOffset("angleValueOffset", angleValueOffset);
		loadOffset("speedLabelOffset", speedLabelOffset);
		loadOffset("speedValueOffset", speedValueOffset);
		loadOffset("directionLabelOffset", directionLabelOffset);
		loadOffset("directionValueOffset", directionValueOffset);
	}
}