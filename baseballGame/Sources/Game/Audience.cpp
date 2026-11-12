#include "pch.h"
#include "Audience.h"
#include "Graphics.h"
#include "input.h"
#include "physxManager.h"
#include "Ball.h"

void Audience::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	audienceModel[static_cast<int>(AudienceModelType::MaleUniform)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::MaleUniform));
	audienceModel[static_cast<int>(AudienceModelType::FemaleUniform)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::FemaleUniform));
	audienceModel[static_cast<int>(AudienceModelType::MaleTShirt)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::MaleTShirt));
	audienceModel[static_cast<int>(AudienceModelType::FemaleTShirt)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::FemaleTShirt));
	audienceModel[static_cast<int>(AudienceModelType::MaleFlag)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::MaleFlag));
	audienceModel[static_cast<int>(AudienceModelType::FemaleFlag)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::FemaleFlag));
	audienceModel[static_cast<int>(AudienceModelType::MaleParker)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::MaleParker));
	audienceModel[static_cast<int>(AudienceModelType::Suits)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::Suits));
	audienceModel[static_cast<int>(AudienceModelType::Elderly)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::Elderly));
	audienceModel[static_cast<int>(AudienceModelType::Child)] = std::make_unique<gltf_model>(device,GetModelFileName(AudienceModelType::Child));
	
	for(int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		if(audienceModel[i])
		{
			audienceModel[i]->build_static_batches(device);
			animated_nodes = audienceModel[i]->nodes; // 初期状態のノードをコピー
		}
	}
	
	initialized = true;
	
	currentModelType = AudienceModelType::MaleUniform;

	jumpHeight = 0.5f; // ジャンプの高さを設定
	jumpTimer = 0.0f; // ジャンプのタイマーを初期化
	junpDuration = 0.5f; // ジャンプの持続時間を設定

	

}

void Audience::Uninitialize()
{
	for (int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		audienceModel[i].reset();
	}
	animated_nodes.clear();
	initialized = false;
}


void Audience::Update(float elapsedTime)
{
	if (!initialized) return;
	
	// アニメーションの更新
	UpdateTransform();

	//UpdateAnimation(elapsedTime);

	//レイキャストで置いた観客はCTRL+Zで元に戻す
	if(GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState('Z') & 0x8000)
	{
		int currentModelTypeIndex = static_cast<int>(currentModelType);
		if(!audiencePositions[currentModelTypeIndex].empty())
		{
			audiencePositions[currentModelTypeIndex].pop_back();
			audienceScales[currentModelTypeIndex].pop_back();
			audienceRotations[currentModelTypeIndex].pop_back();
			audienceTransforms[currentModelTypeIndex].pop_back();
		}
	}


	
	
}

void Audience::UpdateAudienceJump(float elapsedTime)
{
	jumpTimer += elapsedTime;
	if (jumpTimer >= junpDuration)
	{
		jumpTimer = 0.0f;
	}
	
	for (int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		for (size_t j = 0; j < audiencePositions[i].size(); ++j)
		{
			
			//各モデルでジャンプするまでの遅延時間を0.1秒ずつずらして設定
			float delayTime = audienceJumpDelays[i][j]; // 各観客の遅延時間を取得

			float currentTime = fmodf(jumpTimer + delayTime, junpDuration);// 現在のジャンプ時間を計算（遅延時間を加算してループさせる）

			
			float jumpProgress = currentTime / junpDuration; // ジャンプの進行度（0.0～1.0）
			float jumpOffset = sinf(jumpProgress * DirectX::XM_PI) * jumpHeight; // サイン波でジャンプのオフセットを計算

			
			audiencePositions[i][j].y = currentAudiencePositions[i][j].y + jumpOffset; // y座標にジャンプのオフセットを加算
			
		}
	}
}

bool Audience::CanPlace(const DirectX::XMFLOAT3& newPos)
{
	
	for(int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		for (const auto& pos : audiencePositions[i])
		{
			float dx = pos.x - newPos.x;
			float dz = pos.z - newPos.z;

			float distanceSquared = dx * dx + dz * dz;// y座標は無視してXZ平面での距離を計算

			if (distanceSquared < radius * radius)
			{
				return false; // 衝突している場合は配置できない
			}
		}
	}

	return true; // 衝突していない場合は配置できる
}

bool Audience::UpdateRayCastToStand(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
	float viewportWidth, float viewportHeight, DirectX::XMFLOAT3& outHitPos,
	const DirectX::XMFLOAT2& mouseInViewport)
{
	for (int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		if (!initialized || !audienceModel[i] || !audienceModel[i]->IsInitialized()) return false;
	}
	if (viewportHeight <= 0.0f || viewportWidth <= 0.0f) return false;
	

	//マウスの位置を取得
	DirectX::XMFLOAT2 mousePos = mouseInViewport;

	
	//スクリーン座標を正規化デバイス座標に変換
	float ndcX = (mousePos.x / viewportWidth) *  2.0f - 1.0f;
	float ndcY = 1.0f - (mousePos.y / viewportHeight) * 2.0f;

	if (!std::isfinite(proj._11) || !std::isfinite(proj._22) ||
		fabsf(proj._11) < 1e-6f || fabsf(proj._22) < 1e-6f) return false;

	DirectX::XMMATRIX View = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, View);

	float zDir = (proj._34 >= 0.0f) ? 1.0f : -1.0f;   // 左手系 +Z / 右手系 -Z
	DirectX::XMVECTOR dirView = DirectX::XMVectorSet(ndcX / proj._11, ndcY / proj._22, zDir, 0.0f);

	DirectX::XMVECTOR dir = DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(dirView, invView));
	DirectX::XMVECTOR rayStart = invView.r[3];   // カメラ位置

	DirectX::XMFLOAT3 rayOrigin, rayDir;
	DirectX::XMStoreFloat3(&rayOrigin, rayStart);
	DirectX::XMStoreFloat3(&rayDir, dir);
	if (!std::isfinite(rayDir.x + rayDir.y + rayDir.z)) return false;

	physx::PxScene* pxScene = Physics::Instance().GetScene();// PhysXのシーンを取得
	if (!pxScene) return false;// シーンが存在しない場合は処理を終了
	physx::PxRaycastBuffer hitInfo;// レイキャストの結果を格納するバッファ
	physx::PxQueryFilterData filterData;// フィルタリング用のデータ
	filterData.flags |= physx::PxQueryFlag::eSTATIC; // 静的オブジェクトのみを対象

	char buf[256];
	sprintf_s(buf, "mouse=(%f,%f) vp=(%f,%f) ndc=(%f,%f) dir=(%f,%f,%f) org=(%f,%f,%f)\n",
		mousePos.x, mousePos.y, viewportWidth, viewportHeight, ndcX, ndcY,
		rayDir.x, rayDir.y, rayDir.z, rayOrigin.x, rayOrigin.y, rayOrigin.z);
	OutputDebugStringA(buf);
	if (!std::isfinite(rayDir.x + rayDir.y + rayDir.z)) return false;

	// レイキャストを実行
	bool hit = pxScene->raycast(
		physx::PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z),// レイの原点
		physx::PxVec3(rayDir.x, rayDir.y, rayDir.z),// レイの方向
		1000.0f, // 最大距離
		hitInfo,// ヒット情報を格納するバッファ
		physx::PxHitFlag::eDEFAULT,// ヒット情報の取得フラグ
		filterData// フィルタリング情報
	);

	//ヒットした場合、その位置に観客のモデルを配置する
	if(hit && hitInfo.hasBlock)
	{
		physx::PxRigidActor* actor = hitInfo.block.actor;
		if (actor && actor->getName())
		{
			std::string actorName = actor->getName();// ヒットしたアクターの名前を取得
			if (actorName == "Stand")// ヒットしたアクターが"Stand"の場合
			{
				//置けるかどうか
				if (CanPlace(DirectX::XMFLOAT3(hitInfo.block.position.x, hitInfo.block.position.y, hitInfo.block.position.z)))
				{
					outHitPos.x = hitInfo.block.position.x;
					outHitPos.y = hitInfo.block.position.y;
					outHitPos.z = hitInfo.block.position.z;

					return true;
				}
			}

			//ヒットしたらヒットしたアクターの名前を出力する
			char buf[256];
			sprintf_s(buf, "Hit Actor Name: %s\n", actorName.c_str());
			OutputDebugStringA(buf);
		}	
	}

	return false;
}

void Audience::UpdateAnimation(float elapsedTime)
{
	if (!initialized) return;

	// アニメーションの更新
	for (int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		if (!audienceModel[i]->animations.empty())
		{
			//カメラとの距離を取得
			Camera& camera = Camera::Instance();
			const DirectX::XMFLOAT3 cameraEye = camera.GetEye();

			DirectX::XMFLOAT3 audiencePosition = GetPosition();

			//距離に応じてアニメーションするかしないかを決める
			DirectX::XMFLOAT3 diff;
			diff.x = audiencePosition.x - cameraEye.x;
			diff.y = audiencePosition.y - cameraEye.y;
			diff.z = audiencePosition.z - cameraEye.z;

			float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

			if (distance < 50.0f) // 距離が50.0f未満の場合のみアニメーションを更新
			{

				animation_time += elapsedTime;
				audienceModel[i]->animate(current_animation_index, animation_time, animated_nodes);

				if (animation_time > audienceModel[i]->animations[current_animation_index].duration)
				{
					animation_time = 0.0f; // アニメーションの時間をリセット
				}
			}
		}
	}

}

void Audience::Render(FrustumCulling* frustumCulling)
{
	for (int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		if (!initialized || !audienceModel[i] || !audienceModel[i]->IsInitialized()) return;
	
		ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
		RenderState* renderState = Graphics::Instance().GetRenderState();
		
		// モデルの描画
		std::vector<DirectX::XMFLOAT4X4> audienceTransform;
		audienceTransform.reserve(audienceTransforms[i].size());


		ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
		for (auto& transform : audienceTransforms[i])
		{
			if (audienceModel[i])
			{
				bool isVisible = true;

				if (frustumCulling)
				{
					const auto& sphere = audienceModel[i]->GetBoundingSphere();
					isVisible = frustumCulling->IsTransformedSphereVisible(sphere.center, sphere.radius, transform);
				}

				if (isVisible)
				{
					// オーディエンスのモデルをレンダリングする処理をここに追加
					audienceTransform.push_back(transform);
				}
			}
		}

		if(!audienceTransform.empty())
		{
			//背景を透過して描画するために、ブレンドステートを設定

			//dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);


			audienceModel[i]->render_batched_instanced(context, audienceTransform);

			//dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
		}
	}

}

void Audience::DrawGUI()
{
	if (!initialized) return;
	if(ImGui::CollapsingHeader("Audience Manager"))
	{
		// 観客のモデルタイプを選択するコンボボックス
		const char* modelTypeNames[] = {
			"MaleUniform",
			"FemaleUniform",
			"MaleTShirt",
			"FemaleTShirt",
			"MaleFlag",
			"FemaleFlag",
			"MaleParker",
			"Suits",
			"Elderly",
			"Child"
		};

		int currentModelTypeIndex = static_cast<int>(currentModelType);

		if (ImGui::Combo("Model Type", &currentModelTypeIndex, modelTypeNames, IM_ARRAYSIZE(modelTypeNames)))
		{
			currentModelType = static_cast<AudienceModelType>(currentModelTypeIndex);
		}

		//現在のモデルのファイル名を表示
		if (audienceModel[currentModelTypeIndex])
		{
			ImGui::Text("Current Model: %s", GetModelFileName(currentModelType).c_str());
		}
		else
		{
			ImGui::Text("Current Model: None");
		}

		//ジャンプ関連
		ImGui::DragFloat("Jump Duration", &junpDuration, 0.1f, 0.1f, 5.0f);
		ImGui::DragFloat("Jump Height", &jumpHeight, 0.1f, 0.1f, 5.0f);

		ImGui::Checkbox("Is Can Place", &isCanPlacing);
		ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 100.0f);
	}
}

void Audience::SaveToJson(json& j)
{
	for(int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		j["audiencePositions"][i] = json::array();
		for(const auto& pos : audiencePositions[i])
		{
			j["audiencePositions"][i].push_back({pos.x, pos.y, pos.z});
		}

		j["audienceScales"][i] = json::array();
		for(const auto& scale : audienceScales[i])
		{
			j["audienceScales"][i].push_back({scale.x, scale.y, scale.z});
		}
		j["audienceRotations"][i] = json::array();
		for(const auto& rot : audienceRotations[i])
		{
			j["audienceRotations"][i].push_back({rot.x, rot.y, rot.z});
		}
		j["audienceTransforms"][i] = json::array();
		for(const auto& trans : audienceTransforms[i])
		{
			j["audienceTransforms"][i].push_back({
				{trans._11, trans._12, trans._13, trans._14},
				{trans._21, trans._22, trans._23, trans._24},
				{trans._31, trans._32, trans._33, trans._34},
				{trans._41, trans._42, trans._43, trans._44}
			});
		}
	}
}

void Audience::LoadFromJson(const json& j)
{
	for (int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		audiencePositions[i].clear();
		audienceScales[i].clear();
		audienceRotations[i].clear();
		audienceTransforms[i].clear();

	}

	for(int i = 0; i < static_cast<int>(AudienceModelType::Count); ++i)
	{
		if(j.contains("audiencePositions") && j["audiencePositions"].is_array() && j["audiencePositions"].size() > i)
		{
			for(const auto& pos : j["audiencePositions"][i])
			{
				DirectX::XMFLOAT3 p = { pos[0], pos[1], pos[2] };
				audiencePositions[i].push_back(p);

				currentAudiencePositions[i].push_back(p); // 現在の観客の位置も更新
			}
		}
		if(j.contains("audienceScales") && j["audienceScales"].is_array() && j["audienceScales"].size() > i)
		{
			for(const auto& scale : j["audienceScales"][i])
			{
				DirectX::XMFLOAT3 s = { scale[0], scale[1], scale[2] };
				audienceScales[i].push_back(s);
			}
		}
		if(j.contains("audienceRotations") && j["audienceRotations"].is_array() && j["audienceRotations"].size() > i)
		{
			for(const auto& rot : j["audienceRotations"][i])
			{
				DirectX::XMFLOAT3 r = { rot[0], rot[1], rot[2] };
				audienceRotations[i].push_back(r);
			}
		}
		if(j.contains("audienceTransforms") && j["audienceTransforms"].is_array() && j["audienceTransforms"].size() > i)
		{
			for(const auto& trans : j["audienceTransforms"][i])
			{
				DirectX::XMFLOAT4X4 t = {
					trans[0][0], trans[0][1], trans[0][2], trans[0][3],
					trans[1][0], trans[1][1], trans[1][2], trans[1][3],
					trans[2][0], trans[2][1], trans[2][2], trans[2][3],
					trans[3][0], trans[3][1], trans[3][2], trans[3][3]
				};
				audienceTransforms[i].push_back(t);
			}
		}
		
		SetAudienceJumpDelays();
		
	}
	
}

void Audience::SaveToFile(const std::string& filename)
{
	json j;
	SaveToJson(j);
	std::ofstream file(filename);
	if(file.is_open())
	{
		file << j.dump(4);
	}

}

void Audience::LoadFromFile(const std::string& filename)
{
	std::ifstream file(filename);
	if(file.is_open())
	{
		json j;
		file >> j;
		LoadFromJson(j);
	}
}