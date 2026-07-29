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

	//ピッチャー側にも選択されたピッチャーを設定
	Pitcher::Instance().SelectRealPitcher(selectedPitcher);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Selected Pitcher: %d (Index: %zu)", static_cast<int>(selectedPitcher), selectedPitcherIndex);
	OutputDebugStringA(buffer);


	burstElapsedTime = 0.0f; // バーストエフェクトの経過時間をリセット
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

	backGroundData = std::make_unique<BatterSelectSpriteData>();
	backGroundData->texturePath = L".\\resources\\textures\\batterSelectBack.png";
	backGroundData->position = { 0.0f, 0.0f };
	backGroundData->size = { 1920.0f, 1080.0f };
	backGroundData->rotation = 0.0f;
	backGroundData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	backGroundSprite = std::make_unique<sprite>(device, context, backGroundData->texturePath.c_str());

	//ピッチャーのスプライトデータを初期化
	for (size_t i = 0; i < pitcherCount; ++i)
	{
		pitcherSpriteDataArray[i] = std::make_unique<PitcherSpriteData>();
		pitcherSpriteDataArray[i]->texturePath = L".\\resources\\textures\\pitcherParameter\\pitcherParameter" + std::to_wstring(i + 1) + L".png";
		pitcherSpriteDataArray[i]->position = { 1250.0f, 75.0f };
		pitcherSpriteDataArray[i]->size = { 600.0f, 900.0f };
		pitcherSpriteDataArray[i]->rotation = 0.0f;
		pitcherSpriteDataArray[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		pitcherSprites[i] = std::make_unique<sprite>(device, context, pitcherSpriteDataArray[i]->texturePath.c_str());

	}

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
	burstAlpha = 0.0f;
	returnAlpha = 0.0f;
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

	float currentMaxAlpha = (std::max)(uiAlpha, returnAlpha);

	//遷移中はボタンの入力を無効化する
	if(currentState == SequenceState::Transition || currentState == SequenceState::Reverting)
	{
		buttonManager.Update(elapsed_time, 0.0f); // 遷移中はボタンの入力を無効化
	}
	else
	buttonManager.Update(elapsed_time, currentMaxAlpha);

	//ステートに応じた処理
	switch (currentState)
	{
	case SequenceState::Selecting:
	{
		uiAlpha = 1.0f;
		returnAlpha = 0.0f;
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
		// 後半: 光エフェクトが消え終わってからUIをフェードイン
		float uiProgress = std::clamp((transitionTimer - burstFadeDuration) / uiFadeDuration, 0.0f, 1.0f);
		uiAlpha = uiProgress;
		if (transitionTimer >= uiFadeDuration + burstFadeDuration)
		{
			currentState = SequenceState::Selecting;
			uiAlpha = 1.0f;
			returnAlpha = 0.0f;
			for (auto& effect : burstList)
			{
				effect.alpha = 0.0f; // 選択中はエフェクトを非表示
			}
		}
		break;
	}
	}

	for(auto& effect : burstList)
	{
		effect.time += elapsed_time;
	}

	if (!isChangingScene)
	{
		if (buttonManager.IsStartRequested())
		{
			isChangingScene = true;
			hexTransitionEffect.Start(1.0f);
			buttonManager.ResetStartRequest();
		}
	}
	else
	{
		hexTransitionEffect.Update(elapsed_time);

		if (hexTransitionEffect.IsFinished())
		{
			sceneManager::Instance().ChangeScene(new scene_loading(new scene_game()));
		}
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

		buttonManager.Render(uiAlpha, ButtonManager::ButtonType::OK);

	}

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
		if (effect.alpha > 0.001f)
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

	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	buttonManager.Render(returnAlpha, ButtonManager::ButtonType::Return);
	buttonManager.Render(returnAlpha, ButtonManager::ButtonType::Start);

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//選択されたピッチャーのスプライトを描画
	/*if (selectedPitcherIndex < pitcherCount && pitcherSprites[selectedPitcherIndex] && pitcherSpriteDataArray[selectedPitcherIndex])
	{
		pitcherSprites[selectedPitcherIndex]->render(dc, pitcherSpriteDataArray[selectedPitcherIndex]->position.x, pitcherSpriteDataArray[selectedPitcherIndex]->position.y,
			pitcherSpriteDataArray[selectedPitcherIndex]->size.x, pitcherSpriteDataArray[selectedPitcherIndex]->size.y,
			pitcherSpriteDataArray[selectedPitcherIndex]->color.x, pitcherSpriteDataArray[selectedPitcherIndex]->color.y, pitcherSpriteDataArray[selectedPitcherIndex]->color.z, pitcherSpriteDataArray[selectedPitcherIndex]->color.w,
			pitcherSpriteDataArray[selectedPitcherIndex]->rotation);
	}*/

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
	// バッターパラメータスプライトの解放
	batterParamSprite.reset();
	batterParamData.reset();
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
}