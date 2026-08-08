#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "RenderContext.h"
#include "ModelRenderer.h"
#include "game_object.h"
#include "physxManager.h"
#include "json.hpp"

using json = nlohmann::json;

class gltf_model;

class BallNet : public GameObject
{
public:
	//インスタンス
	static BallNet& Instance()
	{
		static BallNet instance;
		return instance;
	}

	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

private:
	static constexpr int NET_COUNT = 4; // ネットの数

	std::unique_ptr<gltf_model> netModels2[NET_COUNT]; // ネットのモデル

	DirectX::XMFLOAT4X4					netTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	DirectX::XMFLOAT3 NetPositions[NET_COUNT] = {
		DirectX::XMFLOAT3(-10.0f, 0.0f, 40.0f), // ネット1の位置
		DirectX::XMFLOAT3(10.0f, 0.0f, 40.0f),  // ネット2の位置
		DirectX::XMFLOAT3(-18.0f, 0.0f, 25.0f), // ネット3の位置
		DirectX::XMFLOAT3(18.0f, 0.0f, 25.0f)   // ネット4の位置
	};

	DirectX::XMFLOAT3 NetScales[NET_COUNT] = {
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f), // ネット1のスケール
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f), // ネット2のスケール
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f), // ネット3のスケール
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f)  // ネット4のスケール
	};

	DirectX::XMFLOAT3 NetAngles[NET_COUNT] = {
		DirectX::XMFLOAT3(0.0f, -0.3f, 0.0f), // ネット1の角度
		DirectX::XMFLOAT3(0.0f, 0.3f, 0.0f), // ネット2の角度
		DirectX::XMFLOAT3(0.0f, -0.8f, 0.0f), // ネット3の角度
		DirectX::XMFLOAT3(0.0f, 0.8f, 0.0f)  // ネット4の角度
	};

	bool isInstancingEnabled = true; // インスタンシング描画の有効/無効


	physx::PxRigidStatic* netCollider[NET_COUNT] = {}; // ネットのコライダー
	physx::PxMaterial* netColliderMaterial = nullptr;   // ネットのトリガー用マテリアル

	DirectX::XMFLOAT3 netColliderPositions[NET_COUNT] = {
		DirectX::XMFLOAT3(-10.0f, 0.0f, 40.0f), // ネット1のコライダー位置
		DirectX::XMFLOAT3(10.0f, 0.0f, 40.0f),  // ネット2のコライダー位置
		DirectX::XMFLOAT3(-18.0f, 0.0f, 25.0f), // ネット3のコライダー位置
		DirectX::XMFLOAT3(18.0f, 0.0f, 25.0f)   // ネット4のコライダー位置
	};

	DirectX::XMFLOAT3 netColliderScales[NET_COUNT] = {
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f), // ネット1のコライダースケール
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f), // ネット2のコライダースケール
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f), // ネット3のコライダースケール
		DirectX::XMFLOAT3(1.2f, 1.0f, 1.0f)  // ネット4のコライダースケール
	};

	DirectX::XMFLOAT3 netColliderAngles[NET_COUNT] = {
		DirectX::XMFLOAT3(0.0f, -0.3f, 0.0f), // ネット1のコライダー角度
		DirectX::XMFLOAT3(0.0f, 0.3f, 0.0f), // ネット2のコライダー角度
		DirectX::XMFLOAT3(0.0f, -0.8f, 0.0f), // ネット3のコライダー角度
		DirectX::XMFLOAT3(0.0f, 0.8f, 0.0f)  // ネット4のコライダー角度
	};

	std::string netColliderNames[NET_COUNT];

};