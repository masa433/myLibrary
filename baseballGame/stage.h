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
	std::unique_ptr<Model> pole;
	std::unique_ptr<gltf_model> stand2;
	std::unique_ptr<gltf_model> ground2;
	std::unique_ptr<gltf_model> pole2;
	std::vector<physx::PxTriangleMesh*> triangle_meshes;
	std::vector<physx::PxActor*> actors;
	DirectX::XMFLOAT4X4					transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

public:
	//ホームラン判定用のボックストリガーコライダー
	physx::PxRigidStatic* homeRunTrigger = nullptr;
	DirectX::XMFLOAT3 hrTriggerPos = { 0.0f, 10.0f, -80.0f }; // トリガーの初期位置
	DirectX::XMFLOAT3 hrTriggerHalfExtents = { 60.0f, 20.0f, 10.0f }; // トリガーの半分のサイズ(XYZ)

	//フェアかファウルかの判定用のボックストリガーコライダー
	physx::PxRigidStatic* fairFoulTrigger = nullptr;
	DirectX::XMFLOAT3 ffTriggerPos = { 0.0f, 10.0f, -80.0f }; // トリガーの初期位置
	DirectX::XMFLOAT3 ffTriggerHalfExtents = { 60.0f, 20.0f, 10.0f }; // トリガーの半分のサイズ(XYZ)
};