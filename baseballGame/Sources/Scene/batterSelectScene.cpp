#include "batterSelectScene.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include <random>
#include "scene_loading.h"
#include "scene_game.h"
#include <fstream>

void batterSelectScene::SelectRandomPitcher()
{
	static std::mt19937 rng(std::random_device{}()); // 乱数生成器の初期化
	std::uniform_int_distribution<int> dist(
		static_cast<int>(Pitcher::RealPitcher::Nakagawa),
		static_cast<int>(Pitcher::RealPitcher::Count) - 1
	);

	selectedPitcher = static_cast<Pitcher::RealPitcher>(dist(rng));

	// マッピングテーブルを使って画像インデックスを取得
	auto it = Pitcher::pitcherToSpriteIndexTable.find(selectedPitcher);
	if (it != Pitcher::pitcherToSpriteIndexTable.end())
	{
		selectedPitcherIndex = static_cast<size_t>(it->second);
		
	}

	previousPitcherIndex = currentPitcherIndex;
	currentPitcherIndex = selectedPitcherIndex;

	//ピッチャー側にも選択されたピッチャーを設定
	Pitcher::Instance().SelectRealPitcher(selectedPitcher);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Selected Pitcher: %d (Index: %zu)", static_cast<int>(selectedPitcher), selectedPitcherIndex);
	OutputDebugStringA(buffer);

	if(currentPitcherIndex != previousPitcherIndex)
	{
		if (Pitcher::Instance().IsRightPitcher()) randomPitcherImageIndex = 2 + rand() % 2; //3か4
		else randomPitcherImageIndex = rand() % 2;//0か1
	}

	

}


void batterSelectScene::initialize()
{

	// スクロールビューの初期化
	ID3D11Device* device = Graphics::Instance().GetDevice();
	playerScrollView = std::make_unique<ScrollView>(device, 550.0f, 600.0f, 500.0f, 600.0f);

	buttonManager.Initialize();

	// added: pitch info font init (日本語対応版)
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.GetAddressOf(), input_layout.GetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.GetAddressOf());

	//背景のスプライトデータを初期化
	backGroundData = std::make_unique<BatterSelectSpriteData>();
	backGroundData->texturePath = L".\\resources\\textures\\batterSelectBack.png";
	backGroundData->position = { 0.0f, 0.0f };
	backGroundData->size = { 1920.0f, 1080.0f };
	backGroundData->rotation = 0.0f;
	backGroundData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	backGroundSprite = std::make_unique<sprite>(device, context, backGroundData->texturePath.c_str());

	//ピッチャーのパラメータ背景のスプライトデータを初期化
	pitcherParamBackGroundData = std::make_unique<BatterSelectSpriteData>();
	pitcherParamBackGroundData->texturePath = L".\\resources\\textures\\scrollViewBack.png";
	pitcherParamBackGroundData->position = { static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) / 2.0f };
	pitcherParamBackGroundData->size = { 1800.0f , 1000.0f };
	pitcherParamBackGroundData->rotation = 0.0f;
	pitcherParamBackGroundData->color = { 1.0f, 1.0f, 1.0f, 0.8f };
	pitcherParamBackGroundSprite = std::make_unique<sprite>(device, context, pitcherParamBackGroundData->texturePath.c_str());

	//閉じるボタンのスプライトデータを初期化
	closeButtonData = std::make_unique<BatterSelectSpriteData>();
	closeButtonData->texturePath = L".\\resources\\textures\\closeButton.png";
	closeButtonData->position = { 1800.0f, 100.0f };
	closeButtonData->size = { 100.0f, 100.0f };
	closeButtonData->rotation = 0.0f;
	closeButtonData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	closeButtonSprite = std::make_unique<sprite>(device, context, closeButtonData->texturePath.c_str());

	//ピッチャーのスプライトデータを初期化
	for (size_t i = 0; i < PITCHER_COUNT; ++i)
	{
		// ピッチャーのパラメータ画像のスプライトデータを初期化
		pitcherSpriteDataArray[i] = std::make_unique<BatterSelectSpriteData>();
		pitcherSpriteDataArray[i]->texturePath = L".\\resources\\textures\\pitcherParameter\\pitcherParameter" + std::to_wstring(i + 1) + L".png";
		pitcherSpriteDataArray[i]->position = { 1150.0f, 150.0f };
		pitcherSpriteDataArray[i]->size = { 550.0f, 825.0f };
		pitcherSpriteDataArray[i]->rotation = 0.0f;
		pitcherSpriteDataArray[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		pitcherSprites[i] = std::make_unique<sprite>(device, context, pitcherSpriteDataArray[i]->texturePath.c_str());

		// ピッチャーの名前タグのスプライトデータを初期化
		pitcherNameSpriteData[i] = std::make_unique<BatterSelectSpriteData>();
		pitcherNameSpriteData[i]->texturePath = L".\\resources\\textures\\pitcherNameTag\\pitcherNameTag" + std::to_wstring(i + 1) + L".png";
		pitcherNameSpriteData[i]->position = { pitcherNamePosition.x, pitcherNamePosition.y };
		pitcherNameSpriteData[i]->size = { pitcherNameSize.x, pitcherNameSize.y };
		pitcherNameSpriteData[i]->rotation = 0.0f;
		pitcherNameSpriteData[i]->color = { pitcherNameColor.x, pitcherNameColor.y, pitcherNameColor.z, pitcherNameColor.w };
		pitcherNameSprite[i] = std::make_unique<sprite>(device, context, pitcherNameSpriteData[i]->texturePath.c_str());

	}

	//ピッチャー画像のスプライトデータを初期化
	for (size_t i = 0; i < PITCHER_IMAGE_COUNT; ++i)
	{
		pitcherImageSpriteDataArray[i] = std::make_unique<BatterSelectSpriteData>();
		pitcherImageSpriteDataArray[i]->texturePath = L".\\resources\\textures\\pitcherImage\\pitcherImage" + std::to_wstring(i + 1) + L".png";
		pitcherImageSpriteDataArray[i]->position = { 1150.0f, 100.0f };
		pitcherImageSpriteDataArray[i]->size = { 600.0f, 800.0f };
		pitcherImageSpriteDataArray[i]->rotation = 0.0f;
		pitcherImageSpriteDataArray[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		pitcherImageSprites[i] = std::make_unique<sprite>(device, context, pitcherImageSpriteDataArray[i]->texturePath.c_str());
	}

	//VSのスプライトデータを初期化
	VSSpriteData = std::make_unique<BatterSelectSpriteData>();
	VSSpriteData->texturePath = L".\\resources\\textures\\VS.png";
	VSSpriteData->position = { VSPosition.x, VSPosition.y };
	VSSpriteData->size = { VSSize.x, VSSize.y };
	VSSpriteData->rotation = 0.0f;
	VSSpriteData->color = { VSColor.x, VSColor.y, VSColor.z, VSColor.w };
	VSSprite = std::make_unique<sprite>(device, context, VSSpriteData->texturePath.c_str());

	//バーストエフェクト
	{
		create_vs_from_cso(device, ".\\resources\\shader\\burstEffect_vs.cso", burstVertexShader.GetAddressOf(), nullptr, nullptr, 0);
		create_ps_from_cso(device, ".\\resources\\shader\\burstEffect_ps.cso", burstPixelShader.GetAddressOf());

		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		desc.ByteWidth = sizeof(BurstTransformBuffer);
		device->CreateBuffer(&desc, nullptr, burstTransformBuffer.GetAddressOf());

		desc.ByteWidth = sizeof(BurstBuffer);
		device->CreateBuffer(&desc, nullptr, burstColorBuffer.GetAddressOf());

	}

	currentState = SequenceState::Selecting;
	uiAlpha = 1.0f;
	returnAlpha = 0.0f;
	batterImageAlpha = 0.0f;
	transitionTimer = 0.0f;

	burstList = {
		{ {1500.0f, 550.0f}, {700.0f, 700.0f}, 1.0f, 0.0f },
		{ {300.0f, 550.0f}, {700.0f, 700.0f}, 1.0f, 0.5f }
	};

	hexTransitionEffect.Initialize();
	isChangingScene = false;

	SelectRandomPitcher(); // ランダムにピッチャーを選択
	LoadSetting(); // 設定をロード
}

void batterSelectScene::update(float elapsed_time)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		SaveSetting();
	}

	// ボタンマネージャーの更新は、選手選択中または遷移完了後にのみ行う
	if(currentState == SequenceState::Selecting || currentState == SequenceState::Finished || currentState == SequenceState::ShowPitcherParam)
	{
		buttonManager.Update(elapsed_time);
	}
	
	

	//ステートに応じた処理
	switch (currentState)
	{
		case SequenceState::Selecting:
		{
			uiAlpha = 1.0f;
			returnAlpha = 0.0f;
			batterImageAlpha = 0.0f;
			for (auto& effect : burstList)
			{
				effect.alpha = 0.0f; // 選択中はエフェクトを非表示
			}

			playerScrollView->Update(elapsed_time);

			if (buttonManager.IsOKRequested())
			{
				currentState = SequenceState::Transition;
				buttonManager.ResetOKRequest(false);
				transitionTimer = 0.0f; // 遷移演出のタイマーをリセット
			}
			break;
		}
		case SequenceState::Transition:
		{
			transitionTimer += elapsed_time;

			const float uiFadeDuration = 0.4f;    // UIがフェードアウトする時間
			const float burstFadeDuration = 0.4f; // 光エフェクトがフェードインする時間

			// 前半: UIのフェードアウト (0 ～ uiFadeDuration)
			float uiProgress = std::clamp(transitionTimer / uiFadeDuration, 0.0f, 1.0f);
			uiAlpha = 1.0f - uiProgress;
			

			// 後半: UIが消え終わってから光エフェクトをフェードイン
			float burstProgress = std::clamp((transitionTimer - uiFadeDuration) / burstFadeDuration, 0.0f, 1.0f);
			for (auto& effect : burstList)
			{
				effect.alpha = burstProgress;
			}
			returnAlpha = burstProgress;
			batterImageAlpha = burstProgress;
			if (transitionTimer >= uiFadeDuration + burstFadeDuration)
			{
				currentState = SequenceState::Finished;
			}
			break;
		}
		case SequenceState::Finished:
		{
			// 遷移完了後の処理
			uiAlpha = 0.0f;
			returnAlpha = 1.0f;
			batterImageAlpha = 1.0f;
			paramImageAlpha = 0.0f;
			for (auto& effect : burstList)
			{
				effect.alpha = 1.0f; // 光エフェクトを完全に表示
			}

			if (buttonManager.IsReturnRequested())
			{
				currentState = SequenceState::Reverting;
				buttonManager.ResetReturnRequest(false);
				transitionTimer = 0.0f; // 遷移演出のタイマーをリセット
			}

			if (buttonManager.IsRerollRequested())
			{			
				buttonManager.ResetRerollRequest(false);


				//3回まで投手変更をできる
				if (currentChangePitcherCount < maxChangePitcherCount)
				{
					SelectRandomPitcher();
					currentChangePitcherCount++;
					
					//ボタンの色をグレーにする
					if (currentChangePitcherCount >= maxChangePitcherCount)
					{
						buttonManager.ChangeColor(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f), ButtonManager::ButtonType::Reroll); // ボタンをグレーアウト
					}
				}
			}

			//imageAlphaが0.0の時はピッチャーのパラメータ画像を表示しない
			if (batterImageAlpha > 0.0f)
			{
				//マウスの位置を取得
				Input& input = Input::Instance();
				float mouseX = input.GetMouse().GetPositionX();
				float mouseY = input.GetMouse().GetPositionY();

				bool isLeftMouseButtonClicked = input.GetMouse().GetButtonDown();

				//ピッチャーのネームタグを押すとパラメータの画像を確認できるようにする
				isNameTagClicked = pitcherNamePosition.x <= mouseX && mouseX <= pitcherNamePosition.x + pitcherNameSize.x &&
					pitcherNamePosition.y <= mouseY && mouseY <= pitcherNamePosition.y + pitcherNameSize.y &&
					isLeftMouseButtonClicked;

				//ネームタグが押されたら、フェードインしてパラメータ画像を表示する
				if (isNameTagClicked)
				{
					transitionTimer = 0.0f; // 遷移演出のタイマーをリセット
					currentState = SequenceState::ShowPitcherParam;
				}
			}

			// シーン遷移中の処理
			if (!isChangingScene)
			{
				if (buttonManager.IsStartRequested())
				{
					isChangingScene = true;
					hexTransitionEffect.Start(1.0f);
					buttonManager.ResetStartRequest(false);
					currentState = SequenceState::ChangeScene;
				}
			}

			break;
		}
		case SequenceState::Reverting:
		{
			transitionTimer += elapsed_time;
			const float uiFadeDuration = 0.4f;    // UIがフェードアウトする時間
			const float burstFadeDuration = 0.4f; // 光エフェクトがフェードインする時間
			// 前半: 光エフェクトのフェードアウト (0 ～ burstFadeDuration)
			float burstProgress = std::clamp(transitionTimer / burstFadeDuration, 0.0f, 1.0f);
			for (auto& effect : burstList)
			{
				effect.alpha = 1.0f - burstProgress;
			}
			returnAlpha = 1.0f - burstProgress;
			batterImageAlpha = 1.0f - burstProgress;
			// 後半: 光エフェクトが消え終わってからUIをフェードイン
			float uiProgress = std::clamp((transitionTimer - burstFadeDuration) / uiFadeDuration, 0.0f, 1.0f);
			uiAlpha = uiProgress;
			if (transitionTimer >= uiFadeDuration + burstFadeDuration)
			{
				currentState = SequenceState::Selecting;
				uiAlpha = 1.0f;
				returnAlpha = 0.0f;
				batterImageAlpha = 0.0f;
				for (auto& effect : burstList)
				{
					effect.alpha = 0.0f; // 選択中はエフェクトを非表示
				}
			}
			break;
		}
		case SequenceState::ShowPitcherParam:
		{
			paramImageAlpha = 1.0f;
			returnAlpha = 0.0f;


			buttonManager.ResetReturnRequest(false);
			buttonManager.ResetOKRequest(false);
			buttonManager.ResetStartRequest(false);

			if (buttonManager.IsCloseRequested())
			{
				currentState = SequenceState::Finished;
				buttonManager.ResetCloseRequest(false);
				transitionTimer = 0.0f; // 遷移演出のタイマーをリセット
			}

			break;
		}
		case SequenceState::ChangeScene:
		{
			
			if(isChangingScene)
			{
				hexTransitionEffect.Update(elapsed_time);

				if (hexTransitionEffect.IsFinished())
				{
					sceneManager::Instance().ChangeScene(new scene_loading(new scene_game()));
				}
			}
			break;
		}
	}

	for(auto& effect : burstList)
	{
		effect.time += elapsed_time;
	}

}

void batterSelectScene::render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨

	if (backGroundData && backGroundSprite)
	{

		backGroundSprite->render(dc, backGroundData->position.x, backGroundData->position.y,
			backGroundData->size.x, backGroundData->size.y,
			backGroundData->color.x, backGroundData->color.y, backGroundData->color.z, backGroundData->color.w,
			backGroundData->rotation);
	}


	if (uiAlpha > 0.001f)
	{
		if (playerScrollView)
		{
			playerScrollView->Render(uiAlpha); // この中でVS/PS/InputLayoutがnullptrに戻る
		}
	}

	buttonManager.Render(uiAlpha, ButtonManager::ButtonType::OK);

	dc->VSSetShader(burstVertexShader.Get(), nullptr, 0);
	dc->PSSetShader(burstPixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(nullptr); // 頂点バッファ不使用なのでレイアウトも不要
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetConstantBuffers(0, 1, burstTransformBuffer.GetAddressOf());
	dc->PSSetConstantBuffers(0, 1, burstColorBuffer.GetAddressOf());

	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);

	for(const auto& effect : burstList )

	{
		if (effect.alpha > 0.001f && currentState !=  SequenceState::ShowPitcherParam)
		{


			D3D11_MAPPED_SUBRESOURCE mapped;

			dc->Map(burstTransformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

			// バーストエフェクトの中心位置をスクリーン座標に変換
			auto* tcb = reinterpret_cast<BurstTransformBuffer*>(mapped.pData);
			tcb->center = effect.position;
			tcb->size = effect.size;
			tcb->screenSize = { static_cast<float>(Graphics::Instance().GetScreenWidth()), static_cast<float>(Graphics::Instance().GetScreenHeight()) };
			dc->Unmap(burstTransformBuffer.Get(), 0);

			dc->Map(burstColorBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			auto* ccb = reinterpret_cast<BurstBuffer*>(mapped.pData);
			ccb->time = effect.time;
			ccb->aspectRatio = 1.0f; // アスペクト比を1.0に設定
			ccb->progress = effect.alpha;
			dc->Unmap(burstColorBuffer.Get(), 0);



			dc->Draw(4, 0); // 頂点バッファなしで4頂点描画（トライアングルストリップ）

			
		}
	}

	//パラメーター出現中のエフェクト描画
	if (isNameTagClicked && paramImageAlpha > 0.001f)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;

		dc->Map(burstTransformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

		// バーストエフェクトの中心位置をスクリーン座標に変換
		auto* tcb = reinterpret_cast<BurstTransformBuffer*>(mapped.pData);
		tcb->center = modalBurstPosition;
		tcb->size = modalBurstSize;
		tcb->screenSize = { static_cast<float>(Graphics::Instance().GetScreenWidth()), static_cast<float>(Graphics::Instance().GetScreenHeight()) };
		dc->Unmap(burstTransformBuffer.Get(), 0);

		dc->Map(burstColorBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		auto* ccb = reinterpret_cast<BurstBuffer*>(mapped.pData);
		ccb->time = burstList.empty() ? 0.0f : burstList[0].time;
		ccb->aspectRatio = 1.0f; // アスペクト比を1.0に設定
		ccb->progress = paramImageAlpha;
		dc->Unmap(burstColorBuffer.Get(), 0);

		dc->Draw(4, 0); // 頂点バッファなしで4頂点描画（トライアングルストリップ）
	}

	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	buttonManager.Render(returnAlpha, ButtonManager::ButtonType::Return);
	buttonManager.Render(returnAlpha, ButtonManager::ButtonType::Start);
	buttonManager.Render(returnAlpha, ButtonManager::ButtonType::Reroll);

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	if(VSSpriteData && VSSprite)
	{
		VSSprite->render(dc, VSPosition.x, VSPosition.y,
			VSSize.x, VSSize.y,
			VSColor.x, VSColor.y, VSColor.z, VSColor.w * batterImageAlpha,
			VSSpriteData->rotation);
	}

	if(batterImageAlpha > 0.001f)
	{
		playerScrollView->RenderBatterImage(dc, batterImageAlpha); // この中でVS/PS/InputLayoutがnullptrに戻る
	}

	//ピッチャーの利き手が右投げならimage3か4、左投げならimage1か2を描画する
	if (randomPitcherImageIndex >= 0)
	{
		pitcherImageSprites[randomPitcherImageIndex]->render(dc, pitcherImageSpriteDataArray[randomPitcherImageIndex]->position.x, pitcherImageSpriteDataArray[randomPitcherImageIndex]->position.y,
			pitcherImageSpriteDataArray[randomPitcherImageIndex]->size.x, pitcherImageSpriteDataArray[randomPitcherImageIndex]->size.y,
			pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.x, pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.y, 
			pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.z, pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.w * batterImageAlpha,
			pitcherImageSpriteDataArray[randomPitcherImageIndex]->rotation);
	}

	
	//選択されたピッチャーの名前タグを描画
	if(selectedPitcherIndex < PITCHER_COUNT && pitcherNameSprite[selectedPitcherIndex] && pitcherNameSpriteData[selectedPitcherIndex])
	{
		pitcherNameSprite[selectedPitcherIndex]->render(dc, pitcherNamePosition.x, pitcherNamePosition.y,
			pitcherNameSize.x, pitcherNameSize.y,
			pitcherNameColor.x, pitcherNameColor.y, pitcherNameColor.z, pitcherNameColor.w * batterImageAlpha,
			pitcherNameSpriteData[selectedPitcherIndex]->rotation);
	}

	if(isNameTagClicked)
	{
		
		//背景描画
		if (pitcherParamBackGroundData && pitcherParamBackGroundSprite)
		{
			pitcherParamBackGroundSprite->render(dc, pitcherParamBackGroundData->position.x - pitcherParamBackGroundData->size.x / 2.0f,
				pitcherParamBackGroundData->position.y - pitcherParamBackGroundData->size.y / 2.0f,
				pitcherParamBackGroundData->size.x, pitcherParamBackGroundData->size.y,
				pitcherParamBackGroundData->color.x, pitcherParamBackGroundData->color.y,
				pitcherParamBackGroundData->color.z, pitcherParamBackGroundData->color.w * paramImageAlpha,
				pitcherParamBackGroundData->rotation);
		}

		dc->VSSetShader(burstVertexShader.Get(), nullptr, 0);
		dc->PSSetShader(burstPixelShader.Get(), nullptr, 0);
		dc->IASetInputLayout(nullptr); // 頂点バッファ不使用なのでレイアウトも不要
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		dc->VSSetConstantBuffers(0, 1, burstTransformBuffer.GetAddressOf());
		dc->PSSetConstantBuffers(0, 1, burstColorBuffer.GetAddressOf());

		dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
		dc->OMSetBlendState(renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);


		//パラメーター出現中のエフェクト描画
		if (isNameTagClicked && paramImageAlpha > 0.001f)
		{
			D3D11_MAPPED_SUBRESOURCE mapped;

			dc->Map(burstTransformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

			// バーストエフェクトの中心位置をスクリーン座標に変換
			auto* tcb = reinterpret_cast<BurstTransformBuffer*>(mapped.pData);
			tcb->center = modalBurstPosition;
			tcb->size = modalBurstSize;
			tcb->screenSize = { static_cast<float>(Graphics::Instance().GetScreenWidth()), static_cast<float>(Graphics::Instance().GetScreenHeight()) };
			dc->Unmap(burstTransformBuffer.Get(), 0);

			dc->Map(burstColorBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			auto* ccb = reinterpret_cast<BurstBuffer*>(mapped.pData);
			ccb->time = burstList.empty() ? 0.0f : burstList[0].time;
			ccb->aspectRatio = 1.0f; // アスペクト比を1.0に設定
			ccb->progress = paramImageAlpha;
			dc->Unmap(burstColorBuffer.Get(), 0);

			dc->Draw(4, 0); // 頂点バッファなしで4頂点描画（トライアングルストリップ）
		}

		dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
		dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
		dc->IASetInputLayout(input_layout.Get());
		
		dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);



		////選択されたピッチャーのスプライトを描画
		if (selectedPitcherIndex < PITCHER_COUNT && pitcherSprites[selectedPitcherIndex] && pitcherSpriteDataArray[selectedPitcherIndex])
		{
			pitcherSprites[selectedPitcherIndex]->render(dc, pitcherSpriteDataArray[selectedPitcherIndex]->position.x, pitcherSpriteDataArray[selectedPitcherIndex]->position.y,
				pitcherSpriteDataArray[selectedPitcherIndex]->size.x, pitcherSpriteDataArray[selectedPitcherIndex]->size.y,
				pitcherSpriteDataArray[selectedPitcherIndex]->color.x, pitcherSpriteDataArray[selectedPitcherIndex]->color.y,
				pitcherSpriteDataArray[selectedPitcherIndex]->color.z, pitcherSpriteDataArray[selectedPitcherIndex]->color.w * paramImageAlpha,
				pitcherSpriteDataArray[selectedPitcherIndex]->rotation);
		}

		if (randomPitcherImageIndex >= 0)
		{
			pitcherImageSprites[randomPitcherImageIndex]->render(dc, modalPitcherPos.x, modalPitcherPos.y,
				modalPitcherSize.x, modalPitcherSize.y,
				pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.x, pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.y,
				pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.z, pitcherImageSpriteDataArray[randomPitcherImageIndex]->color.w * batterImageAlpha,
				pitcherImageSpriteDataArray[randomPitcherImageIndex]->rotation);
		}

		buttonManager.Render(paramImageAlpha, ButtonManager::ButtonType::Close);

		
	}

	if (isChangingScene)
	{
		hexTransitionEffect.Render();
	}

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void batterSelectScene::uninitialize()
{
	// スクロールビューの解放
	playerScrollView.reset();
	VSSprite.reset();
}

void batterSelectScene::DrawGUI()
{
#ifdef _DEBUG


	// スクロールビューのGUI描画
	if (playerScrollView)
	{
		playerScrollView->DrawGUI();
	}
	// ボタンマネージャーのGUI描画
	buttonManager.DrawGUI();

	ImGui::Begin("ScrollView");

	if (ImGui::CollapsingHeader("pitcherInfo"))
	{
		ImGui::DragFloat2("pitcherInfo Position", &pitcherSpriteDataArray[selectedPitcherIndex]->position.x, 1.0f);
		ImGui::DragFloat2("pitcherInfo Size", &pitcherSpriteDataArray[selectedPitcherIndex]->size.x, 1.0f);
	}

	//ImGuiでもランダムにピッチャーを選択するボタンを追加
	if (ImGui::Button("Select Random Pitcher"))
	{
		SelectRandomPitcher();
	}

	if(ImGui::CollapsingHeader("Burst Effect"))
	{
		for(auto& effect : burstList)
		{
			std::string label = "Burst Effect " + std::to_string(&effect - &burstList[0]);
			if (ImGui::TreeNode(label.c_str()))
			{
				ImGui::DragFloat2("Position", &effect.position.x, 1.0f);
				ImGui::DragFloat2("Size", &effect.size.x, 1.0f);
				ImGui::DragFloat("Alpha", &effect.alpha, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Time", &effect.time, 0.01f);
				ImGui::TreePop();
			}
		}
	}

	if(ImGui::CollapsingHeader("Pitcher Name"))
	{
		ImGui::DragFloat2("Position", &pitcherNamePosition.x, 1.0f);
		ImGui::DragFloat2("Size", &pitcherNameSize.x, 1.0f);
		ImGui::ColorEdit4("Color", &pitcherNameColor.x);
	}

	if (ImGui::CollapsingHeader("Modal Copy Elements"))
	{
		ImGui::DragFloat2("Modal Pitcher Image Pos", &modalPitcherPos.x, 1.0f);
		ImGui::DragFloat2("Modal Pitcher Image Size", &modalPitcherSize.x, 1.0f);
		ImGui::DragFloat2("Modal Burst Pos", &modalBurstPosition.x, 1.0f);
		ImGui::DragFloat2("Modal Burst Size", &modalBurstSize.x, 1.0f);
	}

	if(ImGui::CollapsingHeader("VS Sprite"))
	{
		ImGui::DragFloat2("Position", &VSPosition.x, 1.0f);
		ImGui::DragFloat2("Size", &VSSize.x, 1.0f);
		ImGui::ColorEdit4("Color", &VSColor.x);
	}

	ImGui::End();
#endif // !_DEBUG
}

void batterSelectScene::SaveSetting()
{
	// 設定を保存する処理を実装
	json j;
	buttonManager.SaveToJson(j);
	playerScrollView->SaveToJson(j);
	// JSONをファイルに保存する処理を追加
	
	//BurstEffectの設定も保存

	j["burstEffect"] = json::array(); 
	for(auto& effect : burstList)
	{
		json effectJson;
		effectJson["position"] = { effect.position.x, effect.position.y };
		effectJson["size"] = { effect.size.x, effect.size.y };
		effectJson["alpha"] = effect.alpha;
		effectJson["time"] = 0.0f;
		j["burstEffect"].push_back(effectJson);
	}
	
	// ピッチャー名の設定も保存
	json pitcherNameJson;
	pitcherNameJson["position"] = { pitcherNamePosition.x, pitcherNamePosition.y };
	pitcherNameJson["size"] = { pitcherNameSize.x, pitcherNameSize.y };
	pitcherNameJson["color"] = { pitcherNameColor.x, pitcherNameColor.y, pitcherNameColor.z, pitcherNameColor.w };
	j["pitcherName"] = pitcherNameJson;

	json VSJson;
	VSJson["position"] = { VSPosition.x, VSPosition.y };
	VSJson["size"] = { VSSize.x, VSSize.y };
	VSJson["color"] = { VSColor.x, VSColor.y, VSColor.z, VSColor.w };
	j["VS"] = VSJson;

	// ファイルに保存
	std::ofstream file("resources\\setting\\batterSelectSettings.json");
	file << j.dump(4);
}

void batterSelectScene::LoadSetting()
{
	// 設定を読み込む処理を実装
	std::ifstream file("resources\\setting\\batterSelectSettings.json");


	json j;
	file >> j;
	// JSONファイルから読み込む処理を追加
	buttonManager.LoadFromJson(j);
	playerScrollView->LoadFromJson(j);

	if (j.contains("burstEffect") && j["burstEffect"].is_array())
	{
		burstList.clear();// 既存のバーストエフェクトをクリア

		for(const auto& effectJson : j["burstEffect"])
		{
			BurstEffectParam effect;
			if (effectJson.contains("position") && effectJson["position"].is_array() && effectJson["position"].size() == 2)
			{
				effect.position.x = effectJson["position"][0].get<float>();
				effect.position.y = effectJson["position"][1].get<float>();
			}
			if (effectJson.contains("size") && effectJson["size"].is_array() && effectJson["size"].size() == 2)
			{
				effect.size.x = effectJson["size"][0].get<float>();
				effect.size.y = effectJson["size"][1].get<float>();
			}
			if (effectJson.contains("alpha"))
			{
				effect.alpha = effectJson["alpha"].get<float>();
			}
			if (effectJson.contains("time"))
			{
				effect.time = effectJson["time"].get<float>();
			}
			burstList.push_back(effect);
		}
	}

	// ピッチャー名の設定を読み込む

	if (j.contains("pitcherName") && j["pitcherName"].is_object())
	{
		const auto& pitcherNameJson = j["pitcherName"];
		if (pitcherNameJson.contains("position") && pitcherNameJson["position"].is_array() && pitcherNameJson["position"].size() == 2)
		{
			pitcherNamePosition.x = pitcherNameJson["position"][0].get<float>();
			pitcherNamePosition.y = pitcherNameJson["position"][1].get<float>();
		}
		if (pitcherNameJson.contains("size") && pitcherNameJson["size"].is_array() && pitcherNameJson["size"].size() == 2)
		{
			pitcherNameSize.x = pitcherNameJson["size"][0].get<float>();
			pitcherNameSize.y = pitcherNameJson["size"][1].get<float>();
		}
		if (pitcherNameJson.contains("color") && pitcherNameJson["color"].is_array() && pitcherNameJson["color"].size() == 4)
		{
			pitcherNameColor.x = pitcherNameJson["color"][0].get<float>();
			pitcherNameColor.y = pitcherNameJson["color"][1].get<float>();
			pitcherNameColor.z = pitcherNameJson["color"][2].get<float>();
			pitcherNameColor.w = pitcherNameJson["color"][3].get<float>();
		}
	}

	if(j.contains("VS") && j["VS"].is_object())
	{
		const auto& VSJson = j["VS"];
		if (VSJson.contains("position") && VSJson["position"].is_array() && VSJson["position"].size() == 2)
		{
			VSPosition.x = VSJson["position"][0].get<float>();
			VSPosition.y = VSJson["position"][1].get<float>();
		}
		if (VSJson.contains("size") && VSJson["size"].is_array() && VSJson["size"].size() == 2)
		{
			VSSize.x = VSJson["size"][0].get<float>();
			VSSize.y = VSJson["size"][1].get<float>();
		}
		if (VSJson.contains("color") && VSJson["color"].is_array() && VSJson["color"].size() == 4)
		{
			VSColor.x = VSJson["color"][0].get<float>();
			VSColor.y = VSJson["color"][1].get<float>();
			VSColor.z = VSJson["color"][2].get<float>();
			VSColor.w = VSJson["color"][3].get<float>();
		}
	}
}