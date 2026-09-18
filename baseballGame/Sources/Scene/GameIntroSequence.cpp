#include "GameIntroSequence.h"
#include "Player.h"
#include "Pitcher.h"

void GameIntroSequence::UpdateIntro(float elapsed_time,BroadcastCamera& broadcastCamera)
{
    introTimer += elapsed_time;

    if (!introStarted)
    {
        introStarted = true;

        // ピッチャーを映している時はカメラIDを12か13のどちらかに設定する
        int pitcherCameraId, batterCameraId;

        if(Pitcher::Instance().IsRightPitcher())
        {
			pitcherCameraId = 12; // 右投手用のカメラID
        }
        else
        {
			pitcherCameraId = 13; // 左投手用のカメラID
		}

        if (Player::Instance().IsRightBatter())
        {
			batterCameraId = 14; // 右打者用のカメラID
        }
        else
        {
            batterCameraId = 15; // 左打者用のカメラID
		}
        

        int cameraId = (introState == GameIntroState::ShowingPitcher) ? pitcherCameraId : batterCameraId;

        int index = broadcastCamera.GetCameraIndexById(cameraId);

        broadcastCamera.SetActiveIndex(index);
        broadcastCamera.ResetCameraToPreset(index);
        broadcastCamera.StartEventCameraZoom(DirectX::XMConvertToRadians(30.0f), introDuration);
    }

    if (introTimer >= introDuration)
    {
        introTimer = 0.0f;
        introStarted = false;
        if (introState == GameIntroState::ShowingPitcher)
        {
            introState = GameIntroState::ShowingBatter;
			introDuration = 9.0f; // バッターを映す時間に変更
        }
        else if (introState == GameIntroState::ShowingBatter)
        {
            introState = GameIntroState::Playing;
            broadcastCamera.StopAllTracking();
            
        }
    }
}