#pragma once
#include <d3d11.h>

class scene2
{
public:
    scene2() = default;
    virtual ~scene2() = default;
    virtual void initialize() = 0;
    virtual void update(float elapsed_time) = 0;
    virtual void render(float elapsedTime) = 0;
    virtual void uninitialize() = 0;
    // GUI•`‰æˆ—
    virtual void DrawGUI() {}
};