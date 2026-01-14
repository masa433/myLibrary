#include "pch.h"
#include "System/Graphics.h"
#include "SceneTitle.h"
#include "System/Input.h"
#include "SceneGame.h"
#include "SceneManager.h"
#include "SceneLoading.h"



void SceneTitle::Initialize() 
{
	sprite = new Sprite("Data/Sprite/Title.png");
	
}

void SceneTitle::Finalize() 
{
	if (sprite != nullptr)
	{
		delete sprite;
		sprite = nullptr;
	}

	/*if (titleBGM)
	{
		delete titleBGM;
		titleBGM = nullptr;
	}*/
	
}


void SceneTitle::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// なにかボタンを押したらゲームシーンへ切り替え
	const GamePadButton anyButton =
		GamePad::BTN_A
		| GamePad::BTN_B
		| GamePad::BTN_X
		| GamePad::BTN_Y;

	if (gamePad.GetButtonDown() & anyButton)
	{
		//SceneManager::Instance().ChangeScene(new SceneGame);
		//SceneManager::Instance().ChangeScene(new SceneLoading);
		SceneManager::Instance().ChangeScene(new SceneLoading(new SceneGame));

	}

	/*titleBGM->Play(true,0.5f);
	titleBGM->SetVolume(1.0f);*/
}



void SceneTitle::Render() 
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();

	//描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = graphics.GetRenderState();

	//2Dスプライト描画
	{
		//タイトル描画
		float screenWidth = static_cast<float>(graphics.GetScreenWidth());
		float screenHeight = static_cast<float>(graphics.GetScreenHeight());
		sprite->Render(rc,
			0, 0, 0, screenWidth, screenHeight, 
			0,
			1, 1, 1, 1);
	}

}

void SceneTitle::DrawGUI() 
{

}