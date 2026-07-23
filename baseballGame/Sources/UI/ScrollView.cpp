#include "ScrollView.h"
#include "Graphics.h"
#include <shader.h>
#include <imgui.h>

ScrollView::ScrollView(ID3D11Device* device, float topX, float topY, float width, float height)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	// スクロール背景スプライトの初期化
	scrollBackgroundSpriteData = std::make_unique<ScrollBackData>();
	scrollBackgroundSpriteData->texturePath = L".\\resources\\textures\\scrollViewBack.png"; // スクロール背景のテクスチャパス
	scrollBackgroundSpriteData->position = { topX + width / 2.0f, topY + height / 2.0f }; // 中心位置に設定
	scrollBackgroundSpriteData->size = { width, height }; // 幅と高さを設定
	scrollBackgroundSpriteData->rotation = 0.0f; // 回転なし
	scrollBackgroundSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
	scrollBackgroundSprite = std::make_unique<sprite>(device, context, scrollBackgroundSpriteData->texturePath.c_str());
	// シェーダーの作成

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.GetAddressOf(), input_layout.GetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.GetAddressOf());
}

ScrollView::ScrollView(ID3D11Device* device, const char* filePath, float topX, float topY, float width, float height)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	// スクロール背景スプライトの初期化
	scrollBackgroundSpriteData = std::make_unique<ScrollBackData>();
	scrollBackgroundSpriteData->texturePath = std::wstring(filePath, filePath + strlen(filePath)); // スクロール背景のテクスチャパス
	scrollBackgroundSpriteData->position = { topX + width / 2.0f, topY + height / 2.0f }; // 中心位置に設定
	scrollBackgroundSpriteData->size = { width, height }; // 幅と高さを設定
	scrollBackgroundSpriteData->rotation = 0.0f; // 回転なし
	scrollBackgroundSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
	scrollBackgroundSprite = std::make_unique<sprite>(device, context, scrollBackgroundSpriteData->texturePath.c_str());
	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.GetAddressOf(), input_layout.GetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.GetAddressOf());

};

void ScrollView::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(vertex_shader.Get(), nullptr, 0);
	dc->PSSetShader(pixel_shader.Get(), nullptr, 0);
	dc->IASetInputLayout(input_layout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if (scrollBackgroundSprite)
	{
		ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
		scrollBackgroundSprite->render(context,
			scrollBackgroundSpriteData->position.x - scrollBackgroundSpriteData->size.x / 2.0f,
			scrollBackgroundSpriteData->position.y - scrollBackgroundSpriteData->size.y / 2.0f,
			scrollBackgroundSpriteData->size.x,
			scrollBackgroundSpriteData->size.y,
			scrollBackgroundSpriteData->color.x, scrollBackgroundSpriteData->color.y,
			scrollBackgroundSpriteData->color.z, scrollBackgroundSpriteData->color.w,
			scrollBackgroundSpriteData->rotation);
	}

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void ScrollView::Update(float elapsedTime)
{
	// スクロールビューの更新処理をここに追加できます
}

void ScrollView::DrawGUI()
{
#ifdef _DEBUG
	//ポジションやサイズの調整
	if (ImGui::CollapsingHeader("ScrollView"))
	{
		ImGui::Begin("ScrollView");
		ImGui::Text("ScrollView Position: (%.2f, %.2f)", scrollBackgroundSpriteData->position.x, scrollBackgroundSpriteData->position.y);
		ImGui::Text("ScrollView Size: (%.2f, %.2f)", scrollBackgroundSpriteData->size.x, scrollBackgroundSpriteData->size.y);
		ImGui::End();
	}
#endif // _DEBUG
	

}