#include "ballCount.h"
#include "Graphics.h"
#include "shader.h"
#include "imgui.h"
#include "Pitcher.h"

void ballCount::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// シェーダーの初期化
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());
	// ボールカウントスプライトの初期化
	ballCountData = std::make_unique<BallCountData>();
	ballCountData->texturePath = L".\\resources\\textures\\ballCountBack.png";
	ballCountData->position = { ballCountPosition.x, ballCountPosition.y }; // 画面左上に配置
	ballCountData->size = { ballCountSize.x, ballCountSize.y };
	ballCountData->rotation = 0.0f;
	ballCountData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	ballCountSprite = std::make_unique<sprite>(device, context, ballCountData->texturePath.c_str());
	// 球種名と球速表示用のフォントレンダラーの初期化
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	// 球種名と球速表示に必要な文字だけをベイクする
	std::vector<int> pitchInfoCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789"
		
	);
	pitchInfoFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		28.0f,
		screenWidth,
		screenHeight,
		512.0f,
		512.0f,
		&pitchInfoCodepoints);

	remainingBalls = 10; // 初期の残り球数を設定

}

void ballCount::Uninitialize()
{
	ballCountSprite.reset();
	ballCountData.reset();
	pitchInfoFont.Uninitialize();
}

void ballCount::Update(float elapsedTime)
{
	// 必要に応じて残りの球数を更新する処理を追加
}

void ballCount::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	RenderState* renderState = Graphics::Instance().GetRenderState();

	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	
	const auto pitcherState = Pitcher::Instance().GetCurrentState();

	
	//select以外の時は描画しない
	if (pitcherState != Pitcher::State::SelectingPitch) return;

	// Wind と同じようにシェーダーをセット
	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	
	DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(ballCountPosition);
	DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(ballCountSize);

	// ボールカウントの背景を描画
	if (ballCountSprite)
	{
		ballCountSprite->render(dc,
			scaledPos.x, scaledPos.y,
			scaledSize.x, scaledSize.y,
			ballCountData->color.x, ballCountData->color.y, ballCountData->color.z, ballCountData->color.w,
			ballCountData->rotation);
	}

	auto centerTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
		{
			float textWidth = 0.0f;
			float textHeight = 0.0f;
			pitchInfoFont.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
			return { x - textWidth / 2.0f, y - textHeight / 2.0f };
		};

	DirectX::XMFLOAT2 scaledFontPos = screenScaler.Scale(pitchInfoPosition);

	const float scaledFontSize = pitchInfoScale * screenScaler.GetScaleY();

	std::string remainingBallsText = std::to_string(remainingBalls);
	DirectX::XMFLOAT2 fontPos = centerTextPosition(remainingBallsText, scaledFontSize, scaledFontPos.x, scaledFontPos.y);

	
	// 残りの球数を描画
	pitchInfoFont.DrawTextW(dc, remainingBallsText.c_str(),
		fontPos.x, fontPos.y, scaledFontSize,
		pitchInfoColor.x, pitchInfoColor.y, pitchInfoColor.z, pitchInfoColor.w);
	

	// 後始末（Wind と同じ）
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void ballCount::DrawGUI()
{
	if(ImGui::CollapsingHeader("Ball Count Settings"))
	{
		ImGui::DragInt("Remaining Balls: %d", &remainingBalls);
		
		if(ImGui::CollapsingHeader("Ball Count Background Settings"))
		{
			ImGui::DragFloat2("Position", &ballCountPosition.x, 1.0f);
			ImGui::DragFloat2("Size", &ballCountSize.x, 1.0f);
		}
		if(ImGui::CollapsingHeader("Pitch Info Settings"))
		{
			ImGui::DragFloat2("Position", &pitchInfoPosition.x, 1.0f);
			ImGui::DragFloat("Scale", &pitchInfoScale, 0.01f, 0.1f, 5.0f);
			ImGui::ColorEdit4("Color", &pitchInfoColor.x);
		}
	}
}

void ballCount::SaveToJson(json& j)
{
	j["ballCountPosition"] = { ballCountPosition.x, ballCountPosition.y };
	j["ballCountSize"] = { ballCountSize.x, ballCountSize.y };
	j["pitchInfoPosition"] = { pitchInfoPosition.x, pitchInfoPosition.y };
	j["pitchInfoScale"] = pitchInfoScale;
	j["pitchInfoColor"] = { pitchInfoColor.x, pitchInfoColor.y, pitchInfoColor.z, pitchInfoColor.w };
}

void ballCount::LoadFromJson(const json& j)
{
	if (j.contains("ballCountPosition"))
	{
		ballCountPosition.x = j["ballCountPosition"][0].get<float>();
		ballCountPosition.y = j["ballCountPosition"][1].get<float>();
	}
	if (j.contains("ballCountSize"))
	{
		ballCountSize.x = j["ballCountSize"][0].get<float>();
		ballCountSize.y = j["ballCountSize"][1].get<float>();
	}
	if (j.contains("pitchInfoPosition"))
	{
		pitchInfoPosition.x = j["pitchInfoPosition"][0].get<float>();
		pitchInfoPosition.y = j["pitchInfoPosition"][1].get<float>();
	}
	if (j.contains("pitchInfoScale")) pitchInfoScale = j["pitchInfoScale"].get<float>();
	if (j.contains("pitchInfoColor"))
	{
		pitchInfoColor.x = j["pitchInfoColor"][0].get<float>();
		pitchInfoColor.y = j["pitchInfoColor"][1].get<float>();
		pitchInfoColor.z = j["pitchInfoColor"][2].get<float>();
		pitchInfoColor.w = j["pitchInfoColor"][3].get<float>();
	}
}