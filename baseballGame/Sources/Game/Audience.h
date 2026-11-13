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

	//モデルのy軸の回転をボールの位置によって更新する
	void UpdateAudienceRotation(const DirectX::XMFLOAT3& cameraPosition)
	{
		for (int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
		{
			for (size_t j = 0; j < audiencePositions[i].size(); ++j)
			{
				DirectX::XMFLOAT3 direction;
				direction.x = cameraPosition.x - audiencePositions[i][j].x;
				direction.y = 0.0f; // y軸の回転のみを考慮するため、y成分は0に設定
				direction.z = cameraPosition.z - audiencePositions[i][j].z;
				float angle = atan2f(direction.x, direction.z); // xとzの成分から角度を計算
				audienceRotations[i][j].y = angle; // y軸の回転を更新
				// ワールド行列を再計算
				DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(audienceScales[i][j].x, audienceScales[i][j].y, audienceScales[i][j].z);
				DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(audienceRotations[i][j].x, audienceRotations[i][j].y, audienceRotations[i][j].z);
				DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(audiencePositions[i][j].x, audiencePositions[i][j].y, audiencePositions[i][j].z);
				DirectX::XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
				DirectX::XMStoreFloat4x4(&audienceTransforms[i][j], worldMatrix);
			}
		}
	}


	//positionを更新して、ジャンプしているように見せる
	void UpdateAudienceJump(float elapsedTime);
	
	//置けるかどうかを判定する
	bool CanPlace(const DirectX::XMFLOAT3& newPos);

	bool GetIsCanPlacing() const { return isCanPlacing; }
	
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

	std::vector<DirectX::XMFLOAT3> currentAudiencePositions[static_cast<int>(AudienceModelType::Count)];

	bool initialized = false;
	// アニメーション関連
	float animation_time = 0.0f;
	int current_animation_index = 0;

	float jumpTimer = 0.0f;
	float junpDuration = 0.3f;
	float jumpHeight = 0.5f; // ジャンプの高さ

	float radius = 1.0f; // 観客の半径
	bool isCanPlacing = false; // 置けるかどうかのフラグ

	std::vector<float> audienceJumpDelays[static_cast<int>(AudienceModelType::Count)]; // 観客ごとの遅延時間

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
		
		default:
			return "";
		}
	}

	//ジャンプするまでの遅延時間をモデルタイプごとに設定
	void SetAudienceJumpDelays()
	{
		for(int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
		{
			audienceJumpDelays[i].clear();
			for (size_t j = 0; j < audiencePositions[i].size(); ++j)
			{
				float delay = static_cast<float>(rand()) / RAND_MAX * 2.0f; // 0.0fから2.0fの範囲でランダムな遅延時間を生成
				audienceJumpDelays[i].push_back(delay);

			}
		}	
	}

};