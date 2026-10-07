#include "Result.h"
#include "Graphics.h"
#include "imgui.h"
#include "sceneTransition.h"
#include "Money.h"
#include "Combo.h"
#include "RoundManager.h"
#include "ReplayManager.h"
#include "UiEasing.h"

void Result::Initialize(ID3D11Device* device)
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
	// スプライトの初期化
	resultSpriteData = std::make_unique<Sprite>();
	resultSpriteData->texturePath = L".\\resources\\textures\\scrollViewBack.png";
	resultSpriteData->position = { spritePosition.x, spritePosition.y };
	resultSpriteData->size = { spriteSize.x, spriteSize.y };
	resultSpriteData->rotation = 0.0f;
	resultSpriteData->color = { spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w };
	resultSprite = std::make_unique<sprite>(device, context, resultSpriteData->texturePath.c_str());

	replayTrackingBoard = std::make_unique<Sprite>();
	replayTrackingBoard->texturePath = L".\\resources\\textures\\replayTrackingBoard.png";
	replayTrackingBoard->position = { replayTrackingPosition.x, replayTrackingPosition.y };
	replayTrackingBoard->size = { replayTrackingSize.x, replayTrackingSize.y };
	replayTrackingBoard->rotation = 0.0f;
	replayTrackingBoard->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	replayTrackingSprite = std::make_unique<sprite>(device, context, replayTrackingBoard->texturePath.c_str());

	trackingArrow = std::make_unique<Sprite>();
	trackingArrow->texturePath = L".\\resources\\textures\\trackingArrow.png";
	trackingArrow->position = { trackingArrowPosition.x, trackingArrowPosition.y };
	trackingArrow->size = { trackingArrowSize.x, trackingArrowSize.y };
	trackingArrow->rotation = 0.0f;
	trackingArrow->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	trackingArrowSprite = std::make_unique<sprite>(device, context, trackingArrow->texturePath.c_str());

	replayLogo = std::make_unique<Sprite>();
	replayLogo->texturePath = L".\\resources\\textures\\replayLogo.png";
	replayLogo->position = { replayLogoAnimation.startPosition.x, replayLogoAnimation.startPosition.y };
	replayLogo->size = { replayLogoAnimation.startSize.x, replayLogoAnimation.startSize.y };
	replayLogo->rotation = 0.0f;
	replayLogo->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	replayLogoSprite = std::make_unique<sprite>(device, context, replayLogo->texturePath.c_str());

	// フォントレンダラーの初期化
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	std::vector<int> resultCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"総ホームラン数0123456789本"
		u8"最高飛距離m"
	u8"所持金G"
	u8"最大コンボ数"
	u8"到達ラウンド数"
	u8"km/h°");
	resultFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f,
		screenWidth, screenHeight,
		4096.0f, 4096.0f,
		&resultCodepoints);

	buttonManager.Initialize();

	hexTransitionEffect.Initialize();

	isResultToTitle = false;
	isResultToBatterSelect = false;
	isResultToRetry = false;

	hasEnteredResult = false;
	hasReplay = false;

	replayLogoMoveTime = 0.0f;
	isReplayLogoMovingHalf = false;

	currentState = State::Replay;
}

void Result::Uninitialize()
{
	ReplayManager::Instance().StopPlayback(); //再生停止
	hasEnteredResult = false; // 次のプレイでまた検知できるようにリセット
	resultFont.Uninitialize();
	resultSprite.reset();
	resultSpriteData.reset();
	replayTrackingBoard.reset();
	replayTrackingSprite.reset();
	hexTransitionEffect.Reset();
	buttonManager.Uninitialize();
}

void Result::Update(float elapsedTime)
{
	bool isGameFinished = RoundManager::Instance().IsGameClear() || RoundManager::Instance().IsGameOver();

	if(isGameFinished && !hasEnteredResult && currentState == State::Replay && isReplayLogoMovingHalf)
	{

		hasEnteredResult = true;
		replayLogoMoveTime = 0.0f;

		hasReplay = ReplayManager::Instance().HasSavedReplay();//再生可能なリプレイがあるかどうかを取得
		if (hasReplay)
		{
			ReplayManager::Instance().SetLoopPlayback(true); //ループ再生を有効化
			ReplayManager::Instance().StartPlayback(); //再生開始

			broadcastCamera.SetReplayMode(true);
		}
	}

	if (currentState == State::Replay) UpdateReplayLogoPosition(elapsedTime);

	//リプレイの再生を進める
	if (isGameFinished && hasReplay && ReplayManager::Instance().IsPlaying() && isReplayLogoMovingHalf)
	{
		ReplayManager::Instance().UpdatePlayback(elapsedTime);
		const ReplayFrame& frame = ReplayManager::Instance().GetCurrentPlaybackFrame();

		//リプレイのフレーム情報を使って、必要な処理を行う
		Pitcher::Instance().SetPosition(DirectX::XMFLOAT3(frame.pitcherPosition));
		Pitcher::Instance().SetAngle(DirectX::XMFLOAT3(frame.pitcherRotation));
		Pitcher::Instance().SetAnimationState(frame.pitcherCurrentAnimationIndex, frame.pitcherAnimationTime);
		Pitcher::Instance().SetIsBallThrown(frame.hasThrownBall);

		Player::Instance().SetPosition(DirectX::XMFLOAT3(frame.batterPosition));
		Player::Instance().SetAngle(DirectX::XMFLOAT3(frame.batterRotation));
		Player::Instance().SetAnimationState(frame.batterCurrentAnimationIndex, frame.batterAnimationTime);

		Ball::Instance().SetHasCollidedWithBat(frame.hasCollidedWithBat);

		Ball::Instance().SetWorldPosition(DirectX::XMFLOAT3(frame.ballPosition));
		Ball::Instance().SetVelocity(DirectX::XMFLOAT3(frame.ballVelocity));
		Ball::Instance().SetRotationQuat(DirectX::XMFLOAT4(frame.ballRotation));

		//打球速度と打球角度
		Physics::Instance().SetBallSpeed(frame.ballSpeedKmh);
		Physics::Instance().SetBallAngle(frame.ballLaunchAngleDegrees);
	}


	switch (currentState)
	{
		case State::Replay:
		{
			//仮で右クリックを押したらリザルト画面に遷移するようにする
			if(GetAsyncKeyState(VK_RBUTTON) & 0x8000)
			{
				currentState = State::Result;
			}
			break;
		}

		case State::Result:
		{
			buttonManager.Update(elapsedTime);

			//タイトルシーンに戻るボタンの更新処理
			if (!isResultToTitle && !isResultToRetry && !isResultToBatterSelect)
			{
				if (buttonManager.IsTitleRequested())
				{
					isResultToTitle = true;
					hexTransitionEffect.Start(1.0f);
					buttonManager.ResetTitleRequest(false);
					currentState = State::Transition;

				}
				if (buttonManager.IsRetryRequested())
				{
					isResultToRetry = true;
					hexTransitionEffect.Start(1.0f);
					buttonManager.ResetRetryRequest(false);
					currentState = State::Transition;

				}
				if (buttonManager.IsBatterSelectRequested())
				{
					isResultToBatterSelect = true;
					hexTransitionEffect.Start(1.0f);
					buttonManager.ResetBatterSelectRequest(false);
					currentState = State::Transition;

				}
			}

			break;
		}

		case State::Transition:
		{
			hexTransitionEffect.Update(elapsedTime);

			if (hexTransitionEffect.IsFinished())
			{
				if (isResultToTitle)
				{
					ChangeSceneGameToTitle();
				}
				else if (isResultToRetry)
				{
					ChangeSceneGameToGame();
				}
				else if (isResultToBatterSelect)
				{
					ChangeSceneGameToBatterSelect();
				}
			}
			break;
		}
	}

	
	
}

void Result::UpdateReplayLogoPosition(float elapsedTime)
{
	if (replayLogo)
	{
		replayLogoMoveTime += elapsedTime;
		float t = replayLogoMoveTime / replayLogoMoveDuration;
		if (t > 1.0f) t = 1.0f;

		// 線形補間で位置を更新
		//右側からスケールを大きくして真ん中へ移動→
		// 真ん中付近でゆっくりになって一定時間がたったら早く左側へ移動
		//スケールも小さくする

		replayLogoAnimation.currentPosition = UiEasing::Lerp(
			replayLogoAnimation.startPosition, 
			replayLogoAnimation.endPosition,
			t,UiEasing::EasingType::InOutSine);
		replayLogoAnimation.currentSize = UiEasing::Lerp(
			replayLogoAnimation.startSize,
			replayLogoAnimation.endSize,
			t, UiEasing::EasingType::InOutSine);

		if(t < 0.5f)
		{
			float t1 = t / 0.5f; // 0.0から0.5秒の間を0.0から1.0に正規化

			replayLogoAnimation.currentPosition = UiEasing::Lerp(
				replayLogoAnimation.startPosition,
				replayLogoAnimation.targetPosition,
				t1, UiEasing::EasingType::OutQuad);

			replayLogoAnimation.currentSize = UiEasing::Lerp(
				replayLogoAnimation.startSize,
				replayLogoAnimation.targetSize,
				t1, UiEasing::EasingType::OutQuad);

			isReplayLogoMovingHalf = true;
		}
		else
		{
			float t2 = (t - 0.5f) / 0.5f; // 0.5から1.0秒の間を0.0から1.0に正規化
			replayLogoAnimation.currentPosition = UiEasing::Lerp(
				replayLogoAnimation.targetPosition,
				replayLogoAnimation.endPosition,
				t2, UiEasing::EasingType::InQuad);

			replayLogoAnimation.currentSize = UiEasing::Lerp(
				replayLogoAnimation.targetSize,
				replayLogoAnimation.endSize,
				t2, UiEasing::EasingType::InQuad);
		}

		
	}
}

void Result::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ScreenScaler& screenScaler = Graphics::Instance().GetScreenScaler();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨
	
	const auto& savedList = ReplayManager::Instance().GetSavedReplayList();//保存済みのリプレイデータを取得
	const ReplayFrame& frame = ReplayManager::Instance().GetCurrentPlaybackFrame();//現在の再生時間における補間済みフレームを取得

	if(replayTrackingBoard && replayTrackingSprite && currentState == State::Replay)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(replayTrackingPosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(replayTrackingSize);

		DirectX::XMFLOAT2 scaledCenterPos =
		{
			scaledPosition.x - scaledSize.x / 2.0f,
			scaledPosition.y - scaledSize.y / 2.0f
		};

		replayTrackingSprite->render(dc,
			scaledCenterPos.x, scaledCenterPos.y,
			scaledSize.x, scaledSize.y,
			replayTrackingBoard->color.x, replayTrackingBoard->color.y, replayTrackingBoard->color.z, replayTrackingBoard->color.w,
			replayTrackingBoard->rotation);
	}

	auto scaledText = [&](const DirectX::XMFLOAT2& scaledPos, float scaledSize) -> std::pair<DirectX::XMFLOAT2, float>
		{
			return {
				screenScaler.Scale(scaledPos),
				scaledSize * screenScaler.GetUniformScale()
			};
		};

	//保存した打球速度と打球角度を表示
	if(resultFont.IsValid() && currentState == State::Replay)
	{
		

		float targetSpeed = savedList.back().ballSpeedKmh;//保存済みのリプレイデータの最後のフレームの打球速度を取得
		float targetAngle = savedList.back().ballLaunchAngleDegrees;//保存済みのリプレイデータの最後のフレームの打球角度を取得

		float progress = 1.0f; // デフォルトで1.0に設定

		if(!ReplayManager::Instance().HasEverLooped())
		{
			float currentPlaybackTime = frame.time;
			float duration = 0.3f; // 0.3秒間で変化させる

			progress = currentPlaybackTime / duration;
			if (progress < 0.0f) progress = 0.0f;
			if (progress > 1.0f) progress = 1.0f;
		}

		int displayedSpeed = static_cast<int>(targetSpeed * progress);
		int displayedAngle = static_cast<int>(targetAngle * progress);

		std::string speedText = std::to_string(displayedSpeed) + u8"km/h";
		std::string angleText = std::to_string(displayedAngle) + u8"°";
		//打球速度の描画
		auto [scaledSpeedFontPosition, scaledSpeedFontSize] = scaledText(speedFontPosition, speedFontSize);

		float speedTextWidth, speedTextHeight;
		resultFont.MeasureText(speedText.c_str(), scaledSpeedFontSize, speedTextWidth, speedTextHeight);
		float drawSpeedX = scaledSpeedFontPosition.x - speedTextWidth / 2.0f;
		float drawSpeedY = scaledSpeedFontPosition.y - speedTextHeight / 2.0f;

		resultFont.DrawTextW(dc, speedText.c_str(),
			drawSpeedX, drawSpeedY,
			scaledSpeedFontSize,
			1.0f, 1.0f, 1.0f, 1.0f);

		//打球角度の描画
		auto [scaledAngleFontPosition, scaledAngleFontSize] = scaledText(angleFontPosition, angleFontSize);

		float angleTextWidth, angleTextHeight;
		resultFont.MeasureText(angleText.c_str(), scaledAngleFontSize, angleTextWidth, angleTextHeight);
		float drawAngleX = scaledAngleFontPosition.x - angleTextWidth / 2.0f;
		float drawAngleY = scaledAngleFontPosition.y - angleTextHeight / 2.0f;

		resultFont.DrawTextW(dc, angleText.c_str(),
			drawAngleX, drawAngleY,
			scaledAngleFontSize,
			1.0f, 1.0f, 1.0f, 1.0f);

	}

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨

	if(trackingArrow && trackingArrowSprite && currentState == State::Replay && isReplayLogoMovingHalf)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(trackingArrowPosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(trackingArrowSize);
		DirectX::XMFLOAT2 scaledCenterPos =
		{
			scaledPosition.x - scaledSize.x / 2.0f,
			scaledPosition.y - scaledSize.y / 2.0f
		};

		float targetAngle = savedList.back().ballLaunchAngleDegrees; //保存済みのリプレイデータの最後のフレームの打球角度を取得

		float progress = 1.0f; // デフォルトで1.0に設定

		if (!ReplayManager::Instance().HasEverLooped())
		{
			float currentPlaybackTime = frame.time;
			float duration = 0.3f; // 0.3秒間で変化させる

			progress = currentPlaybackTime / duration;
			if (progress < 0.0f) progress = 0.0f;
			if (progress > 1.0f) progress = 1.0f;
		}

		// 角度の補間
		float displayedAngle = targetAngle * progress;

		float centerX = scaledCenterPos.x + scaledSize.x / 2.0f; // 中心のX座標
		float centerY = scaledCenterPos.y + scaledSize.y / 2.0f; // 中心のY座標

		//支点を右端に移動差焦る
		float pivotX = scaledCenterPos.x + scaledSize.x; // 右端に移動するためのオフセット
		float pivotY = centerY; // 中心のY座標を維持

		float radianAngle = DirectX::XMConvertToRadians(displayedAngle);

		float dx = centerX - pivotX;
		float dy = centerY - pivotY;

		// 回転後の座標を計算
		float rotatedX = pivotX + (dx * cos(radianAngle) - dy * sin(radianAngle));
		float rotatedY = pivotY + (dx * sin(radianAngle) + dy * cos(radianAngle));

		// 回転後の座標を描画位置として使用
		float finalDrawX = rotatedX - scaledSize.x / 2.0f; // 中心に戻すために半分の幅を引く
		float finalDrawY = rotatedY - scaledSize.y / 2.0f; // 中心に戻すために半分の高さを引く

		trackingArrowSprite->render(dc,
			finalDrawX, finalDrawY,
			scaledSize.x, scaledSize.y,
			trackingArrow->color.x, trackingArrow->color.y, trackingArrow->color.z, trackingArrow->color.w,
			displayedAngle);
	}

	if(replayLogo && replayLogoSprite && currentState == State::Replay)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(replayLogoAnimation.currentPosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(replayLogoAnimation.currentSize);
		DirectX::XMFLOAT2 scaledCenterPos =
		{
			scaledPosition.x - scaledSize.x / 2.0f,
			scaledPosition.y - scaledSize.y / 2.0f
		};
		replayLogoSprite->render(dc,
			scaledCenterPos.x, scaledCenterPos.y,
			scaledSize.x, scaledSize.y,
			replayLogo->color.x, replayLogo->color.y, replayLogo->color.z, replayLogo->color.w,
			replayLogo->rotation);
	}

	// スプライトの描画
	/*if (resultSprite)
	{
		DirectX::XMFLOAT2 scaledPosition = screenScaler.Scale(spritePosition);
		DirectX::XMFLOAT2 scaledSize = screenScaler.ScaleSize(spriteSize);

		resultSprite->render(dc,
			scaledPosition.x, scaledPosition.y,
			scaledSize.x, scaledSize.y,
			spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w,
			resultSpriteData->rotation);
	}*/
	// フォントの描画
	if (resultFont.IsValid() && (currentState != State::Replay))
	{

		float spacingY = 100.0f; // 各テキストの垂直間隔

		struct TextInfo
		{
			std::string label;
			std::string value;
			DirectX::XMFLOAT2 position;
			float fontSize;
			DirectX::XMFLOAT4 color;
		};

		TextInfo textInfos[] = {
			
			{ u8"総ホームラン数: ", std::to_string(HomeRunCount::Instance().GetTotalHomeRunCount()) + u8"本", homeRunFontPosition, homeRunFontSize, homeRunFontColor },
			{ u8"最高飛距離: ", std::to_string(static_cast<int>(BallDistance::Instance().GetMaxDistance())) + u8"m", distanceFontPosition, distanceFontSize, distanceFontColor },
			{ u8"最大コンボ数: ", std::to_string(Combo::Instance().GetMaxCombo()), comboFontPosition, comboFontSize, comboFontColor },
			{ u8"到達ラウンド数: ", std::to_string(RoundManager::Instance().GetCurrentRound()), roundFontPosition, roundFontSize, roundFontColor }
		};

		auto [scaledMoneyFontPosition, scaledMoneyFontSize] = scaledText(moneyFontPosition, moneyFontSize);

		std::string moneyText = u8"所持金: " + std::to_string(Money::Instance().GetCurrentMoney()) + u8"G";

		float moneyTextWidth, moneyTextHeight;
		resultFont.MeasureText(moneyText.c_str(), scaledMoneyFontSize, moneyTextWidth, moneyTextHeight);
		float drawX = scaledMoneyFontPosition.x - moneyTextWidth / 2.0f;
		float drawY = scaledMoneyFontPosition.y - moneyTextHeight / 2.0f;

		resultFont.DrawTextW(dc, moneyText.c_str(),
			drawX, drawY,
			scaledMoneyFontSize,
			moneyFontColor.x, moneyFontColor.y, moneyFontColor.z, moneyFontColor.w);


		for(const auto& textInfo : textInfos)
		{
			float textWidth, textHeight;

			auto [scaledPosition, scaledFontSize] = scaledText(textInfo.position, textInfo.fontSize);

			// テキストの幅と高さを計算して中央揃えの位置を決定
			resultFont.MeasureText(textInfo.label.c_str(), scaledFontSize, textWidth, textHeight);
			float labelX = scaledPosition.x - textWidth / 2.0f;
			float labelY = scaledPosition.y - textHeight / 2.0f;

			// テキストを描画
			resultFont.DrawTextW(dc,textInfo.label.c_str(),
				labelX, labelY, 
				scaledFontSize, 
				textInfo.color.x, textInfo.color.y, textInfo.color.z, textInfo.color.w);

			resultFont.MeasureText(textInfo.value.c_str(), scaledFontSize, textWidth, textHeight);
			float valueX = scaledPosition.x - textWidth / 2.0f; // ラベルの右側に配置
			float valueY = labelY + spacingY; // ラベルの下に配置

			resultFont.DrawTextW(dc,textInfo.value.c_str(),
				valueX, valueY, 
				scaledFontSize, 
				textInfo.color.x, textInfo.color.y, textInfo.color.z, textInfo.color.w);
		}
		

		
	}

	if((currentState != State::Replay))
	{
		buttonManager.Render(1.0f, ButtonManager::ButtonType::Title);
		buttonManager.Render(1.0f, ButtonManager::ButtonType::Retry);
		buttonManager.Render(1.0f, ButtonManager::ButtonType::BatterSelect);
	}


	if(isResultToTitle || isResultToRetry || isResultToBatterSelect)
	{
		hexTransitionEffect.Render();
	}

	// 描画後の状態をリセット
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

}

void Result::DrawGUI()
{
	if(ImGui::CollapsingHeader("Result Settings"))
	{
		ImGui::DragFloat2("Money Font Position", &moneyFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Money Font Size", &moneyFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Money Font Color", &moneyFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Home Run Font Position", &homeRunFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Home Run Font Size", &homeRunFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Home Run Font Color", &homeRunFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Distance Font Position", &distanceFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Distance Font Size", &distanceFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Distance Font Color", &distanceFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Combo Font Position", &comboFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Combo Font Size", &comboFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Combo Font Color", &comboFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Round Font Position", &roundFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Round Font Size", &roundFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::ColorEdit4("Round Font Color", &roundFontColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Result Sprite Position", &spritePosition.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat2("Result Sprite Size", &spriteSize.x, 0.01f, 0.0f, 1.0f);
		ImGui::ColorEdit4("Result Sprite Color", &spriteColor.x);

		ImGui::Separator();
		ImGui::DragFloat2("Replay Tracking Position", &replayTrackingPosition.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat2("Replay Tracking Size", &replayTrackingSize.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat2("Speed Font Position", &speedFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Speed Font Size", &speedFontSize, 0.1f, 10.0f, 100.0f);
		ImGui::DragFloat2("Angle Font Position", &angleFontPosition.x, 0.1f, 0.0f, 1920.0f);
		ImGui::DragFloat("Angle Font Size", &angleFontSize, 0.1f, 10.0f, 100.0f);

		ImGui::Separator();
		ImGui::DragFloat2("Tracking Arrow Position", &trackingArrowPosition.x, 0.01f, 0.0f, 2000.0f);
		ImGui::DragFloat2("Tracking Arrow Size", &trackingArrowSize.x, 0.01f, 0.0f, 2000.0f);
	}

	buttonManager.DrawGUI();
}

void Result::SaveToJson(nlohmann::json& j)
{
	j["MoneyFontPosition"] = { moneyFontPosition.x, moneyFontPosition.y };
	j["MoneyFontSize"] = moneyFontSize;
	j["MoneyFontColor"] = { moneyFontColor.x, moneyFontColor.y, moneyFontColor.z, moneyFontColor.w };
	j["HomeRunFontPosition"] = { homeRunFontPosition.x, homeRunFontPosition.y };
	j["HomeRunFontSize"] = homeRunFontSize;
	j["HomeRunFontColor"] = { homeRunFontColor.x, homeRunFontColor.y, homeRunFontColor.z, homeRunFontColor.w };
	j["DistanceFontPosition"] = { distanceFontPosition.x, distanceFontPosition.y };
	j["DistanceFontSize"] = distanceFontSize;
	j["DistanceFontColor"] = { distanceFontColor.x, distanceFontColor.y, distanceFontColor.z, distanceFontColor.w };
	j["ResultSpritePosition"] = { spritePosition.x, spritePosition.y };
	j["ResultSpriteSize"] = { spriteSize.x, spriteSize.y };
	j["ResultSpriteColor"] = { spriteColor.x, spriteColor.y, spriteColor.z, spriteColor.w };
	j["ComboFontPosition"] = { comboFontPosition.x, comboFontPosition.y };
	j["ComboFontSize"] = comboFontSize;
	j["ComboFontColor"] = { comboFontColor.x, comboFontColor.y, comboFontColor.z, comboFontColor.w };
	j["RoundFontPosition"] = { roundFontPosition.x, roundFontPosition.y };
	j["RoundFontSize"] = roundFontSize;
	j["RoundFontColor"] = { roundFontColor.x, roundFontColor.y, roundFontColor.z, roundFontColor.w };
	j["ReplayTrackingPosition"] = { replayTrackingPosition.x, replayTrackingPosition.y };
	j["ReplayTrackingSize"] = { replayTrackingSize.x, replayTrackingSize.y };
	j["SpeedFontPosition"] = { speedFontPosition.x, speedFontPosition.y };
	j["SpeedFontSize"] = speedFontSize;
	j["AngleFontPosition"] = { angleFontPosition.x, angleFontPosition.y }; 
	j["AngleFontSize"] = angleFontSize;
	j["TrackingArrowPosition"] = { trackingArrowPosition.x, trackingArrowPosition.y };
	j["TrackingArrowSize"] = { trackingArrowSize.x, trackingArrowSize.y };

	buttonManager.SaveToJson(j);
}

void Result::LoadFromJson(const nlohmann::json& j)
{
	if(j.contains("MoneyFontPosition"))
	{
		moneyFontPosition.x = j["MoneyFontPosition"][0].get<float>();
		moneyFontPosition.y = j["MoneyFontPosition"][1].get<float>();
	}
	if (j.contains("MoneyFontSize"))
	{
		moneyFontSize = j["MoneyFontSize"].get<float>();
	}
	if(j.contains("MoneyFontColor"))
	{
		moneyFontColor.x = j["MoneyFontColor"][0].get<float>();
		moneyFontColor.y = j["MoneyFontColor"][1].get<float>();
		moneyFontColor.z = j["MoneyFontColor"][2].get<float>();
		moneyFontColor.w = j["MoneyFontColor"][3].get<float>();
	}
	if (j.contains("HomeRunFontPosition"))
	{
		homeRunFontPosition.x = j["HomeRunFontPosition"][0].get<float>();
		homeRunFontPosition.y = j["HomeRunFontPosition"][1].get<float>();
	}
	if (j.contains("HomeRunFontSize"))
	{
		homeRunFontSize = j["HomeRunFontSize"].get<float>();
	}
	if (j.contains("HomeRunFontColor"))
	{
		homeRunFontColor.x = j["HomeRunFontColor"][0].get<float>();
		homeRunFontColor.y = j["HomeRunFontColor"][1].get<float>();
		homeRunFontColor.z = j["HomeRunFontColor"][2].get<float>();
		homeRunFontColor.w = j["HomeRunFontColor"][3].get<float>();
	}
	if (j.contains("DistanceFontPosition"))
	{
		distanceFontPosition.x = j["DistanceFontPosition"][0].get<float>();
		distanceFontPosition.y = j["DistanceFontPosition"][1].get<float>();
	}
	if (j.contains("DistanceFontSize"))
	{
		distanceFontSize = j["DistanceFontSize"].get<float>();
	}
	if (j.contains("DistanceFontColor"))
	{
		distanceFontColor.x = j["DistanceFontColor"][0].get<float>();
		distanceFontColor.y = j["DistanceFontColor"][1].get<float>();
		distanceFontColor.z = j["DistanceFontColor"][2].get<float>();
		distanceFontColor.w = j["DistanceFontColor"][3].get<float>();
	}
	if(j.contains("ComboFontPosition"))
	{
		comboFontPosition.x = j["ComboFontPosition"][0].get<float>();
		comboFontPosition.y = j["ComboFontPosition"][1].get<float>();
	}
	if (j.contains("ComboFontSize"))
	{
		comboFontSize = j["ComboFontSize"].get<float>();
	}
	if (j.contains("ComboFontColor"))
	{
		comboFontColor.x = j["ComboFontColor"][0].get<float>();
		comboFontColor.y = j["ComboFontColor"][1].get<float>();
		comboFontColor.z = j["ComboFontColor"][2].get<float>();
		comboFontColor.w = j["ComboFontColor"][3].get<float>();
	}
	if (j.contains("RoundFontPosition"))
	{
		roundFontPosition.x = j["RoundFontPosition"][0].get<float>();
		roundFontPosition.y = j["RoundFontPosition"][1].get<float>();
	}
	if(j.contains("RoundFontSize"))
	{
		roundFontSize = j["RoundFontSize"].get<float>();
	}
	if(j.contains("RoundFontColor"))
	{
		roundFontColor.x = j["RoundFontColor"][0].get<float>();
		roundFontColor.y = j["RoundFontColor"][1].get<float>();
		roundFontColor.z = j["RoundFontColor"][2].get<float>();
		roundFontColor.w = j["RoundFontColor"][3].get<float>();
	}
	if (j.contains("ResultSpritePosition"))
	{
		spritePosition.x = j["ResultSpritePosition"][0].get<float>();
		spritePosition.y = j["ResultSpritePosition"][1].get<float>();
	}
	if (j.contains("ResultSpriteSize"))
	{
		spriteSize.x = j["ResultSpriteSize"][0].get<float>();
		spriteSize.y = j["ResultSpriteSize"][1].get<float>();
	}
	if (j.contains("ResultSpriteColor"))
	{
		spriteColor.x = j["ResultSpriteColor"][0].get<float>();
		spriteColor.y = j["ResultSpriteColor"][1].get<float>();
		spriteColor.z = j["ResultSpriteColor"][2].get<float>();
		spriteColor.w = j["ResultSpriteColor"][3].get<float>();
	}
	if(j.contains("ReplayTrackingPosition"))
	{
		replayTrackingPosition.x = j["ReplayTrackingPosition"][0].get<float>();
		replayTrackingPosition.y = j["ReplayTrackingPosition"][1].get<float>();
	}
	if(j.contains("ReplayTrackingSize"))
	{
		replayTrackingSize.x = j["ReplayTrackingSize"][0].get<float>();
		replayTrackingSize.y = j["ReplayTrackingSize"][1].get<float>();
	}
	if(j.contains("SpeedFontPosition"))
	{
		speedFontPosition.x = j["SpeedFontPosition"][0].get<float>();
		speedFontPosition.y = j["SpeedFontPosition"][1].get<float>();
	}
	if(j.contains("SpeedFontSize"))
	{
		speedFontSize = j["SpeedFontSize"].get<float>();
	}
	if(j.contains("AngleFontPosition"))
	{
		angleFontPosition.x = j["AngleFontPosition"][0].get<float>();
		angleFontPosition.y = j["AngleFontPosition"][1].get<float>();
	}
	if(j.contains("AngleFontSize"))
	{
		angleFontSize = j["AngleFontSize"].get<float>();
	}
	if(j.contains("TrackingArrowPosition"))
	{
		trackingArrowPosition.x = j["TrackingArrowPosition"][0].get<float>();
		trackingArrowPosition.y = j["TrackingArrowPosition"][1].get<float>();
	}
	if(j.contains("TrackingArrowSize"))
	{
		trackingArrowSize.x = j["TrackingArrowSize"][0].get<float>();
		trackingArrowSize.y = j["TrackingArrowSize"][1].get<float>();
	}

	buttonManager.LoadFromJson(j);

}