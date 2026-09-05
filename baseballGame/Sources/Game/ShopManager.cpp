#include "ShopManager.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include "UiEasing.h"
#include "Money.h"


void ShopManager::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	const static int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const static int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());
	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", pixel_shader.ReleaseAndGetAddressOf());
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", vertex_shader.ReleaseAndGetAddressOf(), input_layout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));

	shopBackSpriteData = std::make_unique<ShopSprite>();
	shopBackSpriteData->texturePath = L".\\resources\\textures\\shopBoard.png";
	shopBackSpriteData->position = { 960.0f, 540.0f }; // 適切な位置に配置
	shopBackSpriteData->size = { 1800.0f, 1000.0f }; // 適切なサイズに設定
	shopBackSpriteData->rotation = 0.0f;
	shopBackSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	shopBackSprite = std::make_unique<sprite>(device, context, shopBackSpriteData->texturePath.c_str());

	currentPosition = startPosition;
	moneyFontCurrentPosition = moneyFontStartPosition;

	static std::vector<int> trackingDataCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789");

	moneyFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		50.0f,
		screenWidth,
		screenHeight,
		1024, 1024,
		&trackingDataCodepoints);

	

}

void ShopManager::Uninitialize()
{
	shopBackSprite.reset();
	shopBackSpriteData.reset();
	pixel_shader.Reset();
	vertex_shader.Reset();
	input_layout.Reset();
}

void ShopManager::Update(float elapsedTime)
{
	if(!isAnimating)
	{
		return; // アニメーション中でない場合は更新しない
	}

	if(isShopOpen)
	{
		// タイマーを加算
		easingTimer += elapsedTime;

		// 0.0f ～ 1.0f の範囲にクランプ
		float t = easingTimer / easingDuration;
		if (t > 1.0f) t = 1.0f;

		// イージングで現在位置を計算
		currentPosition = UiEasing::Lerp(startPosition, targetPosition, t, UiEasing::EasingType::OutBack);
		moneyFontCurrentPosition = UiEasing::Lerp(moneyFontStartPosition, moneyFontTargetPosition, t, UiEasing::EasingType::OutBack);
		if (t >= 1.0f)
		{
			isAnimating = false; // アニメーション終了
		}
	}
	else if(isShopClosed)
	{
		// タイマーを加算
		easingTimer += elapsedTime;
		// 0.0f ～ 1.0f の範囲にクランプ
		//倍速で再生する
		float t = easingTimer / (easingDuration * 0.5f);
		if (t > 1.0f) t = 1.0f;
		// イージングで現在位置を計算
		currentPosition = UiEasing::Lerp(startPosition, targetPosition, t, UiEasing::EasingType::InCubic);
		moneyFontCurrentPosition = UiEasing::Lerp(moneyFontStartPosition, moneyFontTargetPosition, t, UiEasing::EasingType::InCubic);
		if (t >= 1.0f)
		{
			isAnimating = false; // アニメーション終了
			isShopClosed = false; // ショップが閉じた状態にする
		}
	}
}

void ShopManager::Render()
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	context->IASetInputLayout(input_layout.Get());

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);


	if (shopBackSprite)
	{
		shopBackSprite->render(context,
			currentPosition.x - shopBackSpriteData->size.x / 2.0f,
			currentPosition.y - shopBackSpriteData->size.y / 2.0f,
			shopBackSpriteData->size.x,
			shopBackSpriteData->size.y,
			shopBackSpriteData->color.x,
			shopBackSpriteData->color.y,
			shopBackSpriteData->color.z,
			shopBackSpriteData->color.w,
			shopBackSpriteData->rotation);
	}

	float moneyWidth, moneyHeight;

	moneyFont.MeasureText(std::to_string(Money::Instance().GetCurrentMoney()).c_str(), moneyFontScale, moneyWidth, moneyHeight);

	float adjustedX = moneyFontCurrentPosition.x - moneyWidth / 2.0f; // 中央揃えのためにX座標を調整
	float adjustedY = moneyFontCurrentPosition.y - moneyHeight / 2.0f; // 中央揃えのためにY座標を調整

	moneyFont.DrawTextW(context, std::to_string(Money::Instance().GetCurrentMoney()).c_str(),
		adjustedX, adjustedY,
		moneyFontScale,
		moneyFontColor.x, moneyFontColor.y, moneyFontColor.z, moneyFontColor.w);



	//シェーダーの設定を解除
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	context->IASetInputLayout(nullptr);

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void ShopManager::DrawGUI()
{
	if(ImGui::CollapsingHeader("ShopManager"))
	{
		ImGui::DragFloat2("fontPosition", &moneyFontPosition.x, 1.0f, 0.0f);
	}
}