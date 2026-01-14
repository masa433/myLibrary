#include "pch.h"
#include "System/Input.h"

// ‰Šú‰»
void Input::Initialize(HWND hWnd)
{
	gamePad = std::make_unique<GamePad>();
	mouse = std::make_unique<Mouse>(hWnd);
}

// XVˆ—
void Input::Update()
{
	gamePad->Update();
	mouse->Update();
}
