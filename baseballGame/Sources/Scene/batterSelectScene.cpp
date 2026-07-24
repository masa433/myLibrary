#include "batterSelectScene.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"


void batterSelectScene::initialize()
{
	// 選手のリストを初期化
	playerList.clear();
	
	for (int i = 1; i < static_cast<int>(Player::RealBatter::Count); ++i)
	{
		auto realBatter = static_cast<Player::RealBatter>(i);// RealBatter列挙型の値を取得

		std::vector<Player::RealArsenalInfo> arsenalData;
		bool isRight = false;
		const char* name = "";

		if(Player::GetRealBatterArsenalData(realBatter, arsenalData, isRight, name))
		{
			// アーセナルデータが取得できた場合のみリストに追加
			BatterEntry entry;
			entry.name = name ? name :  Player::GetRealBatterName(realBatter);
			entry.power = !arsenalData.empty() ? arsenalData[0].power : 0;
			entry.isRightBatter = isRight;
			playerList.push_back(entry);
		}
		else
		{
			continue; // データが取得できなかった場合はスキップ
		}

		
	}

	selectedPlayerIndex = -1; // 選択された選手のインデックスを初期化

	// スクロールビューの初期化
	ID3D11Device* device = Graphics::Instance().GetDevice();
	playerScrollView = std::make_unique<ScrollView>(device, 200.0f, 540.0f, 335.0f, 1000.0f);

	buttonManager.Initialize();

	// added: pitch info font init (日本語対応版)
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> codepoints = FontRenderer::Utf8ToCodepoints(
		u8" !\"#$%&'()*+,-./0123456789:;<=>?@"
		u8"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
		u8"abcdefghijklmnopqrstuvwxyz{|}~"
		u8"あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをんゃゅょっー"
		u8"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンャュョッー"
	);

	// 日本語グリフを持つフォントを用意して配置する
	fontRenderer.Initialize(device,
		L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&codepoints);

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

	batterParamData = std::make_unique<BatterSelectSpriteData>();
	batterParamData->texturePath = L".\\resources\\textures\\batterParam(Ohtani).png";
	batterParamData->position = { 100.0f, 100.0f };
	batterParamData->size = { 500.0f, 400.0f };
	batterParamData->rotation = 0.0f;
	batterParamData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	
	//batterParamSprite = std::make_unique<sprite>(device, context, batterParamData->texturePath.c_str());

	backGroundData = std::make_unique<BatterSelectSpriteData>();
	backGroundData->texturePath = L".\\resources\\textures\\batterSelectBack.png";
	backGroundData->position = { 0.0f, 0.0f };
	backGroundData->size = { 1920.0f, 1080.0f };
	backGroundData->rotation = 0.0f;	
	backGroundData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	backGroundSprite = std::make_unique<sprite>(device, context, backGroundData->texturePath.c_str());
}

void batterSelectScene::update(float elapsed_time)
{
	playerScrollView->Update(elapsed_time);

	buttonManager.Update(elapsed_time);
}

void batterSelectScene::render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

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

	// --- フォント描画(これは別のシェーダーをfontRenderer内部で使うはずなので、この位置で問題なし) ---
	auto centerTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
		{
			float textWidth = 0.0f;
			float textHeight = 0.0f;
			fontRenderer.MeasureText(text.c_str(), fontSize, textWidth, textHeight);
			return { x - textWidth / 2.0f, y - textHeight / 2.0f };
		};

	DirectX::XMFLOAT2 fontPos = centerTextPosition(text, fontSize, fontPosition.x, fontPosition.y);

	fontRenderer.DrawTextW(dc, text, fontPos.x, fontPos.y, fontSize,
		fontColor.x, fontColor.y, fontColor.z, fontColor.w);

	// --- batterParamSpriteの描画 ---
	//if (batterParamData && batterParamSprite)
	//{
	//	// 念のため直前でも再度バインド(フォント描画がシェーダーを変えている場合の保険)
	//	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	//	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	//	dc->IASetInputLayout(input_layout.Get());

	//	batterParamSprite->render(dc, batterParamData->position.x, batterParamData->position.y,
	//		batterParamData->size.x, batterParamData->size.y,
	//		batterParamData->color.x, batterParamData->color.y, batterParamData->color.z, batterParamData->color.w,
	//		batterParamData->rotation);
	//}

	

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void batterSelectScene::uninitialize()
{
	// 選手のリストをクリア
	playerList.clear();
	// スクロールビューの解放
	playerScrollView.reset();
	// バッターパラメータスプライトの解放
	batterParamSprite.reset();
	batterParamData.reset();
}

void batterSelectScene::DrawGUI()
{
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
		// 背景の中心位置に合わせて渡す（必要に応じて計算を調整してください）
		playerScrollView->SetBackGroundTransform(
			scrollViewPosition.x + scrollViewSize.x / 2.0f,
			scrollViewPosition.y + scrollViewSize.y / 2.0f,
			scrollViewSize.x,
			scrollViewSize.y
		);
	}
	ImGui::End();
}