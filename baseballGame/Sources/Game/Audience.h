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
		audiencePositions[static_cast<int>(currentModelType)].push_back(position);
		audienceScales[static_cast<int>(currentModelType)].push_back(scale);
		audienceRotations[static_cast<int>(currentModelType)].push_back(rotation);
		DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		DirectX::XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		DirectX::XMFLOAT4X4 worldTransform;
		DirectX::XMStoreFloat4x4(&worldTransform, worldMatrix);
		audienceTransforms[static_cast<int>(currentModelType)].push_back(worldTransform);
	}


private:
	enum class AudienceModelType
	{

		MaleUniform,
		FemaleUniform,
		MaleTShirt,
		FemaleTShirt,
		MaleFlag,
		FemaleFlag,
		MaleParker,
		Suits,
		Elderly,
		Child,


		Count,
	};

	AudienceModelType currentModelType = AudienceModelType::MaleUniform;

private:
	std::unique_ptr<gltf_model> audienceModel[static_cast<int>(AudienceModelType::Count)];
	std::vector<gltf_model::node> animated_nodes;
	std::vector<DirectX::XMFLOAT4X4> audienceTransforms[static_cast<int>(AudienceModelType::Count)];
	std::vector<DirectX::XMFLOAT3> audiencePositions[static_cast<int>(AudienceModelType::Count)];
	std::vector<DirectX::XMFLOAT3> audienceScales[static_cast<int>(AudienceModelType::Count)];
	std::vector<DirectX::XMFLOAT3> audienceRotations[static_cast<int>(AudienceModelType::Count)];
	bool initialized = false;
	// アニメーション関連
	float animation_time = 0.0f;
	int current_animation_index = 0;

	//ファイル名を取得
	std::string GetModelFileName(AudienceModelType modelType)
	{
		switch (modelType)
		{
		case AudienceModelType::MaleUniform:
			return ".\\resources\\audience\\maleUniform.glb";
		case AudienceModelType::FemaleUniform:
			return ".\\resources\\audience\\femaleUniform.glb";
		case AudienceModelType::MaleTShirt:
			return ".\\resources\\audience\\maleTShirt.glb";
		case AudienceModelType::FemaleTShirt:
			return ".\\resources\\audience\\femaleTShirt.glb";
		case AudienceModelType::MaleFlag:
			return ".\\resources\\audience\\maleFlag.glb";
		case AudienceModelType::FemaleFlag:
			return ".\\resources\\audience\\femaleFlag.glb";
		case AudienceModelType::MaleParker:
			return ".\\resources\\audience\\maleParker.glb";
		case AudienceModelType::Suits:
			return ".\\resources\\audience\\suits.glb";
		case AudienceModelType::Elderly:
			return ".\\resources\\audience\\elderly.glb";
		case AudienceModelType::Child:
			return ".\\resources\\audience\\child.glb";
		default:
			return "";
		}
	}
};