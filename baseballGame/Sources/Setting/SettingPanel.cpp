#include "pch.h"
#include "shader.h"
#include "SettingPanel.h"
#include "Graphics.h"
#include "ScreenScaler.h"
#include "input.h"
#include "Pitcher.h"
#include "GameModeManager.h"

void AudioSettingPanel::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	
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
	}

	//スライダーボタンのテクスチャの読み込み
	for (int i = 0; i < numVolumeTypes; ++i)
	{
		volumeSliderButtonData[i] = std::make_unique<SpriteData>();
		volumeSliderButtonData[i]->texturePath = L".\\resources\\textures\\Ball.png";
		//ボタンの初期位置をスライダーの右端に設定
		volumeSliderButtonData[i]->position = { volumeSliderButtonPosition[i].x, volumeSliderButtonPosition[i].y};
		volumeSliderButtonData[i]->size = { 50.0f, 50.0f };
		volumeSliderButtonData[i]->rotation = 0.0f;
		volumeSliderButtonData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		volumeSliderButtonSprite[i] = std::make_unique<sprite>(device, context, volumeSliderButtonData[i]->texturePath.c_str());

		volumeSliderButtonPosition[i].x = volumeSliderBackgroundPosition[i].x + volumeSliderBackgroundData[i]->size.x / 2.0f;
	}



	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"全体BGMSE"
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
}
void AudioSettingPanel::Update(float elapsedTime)
{
	Input& input = Input::Instance();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	// スライダーのドラッグ処理
	// マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = DirectX::XMFLOAT2(input.GetMouse().GetPositionX(), input.GetMouse().GetPositionY());

	for (int i = 0; i < numVolumeTypes; ++i)
	{
		// スライダーのボタンの位置を取得
		DirectX::XMFLOAT2 scaledButtonPos = screenScaler.Scale(volumeSliderButtonPosition[i]);
		DirectX::XMFLOAT2 scaledButtonSize = screenScaler.ScaleSize(volumeSliderButtonData[i]->size);
		DirectX::XMFLOAT2 scaledButtonCenterPos = { scaledButtonPos.x - scaledButtonSize.x / 2.0f, scaledButtonPos.y - scaledButtonSize.y / 2.0f };

		// 間隔調整の変数
		float sliderSpacing = 130.0f * screenScaler.GetUniformScale();

		float offsetX = scaledButtonCenterPos.x + volumeSliderButtonOffset.x;
		float offsetY = scaledButtonCenterPos.y + i * sliderSpacing + volumeSliderButtonOffset.y;
		float buttonWidth = scaledButtonSize.x;
		float buttonHeight = scaledButtonSize.y;

		bool isHovered = (mousePos.x >= offsetX && mousePos.x <= offsetX + buttonWidth &&
			mousePos.y >= offsetY && mousePos.y <= offsetY + buttonHeight);

		bool isPressed = (input.GetMouse().GetButton() & Mouse::BTN_LEFT);// 左クリックが押されているかどうか

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

			// ボリュームの値を計算
			float volumeValue = (volumeSliderButtonPosition[i].x - minX) / (maxX - minX);

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
	if(volumeSliderBackgroundSprite && volumeSliderBackgroundData)
	{
		for(int i  = 0; i < numVolumeTypes; ++i)
		{
			DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(volumeSliderBackgroundPosition[i]);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(volumeSliderBackgroundData[i]->size);

			DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };

			//間隔調整の変数
			float sliderSpacing = 130.0f * screenScaler.GetUniformScale();

			//3回描画することで、マスターボリューム、BGMボリューム、SEボリュームのスライダー背景を描画
			DirectX::XMFLOAT2 offsetPos = { scaledCenterPos.x + volumeSliderOffset.x, scaledCenterPos.y + i * sliderSpacing + volumeSliderOffset.y };

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
	if(volumeSliderButtonSprite && volumeSliderButtonData)
	{
		for(int i = 0; i < numVolumeTypes; ++i)
		{
			DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(volumeSliderButtonPosition[i]);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(volumeSliderButtonData[i]->size);
			DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };
			//間隔調整の変数
			float sliderSpacing = 130.0f * screenScaler.GetUniformScale();

			DirectX::XMFLOAT2 offsetPos = { scaledCenterPos.x + volumeSliderButtonOffset.x, scaledCenterPos.y + i * sliderSpacing + volumeSliderButtonOffset.y };

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

	sliderButtonData = std::make_unique<SpriteData>();
	sliderButtonData->texturePath = L".\\resources\\textures\\Ball.png";
	sliderButtonData->position = { sliderButtonPosition.x, sliderButtonPosition.y };
	sliderButtonData->size = { sliderButtonSize.x, sliderButtonSize.y };
	sliderButtonData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	sliderButtonData->rotation = 0.0f;
	sliderButtonSprite = std::make_unique<sprite>(device, context, sliderButtonData->texturePath.c_str());


	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"バットカーソル感度"
		u8"打撃後のゲーム速度操作方法表示"
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
	float buttonX = scaledCenterPos.x + offsetX;
	float buttonY = scaledCenterPos.y + offsetY;
	float buttonWidth = scaledButtonSize.x;
	float buttonHeight = scaledButtonSize.y;

	// マウスがスライダーボタンの範囲内にあるかどうかを判定
	bool isHover = (mousePos.x >= buttonX && mousePos.x <= buttonX + buttonWidth &&
		mousePos.y >= buttonY && mousePos.y <= buttonY + buttonHeight);
	bool isPressed = (input.GetMouse().GetButton() & Mouse::BTN_LEFT);// 左クリックが押されているかどうか
	
	sliderButtonSize = isPressed ? sliderButtonOriginalSize : (isHover ? sliderButtonHoverSize : sliderButtonOriginalSize);


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
		sliderButtonPosition.x = mousePos.x - offsetX;
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
			scaledCenterPos.x + offsetX, scaledCenterPos.y + offsetY,
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
			scaledCenterPos.x + offsetX, scaledCenterPos.y + offsetY,
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
		valueDrawPosX + cursorValueOffset.x, valueDrawPosY + cursorValueOffset.y,
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
	showOperationCheckboxData = std::make_unique<SpriteData>();
	showOperationCheckboxData->texturePath = L".\\resources\\textures\\checkBoxFrame.png";
	showOperationCheckboxData->position = checkboxPosition;
	showOperationCheckboxData->size = checkboxSize;
	showOperationCheckboxData->rotation = 0.0f;
	showOperationCheckboxData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	showOperationCheckboxSprite = std::make_unique<sprite>(device, context, showOperationCheckboxData->texturePath.c_str());

	checkIconData = std::make_unique<SpriteData>();
	checkIconData->texturePath = L".\\resources\\textures\\checkIcon.png";
	checkIconData->position = checkIconPosition;
	checkIconData->size = checkIconSize;
	checkIconData->rotation = 0.0f;
	checkIconData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	checkIconSprite = std::make_unique<sprite>(device, context, checkIconData->texturePath.c_str());
	
	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"打撃後のゲーム速度操作方法表示"
		u8"通常中速高速"
	);

	// フォントレンダラーの初期化
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	// フォントデータの設定
	gameSpeedFontData.position = { 600.0f, 550.0f };
	gameSpeedFontData.size = 0.3f;

	showOperationFontData.position = { 600.0f, 700.0f };
	showOperationFontData.size = 0.3f;

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
	checkboxPosition = { showOperationFontData.position.x + offsetX, showOperationFontData.position.y + offsetY };
	checkIconPosition = { checkboxPosition.x, checkboxPosition.y };
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
	for(int i = 0; i < numGameSpeedTypes; ++i)
	{
		gameSpeedTypeFontData[i] = {};
	}

	// チェックボックスとチェックアイコンの解放
	showOperationCheckboxData.reset();
	showOperationCheckboxSprite.reset();
	checkIconData.reset();
	checkIconSprite.reset();

}

void OtherSettingPanel::Update(float elapsedTime)
{
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

	//チェックボックスの位置とサイズをスケーリング
	DirectX::XMFLOAT2 scaledCheckboxPos = screenScaler.Scale(checkboxPosition);
	DirectX::XMFLOAT2 scaledCheckboxSize = screenScaler.ScaleSize(checkboxSize);
	DirectX::XMFLOAT2 scaledCenterPos = { scaledCheckboxPos.x - scaledCheckboxSize.x / 2.0f, scaledCheckboxPos.y - scaledCheckboxSize.y / 2.0f };

	//チェックボックスの範囲を計算
	float checkBoxX = scaledCenterPos.x;
	float checkBoxY = scaledCenterPos.y;
	float checkBoxWidth = scaledCheckboxSize.x;
	float checkBoxHeight = scaledCheckboxSize.y;

	bool isHovered = (mousePos.x >= checkBoxX && mousePos.x <= checkBoxX + checkBoxWidth &&
		mousePos.y >= checkBoxY && mousePos.y <= checkBoxY + checkBoxHeight);

	bool isPressed = isHovered && (input.GetMouse().GetButton() & input.GetMouse().BTN_LEFT);// 左クリックが押されているかどうか

	// チェックボックスのサイズを変更
	checkboxSize = isPressed ? checkBoxOriginalSize : (isHovered ? checkBoxHoverSize : checkBoxOriginalSize);

	if (isHovered)
	{
		if (isClicked)
		{
			showCheckIcon = !showCheckIcon;
			Pitcher::Instance().SetShowOperationGuide(showCheckIcon);
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

	int prevHoveredCheckboxIndex = hoveredCheckboxIndex;
	hoveredCheckboxIndex = -1; // 初期化

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
			hoveredCheckboxIndex = i; // マウスオーバーしているチェックボックスのインデックスを更新

			if (isClicked)
			{
				// クリックされたチェックボックスのインデックスを保存
				clickedCheckboxIndex = i;

				// ゲームスピードの変更処理
				if(hoveredCheckboxIndex >= 0 && hoveredCheckboxIndex < numGameSpeedTypes)
				{
					switch (hoveredCheckboxIndex)
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
					if(j == hoveredCheckboxIndex)
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
	if(showOperationCheckboxData && showOperationCheckboxSprite)
	{
		DirectX::XMFLOAT2 scaledCheckboxPos = screenScaler.Scale(checkboxPosition);
		DirectX::XMFLOAT2 scaledCheckboxSize = screenScaler.ScaleSize(checkboxSize);

		DirectX::XMFLOAT2 checkBoxCenterPos = { scaledCheckboxPos.x - scaledCheckboxSize.x / 2.0f, scaledCheckboxPos.y - scaledCheckboxSize.y / 2.0f };

		showOperationCheckboxSprite->render(
			dc, checkBoxCenterPos.x, checkBoxCenterPos.y, 
			scaledCheckboxSize.x, scaledCheckboxSize.y,
			showOperationCheckboxData->color.x, showOperationCheckboxData->color.y, showOperationCheckboxData->color.z, showOperationCheckboxData->color.w, 
			showOperationCheckboxData->rotation);
		
	}

	// チェックアイコンの描画
	if (checkIconData && checkIconSprite && showCheckIcon)
	{
		DirectX::XMFLOAT2 scaledCheckIconPos = screenScaler.Scale(checkIconPosition);
		DirectX::XMFLOAT2 scaledCheckIconSize = screenScaler.ScaleSize(checkIconSize);
		DirectX::XMFLOAT2 checkIconCenterPos = { scaledCheckIconPos.x - scaledCheckIconSize.x / 2.0f, scaledCheckIconPos.y - scaledCheckIconSize.y / 2.0f };
		checkIconSprite->render(
			dc, checkIconCenterPos.x, checkIconCenterPos.y,
			scaledCheckIconSize.x, scaledCheckIconSize.y,
			checkIconData->color.x, checkIconData->color.y, checkIconData->color.z, checkIconData->color.w,
			checkIconData->rotation);

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
	ImGui::Checkbox("Show Operation", &showCheckIcon);

	ImGui::End();
}