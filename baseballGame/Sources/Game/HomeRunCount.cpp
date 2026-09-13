#include "HomeRunCount.h"
#include "Graphics.h"
#include "imgui.h"
#include "RoundManager.h"
#include "Combo.h"
#include <Pitcher.h>
#include "SubMission.h"

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
		u8"0123456789HOMERUN/"
	u8"お金をためよう！"
	u8"ホームランを打とう");
	// フォントレンダラーの初期化
	homeRunCountFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&homeRunCountCodepoints);

	missionFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&homeRunCountCodepoints);

	Combo::Instance().Initialize(device);

	SubMission::Instance().Initialize(device);

	ResetCount(); // ホームラン数を初期化
}

void HomeRunCount::Uninitialize()
{
	homeRunCountFont.Uninitialize();
	missionFont.Uninitialize();
	homeRunCountSprite.reset();
	homeRunCountSpriteData.reset();
	Combo::Instance().Uninitialize();
	SubMission::Instance().Uninitialize();
}

void HomeRunCount::Update(float elapsedTime)
{
	// ホームラン数が増えたら、表示を一旦大きくしてからアニメーションで戻す
	if (homeRunCount != previousHomeRunCount)
	{
		previousHomeRunCount = homeRunCount;
		numberDisplayScale = numberScale * numberPopScaleMultiplier; // 大きい状態から開始
		goldColorTime = 1.0f; // ゴールドカラーの表示時間をリセット
		Combo::Instance().AddCombo(1); // コンボを追加
	}

	// 現在のスケールを基準サイズへ滑らかに近づける
	isAnimating = numberDisplayScale > numberScale;

	if (isAnimating)
	{
		numberDisplayScale -= numberScaleAnimSpeed * elapsedTime;
		if (numberDisplayScale < numberScale)
		{
			numberDisplayScale = numberScale;
		}

	}

	bool isHomeRun = Ball::Instance().GetHasPassedHomeRunZone() ||
		Ball::Instance().GetHasCollidedWithPole();
	bool isHitFinished = Ball::Instance().GetHasCollidedWithFence() ||
		Ball::Instance().GetHasCollidedWithGround();

	if (isAnimating)
	{
		countColor = DirectX::XMFLOAT4(1.0f, 0.84f, 0.0f, 1.0f); // 金色
	}
	else if (goldColorTime > 0.0f || (isHitFinished && isHomeRun))
	{
		if (goldColorTime > 0.0f)
		{
			goldColorTime -= elapsedTime;
		}
		countColor = DirectX::XMFLOAT4(1.0f, 0.84f, 0.0f, 1.0f); // 金色
	}
	
	else
	{
		countColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
	}

	Combo::Instance().Update(elapsedTime);

	SubMission::Instance().Update(elapsedTime);

	broadcastCamera.Update(elapsedTime, Ball::Instance().GetHasCollidedWithBat());
}

void HomeRunCount::Render()
{
	
	bool isSelectPitching = Pitcher::Instance().GetCurrentState() == Pitcher::State::SelectingPitch;
	bool isTracking = broadcastCamera.IsTrackingBall();

	bool shouldRenderHomeRunCount = isSelectPitching || isTracking;// セレクト中かつヒットカメラでない場合、またはセレクト中でなくヒットカメラの場合に描画する
	if (shouldRenderHomeRunCount) 
	{
		ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
		RenderState* renderState = Graphics::Instance().GetRenderState();
		ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

		dc->VSSetShader(spriteVS.Get(), nullptr, 0);
		dc->PSSetShader(spritePS.Get(), nullptr, 0);
		dc->IASetInputLayout(spriteInputLayout.Get());

		dc->OMSetDepthStencilState(
			renderState->GetDepthStencilState(DepthState::TestOnly), 0);

		// ホームラン数の描画
		DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(homeRunCountSpriteData->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(homeRunCountSpriteData->size);

		homeRunCountSprite->render(Graphics::Instance().GetDeviceContext(),
			scaledPos.x, scaledPos.y,
			scaledSize.x, scaledSize.y,
			homeRunCountSpriteData->color.x, homeRunCountSpriteData->color.y,
			homeRunCountSpriteData->color.z, homeRunCountSpriteData->color.w,
			homeRunCountSpriteData->rotation);



		// 数字だけ大きく、ラベルの下に描画
		int currentHomeRunTarget = RoundManager::Instance().GetCurrentTarget();
		int currentRound = RoundManager::Instance().GetCurrentRound();
		char roundBuffer[64];
		if (currentRound == 1)
		{
			sprintf_s(roundBuffer, sizeof(roundBuffer), u8"お金をためよう！");
		}
		else
		{
			sprintf_s(roundBuffer, sizeof(roundBuffer), u8"ホームランを打とう！");
		}





		char countBuffer[16];
		char slashBuffer[4] = " / ";
		char targetBuffer[16];
		sprintf_s(countBuffer, sizeof(countBuffer), "%d", homeRunCount);
		sprintf_s(targetBuffer, sizeof(targetBuffer), "%d", currentHomeRunTarget);

		// 中央寄せしたい場合は幅を測ってから位置を調整
		float countWidth = 0.0f, countHeight = 0.0f;
		float slashWidth = 0.0f, slashHeight = 0.0f;
		float targetWidth = 0.0f, targetHeight = 0.0f;

		//文字列のスケーリングを行うラムダ式
		auto TextScaling = [&](const DirectX::XMFLOAT2& position, float size) -> std::pair<DirectX::XMFLOAT2, float>
		{
			return {
				screenScaler.Scale(position),                 
				size * screenScaler.GetUniformScale()     
			};
		};


		homeRunCountFont.MeasureText(countBuffer, numberDisplayScale, countWidth, countHeight);
		homeRunCountFont.MeasureText(slashBuffer, slashDisplayScale, slashWidth, slashHeight);
		homeRunCountFont.MeasureText(targetBuffer, targetDisplayScale, targetWidth, targetHeight);

		float totalWidth = countWidth + slashWidth + targetWidth;
		float startX = homeRunCountSpriteData->position.x + (homeRunCountSpriteData->size.x - totalWidth) / 2.0f;

		if (currentRound == 1)
		{
			auto [scaledLabelPos, scaledLabelScale] = TextScaling({ labelPositionX, labelPositionY }, labelScale);

			homeRunCountFont.DrawTextW(dc,
				roundBuffer,
				scaledLabelPos.x,
				scaledLabelPos.y,
				scaledLabelScale,
				1.0f, 1.0f, 1.0f, 1.0f); // 白色
		}
		else
		{
			auto [scaledNumberPos, scaledNumberScale] = TextScaling({ startX, numberPositionY }, numberScale);

			homeRunCountFont.DrawTextW(dc,
				countBuffer,
				scaledNumberPos.x,
				scaledNumberPos.y,
				scaledNumberScale,
				countColor.x, countColor.y, countColor.z, countColor.w); // 金色にして目立たせる例

			auto [scaledSlashPos, scaledSlashScale] = TextScaling({ startX + countWidth, numberPositionY }, slashDisplayScale);

			homeRunCountFont.DrawTextW(dc,
				slashBuffer,
				scaledSlashPos.x,
				scaledSlashPos.y,
				scaledSlashScale,
				slashColor.x, slashColor.y, slashColor.z, slashColor.w); // 金色にして目立たせる例
			
			auto [scaledTargetPos, scaledTargetScale] = TextScaling({ startX + countWidth + slashWidth, numberPositionY }, targetDisplayScale);

			homeRunCountFont.DrawTextW(dc,
				targetBuffer,
				scaledTargetPos.x,
				scaledTargetPos.y,
				scaledTargetScale,
				targetColor.x, targetColor.y, targetColor.z, targetColor.w); // 金色にして目立たせる例

			auto [scaledMissionPos, scaledMissionScale] =
				TextScaling(missionLabelPosition, missionLabelScale);

			missionFont.DrawTextW(dc,
				roundBuffer,
				scaledMissionPos.x,
				scaledMissionPos.y,
				scaledMissionScale,
				1.0f, 1.0f, 1.0f, 1.0f); // 白色

		}

		bool isFinished = (Ball::Instance().GetHasPassedHomeRunZone() && (Ball::Instance().GetHasCollidedWithFence() || Ball::Instance().GetHasCollidedWithGround()) || Ball::Instance().GetHasCollidedWithPole());

		if (isFinished && Pitcher::Instance().GetCurrentState() == Pitcher::State::WaitingForResult)
		{
			Combo::Instance().Render();
		}

		SubMission::Instance().Render();

		// 後始末（Wind と同じ）
		dc->VSSetShader(nullptr, nullptr, 0);
		dc->PSSetShader(nullptr, nullptr, 0);
		dc->IASetInputLayout(nullptr);

		dc->OMSetDepthStencilState(
			renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	}
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
		ImGui::DragFloat("Number Display Scale", &numberDisplayScale);
		ImGui::ColorEdit4("Number Count Color", &countColor.x);
		ImGui::DragFloat("Slash Scale", &slashDisplayScale);
		ImGui::ColorEdit4("Number Slash Color", &slashColor.x);
		ImGui::DragFloat("Target Scale", &targetDisplayScale);
		ImGui::ColorEdit4("Number Target Color", &targetColor.x);

		ImGui::Separator();
		ImGui::DragFloat("Pop Scale Multiplier", &numberPopScaleMultiplier, 0.05f, 1.0f, 5.0f);
		ImGui::DragFloat("Pop Anim Speed", &numberScaleAnimSpeed, 0.1f, 0.5f, 20.0f);

		ImGui::Separator();
		ImGui::DragFloat2("Mission Label Position", &missionLabelPosition.x);
		ImGui::DragFloat("Mission Label Scale", &missionLabelScale);
	}
	Combo::Instance().DrawGUI();

	SubMission::Instance().DrawGUI();
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
		{"numberDisplayScale", numberDisplayScale},
		{"slashScale", slashDisplayScale},
		{"targetScale", targetDisplayScale},
		{"numberCountColor", {countColor.x, countColor.y, countColor.z, countColor.w}},
		{"numberSlashColor", {slashColor.x, slashColor.y, slashColor.z, slashColor.w}},
		{"numberTargetColor", {targetColor.x, targetColor.y, targetColor.z, targetColor.w}},
		{"missionLabelPosition", {missionLabelPosition.x, missionLabelPosition.y}},
		{"missionLabelScale", missionLabelScale}
	};

	Combo::Instance().SaveToJson(j["combo"]);

	SubMission::Instance().SaveToJson(j);
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
		if (fontJson.contains("numberCountColor"))
		{
			const auto& color = fontJson["numberCountColor"];
			if (color.is_array() && color.size() == 4)
			{
				countColor.x = color[0].get<float>();
				countColor.y = color[1].get<float>();
				countColor.z = color[2].get<float>();
				countColor.w = color[3].get<float>();
			}
		}
		if (fontJson.contains("numberSlashColor"))
		{
			const auto& color = fontJson["numberSlashColor"];
			if (color.is_array() && color.size() == 4)
			{
				slashColor.x = color[0].get<float>();
				slashColor.y = color[1].get<float>();
				slashColor.z = color[2].get<float>();
				slashColor.w = color[3].get<float>();
			}
		}
		if (fontJson.contains("numberTargetColor"))
		{
			const auto& color = fontJson["numberTargetColor"];
			if (color.is_array() && color.size() == 4)
			{
				targetColor.x = color[0].get<float>();
				targetColor.y = color[1].get<float>();
				targetColor.z = color[2].get<float>();
				targetColor.w = color[3].get<float>();
			}
		}
		if (fontJson.contains("numberDisplayScale"))
			numberDisplayScale = fontJson["numberDisplayScale"].get<float>();
		if (fontJson.contains("slashScale"))
			slashDisplayScale = fontJson["slashScale"].get<float>();
		if (fontJson.contains("targetScale"))
			targetDisplayScale = fontJson["targetScale"].get<float>();
		if (fontJson.contains("missionLabelPosition"))
			{
			const auto& pos = fontJson["missionLabelPosition"];
			if (pos.is_array() && pos.size() == 2)
			{
				missionLabelPosition.x = pos[0].get<float>();
				missionLabelPosition.y = pos[1].get<float>();
			}
		}
		if (fontJson.contains("missionLabelScale"))
			missionLabelScale = fontJson["missionLabelScale"].get<float>();
		
	}

	Combo::Instance().LoadFromJson(j["combo"]);

	SubMission::Instance().LoadFromJson(j);

	ResetCount();
}