#include "pch.h"
#include "System/Graphics.h"
#include "System/Input.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include <cmath> // sin関数を使用

void SceneLoading::Initialize()
{
	sprite = new Sprite("Data/Sprite/Loading.png");
	dot = new Sprite("Data/Sprite/LoadingDot.png");

	time = 0.0f;             // 経過時間の初期化
	bounceTimer = 0.0f;      // 跳ねる間隔タイマーの初期化
	currentDotIndex = 0;     // 最初に跳ねる点のインデックス

	// スレッド開始
	thread = new std::thread(LoadingThread, this);
}

void SceneLoading::Finalize()
{
	// スレッド終了化
	if (thread != nullptr)
	{
		thread->join();
		delete thread;
		thread = nullptr;
	}

	if (sprite != nullptr)
	{
		delete sprite;
		sprite = nullptr;
	}

	if (dot != nullptr)
	{
		delete dot;
		dot = nullptr;
	}
}

void SceneLoading::Update(float elapsedTime)
{
	// 経過時間の更新
	time += elapsedTime;
	bounceTimer += elapsedTime;

	// 現在の dot の跳ねる動作を終了して次の dot へ
	constexpr float bounceInterval = 0.5f; // 0.5秒間隔
	if (bounceTimer >= bounceInterval)
	{
		bounceTimer = 0.0f;
		currentDotIndex = (currentDotIndex + 1) % 4; // 0 → 1 → 2 → 0 の循環
	}

	// 次のシーンの準備が完了したらシーンを切り替える
	if (nextScene->IsReady())
	{
		SceneManager::Instance().ChangeScene(nextScene);
		nextScene = nullptr;
	}
}

void SceneLoading::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();

	// 描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;

	// 画面右下にローディングアイコンを描画
	float screenWidth = static_cast<float>(graphics.GetScreenWidth());
	float screenHeight = static_cast<float>(graphics.GetScreenHeight());
	float spriteWidth = 300;
	float spriteHeight = 64;
	float positionX = 0;
	float positionY = screenHeight - spriteHeight;

	// "Loading" スプライト描画
	sprite->Render(rc,
		positionX, positionY, 0, spriteWidth, spriteHeight,
		0,
		1, 1, 1, 1);

	// "LoadingDot" スプライトを描画
	float dotWidth = 16;  // Dotの幅
	float dotHeight = 16; // Dotの高さ
	float dotSpacing = 32; // Dot同士の間隔
	float dotStartX = positionX + spriteWidth + 16; // "Loading" スプライトの右端から少し空ける
	float baseY = positionY + (spriteHeight - dotHeight) / 2; // 垂直方向中央揃え

	for (int i = 0; i < 3; ++i)
	{
		// 跳ねる動作の計算
		float bounce = 0.0f;
		if (i == currentDotIndex) // 現在跳ねる対象の点
		{
			// 時間経過に基づく跳ねる動作
			constexpr float bounceDuration = 0.5f; // 跳ねる時間
			float progress = (bounceTimer / bounceDuration); // 0.0～1.0の進行度
			if (progress <= 0.5f) // 上昇
			{
				bounce = std::sin(progress * DirectX::XM_PI) * 50.0f; // 振幅20
			}
		}

		float dotPositionX = dotStartX + i * (dotWidth + dotSpacing);
		float dotPositionY = baseY - bounce;

		dot->Render(rc,
			dotPositionX, dotPositionY, 0, dotWidth, dotHeight,
			0,
			1, 1, 1, 1);
	}
}

void SceneLoading::DrawGUI()
{
	// GUI描画 (必要に応じて実装)
}

void SceneLoading::LoadingThread(SceneLoading* scene)
{
	// COM関連の初期化
	CoInitialize(nullptr);

	// 次のシーンの初期化を行う
	scene->nextScene->Initialize();

	// COM関連の終了化
	CoUninitialize();

	// 次のシーンの準備完了設定
	scene->nextScene->SetReady();
}
