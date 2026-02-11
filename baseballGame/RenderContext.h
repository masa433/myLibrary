#pragma once
#include "camera.h"
#include "Light.h"
#include "RenderState.h"

class RenderContext
{
public:
	ID3D11DeviceContext* context;
	const Camera* camera;
	const RenderState* renderState;
	const Light* light;
};