#include "GameIntroSequence.h"
#include "Player.h"
#include "Pitcher.h"
#include "Graphics.h"
#include "shader.h"
#include "UiEasing.h"

void GameIntroSequence::Initialize(ID3D11Device* device)
{

    ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

    D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(),
        input_element_desc, _countof(input_element_desc));
    create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

    for(int i = 0; i < PITCHER_COUNT; ++i)
    {
        pitcherIntroSpriteData[i] = std::make_unique<IntroData>();
        pitcherIntroSpriteData[i]->texturePath = L".\\resources\\textures\\pitcherIntroList\\pitcherIntroBoard" + std::to_wstring(i + 1) + L".png";
        pitcherIntroSpriteData[i]->position = pitcherIntroPosition;
        pitcherIntroSpriteData[i]->size = pitcherIntroSize;
        pitcherIntroSpriteData[i]->rotation = 0.0f;
        pitcherIntroSpriteData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        pitcherIntroSprite[i] = std::make_unique<sprite>(device, context, pitcherIntroSpriteData[i]->texturePath.c_str());
	}

    for(int i = 0; i < BATTER_COUNT; ++i)
    {
        batterIntroSpriteData[i] = std::make_unique<IntroData>();
        batterIntroSpriteData[i]->texturePath = L".\\resources\\textures\\batterIntroList\\batterIntroBoard" + std::to_wstring(i + 1) + L".png";
        batterIntroSpriteData[i]->position = batterIntroPosition;
        batterIntroSpriteData[i]->size = batterIntroSize;
        batterIntroSpriteData[i]->rotation = 0.0f;
        batterIntroSpriteData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        batterIntroSprite[i] = std::make_unique<sprite>(device, context, batterIntroSpriteData[i]->texturePath.c_str());
    }

    // 初期化処理
    introState = GameIntroState::ShowingPitcher;
    introTimer = 0.0f;
    introStarted = false;
    introDuration = 10.0f; // 初期の表示時間を設定
	introAmount = 0.0f; // 初期の進行度を設定

	//テクスチャを左側から順に表示するためのラスタライザステートを作成
    D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;// 塗りつぶしモードを設定
	rsDesc.CullMode = D3D11_CULL_NONE;// カリングを無効にする
	rsDesc.DepthClipEnable = TRUE;// デプスクリッピングを有効にする
	rsDesc.ScissorEnable = TRUE;// スクリーン外の描画を防ぐためにシザーを有効にする
    device->CreateRasterizerState(&rsDesc, rasterizerState.ReleaseAndGetAddressOf());
}

void GameIntroSequence::Uninitialize()
{
    // リソースの解放
    spriteVS.Reset();
    spritePS.Reset();
    spriteInputLayout.Reset();
    for(int i = 0; i < PITCHER_COUNT; ++i)
    {
        pitcherIntroSprite[i].reset();
        pitcherIntroSpriteData[i].reset();
    }
    for(int i = 0; i < BATTER_COUNT; ++i)
    {
        batterIntroSprite[i].reset();
        batterIntroSpriteData[i].reset();
    }
}

void GameIntroSequence::UpdateIntro(float elapsed_time,BroadcastCamera& broadcastCamera)
{
   
    introTimer += elapsed_time;

	amountTimer += elapsed_time;

	float amountProgress = amountTimer / amountDuration;


	//イージング関数を用いてintroAmountを0.0から1.0に変化させる
    if (introState == GameIntroState::ShowingPitcher)
    {
		introAmount = UiEasing::Lerp(0.0f, 1.0f, amountProgress, UiEasing::EasingType::OutQuint);
    }
    else if (introState == GameIntroState::ShowingBatter)
    {
        introAmount = UiEasing::Lerp(0.0f, 1.0f, amountProgress, UiEasing::EasingType::OutQuint);
	}

    if (!introStarted)
    {
        introStarted = true;

        // ピッチャーを映している時はカメラIDを12か13のどちらかに設定する
        int pitcherCameraId, batterCameraId;

        if(Pitcher::Instance().IsRightPitcher())
        {
			pitcherCameraId = 12; // 右投手用のカメラID
        }
        else
        {
			pitcherCameraId = 13; // 左投手用のカメラID
		}

        if (Player::Instance().IsRightBatter())
        {
			batterCameraId = 14; // 右打者用のカメラID
        }
        else
        {
            batterCameraId = 15; // 左打者用のカメラID
		}
        

        int cameraId = (introState == GameIntroState::ShowingPitcher) ? pitcherCameraId : batterCameraId;

        int index = broadcastCamera.GetCameraIndexById(cameraId);

        broadcastCamera.SetActiveIndex(index);
        broadcastCamera.ResetCameraToPreset(index);
        broadcastCamera.StartEventCameraZoom(DirectX::XMConvertToRadians(30.0f), introDuration);
    }

    if (introTimer >= introDuration)
    {
        introTimer = 0.0f;
        introStarted = false;
        if (introState == GameIntroState::ShowingPitcher)
        {
            introState = GameIntroState::ShowingBatter;
			introDuration = 9.0f; // バッターを映す時間に変更
			introAmount = 0.0f; // バッターのイントロ用にリセット
			amountTimer = 0.0f; // バッターのイントロ用にリセット
        }
        else if (introState == GameIntroState::ShowingBatter)
        {
            introState = GameIntroState::Playing;
            broadcastCamera.StopAllTracking();
            
        }
    }
}

void GameIntroSequence::Render()
{

	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

    ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
    // スプライト描画用のシェーダーを設定
    context->VSSetShader(spriteVS.Get(), nullptr, 0);
    context->PSSetShader(spritePS.Get(), nullptr, 0);
    context->IASetInputLayout(spriteInputLayout.Get());

    context->OMSetDepthStencilState(
        renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
    context->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    if (introState == GameIntroState::ShowingPitcher)
    {
		Pitcher::RealPitcher selectedPitcher = Pitcher::Instance().GetSelectedRealPitcher();

        selectedPitcherIndex = static_cast<int>(selectedPitcher) - 1;
        if (selectedPitcherIndex >= 0 && selectedPitcherIndex < PITCHER_COUNT)
        {

			DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(pitcherIntroPosition);
			DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(pitcherIntroSize);

            DirectX::XMFLOAT2 scaledCenteredPos =
            {
                scaledPosition.x - scaledSize.x / 2.0f,
                scaledPosition.y - scaledSize.y / 2.0f
			};

			DrawFillHorizontal(pitcherIntroSprite[selectedPitcherIndex].get(), 
				pitcherIntroSpriteData[selectedPitcherIndex].get(), context,
                scaledCenteredPos, scaledSize, 
                introAmount);
        }
    }
    else if (introState == GameIntroState::ShowingBatter)
    {

		Player::RealBatter selectedBatter = Player::Instance().GetSelectedRealBatter();
		selectedBatterIndex = static_cast<int>(selectedBatter) - 1;
        
        if (selectedBatterIndex >= 0 && selectedBatterIndex < BATTER_COUNT)
        {
            DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(batterIntroPosition);
            DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(batterIntroSize);
            DirectX::XMFLOAT2 scaledCenteredPos =
            {
                scaledPosition.x - scaledSize.x / 2.0f,
                scaledPosition.y - scaledSize.y / 2.0f
            };

			DrawFillHorizontal(batterIntroSprite[selectedBatterIndex].get(),
                batterIntroSpriteData[selectedBatterIndex].get(), context,
                scaledCenteredPos, scaledSize, 
				introAmount);
        }
    }

	//シェーダーの設定を解除
    context->VSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
	context->IASetInputLayout(nullptr);

    context->OMSetDepthStencilState(
        renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void GameIntroSequence::DrawFillHorizontal(sprite* spr, IntroData* data, ID3D11DeviceContext* context,
	DirectX::XMFLOAT2& pos, DirectX::XMFLOAT2& size, float amount)
{
	// amountは0.0から1.0の範囲で、描画する幅の割合を示す
	amount = (std::max)(0.0f, (std::min)(1.0f, amount)); // 0.0から1.0の範囲に制限

	//左端から描画する幅を計算
	D3D11_RECT scissorRect;
	scissorRect.left = static_cast<LONG>(pos.x);
	scissorRect.top = static_cast<LONG>(pos.y);
	scissorRect.right = static_cast<LONG>(pos.x + size.x * amount);
	scissorRect.bottom = static_cast<LONG>(pos.y + size.y);

	context->RSSetScissorRects(1, &scissorRect);// シザー矩形を設定
	context->RSSetState(rasterizerState.Get());// ラスタライザステートを設定

	// スプライトを描画
    spr->render(context,
        pos.x,
        pos.y,
        size.x,
        size.y,
        data->color.x,
        data->color.y,
        data->color.z,
        data->color.w,
		data->rotation);

	// シザー矩形を元に戻す
	D3D11_RECT fullRect;
	fullRect.left = 0;
	fullRect.top = 0;
	fullRect.right = static_cast<LONG>(Graphics::Instance().GetScreenWidth());
	fullRect.bottom = static_cast<LONG>(Graphics::Instance().GetScreenHeight());

	context->RSSetScissorRects(1, &fullRect);// シザー矩形を元に戻す
}