#include "GameIntroSequence.h"

void GameIntroSequence::UpdateIntro(float elapsed_time,BroadcastCamera& broadcastCamera)
{
    introTimer += elapsed_time;

    if (!introStarted)
    {
        introStarted = true;

        // ピッチャーを映している時はカメラIDを12か13のどちらかに設定する

        int pitcherCameraId = (rand() % 2 == 0) ? 12 : 13; // ピッチャーカメラ1 or 2
        int batterCameraId = (rand() % 2 == 0) ? 14 : 15; // バッターカメラ1 or 2

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
        }
        else if (introState == GameIntroState::ShowingBatter)
        {
            introState = GameIntroState::Playing;
            broadcastCamera.StopAllTracking();
            
        }
    }
}