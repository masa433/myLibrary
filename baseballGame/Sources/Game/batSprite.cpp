#include "batSprite.h"
#include "Graphics.h"
#include <shader.h>
#include <imgui.h>
#include "input.h"
#include "ballSprite.h"
#include <algorithm>

void BatSprite::Initialize(ID3D11Device* device)
{
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, "sprite_vs.cso", spriteVS.GetAddressOf(), spriteInputLayout.GetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, "sprite_ps.cso", spritePS.GetAddressOf());

	batSpriteData = std::make_unique<Sprite>();
	batSpriteData->texturePath = L".\\resources\\textures\\bat.png";
	batSpriteData->position = { 1100.0f, 400.0f };
	batSpriteData->size = { 230.0f, 40.0f };
	batSpriteData->rotation = 0.0f;
	batSpriteData->color = { 1.0f, 1.0f, 1.0f, 0.7f };
	batSprite = std::make_unique<sprite>(device, batSpriteData->texturePath.c_str());

	batCursorSpriteData = std::make_unique<Sprite>();
	batCursorSpriteData->texturePath = L".\\resources\\textures\\batCursor.png";
	batCursorSpriteData->position = { 1100.0f, 400.0f };
	batCursorSpriteData->size = { 30.0f, 30.0f };
	batCursorSpriteData->rotation = 0.0f;
	batCursorSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	batCursorSprite = std::make_unique<sprite>(device, batCursorSpriteData->texturePath.c_str());
	//ShowCursor(FALSE);

}

void BatSprite::Uninitialize()
{
	batSprite.reset();
	batSpriteData.reset();
	ShowCursor(TRUE);
}

void BatSprite::Update(float elapsedTime)
{
	
}

void BatSprite::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// Win32 APIで直接クライアント座標を取得
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(GetForegroundWindow(), &pt);


	float mouseX = static_cast<float>(pt.x);
	float mouseY = static_cast<float>(pt.y); 

	DirectX::XMFLOAT2 zoneTopLeft, zoneBottomRight;
	ballSprite::Instance().GetBallZoneScreenBounds(zoneTopLeft, zoneBottomRight);

	mouseX = (std::max)(zoneTopLeft.x, (std::min)(zoneBottomRight.x, mouseX));
	mouseY = (std::max)(zoneTopLeft.y, (std::min)(zoneBottomRight.y, mouseY));
	
	if (batSprite && batSpriteData)
	{
		// 画像の中心をマウス位置に合わせる
		float drawX = mouseX - batSpriteData->size.x * 0.7f;
		float drawY = mouseY - batSpriteData->size.y;

		//左バッターの時は反転させる
		Player& player = Player::Instance();
		if (player.IsRightBatter())
		{
			batSpriteData->rotation = 25.0f; // 右バッターの場合は回転させない
		}
		else
		{
			batSpriteData->rotation = 155.0f; // 左バッターの場合は180度回転させる
			drawX = mouseX - batSpriteData->size.x * 0.3f; // 左バッターの場合は位置を調整
		}


		batSprite->render(dc,
			drawX, drawY,
			batSpriteData->size.x, batSpriteData->size.y,
			batSpriteData->color.x, batSpriteData->color.y,
			batSpriteData->color.z, batSpriteData->color.w,
			batSpriteData->rotation);
	}

	if(batCursorSprite && batCursorSpriteData)
	{
		// 画像の中心をマウス位置に合わせる
		float drawX = mouseX - batCursorSpriteData->size.x * 0.5f;
		float drawY = mouseY - batCursorSpriteData->size.y * 0.5f;
		batCursorSprite->render(dc,
			drawX, drawY,
			batCursorSpriteData->size.x, batCursorSpriteData->size.y,
			batCursorSpriteData->color.x, batCursorSpriteData->color.y,
			batCursorSpriteData->color.z, batCursorSpriteData->color.w,
			batCursorSpriteData->rotation);
	}

	// 後始末（Wind と同じ）
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void BatSprite::DrawGUI()
{
	if (batSpriteData)
	{
		if (ImGui::CollapsingHeader(u8"2D スプライト"))
		{
			ImGui::DragFloat2(u8"ゾーン 位置(px)", &batSpriteData->position.x, 1.0f);
			ImGui::DragFloat2(u8"ゾーン サイズ(px)", &batSpriteData->size.x, 1.0f, 1.0f, 2000.0f);
			ImGui::ColorEdit4(u8"ゾーン 透明度", &batSpriteData->color.x);

		
		}
	}
}