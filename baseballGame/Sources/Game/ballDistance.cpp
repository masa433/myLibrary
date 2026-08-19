#include "ballDistance.h"
#include "Graphics.h"
#include "Ball.h"
#include "physxManager.h"
#include <imgui.h>
#include <shader.h>

void BallDistance::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	// 球種名と球速表示に必要な文字だけをベイクする
	std::vector<int> pitchInfoCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789m"
	);

	// 日本語グリフを持つフォントを用意して配置する
	ballDistanceFont.Initialize(device,
		L".\\resources\\fonts\\Futur12.ttf",
		100.0f,
		screenWidth, screenHeight,
		512, 512,
		&pitchInfoCodepoints);

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	distanceBackData = std::make_unique<DistanceBackData>();
	distanceBackData->texturePath = L".\\resources\\textures\\distanceBack.png";
	distanceBackData->position = { distanceBackPosition.x, distanceBackPosition.y };
	distanceBackData->size = { distanceBackSize.x, distanceBackSize.y };
	distanceBackData->rotation = 0.0f;
	distanceBackData->color = { distanceBackColor.x, distanceBackColor.y, distanceBackColor.z, distanceBackColor.w };
	distanceBackSprite = std::make_unique<sprite>(device, dc, distanceBackData->texturePath.c_str());

	ResetMaxDistance();

}

void BallDistance::Uninitialize()
{
	ballDistanceFont.Uninitialize();
	distanceBackSprite.reset();
	distanceBackData.reset();
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
		fontColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色に戻す
	}
	else 
	{
		currentDistance = Physics::Instance().GetLastDistanceWasTotal()
			? Physics::Instance().GetBallTotalDistance()
			: Physics::Instance().GetBallHorizontalDistance();

		// 最終的な飛距離を保持する
		finalDistance = currentDistance;

		snprintf(distanceText, sizeof(distanceText), "%.fm", currentDistance);
		hasDistanceText = true;
		isDistanceLocked = true; // 以後は加算・更新しない

		// 最高飛距離を更新する
		if(currentDistance > maxDistance)
		{
			maxDistance = currentDistance;
		}

		if (Ball::Instance().GetHasPassedHomeRunZone())
		{
			fontColor = { 1.0f,0.85f,0.0f,1.0f };// ホームランゾーンを超えたら、金色にする
		}
	}
	
}

void BallDistance::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	
	RenderState* renderState = Graphics::Instance().GetRenderState();

	// Wind と同じようにシェーダーをセット
	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// ボールがバットに当たったら、距離を表示する(ボールの位置でリアルタイムに更新する)
	
	if (!hasDistanceText) return;

	if(distanceBackData && distanceBackSprite)
	{
		distanceBackSprite->render(dc,
			distanceBackPosition.x - distanceBackSize.x / 2.0f,
			distanceBackPosition.y - distanceBackSize.y / 2.0f,
			distanceBackSize.x, distanceBackSize.y,
			distanceBackColor.x, distanceBackColor.y,
			distanceBackColor.z, distanceBackColor.w,
			distanceBackData->rotation);
	}


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

	// 後始末（Wind と同じ）
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
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

	if(ImGui::CollapsingHeader("Distance Background Settings"))
	{
		ImGui::Text("Background Position");
		ImGui::DragFloat2("Position", &distanceBackPosition.x);
		ImGui::Text("Background Size");
		ImGui::DragFloat2("Size", &distanceBackSize.x);
		ImGui::Text("Background Color");
		ImGui::ColorEdit4("Color", reinterpret_cast<float*>(&distanceBackColor));
	}


	ImGui::Text(" maxDistance: %.f", maxDistance);
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

	//飛距離表示の背景の保存
	j["distanceBackPositionX"] = distanceBackPosition.x;
	j["distanceBackPositionY"] = distanceBackPosition.y;
	j["distanceBackSizeX"] = distanceBackSize.x;
	j["distanceBackSizeY"] = distanceBackSize.y;
	j["distanceBackColorR"] = distanceBackColor.x;
	j["distanceBackColorG"] = distanceBackColor.y;
	j["distanceBackColorB"] = distanceBackColor.z;
	j["distanceBackColorA"] = distanceBackColor.w;

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

	if (j.contains("distanceBackPositionX") && j.contains("distanceBackPositionY"))
	{
		distanceBackPosition.x = j["distanceBackPositionX"].get<float>();
		distanceBackPosition.y = j["distanceBackPositionY"].get<float>();
	}
	if(j.contains("distanceBackSizeX") && j.contains("distanceBackSizeY"))
	{
		distanceBackSize.x = j["distanceBackSizeX"].get<float>();
		distanceBackSize.y = j["distanceBackSizeY"].get<float>();
	}
	if (j.contains("distanceBackColorR") && j.contains("distanceBackColorG") && j.contains("distanceBackColorB") && j.contains("distanceBackColorA"))
	{
		distanceBackColor.x = j["distanceBackColorR"].get<float>();
		distanceBackColor.y = j["distanceBackColorG"].get<float>();
		distanceBackColor.z = j["distanceBackColorB"].get<float>();
		distanceBackColor.w = j["distanceBackColorA"].get<float>();
	}
}