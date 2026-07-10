#include "HomeRunCount.h"
#include "Graphics.h"
#include "imgui.h"

void HomeRunCount::Initialize(ID3D11Device* device)
{
	// シェーダーの読み込み
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, "sprite_vs.cso", spriteVS.GetAddressOf(), spriteInputLayout.GetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, "sprite_ps.cso", spritePS.GetAddressOf());


	// スプライトの初期化
	homeRunCountSpriteData = std::make_unique<Sprite>();
	homeRunCountSpriteData->texturePath = L".\\resources\\textures\\homeRunCountBoard.png";
	homeRunCountSpriteData->position = { 10.0f, 10.0f };
	homeRunCountSpriteData->size = { 200.0f, 50.0f };
	homeRunCountSpriteData->rotation = 0.0f;
	homeRunCountSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	homeRunCountSprite = std::make_unique<sprite>(device, homeRunCountSpriteData->texturePath.c_str());
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	// ホームラン数表示に必要な文字だけをベイクする
	std::vector<int> homeRunCountCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789HOMERUN");
	// フォントレンダラーの初期化
	homeRunCountFont.Initialize(device,
		L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&homeRunCountCodepoints);
}

void HomeRunCount::Uninitialize()
{
	homeRunCountFont.Uninitialize();
	homeRunCountSprite.reset();
	homeRunCountSpriteData.reset();
}

void HomeRunCount::Update(float elapsedTime)
{
	// ホームラン数の更新はここで行う
	// 例: homeRunCount += 1; // ホームランが出た場合に呼び出す
}

void HomeRunCount::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// ホームラン数の描画
	homeRunCountSprite->render(Graphics::Instance().GetDeviceContext(),
		homeRunCountSpriteData->position.x, homeRunCountSpriteData->position.y,
		homeRunCountSpriteData->size.x, homeRunCountSpriteData->size.y,
		homeRunCountSpriteData->color.x, homeRunCountSpriteData->color.y,
		homeRunCountSpriteData->color.z, homeRunCountSpriteData->color.w,
		homeRunCountSpriteData->rotation);
	// ホームラン数のテキスト描画
	char buffer[64];
	sprintf_s(buffer, "HOMERUN: %d", homeRunCount);

	homeRunCountFont.DrawTextW(Graphics::Instance().GetDeviceContext(),
		buffer,
		homeRunCountSpriteData->position.x + 10.0f, // テキストのX位置
		homeRunCountSpriteData->position.y + 10.0f, // テキストのY位置
		1.0f, // スケール
		1.0f, 1.0f, 1.0f, 1.0f); // 白色で描画

	// 後始末（Wind と同じ）
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}