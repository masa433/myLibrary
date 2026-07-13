#include "FoulSprite.h"
#include "Graphics.h"
#include <imgui.h>
#include "Ball.h"


void FoulSprite::Initialize(ID3D11Device* device)
{
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.GetAddressOf(), spriteInputLayout.GetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.GetAddressOf());

	foulSprite = std::make_unique<Sprite>();
	foulSprite->texturePath = L".\\resources\\textures\\Foul.png";
	foulSprite->position = { Graphics::Instance().GetScreenWidth() * 0.5f, Graphics::Instance().GetScreenHeight() * 0.5f }; // 中央に表示
	foulSprite->size = { 300.0f, 100.0f };
	foulSprite->rotation = 0.0f;
	foulSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	foulSpriteRenderer = std::make_unique<sprite>(device, foulSprite->texturePath.c_str());
}

void FoulSprite::Uninitialize()
{
	foulSpriteRenderer.reset();
	foulSprite.reset();
}

void FoulSprite::Update(float elapsedTime)
{
	if (!showFoulSprite) return;

	showDuration += elapsedTime;

	// フェードイン中
	if (showDuration < fadeInTime)
	{
		alpha = showDuration / fadeInTime;
		if (alpha > 1.0f) alpha = 1.0f;
	}
	// 表示中（フェードイン完了後）
	else if (showDuration < fadeInTime + displayTime)
	{
		alpha = 1.0f;
	}
	// フェードアウト中
	else if (showDuration < fadeInTime + displayTime + fadeOutTime)
	{
		float fadeOutProgress = (showDuration - (fadeInTime + displayTime)) / fadeOutTime;
		alpha = 1.0f - fadeOutProgress;
		if (alpha < 0.0f) alpha = 0.0f;
	}
	// 完全に消えたらリセット
	else
	{
		showFoulSprite = false;
		showDuration = 0.0f;
		alpha = 0.0f;
	}
}


void FoulSprite::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if (showFoulSprite && foulSpriteRenderer)
	{
		foulSpriteRenderer->render(Graphics::Instance().GetDeviceContext(),
			foulSprite->position.x - foulSprite->size.x * 0.5f,
			foulSprite->position.y - foulSprite->size.y * 0.5f,
			foulSprite->size.x,
			foulSprite->size.y,
			foulSprite->color.x,
			foulSprite->color.y,
			foulSprite->color.z,
			foulSprite->color.w * alpha,
			foulSprite->rotation);
	}

	// 後始末（Wind と同じ）
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void FoulSprite::DrawGUI()
{

	if (ImGui::CollapsingHeader(u8"フール表示スプライト"))
	{
		ImGui::Checkbox(u8"フール表示", &showFoulSprite);
		ImGui::SliderFloat(u8"最大表示時間", &maxShowTime, 0.1f, 5.0f);
		ImGui::DragFloat2(u8"位置", &foulSprite->position.x, 1.0f, 0.0f, static_cast<float>(Graphics::Instance().GetScreenWidth()));
		ImGui::DragFloat2(u8"サイズ", &foulSprite->size.x, 1.0f, 0.0f, static_cast<float>(Graphics::Instance().GetScreenWidth()));
		ImGui::ColorEdit4(u8"色", &foulSprite->color.x);
		ImGui::DragFloat(u8"フェードイン時間", &fadeInTime, 0.1f, 0.0f, 5.0f);
		ImGui::DragFloat(u8"表示時間", &displayTime, 0.1f, 0.0f, 5.0f);
		ImGui::DragFloat(u8"フェードアウト時間", &fadeOutTime, 0.1f, 0.0f, 5.0f);
	}
}