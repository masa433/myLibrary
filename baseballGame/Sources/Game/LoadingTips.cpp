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

	tipFontPosition = { 450.0f, 580.0f };
	tipFontScale = 1.3f;

	loadingFont.Initialize(device,
		L".\\resources\\fonts\\Oxanium-Bold.ttf",
		200.0f, screenWidth, screenHeight, 4096, 4096, &codepoints);

	loadingFontPosition = { 1100.0f, 1000.0f };
	loadingFontScale = 0.4f;

	SelectRandomTip();

	loadingArrowSound = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\HoverButton.wav");
	clickArrowSound = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\ClickBatterName.wav");
}

void LoadingTips::Uninitialize()
{
	for (int i = 0; i < TIP_COUNT; ++i)
	{
		tipSprite[i].reset();
		tipSpriteData[i].reset();
	}

	delete loadingArrowSound;
}

void LoadingTips::Update(float elapsedTime)
{
	Input& input = Input::Instance();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

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

	DirectX::XMFLOAT2 scaledRightPos = screenScaler.Scale(rightArrowData->position);
	DirectX::XMFLOAT2 scaledRightSize = screenScaler.ScaleSize(rightArrowData->size);
	DirectX::XMFLOAT2 scaledLeftPos = screenScaler.Scale(leftArrowData->position);
	DirectX::XMFLOAT2 scaledLeftSize = screenScaler.ScaleSize(leftArrowData->size);


	

	//矢印のホバー判定と押下判定
	{
		bool rightHovered =
			input.GetMouse().GetPositionX() >= scaledRightPos.x - scaledRightSize.y / 2.0f &&
			input.GetMouse().GetPositionX() <= scaledRightPos.x + scaledRightSize.y / 2.0f &&
			input.GetMouse().GetPositionY() >= scaledRightPos.y - scaledRightSize.x / 2.0f &&
			input.GetMouse().GetPositionY() <= scaledRightPos.y + scaledRightSize.x / 2.0f;

		bool leftHovered =
			input.GetMouse().GetPositionX() >= scaledLeftPos.x - scaledLeftSize.y / 2.0f &&
			input.GetMouse().GetPositionX() <= scaledLeftPos.x + scaledLeftSize.y / 2.0f &&
			input.GetMouse().GetPositionY() >= scaledLeftPos.y - scaledLeftSize.x / 2.0f &&
			input.GetMouse().GetPositionY() <= scaledLeftPos.y + scaledLeftSize.x / 2.0f;
		bool rightPressed = rightHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT); // 押しっぱなし判定
		bool leftPressed = leftHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT);


	
		rightArrowData->size = rightPressed ? originalArrowSize : (rightHovered ? targetArrowSize : originalArrowSize);
		leftArrowData->size = leftPressed ? originalArrowSize : (leftHovered ? targetArrowSize : originalArrowSize);

		if (rightHovered && !rightPressed && !isArrowHovered)
		{
			
			if(loadingArrowSound)
			{
				loadingArrowSound->PlayOneShot();
				isArrowHovered = true;
			}

		}
		else if (leftHovered && !leftPressed && !isArrowHovered)
		{
			
			if(loadingArrowSound)
			{
				loadingArrowSound->PlayOneShot();
				isArrowHovered = true;
			}
		}

		//矢印をクリックしたら、次のチップを表示する
		if(rightHovered && (input.GetMouse().GetButtonDown() & input.GetMouse().BTN_LEFT))
		{
			ShowNextTip();
			if(clickArrowSound)
			{
				clickArrowSound->PlayOneShot();
			}
		}
		else if(leftHovered && (input.GetMouse().GetButtonDown() & input.GetMouse().BTN_LEFT))
		{
			ShowPreviousTip();
			if(clickArrowSound)
			{
				clickArrowSound->PlayOneShot();
			}
		}

		if(!rightHovered && !leftHovered)
		{
			isArrowHovered = false;
		}
	}

}

void LoadingTips::Render(float alpha)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

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
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(tipSpriteData[selectedTipIndex]->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(tipSpriteData[selectedTipIndex]->size);

		DirectX::XMFLOAT2 centerPosition = {
			scaledPosition.x - scaledSize.x * 0.5f,
			scaledPosition.y - scaledSize.y * 0.5f
		};

		tipSprite[selectedTipIndex]->render(context,
			centerPosition.x,
			centerPosition.y,
			scaledSize.x,
			scaledSize.y,
			tipSpriteData[selectedTipIndex]->color.x,
			tipSpriteData[selectedTipIndex]->color.y,
			tipSpriteData[selectedTipIndex]->color.z,
			alpha,
			tipSpriteData[selectedTipIndex]->rotation);
	}

	if (loadingBallSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(loadingBallSpriteData->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(loadingBallSpriteData->size);

		DirectX::XMFLOAT2 centerPosition = {
			scaledPosition.x - scaledSize.x * 0.5f,
			scaledPosition.y - scaledSize.y * 0.5f
		};

		loadingBallSprite->render(context,
			centerPosition.x,
			centerPosition.y,
			scaledSize.x,
			scaledSize.y,
			loadingBallSpriteData->color.x,
			loadingBallSpriteData->color.y,
			loadingBallSpriteData->color.z,
			alpha,
			angle);
	}

	//矢印の描画
	if (leftArrowSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(leftArrowData->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(leftArrowData->size);

		DirectX::XMFLOAT2 centerPosition = {
			scaledPosition.x - scaledSize.x * 0.5f,
			scaledPosition.y - scaledSize.y * 0.5f
		};

		leftArrowSprite->render(context,
			centerPosition.x,
			centerPosition.y,
			scaledSize.x,
			scaledSize.y,
			leftArrowData->color.x,
			leftArrowData->color.y,
			leftArrowData->color.z,
			alpha,
			leftArrowData->rotation);
	}

	if (rightArrowSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(rightArrowData->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(rightArrowData->size);

		DirectX::XMFLOAT2 centerPosition = {
			scaledPosition.x - scaledSize.x * 0.5f,
			scaledPosition.y - scaledSize.y * 0.5f
		};

		rightArrowSprite->render(context,
			centerPosition.x,
			centerPosition.y,
			scaledSize.x,
			scaledSize.y,
			rightArrowData->color.x,
			rightArrowData->color.y,
			rightArrowData->color.z,
			alpha,
			rightArrowData->rotation);
	}

	float tipFontWidth, tipFontHeight;
	const char* TipsText = u8"TIPS";

	DirectX::XMFLOAT2 scaledTipFontPosition = screenScaler.Scale(tipFontPosition);
	const float scaledTipFontScale = tipFontScale * screenScaler.GetUniformScale();

	tipFont.MeasureText(TipsText, scaledTipFontScale, tipFontWidth, tipFontHeight);

	float drawX = scaledTipFontPosition.x - tipFontWidth / 2.0f;
	//float drawY = scaledTipFontPosition.y - tipFontHeight / 2.0f;

	tipFont.DrawTextW(context,
		TipsText,
		drawX,
		scaledTipFontPosition.y,
		scaledTipFontScale, 
		tipFontColor.x, tipFontColor.y, tipFontColor.z, tipFontColor.w * alpha);

	const char* loadingText[] = {
		u8"Now Loading",
		u8"Now Loading .",
		u8"Now Loading . .",
		u8"Now Loading . . ."
	};

	DirectX::XMFLOAT2 scaledLoadingFontPosition = screenScaler.Scale(loadingFontPosition);
	const float scaledLoadingFontScale = loadingFontScale * screenScaler.GetUniformScale();

	loadingFont.DrawTextW(context,
		loadingText[currentDotCount],
		scaledLoadingFontPosition.x,
		scaledLoadingFontPosition.y,
		scaledLoadingFontScale,
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