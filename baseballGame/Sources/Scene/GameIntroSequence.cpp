#include "GameIntroSequence.h"
#include "Player.h"
#include "Pitcher.h"
#include "Graphics.h"
#include "shader.h"
#include "UiEasing.h"
#include "imgui.h"

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

	pitchParamData = std::make_unique<IntroData>();
	pitchParamData->texturePath = L".\\resources\\textures\\pitchParamBoard.png";
	pitchParamData->position = pitchParamPosition;
    pitchParamData->size = pitchParamSize;
	pitchParamData->rotation = 0.0f;
	pitchParamData->color = { 1.0f, 1.0f, 1.0f, 0.9f };
	pitchParamSprite = std::make_unique<sprite>(device, context, pitchParamData->texturePath.c_str());

    for(int i = 0; i < GRAPH_COUNT; ++i)
    {
        graphData[i] = std::make_unique<IntroData>();
        graphData[i]->texturePath = L".\\resources\\textures\\pitchWeightGraphs\\pitchWeightGraph" + std::to_wstring(i + 1) + L".png";
        graphData[i]->position = graphPosition;
        graphData[i]->size = graphSize;
        graphData[i]->rotation = 0.0f;
        graphData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        graphSprite[i] = std::make_unique<sprite>(device, context, graphData[i]->texturePath.c_str());
	}

	introBoardData = std::make_unique<IntroData>();
	introBoardData->texturePath = L".\\resources\\textures\\introBoard.png";
	introBoardData->position = introBoardPosition;
	introBoardData->size = introBoardSize;
	introBoardData->rotation = 0.0f;
	introBoardData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	introBoardSprite = std::make_unique<sprite>(device, context, introBoardData->texturePath.c_str());

    // 初期化処理
    introTimer = 0.0f;
    introStarted = false;
    introDuration = 10.0f; // 初期の表示時間を設定
	introAmount = 0.0f; // 初期の進行度を設定
    introState = (rand() % 2 == 0) ? GameIntroState::ShowingGround : GameIntroState::ShowingStand;
	
	fadeTimer = 0.0f;
	fadeDuration = 0.5f;
	fadeAlpha = 0.0f;

    introBoardAlpha = 1.0f;
    introBoardFadeTimer = 0.0f; 
    introBoardFadeDuration = 2.0f;

	//テクスチャを左側から順に表示するためのラスタライザステートを作成
    D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;// 塗りつぶしモードを設定
	rsDesc.CullMode = D3D11_CULL_NONE;// カリングを無効にする
	rsDesc.DepthClipEnable = TRUE;// デプスクリッピングを有効にする
	rsDesc.ScissorEnable = TRUE;// スクリーン外の描画を防ぐためにシザーを有効にする
    device->CreateRasterizerState(&rsDesc, rasterizerState.ReleaseAndGetAddressOf());

    //フォントレンダラー初期化

	Graphics& graphics = Graphics::Instance();
	const int screenWidth = static_cast<int>(graphics.GetScreenWidth());
	const int screenHeight = static_cast<int>(graphics.GetScreenHeight());

	std::vector<int> codepoints = FontRenderer::Utf8ToCodepoints(
        u8"0123456789.%"
        u8"持ち球種投球割合"
        u8"失投ストレートスライダー"
        u8"カーブチェンジアップフォーク"
        u8"ツーシームカットボールシンカー"
        u8"スクリュー縦スプリットスローカーブ"
        u8"シュートナックルボールスイーパーパーム"
        u8"ナチュラルシュート真っスラ火の玉ストレート");

	pitchParamFont.Initialize(device, 
        L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf", 
        100.0f, screenWidth, screenHeight, 
        2048, 2048, &codepoints);

    buttonManager.Initialize();
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
    for(int i = 0; i < GRAPH_COUNT; ++i)
    {
        graphSprite[i].reset();
        graphData[i].reset();
	}
	pitchParamFont.Uninitialize();
	buttonManager.Uninitialize();
}

void GameIntroSequence::UpdateIntro(float elapsed_time,BroadcastCamera& broadcastCamera)
{
	buttonManager.Update(elapsed_time);

    //スキップボタンが押されたときの処理
    if (buttonManager.IsSkipRequested())
    {
		buttonManager.ResetSkipRequest(false); // スキップボタンのリクエストをリセット
        introState = GameIntroState::ShowingIntroBoard;
        introTimer = 0.0f;
        introStarted = false;
        introDuration = 3.0f;
        introAmount = 1.0f; // 進行度を1.0に設定
        amountTimer = 0.0f; // 進行度のタイマーをリセット
		graphAmount = 1.0f; // グラフの進行度を1.0に設定
		graphTimer = 0.0f; // グラフのタイマーをリセット
		fadeAlpha = 0.0f; // フェードの透明度をリセット
        showNameBoardTimer = maxShowNameBoardTime; // スタジアム名ボードの表示タイマーを最大値に設定

        introBoardAlpha = 1.0f;
        introBoardFadeTimer = 0.0f;
        introBoardFadeDuration = 2.0f;
		
        //アクティブカメラをデフォルトのカメラに戻す
        int defaultCameraId = 0; // デフォルトのカメラID
        int index = broadcastCamera.GetCameraIndexById(defaultCameraId);
        broadcastCamera.SetActiveIndex(index);
        broadcastCamera.ResetCameraToPreset(index);

		broadcastCamera.SetReplayMode(false); // リプレイモードを無効化
		Pitcher& pitcher = Pitcher::Instance();


		pitcher.SetCurrentState(Pitcher::State::SelectingPitch); // ピッチャーの状態を判定待ちに設定
		pitcher.SetAnimationTime(0.0f); // ピッチャーのアニメーション時間をリセット
		pitcher.UpdateAnimation(0.0f); // ピッチャーのアニメーションを更新
		return;// 右クリックが押された場合は以降の処理をスキップ
    }


	UpdateFadeOut(elapsed_time);
	UpdateFadeIn(elapsed_time);

    introTimer += elapsed_time;

	amountTimer += elapsed_time;
	showNameBoardTimer += elapsed_time;
	graphTimer += elapsed_time;

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

    if (introState == GameIntroState::ShowingIntroBoard)
    {
		introBoardFadeTimer += elapsed_time;

		if (introBoardFadeTimer >= introBoardFadeDuration)
        {
			introBoardFadeTimer = introBoardFadeDuration; // フェードアウトが完了したらタイマーを最大値に固定

            introBoardAlpha -= elapsed_time / (introBoardFadeDuration * 0.5f); // イントロボードを徐々にフェードアウト
            if (introBoardAlpha < 0.0f)
            {
                introBoardAlpha = 0.0f; // 透明度が負にならないように制限
            }
        }

    }

	float amountProgress = amountTimer / amountDuration;

	float graphProgress = graphTimer / graphDuration;

	//イージング関数を用いてintroAmountを0.0から1.0に変化させる
    introAmount = UiEasing::Lerp(0.0f, 1.0f, amountProgress, UiEasing::EasingType::OutQuint);
	graphAmount = UiEasing::Lerp(0.0f, 1.0f, graphProgress, UiEasing::EasingType::InOutSine);

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
			graphAmount = 0.0f; // グラフの進行度をリセット
			graphTimer = 0.0f; // グラフのタイマーをリセット

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
            introState = GameIntroState::ShowingIntroBoard;
			//アクティブカメラをデフォルトのカメラに戻す
			int defaultCameraId = 0; // デフォルトのカメラID
			int index = broadcastCamera.GetCameraIndexById(defaultCameraId);
            broadcastCamera.SetActiveIndex(index);
            broadcastCamera.ResetCameraToPreset(index);
			introDuration = 3.0f; // イントロボードを映す時間に変更
            
        }
        else if (introState == GameIntroState::ShowingIntroBoard)
        {
            if(introBoardAlpha <= 0.0f)
            {
                introBoardAlpha = 0.0f; // 透明度が負にならないように制限
                introState = GameIntroState::Playing;
			}

            
            broadcastCamera.SetReplayMode(false); // リプレイモードを無効化
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

        //ピッチャーの持っている球種と投球割合を上から順に表示する
        auto& pitchType = Pitcher::Instance().GetCurrentPitcherPitchTypes();
        auto& weights = Pitcher::Instance().GetCurrentPitcherPitchWeights();

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

		//1行当たりの高さを37.5に設定
		const float headerHeight = 50.0f;
		const float rowHeight = 37.5f;

		//表示に必要な行数を下から計算
        size_t activeRowCount = pitchType.size();
        if (activeRowCount > 8) activeRowCount = 8; // 最大8行に制限

        float currentParamHeight = headerHeight + (rowHeight * static_cast<float>(activeRowCount));

        //パラメーターボードの描画
		DirectX::XMFLOAT2 scaledPitchParamPosition = screenScaler.Scale(pitchParamPosition);
		DirectX::XMFLOAT2 scaledPitchParamSize = screenScaler.ScaleSize(pitchParamSize);

       
        DirectX::XMFLOAT2 scaledPitchParamCenteredPos =
        {
            scaledPitchParamPosition.x - scaledPitchParamSize.x / 2.0f,
            scaledPitchParamPosition.y - scaledPitchParamSize.y / 2.0f
		};

		float scaleY = screenScaler.GetScaleY();
        float scaledVisibleHeight = currentParamHeight * scaleY;

        if(pitchParamData && pitchParamSprite)
        {
			//シザー領域で描画範囲を制限する
			D3D11_RECT scissorRect;
			scissorRect.left = static_cast<LONG>(scaledPitchParamCenteredPos.x);
			scissorRect.top = static_cast<LONG>(scaledPitchParamCenteredPos.y);
			scissorRect.right = static_cast<LONG>(scaledPitchParamCenteredPos.x + scaledPitchParamSize.x);
			scissorRect.bottom = static_cast<LONG>(scaledPitchParamCenteredPos.y + scaledVisibleHeight);

            context->RSSetScissorRects(1, &scissorRect);
            context->RSSetState(rasterizerState.Get()); // ScissorEnable = TRUE のステートを適用

            pitchParamSprite->render(context, scaledPitchParamCenteredPos.x, scaledPitchParamCenteredPos.y,
                scaledPitchParamSize.x, scaledPitchParamSize.y, 1.0f, 1.0f, 1.0f, pitchParamData->color.w, pitchParamData->rotation);

			// スプライト描画後にシザーを無効化する
            // シザー領域を画面全体に戻す
            D3D11_RECT fullRect = { 0, 0, static_cast<LONG>(Graphics::Instance().GetScreenWidth()), static_cast<LONG>(Graphics::Instance().GetScreenHeight()) };
            context->RSSetScissorRects(1, &fullRect);
		}


        //右ぞろえにするヘルパー関数
        auto rightTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
            {
                float textWidth = 0.0f;
                float textHeight = 0.0f;
                pitchParamFont.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
                return { x - textWidth, y - textHeight / 2.0f };
            };

		//中央ぞろえにするヘルパー関数
        auto centerTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
            {
                float textWidth = 0.0f;
                float textHeight = 0.0f;
                pitchParamFont.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
                return { x - textWidth / 2.0f, y - textHeight / 2.0f };
			};


		//球種と投球割合のラベルを表示する
		char pitchTypeLabelBuffer[256];
        snprintf(pitchTypeLabelBuffer, sizeof(pitchTypeLabelBuffer), u8"持ち球");
		DirectX::XMFLOAT2 scaledLabelPosition = screenScaler.Scale(pitchTypeLabelPosition);
		const float labelScale = pitchTypeLabelScale * screenScaler.GetUniformScale();
		//中央ぞろえの位置を計算       
		DirectX::XMFLOAT2 centerAlignedLabelPos = centerTextPosition(pitchTypeLabelBuffer, labelScale, scaledLabelPosition.x, scaledLabelPosition.y);
		pitchParamFont.DrawTextW(context, pitchTypeLabelBuffer, centerAlignedLabelPos.x, centerAlignedLabelPos.y, labelScale, 1.0f, 1.0f, 1.0f, 1.0f);

		//投球割合のラベルを表示する
        char pitchWeightLabelBuffer[256];
		snprintf(pitchWeightLabelBuffer, sizeof(pitchWeightLabelBuffer), u8"投球割合");
		DirectX::XMFLOAT2 scaledWeightLabelPosition = screenScaler.Scale(pitchWeightLabelPosition);
		const float weightLabelScale = pitchWeightLabelScale * screenScaler.GetUniformScale();
        //中央ぞろえの位置を計算		
		DirectX::XMFLOAT2 centerAlignedWeightLabelPos = centerTextPosition(pitchWeightLabelBuffer, weightLabelScale, scaledWeightLabelPosition.x, scaledWeightLabelPosition.y);
		pitchParamFont.DrawTextW(context, pitchWeightLabelBuffer, centerAlignedWeightLabelPos.x, centerAlignedWeightLabelPos.y, weightLabelScale, 1.0f, 1.0f, 1.0f, 1.0f);

        for(size_t i = 0;i < pitchType.size(); ++i)
        {
			
            //球種名の表示
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "%s", Pitcher::Instance().GetPitchTypeName(pitchType[i]));
            DirectX::XMFLOAT2 textPosition = {pitchTypeFontPosition.x, pitchTypeFontPosition.y + static_cast<float>(i) * offsetY}; // 適切な位置に調整
			textPosition = screenScaler.Scale(textPosition);
			const float textScale = pitchTypeFontScale * screenScaler.GetUniformScale();
            
			//中央ぞろえの位置を計算
			DirectX::XMFLOAT2 centerAlignedPos = centerTextPosition(buffer, textScale, textPosition.x, textPosition.y);       
			pitchParamFont.DrawTextW(context, buffer, centerAlignedPos.x, centerAlignedPos.y, textScale, 1.0f, 1.0f, 1.0f, 1.0f);
            

			float targetWeight = weights[i];
			float currentWeight = targetWeight * graphAmount; // graphAmountに応じて投球割合を増加させる


			//投球割合の表示
			char weightBuffer[256];
			snprintf(weightBuffer, sizeof(weightBuffer), u8"%.1f%%", currentWeight);

			
			DirectX::XMFLOAT2 weightTextPosition = { pitchWeightFontPosition.x, pitchWeightFontPosition.y + static_cast<float>(i) * offsetY }; // 適切な位置に調整
			weightTextPosition = screenScaler.Scale(weightTextPosition);
            const float weightTextScale = pitchWeightFontScale * screenScaler.GetUniformScale();
			
			//右ぞろえの位置を計算
			DirectX::XMFLOAT2 rightAlignedWeightPos = rightTextPosition(weightBuffer, weightTextScale, weightTextPosition.x, weightTextPosition.y);
			pitchParamFont.DrawTextW(context, weightBuffer, rightAlignedWeightPos.x, rightAlignedWeightPos.y, weightTextScale, 1.0f, 1.0f, 1.0f, 1.0f);
		}

        context->VSSetShader(spriteVS.Get(), nullptr, 0);
        context->PSSetShader(spritePS.Get(), nullptr, 0);
        context->IASetInputLayout(spriteInputLayout.Get());

		//グラフの描画
        //投球割合の大きさで描画するグラフを選択する(40%以上でグラフ1、30%～39%でグラフ2など)
        auto getGraphIndex = [](float weight) -> int
        {
            if(weight >= 40.0f) return 0;
            else if(weight >= 30.0f) return 1;
            else if(weight >= 20.0f) return 2;
            else if(weight >= 10.0f) return 3;
			else if (weight >= 1.0f) return 4;
            else return 5; // 10%未満の場合はグラフ6を使用
		};
        
        DirectX::XMFLOAT2 scaledGraphPosition = screenScaler.Scale(graphPosition);
        DirectX::XMFLOAT2 scaledGraphSize = screenScaler.ScaleSize(graphSize);
        float graphScaleY = screenScaler.GetScaleY();
        float scaledOffsetY = offsetY * graphScaleY;

        DirectX::XMFLOAT2 scaledCenteredGraphPos =
        {
            scaledGraphPosition.x - scaledGraphSize.x / 2.0f,
            scaledGraphPosition.y - scaledGraphSize.y / 2.0f
		};

        for (size_t i = 0; i < weights.size(); ++i)
        {
            int graphIndex = getGraphIndex(weights[i]);
            DirectX::XMFLOAT2 graphPos = { scaledCenteredGraphPos.x, scaledCenteredGraphPos.y + static_cast<float>(i) * scaledOffsetY };
            if (graphIndex >= 0 && graphIndex < GRAPH_COUNT)
            {
				//グラフの描画を投球割合の大きさに応じてグラフの横幅を変化させる

				float graphWidth = scaledGraphSize.x * (weights[i] / 100.0f); // 投球割合に応じて横幅を計算

                DrawFillGraph(graphSprite[graphIndex].get(), graphData[graphIndex].get(), context, graphPos, DirectX::XMFLOAT2(graphWidth, scaledGraphSize.y), graphAmount);

            }
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
    else if (introState == GameIntroState::ShowingIntroBoard)
    {
        DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(introBoardPosition);
        DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(introBoardSize);
        DirectX::XMFLOAT2 scaledCenteredPos =
        {
            scaledPosition.x - scaledSize.x / 2.0f,
            scaledPosition.y - scaledSize.y / 2.0f
        };

		introBoardSprite->render(
            context,
            scaledCenteredPos.x, scaledCenteredPos.y,
            scaledSize.x, scaledSize.y,
			introBoardData->color.x,
            introBoardData->color.y,
            introBoardData->color.z,
			introBoardData->color.w * introBoardAlpha,
			introBoardData->rotation);
    }

    //スキップボタンの描画
    if(introState != GameIntroState::ShowingIntroBoard && !IsPlaying()) buttonManager.Render(1.0f, ButtonManager::ButtonType::Skip);

    context->VSSetShader(spriteVS.Get(), nullptr, 0);
    context->PSSetShader(spritePS.Get(), nullptr, 0);
    context->IASetInputLayout(spriteInputLayout.Get());

    context->OMSetDepthStencilState(
        renderState->GetDepthStencilState(DepthState::TestOnly), 0);

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

void GameIntroSequence::DrawFillGraph(sprite* spr, IntroData* data, ID3D11DeviceContext* context,
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

void GameIntroSequence::DrawGUI()
{
    if (ImGui::CollapsingHeader("IntroSequence"))
    {
        //テクスチャの設定
		ImGui::DragFloat2("ParamBoard Position", &pitchParamPosition.x, 1.0f);
        ImGui::DragFloat2("ParamBoard Size", &pitchParamSize.x, 1.0f);
		ImGui::Separator();
        ImGui::DragFloat2("PitchType Font Position", &pitchTypeFontPosition.x, 1.0f);
        ImGui::DragFloat("PitchType Font Scale", &pitchTypeFontScale, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat2("PitchWeight Font Position", &pitchWeightFontPosition.x, 1.0f);
        ImGui::DragFloat("PitchWeight Font Scale", &pitchWeightFontScale, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat2("PitchType Label Position", &pitchTypeLabelPosition.x, 1.0f);
        ImGui::DragFloat("PitchType Label Scale", &pitchTypeLabelScale, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat2("PitchWeight Label Position", &pitchWeightLabelPosition.x, 1.0f);
		ImGui::DragFloat("PitchWeight Label Scale", &pitchWeightLabelScale, 0.01f, 0.01f, 10.0f);
		ImGui::DragFloat("Offset Y", &offsetY, 1.0f, 0.0f, 100.0f);
		ImGui::Separator();
		ImGui::DragFloat2("Graph Position", &graphPosition.x, 1.0f);
		ImGui::DragFloat2("Graph Size", &graphSize.x, 1.0f);

		buttonManager.DrawGUI();
    }
   
}

void GameIntroSequence::SaveToJson(nlohmann::json& json)
{
    json["pitchParamPosition"] = { pitchParamPosition.x, pitchParamPosition.y };
    json["pitchParamSize"] = { pitchParamSize.x, pitchParamSize.y };
    json["pitchTypeFontPosition"] = { pitchTypeFontPosition.x, pitchTypeFontPosition.y };
    json["pitchTypeFontScale"] = pitchTypeFontScale;
    json["pitchWeightFontPosition"] = { pitchWeightFontPosition.x, pitchWeightFontPosition.y };
    json["pitchWeightFontScale"] = pitchWeightFontScale;
    json["pitchTypeLabelPosition"] = { pitchTypeLabelPosition.x, pitchTypeLabelPosition.y };
    json["pitchTypeLabelScale"] = pitchTypeLabelScale;
    json["pitchWeightLabelPosition"] = { pitchWeightLabelPosition.x, pitchWeightLabelPosition.y };
    json["pitchWeightLabelScale"] = pitchWeightLabelScale;
	json["offsetY"] = offsetY;
	json["graphPosition"] = { graphPosition.x, graphPosition.y };
	json["graphSize"] = { graphSize.x, graphSize.y };

	buttonManager.SaveToJson(json["buttonManager"]);
}

void GameIntroSequence::LoadFromJson(const nlohmann::json& json)
{
    if (json.contains("pitchParamPosition"))
    {
        pitchParamPosition.x = json["pitchParamPosition"][0];
        pitchParamPosition.y = json["pitchParamPosition"][1];
    }
    if (json.contains("pitchParamSize"))
    {
        pitchParamSize.x = json["pitchParamSize"][0];
        pitchParamSize.y = json["pitchParamSize"][1];
    }
    if (json.contains("pitchTypeFontPosition"))
    {
        pitchTypeFontPosition.x = json["pitchTypeFontPosition"][0];
        pitchTypeFontPosition.y = json["pitchTypeFontPosition"][1];
    }
    if (json.contains("pitchTypeFontScale"))
    {
        pitchTypeFontScale = json["pitchTypeFontScale"];
    }
    if (json.contains("pitchWeightFontPosition"))
    {
        pitchWeightFontPosition.x = json["pitchWeightFontPosition"][0];
        pitchWeightFontPosition.y = json["pitchWeightFontPosition"][1];
    }
    if (json.contains("pitchWeightFontScale"))
    {
        pitchWeightFontScale = json["pitchWeightFontScale"];
    }
    if (json.contains("pitchTypeLabelPosition"))
    {
        pitchTypeLabelPosition.x = json["pitchTypeLabelPosition"][0];
        pitchTypeLabelPosition.y = json["pitchTypeLabelPosition"][1];
    }
    if (json.contains("pitchTypeLabelScale"))
    {
        pitchTypeLabelScale = json["pitchTypeLabelScale"];
    }
    if (json.contains("pitchWeightLabelPosition"))
    {
        pitchWeightLabelPosition.x = json["pitchWeightLabelPosition"][0];
        pitchWeightLabelPosition.y = json["pitchWeightLabelPosition"][1];
    }
    if (json.contains("pitchWeightLabelScale"))
    {
        pitchWeightLabelScale = json["pitchWeightLabelScale"];
    }
    if (json.contains("offsetY"))
    {
        offsetY = json["offsetY"];
    }
    if (json.contains("graphPosition"))
    {
        graphPosition.x = json["graphPosition"][0];
        graphPosition.y = json["graphPosition"][1];
	}
    if (json.contains("graphSize"))
    {
        graphSize.x = json["graphSize"][0];
        graphSize.y = json["graphSize"][1];
	}
    if (json.contains("buttonManager"))
    {
        buttonManager.LoadFromJson(json["buttonManager"]);
    }
}