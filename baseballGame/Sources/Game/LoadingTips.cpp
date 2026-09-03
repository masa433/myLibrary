#include "LoadingTips.h"
#include "Graphics.h"
#include "imgui.h"

void LoadingTips::Initialize(ID3D11Device* device)
{
	//シェーダー
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());


	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	for (int i = 0; i < TIP_COUNT; ++i)
	{
		tipSpriteData[i] = std::make_unique<TipSpriteData>();
		tipSpriteData[i]->texturePath = L".\\resources\\textures\\Tips\\tips" + std::to_wstring(i + 1) + L".png";
		tipSpriteData[i]->position = { 1300.0f, 540.0f }; // Center of the screen
		tipSpriteData[i]->size = { 830.0f, 350.0f }; // Example size
		tipSpriteData[i]->rotation = 0.0f;
		tipSpriteData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		tipSprite[i] = std::make_unique<sprite>(device, context, tipSpriteData[i]->texturePath.c_str());
	}

	//フォントレンダラー初期化
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> codepoints = FontRenderer::Utf8ToCodepoints(
		u8"TIPS");

	tipFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		200.0f, screenWidth, screenHeight, 4096, 4096, &codepoints);

	tipFontPosition = { 400.0f, 540.0f };
	tipFontScale = 1.0f;

	SelectRandomTip();
}

void LoadingTips::Uninitialize()
{
	for (int i = 0; i < TIP_COUNT; ++i)
	{
		tipSprite[i].reset();
		tipSpriteData[i].reset();
	}
}

void LoadingTips::Update(float elapsedTime)
{
	
}

void LoadingTips::Render(float alpha)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	// Set shaders
	context->VSSetShader(spriteVS.Get(), nullptr, 0);
	context->PSSetShader(spritePS.Get(), nullptr, 0);
	context->IASetInputLayout(spriteInputLayout.Get());

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// Render the selected tip
	if (tipSprite[selectedTipIndex])
	{
		tipSprite[selectedTipIndex]->render(context,
			tipSpriteData[selectedTipIndex]->position.x - tipSpriteData[selectedTipIndex]->size.x / 2,
			tipSpriteData[selectedTipIndex]->position.y - tipSpriteData[selectedTipIndex]->size.y / 2,
			tipSpriteData[selectedTipIndex]->size.x,
			tipSpriteData[selectedTipIndex]->size.y,
			tipSpriteData[selectedTipIndex]->color.x,
			tipSpriteData[selectedTipIndex]->color.y,
			tipSpriteData[selectedTipIndex]->color.z,
			alpha, // Use the passed alpha value
			tipSpriteData[selectedTipIndex]->rotation);
	}

	float tipFontWidth, tipFontHeight;
	const char* TipsText = u8"TIPS";

	tipFont.MeasureText(TipsText, tipFontScale, tipFontWidth, tipFontHeight);

	float drawX = tipFontPosition.x - tipFontWidth / 2.0f;
	//float drawY = tipFontPosition.y - tipFontHeight / 2.0f;

	tipFont.DrawTextW(context,
		TipsText,
		drawX,
		tipFontPosition.y,
		tipFontScale, 
		tipFontColor.x, tipFontColor.y, tipFontColor.z, tipFontColor.w * alpha);

	// 描画後の状態をリセット
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	context->IASetInputLayout(nullptr);

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void LoadingTips::SelectRandomTip()
{
	int randomTipIndex = rand() % TIP_COUNT;
	selectedTipIndex = randomTipIndex;
}