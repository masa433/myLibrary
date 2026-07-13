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
#include "FrustumCulling.h"
#include "imgui.h"
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
	void render(const RenderContext& rc, ModelRenderer* renderer, class FrustumCulling* frustumCulling = nullptr);
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
	
	// フェンスラインの編集用
	struct LineTriggerEditor
	{
		std::vector<DirectX::XMFLOAT3> linePoints; // フェンスラインの頂点座標
		std::vector<physx::PxRigidStatic*> triggers; // フェンスラインのトリガーコライダー
		bool editMode = false;   // 編集モードのON/OFF
		float thickness = 0.5f; // フェンスラインの厚み
		float extraHeight = 40.0f; // フェンス上端からさらに上へ判定を伸ばす高さ

		std::string triggerName;
		std::string raycastTargetName;
	};

	LineTriggerEditor homerunLineEditor; // ホームランラインの編集用データ
	LineTriggerEditor foulLineEditor; // ファウルラインの編集用データ

	void UpdateLineEditor(LineTriggerEditor& editor,
		const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
		float viewportX, float viewportY, float viewportWidth, float viewportHeight);

	void RebuildLineTriggers(LineTriggerEditor& editor);

	void DrawLineOverlay(const LineTriggerEditor& editor,
		const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
		float viewportX, float viewportY, float viewportWidth, float viewportHeight,
		ImU32 lineColor, ImU32 pointColor);
};