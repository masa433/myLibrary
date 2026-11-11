#include "pch.h"
#include "Audience.h"
#include "Graphics.h"
#include "input.h"
#include "physxManager.h"

void Audience::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	audienceModel = std::make_unique<gltf_model>(device,".\\resources\\audience\\ClappingAudienceLow.glb");
	
	audienceModel->build_static_batches(device); // 静的バッチを構築

	initialized = true;

	animated_nodes = audienceModel->nodes; // 初期状態のノードをコピー

	audienceState = AudienceState::ClappingCheer;
}

void Audience::Uninitialize()
{
	audienceModel.reset();
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
		if(!audiencePositions.empty())
		{
			audiencePositions.pop_back();
			audienceScales.pop_back();
			audienceRotations.pop_back();
			audienceTransforms.pop_back();
		}
	}

}

bool Audience::UpdateRayCastToStand(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
	float viewportWidth, float viewportHeight, DirectX::XMFLOAT3& outHitPos,
	const DirectX::XMFLOAT2& mouseInViewport)
{
	if (!initialized || !audienceModel || !audienceModel->IsInitialized()) return false;
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
				outHitPos.x = hitInfo.block.position.x;
				outHitPos.y = hitInfo.block.position.y;
				outHitPos.z = hitInfo.block.position.z;
				
				return true;
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
	if(!audienceModel->animations.empty())
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
			audienceModel->animate(current_animation_index, animation_time, animated_nodes);

			if (animation_time > audienceModel->animations[current_animation_index].duration)
			{
				animation_time = 0.0f; // アニメーションの時間をリセット
			}
		}
	}

}

void Audience::Render(FrustumCulling* frustumCulling)
{
	if (!initialized || !audienceModel || !audienceModel->IsInitialized()) return;
	
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	
	// モデルの描画
	std::vector<DirectX::XMFLOAT4X4> audienceTransform;
	audienceTransform.reserve(audienceTransforms.size());


	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	for (auto& transform : audienceTransforms)
	{
		if (audienceModel)
		{
			bool isVisible = true;

			if (frustumCulling)
			{
				const auto& sphere = audienceModel->GetBoundingSphere();
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

		dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

		audienceModel->render_batched_instanced(context, audienceTransform);

		dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	}
}

void Audience::DrawGUI()
{
	if (!initialized) return;
	if(ImGui::CollapsingHeader("Audience Manager"))
	{
		//ボタンで観客のモデルの増減
		if(ImGui::Button("Add Audience"))
		{
			//観客のモデルを追加する処理
			AddAudience({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f});
		}
		if(ImGui::Button("Remove Audience"))
		{
			//観客のモデルを削除する処理
			if(!audiencePositions.empty())
			{
				audiencePositions.pop_back();
				audienceScales.pop_back();
				audienceRotations.pop_back();
				audienceTransforms.pop_back();
			}
		}
	}
}

void Audience::SaveToJson(json& j)
{
	for(auto& pos : audiencePositions)
	{
		j["audiencePositions"].push_back({ pos.x, pos.y, pos.z });
	}
	for(auto& scale : audienceScales)
	{
		j["audienceScales"].push_back({ scale.x, scale.y, scale.z });
	}
	for(auto& rot : audienceRotations)
	{
		j["audienceRotations"].push_back({ rot.x, rot.y, rot.z });
	}
	for(auto& transform : audienceTransforms)
	{
		j["audienceTransforms"].push_back({
			{transform._11, transform._12, transform._13, transform._14},
			{transform._21, transform._22, transform._23, transform._24},
			{transform._31, transform._32, transform._33, transform._34},
			{transform._41, transform._42, transform._43, transform._44}
		});
	}

}

void Audience::LoadFromJson(const json& j)
{
	audiencePositions.clear();
	audienceScales.clear();
	audienceRotations.clear();
	audienceTransforms.clear();
	for(auto& pos : j["audiencePositions"])
	{
		audiencePositions.push_back({ pos[0], pos[1], pos[2] });
	}
	for(auto& scale : j["audienceScales"])
	{
		audienceScales.push_back({ scale[0], scale[1], scale[2] });
	}
	for(auto& rot : j["audienceRotations"])
	{
		audienceRotations.push_back({ rot[0], rot[1], rot[2] });
	}
	for(auto& transform : j["audienceTransforms"])
	{
		audienceTransforms.push_back({
			transform[0][0], transform[0][1], transform[0][2], transform[0][3],
			transform[1][0], transform[1][1], transform[1][2], transform[1][3],
			transform[2][0], transform[2][1], transform[2][2], transform[2][3],
			transform[3][0], transform[3][1], transform[3][2], transform[3][3]
		});
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