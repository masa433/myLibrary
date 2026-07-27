#include "batterSelectScene.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include <random>
#include "scene_loading.h"
#include "scene_game.h"

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
}


void batterSelectScene::initialize()
{
	
	// スクロールビューの初期化
	ID3D11Device* device = Graphics::Instance().GetDevice();
	playerScrollView = std::make_unique<ScrollView>(device, 200.0f, 540.0f, 335.0f, 1000.0f);

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
	for(size_t i = 0; i < pitcherCount; ++i)
	{
		pitcherSpriteDataArray[i] = std::make_unique<PitcherSpriteData>();
		pitcherSpriteDataArray[i]->texturePath = L".\\resources\\textures\\pitcherParameter\\pitcherParameter" + std::to_wstring(i + 1) + L".png";
		pitcherSpriteDataArray[i]->position = { 1250.0f, 75.0f };
		pitcherSpriteDataArray[i]->size = { 600.0f, 900.0f };
		pitcherSpriteDataArray[i]->rotation = 0.0f;
		pitcherSpriteDataArray[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		pitcherSprites[i] = std::make_unique<sprite>(device, context, pitcherSpriteDataArray[i]->texturePath.c_str());

	}

	SelectRandomPitcher(); // ランダムにピッチャーを選択
}

void batterSelectScene::update(float elapsed_time)
{
	playerScrollView->Update(elapsed_time);

	buttonManager.Update(elapsed_time);

	GamePad& pad = Input::Instance().GetGamePad();

	const GamePadButton anyButton =
		GamePad::BTN_A
		| GamePad::BTN_B
		| GamePad::BTN_X
		| GamePad::BTN_Y;
	
	if (anyButton & pad.GetButtonDown())
	{
		// 選手が選択されたらゲームシーンに遷移
		sceneManager::Instance().ChangeScene(new scene_loading(new scene_game()));
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

	if (playerScrollView)
	{
		playerScrollView->Render(); // この中でVS/PS/InputLayoutがnullptrに戻る
	}

	buttonManager.Render(); 

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨

	//選択されたピッチャーのスプライトを描画
	if(selectedPitcherIndex < pitcherCount && pitcherSprites[selectedPitcherIndex] && pitcherSpriteDataArray[selectedPitcherIndex])
	{
		pitcherSprites[selectedPitcherIndex]->render(dc, pitcherSpriteDataArray[selectedPitcherIndex]->position.x, pitcherSpriteDataArray[selectedPitcherIndex]->position.y,
			pitcherSpriteDataArray[selectedPitcherIndex]->size.x, pitcherSpriteDataArray[selectedPitcherIndex]->size.y,
			pitcherSpriteDataArray[selectedPitcherIndex]->color.x, pitcherSpriteDataArray[selectedPitcherIndex]->color.y, pitcherSpriteDataArray[selectedPitcherIndex]->color.z, pitcherSpriteDataArray[selectedPitcherIndex]->color.w,
			pitcherSpriteDataArray[selectedPitcherIndex]->rotation);
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

	bool isChanged = false;
	isChanged |= ImGui::DragFloat2("ScrollView Position", &scrollViewPosition.x, 1.0f);
	isChanged |= ImGui::DragFloat2("ScrollView Size", &scrollViewSize.x, 1.0f);

	if (isChanged && playerScrollView)
	{
		// 背景の中心位置に合わせて渡す
		playerScrollView->SetBackGroundTransform(
			scrollViewPosition.x + scrollViewSize.x / 2.0f,
			scrollViewPosition.y + scrollViewSize.y / 2.0f,
			scrollViewSize.x,
			scrollViewSize.y
		);
	}

	if(ImGui::CollapsingHeader("pitcherInfo"))
	{
		ImGui::DragFloat2("pitcherInfo Position", &pitcherSpriteDataArray[selectedPitcherIndex]->position.x, 1.0f);
		ImGui::DragFloat2("pitcherInfo Size", &pitcherSpriteDataArray[selectedPitcherIndex]->size.x, 1.0f);
	}

	//ImGuiでもランダムにピッチャーを選択するボタンを追加
	if (ImGui::Button("Select Random Pitcher"))
	{
		SelectRandomPitcher();
	}

	ImGui::End();
#endif // !_DEBUG
}