#include "pch.h"
#include "shader.h"
#include "SettingPanel.h"
#include "Graphics.h"
#include "ScreenScaler.h"

void AudioSettingPanel::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	//テクスチャの読み込み
	audioPanelBackgroundData = std::make_unique<SpriteData>();
	audioPanelBackgroundData->texturePath = L".\\resources\\textures\\ball.png";
	audioPanelBackgroundData->position = { 960.0f, 540.0f };
	audioPanelBackgroundData->size = { 100.0f, 100.0f };
	audioPanelBackgroundData->rotation = 0.0f;
	audioPanelBackgroundData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	audioPanelBackgroundSprite = std::make_unique<sprite>(device, context, audioPanelBackgroundData->texturePath.c_str());


	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	
}

void AudioSettingPanel::Uninitialize()
{
	// リソースの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
	audioPanelBackgroundData.reset();
	audioPanelBackgroundSprite.reset();
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

	if(audioPanelBackgroundData && audioPanelBackgroundSprite)
	{
		// スケーリングを適用して描画
		DirectX::XMFLOAT2 scaledPos = screenScaler.Scale(audioPanelBackgroundData->position);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(audioPanelBackgroundData->size);

		DirectX::XMFLOAT2 scaledCenterPos = { scaledPos.x - scaledSize.x / 2.0f, scaledPos.y - scaledSize.y / 2.0f };

		audioPanelBackgroundSprite->render(dc,
			scaledCenterPos.x, scaledCenterPos.y,
			scaledSize.x, scaledSize.y,
			audioPanelBackgroundData->color.x, audioPanelBackgroundData->color.y, audioPanelBackgroundData->color.z, audioPanelBackgroundData->color.w,
			audioPanelBackgroundData->rotation);
	}

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
}

void CursorSettingPanel::Uninitialize()
{
	// リソースの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
}

void CursorSettingPanel::Update(float elapsedTime)
{
	// 設定パネルの更新処理
}

void CursorSettingPanel::Render()
{
	// 設定パネルの描画処理
}

void GameSpeedSettingPanel::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
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
}

void GameSpeedSettingPanel::Uninitialize()
{
	// リソースの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
}

void GameSpeedSettingPanel::Update(float elapsedTime)
{
	// 設定パネルの更新処理
}

void GameSpeedSettingPanel::Render()
{
	// 設定パネルの描画処理
}