#pragma once
#include "game_object.h"
#include "../Model/gltf_model.h"
#include "FrustumCulling.h"

using json = nlohmann::json;

class Audience : public GameObject
{
public:

	//インスタンス
	static Audience& Instance()
	{
		static Audience instance;
		return instance;
	}

	Audience() = default;
	~Audience() = default;
	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(FrustumCulling* frustumCulling);
	void DrawGUI();
	void UpdateAnimation(float elapsedTime);

	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	//観客の情報を個別のjsonに保存するメソッド
	void SaveToFile(const std::string& filename);
	void LoadFromFile(const std::string& filename);

	//スタンドとマウスカーソルのレイキャストを更新するメソッド
	bool UpdateRayCastToStand(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
		float viewportWidth, float viewportHeight, DirectX::XMFLOAT3& outHitPos,
		const DirectX::XMFLOAT2& mouseInViewport);

	
	//モデルの情報を格納
	void AddAudience(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& scale, const DirectX::XMFLOAT3& rotation)
	{
		audiencePositions.push_back(position);
		audienceScales.push_back(scale);
		audienceRotations.push_back(rotation);
		DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		DirectX::XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		DirectX::XMFLOAT4X4 worldTransform;
		DirectX::XMStoreFloat4x4(&worldTransform, worldMatrix);
		audienceTransforms.push_back(worldTransform);
	}

private:
	std::unique_ptr<gltf_model> audienceModel;
	std::vector<gltf_model::node> animated_nodes;
	std::vector<DirectX::XMFLOAT4X4> audienceTransforms;
	std::vector<DirectX::XMFLOAT3> audiencePositions;
	std::vector<DirectX::XMFLOAT3> audienceScales;
	std::vector<DirectX::XMFLOAT3> audienceRotations;
	bool initialized = false;
	// アニメーション関連
	float animation_time = 0.0f;
	int current_animation_index = 0;

	enum class AudienceState
	{
		ClappingCheer,
	} 
	audienceState = AudienceState::ClappingCheer;
};