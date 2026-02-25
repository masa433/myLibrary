#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "Model.h"
#include "game_object.h"
#include "RenderContext.h"
#include "physxManager.h"

class PitchingNet : public GameObject
{
public:
	//インスタンス
	static PitchingNet& Instance()
	{
		static PitchingNet instance;
		return instance;
	}
	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(RenderContext& rc);
	void DrawGUI();

private:
	std::unique_ptr<Model> net;
	std::vector<physx::PxTriangleMesh*> triangle_meshes;
	std::vector<physx::PxActor*> actors;
	DirectX::XMFLOAT4X4					transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
};