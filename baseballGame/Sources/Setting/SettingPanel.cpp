#include "pch.h"
#include "shader.h"
#include "SettingPanel.h"
#include "Graphics.h"
#include "ScreenScaler.h"
#include "input.h"
#include "Pitcher.h"
#include "GameModeManager.h"
#include "Audience.h"

void AudioSettingPanel::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (!device || !context) return;
	
	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	//テクスチャの読み込み
	for (int i = 0; i < numVolumeTypes; ++i)
	{
		volumeSliderBackgroundData[i] = std::make_unique<SpriteData>();
		volumeSliderBackgroundData[i]->texturePath = L".\\resources\\textures\\scrollViewBack.png";
		volumeSliderBackgroundData[i]->position = { volumeSliderBackgroundPosition[i].x, volumeSliderBackgroundPosition[i].y };
		volumeSliderBackgroundData[i]->size = { 500.0f, 20.0f };
		volumeSliderBackgroundData[i]->rotation = 0.0f;
		volumeSliderBackgroundData[i]->color = { 0.0f, 0.0f, 0.0f, 1.0f };
		volumeSliderBackgroundSprite[i] = std::make_unique<sprite>(device, context, volumeSliderBackgroundData[i]->texturePath.c_str());

		if(!volumeSliderBackgroundSprite[i] || !volumeSliderBackgroundSprite[i]->IsValid())
		{
			MessageBoxW(nullptr, L"オーディオ背景のテクスチャが読み込めませんでした", L"Error", MB_OK | MB_ICONERROR);

			volumeSliderBackgroundSprite[i].reset();//リソースの解放
		}
	}

	//スライダーボタンのテクスチャの読み込み
	for (int i = 0; i < numVolumeTypes; ++i)
	{
		volumeSliderButtonData[i] = std::make_unique<SpriteData>();
		volumeSliderButtonData[i]->texturePath = L".\\resources\\textures\\Ball.png";
		//ボタンの初期位置をスライダーの右端に設定
		
		volumeSliderButtonData[i]->size = { sliderButtonSize[i].x, sliderButtonSize[i].y };
		volumeSliderButtonData[i]->rotation = 0.0f;
		volumeSliderButtonData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		volumeSliderButtonSprite[i] = std::make_unique<sprite>(device, context, volumeSliderButtonData[i]->texturePath.c_str());

		float minX = volumeSliderBackgroundPosition[i].x - volumeSliderBackgroundData[i]->size.x / 2.0f;
		float maxX = volumeSliderBackgroundPosition[i].x + volumeSliderBackgroundData[i]->size.x / 2.0f;

		// スライダーのボタンの初期位置を設定
		float volumeValue = 1.0f; // 初期ボリューム値を1.0fに設定

		switch (i)
		{
			case 0:
				volumeValue = Audio::Instance().GetMasterVolume();
				break;
			case 1:
				volumeValue = Audio::Instance().GetBGMVolume();
				break;
			case 2:
				volumeValue = Audio::Instance().GetSEVolume();
				break;
			default:
				break;
		}

		// スライダーのボタンの初期位置を計算
		savedButtonPosX[i] = std::clamp(minX + volumeValue * (maxX - minX), minX, maxX);
		volumeSliderButtonPosition[i].x = savedButtonPosX[i];

		volumeSliderButtonData[i]->position = { volumeSliderButtonPosition[i].x, volumeSliderBackgroundPosition[i].y };
		
		if(!volumeSliderButtonSprite[i] || !volumeSliderButtonSprite[i]->IsValid())
		{
			MessageBoxW(nullptr, L"オーディオスライダーボタンのテクスチャが読み込めませんでした", L"Error", MB_OK | MB_ICONERROR);
			volumeSliderButtonSprite[i].reset();//リソースの解放
		}

	}



	// フォントレンダラーの初期化
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"全体BGMSE"
		u8"0123456789"
	);

	// フォントレンダラーの初期化
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	masterVolumeFontData.position = { 600.0f, 550.0f };
	masterVolumeFontData.size = 0.5f;

	bgmVolumeFontData.position = { 600.0f, 680.0f };
	bgmVolumeFontData.size = 0.5f;

	seVolumeFontData.position = { 600.0f, 810.0f };
	seVolumeFontData.size = 0.5f;

	for(int i = 0; i < numVolumeTypes; ++i)
	{
		volumeValueFontData[i].position = { masterVolumeFontData.position.x, masterVolumeFontData.position.y + i * 130.0f };
		volumeValueFontData[i].size = 0.5f;
	}
	
	hoverSound = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\HoverButton.wav", SoundType::SE);

}

void AudioSettingPanel::Uninitialize()
{
	// リソースの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
	fontRenderer.Uninitialize();
	masterVolumeFontData = {};
	bgmVolumeFontData = {};
	seVolumeFontData = {};
	for (int i = 0; i < 3; ++i)
	{
		volumeSliderBackgroundData[i].reset();
		volumeSliderBackgroundSprite[i].reset();
		volumeSliderButtonData[i].reset();
		volumeSliderButtonSprite[i].reset();
	}

	delete hoverSound;

}
void AudioSettingPanel::Update(float elapsedTime)
{
	if (hoverSound) hoverSound->Update();

	Input& input = Input::Instance();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	// スライダーのドラッグ処理
	// マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY());

	int previousHoveredButtonIndex = hoveredButtonIndex;
	hoveredButtonIndex = -1; // 初期化

	for (int i = 0; i < numVolumeTypes; ++i)
	{
		// スライダーのボタンの位置を取得
		DirectX::XMFLOAT2 scaledButtonPos = screenScaler.Scale(volumeSliderButtonPosition[i]);
		DirectX::XMFLOAT2 scaledButtonSize = screenScaler.ScaleSize(sliderButtonSize[i]);
		DirectX::XMFLOAT2 scaledButtonCenterPos = { scaledButtonPos.x - scaledButtonSize.x / 2.0f, scaledButtonPos.y - scaledButtonSize.y / 2.0f };

		// 間隔調整の変数
		float sliderSpacing = 130.0f * screenScaler.GetUniformScale();

		float offsetX = scaledButtonCenterPos.x + volumeSliderButtonOffset.x * screenScaler.GetUniformScale();
		float offsetY = scaledButtonCenterPos.y + i * sliderSpacing + volumeSliderButtonOffset.y * screenScaler.GetUniformScale();
		float buttonWidth = scaledButtonSize.x;
		float buttonHeight = scaledButtonSize.y;

		bool isHovered = (mousePos.x >= offsetX && mousePos.x <= offsetX + buttonWidth &&
			mousePos.y >= offsetY && mousePos.y <= offsetY + buttonHeight);

		bool isPressed = (input.GetMouse().GetButton() & Mouse::BTN_LEFT);// 左クリックが押されているかどうか

		sliderButtonSize[i] = isPressed ? sliderButtonOriginalSize[i] : (isHovered ? sliderButtonHoverSize[i] : sliderButtonOriginalSize[i]	);

		if(isHovered)
		{
			hoveredButtonIndex = i;

			if (previousHoveredButtonIndex != i && hoverSound)
			{
				hoverSound->PlayOneShot();
			}
		}

		

		if(!isPressed)
		{
			isDragging = false;
		}
		else if(isHovered && isPressed)
		{
			isDragging = true;
			hoveredButtonIndex = i;

		}
		if(isDragging && hoveredButtonIndex == i)
		{
			volumeSliderButtonPosition[i].x = mousePos.x - volumeSliderButtonOffset.x;

			float minX = volumeSliderBackgroundPosition[i].x - volumeSliderBackgroundData[i]->size.x / 2.0f;
			float maxX = volumeSliderBackgroundPosition[i].x + volumeSliderBackgroundData[i]->size.x / 2.0f;

			volumeSliderButtonPosition[i].x = std::clamp(volumeSliderButtonPosition[i].x, minX, maxX);

			savedButtonPosX[i] = volumeSliderButtonPosition[i].x;

			// ボリュームの値を計算
			float volumeValue = (volumeSliderButtonPosition[i].x - minX) / (maxX - minX);//最小値から最大値までの範囲で正規化された値を計算

			volumeValue /= 1.0f; // 0.0fから1.0fの範囲に正規化

			switch(i)
			{
			case 0:
				Audio::Instance().SetMasterVolume(volumeValue);
				break;
			case 1:
				Audio::Instance().SetBGMVolume(volumeValue);
				break;
			case 2:
				Audio::Instance().SetSEVolume(volumeValue);
				break;
			}

		}
	}
}

void AudioSettingPanel::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// テクスチャの描画	
	for(int i  = 0; i < numVolumeTypes; ++i)
	{
		if(volumeSliderBackgroundData[i] && volumeSliderBackgroundSprite[i])
		{
			DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(volumeSliderBackgroundPosition[i]);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(volumeSliderBackgroundData[i]->size);

			DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };

			//間隔調整の変数
			float sliderSpacing = 130.0f * screenScaler.GetUniformScale();

			//3回描画することで、マスターボリューム、BGMボリューム、SEボリュームのスライダー背景を描画
			DirectX::XMFLOAT2 offsetPos = { scaledCenterPos.x + volumeSliderOffset.x * screenScaler.GetUniformScale(), scaledCenterPos.y + i * sliderSpacing + volumeSliderOffset.y * screenScaler.GetUniformScale() };

			volumeSliderBackgroundSprite[i]->render(dc,
				offsetPos.x, offsetPos.y,
				scaledSize.x, scaledSize.y,
				volumeSliderBackgroundData[i]->color.x,
				volumeSliderBackgroundData[i]->color.y,
				volumeSliderBackgroundData[i]->color.z,
				volumeSliderBackgroundData[i]->color.w,
				volumeSliderBackgroundData[i]->rotation);
		}
	}

	

	// スライダーボタンの描画	
	for(int i = 0; i < numVolumeTypes; ++i)
	{
		if (volumeSliderButtonSprite[i] && volumeSliderButtonData[i])
		{
		DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(volumeSliderButtonPosition[i]);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(sliderButtonSize[i]);
		DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };
		//間隔調整の変数
		float sliderSpacing = 130.0f * screenScaler.GetUniformScale();

		DirectX::XMFLOAT2 offsetPos = { scaledCenterPos.x + volumeSliderButtonOffset.x * screenScaler.GetUniformScale(), scaledCenterPos.y + i * sliderSpacing + volumeSliderButtonOffset.y * screenScaler.GetUniformScale() };

		volumeSliderButtonSprite[i]->render(dc,
			offsetPos.x, offsetPos.y,
			scaledSize.x, scaledSize.y,
			volumeSliderButtonData[i]->color.x,
			volumeSliderButtonData[i]->color.y,
			volumeSliderButtonData[i]->color.z,
			volumeSliderButtonData[i]->color.w,
			volumeSliderButtonData[i]->rotation);
		}
	}
		
	

	// フォントの描画
	//マスターボリュームの表示
	DirectX::XMFLOAT2 scaledFontPos = screenScaler.Scale(masterVolumeFontData.position);
	const float scaledFontScale = masterVolumeFontData.size * screenScaler.GetUniformScale();

	float textWidth, textHeight;
	const char* text = u8"全体";
	fontRenderer.MeasureText(text,scaledFontScale, textWidth, textHeight);
	float drawPosX = scaledFontPos.x - textWidth / 2.0f;
	float drawPosY = scaledFontPos.y - textHeight / 2.0f;

	fontRenderer.DrawTextW(dc, text, drawPosX, drawPosY, scaledFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	//BGMボリュームの表示
	DirectX::XMFLOAT2 scaledBgmFontPos = screenScaler.Scale(bgmVolumeFontData.position);
	const float scaledBgmFontScale = bgmVolumeFontData.size * screenScaler.GetUniformScale();

	float bgmTextWidth, bgmTextHeight;
	const char* bgmText = u8"BGM";
	fontRenderer.MeasureText(bgmText, scaledBgmFontScale, bgmTextWidth, bgmTextHeight);
	float bgmDrawPosX = scaledBgmFontPos.x - bgmTextWidth / 2.0f;
	float bgmDrawPosY = scaledBgmFontPos.y - bgmTextHeight / 2.0f;

	fontRenderer.DrawTextW(dc, bgmText, bgmDrawPosX, bgmDrawPosY, scaledBgmFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	//SEボリュームの表示
	DirectX::XMFLOAT2 scaledSeFontPos = screenScaler.Scale(seVolumeFontData.position);
	const float scaledSeFontScale = seVolumeFontData.size * screenScaler.GetUniformScale();

	float seTextWidth, seTextHeight;
	const char* seText = u8"SE";
	fontRenderer.MeasureText(seText, scaledSeFontScale, seTextWidth, seTextHeight);
	float seDrawPosX = scaledSeFontPos.x - seTextWidth / 2.0f;
	float seDrawPosY = scaledSeFontPos.y - seTextHeight / 2.0f;

	fontRenderer.DrawTextW(dc, seText, seDrawPosX, seDrawPosY, scaledSeFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	// ボリューム値の表示
	for(int i = 0; i < numVolumeTypes; ++i)
	{
		DirectX::XMFLOAT2 scaledVolumeFontPos = screenScaler.Scale(volumeValueFontData[i].position);
		const float scaledVolumeFontScale = volumeValueFontData[i].size * screenScaler.GetUniformScale();

		//オフセットもスケーリング
		const float scaledOffsetX = volumeValueOffset[i].x * screenScaler.GetUniformScale();
		const float scaledOffsetY = volumeValueOffset[i].y * screenScaler.GetUniformScale();

		float volumeValue = 0.0f;
		switch(i)
		{
		case 0:
			volumeValue = Audio::Instance().GetMasterVolume();
			break;
		case 1:
			volumeValue = Audio::Instance().GetBGMVolume();
			break;
		case 2:
			volumeValue = Audio::Instance().GetSEVolume();
			break;
		}
		char volumeText[16];
		sprintf_s(volumeText, "%.f", volumeValue * 100.0f);
		float volumeTextWidth, volumeTextHeight;
		fontRenderer.MeasureText(volumeText, scaledVolumeFontScale, volumeTextWidth, volumeTextHeight);
		float volumeDrawPosX = scaledVolumeFontPos.x - volumeTextWidth / 2.0f;
		float volumeDrawPosY = scaledVolumeFontPos.y - volumeTextHeight / 2.0f;
		fontRenderer.DrawTextW(dc, volumeText, volumeDrawPosX + scaledOffsetX, volumeDrawPosY + scaledOffsetY, scaledVolumeFontScale, 1.0f, 1.0f, 1.0f, 1.0f);
	}


	// 描画後の状態をリセット
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void AudioSettingPanel::DrawGUI()
{
	// ImGuiを使ったGUI描画処理
	ImGui::Begin("Audio Settings");
	ImGui::DragFloat2("slider back offset", &volumeSliderOffset.x, 1.0f, -1000.0f, 1000.0f);
	ImGui::End();
}


void CursorSettingPanel::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (!device || !context) return;
	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	//テクスチャの読み込み
	sliderBackgroundData = std::make_unique<SpriteData>();
	sliderBackgroundData->texturePath = L".\\resources\\textures\\scrollViewBack.png";
	sliderBackgroundData->position = { sliderBackgroundPosition.x, sliderBackgroundPosition.y };
	sliderBackgroundData->size = { 500.0f, 20.0f };
	sliderBackgroundData->color = { 0.0f, 0.0f, 0.0f, 1.0f };
	sliderBackgroundData->rotation = 0.0f;
	sliderBackgroundSprite = std::make_unique<sprite>(device, context, sliderBackgroundData->texturePath.c_str());

	if(!sliderBackgroundSprite || !sliderBackgroundSprite->IsValid())
	{
		MessageBoxW(nullptr, L"カーソル背景のテクスチャが読み込めませんでした", L"Error", MB_OK | MB_ICONERROR);
		sliderBackgroundSprite.reset();//リソースの解放
	}

	sliderButtonData = std::make_unique<SpriteData>();
	sliderButtonData->texturePath = L".\\resources\\textures\\Ball.png";
	sliderButtonData->position = { sliderButtonPosition.x, sliderButtonPosition.y };
	sliderButtonData->size = { sliderButtonSize.x, sliderButtonSize.y };
	sliderButtonData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	sliderButtonData->rotation = 0.0f;
	sliderButtonSprite = std::make_unique<sprite>(device, context, sliderButtonData->texturePath.c_str());

	if(!sliderButtonSprite || !sliderButtonSprite->IsValid())
	{
		MessageBoxW(nullptr, L"カーソルスライダーボタンのテクスチャが読み込めませんでした", L"Error", MB_OK | MB_ICONERROR);
		sliderButtonSprite.reset();//リソースの解放
	}

	// フォントレンダラーの初期化
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"バットカーソル感度"
		u8"打撃後のゲーム速度操作方法表示"
		u8"観客の表示/非表示"
		u8"1234567890"
	);

	// フォントレンダラーの初期化
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	// フォントデータの設定
	cursorFontData.position = { 600.0f, 600.0f };
	cursorFontData.size = 0.3f;

	cursorValueFontData.position = {cursorFontData.position.x, cursorFontData.position.y};
	cursorValueFontData.size = 0.5f;

	//保存されているスライダーボタンのインデックスに応じて現在のスライダーの位置を設定
	currentSliderPosition = static_cast<SliderPosition>(currentSliderButtonIndex);

	hoverSound = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\HoverButton.wav", SoundType::SE);
}

void CursorSettingPanel::Uninitialize()
{
	// リソースの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
	fontRenderer.Uninitialize();
	cursorFontData = {};
	cursorValueFontData = {};
	sliderBackgroundData.reset();
	sliderBackgroundSprite.reset();
	sliderButtonData.reset();
	sliderButtonSprite.reset();
	delete hoverSound;
	hoverSound = nullptr;
}

void CursorSettingPanel::Update(float elapsedTime)
{
	// 設定パネルの更新処理
	//スライダーボタンをマウスでドラッグして動かす処理
	// マウスの位置を取得
	Input& input = Input::Instance();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2( input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY() );


	//スライダーボタンのスケーリング
	DirectX::XMFLOAT2 scaledButtonPos = screenScaler.Scale(sliderButtonPosition);
	DirectX::XMFLOAT2 scaledButtonSize = screenScaler.ScaleSize(sliderButtonSize);
	DirectX::XMFLOAT2 scaledCenterPos = { scaledButtonPos.x - scaledButtonSize.x / 2.0f, scaledButtonPos.y - scaledButtonSize.y / 2.0f };

	// スライダーボタンの矩形範囲を計算
	float buttonX = scaledCenterPos.x + offsetX * screenScaler.GetUniformScale();
	float buttonY = scaledCenterPos.y + offsetY * screenScaler.GetUniformScale();
	float buttonWidth = scaledButtonSize.x;
	float buttonHeight = scaledButtonSize.y;

	// マウスがスライダーボタンの範囲内にあるかどうかを判定
	bool isHover = (mousePos.x >= buttonX && mousePos.x <= buttonX + buttonWidth &&
		mousePos.y >= buttonY && mousePos.y <= buttonY + buttonHeight);
	bool isPressed = (input.GetMouse().GetButton() & Mouse::BTN_LEFT);// 左クリックが押されているかどうか
	
	sliderButtonSize = isPressed ? sliderButtonOriginalSize : (isHover ? sliderButtonHoverSize : sliderButtonOriginalSize);


	if (isHover)
	{
		if (hoverSound && !isHoverSoundPlayed)
		{
			hoverSound->PlayOneShot();
			isHoverSoundPlayed = true;
		}
	}
	else
	{
		isHoverSoundPlayed = false;
	}

	//スライダーボタンがクリックされた瞬間にドラッグ状態を開始
	if(!isPressed)
	{
		isDragging = false;
	}
	
	else if(isHover && isPressed)
	{
		isDragging = true;
	}
	//長押し中はホバーしていなくても動かせるようにする
	if (isDragging)
	{
		// スライダーボタンのX座標をマウスのX座標に合わせる
		sliderButtonPosition.x = mousePos.x - offsetX * screenScaler.GetUniformScale();
		// スライダーボタンの位置をスライダー背景の範囲内に制限
		float minX = sliderBackgroundPosition.x - sliderBackgroundData->size.x / 2.0f;
		float maxX = sliderBackgroundPosition.x + sliderBackgroundData->size.x / 2.0f;
		// スライダーボタンの位置を制限(スライダーの背景の左端から右端までの範囲内に収める)
		//右端を超えたら右端に固定、左端を超えたら左端に固定
		sliderButtonPosition.x = std::max(minX, std::min(maxX, sliderButtonPosition.x));
		
		//5分割して、スライダーボタンの位置を制限する
		int segments = numSliderPositions; // ステップ数に応じた分割数(今回は4箇所)
		int segmentSize = sliderBackgroundData->size.x / segments; // 各分割の幅

		
		// スライダーボタンの位置を分割の中心にスナップさせる
		for(int i = 0; i <= segments; ++i)
		{
			
			pointX = minX + (segmentSize * i);// 分割の中心点のX座標
			int harfSegmentSize = segmentSize / 2;// 分割の半分の幅

			// スライダーボタンの位置が分割の中心点に近い場合、スナップさせる
			if(std::abs(sliderButtonPosition.x - pointX) <= harfSegmentSize)
			{
				sliderButtonPosition.x = pointX;// スナップさせる
				currentSliderPosition = static_cast<SliderPosition>(i);// スライダーボタンの位置に応じて現在のスライダーの位置を更新
				currentSliderButtonIndex = i;// スライダーボタンのインデックスを更新
				break;// 近い分割の中心点にスナップさせたらループを抜ける
			}

		}

		//スライダーボタンの位置に応じてカーソル感度を設定する

		switch (currentSliderPosition)
		{
			case SliderPosition::Low:
			{
				//カーソル感度を0.5に設定
				input.GetMouse().SetMouseSensitivity(0.5f);
				break;
			}
			case SliderPosition::Medium:
			{
				//カーソル感度を0.75に設定
				input.GetMouse().SetMouseSensitivity(0.75f);
				break;
			}
			case SliderPosition::Normal:
			{
				//カーソル感度を1.0に設定
				input.GetMouse().SetMouseSensitivity(1.0f);
				break;
			}
			case SliderPosition::MediumHigh:
			{
				//カーソル感度を1.25に設定
				input.GetMouse().SetMouseSensitivity(1.25f);
				break;
			}
			case SliderPosition::High:
			{
				//カーソル感度を1.5に設定
				input.GetMouse().SetMouseSensitivity(1.5f);
				break;
			}
		}
		
	}
}

void CursorSettingPanel::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// スライダー背景の描画
	if(sliderBackgroundData && sliderBackgroundSprite)
	{
		DirectX::XMFLOAT2 scaledSliderPos = screenScaler.Scale(sliderBackgroundPosition);
		DirectX::XMFLOAT2 scaledSliderSize = screenScaler.ScaleSize(sliderBackgroundData->size);
		DirectX::XMFLOAT2 scaledCenterPos = { scaledSliderPos.x - scaledSliderSize.x / 2.0f, scaledSliderPos.y - scaledSliderSize.y / 2.0f };

		sliderBackgroundSprite->render(dc,
			scaledCenterPos.x + offsetX * screenScaler.GetUniformScale(), scaledCenterPos.y + offsetY * screenScaler.GetUniformScale(),
			scaledSliderSize.x, scaledSliderSize.y,
			sliderBackgroundData->color.x, sliderBackgroundData->color.y, sliderBackgroundData->color.z, sliderBackgroundData->color.w,
			sliderBackgroundData->rotation);
	}

	// スライダーボタンの描画
	if(sliderButtonData && sliderButtonSprite)
	{
		DirectX::XMFLOAT2 scaledButtonPos = screenScaler.Scale(sliderButtonPosition);
		DirectX::XMFLOAT2 scaledButtonSize = screenScaler.ScaleSize(sliderButtonSize);
		DirectX::XMFLOAT2 scaledCenterPos = { scaledButtonPos.x - scaledButtonSize.x / 2.0f, scaledButtonPos.y - scaledButtonSize.y / 2.0f };
		sliderButtonSprite->render(dc,
			scaledCenterPos.x + offsetX * screenScaler.GetUniformScale(), scaledCenterPos.y + offsetY * screenScaler.GetUniformScale(),
			scaledButtonSize.x, scaledButtonSize.y,
			sliderButtonData->color.x, sliderButtonData->color.y, sliderButtonData->color.z, sliderButtonData->color.w,
			sliderButtonData->rotation);
	}

	
	// フォントの描画
	//カーソル感度の表示
	DirectX::XMFLOAT2 scaledFontPos = screenScaler.Scale(cursorFontData.position);
	const float scaledFontScale = cursorFontData.size * screenScaler.GetUniformScale();

	float textWidth, textHeight;
	const char* text = u8"バットカーソル感度";
	fontRenderer.MeasureText(text, scaledFontScale, textWidth, textHeight);
	float drawPosX = scaledFontPos.x - textWidth / 2.0f;
	float drawPosY = scaledFontPos.y - textHeight / 2.0f;

	fontRenderer.DrawTextW(dc, text, drawPosX, drawPosY, scaledFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	//カーソル感度の値の表示
	DirectX::XMFLOAT2 scaledValueFontPos = screenScaler.Scale(cursorValueFontData.position);
	const float scaledValueFontScale = cursorValueFontData.size * screenScaler.GetUniformScale();
	
	DirectX::XMFLOAT2 scaledOffsetPos = screenScaler.Scale(cursorValueOffset);

	float valueTextWidth, valueTextHeight;
	const char* valueText = nullptr;

	switch(currentSliderPosition)
	{
		case SliderPosition::Low:
			valueText = u8"1";
			break;
		case SliderPosition::Medium:
			valueText = u8"2";
			break;
		case SliderPosition::Normal:
			valueText = u8"3";
			break;
		case SliderPosition::MediumHigh:
			valueText = u8"4";
			break;
		case SliderPosition::High:
			valueText = u8"5";
			break;
	}

	fontRenderer.MeasureText(valueText, scaledValueFontScale, valueTextWidth, valueTextHeight);
	float valueDrawPosX = scaledValueFontPos.x - valueTextWidth / 2.0f;
	float valueDrawPosY = scaledValueFontPos.y - valueTextHeight / 2.0f;

	fontRenderer.DrawTextW(dc, valueText, 
		valueDrawPosX + scaledOffsetPos.x, valueDrawPosY + scaledOffsetPos.y,
		scaledValueFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	// 描画後の状態をリセット
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void CursorSettingPanel::DrawGUI()
{
	// ImGuiを使ったGUI描画処理
	ImGui::Begin("Cursor Settings");
	// カーソル感度のスライダー
	static float cursorSensitivity = 1.0f;
	if (ImGui::SliderFloat("Cursor Sensitivity", &cursorSensitivity, 0.1f, 10.0f))
	{
		// カーソル感度の変更処理
	}

	ImGui::DragFloat("offsetX", &offsetX, 1.0f, -100.0f, 100.0f);
	ImGui::DragFloat("offsetY", &offsetY, 1.0f, -100.0f, 100.0f);
	ImGui::DragFloat2("valueOffset", &cursorValueOffset.x, 1.0f, 100.0f, 1000.0f);

	ImGui::End();
}

void OtherSettingPanel::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (!device || !context) return;
	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	//テクスチャの読み込み
	for (int i = 0; i < numCheckBoxes; ++i)
	{
		showOperationCheckboxData[i] = std::make_unique<SpriteData>();
		showOperationCheckboxData[i]->texturePath = L".\\resources\\textures\\checkBoxFrame.png";
		showOperationCheckboxData[i]->position = checkboxPosition[i];
		showOperationCheckboxData[i]->size = checkboxSize[i];
		showOperationCheckboxData[i]->rotation = 0.0f;
		showOperationCheckboxData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		showOperationCheckboxSprite[i] = std::make_unique<sprite>(device, context, showOperationCheckboxData[i]->texturePath.c_str());

		checkIconData[i] = std::make_unique<SpriteData>();
		checkIconData[i]->texturePath = L".\\resources\\textures\\checkIcon.png";
		checkIconData[i]->position = checkIconPosition[i];
		checkIconData[i]->size = checkIconSize[i];
		checkIconData[i]->rotation = 0.0f;
		checkIconData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		checkIconSprite[i] = std::make_unique<sprite>(device, context, checkIconData[i]->texturePath.c_str());
	}
	
	// フォントレンダラーの初期化
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"打撃後のゲーム速度操作方法表示"
		u8"通常中速高速"
		u8"観客の表示"
	);

	// フォントレンダラーの初期化
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	// フォントデータの設定
	gameSpeedFontData.position = { 600.0f, 500.0f };
	gameSpeedFontData.size = 0.3f;

	showOperationFontData.position = { 600.0f, 650.0f };
	showOperationFontData.size = 0.3f;

	showAudienceFontData.position = { 600.0f, 800.0f };
	showAudienceFontData.size = 0.3f;

	for (int i = 0; i < numGameSpeedTypes; ++i)
	{
		gameSpeedTypeFontData[i].position = { gameSpeedFontData.position.x + gameSpeedOffsetX[i], gameSpeedFontData.position.y - gameSpeedOffsetY };
		gameSpeedTypeFontData[i].size = 0.7f;
		gameSpeedTypeFontData[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	// 初期状態では、クリックされたチェックボックスはないため、clickedCheckboxIndexを-1に設定

	//クリックされたときの色を設定
	gameSpeedTypeFontData[clickedCheckboxIndex].color = { 1.0f, 1.0f, 0.0f, 1.0f };

	// チェックボックスの位置をフォントの右側に配置するためのオフセットを計算
	for (int i = 0; i < numCheckBoxes; ++i)
	{
		checkboxPosition[i] = { showOperationFontData.position.x + offsetX, showOperationFontData.position.y + offsetY };
		checkIconPosition[i] = { checkboxPosition[i].x, checkboxPosition[i].y };

	}

	hoverSound = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\HoverButton.wav", SoundType::SE);
	clickSound = Audio::Instance().LoadAudioSource(".\\resources\\sounds\\SE\\Clog.wav", SoundType::SE);
}

void OtherSettingPanel::Uninitialize()
{
	// リソースの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
	fontRenderer.Uninitialize();
	showOperationFontData = {};
	gameSpeedFontData = {};
	showAudienceFontData = {};
	for(int i = 0; i < numGameSpeedTypes; ++i)
	{
		gameSpeedTypeFontData[i] = {};
	}

	// チェックボックスとチェックアイコンの解放
	for (int i = 0; i < numCheckBoxes; ++i)
	{
		showOperationCheckboxData[i].reset();
		showOperationCheckboxSprite[i].reset();
		checkIconData[i].reset();
		checkIconSprite[i].reset();
	}
}

void OtherSettingPanel::Update(float elapsedTime)
{
	if (hoverSound) hoverSound->Update();
	if (clickSound) clickSound->Update();

	// 設定パネルの更新処理
	UpdateShowOperationCheckbox();

	UpdateGameSpeedDisplay();
}

void OtherSettingPanel::UpdateShowOperationCheckbox()
{

	Input& input = Input::Instance();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	//マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY());

	bool isClicked = input.GetMouse().GetButtonDown() & Mouse::BTN_LEFT;
	int prevHoveredCheckBoxIndex = hoveredCheckboxIndex;
	hoveredCheckboxIndex = -1; // 初期化

	for (int i = 0; i < numCheckBoxes; ++i)
	{
		//チェックボックスの位置とサイズをスケーリング
		DirectX::XMFLOAT2 scaledCheckboxPos = screenScaler.Scale(checkboxPosition[i]);
		DirectX::XMFLOAT2 scaledCheckboxSize = screenScaler.ScaleSize(checkboxSize[i]);
		DirectX::XMFLOAT2 scaledCenterPos = { scaledCheckboxPos.x - scaledCheckboxSize.x / 2.0f, scaledCheckboxPos.y - scaledCheckboxSize.y / 2.0f };

		// 間隔調整の変数
		float sliderSpacing = 140.0f * screenScaler.GetUniformScale();

		//チェックボックスの範囲を計算
		float checkBoxX = scaledCenterPos.x + offsetX * screenScaler.GetUniformScale();
		float checkBoxY = scaledCenterPos.y + i * sliderSpacing + offsetY * screenScaler.GetUniformScale();
		float checkBoxWidth = scaledCheckboxSize.x;
		float checkBoxHeight = scaledCheckboxSize.y;

		bool isHovered = (mousePos.x >= checkBoxX && mousePos.x <= checkBoxX + checkBoxWidth &&
			mousePos.y >= checkBoxY && mousePos.y <= checkBoxY + checkBoxHeight);

		bool isPressed = isHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT);// 左クリックが押されているかどうか

		// チェックボックスのサイズを変更
		checkboxSize[i] = isPressed ? checkBoxOriginalSize[i] : (isHovered ? checkBoxHoverSize[i] : checkBoxOriginalSize[i]);

		if (isHovered)
		{
			hoveredCheckboxIndex = i; // マウスオーバーしているチェックボックスのインデックスを更新

			if (prevHoveredCheckBoxIndex != i && hoverSound && !isHoverSoundPlayed)
			{
				hoverSound->PlayOneShot();
				isHoverSoundPlayed = true;
			}


			if (isClicked)
			{
				showCheckIcon[i] = !showCheckIcon[i];
				switch(hoveredCheckboxIndex)
				{
					case 0:
						// 打撃後のゲーム速度操作方法表示のON/OFFを切り替える処理
						Pitcher::Instance().SetShowOperationGuide(showCheckIcon[i]);
						break;
					case 1:
						// 観客の表示/非表示のON/OFFを切り替える処理
						Audience::Instance().SetIsShowAudience(showCheckIcon[i]);
						break;
				}

				if (clickSound)
				{
					clickSound->PlayOneShot();
				}
			}
		}
		else
		{
			isHoverSoundPlayed = false;
		}
	}

}

void OtherSettingPanel::UpdateGameSpeedDisplay()
{
	// ゲームスピードの表示更新処理
	Input& input = Input::Instance();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	//マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY());

	bool isClicked = input.GetMouse().GetButtonDown() & Mouse::BTN_LEFT;

	int prevHoveredGameSpeedIndex = hoveredGameSpeedIndex;
	hoveredGameSpeedIndex = -1; // 初期化

	//チェックボックスの位置とサイズをスケーリング
	for (int i = 0; i < numGameSpeedTypes; ++i)
	{
		DirectX::XMFLOAT2 scaledSpeedTypePos = screenScaler.Scale(gameSpeedTypeFontData[i].position);
		const float scaledSpeedTypeFontScale = gameSpeedTypeFontData[i].size * screenScaler.GetUniformScale();
		float textWidth, textHeight;
		fontRenderer.MeasureText(i == 0 ? u8"通常" : (i == 1 ? u8"中速" : u8"高速"), scaledSpeedTypeFontScale, textWidth, textHeight);
		
		float speedTypeX = scaledSpeedTypePos.x - textWidth / 2.0f;
		float speedTypeY = scaledSpeedTypePos.y - textHeight / 2.0f;
		

		//左下基準でマウスが重なっているか
		bool isHovered = (mousePos.x >= speedTypeX && mousePos.x <= speedTypeX + textWidth &&
			mousePos.y >= speedTypeY - textHeight && mousePos.y <= speedTypeY);
		bool isPressed = isHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT);// 左クリックが押されているかどうか

		gameSpeedTypeFontData[i].size = isPressed ? gameSpeedTypeOriginalSize[i] : (isHovered ? gameSpeedTypeHoverSize[i] : gameSpeedTypeOriginalSize[i]);

		if (isHovered)
		{
			hoveredGameSpeedIndex = i; // マウスオーバーしているゲームスピードのインデックスを更新

			if(prevHoveredGameSpeedIndex != i && hoverSound)
			{
				hoverSound->PlayOneShot();
			}

			if (isClicked)
			{
				if (clickSound)
				{
					clickSound->PlayOneShot();
				}

				// クリックされたゲームスピードのインデックスを保存
				clickedGameSpeedIndex = i;

				// ゲームスピードの変更処理
				if(hoveredGameSpeedIndex >= 0 && hoveredGameSpeedIndex < numGameSpeedTypes)
				{
					switch (hoveredGameSpeedIndex)
					{
						case 0:
						//GameModeManager::Instance().SetTimeScale(1.0f);
						GameModeManager::Instance().ChangeTimeScaleMode(TimeScaleMode::Normal);
						break;
						case 1:
						//GameModeManager::Instance().SetTimeScale(1.5f);
						GameModeManager::Instance().ChangeTimeScaleMode(TimeScaleMode::Medium);
						break;
						case 2:
						//GameModeManager::Instance().SetTimeScale(2.0f);
						GameModeManager::Instance().ChangeTimeScaleMode(TimeScaleMode::Fast);
						break;
					}
				}

				//選ばれたボタンはフォントの色をきいろにする
				for(int j = 0; j < numGameSpeedTypes; ++j)
				{
					if(j == clickedGameSpeedIndex)
					{
						gameSpeedTypeFontData[j].color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 選択されたボタンの色を黄色に設定
					}
					else
					{
						gameSpeedTypeFontData[j].color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 選択されていないボタンの色を白に設定
					}
				}
				
			}
		}
	}
}


void OtherSettingPanel::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// チェックボックスの描画
	for (int i = 0; i < numCheckBoxes; ++i)
	{
		if (showOperationCheckboxData && showOperationCheckboxSprite)
		{
			DirectX::XMFLOAT2 scaledCheckboxPos = screenScaler.Scale(checkboxPosition[i]);
			DirectX::XMFLOAT2 scaledCheckboxSize = screenScaler.ScaleSize(checkboxSize[i]);

			DirectX::XMFLOAT2 checkBoxCenterPos = { scaledCheckboxPos.x - scaledCheckboxSize.x / 2.0f, scaledCheckboxPos.y - scaledCheckboxSize.y / 2.0f };

			//間隔調整の変数
			float sliderSpacing = 150.0f * screenScaler.GetUniformScale();

			//3回描画することで、マスターボリューム、BGMボリューム、SEボリュームのスライダー背景を描画
			DirectX::XMFLOAT2 offsetPos = { checkBoxCenterPos.x + offsetX * screenScaler.GetUniformScale(), checkBoxCenterPos.y + i * sliderSpacing + offsetY * screenScaler.GetUniformScale() };

			showOperationCheckboxSprite[i]->render(
				dc, offsetPos.x, offsetPos.y,
				scaledCheckboxSize.x, scaledCheckboxSize.y,
				showOperationCheckboxData[i]->color.x, showOperationCheckboxData[i]->color.y, showOperationCheckboxData[i]->color.z, showOperationCheckboxData[i]->color.w,
				showOperationCheckboxData[i]->rotation);
		}

		// チェックアイコンの描画
		if (checkIconData && checkIconSprite && showCheckIcon[i])
		{
			DirectX::XMFLOAT2 scaledCheckIconPos = screenScaler.Scale(checkIconPosition[i]);
			DirectX::XMFLOAT2 scaledCheckIconSize = screenScaler.ScaleSize(checkIconSize[i]);
			DirectX::XMFLOAT2 checkIconCenterPos = { scaledCheckIconPos.x - scaledCheckIconSize.x / 2.0f, scaledCheckIconPos.y - scaledCheckIconSize.y / 2.0f };

			//間隔調整の変数
			float sliderSpacing = 150.0f * screenScaler.GetUniformScale();

			DirectX::XMFLOAT2 offsetPos = { checkIconCenterPos.x + offsetX * screenScaler.GetUniformScale(), checkIconCenterPos.y + i * sliderSpacing + offsetY * screenScaler.GetUniformScale() };

			checkIconSprite[i]->render(
				dc, offsetPos.x, offsetPos.y,
				scaledCheckIconSize.x, scaledCheckIconSize.y,
				checkIconData[i]->color.x, checkIconData[i]->color.y, checkIconData[i]->color.z, checkIconData[i]->color.w,
				checkIconData[i]->rotation);
		}
	}

	//ゲームスピードタイプの描画
	if (gameSpeedTypeFontData)
	{
		for (int i = 0; i < numGameSpeedTypes; ++i)
		{
			DirectX::XMFLOAT2 scaledGameSpeedTypeFontPos = screenScaler.Scale(gameSpeedTypeFontData[i].position);
			const float scaledGameSpeedTypeFontScale = gameSpeedTypeFontData[i].size * screenScaler.GetUniformScale();
			float gameSpeedTypeTextWidth, gameSpeedTypeTextHeight;
			const char* gameSpeedTypeText = nullptr;
			switch (i)
			{
			case 0:
				gameSpeedTypeText = u8"通常";
				break;
			case 1:
				gameSpeedTypeText = u8"中速";
				break;
			case 2:
				gameSpeedTypeText = u8"高速";
				break;
			default:
				gameSpeedTypeText = u8"";
				break;
			}
			fontRenderer.MeasureText(gameSpeedTypeText, scaledGameSpeedTypeFontScale, gameSpeedTypeTextWidth, gameSpeedTypeTextHeight);
			float gameSpeedTypeDrawPosX = scaledGameSpeedTypeFontPos.x - gameSpeedTypeTextWidth / 2.0f;
			float gameSpeedTypeDrawPosY = scaledGameSpeedTypeFontPos.y - gameSpeedTypeTextHeight / 2.0f;
			fontRenderer.DrawTextW(dc, 
				gameSpeedTypeText, 
				gameSpeedTypeDrawPosX,gameSpeedTypeDrawPosY, 
				scaledGameSpeedTypeFontScale, 
				gameSpeedTypeFontData[i].color.x, gameSpeedTypeFontData[i].color.y, 
				gameSpeedTypeFontData[i].color.z, gameSpeedTypeFontData[i].color.w);
		}
	}


	// フォントの描画
	//操作方法の表示
	DirectX::XMFLOAT2 scaledFontPos = screenScaler.Scale(showOperationFontData.position);
	const float scaledFontScale = showOperationFontData.size * screenScaler.GetUniformScale();

	float textWidth, textHeight;
	const char* text = u8"操作方法表示";
	fontRenderer.MeasureText(text, scaledFontScale, textWidth, textHeight);
	float drawPosX = scaledFontPos.x - textWidth / 2.0f;
	float drawPosY = scaledFontPos.y - textHeight / 2.0f;

	fontRenderer.DrawTextW(dc, text, drawPosX, drawPosY, scaledFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	//ゲームスピードの表示
	DirectX::XMFLOAT2 scaledGameSpeedFontPos = screenScaler.Scale(gameSpeedFontData.position);
	const float scaledGameSpeedFontScale = gameSpeedFontData.size * screenScaler.GetUniformScale();

	float gameSpeedTextWidth, gameSpeedTextHeight;
	const char* gameSpeedText = u8"打撃後のゲーム速度";
	fontRenderer.MeasureText(gameSpeedText, scaledGameSpeedFontScale, gameSpeedTextWidth, gameSpeedTextHeight);
	float gameSpeedDrawPosX = scaledGameSpeedFontPos.x - gameSpeedTextWidth / 2.0f;
	float gameSpeedDrawPosY = scaledGameSpeedFontPos.y - gameSpeedTextHeight / 2.0f;

	fontRenderer.DrawTextW(dc, gameSpeedText, gameSpeedDrawPosX, gameSpeedDrawPosY, scaledGameSpeedFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	//観客の表示
	DirectX::XMFLOAT2 scaledAudienceFontPos = screenScaler.Scale(showAudienceFontData.position);
	const float scaledAudienceFontScale = showAudienceFontData.size * screenScaler.GetUniformScale();

	float audienceTextWidth, audienceTextHeight;
	const char* audienceText = u8"観客の表示";
	fontRenderer.MeasureText(audienceText, scaledAudienceFontScale, audienceTextWidth, audienceTextHeight);
	float audienceDrawPosX = scaledAudienceFontPos.x - audienceTextWidth / 2.0f;
	float audienceDrawPosY = scaledAudienceFontPos.y - audienceTextHeight / 2.0f;

	fontRenderer.DrawTextW(dc, audienceText, audienceDrawPosX, audienceDrawPosY, scaledAudienceFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

	// 描画後の状態をリセット
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void OtherSettingPanel::DrawGUI()
{
	// ImGuiを使ったGUI描画処理
	ImGui::Begin("Other Settings");
	// 操作方法表示のチェックボックス
	ImGui::Checkbox("Show Operation", &showCheckIcon[0]);
	ImGui::Checkbox("Show Audience", &showCheckIcon[1]);

	ImGui::End();
}