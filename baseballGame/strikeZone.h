#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "sprite.h"
#include "Graphics.h"
#include "RenderContext.h"

class strikeZone 
{
	public:
		//インスタンス
		static strikeZone& Instance()
		{
			static strikeZone instance;
			return instance;
		}
		void Initialize();
		void Uninitialize();
		void Update(float elapsedTime);
		void Render(RenderContext& rc);
		void DrawGUI();

		bool IsStrike(const DirectX::XMFLOAT2& ballPosition);


		// 外部から制御
		void SetBallScreenPosition(const DirectX::XMFLOAT2& pos);
		void SetBallVisible(bool visible);

		//バットミートカーソル用
		std::unique_ptr<sprite> batCursor;
		bool showBatCursor = true;
		DirectX::XMFLOAT2 cursorPos = { 0.0f,0.0f };
		DirectX::XMFLOAT2 normalizedCursorPos = { 0.0f,0.0f };
		DirectX::XMFLOAT2 cursorScale = { 0.1f,0.1f };

		DirectX::XMFLOAT2 mouseCursorPos = { 0.0f,0.0f };

		// ストライクゾーン表示用スプライト
		std::unique_ptr<sprite> strikeZoneSprite;
		bool showStrikeZoneImage = true;
		DirectX::XMFLOAT2 spritePosition = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 spriteScale = { 0.0f, 0.0f };
		DirectX::XMFLOAT4 spriteTint = { 1.0f, 1.0f, 1.0f, 1.0f };

		bool isMoveCursor = false;

		//ボール表示
		std::unique_ptr<sprite> ballSprite;
		DirectX::XMFLOAT2 ballSpritePosition = { 0.0f,0.0f };
		DirectX::XMFLOAT2 ballSpriteScale = { 0.05f,0.05f };
		bool showBallSprite = false;

};