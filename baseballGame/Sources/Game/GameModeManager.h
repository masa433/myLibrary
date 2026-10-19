#pragma once


enum class GameMode
{
	Game,
	Tutorial,
	FreeBatting,
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

public:
    GameModeManager() {};

    // モードの変更と取得
	void ChangeMode(GameMode newMode) { currentMode = newMode; }
        
	GameMode GetCurrentMode() const { return currentMode; }
};
