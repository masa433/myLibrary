#include "stage.h"
#include "Graphics.h"

// ワールド変換済み頂点でPxTriangleMeshを作り、剛体を生成する
static void CreateStaticMeshActor(
	physx::PxPhysics* pxPhysics, physx::PxScene* pxScene,
	const ModelResource::Mesh& mesh, const DirectX::XMMATRIX& NodeTransform,
	physx::PxMaterial* material, const char* name,
	std::vector<physx::PxActor*>& actors,
	std::vector<physx::PxTriangleMesh*>& triangle_meshes)
{
	using namespace DirectX;

	//頂点をワールド変換する
	std::vector<physx::PxVec3> transformedVertices;
	transformedVertices.reserve(mesh.vertices.size());// 変換後の頂点を格納するベクターを確保
	for(const auto& vertex : mesh.vertices)
	{
		XMVECTOR pos = XMVectorSet(vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);
		pos = XMVector3TransformCoord(pos, NodeTransform);
		XMFLOAT3 transformedPos;
		XMStoreFloat3(&transformedPos, pos);
		transformedVertices.emplace_back(transformedPos.x, transformedPos.y, transformedPos.z);
	}

	// 鏡映(負の行列式)があれば三角形の巻き順を反転して法線の向きを正しく戻す
	float detValue = XMVectorGetX(XMMatrixDeterminant(NodeTransform));
	std::vector<UINT> fixedIndices(mesh.indices.begin(), mesh.indices.end());
	if (detValue < 0.0f)
	{
		for (size_t i = 0; i + 2 < fixedIndices.size(); i += 3)
		{
			std::swap(fixedIndices[i + 1], fixedIndices[i + 2]);
		}
	}

	physx::PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = static_cast<physx::PxU32>(transformedVertices.size());
	meshDesc.points.data = transformedVertices.data();
	meshDesc.points.stride = sizeof(physx::PxVec3);
	meshDesc.triangles.count = static_cast<physx::PxU32>(fixedIndices.size() / 3);
	meshDesc.triangles.data = fixedIndices.data();
	meshDesc.triangles.stride = sizeof(UINT) * 3;

	physx::PxTolerancesScale pxTolerances;
	const physx::PxCookingParams cookingParams(pxTolerances);
	physx::PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc);
	_ASSERT_EXPR(pxTriangleMesh != nullptr, "Failed to cook triangle mesh");

	physx::PxTransform pxTransform(physx::PxIdentity); // 変換は頂点に焼き込み済み
	physx::PxRigidStatic* pxRigidBody = pxPhysics->createRigidStatic(pxTransform);
	_ASSERT_EXPR(pxRigidBody != nullptr, "Failed to create rigid body");

	physx::PxTriangleMeshGeometry pxMeshGeometry(pxTriangleMesh); // スケールは等倍
	physx::PxRigidActorExt::createExclusiveShape(*pxRigidBody, pxMeshGeometry, *material);
	pxRigidBody->setName(name);

	pxScene->addActor(*pxRigidBody);
	actors.emplace_back(pxRigidBody);
	triangle_meshes.emplace_back(pxTriangleMesh);
}

// 初期化
void stage::initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	stand = std::make_unique<Model>(".\\resources\\field\\stand.mdl");
	ground = std::make_unique<Model>(".\\resources\\field\\ground.mdl");
	stand2 = std::make_unique<gltf_model>(device, ".\\resources\\field\\stand.glb");
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
	standAngle = { 0.0f, 0.0f, 0.0f };

	groundPosition = { 0.0f, 0.0f, 0.0f };
	groundScale = { 1.0f, 1.0f, 1.0f };
	groundAngle = { 0.0f, 0.0f, 0.0f };

	polePosition = { 0.0f, 0.0f, 0.0f };
	poleScale = { 1.0f, 1.0f, 1.0f };
	poleAngle = { 0.0f, 0.0f, 0.0f };

	homerunLineEditor.triggerName = "HomeRunTrigger";
	homerunLineEditor.raycastTargetNames = { "Stand" };
	homerunLineEditor.thickness = 0.5f;
	homerunLineEditor.extraHeight = 40.0f;

	foulLineEditor.triggerName = "FoulTrigger";
	foulLineEditor.raycastTargetNames = { "Stand", "Ground" };  //ファウルラインはスタンドとグラウンドの両方に当たる可能性があるため、両方を指定
	foulLineEditor.thickness = 0.5f;
	foulLineEditor.extraHeight = 3.0f;

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

			const Model::Node& node = stand->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(standScale.x, standScale.y, standScale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(standAngle.x, standAngle.y, standAngle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * StandTransform;

			CreateStaticMeshActor(pxPhysics, pxScene, mesh, NodeTransform, standMaterial, "Stand", actors, triangle_meshes);
		}

		// Ground モデルのメッシュを処理
		const ModelResource* groundResources = ground->GetResource();
		for (const ModelResource::Mesh& mesh : groundResources->GetMeshes())
		{
			
			const Model::Node& node = ground->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(groundScale.x, groundScale.y, groundScale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(groundAngle.x, groundAngle.y, groundAngle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * GroundTransform;
			
			CreateStaticMeshActor(pxPhysics, pxScene, mesh, NodeTransform, groundMaterial, "Ground", actors, triangle_meshes);
		}

		// Pole モデルのメッシュを処理
		const ModelResource* poleResources = pole->GetResource();
		for (const ModelResource::Mesh& mesh : poleResources->GetMeshes())
		{
			const Model::Node& node = pole->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(poleScale.x, poleScale.y, poleScale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(poleAngle.x, poleAngle.y, poleAngle.z);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform) * S * R * PoleTransform;
			CreateStaticMeshActor(pxPhysics, pxScene, mesh, NodeTransform, poleMaterial, "Pole", actors, triangle_meshes);
		}

	}

	
}

void stage::UpdateLineEditor(LineTriggerEditor& editor,
	const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
	float viewportX, float viewportY, float viewportWidth, float viewportHeight)
{
	if (!editor.editMode)return;
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
		if (actor && actor->getName())
		{
			std::string actorName(actor->getName());
			for (const auto& targetName : editor.raycastTargetNames)
			{
				if (actorName == targetName)
				{
					physx::PxVec3 p = hitBuffer.block.position;
					editor.linePoints.push_back({ p.x, p.y, p.z });
					break;
				}
			}
		}
	}
}

void stage::RebuildLineTriggers(LineTriggerEditor& editor)
{
	physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
	physx::PxScene* pxScene = Physics::Instance().GetScene();

	// 既存のフェンスラインのトリガーコライダーを削除
	for (auto* actor : editor.triggers)
	{
		pxScene->removeActor(*actor);
		actor->release();
	}
	editor.triggers.clear();

	if(editor.linePoints.size() < 2)
	{
		return; // フェンスラインの頂点が2つ未満の場合は何もしない
	}

	physx::PxMaterial* triggerMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.5f);
	

	for(size_t i = 0; i < editor.linePoints.size() - 1; ++i)
	{
		const auto& p1 = editor.linePoints[i];
		const auto& p2 = editor.linePoints[i + 1];
		
		// フェンスラインの中点を計算
		float dx = p2.x - p1.x;
		float dz = p2.z - p1.z;
		float length = std::sqrt(dx * dx + dz * dz);
		if (length < 1e-3f) continue; // 長さがほぼゼロの場合はスキップ

		float midX = (p1.x + p2.x) * 0.5f;
		float midZ = (p1.z + p2.z) * 0.5f;
		float baseY = (std::min)(p1.y, p2.y); // フェンスラインの下端のY座標
		float height = editor.extraHeight;// フェンスラインの上端からさらに上へ伸ばす高さ
		float midY = baseY + height * 0.5f; // フェンスラインの中点のY座標

		float angle = std::atan2(-dz, dx);// フェンスラインの角度を計算

		physx::PxTransform triggerTransform(
			physx::PxVec3(midX, midY, midZ),
			physx::PxQuat(angle, physx::PxVec3(0, 1, 0)) // Y軸回転
		);

		physx::PxBoxGeometry geometry(length * 0.5f, height * 0.5f, editor.thickness * 0.5f);

		physx::PxRigidStatic* actor = pxPhysics->createRigidStatic(triggerTransform);
		physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, geometry, *triggerMaterial);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);

		actor->setName(editor.triggerName.c_str());

		pxScene->addActor(*actor);
		editor.triggers.push_back(actor);
	}
}

void stage::DrawLineOverlay(const LineTriggerEditor& editor,
	const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj,
	float viewportX, float viewportY, float viewportWidth, float viewportHeight,
	ImU32 lineColor, ImU32 pointColor)
{
	if (editor.linePoints.empty()) return;

	DirectX::XMMATRIX View = DirectX::XMLoadFloat4x4(&view);
	DirectX::XMMATRIX Proj = DirectX::XMLoadFloat4x4(&proj);
	DirectX::XMMATRIX VP = View * Proj;

	auto worldToScreen = [&](const DirectX::XMFLOAT3& worldPos, ImVec2& outScreen)->bool
		{
			DirectX::XMFLOAT4 clipCheck;
			DirectX::XMStoreFloat4(&clipCheck, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&worldPos), VP));
			if (clipCheck.w <= 0.0f) return false;

			DirectX::XMVECTOR clip = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&worldPos), VP);
			DirectX::XMFLOAT3 c;
			DirectX::XMStoreFloat3(&c, clip);

			outScreen.x = viewportX + (c.x * 0.5f + 0.5f) * viewportWidth;
			outScreen.y = viewportY + (1.0f - (c.y * 0.5f + 0.5f)) * viewportHeight;
			return true;
		};

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImVec2 prevScreen;
	bool hasPrev = false;

	for (const auto& p : editor.linePoints)
	{
		ImVec2 screenPos;
		if (!worldToScreen(p, screenPos)) { hasPrev = false; continue; }

		if (hasPrev) drawList->AddLine(prevScreen, screenPos, lineColor, 2.0f);
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
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(standScale.x, standScale.y, standScale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(standAngle.x, standAngle.y, standAngle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(standPosition.x, standPosition.y, standPosition.z);
	DirectX::XMMATRIX world = S * R * T;
	DirectX::XMStoreFloat4x4(&standTransform, world);

	//グラウンド用
	S = DirectX::XMMatrixScaling(groundScale.x, groundScale.y, groundScale.z);
	R = DirectX::XMMatrixRotationRollPitchYaw(groundAngle.x, groundAngle.y, groundAngle.z);
	T = DirectX::XMMatrixTranslation(groundPosition.x, groundPosition.y, groundPosition.z);
	world = S * R * T;
	DirectX::XMStoreFloat4x4(&groundTransform, world);

	//ポール用
	S = DirectX::XMMatrixScaling(poleScale.x, poleScale.y, poleScale.z);
	R = DirectX::XMMatrixRotationRollPitchYaw(poleAngle.x, poleAngle.y, poleAngle.z);
	T = DirectX::XMMatrixTranslation(polePosition.x, polePosition.y, polePosition.z);
	world = S * R * T;
	DirectX::XMStoreFloat4x4(&poleTransform, world);

	//ボックスの位置とサイズを更新

	UpdateTransform();

	
}

void stage::render(const RenderContext& rc, ModelRenderer* renderer, FrustumCulling* frustumCulling)
{
	//renderer->Render(rc, transform, stand.get(), ShaderId::ShadowMap);
	//renderer->Render(rc, transform, ground.get(), ShaderId::ShadowMap);

	if (stand2)
	{
		bool isVisibled = true;//フラスタムカリングの判定
		if (frustumCulling)
		{
			const auto& sphere = stand2->GetBoundingSphere();//バウンディングスフィアを取得
			//フラスタムカリングの判定
			isVisibled = frustumCulling->IsTransformedSphereVisible(sphere.center, sphere.radius, standTransform);
		}
		if(isVisibled)
		{
			//フラスタムカリングに入っている場合のみ描画する
			stand2->render_batched(rc.deviceContext, standTransform, {});
		}
	}

	if(ground2)
	{
		bool isVisibled = true;//フラスタムカリングの判定
		if (frustumCulling)
		{
			const auto& sphere = ground2->GetBoundingSphere();//バウンディングスフィアを取得
			//フラスタムカリングの判定
			isVisibled = frustumCulling->IsTransformedSphereVisible(sphere.center, sphere.radius, groundTransform);
		}
		if(isVisibled)
		{
			//フラスタムカリングに入っている場合のみ描画する
			ground2->render_batched(rc.deviceContext, groundTransform, {});
		}
	}

	if(pole2)
	{
		bool isVisibled = true;//フラスタムカリングの判定
		if (frustumCulling)
		{
			const auto& sphere = pole2->GetBoundingSphere();//バウンディングスフィアを取得
			//フラスタムカリングの判定
			isVisibled = frustumCulling->IsTransformedSphereVisible(sphere.center, sphere.radius, poleTransform);
		}
		if(isVisibled)
		{
			//フラスタムカリングに入っている場合のみ描画する
			pole2->render_batched(rc.deviceContext, poleTransform, {});
		}
	}

	// ライトタワーをスポットライトの位置に4箇所配置
	for (int i = 0; i < TOWER_COUNT; i++)
	{
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(lightScale[i].x, lightScale[i].y, lightScale[i].z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(towerAngle[i].x, towerAngle[i].y, towerAngle[i].z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			towerPositions[i].x, towerPositions[i].y, towerPositions[i].z);
		DirectX::XMFLOAT4X4 towerTransform;
		DirectX::XMStoreFloat4x4(&towerTransform, S * R * T);

		bool isVisible = true;
		if (frustumCulling)
		{
			const auto& sphere = lightTower2->GetBoundingSphere();
			isVisible = frustumCulling->IsTransformedSphereVisible(sphere.center, sphere.radius, towerTransform);
		}
		if (isVisible)
		{
			lightTower2->render_batched(rc.deviceContext, towerTransform, {});
		}
	}

	/*for (Flag& f : flags)
	{
		f.Render(rc, renderer);
	}*/

}

// 終了
void stage::uninitialize()
{
	for (physx::PxTriangleMesh* pxTriangleMesh : triangle_meshes)
	{
		pxTriangleMesh->release();
	}
	triangle_meshes.clear();

	if (actors.size() > 0)
	{
		physx::PxScene* pxScene = Physics::Instance().GetScene();
		pxScene->removeActors(actors.data(), static_cast<physx::PxU32>(actors.size()));
	}
	for(physx::PxActor* actor : actors)
	{
		actor->release();
	}
	actors.clear();

	stand2.reset();
	ground2.reset();
	pole2.reset();
	lightTower2.reset();
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

	if (ImGui::CollapsingHeader("Fence Line Editor"))
	{
		if (ImGui::Checkbox(u8"Edit Mode (Homerun)", &homerunLineEditor.editMode))
		{
			if (homerunLineEditor.editMode) foulLineEditor.editMode = false;
		}
		ImGui::Text("Points: %d", (int)homerunLineEditor.linePoints.size());

		if (ImGui::Button("Undo Last Point") && !homerunLineEditor.linePoints.empty())
			homerunLineEditor.linePoints.pop_back();
		ImGui::SameLine();
		if (ImGui::Button("Clear All"))
			homerunLineEditor.linePoints.clear();
		ImGui::SameLine();
		if (ImGui::Button("Rebuild Triggers"))
		{
			RebuildLineTriggers(homerunLineEditor);
			RebuildLineTriggers(foulLineEditor);
		}

		ImGui::DragFloat("Extra Height", &homerunLineEditor.extraHeight, 0.5f);
		ImGui::DragFloat("Thickness", &homerunLineEditor.thickness, 0.1f);

		for (size_t i = 0; i < homerunLineEditor.linePoints.size(); ++i)
		{
			ImGui::Text("[%d] (%.2f, %.2f, %.2f)", (int)i,
				homerunLineEditor.linePoints[i].x, homerunLineEditor.linePoints[i].y, homerunLineEditor.linePoints[i].z);
		}

		ImGui::Separator();
		ImGui::Text("Foul Line");

		if (ImGui::Checkbox(u8"Edit Mode (Foul)", &foulLineEditor.editMode))
		{
			if (foulLineEditor.editMode) homerunLineEditor.editMode = false;
		}
		ImGui::Text("Points: %d", (int)foulLineEditor.linePoints.size());
		if (ImGui::Button("Undo Last Point (Foul)") && !foulLineEditor.linePoints.empty())
			foulLineEditor.linePoints.pop_back();
		if (ImGui::Button("Clear All (Foul)"))
			foulLineEditor.linePoints.clear();
		ImGui::DragFloat("Extra Height (Foul)", &foulLineEditor.extraHeight, 0.5f);
		ImGui::DragFloat("Thickness (Foul)", &foulLineEditor.thickness, 0.1f);
	}

#endif //  USE_IMGUI
}

void stage::SaveToJson(json& j)
{
	/*j["standPosition"] = { standPosition.x, standPosition.y, standPosition.z };
	j["standScale"] = { standScale.x, standScale.y, standScale.z };
	j["standAngle"] = { standAngle.x, standAngle.y, standAngle.z };
	j["groundPosition"] = { groundPosition.x, groundPosition.y, groundPosition.z };
	j["groundScale"] = { groundScale.x, groundScale.y, groundScale.z };
	j["groundAngle"] = { groundAngle.x, groundAngle.y, groundAngle.z };
	j["polePosition"] = { polePosition.x, polePosition.y, polePosition.z };
	j["poleScale"] = { poleScale.x, poleScale.y, poleScale.z };
	j["poleAngle"] = { poleAngle.x, poleAngle.y, poleAngle.z };*/
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
	for (const auto& point : homerunLineEditor.linePoints)
	{
		j["fenceLinePoints"].push_back({ point.x, point.y, point.z });
	}
	j["fenceExtraHeight"] = homerunLineEditor.extraHeight;
	j["fenceThickness"] = homerunLineEditor.thickness;

	// ファウルライン用も追加保存
	j["foulLinePoints"] = json::array();
	for (const auto& point : foulLineEditor.linePoints)
	{
		j["foulLinePoints"].push_back({ point.x, point.y, point.z });
	}
	j["foulExtraHeight"] = foulLineEditor.extraHeight;
	j["foulThickness"] = foulLineEditor.thickness;
	
}

void stage::LoadFromJson(const json& j)
{
	/*standPosition = { j["standPosition"][0], j["standPosition"][1], j["standPosition"][2] };
	standScale = { j["standScale"][0], j["standScale"][1], j["standScale"][2] };
	standAngle = { j["standAngle"][0], j["standAngle"][1], j["standAngle"][2] };
	groundPosition = { j["groundPosition"][0], j["groundPosition"][1], j["groundPosition"][2] };
	groundScale = { j["groundScale"][0], j["groundScale"][1], j["groundScale"][2] };
	groundAngle = { j["groundAngle"][0], j["groundAngle"][1], j["groundAngle"][2] };*/
	/*polePosition = { j["polePosition"][0], j["polePosition"][1], j["polePosition"][2] };
	poleScale = { j["poleScale"][0], j["poleScale"][1], j["poleScale"][2] };
	poleAngle = { j["poleAngle"][0], j["poleAngle"][1], j["poleAngle"][2] };*/

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
	homerunLineEditor.linePoints.clear();
	if (j.contains("fenceLinePoints") && j["fenceLinePoints"].is_array())
	{
		for (const auto& point : j["fenceLinePoints"])
		{
			homerunLineEditor.linePoints.push_back({ point[0], point[1], point[2] });
		}
	}
	if (j.contains("fenceExtraHeight"))
	{
		homerunLineEditor.extraHeight = j["fenceExtraHeight"];
	}
	if (j.contains("fenceThickness"))
	{
		homerunLineEditor.thickness = j["fenceThickness"];
	}

	// ファウルライン用の読み込み
	foulLineEditor.linePoints.clear();
	if (j.contains("foulLinePoints") && j["foulLinePoints"].is_array())
	{
		for (const auto& point : j["foulLinePoints"])
		{
			foulLineEditor.linePoints.push_back({ point[0], point[1], point[2] });
		}
	}
	if (j.contains("foulExtraHeight"))
	{
		foulLineEditor.extraHeight = j["foulExtraHeight"];
	}
	if (j.contains("foulThickness"))
	{
		foulLineEditor.thickness = j["foulThickness"];
	}

	RebuildLineTriggers(homerunLineEditor);
	RebuildLineTriggers(foulLineEditor);

}