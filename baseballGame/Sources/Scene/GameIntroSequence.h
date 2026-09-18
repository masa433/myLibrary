#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "BroadcastCamera.h"

class GameIntroSequence
{
public:
	enum class GameIntroState
	{
		ShowingPitcher,
		ShowingBatter,
		Playing,
	};

	void UpdateIntro(float elapsedTime,BroadcastCamera& broadcastCamera);// イントロの更新処理


	GameIntroState GetIntroState() const { return introState; }
	bool IsPlaying() const { return introState == GameIntroState::Playing; }

private:
	GameIntroState introState = GameIntroState::ShowingPitcher;
	float introTimer = 0.0f;// イントロのタイマー
	bool introStarted = false;
	float introDuration = 10.0f; // イントロの表示時間(カメラのズーム終了までにかかる時間)
};