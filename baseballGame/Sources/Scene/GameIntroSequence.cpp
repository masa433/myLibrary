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

	stadiumNameBoardData = std::make_unique<IntroData>();
	stadiumNameBoardData->texturePath = L".\\resources\\textures\\stadiumNameBoard.png";
	stadiumNameBoardData->position = stadiumNameBoardPosition;
	stadiumNameBoardData->size = stadiumNameBoardSize;
	stadiumNameBoardData->rotation = 0.0f;
	stadiumNameBoardData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	stadiumNameBoardSprite = std::make_unique<sprite>(device, context, stadiumNameBoardData->texturePath.c_str());

	cameraFadeData = std::make_unique<IntroData>();
	cameraFadeData->texturePath = L".\\resources\\textures\\scrollViewBack.png";
	cameraFadeData->position = { 960.0f, 540.0f };
	cameraFadeData->size = { 1920.0f, 1080.0f };
	cameraFadeData->rotation = 0.0f;
	cameraFadeData->color = { 0.0f, 0.0f, 0.0f, 1.0f };
	cameraFadeSprite = std::make_unique<sprite>(device, context, cameraFadeData->texturePath.c_str());

    // 初期化処理
    introTimer = 0.0f;
    introStarted = false;
    introDuration = 10.0f; // 初期の表示時間を設定
	introAmount = 0.0f; // 初期の進行度を設定
    introState = (rand() % 2 == 0) ? GameIntroState::ShowingGround : GameIntroState::ShowingStand;
	
	fadeTimer = 0.0f;
	fadeDuration = 0.5f;
	fadeAlpha = 0.0f;

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
	//仮で右クリックを押したらすべてのイントロをスキップする
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        introState = GameIntroState::Playing;
        introTimer = 0.0f;
        introStarted = false;
        introDuration = 0.0f;
        introAmount = 1.0f; // 進行度を1.0に設定
        amountTimer = 0.0f; // 進行度のタイマーをリセット
        showNameBoardTimer = maxShowNameBoardTime; // スタジアム名ボードの表示タイマーを最大値に設定
		broadcastCamera.StopAllTracking(); // カメラの追跡を停止
		return;// 右クリックが押された場合は以降の処理をスキップ
    }


	UpdateFadeOut(elapsed_time);
	UpdateFadeIn(elapsed_time);

    introTimer += elapsed_time;

	amountTimer += elapsed_time;
	showNameBoardTimer += elapsed_time;

    if (introState == GameIntroState::ShowingGround || introState == GameIntroState::ShowingStand)
    {
        if (introTimer >= (introDuration - 0.5f) && !isFadingOut && fadeAlpha < 1.0f)
        {
			isFadingOut = true;
            fadeTimer = 0.0f;
            fadeDuration = 0.5f;
		}
    }

    if(showNameBoardTimer >= maxShowNameBoardTime)
    {
        showNameBoardTimer = maxShowNameBoardTime;
		stadiumNameBoardData->color.w -= elapsed_time / 0.2f; // 0.2秒かけてフェードアウト

        if(stadiumNameBoardData->color.w < 0.0f)
        {
            stadiumNameBoardData->color.w = 0.0f; // 透明度が負にならないように制限
		}
	}

	float amountProgress = amountTimer / amountDuration;


	//イージング関数を用いてintroAmountを0.0から1.0に変化させる
    introAmount = UiEasing::Lerp(0.0f, 1.0f, amountProgress, UiEasing::EasingType::OutQuint);

    if (!introStarted)
    {
        introStarted = true;

        // ピッチャーを映している時はカメラIDを12か13のどちらかに設定する
        int pitcherCameraId, batterCameraId, groundCameraId, standCameraId;

        if(introState == GameIntroState::ShowingGround)
        {
            groundCameraId = 4; // グラウンドを映すカメラID
            int index = broadcastCamera.GetCameraIndexById(groundCameraId);
            broadcastCamera.SetActiveIndex(index);
            broadcastCamera.ResetCameraToPreset(index);
            broadcastCamera.StartEventCameraFocusYShift(0.0f, introDuration + 1.0f);
		}

        if (introState == GameIntroState::ShowingStand)
        {
			standCameraId = (rand() % 2 == 0) ? 16 : 21; // ランダムにスタンドカメラIDを選択
            int index = broadcastCamera.GetCameraIndexById(standCameraId);
            broadcastCamera.SetActiveIndex(index);
            broadcastCamera.ResetCameraToPreset(index);
            if(standCameraId == 16)
            {
                broadcastCamera.StartEventCameraZoom(DirectX::XMConvertToRadians(45.0f), introDuration);
                broadcastCamera.StartEventCameraFocusZShift(30.0f, introDuration + 1.0f);
                broadcastCamera.StartEventCameraFocusYShift(0.0f, introDuration);
            }
            else if(standCameraId == 21)
            {
				broadcastCamera.StartEventCameraEyeXZShift(-65.0f, 15.0f, -25.0f, introDuration + 1.0f);
			}
        }

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

		if (introState == GameIntroState::ShowingPitcher || introState == GameIntroState::ShowingBatter)
        {
            broadcastCamera.SetActiveIndex(index);
            broadcastCamera.ResetCameraToPreset(index);
            broadcastCamera.StartEventCameraZoom(DirectX::XMConvertToRadians(30.0f), introDuration);
        }
      
    }

    if (introTimer >= introDuration)
    {
        introTimer = 0.0f;
        introStarted = false;
        if(introState == GameIntroState::ShowingStand || introState == GameIntroState::ShowingGround)
        {
			introState = GameIntroState::ShowingPitcher;
			introDuration = 10.0f; // グラウンドを映す時間に変更
			introAmount = 0.0f; // グラウンドのイントロ用にリセット
            amountTimer = 0.0f; // グラウンドのイントロ用にリセット

            isFadingOut = false; // フェードアウト停止
            isFadingIn = true;   // フェードイン開始
            fadeTimer = 0.0f;
            fadeDuration = 0.5f; // 0.5秒かけて徐々に明らむ
            fadeAlpha = 1.0f;    // 開始時点は真っ黒にする
        }
        else if (introState == GameIntroState::ShowingPitcher)
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

void GameIntroSequence::UpdateFadeIn(float elapsedTime)
{
    if (isFadingIn)
    {
        fadeTimer += elapsedTime;
        fadeAlpha = 1.0f - (fadeTimer / fadeDuration);
        if (fadeAlpha < 0.0f)
        {
            fadeAlpha = 0.0f;
            isFadingIn = false; // フェードイン完了
        }
    }
}

void GameIntroSequence::UpdateFadeOut(float elapsedTime)
{
    if (isFadingOut)
    {
        fadeTimer += elapsedTime;
        fadeAlpha = fadeTimer / fadeDuration;
        if (fadeAlpha > 1.0f)
        {
            fadeAlpha = 1.0f;
            isFadingOut = false; // フェードアウト完了
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

    if (introState == GameIntroState::ShowingStand || introState == GameIntroState::ShowingGround)
    {

        DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(stadiumNameBoardPosition);
        DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(stadiumNameBoardSize);
        DirectX::XMFLOAT2 scaledCenteredPos =
        {
            scaledPosition.x - scaledSize.x / 2.0f,
            scaledPosition.y - scaledSize.y / 2.0f
        };

        DrawFillHorizontalFromCenter(stadiumNameBoardSprite.get(), stadiumNameBoardData.get(), context,
			scaledCenteredPos, scaledSize, introAmount);

    }
    else if (introState == GameIntroState::ShowingPitcher)
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

    if(cameraFadeData && cameraFadeSprite && (isFadingIn || isFadingOut))
    {
        DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(cameraFadeData->position);
        DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(cameraFadeData->size);
        DirectX::XMFLOAT2 scaledCenteredPos =
        {
            scaledPosition.x - scaledSize.x / 2.0f,
            scaledPosition.y - scaledSize.y / 2.0f
        };
        cameraFadeSprite->render(context,
            scaledCenteredPos.x,
            scaledCenteredPos.y,
            scaledSize.x,
            scaledSize.y,
            cameraFadeData->color.x,
            cameraFadeData->color.y,
            cameraFadeData->color.z,
			cameraFadeData->color.w * fadeAlpha,
            cameraFadeData->rotation);
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

void GameIntroSequence::DrawFillHorizontalFromCenter(sprite* spr, IntroData* data, ID3D11DeviceContext* context,
    DirectX::XMFLOAT2& pos, DirectX::XMFLOAT2& size, float amount)
{
    // amountは0.0から1.0の範囲で、描画する幅の割合を示す
    amount = (std::max)(0.0f, (std::min)(1.0f, amount)); // 0.0から1.0の範囲に制限
    
	//中心から両端に描画する幅を計算
	float halfWidth = size.x * amount / 2.0f;
	float drawWidth = halfWidth * 2.0f;

	D3D11_RECT scissorRect;
	scissorRect.left = static_cast<LONG>(pos.x + size.x / 2.0f - halfWidth);
	scissorRect.top = static_cast<LONG>(pos.y);
	scissorRect.right = static_cast<LONG>(pos.x + size.x / 2.0f + halfWidth);
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