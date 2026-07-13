#include "input.h"

// 初期化
void Input::Initialize(HWND hWnd)
{
	gamePad = std::make_unique<GamePad>();
	mouse = std::make_unique<Mouse>(hWnd);
}

// 更新処理
void Input::Update()
{
	gamePad->Update();
	mouse->Update();
}
