#include "LoadingTips.h"
#include "Graphics.h"
#include "imgui.h"
#include "input.h"

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
		tipSpriteData[i]->position = { 1400.0f, 540.0f }; // Center of the screen
		tipSpriteData[i]->size = { 830.0f, 350.0f }; // Example size
		tipSpriteData[i]->rotation = 0.0f;
		tipSpriteData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		tipSprite[i] = std::make_unique<sprite>(device, context, tipSpriteData[i]->texturePath.c_str());
	}
	
	loadingBallSpriteData = std::make_unique<TipSpriteData>();
	loadingBallSpriteData->texturePath = L".\\resources\\textures\\ball.png";
	loadingBallSpriteData->position = { 1800.0f, 990.0f }; 
	loadingBallSpriteData->size = { 50.0f, 50.0f };
	loadingBallSpriteData->rotation = angle;
	loadingBallSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	loadingBallSprite = std::make_unique<sprite>(device, context, loadingBallSpriteData->texturePath.c_str());

	leftArrowData = std::make_unique<TipSpriteData>();
	leftArrowData->texturePath = L".\\resources\\textures\\arrow.png";
	leftArrowData->position = { 900.0f, 500.0f };
	leftArrowData->size = { 100.0f, 50.0f };
	leftArrowData->rotation = -90.0f;
	leftArrowData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	leftArrowSprite = std::make_unique<sprite>(device, context, leftArrowData->texturePath.c_str());

	rightArrowData = std::make_unique<TipSpriteData>();
	rightArrowData->texturePath = L".\\resources\\textures\\arrow.png";
	rightArrowData->position = { 1850.0f, 500.0f };
	rightArrowData->size = { 100.0f, 50.0f };
	rightArrowData->rotation = 90.0f;
	rightArrowData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	rightArrowSprite = std::make_unique<sprite>(device, context, rightArrowData->texturePath.c_str());

	//フォントレンダラー初期化
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> codepoints = FontRenderer::Utf8ToCodepoints(
		u8"TIPS"
	u8"Now Loading.");

	tipFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		200.0f, screenWidth, screenHeight, 4096, 4096, &codepoints);

	tipFontPosition = { 500.0f, 580.0f };
	tipFontScale = 1.3f;

	loadingFont.Initialize(device,
		L".\\resources\\fonts\\Oxanium-Bold.ttf",
		200.0f, screenWidth, screenHeight, 4096, 4096, &codepoints);

	loadingFontPosition = { 1100.0f, 1000.0f };
	loadingFontScale = 0.4f;

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
	Input& input = Input::Instance();

	//ドットアニメーション
	dotAnimationTime += elapsedTime;
	if(dotAnimationTime >= dotAnimationInterval)
	{
		dotAnimationTime = 0.0f;
		currentDotCount = (currentDotCount + 1) % 4; // 0, 1, 2, 3の順にループ
	}

	//ボールローテーションアニメーション
	constexpr float speed = 180.0f;
	angle += speed * elapsedTime; // Adjust the speed as needed


	//矢印のホバー判定と押下判定
	{
		bool rightHovered =
			input.GetMouse().GetPositionX() >= rightArrowData->position.x - originalArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionX() <= rightArrowData->position.x + originalArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionY() >= rightArrowData->position.y - originalArrowSize.y / 2.0f &&
			input.GetMouse().GetPositionY() <= rightArrowData->position.y + originalArrowSize.y / 2.0f;

		bool leftHovered =
			input.GetMouse().GetPositionX() >= leftArrowData->position.x - originalArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionX() <= leftArrowData->position.x + originalArrowSize.x / 2.0f &&
			input.GetMouse().GetPositionY() >= leftArrowData->position.y - originalArrowSize.y / 2.0f &&
			input.GetMouse().GetPositionY() <= leftArrowData->position.y + originalArrowSize.y / 2.0f;
		bool rightPressed = rightHovered && input.GetMouse().GetButton(); // 押しっぱなし判定
		bool leftPressed = leftHovered && input.GetMouse().GetButton();


		rightArrowData->size = rightPressed ? originalArrowSize : (rightHovered ? targetArrowSize : originalArrowSize);
		leftArrowData->size = leftPressed ? originalArrowSize : (leftHovered ? targetArrowSize : originalArrowSize);

		if (rightHovered && input.GetMouse().GetButtonDown())
		{
			ShowNextTip();
		}
		else if (leftHovered && input.GetMouse().GetButtonDown())
		{
			ShowPreviousTip();
		}
	}

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

	context->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

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
			alpha,
			tipSpriteData[selectedTipIndex]->rotation);
	}

	if (loadingBallSprite)
	{
		loadingBallSprite->render(context,
			loadingBallSpriteData->position.x - loadingBallSpriteData->size.x / 2,
			loadingBallSpriteData->position.y - loadingBallSpriteData->size.y / 2,
			loadingBallSpriteData->size.x,
			loadingBallSpriteData->size.y,
			loadingBallSpriteData->color.x,
			loadingBallSpriteData->color.y,
			loadingBallSpriteData->color.z,
			alpha,
			angle);
	}

	//矢印の描画
	if (leftArrowSprite)
	{
		leftArrowSprite->render(context,
			leftArrowData->position.x - leftArrowData->size.x / 2,
			leftArrowData->position.y - leftArrowData->size.y / 2,
			leftArrowData->size.x,
			leftArrowData->size.y,
			leftArrowData->color.x,
			leftArrowData->color.y,
			leftArrowData->color.z,
			alpha,
			leftArrowData->rotation);
	}

	if (rightArrowSprite)
	{
		rightArrowSprite->render(context,
			rightArrowData->position.x - rightArrowData->size.x / 2,
			rightArrowData->position.y - rightArrowData->size.y / 2,
			rightArrowData->size.x,
			rightArrowData->size.y,
			rightArrowData->color.x,
			rightArrowData->color.y,
			rightArrowData->color.z,
			alpha,
			rightArrowData->rotation);
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

	const char* loadingText[] = {
		u8"Now Loading",
		u8"Now Loading .",
		u8"Now Loading . .",
		u8"Now Loading . . ."
	};

	loadingFont.DrawTextW(context,
		loadingText[currentDotCount],
		loadingFontPosition.x,
		loadingFontPosition.y,
		loadingFontScale,
		loadingFontColor.x, loadingFontColor.y, loadingFontColor.z, loadingFontColor.w * alpha);

	context->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);

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

void LoadingTips::ShowNextTip()
{
	selectedTipIndex = (selectedTipIndex + 1) % TIP_COUNT;
}

void LoadingTips::ShowPreviousTip()
{
	selectedTipIndex = (selectedTipIndex - 1 + TIP_COUNT) % TIP_COUNT;
}