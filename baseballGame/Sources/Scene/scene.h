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
    // GUI•`‰æˆ—
    virtual void DrawGUI() {}

    //€”õŠ®—¹‚µ‚Ä‚¢‚é‚©
	bool IsReady() const { return isReady; }

    //€”õŠ®—¹İ’è
	void SetReady() { isReady = true; }
private:
    bool isReady = false;
};