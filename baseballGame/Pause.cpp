#include "pch.h"
#include "Pause.h"
#include "Graphics.h"
#include "shader.h"

void Pause::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
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

	// ポーズ画面のスプライトとデータの初期化
	pauseBackGroundData = std::make_unique<PauseData>();
	pauseBackGroundData->texturePath = L".\\resources\\textures\\pauseBoard.png";
	pauseBackGroundData->position = pauseBackGroundPosition;
	pauseBackGroundData->size = pauseBackGroundSize;
	pauseBackGroundData->rotation = 0.0f;
	pauseBackGroundData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pauseBackGroundSprite = std::make_unique<sprite>(device, context, pauseBackGroundData->texturePath.c_str());

	// ポーズ画面を初期状態では非表示にする
	isPauseOpen = false;
}

void Pause::Uninitialize()
{
	pauseBackGroundSprite.reset();
	pauseBackGroundData.reset();
	spritePS.Reset();
	spriteVS.Reset();
	spriteInputLayout.Reset();
}

void Pause::Update(float elapsedTime)
{
	// ポーズ画面の更新処理が必要な場合はここに記述
}

void Pause::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if (isPauseOpen && pauseBackGroundSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(pauseBackGroundPosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(pauseBackGroundSize);

		DirectX::XMFLOAT2 scaledCenterPos = { scaledPosition.x - scaledSize.x / 2.0f, scaledPosition.y - scaledSize.y / 2.0f };

		pauseBackGroundSprite->render(dc,
			scaledCenterPos.x, scaledCenterPos.y,
			scaledSize.x, scaledSize.y,
			pauseBackGroundData->color.x, pauseBackGroundData->color.y, pauseBackGroundData->color.z, pauseBackGroundData->color.w,
			pauseBackGroundData->rotation);
	}
}