#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "game_object.h"
#include "RenderContext.h"
#include "physxManager.h"
#include "ModelRenderer.h"
#include "ShaderId.h"
#include "Flag.h"
#include "json.hpp"

using json = nlohmann::json;

class gltf_model;
class Model;

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
	void DrawGUI();

	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

private:

	std::unique_ptr<Model> stand;
	std::unique_ptr<Model> ground;
	std::unique_ptr<Model> pole;
	std::unique_ptr<Model> lightTower;
	std::unique_ptr<gltf_model> stand2;
	std::unique_ptr<gltf_model> ground2;
	std::unique_ptr<gltf_model> pole2;
	std::unique_ptr<gltf_model> lightTower2;
	std::vector<physx::PxTriangleMesh*> triangle_meshes;
	std::vector<physx::PxActor*> actors;
	DirectX::XMFLOAT4X4					transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };


	DirectX::XMFLOAT3 standPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 standScale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 standAngle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 standTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	DirectX::XMFLOAT3 groundPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 groundScale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 groundAngle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 groundTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	DirectX::XMFLOAT3 polePosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 poleScale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 poleAngle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 poleTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	static constexpr int FLAG_COUNT = 5;
	std::vector<Flag> flags;
	
	static constexpr int TOWER_COUNT = 4;

	// ライトタワーの位置（4基分）
	DirectX::XMFLOAT3 towerPositions[TOWER_COUNT] =
	{
		
		{ -160.0f, 0.0f,  40.0f },
		{  160.0f, 0.0f,  40.0f },
		{  80.0f, 0.0f,  140.0f },
		{ -80.0f, 0.0f,  140.0f },
	};

	DirectX::XMFLOAT3 towerAngle[TOWER_COUNT] = {	
		
		{ 0.0f, DirectX::XMConvertToRadians(90.0f), 0.0f },
		{ 0.0f, DirectX::XMConvertToRadians(-90.0f), 0.0f },
		{ 0.0f, DirectX::XMConvertToRadians(-145.0f), 0.0f },
		{ 0.0f, DirectX::XMConvertToRadians(145.0f), 0.0f }
	};

	DirectX::XMFLOAT3 lightScale[TOWER_COUNT] =
	{
		
		{ 5.0f, 3.0f, 3.0f },
		{ 5.0f, 3.0f, 3.0f },
		{ 5.0f, 2.5f, 3.0f },
		{ 5.0f, 2.5f, 3.0f }
	};

public:
	//ホームラン判定用のボックストリガーコライダー
	physx::PxRigidStatic* homeRunTrigger = nullptr;
	DirectX::XMFLOAT3 hrTriggerPos = { 0.0f, 10.0f, -80.0f }; // トリガーの初期位置
	DirectX::XMFLOAT3 hrTriggerHalfExtents = { 60.0f, 20.0f, 10.0f }; // トリガーの半分のサイズ(XYZ)


};