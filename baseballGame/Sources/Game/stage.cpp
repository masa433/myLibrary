#include "stage.h"
#include "imgui.h"
#include "Graphics.h"

// 初期化
void stage::initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	stand = std::make_unique<Model>(".\\resources\\field\\field.mdl");
	ground = std::make_unique<Model>(".\\resources\\field\\ground.mdl");
	stand2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\field.glb");
	ground2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\ground.glb");
	pole = std::make_unique<Model>(".\\resources\\field\\pole.mdl");
	pole2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\pole.glb");
	lightTower = std::make_unique<Model>(".\\resources\\field\\lightTower.mdl");
	lightTower2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\lightTower.glb");

	stand2->build_static_batches(device);
	ground2->build_static_batches(device);
	pole2->build_static_batches(device);
	lightTower2->build_static_batches(device);

	// 位置、スケール、回転の初期化
	standPosition = { 0.0f, 0.0f, 0.0f };
	standScale = { 1.0f, 1.0f, 1.0f };
	standAngle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f };

	groundPosition = { 0.0f, 0.0f, 0.0f };
	groundScale = { 1.0f, 1.0f, 1.0f };
	groundAngle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f };

	polePosition = { 0.0f, 0.0f, 0.0f };
	poleScale = { 1.0f, 1.0f, 1.0f };
	poleAngle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f };

	hrTriggerPos = { 0.0f, 55.0f, 67.5f }; // トリガーの初期位置
	hrTriggerHalfExtents = { 67.0f, 55.0f, 0.5f }; // トリガーの半分のサイズ(XYZ)

	//静的剛体の作成
	{
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxScene* pxScene = Physics::Instance().GetScene();

		// Ground用のマテリアル（よく跳ねる）
		physx::PxMaterial* groundMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.3f);

		// Stand用のマテリアル（ほぼ跳ねない）
		physx::PxMaterial* standMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.0f);

		//Pole用のマテリアル（あまり跳ねない）
		physx::PxMaterial* poleMaterial = pxPhysics->createMaterial(1.0f, 1.0f, 0.2f);

		DirectX::XMMATRIX StandTransform = DirectX::XMLoadFloat4x4(&standTransform);
		DirectX::XMMATRIX GroundTransform = DirectX::XMLoadFloat4x4(&groundTransform);
		DirectX::XMMATRIX PoleTransform = DirectX::XMLoadFloat4x4(&poleTransform);

		// Stand モデルのメッシュを処理
		const ModelResource* standResources = stand->GetResource();
		for (const ModelResource::Mesh& mesh : standResources->GetMeshes())
		{
			physx::PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = static_cast<physx::PxU32>(mesh.vertices.size());
			meshDesc.points.data = mesh.vertices.data();
			meshDesc.points.stride = sizeof(ModelResource::Vertex);
			meshDesc.triangles.count = static_cast<physx::PxU32>(mesh.indices.size() / 3);
			meshDesc.triangles.data = mesh.indices.data();
			meshDesc.triangles.stride = sizeof(UINT) * 3;

			physx::PxTolerancesScale pxTolerances;
			const physx::PxCookingParams cookingParams(pxTolerances);
			physx::PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc);

			const Model::Node& node = stand->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(standScale.x, standScale.y, standScale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(standAngle.x, standAngle.y, standAngle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * StandTransform;
			physx::PxVec3 pxScale(
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[0])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[1])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[2]))
			);
			NodeTransform.r[0] = DirectX::XMVector3Normalize(NodeTransform.r[0]);
			NodeTransform.r[1] = DirectX::XMVector3Normalize(NodeTransform.r[1]);
			NodeTransform.r[2] = DirectX::XMVector3Normalize(NodeTransform.r[2]);

			DirectX::XMFLOAT4X4 nodeTransform;
			DirectX::XMStoreFloat4x4(&nodeTransform, NodeTransform);
			physx::PxTransform pxTransform(physx::PxMat44(
				physx::PxVec3(nodeTransform._11, nodeTransform._12, nodeTransform._13),
				physx::PxVec3(nodeTransform._21, nodeTransform._22, nodeTransform._23),
				physx::PxVec3(nodeTransform._31, nodeTransform._32, nodeTransform._33),
				physx::PxVec3(nodeTransform._41, nodeTransform._42, nodeTransform._43)
			));
			physx::PxRigidStatic* pxRigidBody = pxPhysics->createRigidStatic(pxTransform);
			_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create stand rigid body");

			physx::PxMeshScale pxMeshScale(pxScale);
			physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh, pxMeshScale);
			physx::PxShape* pxShape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *standMaterial);

			pxRigidBody->setName("Stand");

			pxScene->addActor(*pxRigidBody);

			actors.emplace_back(pxRigidBody);
			triangle_meshes.emplace_back(pxTriangleMesh);
		}

		// Ground モデルのメッシュを処理
		const ModelResource* groundResources = ground->GetResource();
		for (const ModelResource::Mesh& mesh : groundResources->GetMeshes())
		{
			physx::PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = static_cast<physx::PxU32>(mesh.vertices.size());
			meshDesc.points.data = mesh.vertices.data();
			meshDesc.points.stride = sizeof(ModelResource::Vertex);
			meshDesc.triangles.count = static_cast<physx::PxU32>(mesh.indices.size() / 3);
			meshDesc.triangles.data = mesh.indices.data();
			meshDesc.triangles.stride = sizeof(UINT) * 3;

			physx::PxTolerancesScale pxTolerances;
			const physx::PxCookingParams cookingParams(pxTolerances);
			physx::PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc);

			const Model::Node& node = ground->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(groundScale.x, groundScale.y, groundScale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(groundAngle.x, groundAngle.y, groundAngle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * GroundTransform;
			physx::PxVec3 pxScale(
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[0])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[1])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[2]))
			);
			NodeTransform.r[0] = DirectX::XMVector3Normalize(NodeTransform.r[0]);
			NodeTransform.r[1] = DirectX::XMVector3Normalize(NodeTransform.r[1]);
			NodeTransform.r[2] = DirectX::XMVector3Normalize(NodeTransform.r[2]);

			DirectX::XMFLOAT4X4 nodeTransform;
			DirectX::XMStoreFloat4x4(&nodeTransform, NodeTransform);
			physx::PxTransform pxTransform(physx::PxMat44(
				physx::PxVec3(nodeTransform._11, nodeTransform._12, nodeTransform._13),
				physx::PxVec3(nodeTransform._21, nodeTransform._22, nodeTransform._23),
				physx::PxVec3(nodeTransform._31, nodeTransform._32, nodeTransform._33),
				physx::PxVec3(nodeTransform._41, nodeTransform._42, nodeTransform._43)
			));
			physx::PxRigidStatic* pxRigidBody = pxPhysics->createRigidStatic(pxTransform);
			_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create ground rigid body");

			physx::PxMeshScale pxMeshScale(pxScale);
			physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh, pxMeshScale);
			physx::PxShape* pxShape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *groundMaterial);

			pxRigidBody->setName("Ground");

			pxScene->addActor(*pxRigidBody);

			actors.emplace_back(pxRigidBody);
			triangle_meshes.emplace_back(pxTriangleMesh);
		}

		// Pole モデルのメッシュを処理
		const ModelResource* poleResources = pole->GetResource();
		for (const ModelResource::Mesh& mesh : poleResources->GetMeshes())
		{
			physx::PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = static_cast<physx::PxU32>(mesh.vertices.size());
			meshDesc.points.data = mesh.vertices.data();
			meshDesc.points.stride = sizeof(ModelResource::Vertex);
			meshDesc.triangles.count = static_cast<physx::PxU32>(mesh.indices.size() / 3);
			meshDesc.triangles.data = mesh.indices.data();
			meshDesc.triangles.stride = sizeof(UINT) * 3;

			physx::PxTolerancesScale pxTolerances;
			const physx::PxCookingParams cookingParams(pxTolerances);
			physx::PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc);

			const Model::Node& node = pole->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(poleScale.x, poleScale.y, poleScale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(poleAngle.x, poleAngle.y, poleAngle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * PoleTransform;
			physx::PxVec3 pxScale(
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[0])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[1])),
				DirectX::XMVectorGetX(DirectX::XMVector3Length(NodeTransform.r[2]))
			);
			NodeTransform.r[0] = DirectX::XMVector3Normalize(NodeTransform.r[0]);
			NodeTransform.r[1] = DirectX::XMVector3Normalize(NodeTransform.r[1]);
			NodeTransform.r[2] = DirectX::XMVector3Normalize(NodeTransform.r[2]);

			DirectX::XMFLOAT4X4 nodeTransform;
			DirectX::XMStoreFloat4x4(&nodeTransform, NodeTransform);
			physx::PxTransform pxTransform(physx::PxMat44(
				physx::PxVec3(nodeTransform._11, nodeTransform._12, nodeTransform._13),
				physx::PxVec3(nodeTransform._21, nodeTransform._22, nodeTransform._23),
				physx::PxVec3(nodeTransform._31, nodeTransform._32, nodeTransform._33),
				physx::PxVec3(nodeTransform._41, nodeTransform._42, nodeTransform._43)
			));
			physx::PxRigidStatic* pxRigidBody = pxPhysics->createRigidStatic(pxTransform);
			_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create pole rigid body");

			physx::PxMeshScale pxMeshScale(pxScale);
			physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh, pxMeshScale);
			physx::PxShape* pxShape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *poleMaterial);
			pxRigidBody->setName("Pole");

			pxScene->addActor(*pxRigidBody);

			actors.emplace_back(pxRigidBody);
			triangle_meshes.emplace_back(pxTriangleMesh);
		}

		// ホームラン判定用トリガーの作成
		{
			//physx::PxMaterial* triggerMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.5f);
			//physx::PxTransform triggerTransform(physx::PxVec3(hrTriggerPos.x, hrTriggerPos.y, hrTriggerPos.z));
			//homeRunTrigger = pxPhysics->createRigidStatic(triggerTransform);

			//physx::PxBoxGeometry triggerGeometry(physx::PxVec3(hrTriggerHalfExtents.x, hrTriggerHalfExtents.y, hrTriggerHalfExtents.z));
			//physx::PxShape* triggerShape = physx::PxRigidActorExt::createExclusiveShape(*homeRunTrigger, triggerGeometry, *triggerMaterial);

			//// 物理的な衝突を無効にし、トリガー（重なり判定）として設定する
			//triggerShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
			//triggerShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);

			//homeRunTrigger->setName("HomeRunTrigger");
			//pxScene->addActor(*homeRunTrigger);
		}
	}

	// 旗の初期化
	{
		//旗を等間隔で並べる
		static const Flag::FlagColor flagColors[FLAG_COUNT] =
		{
			Flag::FlagColor::Blue,
			Flag::FlagColor::Green,
			Flag::FlagColor::Japan,
			Flag::FlagColor::Red,
			Flag::FlagColor::Yellow,
		};
		const DirectX::XMFLOAT3 flagBasePosition = { 0.0f, 70.0f, 140.0f };
		const float flagSpacing = 15.0f;

		flags.resize(FLAG_COUNT);
		for (int i = 0; i < FLAG_COUNT; ++i)
		{
			//iが2の時だけpositionを70にして、他の旗はpositionを60にする
			DirectX::XMFLOAT3 flagPosition = flagBasePosition;
			if (i != 2)
			{
				flagPosition.y = 60.0f;
			}

			flags[i].Initialize(i, flagColors[i], flagSpacing, flagPosition);
		}
	}

}

void stage::UpdateFenceEditor(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
	float viewportX, float viewportY, float viewportWidth, float viewportHeight)
{
	if (!fenceEditMode)return;
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;// 左クリックが押されていない場合は何もしない

	ImVec2 mousePos = ImGui::GetMousePos();

	//GameViewウィンドウ内かどうか判定
	float localX = mousePos.x - viewportX;
	float localY = mousePos.y - viewportY;
	if(localX < 0 || localX > viewportWidth || localY < 0 || localY > viewportHeight)
	{
		return; // GameViewウィンドウ外なら何もしない
	}

	//スクリーン座標を正規化デバイス座標に変換
	float ndcX = (localX / viewportWidth) * 2.0f - 1.0f;
	float ndcY = 1.0f - (localY / viewportHeight) * 2.0f; // Y軸反転

	DirectX::XMMATRIX View = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX Proj = DirectX::XMLoadFloat4x4(&proj);
	DirectX::XMMATRIX invVP = DirectX::XMMatrixInverse(nullptr, View * Proj);

	DirectX::XMVECTOR nearP = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invVP);
	DirectX::XMVECTOR farP = DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invVP);

	DirectX::XMVECTOR dirV = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(farP, nearP));

	DirectX::XMFLOAT3 rayOrigin, rayDir;
	DirectX::XMStoreFloat3(&rayOrigin, nearP);
	DirectX::XMStoreFloat3(&rayDir, dirV);

	physx::PxScene* pxScene = Physics::Instance().GetScene();
	physx::PxRaycastBuffer hitBuffer;
	physx::PxQueryFilterData filterData;
	filterData.flags |= physx::PxQueryFlag::eSTATIC; // 静的オブジェクトのみを対象

	bool hit = pxScene->raycast(
		physx::PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z),// レイの原点
		physx::PxVec3(rayDir.x, rayDir.y, rayDir.z),// レイの方向
		1000.0f, // 最大距離
		hitBuffer,// ヒット情報を格納するバッファ
		physx::PxHitFlag::eDEFAULT,// ヒット情報の取得フラグ
		filterData// フィルタリング情報
	);

	// ヒットした場合、ヒットした位置をフェンスラインの頂点として追加
	if (hit && hitBuffer.hasBlock)
	{
		physx::PxRigidActor* actor = hitBuffer.block.actor;
		if (actor && actor->getName() && std::string(actor->getName()) == "Stand")
		{
			physx::PxVec3 p = hitBuffer.block.position;
			fenceLinePoints.push_back({ p.x, p.y, p.z });
		}
	}
}

void stage::RebuildFenceTriggers()
{
	physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
	physx::PxScene* pxScene = Physics::Instance().GetScene();

	// 既存のフェンスラインのトリガーコライダーを削除
	for (auto* actor : fenceTriggers)
	{
		pxScene->removeActor(*actor);
		actor->release();
	}
	fenceTriggers.clear();

	if(fenceLinePoints.size() < 2)
	{
		return; // フェンスラインの頂点が2つ未満の場合は何もしない
	}

	physx::PxMaterial* triggerMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.5f);
	

	for(size_t i = 0; i < fenceLinePoints.size() - 1; ++i)
	{
		const auto& p1 = fenceLinePoints[i];
		const auto& p2 = fenceLinePoints[i + 1];
		
		// フェンスラインの中点を計算
		float dx = p2.x - p1.x;
		float dz = p2.z - p1.z;
		float length = std::sqrt(dx * dx + dz * dz);
		if (length < 1e-3f) continue; // 長さがほぼゼロの場合はスキップ

		float midX = (p1.x + p2.x) * 0.5f;
		float midZ = (p1.z + p2.z) * 0.5f;
		float baseY = (std::min)(p1.y, p2.y); // フェンスラインの下端のY座標
		float height = fenceExtraHeight;// フェンスラインの上端からさらに上へ伸ばす高さ
		float midY = baseY + height * 0.5f; // フェンスラインの中点のY座標

		float angle = std::atan2(-dz, dx);// フェンスラインの角度を計算

		physx::PxTransform triggerTransform(
			physx::PxVec3(midX, midY, midZ),
			physx::PxQuat(angle, physx::PxVec3(0, 1, 0)) // Y軸回転
		);

		physx::PxBoxGeometry geometry(length * 0.5f, height * 0.5f, fenceThickness * 0.5f);

		physx::PxRigidStatic* actor = pxPhysics->createRigidStatic(triggerTransform);
		physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, geometry, *triggerMaterial);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);

		actor->setName("HomeRunTrigger");

		pxScene->addActor(*actor);
		fenceTriggers.push_back(actor);
	}
}

void stage::DrawFenceOverlay(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
	float viewportX, float viewportY, float viewportWidth, float viewportHeight)
{
	if (fenceLinePoints.empty()) return;

	DirectX::XMMATRIX View = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX Proj = DirectX::XMLoadFloat4x4(&proj);
	DirectX::XMMATRIX VP = View * Proj;

	auto worldToScreen = [&](const DirectX::XMFLOAT3& worldPos, ImVec2& outScreen)->bool
	{
		DirectX::XMVECTOR clip = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&worldPos), VP);

		//画面の裏側にある場合は描画しない
		DirectX::XMFLOAT4 clipCheck;
		DirectX::XMStoreFloat4(&clipCheck, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&worldPos), VP));
		if (clipCheck.w <= 0.0f) return false;

		// NDC座標に変換
		float ndcX, ndcY;
		DirectX::XMFLOAT3 c;
		DirectX::XMStoreFloat3(&c, clip);
		ndcX = c.x;
		ndcY = c.y;

		// スクリーン座標に変換
		outScreen.x = viewportX + (ndcX * 0.5f + 0.5f) * viewportWidth;
		outScreen.y = viewportY + (1.0f - (ndcY * 0.5f + 0.5f)) * viewportHeight;

		return true;
	};

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 lineColor = IM_COL32(255, 60, 60, 255);
	const ImU32 pointColor = IM_COL32(255, 255, 0, 255);

	ImVec2 prevScreen;
	bool hasPrev = false;

	for (const auto& p : fenceLinePoints)
	{
		ImVec2 screenPos;
		if (!worldToScreen(p, screenPos))
		{
			hasPrev = false;
			continue;
		}

		if (hasPrev)
		{
			drawList->AddLine(prevScreen, screenPos, lineColor, 2.0f);
		}
		drawList->AddCircleFilled(screenPos, 4.0f, pointColor);

		prevScreen = screenPos;
		hasPrev = true;
	}

}

// 更新
void stage::update(float elapsedTime)
{

	//スタンド用
	//スタンドだけ右手系で描画する
	DirectX::XMMATRIX CStand = { -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(standScale.x, standScale.y, standScale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(standAngle.x, standAngle.y, standAngle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(standPosition.x, standPosition.y, standPosition.z);
	DirectX::XMMATRIX world = CStand * S * R * T;
	DirectX::XMStoreFloat4x4(&standTransform, world);

	//グラウンド用
	S = DirectX::XMMatrixScaling(groundScale.x, groundScale.y, groundScale.z);
	R = DirectX::XMMatrixRotationRollPitchYaw(groundAngle.x, groundAngle.y, groundAngle.z);
	T = DirectX::XMMatrixTranslation(groundPosition.x, groundPosition.y, groundPosition.z);
	world = S * R * T;
	DirectX::XMStoreFloat4x4(&groundTransform, world);

	//ポール用
	DirectX::XMMATRIX CPole = { -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	S = DirectX::XMMatrixScaling(poleScale.x, poleScale.y, poleScale.z);
	R = DirectX::XMMatrixRotationRollPitchYaw(poleAngle.x, poleAngle.y, poleAngle.z);
	T = DirectX::XMMatrixTranslation(polePosition.x, polePosition.y, polePosition.z);
	world = CPole * S * R * T;
	DirectX::XMStoreFloat4x4(&poleTransform, world);

	//ボックスの位置とサイズを更新

	UpdateTransform();

	for (Flag& f : flags)
	{
		f.Update(elapsedTime);
	}
}

void stage::render(const RenderContext& rc, ModelRenderer* renderer)
{
	//renderer->Render(rc, transform, stand.get(), ShaderId::ShadowMap);
	//renderer->Render(rc, transform, ground.get(), ShaderId::ShadowMap);

	stand2->render_batched(rc.deviceContext, standTransform, {});
	ground2->render_batched(rc.deviceContext, groundTransform, {});
	pole2->render_batched(rc.deviceContext, poleTransform, {});
	// ライトタワーをスポットライトの位置に4箇所配置
	for (int i = 0; i < TOWER_COUNT; i++)
	{
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(lightScale[i].x, lightScale[i].y, lightScale[i].z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(towerAngle[i].x, towerAngle[i].y, towerAngle[i].z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			towerPositions[i].x, towerPositions[i].y, towerPositions[i].z);
		DirectX::XMFLOAT4X4 towerTransform;
		DirectX::XMStoreFloat4x4(&towerTransform, S * R * T);

		lightTower2->render_batched(rc.deviceContext, towerTransform, {});
	}

	/*for (Flag& f : flags)
	{
		f.Render(rc, renderer);
	}*/

}

// 終了
void stage::uninitialize()
{
	for(physx::PxTriangleMesh* pxTriangleMesh : triangle_meshes)
	{
		pxTriangleMesh->release();
	}
	triangle_meshes.clear();

	physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
	physx::PxScene* pxScene = Physics::Instance().GetScene();

	if (actors.size() > 0) 
	{
		pxScene->removeActors(actors.data(), static_cast<physx::PxU32>(actors.size()));
	}

	actors.clear();

	/*for (auto* boxCollider : boxColliders)
	{
		boxCollider->release();
	}
	boxColliders.clear();*/

	stand2.reset();
	ground2.reset();
	pole2.reset();
	lightTower2.reset();

	for (Flag& f : flags)
	{
		f.UnInitialize();
	}
}

void stage::DrawGUI()
{
#ifdef  USE_IMGUI
	if (ImGui::CollapsingHeader("Stage Info"))
	{
		// スタンドの位置、スケール、回転を表示
		ImGui::DragFloat3("Stand Position", &standPosition.x, 0.5f);
		ImGui::DragFloat3("Stand Scale", &standScale.x, 0.1f);
		ImGui::DragFloat3("Stand Angle", &standAngle.x, 0.01f);

		ImGui::Separator();

		// グラウンドの位置、スケール、回転を表示
		ImGui::DragFloat3("Ground Position", &groundPosition.x, 0.5f);
		ImGui::DragFloat3("Ground Scale", &groundScale.x, 0.1f);
		ImGui::DragFloat3("Ground Angle", &groundAngle.x, 0.01f);

		ImGui::Separator();

		// ポールの位置、スケール、回転を表示
		ImGui::DragFloat3("Pole Position", &polePosition.x, 0.5f);
		ImGui::DragFloat3("Pole Scale", &poleScale.x, 0.1f);
		ImGui::DragFloat3("Pole Angle", &poleAngle.x, 0.01f);
	}

	if (ImGui::CollapsingHeader("Home Run Trigger"))
	{
		ImGui::DragFloat3("Trigger Position", &hrTriggerPos.x, 0.5f);
		ImGui::DragFloat3("Trigger Half Extents (Size)", &hrTriggerHalfExtents.x, 0.5f);

		if (homeRunTrigger)
		{
			// 位置の更新
			physx::PxTransform transform(physx::PxVec3(hrTriggerPos.x, hrTriggerPos.y, hrTriggerPos.z));
			homeRunTrigger->setGlobalPose(transform);

			// サイズの更新
			physx::PxShape* shape = nullptr;
			homeRunTrigger->getShapes(&shape, 1);
			if (shape)
			{
				shape->setGeometry(physx::PxBoxGeometry(hrTriggerHalfExtents.x, hrTriggerHalfExtents.y, hrTriggerHalfExtents.z));
			}
		}
	}

	if (ImGui::CollapsingHeader("LightTower"))
	{
		for (int i = 0; i < TOWER_COUNT; i++)
		{
			std::string label = "Tower[" + std::to_string(i) + "]";
			ImGui::DragFloat3(label.c_str(), &towerPositions[i].x, 0.5f);
			ImGui::DragFloat3((label + " Angle").c_str(), &towerAngle[i].x, 0.01f);
			ImGui::DragFloat3((label + " Scale").c_str(), &lightScale[i].x, 0.01f);
		}
	}

	for (int i = 0; i < FLAG_COUNT; ++i)
	{
		std::string flagLabel = "Flag " + std::to_string(i);
		if (ImGui::CollapsingHeader(flagLabel.c_str()))
		{
			flags[i].DrawGUI(i);
		}
	}

	if (ImGui::CollapsingHeader("Fence Line Editor"))
	{
		ImGui::Checkbox(u8"Edit Mode (Game View上でフェンスをクリック)", &fenceEditMode);
		ImGui::Text("Points: %d", (int)fenceLinePoints.size());

		if (ImGui::Button("Undo Last Point") && !fenceLinePoints.empty())
			fenceLinePoints.pop_back();
		ImGui::SameLine();
		if (ImGui::Button("Clear All"))
			fenceLinePoints.clear();
		ImGui::SameLine();
		if (ImGui::Button("Rebuild Triggers"))
			RebuildFenceTriggers();

		ImGui::DragFloat("Extra Height", &fenceExtraHeight, 0.5f);
		ImGui::DragFloat("Thickness", &fenceThickness, 0.1f);

		for (size_t i = 0; i < fenceLinePoints.size(); ++i)
		{
			ImGui::Text("[%d] (%.2f, %.2f, %.2f)", (int)i,
				fenceLinePoints[i].x, fenceLinePoints[i].y, fenceLinePoints[i].z);
		}
	}

#endif //  USE_IMGUI
}

void stage::SaveToJson(json& j)
{
	j["standPosition"] = { standPosition.x, standPosition.y, standPosition.z };
	j["standScale"] = { standScale.x, standScale.y, standScale.z };
	j["standAngle"] = { standAngle.x, standAngle.y, standAngle.z };
	j["groundPosition"] = { groundPosition.x, groundPosition.y, groundPosition.z };
	j["groundScale"] = { groundScale.x, groundScale.y, groundScale.z };
	j["groundAngle"] = { groundAngle.x, groundAngle.y, groundAngle.z };
	j["polePosition"] = { polePosition.x, polePosition.y, polePosition.z };
	j["poleScale"] = { poleScale.x, poleScale.y, poleScale.z };
	j["poleAngle"] = { poleAngle.x, poleAngle.y, poleAngle.z };
	// ホームラン判定用トリガーの位置とサイズを保存
	j["hrTriggerPos"] = { hrTriggerPos.x, hrTriggerPos.y, hrTriggerPos.z };
	j["hrTriggerHalfExtents"] = { hrTriggerHalfExtents.x, hrTriggerHalfExtents.y, hrTriggerHalfExtents.z };
	// ライトタワーの位置、角度、スケールを保存
	for (int i = 0; i < TOWER_COUNT; ++i)
	{
		std::string towerKey = "tower" + std::to_string(i);
		j[towerKey]["position"] = { towerPositions[i].x, towerPositions[i].y, towerPositions[i].z };
		j[towerKey]["angle"] = { towerAngle[i].x, towerAngle[i].y, towerAngle[i].z };
		j[towerKey]["scale"] = { lightScale[i].x, lightScale[i].y, lightScale[i].z };
	}

	// フェンスラインの頂点を保存
	j["fenceLinePoints"] = json::array();
	for (const auto& point : fenceLinePoints)
	{
		j["fenceLinePoints"].push_back({ point.x, point.y, point.z });
	}
	// フェンスラインの追加高さと厚さを保存
	j["fenceExtraHeight"] = fenceExtraHeight;
	j["fenceThickness"] = fenceThickness;
	
}

void stage::LoadFromJson(const json& j)
{
	standPosition = { j["standPosition"][0], j["standPosition"][1], j["standPosition"][2] };
	standScale = { j["standScale"][0], j["standScale"][1], j["standScale"][2] };
	standAngle = { j["standAngle"][0], j["standAngle"][1], j["standAngle"][2] };
	groundPosition = { j["groundPosition"][0], j["groundPosition"][1], j["groundPosition"][2] };
	groundScale = { j["groundScale"][0], j["groundScale"][1], j["groundScale"][2] };
	groundAngle = { j["groundAngle"][0], j["groundAngle"][1], j["groundAngle"][2] };
	/*polePosition = { j["polePosition"][0], j["polePosition"][1], j["polePosition"][2] };
	poleScale = { j["poleScale"][0], j["poleScale"][1], j["poleScale"][2] };
	poleAngle = { j["poleAngle"][0], j["poleAngle"][1], j["poleAngle"][2] };*/
	// ホームラン判定用トリガーの位置とサイズを読み込み
	hrTriggerPos = { j["hrTriggerPos"][0], j["hrTriggerPos"][1], j["hrTriggerPos"][2] };
	hrTriggerHalfExtents = { j["hrTriggerHalfExtents"][0], j["hrTriggerHalfExtents"][1], j["hrTriggerHalfExtents"][2] };
	// ライトタワーの位置、角度、スケールを読み込み
	for (int i = 0; i < TOWER_COUNT; ++i)
	{
		std::string towerKey = "tower" + std::to_string(i);
		towerPositions[i] = {
			j[towerKey]["position"][0],
			j[towerKey]["position"][1],
			j[towerKey]["position"][2]
		};
		towerAngle[i] = {
			j[towerKey]["angle"][0],
			j[towerKey]["angle"][1],
			j[towerKey]["angle"][2]
		};
		lightScale[i] = {
			j[towerKey]["scale"][0],
			j[towerKey]["scale"][1],
			j[towerKey]["scale"][2]
		};
	}

	// フェンスラインの頂点を読み込み
	fenceLinePoints.clear();
	if (j.contains("fenceLinePoints") && j["fenceLinePoints"].is_array())
	{
		for (const auto& point : j["fenceLinePoints"])
		{
			fenceLinePoints.push_back({ point[0], point[1], point[2] });
		}
	}

	// フェンスラインの追加高さと厚さを読み込み
	if (j.contains("fenceExtraHeight"))
	{
		fenceExtraHeight = j["fenceExtraHeight"];
	}

	if (j.contains("fenceThickness"))
	{
		fenceThickness = j["fenceThickness"];
	}

	RebuildFenceTriggers(); // フェンスラインのトリガーを再構築
}