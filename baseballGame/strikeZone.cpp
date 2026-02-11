#include "strikeZone.h"
#include "input.h"
#include "imgui.h"
#include <algorithm>
#include <Windows.h>

// 初期化
void strikeZone::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	// バットミートカーソルのスプライト作成
	batCursor = std::make_unique<sprite>(device, L".\\resources\\sprite\\cursor.png");
	cursorScale = { 0.12f,0.12f };

	// ストライクゾーン表示用スプライトの作成
	strikeZoneSprite = std::make_unique<sprite>(device, L".\\resources\\sprite\\strikeZone.png");
	spritePosition = { 560.0f,330.0f };
	spriteScale = { 0.2f, 0.25f };

	ballSprite = std::make_unique<sprite>(device, L".\\resources\\sprite\\ball.png");
	ballSpritePosition = { 0.0f, 0.0f };
	ballSpriteScale = { 0.05f, 0.05f };
	showBallSprite = false;
}

// 解放
void strikeZone::Uninitialize()
{
}

// 更新
void strikeZone::Update(float elapsedTime)
{
	POINT mousePoint;
	GetCursorPos(&mousePoint);

	// スクリーン座標をウィンドウ座標に変換
	ScreenToClient(GetActiveWindow(), &mousePoint);

	cursorPos.x = static_cast<float>(mousePoint.x);
	cursorPos.y = static_cast<float>(mousePoint.y);

	// ストライクゾーンの範囲を取得
	float strikeZoneLeft = spritePosition.x;
	float strikeZoneRight = spritePosition.x + (strikeZoneSprite->texture2d_desc.Width * spriteScale.x);
	float strikeZoneTop = spritePosition.y;
	float strikeZoneBottom = spritePosition.y + (strikeZoneSprite->texture2d_desc.Height * spriteScale.y);

	// マウスカーソルの行動制限範囲を指定
	float mouseLimitLeft = strikeZoneLeft - 70.0f;   // ストライクゾーンの左側に50ピクセルの余裕を持たせる
	float mouseLimitRight = strikeZoneRight + 70.0f; // ストライクゾーンの右側に50ピクセルの余裕を持たせる
	float mouseLimitTop = strikeZoneTop - 70.0f;     // ストライクゾーンの上側に50ピクセルの余裕を持たせる
	float mouseLimitBottom = strikeZoneBottom + 70.0f; // ストライクゾーンの下側に50ピクセルの余裕を持たせる

	if (!isMoveCursor)
	{
		// マウスカーソル位置を制限
		if (cursorPos.x < mouseLimitLeft) cursorPos.x = mouseLimitLeft;
		if (cursorPos.x > mouseLimitRight) cursorPos.x = mouseLimitRight;
		if (cursorPos.y < mouseLimitTop) cursorPos.y = mouseLimitTop;
		if (cursorPos.y > mouseLimitBottom) cursorPos.y = mouseLimitBottom;
	}
	
	// システムマウスカーソルを制限範囲内に移動
	POINT clampedMousePoint;
	clampedMousePoint.x = static_cast<LONG>(cursorPos.x);
	clampedMousePoint.y = static_cast<LONG>(cursorPos.y);
	ClientToScreen(GetActiveWindow(), &clampedMousePoint);
	SetCursorPos(clampedMousePoint.x, clampedMousePoint.y);

	// ミートカーソルの位置をストライクゾーン内に制限
	float meetCursorX = std::clamp(cursorPos.x, strikeZoneLeft, strikeZoneRight);
	float meetCursorY = std::clamp(cursorPos.y, strikeZoneTop, strikeZoneBottom);

	// ミートカーソルの中心位置を更新
	cursorPos.x = meetCursorX;
	cursorPos.y = meetCursorY;

	if (GetAsyncKeyState('X') & 0x8000)
	{
		isMoveCursor = !isMoveCursor;
	}
}

// 描画
void strikeZone::Render(RenderContext& rc)
{
	// ストライクゾーン画像の描画
	if (showStrikeZoneImage && strikeZoneSprite)
	{
		strikeZoneSprite->render(
			rc.context,
			spritePosition.x,
			spritePosition.y,
			strikeZoneSprite->texture2d_desc.Width * spriteScale.x,
			strikeZoneSprite->texture2d_desc.Height * spriteScale.y,
			spriteTint.x,
			spriteTint.y,
			spriteTint.z,
			spriteTint.w,
			0.0f);
	}
	// バットミートカーソルの描画
	if (showBatCursor && batCursor) 
	{
		
		//カーソルの中心位置を変更
		float cursorCenterPosX = cursorPos.x - (batCursor->texture2d_desc.Width * cursorScale.x) / 1.5f;
		float cursorCenterPosY = cursorPos.y - (batCursor->texture2d_desc.Height * cursorScale.y) / 2.0f;


		// カーソルを描画
		batCursor->render(
			rc.context,
			cursorCenterPosX,                    // X座標
			cursorCenterPosY,                    // Y座標
			batCursor->texture2d_desc.Width * cursorScale.x,   // 幅
			batCursor->texture2d_desc.Height * cursorScale.y,  // 高さ
			1.0f, 1.0f, 1.0f, 1.0f,              // 色
			0.0f                                 // 回転角度
		);


	}
	// ===== ボールスプライト描画 =====
	if (showBallSprite && ballSprite)
	{
		float sizeX = ballSprite->texture2d_desc.Width * ballSpriteScale.x;
		float sizeY = ballSprite->texture2d_desc.Height * ballSpriteScale.y;

		ballSprite->render(
			rc.context,
			ballSpritePosition.x - sizeX * 0.5f,
			ballSpritePosition.y - sizeY * 0.5f,
			sizeX,
			sizeY,
			1, 1, 1, 1,
			0.0f
		);
	}

}

// GUI描画
void strikeZone::DrawGUI()
{
	// ストライクゾーン画像の制御
	if (ImGui::CollapsingHeader("Strike Zone Image"))
	{
		ImGui::Checkbox("Show Image", &showStrikeZoneImage);
		ImGui::DragFloat2("Screen Position", &spritePosition.x, 1.0f, 0.0f, 2000.0f);
		ImGui::DragFloat2("Scale", &spriteScale.x, 0.01f, 0.1f, 5.0f);
		ImGui::ColorEdit4("Tint", &spriteTint.x);
	}

	// バットミートカーソルの制御
	if (ImGui::CollapsingHeader("Bat Meet Cursor"))
	{
		ImGui::Checkbox("Show Cursor", &showBatCursor);
		ImGui::DragFloat2("Cursor Scale", &cursorScale.x, 0.01f, 0.01f, 5.0f);
	}

	//ボールスプライトの制御
	if (ImGui::CollapsingHeader("Ball Sprite"))
	{
		ImGui::Checkbox("Show Ball Sprite", &showBallSprite);
		ImGui::DragFloat2("Ball Sprite Scale", &ballSpriteScale.x, 0.01f, 0.01f, 5.0f);
		ImGui::DragFloat2("Ball Sprite Position", &ballSpritePosition.x, 1.0f, 0.0f, 2000.0f);
	}

	ImGui::Checkbox("Enable Cursor Movement", &isMoveCursor);
}



void strikeZone::SetBallScreenPosition(const DirectX::XMFLOAT2& pos)
{
	ballSpritePosition = pos;
}

void strikeZone::SetBallVisible(bool visible)
{
	showBallSprite = visible;
}
