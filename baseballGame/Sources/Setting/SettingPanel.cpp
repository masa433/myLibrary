#include "pch.h"
#include "shader.h"
#include "SettingPanel.h"
#include "Graphics.h"
#include "ScreenScaler.h"

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
	showOperationFontData.position = { 600.0f, 550.0f };
	showOperationFontData.size = 0.3f;

	gameSpeedFontData.position = { 600.0f, 680.0f };
	gameSpeedFontData.size = 0.3f;
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
}

void OtherSettingPanel::Update(float elapsedTime)
{
	// 設定パネルの更新処理
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