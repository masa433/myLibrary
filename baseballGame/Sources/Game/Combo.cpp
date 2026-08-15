#include "Combo.h"
#include "Graphics.h"
#include "imgui.h"

void Combo::Initialize(ID3D11Device* device)
{
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	// コンボ数表示に必要な文字だけをベイクする
	std::vector<int> comboCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789Combo!"
	);
	// 日本語グリフを持つフォントを用意して配置する
	numberFont.Initialize(device,
		L".\\resources\\fonts\\Futur12.ttf",
		150.0f,
		screenWidth, screenHeight,
		512, 512,
		&comboCodepoints);
	labelFont.Initialize(device,
		L".\\resources\\fonts\\Futur12.ttf",
		150.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&comboCodepoints);

	currentCombo = 0;
	maxCombo = 0;
}

void Combo::Uninitialize()
{
	numberFont.Uninitialize();
	labelFont.Uninitialize();
	consoleLog = nullptr;
}

void Combo::AddCombo(int amount)
{
	currentCombo += amount;
	if (currentCombo > maxCombo)
	{
		maxCombo = currentCombo;
	}
}

void Combo::Update(float elapsedTime)
{
	// コンボ数の更新処理はここに追加できます
}

void Combo::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	// コンボ数を描画
	std::string comboText = std::to_string(currentCombo);
	numberFont.DrawTextW(dc, comboText.c_str(), numberFontPosition.x, numberFontPosition.y, numberFontSize,
		numberFontColor.x, numberFontColor.y, numberFontColor.z, numberFontColor.w);
	// "Combo!" ラベルを描画
	labelFont.DrawTextW(dc, "Combo!", labelFontPosition.x, labelFontPosition.y, labelFontSize,
		labelFontColor.x, labelFontColor.y, labelFontColor.z, labelFontColor.w);
}

void Combo::DrawGUI()
{
	if(ImGui::CollapsingHeader("Combo Settings"))
	{
		ImGui::SliderFloat2("Number Font Position", &numberFontPosition.x, 0.0f, 1920.0f);
		ImGui::SliderFloat("Number Font Size", &numberFontSize, 10.0f, 100.0f);
		ImGui::ColorEdit4("Number Font Color", &numberFontColor.x);
		ImGui::Separator();
		ImGui::SliderFloat2("Label Font Position", &labelFontPosition.x, 0.0f, 1920.0f);
		ImGui::SliderFloat("Label Font Size", &labelFontSize, 10.0f, 100.0f);
		ImGui::ColorEdit4("Label Font Color", &labelFontColor.x);
	}
}

void Combo::SaveToJson(json& j)
{
	
	j["numberFontPosition"] = { numberFontPosition.x, numberFontPosition.y };
	j["numberFontSize"] = numberFontSize;
	j["numberFontColor"] = { numberFontColor.x, numberFontColor.y, numberFontColor.z, numberFontColor.w };
	j["labelFontPosition"] = { labelFontPosition.x, labelFontPosition.y };
	j["labelFontSize"] = labelFontSize;
	j["labelFontColor"] = { labelFontColor.x, labelFontColor.y, labelFontColor.z, labelFontColor.w };
}

void Combo::LoadFromJson(const json& j)
{
	if (j.contains("numberFontPosition") && j["numberFontPosition"].is_array() && j["numberFontPosition"].size() == 2)
	{
		numberFontPosition.x = j["numberFontPosition"][0].get<float>();
		numberFontPosition.y = j["numberFontPosition"][1].get<float>();
	}
	if (j.contains("numberFontSize") && j["numberFontSize"].is_number())
	{
		numberFontSize = j["numberFontSize"].get<float>();
	}
	if (j.contains("numberFontColor") && j["numberFontColor"].is_array() && j["numberFontColor"].size() == 4)
	{
		numberFontColor.x = j["numberFontColor"][0].get<float>();
		numberFontColor.y = j["numberFontColor"][1].get<float>();
		numberFontColor.z = j["numberFontColor"][2].get<float>();
		numberFontColor.w = j["numberFontColor"][3].get<float>();
	}
	if (j.contains("labelFontPosition") && j["labelFontPosition"].is_array() && j["labelFontPosition"].size() == 2)
	{
		labelFontPosition.x = j["labelFontPosition"][0].get<float>();
		labelFontPosition.y = j["labelFontPosition"][1].get<float>();
	}
	if (j.contains("labelFontSize") && j["labelFontSize"].is_number())
	{
		labelFontSize = j["labelFontSize"].get<float>();
	}
	if (j.contains("labelFontColor") && j["labelFontColor"].is_array() && j["labelFontColor"].size() == 4)
	{
		labelFontColor.x = j["labelFontColor"][0].get<float>();
		labelFontColor.y = j["labelFontColor"][1].get<float>();
		labelFontColor.z = j["labelFontColor"][2].get<float>();
		labelFontColor.w = j["labelFontColor"][3].get<float>();
	}
}