#include "ballDistance.h"
#include "Graphics.h"
#include "Ball.h"
#include "physxManager.h"
#include <imgui.h>

void BallDistance::Initialize(ID3D11Device* device)
{
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	// 球種名と球速表示に必要な文字だけをベイクする
	std::vector<int> pitchInfoCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789m"
	);

	// 日本語グリフを持つフォントを用意して配置する
	ballDistanceFont.Initialize(device,
		L".\\resources\\fonts\\Futur12.ttf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&pitchInfoCodepoints);

}

void BallDistance::Uninitialize()
{
	ballDistanceFont.Uninitialize();
}

void BallDistance::Update(float elapsedTime)
{
	if(!Ball::Instance().GetHasCollidedWithBat())
	{
		hasDistanceText = false;
		isDistanceLocked = false;
		return;
	}

	// ボールがバットに当たった後、地面またはフェンスに当たるまでの間、距離を表示する
	bool isFinished = Ball::Instance().GetHasCollidedWithFence() || Ball::Instance().GetHasCollidedWithGround();

	// ボールが地面またはフェンスに当たったら、距離をロックする
	if (isDistanceLocked) return;

	if (!isFinished)
	{
		DirectX::XMFLOAT3 ballPosition = Ball::Instance().GetWorldPosition();

		// 高さ(Y)を含めない水平距離
		currentDistance = sqrtf(
			ballPosition.x * ballPosition.x +
			ballPosition.z * ballPosition.z);
		
		//ボールが地面につくかフェンスに当たったら、その位置の距離を表示する

		snprintf(distanceText, sizeof(distanceText), "%.fm", currentDistance);
		hasDistanceText = true;
	}
	else 
	{
		currentDistance = Physics::Instance().GetLastDistanceWasTotal()
			? Physics::Instance().GetBallTotalDistance()
			: Physics::Instance().GetBallHorizontalDistance();

		snprintf(distanceText, sizeof(distanceText), "%.fm", currentDistance);
		hasDistanceText = true;
		isDistanceLocked = true; // 以後は加算・更新しない
	}
}

void BallDistance::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	
	// ボールがバットに当たったら、距離を表示する(ボールの位置でリアルタイムに更新する)
	
	if (!hasDistanceText) return;

	//中央ぞろえにするヘルパー関数
	auto centerTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
	{
		float textWidth = 0.0f;
		float textHeight = 0.0f;
		ballDistanceFont.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
		return { x - textWidth / 2.0f, y - textHeight / 2.0f };
	};

	DirectX::XMFLOAT2 fontPos = centerTextPosition(distanceText, fontSize, fontPosition.x, fontPosition.y);

	ballDistanceFont.DrawTextW(dc, distanceText, fontPos.x, fontPos.y, fontSize,
		fontColor.x, fontColor.y, fontColor.z, fontColor.w);
}

void BallDistance::DrawGUI()
{
	// ImGuiを使ってフォントの位置、サイズ、色を調整するGUIを作る
	if (ImGui::CollapsingHeader("Ball Distance Settings"))
	{
		ImGui::Text("Font Position");
		ImGui::DragFloat2("Position", &fontPosition.x);
		ImGui::Text("Font Size");
		ImGui::SliderFloat("Size", &fontSize, 0.1f, 5.0f);
		ImGui::Text("Font Color");
		ImGui::ColorEdit4("Color", reinterpret_cast<float*>(&fontColor));
	}
}

void BallDistance::SaveToJson(nlohmann::json& j)
{
	j["fontPositionX"] = fontPosition.x;
	j["fontPositionY"] = fontPosition.y;
	j["fontSize"] = fontSize;
	j["fontColorR"] = fontColor.x;
	j["fontColorG"] = fontColor.y;
	j["fontColorB"] = fontColor.z;
	j["fontColorA"] = fontColor.w;
}

void BallDistance::LoadFromJson(const nlohmann::json& j)
{
	if (j.contains("fontPositionX") && j.contains("fontPositionY"))
	{
		fontPosition.x = j["fontPositionX"].get<float>();
		fontPosition.y = j["fontPositionY"].get<float>();
	}
	if (j.contains("fontSize"))
	{
		fontSize = j["fontSize"].get<float>();
	}
	if (j.contains("fontColorR") && j.contains("fontColorG") && j.contains("fontColorB") && j.contains("fontColorA"))
	{
		fontColor.x = j["fontColorR"].get<float>();
		fontColor.y = j["fontColorG"].get<float>();
		fontColor.z = j["fontColorB"].get<float>();
		fontColor.w = j["fontColorA"].get<float>();
	}
}