#include "batSprite.h"
#include "Graphics.h"
#include <shader.h>
#include <imgui.h>
#include "input.h"
#include "ballSprite.h"
#include <algorithm>
#include "Player.h"
#include <TrackingData.h>
#include <GameTimer.h>

//ラープ関数
float Lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

void BatSprite::Initialize(ID3D11Device* device)
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

	batSpriteData = std::make_unique<Sprite>();
	batSpriteData->texturePath = L".\\resources\\textures\\bat.png";
	batSpriteData->position = { 1100.0f, 400.0f };
	batSpriteData->size = { 230.0f, 40.0f };
	batSpriteData->rotation = 0.0f;
	batSpriteData->color = { 1.0f, 1.0f, 1.0f, 0.7f };
	batSprite = std::make_unique<sprite>(device, context, batSpriteData->texturePath.c_str());

	batCursorSpriteData = std::make_unique<Sprite>();
	batCursorSpriteData->texturePath = L".\\resources\\textures\\batCursor.png";
	batCursorSpriteData->position = { 1100.0f, 400.0f };
	batCursorSpriteData->size = { 30.0f, 30.0f };
	batCursorSpriteData->rotation = 0.0f;
	batCursorSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	batCursorSprite = std::make_unique<sprite>(device, context, batCursorSpriteData->texturePath.c_str());

#ifndef _DEBUG
	ShowCursor(FALSE);
#endif // !_DEBUG

	

}

void BatSprite::Uninitialize()
{
	ClipCursor(nullptr); // 必ず解除してから終了
	batSprite.reset();
	batSpriteData.reset();

#ifndef _DEBUG
	ShowCursor(TRUE);
#endif // !_DEBUG

	
}

void BatSprite::Update(float elapsedTime)
{
	// 左コントロールキーでカーソル制限をトグル
	//static bool prevCtrl = false;
	//bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	//if (ctrl && !prevCtrl)
	//{
	//	cursorClipped = !cursorClipped;
	//	if (!cursorClipped)
	//		ClipCursor(nullptr); // 解除
	//}
	//prevCtrl = ctrl;
	//
	//if (cursorClipped)
	//{
	//	// ストライクゾーンのスクリーン境界を取得
	//	DirectX::XMFLOAT2 zoneTopLeft, zoneBottomRight;
	//	ballSprite::Instance().GetBallZoneScreenBounds(zoneTopLeft, zoneBottomRight);

	//	// クライアント座標 → スクリーン座標に変換
	//	HWND hwnd = GetForegroundWindow();
	//	POINT tl = { (LONG)zoneTopLeft.x,     (LONG)zoneTopLeft.y };
	//	POINT br = { (LONG)zoneBottomRight.x,  (LONG)zoneBottomRight.y };
	//	ClientToScreen(hwnd, &tl);
	//	ClientToScreen(hwnd, &br);

	//	RECT clipRect = { tl.x, tl.y, br.x, br.y };
	//	ClipCursor(&clipRect);
	//}

	if (GameTimer::Instance().GetRemainingTime() <= 0.0f && !Ball::Instance().GetHasCollidedWithBat() && !(Pitcher::Instance().GetCurrentState() == Pitcher::State::Throwing))
	{
		// 投球が終わったらフラグをリセット
		isAssisting = false;
		return;
	}
	
	Pitcher& pitcher = Pitcher::Instance();
	ballSprite& bs = ballSprite::Instance();

	//ボールスプライトの位置を取得
	if (pitcher.GetIsBallThrown())
	{
		////アシスト中に自分でマウスを動かした場合はアシストを終了する
		//POINT currentMousePos;
		//GetCursorPos(&currentMousePos);

		//if (currentMousePos.x != assistStartMousePos.x || currentMousePos.y != assistStartMousePos.y)
		//{
		//	isAssisting = false;
		//	return;
		//}

		//アシストを始めていなかったら、最初の1フレームのみ初期化
		if (!isAssisting && isMeetAssistEnabled)
		{
			isAssisting = true;
			GetCursorPos(&assistStartMousePos);
			assistTimer = 0.0f;
		}

		//アシスト中はカーソルの位置をボールの位置に合わせる
		if (isAssisting)
		{
			assistTimer += elapsedTime;

			// 1. X軸とY軸でそれぞれ進捗率を計算する
			float progressX = (std::min)(assistTimer / assistDuration, 1.0f);

			// yMoveScale（例: 0.5f）を掛けることで、Y軸の補間スピードだけを遅らせる
			float yMoveScale = bs.GetYMoveScale(
				bs.GetCurrentPitchIndex(),
				bs.GetFinalScreenPos().y,
				bs.GetStartScreenPos().y,
				bs.GetZoneCenterY()
			);
			float progressY = (std::min)((assistTimer / assistDuration) * yMoveScale, 1.0f);

			// 2. ボールの最終到達地点を取得（オフセット調整が必要な場合は固定値で足す）
			DirectX::XMFLOAT2 ballFinalPos = bs.GetFinalScreenPos();

			// バットカーソルの中心・芯に合わせるためのオフセット調整（必要な場合）
			float offsetY = batCursorSpriteData->size.y * 0.2f;

			float actualFinalY = bs.GetStartScreenPos().y + (ballFinalPos.y - bs.GetStartScreenPos().y) * yMoveScale;

			POINT targetPt = {
				static_cast<LONG>(ballFinalPos.x),
				static_cast<LONG>(actualFinalY + offsetY)
			};

			// スクリーン座標に変換
			HWND hwnd = GetForegroundWindow();
			ClientToScreen(hwnd, &targetPt);

			// 3. X軸とY軸で異なる進捗率を使ってカーソル位置を補間
			POINT currentPt;
			currentPt.x = static_cast<LONG>(assistStartMousePos.x + (targetPt.x - assistStartMousePos.x) * progressX);
			currentPt.y = static_cast<LONG>(assistStartMousePos.y + (targetPt.y - assistStartMousePos.y) * progressY);

			SetCursorPos(currentPt.x, currentPt.y);
			
		}

		
	}
	else
	{
		// 投球が終わったらフラグをリセット
		isAssisting = false;
	}

	

}

void BatSprite::UpdateCursorSizeByContact(int contact)
{
	if (batCursorSpriteData == nullptr) return;

	//0～99の範囲を0～1にクランプ
	float t = (std::max)(0, (std::min)(99,contact)) / 99.0f;

	// クランプされた値を使ってカーソルサイズを更新
	float scale = minCursorScale * (maxCursorScale - minCursorScale) * t;

	if (batCursorSpriteData)
	{
		batCursorSpriteData->size =
		{
			originalCursorSize.x * scale,
			originalCursorSize.y * scale,
		};
	}

	HitJudge2D::Instance().cursorRadius = batCursorSpriteData->size.x * 0.5f;
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

	// トラッキングデータが表示されている場合はバットスプライトを描画しない
	if (TrackingData::Instance().IsTrackingDataVisible()) return;

	//確信ホームランのときも描画しない
	if (Physics::Instance().GetIsHomeRun()) return;
	
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
	if (ImGui::CollapsingHeader(u8"ミートアシスト"))
	{
		ImGui::Checkbox(u8"ミートアシスト有効", &isMeetAssistEnabled);
		ImGui::DragFloat(u8"ミートアシスト時間(秒)", &assistDuration, 0.01f, 0.1f, 5.0f);
	}

}