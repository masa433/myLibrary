#include "Wind.h"
#include "Graphics.h"
#include <cmath>
#include "physxManager.h"
#include "imgui.h"
#include "Ball.h"
#include <shader.h>
#include "FontRenderer.h"

void Wind::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, "sprite_vs.cso", spriteVS.GetAddressOf(), spriteInputLayout.GetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, "sprite_ps.cso", spritePS.GetAddressOf());

	// スプライトの初期化
	windDirectionSprite = std::make_unique<Sprite>();
	windDirectionSprite->texturePath = L".\\resources\\textures\\windDirection.png";
	windDirectionSprite->position = { 1150.0f, 100.0f };
	windDirectionSprite->size = { 50.0f, 70.0f };
	windDirectionSprite->rotation = 0.0f;
	windDirectionSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	windDirectionSpriteRenderer = std::make_unique<sprite>(device, windDirectionSprite->texturePath.c_str());

	windGroundSprite = std::make_unique<Sprite>();
	windGroundSprite->texturePath = L".\\resources\\textures\\ground.png";
	windGroundSprite->position = { 1100.0f, 100.0f };
	windGroundSprite->size = { 150.0f, 100.0f };
	windGroundSprite->rotation = 0.0f;
	windGroundSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	windGroundSpriteRenderer = std::make_unique<sprite>(device, windGroundSprite->texturePath.c_str());

	windBoardSprite = std::make_unique<Sprite>();
	windBoardSprite->texturePath = L".\\resources\\textures\\windBoard.png";
	windBoardSprite->position = { 1100.0f, 200.0f };
	windBoardSprite->size = { 150.0f, 100.0f };
	windBoardSprite->rotation = 0.0f;
	windBoardSprite->color = { 1.0f, 1.0f, 1.0f, 0.8f };
	windBoardSpriteRenderer = std::make_unique<sprite>(device, windBoardSprite->texturePath.c_str());

	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> windStrengthCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789.m"
	);

	windStrengthFont.Initialize(device,
		L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf",
		32.0f,
		screenWidth, screenHeight,
		/*atlasWidth*/ 256, /*atlasHeight*/ 256,
		&windStrengthCodepoints);

	// 風表現用の流線を生成
	windLines.clear();
	windLines.reserve(100);
	for (int i = 0; i < 100; ++i)
	{
		const float t = static_cast<float>(i);
		WindLine line{};
		line.position = {
			-30.0f + std::fmod(t * 7.3f, 60.0f),
			0.0f,
			-5.0f + std::fmod(t * 5.1f, 100.0f)
		};
		// Y軸の相対的な位置割合(0.0 ～ 1.0)を決定して保存する
		line.baseYOffset = std::fmod(t * 1.7f, 1.0f);

		line.speed = windStrength * (0.6f + std::fmod(t * 0.37f, 1.0f));
		line.length = 1.5f + std::fmod(t * 0.23f, 2.0f);
		line.phase = t * 0.4f;
		windLines.push_back(line);
	}

	windHeight = 20.0f; // 風の流線の高さ
	windThickness = 50.0f; // 風の流線の厚み
}

void Wind::Uninitialize()
{
	windDirectionSpriteRenderer.reset();
	windGroundSpriteRenderer.reset();
	windStrengthFont.Uninitialize(); // ★変更
}

void Wind::Update(float elapsedTime)
{
	// 風の流線のアニメーション
	for (auto& line : windLines)
	{
		line.position.x += windDirection.x * line.speed * elapsedTime;
		line.baseYOffset += (windDirection.y * line.speed * elapsedTime) / (windThickness > 0.01f ? windThickness : 0.01f);
		line.position.z += windDirection.z * line.speed * elapsedTime;
		line.phase += elapsedTime * 4.0f;

		// 画面外に出たらループさせる (X軸とZ軸)
		if (line.position.x > 100.0f) line.position.x -= 200.0f;
		else if (line.position.x < -100.0f) line.position.x += 200.0f;

		if (line.position.z > 100.0f) line.position.z -= 105.0f;
		else if (line.position.z < -5.0f) line.position.z += 105.0f;

		// Y軸(上下)の相対範囲ループ (0.0 ～ 1.0)
		if (line.baseYOffset > 1.0f) line.baseYOffset -= 1.0f;
		else if (line.baseYOffset < 0.0f) line.baseYOffset += 1.0f;

		// 実際のY座標を計算して更新
		line.position.y = line.baseYOffset * windThickness;
	}
}

void Wind::Render(const RenderContext& rc)
{
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// 風の流線を描画
	for (const auto& line : windLines)
	{
		DirectX::XMFLOAT3 start = line.position;
		start.y += windHeight; // 風の高さを加算
		DirectX::XMFLOAT3 end = {
			line.position.x - windDirection.x * line.length,
			(line.position.y + windHeight) - windDirection.y * line.length,
			line.position.z - windDirection.z * line.length
		};

		DirectX::XMFLOAT4 color = { 0.8f, 0.9f, 1.0f, 0.35f };

		primitiveRenderer->AddVertex(start, color);
		primitiveRenderer->AddVertex(end, color);
	}

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0); // 書き込みなし

	if (windGroundSprite && windGroundSpriteRenderer)
	{
		windGroundSpriteRenderer->render(rc.deviceContext, windGroundSprite->position.x, windGroundSprite->position.y,
			windGroundSprite->size.x, windGroundSprite->size.y,
			windGroundSprite->color.x, windGroundSprite->color.y, windGroundSprite->color.z, windGroundSprite->color.w,
			0.0f);
	}

	// 風向きスプライトの描画
	if (windDirectionSprite && windDirectionSpriteRenderer)
	{
		windDirectionSprite->rotation = atan2f(windDirection.x, windDirection.z); // 風向きに合わせて回転
		windDirectionSpriteRenderer->render(rc.deviceContext, windDirectionSprite->position.x, windDirectionSprite->position.y,
			windDirectionSprite->size.x, windDirectionSprite->size.y,
			windDirectionSprite->color.x, windDirectionSprite->color.y, windDirectionSprite->color.z, windDirectionSprite->color.w,
			DirectX::XMConvertToDegrees(windDirectionSprite->rotation));


	}

	// 風の強さを示すボードスプライトの描画
	if (windBoardSprite && windBoardSpriteRenderer)
	{
		windBoardSpriteRenderer->render(rc.deviceContext, windBoardSprite->position.x, windBoardSprite->position.y,
			windBoardSprite->size.x, windBoardSprite->size.y,
			windBoardSprite->color.x, windBoardSprite->color.y, windBoardSprite->color.z, windBoardSprite->color.w,
			0.0f);
	}

	// 風の強さテキストをFontRenderer(TTF直読み)で描画
	if (windStrengthFont.IsValid())
	{
		physx::PxVec3 windVec(windDirection.x * windStrength, windDirection.y * windStrength, windDirection.z * windStrength);
		float currentWindSpeed = windVec.magnitude();

		char speedText[64];
		snprintf(speedText, sizeof(speedText), "%.f m", currentWindSpeed);

		// アイコンの座標に基づいてテキスト位置を決定
		float textX = windDirectionSprite->position.x + 80.0f;
		float textY = windDirectionSprite->position.y + 100.0f;

		// scale=1.0でInitialize時のpixelHeight(32px)相当の大きさになる。
		// 大きさを変えたい場合はscaleを調整する(例: 1.5fで1.5倍)。
		windStrengthFont.DrawTextW(dc, speedText,
			textX, textY,
			1.0f,
			1.0f, 1.0f, 1.0f, 1.0f);
	}

	// 描画後に元に戻す
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}


void Wind::DrawGUI()
{
#ifdef USE_IMGUI

	if (ImGui::CollapsingHeader("Wind Settings"))
	{
		// 風向の操作
		ImGui::DragFloat3("Wind Direction", &windDirection.x, 0.01f, -1.0f, 1.0f);
		if (ImGui::Button("Normalize Wind Direction"))
		{
			DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&windDirection);
			// ゼロベクトルの場合は正規化しない
			if (DirectX::XMVector3NotEqual(dir, DirectX::XMVectorZero()))
			{
				dir = DirectX::XMVector3Normalize(dir);
				DirectX::XMStoreFloat3(&windDirection, dir);
			}
		}

		// 風の強さの操作
		ImGui::DragFloat("Wind Strength", &windStrength, 0.1f, 0.0f, 50.0f);

		// 風の基本高さの操作
		ImGui::DragFloat("Wind Height", &windHeight, 0.1f, -10.0f, 50.0f);

		// 風の厚みの操作
		ImGui::DragFloat("Wind Thickness", &windThickness, 0.1f, 0.1f, 100.0f);

		// 流線の描画などに強さの変更を即時反映させるため、表示用に現在の風ベクトルも表示する
		ImGui::Text("Current Wind Velocity: (%.2f, %.2f, %.2f)",
			windDirection.x * windStrength,
			windDirection.y * windStrength,
			windDirection.z * windStrength);
	}

	//スプライトのデバッグ表示
	if (ImGui::CollapsingHeader("Sprite Debug"))
	{
		if (windDirectionSprite)
		{
			ImGui::DragFloat2("Wind Direction Sprite Position", &windDirectionSprite->position.x, 1.0f, 0.0f, 1280.0f);
			ImGui::DragFloat2("Wind Direction Sprite Size", &windDirectionSprite->size.x, 1.0f, 1.0f, 500.0f);
			ImGui::DragFloat("Wind Direction Sprite Rotation", &windDirectionSprite->rotation, 1.0f, 0.0f, 360.0f);
			ImGui::ColorEdit4("Wind Direction Sprite Color", &windDirectionSprite->color.x);
		}
		ImGui::Separator();
		if (windGroundSprite)
		{
			ImGui::DragFloat2("Wind Ground Sprite Position", &windGroundSprite->position.x, 1.0f, 0.0f, 1280.0f);
			ImGui::DragFloat2("Wind Ground Sprite Size", &windGroundSprite->size.x, 1.0f, 1.0f, 500.0f);
			ImGui::ColorEdit4("Wind Ground Sprite Color", &windGroundSprite->color.x);
		}
		ImGui::Separator();
		if (windBoardSprite)
		{
			ImGui::DragFloat2("Wind Board Sprite Position", &windBoardSprite->position.x, 1.0f, 0.0f, 1280.0f);
			ImGui::DragFloat2("Wind Board Sprite Size", &windBoardSprite->size.x, 1.0f, 1.0f, 500.0f);
			ImGui::ColorEdit4("Wind Board Sprite Color", &windBoardSprite->color.x);
		}
	}
#endif
}

bool Wind::IsBallInWindArea() const
{
	// 流線の描画範囲に合わせて風の有効範囲を定義
	if (Ball::Instance().GetWorldPosition().x < -100.0f || Ball::Instance().GetWorldPosition().x > 100.0f) return false;
	if (Ball::Instance().GetWorldPosition().y < windHeight || Ball::Instance().GetWorldPosition().y > windHeight + windThickness) return false;
	if (Ball::Instance().GetWorldPosition().z < -5.0f || Ball::Instance().GetWorldPosition().z > 95.0f) return false;

	return true;
}

void Wind::SaveToJson(json& j)
{
	j["direction"] = { windDirection.x, windDirection.y, windDirection.z };
	j["strength"] = windStrength;
	j["height"] = windHeight;
	j["thickness"] = windThickness;

	if (windDirectionSprite)
	{
		j["dir_sprite"]["position"] = { windDirectionSprite->position.x, windDirectionSprite->position.y };
		j["dir_sprite"]["size"] = { windDirectionSprite->size.x, windDirectionSprite->size.y };
		j["dir_sprite"]["rotation"] = windDirectionSprite->rotation;
		j["dir_sprite"]["color"] = { windDirectionSprite->color.x, windDirectionSprite->color.y, windDirectionSprite->color.z, windDirectionSprite->color.w };
	}
	if (windGroundSprite)
	{
		j["ground_sprite"]["position"] = { windGroundSprite->position.x, windGroundSprite->position.y };
		j["ground_sprite"]["size"] = { windGroundSprite->size.x, windGroundSprite->size.y };
		j["ground_sprite"]["color"] = { windGroundSprite->color.x, windGroundSprite->color.y, windGroundSprite->color.z, windGroundSprite->color.w };
	}
	if(windBoardSprite)
	{
		j["board_sprite"]["position"] = { windBoardSprite->position.x, windBoardSprite->position.y };
		j["board_sprite"]["size"] = { windBoardSprite->size.x, windBoardSprite->size.y };
		j["board_sprite"]["color"] = { windBoardSprite->color.x, windBoardSprite->color.y, windBoardSprite->color.z, windBoardSprite->color.w };
	}
}

void Wind::LoadFromJson(const json& j)
{
	if (j.contains("direction"))  windDirection = { j["direction"][0], j["direction"][1], j["direction"][2] };
	if (j.contains("strength"))   windStrength = j["strength"];
	if (j.contains("height"))     windHeight = j["height"];
	if (j.contains("thickness"))  windThickness = j["thickness"];

	if (j.contains("dir_sprite") && windDirectionSprite)
	{
		windDirectionSprite->position = { j["dir_sprite"]["position"][0], j["dir_sprite"]["position"][1] };
		windDirectionSprite->size = { j["dir_sprite"]["size"][0], j["dir_sprite"]["size"][1] };
		windDirectionSprite->rotation = j["dir_sprite"]["rotation"];
		windDirectionSprite->color = { j["dir_sprite"]["color"][0], j["dir_sprite"]["color"][1], j["dir_sprite"]["color"][2], j["dir_sprite"]["color"][3] };
	}
	if (j.contains("ground_sprite") && windGroundSprite)
	{
		windGroundSprite->position = { j["ground_sprite"]["position"][0], j["ground_sprite"]["position"][1] };
		windGroundSprite->size = { j["ground_sprite"]["size"][0], j["ground_sprite"]["size"][1] };
		windGroundSprite->color = { j["ground_sprite"]["color"][0], j["ground_sprite"]["color"][1], j["ground_sprite"]["color"][2], j["ground_sprite"]["color"][3] };
	}
	if (j.contains("board_sprite") && windBoardSprite)
	{
		windBoardSprite->position = { j["board_sprite"]["position"][0], j["board_sprite"]["position"][1] };
		windBoardSprite->size = { j["board_sprite"]["size"][0], j["board_sprite"]["size"][1] };
		windBoardSprite->color = { j["board_sprite"]["color"][0], j["board_sprite"]["color"][1], j["board_sprite"]["color"][2], j["board_sprite"]["color"][3] };
	}
}