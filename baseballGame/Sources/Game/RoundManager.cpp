#include "RoundManager.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include "ballCount.h"
#include "Pitcher.h"
#include "HomeRunCount.h"
#include <Combo.h>
#include <algorithm>	
#include "input.h"
#include "SubMission.h"

void RoundManager::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789/");

	// フォントレンダラーの初期化
	roundFont.Initialize(device,
		L".\\resources\\fonts\\Futur12.ttf",
		50.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.ReleaseAndGetAddressOf(), input_layout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.ReleaseAndGetAddressOf());

	roundSpriteData = std::make_unique<RoundSpriteData>();
	roundSpriteData->texturePath = L".\\resources\\textures\\roundBoard.png";
	roundSpriteData->position = { spritePosition.x, spritePosition.y };
	roundSpriteData->size = { spriteSize.x, spriteSize.y };
	roundSpriteData->rotation = 0.0f;
	roundSpriteData->color = { spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w };
	roundSprite = std::make_unique<sprite>(device, context, roundSpriteData->texturePath.c_str());

	abilityBackSpriteData = std::make_unique<RoundSpriteData>();
	abilityBackSpriteData->texturePath = L".\\resources\\textures\\scrollViewBack.png";
	abilityBackSpriteData->position = { 0.0f, 0.0f }; // 適切な位置に配置
	abilityBackSpriteData->size = { 1920.0f, 1080.0f }; // 適切なサイズに設定
	abilityBackSpriteData->rotation = 0.0f;
	abilityBackSpriteData->color = { 1.0f, 1.0f, 1.0f, 0.5f };
	abilityBackSprite = std::make_unique<sprite>(device, context, abilityBackSpriteData->texturePath.c_str());

	isGameClear = false;
	isGameOver = false;

	currentRound = 1; // 初期ラウンドを設定
	totalRounds = 7; // 総ラウンド数を設定

	SpecialAbility::Instance().RollRoundActivation(); // ラウンドごとの能力発動判定を行う
	SpecialAbility::Instance().ApplyRoundStartAbilities(); // ラウンド開始時に適用される能力を処理する
}

void RoundManager::Uninitialize()
{
	roundFont.Uninitialize();
	
}

void RoundManager::Update(float elapsedTime)
{
	bool isPitchFinished = (ballCount::Instance().GetRemainingBalls() <= 0 &&
		Pitcher::Instance().GetCurrentState() == Pitcher::State::SelectingPitch);

	if(currentState == RoundState::SelectAbility)
	{
		UpdateSelectAbilityState();
		return;
	}
	
	if (isPitchFinished)
	{
		bool targetReached = HomeRunCount::Instance().IsHomeRunCountExceeded(GetCurrentTarget());
		if (targetReached)
		{
			if (IsFinalRound())
			{
				isGameClear = true;
			}
			//ラウンドが偶数で、サブミッションがクリアされている場合は特殊能力選択画面に遷移
			else if(currentRound % 2 ==0 && SubMission::Instance().IsCurrentMissionCleared())
			{
				EnterSelectAbilityState();
			}
			else
			{
				ProcessedToNextRound();
			}
		}
		else
		{
			isGameOver = true;
		}
	}

	if (currentRound <= 3)
	{
		Pitcher::Instance().SetBallSpeedMode(Pitcher::BallSpeedMode::slowSpeed);
	}
	else if(currentRound <= 6)
	{
		Pitcher::Instance().SetBallSpeedMode(Pitcher::BallSpeedMode::highSpeed);
	}
	else
	{
		Pitcher::Instance().SetBallSpeedMode(Pitcher::BallSpeedMode::realSpeed);
	}

}

void RoundManager::EnterSelectAbilityState()
{
	currentState = RoundState::SelectAbility;
	
	abilitiesChoice = SpecialAbility::Instance().GetRandomAbilities(3);
	hoveredAbilityIndex = -1; // ホバー中の特殊能力のインデックスをリセット

	//未所持の能力がなければ選択画面をスキップする
	if(abilitiesChoice.empty())
	{
		ProcessedToNextRound();
		return;
	}
}

void RoundManager::UpdateSelectAbilityState()
{
	Input& input = Input::Instance();


	//マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY());
	bool clicked = input.GetMouse().GetButtonDown() & Mouse::BTN_LEFT;

	hoveredAbilityIndex = -1; // ホバー中の特殊能力のインデックスをリセット

	for(int i = 0; i < abilitiesChoice.size(); ++i)
	{
		const auto& abilityID = abilitiesChoice[i];
		// アイコンの矩形を計算
		float iconX = abilityIconPositions[i].x - abilityIconSize.x / 2.0f;
		float iconY = abilityIconPositions[i].y - abilityIconSize.y / 2.0f;
		float iconWidth = abilityIconSize.x;
		float iconHeight = abilityIconSize.y;
		bool isHovered = (mousePos.x >= iconX && mousePos.x <= iconX + iconWidth &&
			mousePos.y >= iconY && mousePos.y <= iconY + iconHeight);

		if (isHovered)
		{
			hoveredAbilityIndex = i; // ホバー中のインデックスを更新
			if (clicked)
			{
				// 選択された能力を有効化
				SpecialAbility::Instance().SetOwned(abilityID, true);
				abilitiesChoice.clear(); // 選択肢をクリア
				ProcessedToNextRound();
				return;
			}
		}
	}

	

}

void RoundManager::ProcessedToNextRound()
{
	IncreaseRound();
	ballCount::Instance().ResetRemainingBalls();
	HomeRunCount::Instance().ResetCount();
	Combo::Instance().ResetCombo();
	currentState = RoundState::Playing;

	SpecialAbility::Instance().RollRoundActivation(); // ラウンドごとの能力発動判定を行う
	SpecialAbility::Instance().ApplyRoundStartAbilities(); // ラウンド開始時に適用される能力を処理する
}

void RoundManager::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	const auto pitcherState = Pitcher::Instance().GetCurrentState();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

	if (currentState == RoundState::SelectAbility)
	{
		if (abilityBackSprite && abilityBackSpriteData)
		{
			abilityBackSprite->render(dc,
				abilityBackSpriteData->position.x, abilityBackSpriteData->position.y,
				abilityBackSpriteData->size.x, abilityBackSpriteData->size.y,
				abilityBackSpriteData->color.x, abilityBackSpriteData->color.y, abilityBackSpriteData->color.z, abilityBackSpriteData->color.w,
				abilityBackSpriteData->rotation);
		}


		//選択画面
		for (int i = 0; i < abilitiesChoice.size(); ++i)
		{
			bool isHightlighted = (i == hoveredAbilityIndex);// ホバー中のアイコンかどうかを判定
			SpecialAbility::Instance().RenderAbilityIcons(abilitiesChoice[i], abilityIconPositions[i], abilityIconSize, isHightlighted);
		}

		dc->OMSetDepthStencilState(
			renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
		return; // 選択中は通常のラウンド表示をしない
	}

	//select以外の時は描画しない
	if (pitcherState != Pitcher::State::SelectingPitch) return;

	if(roundSpriteData && roundSprite)
	{
		roundSprite->render(dc,
			spritePosition.x - spriteSize.x / 2.0f,
			spritePosition.y - spriteSize.y / 2.0f,
			spriteSize.x, spriteSize.y,
			spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w,
			roundSpriteData->rotation);
	}


	// ラウンド表示の描画
	std::string roundText = std::to_string(currentRound) + " / " + std::to_string(totalRounds);
	roundFont.DrawTextW(Graphics::Instance().GetDeviceContext(), roundText.c_str(),
		roundTextPosition.x, roundTextPosition.y,
		roundTextScale,
		roundTextColor.x, roundTextColor.y, roundTextColor.z, roundTextColor.w);

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void RoundManager::DrawGUI()
{
	ImGui::DragInt("currentRound", &currentRound, 1, 1, totalRounds);

	if (ImGui::CollapsingHeader("Round Font"))
	{
		ImGui::Text("Current Round: %d / %d", currentRound, totalRounds);
		ImGui::DragFloat2("Round Text Position", &roundTextPosition.x, 1.0f, 0.0f, 1920.0f);
		ImGui::DragFloat("Round Text Scale", &roundTextScale, 0.1f, 5.0f);
		ImGui::ColorEdit4("Round Text Color", &roundTextColor.x);
	}
	if(ImGui::CollapsingHeader("Round Sprite"))
	{
		ImGui::DragFloat2("Sprite Position", &spritePosition.x, 1.0f, 0.0f, 1920.0f);
		ImGui::DragFloat2("Sprite Size", &spriteSize.x, 1.0f, 0.0f, 1920.0f);
		ImGui::ColorEdit4("Sprite Color", &spriteColor.x);
	}

}

void RoundManager::SaveToJson(json& j)
{
	
	//j["totalRounds"] = totalRounds;
	j["roundTextPosition"] = { roundTextPosition.x, roundTextPosition.y };
	j["roundTextScale"] = roundTextScale;
	j["roundTextColor"] = { roundTextColor.x, roundTextColor.y, roundTextColor.z, roundTextColor.w };
	//j["targetHomeRuns"] = targetHomeRuns; 
	j["spritePosition"] = { spritePosition.x, spritePosition.y };
	j["spriteSize"] = { spriteSize.x, spriteSize.y };
	j["spriteColor"] = { spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w };

	
}

void RoundManager::LoadFromJson(const json& j)
{
	//if (j.contains("totalRounds")) totalRounds = j["totalRounds"].get<int>();
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
	/*if(j.contains("targetHomeRuns"))
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
	}*/
	if(j.contains("spritePosition"))
	{
		auto pos = j["spritePosition"];
		if (pos.is_array() && pos.size() == 2)
		{
			spritePosition.x = pos[0].get<float>();
			spritePosition.y = pos[1].get<float>();
		}
	}
	if(j.contains("spriteSize"))
	{
		auto size = j["spriteSize"];
		if (size.is_array() && size.size() == 2)
		{
			spriteSize.x = size[0].get<float>();
			spriteSize.y = size[1].get<float>();
		}
	}
	if(j.contains("spriteColor"))
	{
		auto color = j["spriteColor"];
		if (color.is_array() && color.size() == 4)
		{
			spriteColor.x = color[0].get<float>();
			spriteColor.y = color[1].get<float>();
			spriteColor.z = color[2].get<float>();
			spriteColor.w = color[3].get<float>();
		}
	}

}