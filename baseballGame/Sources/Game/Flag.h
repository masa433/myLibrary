#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "../Model/gltf_model.h"
#include "game_object.h"
#include "RenderContext.h"
#include "physxManager.h"
#include "ModelRenderer.h"

class Flag :  public GameObject
{
public:
	enum class FlagColor
	{
		Blue,
		Green,
		Japan,
		Red,
		Yellow,
	};

	void Initialize();
	void UnInitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();

private:
	std::unique_ptr<gltf_model> flagModel;
};