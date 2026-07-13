#pragma once
#include <d3d11.h>

class scene
{
public:
    scene() = default;
    virtual ~scene() = default;
    virtual void initialize() = 0;
    virtual void update(float elapsed_time) = 0;
    virtual void render(float elapsedTime) = 0;
    virtual void uninitialize() = 0;
    // GUI描画処理
    virtual void DrawGUI() {}

    //準備完了しているか
	bool IsReady() const { return isReady; }

    //準備完了設定
	void SetReady() { isReady = true; }
private:
    bool isReady = false;
};