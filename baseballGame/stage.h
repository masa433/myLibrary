#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "Model.h"
#include "game_object.h"
#include "RenderContext.h"
#include "physxManager.h"
#include "ModelRenderer.h"
#include "ShaderId.h"

class stage : public GameObject
{
public:

	//インスタンス
	static stage& Instance()
	{
		static stage instance;
		return instance;
	}

	stage() = default;
	virtual ~stage() = default;
	void initialize();
	void update(float elapsedTime);
	void render(const RenderContext& rc, ModelRenderer* renderer);
	void uninitialize();


private:

	std::unique_ptr<Model> stand;
	std::unique_ptr<Model> ground;
	std::vector<physx::PxTriangleMesh*> triangle_meshes;
	std::vector<physx::PxActor*> actors;
	DirectX::XMFLOAT4X4					transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };


};