#include "pch.h"
#include "Pause.h"
#include "Graphics.h"
#include "shader.h"
#include "SettingManager.h"
#include <sceneTransition.h>

void Pause::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
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
	isChangingScene = false;

	buttonManager.Initialize();

	SettingManager::Instance().Initialize(device, context);

	hexTransitionEffect.Initialize();
}

void Pause::Uninitialize()
{
	pauseBackGroundSprite.reset();
	pauseBackGroundData.reset();
	spritePS.Reset();
	spriteVS.Reset();
	spriteInputLayout.Reset();
	buttonManager.Uninitialize();
	SettingManager::Instance().Uninitialize();
	hexTransitionEffect.Reset();
	hexTransitionEffect.Uninitialize();
}

void Pause::Update(float elapsedTime)
{
	// ポーズ画面の更新処理が必要な場合はここに記述
	if (isPauseOpen && !SettingManager::Instance().IsOpen() && !isChangingScene)
	{
		buttonManager.Update(elapsedTime);
	}

	if (!isChangingScene)
	{
		//ボタンが押されたときの処理
		if (buttonManager.IsRestartRequested())
		{
			//リスタートボタンが押されたときの処理
			buttonManager.ResetRestartRequest(false);
			ClosePauseScreen();
		}
		if (buttonManager.IsSettingsRequested())
		{
			//設定ボタンが押されたときの処理
			buttonManager.ResetSettingsRequest(false);
			SettingManager::Instance().Open();
		}
		if (buttonManager.IsTitleRequested())
		{
			//タイトルボタンが押されたときの処理
			buttonManager.ResetTitleRequest(false);
			isChangingScene = true;
			hexTransitionEffect.Start(1.0f); // 遷移アニメーションを開始
		}
	}
	else
	{
		hexTransitionEffect.Update(elapsedTime);

		if (hexTransitionEffect.IsFinished())
		{
			ChangeSceneGameToTitle();
		}
	}


	if (SettingManager::Instance().IsOpen() && !isChangingScene)
	{
		SettingManager::Instance().Update(elapsedTime);
	}
}

void Pause::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨

	if (isPauseOpen && pauseBackGroundSprite && !SettingManager::Instance().IsOpen())
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

	if (isPauseOpen && !SettingManager::Instance().IsOpen())
	{
		buttonManager.Render(1.0f, ButtonManager::ButtonType::Restart);
		buttonManager.Render(1.0f, ButtonManager::ButtonType::Settings);
		buttonManager.Render(1.0f, ButtonManager::ButtonType::Title);
	}

	if (SettingManager::Instance().IsOpen())
	{
		SettingManager::Instance().Render();
	}



	if (isChangingScene)
	{
		hexTransitionEffect.Render();
	}

	// シェーダーの設定解除
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);


}

void Pause::DrawGUI()
{
	if (!SettingManager::Instance().IsOpen())
	{
		buttonManager.DrawGUI();
	}
	else
	{
		SettingManager::Instance().DrawGUI();
	}

}

void Pause::SaveToJson(json& j)
{

	buttonManager.SaveToJson(j);

	SettingManager::Instance().SaveToJson(j);
}

void Pause::LoadFromJson(const json& j)
{

	buttonManager.LoadFromJson(j);

	SettingManager::Instance().LoadFromJson(j);
}