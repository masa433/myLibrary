#include "RoundManager.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include "ballCount.h"
#include "Pitcher.h"
#include "HomeRunCount.h"

void RoundManager::Initialize(ID3D11Device* device)
{
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789/");

	// フォントレンダラーの初期化
	roundFont.Initialize(device,
		L".\\resources\\fonts\\GarpSansNormalItalic.otf",
		50.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	isGameClear = false;
	isGameOver = false;

	currentRound = 1; // 初期ラウンドを設定
}

void RoundManager::Uninitialize()
{
	roundFont.Uninitialize();
}

void RoundManager::Update(float elapsedTime)
{
	bool isPitchFinished = (ballCount::Instance().GetRemainingBalls() <= 0 &&
		Pitcher::Instance().GetCurrentState() == Pitcher::State::SelectingPitch);

	if (isPitchFinished)
	{
		bool targetReached = HomeRunCount::Instance().IsHomeRunCountExceeded(GetCurrentTarget());
		if (targetReached)
		{
			if (IsFinalRound())
			{
				isGameClear = true;
			}
			else
			{
				IncreaseRound();
				ballCount::Instance().ResetRemainingBalls();
				HomeRunCount::Instance().ResetCount();
			}
		}
		else
		{
			isGameOver = true;
		}
	}

	if (currentRound <= 2)
	{
		Pitcher::Instance().SetBallSpeedMode(Pitcher::BallSpeedMode::slowSpeed);
	}
	else if(currentRound <= 4)
	{
		Pitcher::Instance().SetBallSpeedMode(Pitcher::BallSpeedMode::highSpeed);
	}
	else
	{
		Pitcher::Instance().SetBallSpeedMode(Pitcher::BallSpeedMode::realSpeed);
	}
}

void RoundManager::Render()
{
	// ラウンド表示の描画
	std::string roundText = std::to_string(currentRound) + " / " + std::to_string(totalRounds);
	roundFont.DrawTextW(Graphics::Instance().GetDeviceContext(), roundText.c_str(),
		roundTextPosition.x, roundTextPosition.y,
		roundTextScale,
		roundTextColor.x, roundTextColor.y, roundTextColor.z, roundTextColor.w);
}

void RoundManager::DrawGUI()
{
	if (ImGui::CollapsingHeader("Round Font"))
	{
		ImGui::Text("Current Round: %d / %d", currentRound, totalRounds);
		ImGui::DragFloat2("Round Text Position", &roundTextPosition.x, 1.0f, 0.0f, 1920.0f);
		ImGui::DragFloat("Round Text Scale", &roundTextScale, 0.1f, 5.0f);
		ImGui::ColorEdit4("Round Text Color", &roundTextColor.x);
	}
}

void RoundManager::SaveToJson(json& j)
{
	
	j["totalRounds"] = totalRounds;
	j["roundTextPosition"] = { roundTextPosition.x, roundTextPosition.y };
	j["roundTextScale"] = roundTextScale;
	j["roundTextColor"] = { roundTextColor.x, roundTextColor.y, roundTextColor.z, roundTextColor.w };
	j["targetHomeRuns"] = targetHomeRuns; 
}

void RoundManager::LoadFromJson(const json& j)
{
	if (j.contains("totalRounds")) totalRounds = j["totalRounds"].get<int>();
	if (j.contains("roundTextPosition"))
	{
		auto pos = j["roundTextPosition"];
		if (pos.is_array() && pos.size() == 2)
		{
			roundTextPosition.x = pos[0].get<float>();
			roundTextPosition.y = pos[1].get<float>();
		}
	}
	if (j.contains("roundTextScale")) roundTextScale = j["roundTextScale"].get<float>();
	if (j.contains("roundTextColor"))
	{
		auto color = j["roundTextColor"];
		if (color.is_array() && color.size() == 4)
		{
			roundTextColor.x = color[0].get<float>();
			roundTextColor.y = color[1].get<float>();
			roundTextColor.z = color[2].get<float>();
			roundTextColor.w = color[3].get<float>();
		}
	}
	if(j.contains("targetHomeRuns"))
	{
		auto targets = j["targetHomeRuns"];
		if (targets.is_array())
		{
			targetHomeRuns.clear();
			for (const auto& target : targets)
			{
				targetHomeRuns.push_back(target.get<int>());
			}
		}
	}
}