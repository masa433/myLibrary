#pragma once


enum class GameMode
{
	Game,
	Tutorial,
	FreeBatting,
};

enum class TimeScaleMode
{
	Normal,//通常
	Medium,//中速
	Fast,//高速
	
};

class GameModeManager
{
public:

    static GameModeManager& Instance()
    {
        static GameModeManager instance;
        return instance;
	}

private:
    GameMode currentMode; // 現在のゲームモード
	float timeScale = 1.0f; // ゲームの時間の進み具合を制御する変数

public:
    GameModeManager() {};

    // モードの変更と取得
	void ChangeMode(GameMode newMode) { currentMode = newMode; }
	void ChangeTimeScaleMode(TimeScaleMode newTimeScaleMode)
	{
		switch (newTimeScaleMode)
		{
		case TimeScaleMode::Normal:
			timeScale = 1.0f;
			break;
		case TimeScaleMode::Medium:
			timeScale = 1.25f;
			break;
		case TimeScaleMode::Fast:
			timeScale = 1.5f;
			break;
		default:
			timeScale = 1.0f; // デフォルトはNormal
			break;
		}
	}
        
	GameMode GetCurrentMode() const { return currentMode; }
	TimeScaleMode GetCurrentTimeScaleMode() const
	{
		if (timeScale == 1.0f)
			return TimeScaleMode::Normal;
		else if (timeScale == 1.25f)
			return TimeScaleMode::Medium;
		else if (timeScale == 1.5f)
			return TimeScaleMode::Fast;
		else
			return TimeScaleMode::Normal; // デフォルトはNormal
	}

	float GetTimeScale() const { return timeScale; }
	void SetTimeScale(float newTimeScale) { timeScale = newTimeScale; }
};
