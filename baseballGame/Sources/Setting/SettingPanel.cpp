#include "pch.h"
#include "shader.h"
#include "SettingPanel.h"
#include "Graphics.h"
#include "ScreenScaler.h"
#include "input.h"
#include "Pitcher.h"

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

	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"全体BGMSE"
		u8"カーソル感度"
		u8"打撃後のゲーム速度操作方法表示"
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
	bgmVolumeFontData.size = 0.5f;
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
}

void AudioSettingPanel::Update(float elapsedTime)
{
	// 設定パネルの更新処理
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
	// マスターボリュームのスライダー
	static float masterVolume = 1.0f;
	if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f))
	{
		// マスターボリュームの変更処理
	}
	// BGMボリュームのスライダー
	static float bgmVolume = 1.0f;
	if (ImGui::SliderFloat("BGM Volume", &bgmVolume, 0.0f, 1.0f))
	{
		// BGMボリュームの変更処理
	}
	// SEボリュームのスライダー
	static float seVolume = 1.0f;
	if (ImGui::SliderFloat("SE Volume", &seVolume, 0.0f, 1.0f))
	{
		// SEボリュームの変更処理
	}
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


	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints
	(
		u8"バットカーソル感度"
		u8"打撃後のゲーム速度操作方法表示"
	);

	// フォントレンダラーの初期化
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	// フォントデータの設定
	cursorFontData.position = { 600.0f, 550.0f };
	cursorFontData.size = 0.3f;
}

void CursorSettingPanel::Uninitialize()
{
	// リソースの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
	fontRenderer.Uninitialize();
	cursorFontData = {};
}

void CursorSettingPanel::Update(float elapsedTime)
{
	// 設定パネルの更新処理
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