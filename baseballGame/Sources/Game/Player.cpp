#include "Player.h"
#include "imgui.h"
#include <Windows.h>
#include "Graphics.h"
#include "Pitcher.h"
#include <algorithm>
#include <stdexcept>
#include "stage.h"
#include "collision.h"
#include "input.h"
#include "physxManager.h"


// 初期化
void Player::Initialize()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    // モデルの読み込み
    animated_model = std::make_unique<gltf_model>(device, ".\\resources\\batter\\batter.glb");

    if (IsRightBatter()) 
    {
        position = { -1.0f, 0.01f, -0.4f };
        scale = { 1.0f,1.0f,1.0f };
    }
    else 
    {
        position = { 1.0f, 0.01f, -0.4f };
		scale = { -1.0f,1.0f,1.0f };
    }
    angle = { 0.0f, 0.0f, 0.0f};
	radius = 0.5f;
	height = 3.5f;

    // アニメーション用のノードをコピー
    animated_nodes = animated_model->nodes;

	//ステートごとのアニメーションインデックス設定
	animation_indices[static_cast<int>(State::BattingIdle)] = 0;      // Idleアニメーション
	animation_indices[static_cast<int>(State::BeforeSwing)] = 1; // BattingIdleアニメーション
	animation_indices[static_cast<int>(State::Swinging)] = 2;   // Swingingアニメーション
	animation_indices[static_cast<int>(State::Idle)] = 3;    // HomeRunアニメーション

    // 初期ステート設定
    current_state = State::BattingIdle;
    current_animation_index = animation_indices[static_cast<int>(current_state)];

    //バットモデルの読み込み
    bat = std::make_unique<Model>(".\\resources\\object\\bat.mdl");
	batModel = std::make_unique<gltf_model>(device, ".\\resources\\object\\bat.glb");
    batScale = { 1.2f,1.1f,1.2f };
    batPosition = { 0.08f, 0.0f, 0.05f };
    batAngle = { 0.0f, 0.0f, 1.6f, 0.0f };
	batRadius = 0.2f;
	batHeight = 1.0f;

    meshScale = { 0.03f,0.012f,0.03f };

	sweetSpotOffset = { 0.0f, 0.75f, 0.0f };
	sweetSpotScale = { 0.2f, 0.2f, 0.2f };
   
    //バット型の凸形状のメッシュ作成
    {
		const ModelResource* resource = bat->GetResource();

        //頂点数カウント
        size_t numVertices = 0;
        for (const ModelResource::Mesh& mesh : resource->GetMeshes()) 
        {
			numVertices += mesh.vertices.size();
        }

        //すべてのメッシュの頂点座標を収集
		std::vector<DirectX::XMFLOAT3> vertices(numVertices);
		numVertices = 0;
        for (const ModelResource::Mesh& mesh : resource->GetMeshes())
        {
			const Model::Node& node = bat->GetNodes().at(mesh.nodeIndex);
			DirectX::XMMATRIX NodeTransform = DirectX::XMLoadFloat4x4(&node.globalTransform);

            for (const ModelResource::Vertex& vertex : mesh.vertices)
            {
				DirectX::XMVECTOR Position = DirectX::XMLoadFloat3(&vertex.position);
				Position = DirectX::XMVector3Transform(Position, NodeTransform);

				DirectX::XMFLOAT3& v = vertices.at(numVertices++);
				DirectX::XMStoreFloat3(&v, Position);

				/*v.x *= batScale.x;
				v.y *= batScale.y;
				v.z *= batScale.z;*/
            }
        }

		//凸形状のメッシュ作成
		physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
		physx::PxMaterial* pxMaterial = Physics::Instance().GetMaterial();
		physx::PxScene* pxScene = Physics::Instance().GetScene();

		//pxMaterial->setRestitution(0.2f);// 反発係数を設定
		//pxMaterial->setDynamicFriction(0.3f);// 動的摩擦係数を設定
		//pxMaterial->setStaticFriction(0.3f);// 静止摩擦係数を設定

        //バット専用マテリアルの作成
        pxBatMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.5f);

		physx::PxConvexMeshDesc pxConvexMeshDesc;
		pxConvexMeshDesc.points.count = static_cast<physx::PxU32>(vertices.size());
		pxConvexMeshDesc.points.data = vertices.data();
		pxConvexMeshDesc.points.stride = sizeof(DirectX::XMFLOAT3);
		pxConvexMeshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

        physx::PxTolerancesScale pxTolerances;
		const physx::PxCookingParams pxCookingParams(pxTolerances);
        pxBatConvexMesh = PxCreateConvexMesh(pxCookingParams, pxConvexMeshDesc);

        //動的剛体生成
        physx::PxTransform pxTransform = physx::PxTransform(
            physx::PxVec3(batPosition.x, batPosition.y, batPosition.z),
            physx::PxQuat(0.0f, 0.0f, 0.0f, 1.0f));
        pxBatRigidBody = pxPhysics->createRigidDynamic(pxTransform);
        _ASSERT_EXPR(pxBatRigidBody != nullptr, "Failed to create PxRigidDynamic for bat");

        // キネマティックモードに設定
        pxBatRigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);

        //形状を生成して剛体にアタッチ（スケールを適用）
        physx::PxMeshScale pxMeshScale(
            physx::PxVec3(meshScale.x, meshScale.y, meshScale.z),
            physx::PxQuat(physx::PxIdentity)
        );
        physx::PxConvexMeshGeometry pxConvexGeometry(pxBatConvexMesh, pxMeshScale);
        physx::PxRigidActorExt::createExclusiveShape(*pxBatRigidBody, pxConvexGeometry, *pxBatMaterial);

        // 質量の設定
		//pxBatRigidBody->setMass(0.9f); // バットの質量を設定
        physx::PxRigidBodyExt::setMassAndUpdateInertia(*pxBatRigidBody, 0.9f);

        //シーンに剛体を追加
        pxScene->addActor(*pxBatRigidBody);
    }

    //同じpxBatRigidBodyにスイートスポットシェイプを追加(バットの芯)
    {
        
        physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
        physx::PxScene* pxScene = Physics::Instance().GetScene();

		physx::PxMaterial* sweetSpotMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.5f);

        physx::PxBoxGeometry sweetSpotGeometry(
            sweetSpotScale.x * 0.5f, // バットの芯の幅の半分
            sweetSpotScale.y * 0.5f, // バットの芯の高さの半分
            sweetSpotScale.z * 0.5f  // バットの芯の奥行きの半分
		);

		//ローカルオフセットを指定してシェイプを作成
        physx::PxTransform sweetSpotLocalPose(physx::PxVec3(sweetSpotOffset.x, sweetSpotOffset.y, sweetSpotOffset.z));
        physx::PxShape* sweetSpotShape = physx::PxRigidActorExt::createExclusiveShape(
        *pxBatRigidBody, sweetSpotGeometry, *sweetSpotMaterial);

        sweetSpotShape->setLocalPose(sweetSpotLocalPose);
        sweetSpotShape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
        sweetSpotShape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
        sweetSpotShape->setName("BatSweetSpot");
    }
}

// 解放
void Player::Uninitialize()
{
    PX_RELEASE(pxPlayerCapsuleController);
    physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
    physx::PxScene* pxScene = Physics::Instance().GetScene();

    pxScene->removeActor(*pxBatRigidBody);

    PX_RELEASE(pxBatConvexMesh);
    PX_RELEASE(pxBatMaterial);
}

// プレイヤー固有の更新処理
void Player::Update(float elapsedTime)
{


    // キー入力による移動処理
    HandleInput(elapsedTime);

	// アニメーション更新
	UpdateAnimation(elapsedTime);

    // 位置更新
    UpdateTransform();

    //バットをアタッチメント
	AttachBatToHand();

    // ボールの位置を取得してルックアット処理を実行
    const DirectX::XMFLOAT3& ballPosition = Ball::Instance().GetBallPosition();
    UpdateLookAt(ballPosition);

	// バットとボールの当たり判定
    //CheckBatAndBallCollision(elapsedTime);
}

// キー入力処理
void Player::HandleInput(float elapsedTime)
{
  
    // スペースキーでスイング
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        if (current_state != State::Swinging)
        {
            ChangeState(State::Swinging);
        }
    }

}

void Player::UpdateLookAt(const DirectX::XMFLOAT3& targetPosition)
{
    DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) };
    DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z) };
    DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(position.x, position.y, position.z) };
    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, S * R * T);

    // 首ノードのインデックスを取得
    int neck_joint_index = animated_model->GetNodeIndex("mixamorig:Head");
    if (neck_joint_index < 0) return; // 首ノードが存在しない場合は処理しない

    gltf_model::node& node = animated_nodes.at(neck_joint_index);

    // 首ノードのグローバル空間での位置を取得
    DirectX::XMFLOAT4 joint_position = { node.global_transform._41, node.global_transform._42, node.global_transform._43, 1.0f }; // global space

    // ターゲット位置を取得（ボールの位置）
    DirectX::XMFLOAT4 target_position = { targetPosition.x, targetPosition.y, targetPosition.z, 1.0f }; // world space

    // ターゲット位置をグローバル空間に変換
    DirectX::XMStoreFloat4(&target_position, DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&target_position), DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&world))));

    // ターゲットまでのベクトルを計算（グローバル空間）
    DirectX::XMFLOAT3 to_target = {
        target_position.x - joint_position.x,
        target_position.y - joint_position.y,
        target_position.z - joint_position.z
    };

    // グローバル空間での前方向
    DirectX::XMFLOAT3 forward = { 0, 0, 1 }; // global space

    // グローバル空間からボーン空間に変換
    DirectX::XMMATRIX inverse_global_transform = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&node.global_transform));
    DirectX::XMStoreFloat3(&to_target, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&to_target), inverse_global_transform));
    DirectX::XMStoreFloat3(&forward, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&forward), inverse_global_transform));

    // ベクトルを正規化
    DirectX::XMVECTOR to_target_vec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&to_target));
    DirectX::XMVECTOR forward_vec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&forward));

    // 回転軸と回転角を計算
    DirectX::XMVECTOR axis = DirectX::XMVector3Cross(forward_vec, to_target_vec);
    float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(forward_vec, to_target_vec));
    dot = std::clamp(dot, -1.0f, 1.0f); // 安全のためクランプ
    float angle = acosf(dot);

    // 回転角に制限を設定（-45度から45度の範囲に制限）
    constexpr float max_angle = DirectX::XMConvertToRadians(45.0f); // 最大回転角
    angle = std::clamp(angle, -max_angle, max_angle);

    // 回転行列を作成
    if (!DirectX::XMVector3Equal(axis, DirectX::XMVectorZero())) // 回転軸がゼロでない場合のみ回転
    {
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(DirectX::XMVector3Normalize(axis), angle);

        // 首ノードのグローバル行列を更新
        DirectX::XMStoreFloat4x4(&node.global_transform, rotation * DirectX::XMLoadFloat4x4(&node.global_transform));
    }

    // 子ノードのグローバル行列を再帰的に更新
    std::function<void(int, int)> traverse = [&](int parent_index, int node_index)
        {
            gltf_model::node& node = animated_nodes.at(node_index);
            if (parent_index > -1)
            {
                DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
                DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
                DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);
                DirectX::XMStoreFloat4x4(&node.global_transform, S * R * T * DirectX::XMLoadFloat4x4(&animated_nodes.at(parent_index).global_transform));
            }
            for (int child_index : node.children)
            {
                traverse(node_index, child_index);
            }
        };
    traverse(-1, neck_joint_index);
}
// プレイヤー固有のレンダリング処理
void Player::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    animated_model->render(rc.deviceContext, transform, animated_nodes);
    

    //renderer->Render(rc, batTransform, bat.get(), ShaderId::Phong);
    batModel->render(rc.deviceContext, batTransform, {});
  
}

void Player::DrawGUI()
{
#ifdef USE_IMGUI
    if (ImGui::Begin(u8"プレイヤー"))
    {
        if (ImGui::CollapsingHeader("Player"))
        {
            ImGui::DragFloat3("Position", &position.x);
            ImGui::DragFloat3("Scale", &scale.x);
            ImGui::DragFloat3("Angle", &angle.x);
            ImGui::DragFloat("Move Speed", &move_speed, 0.1f, 0.0f, 100.0f);

            // カプセルのサイズ変更用スライダー
            ImGui::DragFloat("Radius", &radius, 0.1f, 1.0f, 100.0f); // 半径
            ImGui::DragFloat("Height", &height, 0.1f, 1.0f, 200.0f); // 高さ
        }
        if (ImGui::CollapsingHeader("Bat"))
        {
            ImGui::DragFloat3("Bat Position", &batPosition.x);
            ImGui::DragFloat3("Bat Scale", &batScale.x);
            ImGui::DragFloat3("Bat Angle", &batAngle.x);

            // バットの質量を計算して表示
            float originalMass = 0.9f; // 実際のバットの質量 (kg)
            ImGui::Text("Bat Mass (scaled): %.6f kg", originalMass);
        }

        // PhysXメッシュ単体操作用
        if (ImGui::CollapsingHeader("PhysX Bat Mesh Debug"))
        {
            ImGui::Text("PhysX Mesh Transform (Independent)");

            

            
            // PhysXメッシュのスケール  
            if (ImGui::DragFloat3("Mesh Scale", &meshScale.x, 0.01f, 0.01f, 10.0f))
            {
                UpdatePhysXMeshTransform(meshScale);
            }

            // リセットボタン
            if (ImGui::Button("Reset Mesh Transform"))
            {
                meshScale = { 0.03f, 0.012f, 0.03f };
                UpdatePhysXMeshTransform(meshScale);
            }
           

			//バットのスイートスポットの位置とサイズ
			ImGui::DragFloat3("Sweet Spot Offset", &sweetSpotOffset.x, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat3("Sweet Spot Scale", &sweetSpotScale.x, 0.01f, 0.01f, 1.0f);

            if (batSweetSpot)
            {
                // 位置の更新
                physx::PxTransform transform(physx::PxVec3(sweetSpotOffset.x, sweetSpotOffset.y, sweetSpotOffset.z));
                batSweetSpot->setGlobalPose(transform);

                // サイズの更新
                physx::PxShape* shape = nullptr;
                batSweetSpot->getShapes(&shape, 1);
                if (shape)
                {
                    shape->setGeometry(physx::PxBoxGeometry(sweetSpotScale.x / 2.0f, sweetSpotScale.y / 2.0f, sweetSpotScale.z / 2.0f));
                }
            }
        }

        // アニメーションデバッグ用
        if (ImGui::CollapsingHeader("Animation"))
        {
            if (animated_model && !animated_model->animations.empty())
            {
                const gltf_model::animation& animation = animated_model->animations.at(current_animation_index);

                // アニメーション選択
                int prev_animation_index = current_animation_index;
                if (ImGui::SliderInt("Animation Index", &current_animation_index, 0, static_cast<int>(animated_model->animations.size()) - 1))
                {
                    // アニメーションが変更されたら時間をリセット
                    if (prev_animation_index != current_animation_index)
                    {
                        animation_time = 0.0f;
                    }
                }

                // アニメーション名の表示
                ImGui::Text("Current Animation: %s", animation.name.c_str());

                // 再生/停止ボタン
                if (ImGui::Checkbox("Playing", &animation_playing))
                {
                    // チェックボックスの状態が変わったときの処理
                }

                // タイムスライダー
                if (ImGui::SliderFloat("Time", &animation_time, 0.0f, animation.duration))
                {
                    // スライダーで時間を手動調整したときは再生を一時停止
                    animation_playing = false;
                }

                ImGui::Text("Duration: %.2f sec", animation.duration);

                // すべてのアニメーションをリスト表示
                if (ImGui::TreeNode("All Animations"))
                {
                    for (size_t i = 0; i < animated_model->animations.size(); ++i)
                    {
                        const gltf_model::animation& anim = animated_model->animations.at(i);
                        bool is_selected = (i == current_animation_index);

                        if (ImGui::Selectable(anim.name.c_str(), is_selected))
                        {
                            current_animation_index = static_cast<int>(i);
                            animation_time = 0.0f;
                            animation_playing = true;
                        }
                    }
                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::Text("No animations available");
            }
        }
    }
    ImGui::End();
#endif
}

void Player::UpdatePhysXMeshTransform(const DirectX::XMFLOAT3& scale)
{
    if (!pxBatRigidBody) return;

    // スケールも更新
    physx::PxShape* batShape;
    pxBatRigidBody->getShapes(&batShape, 1);

    physx::PxMeshScale pxMeshScale(
        physx::PxVec3(scale.x, scale.y, scale.z),
        physx::PxQuat(physx::PxIdentity)
    );
    physx::PxConvexMeshGeometry pxConvexGeometry(pxBatConvexMesh, pxMeshScale);
    batShape->setGeometry(pxConvexGeometry);
}

void Player::AttachBatToHand()
{
    const char* handName = "mixamorig:LeftHandMiddle1";

    // バットのローカル行列を計算（バット専用の変数を使用）
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(batScale.x, batScale.y, batScale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(batAngle.x, batAngle.y, batAngle.z);
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(batPosition.x, batPosition.y, batPosition.z);
    DirectX::XMMATRIX batLocalMatrix = S * R * T;

    for (const gltf_model::node& node : animated_nodes)
    {
        if (node.name == handName)
        {
            // 左手ノードの行列を取得
            DirectX::XMMATRIX leftHandMatrix = DirectX::XMLoadFloat4x4(&node.global_transform);

            // プレイヤーのワールド行列を取得
            DirectX::XMMATRIX playerWorldMatrix = DirectX::XMLoadFloat4x4(&transform);

            // バットのワールド行列を計算
            DirectX::XMMATRIX batWorldMatrix = batLocalMatrix * leftHandMatrix * playerWorldMatrix;

            // バットの行列を保存（batTransform に保存）
            DirectX::XMStoreFloat4x4(&batTransform, batWorldMatrix);

            // 凸形状メッシュの剛体に反映
            if (pxBatRigidBody)
            {
                // スケール、回転、位置を正しく分解
                DirectX::XMVECTOR scale;
                DirectX::XMVECTOR rotation;
                DirectX::XMVECTOR translation;
                DirectX::XMMatrixDecompose(&scale, &rotation, &translation, batWorldMatrix);

                // クォータニオンに変換
                DirectX::XMFLOAT4 quatFloat;
                DirectX::XMStoreFloat4(&quatFloat, rotation);

                // PhysXのトランスフォームを設定
                physx::PxTransform pxTransform(
                    physx::PxVec3(
                        DirectX::XMVectorGetX(translation),
                        DirectX::XMVectorGetY(translation),
                        DirectX::XMVectorGetZ(translation)
                    ),
                    physx::PxQuat(
                        quatFloat.x,
                        quatFloat.y,
                        quatFloat.z,
                        quatFloat.w
                    )
                );

                // キネマティックモードに設定（アニメーションに追従）
                pxBatRigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
                pxBatRigidBody->setKinematicTarget(pxTransform);
            }

			// スイートスポットの位置も更新
            if (batSweetSpot)
            {
                // ローカルオフセットをワールド行列で変換（スケール・回転・位置すべて考慮）
                DirectX::XMMATRIX offsetMatrix = DirectX::XMMatrixTranslation(
                    sweetSpotOffset.x, sweetSpotOffset.y, sweetSpotOffset.z);
                DirectX::XMMATRIX sweetSpotWorldMatrix = offsetMatrix * batWorldMatrix;

                // 位置・回転を取り出す
                DirectX::XMVECTOR scale;
                DirectX::XMVECTOR rotation;
                DirectX::XMVECTOR translation;
                DirectX::XMMatrixDecompose(&scale, &rotation, &translation, sweetSpotWorldMatrix);

                DirectX::XMFLOAT4 quatFloat;
                DirectX::XMStoreFloat4(&quatFloat, rotation);

                physx::PxTransform sweetSpotTransform(
                    physx::PxVec3(
                        DirectX::XMVectorGetX(translation),
                        DirectX::XMVectorGetY(translation),
                        DirectX::XMVectorGetZ(translation)
                    ),
                    physx::PxQuat(
                        quatFloat.x,
                        quatFloat.y,
                        quatFloat.z,
                        quatFloat.w
                    )
                );

                batSweetSpot->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
                batSweetSpot->setKinematicTarget(sweetSpotTransform);
            }

            // ボーンが見つかったらループを抜ける
            break;
        }
    }
}

// ステートマシン更新
void Player::UpdateAnimation(float elapsedTime)
{
    if (animation_playing && animated_model && !animated_model->animations.empty())
    {
        animation_time += elapsedTime;

        // 現在のアニメーションを再生
        animated_model->animate(current_animation_index, animation_time, animated_nodes);

        // アニメーションの長さを取得
        float animation_duration = animated_model->animations[current_animation_index].duration;

		

        if (Pitcher::Instance().GetCurrentState() == Pitcher::State::Throwing)
        {
            ThrowingStateTime += elapsedTime;

            // ThrowingStateTimeが0.8以上で、まだアニメーションを再生していない場合
            if (ThrowingStateTime >= 0.75f && !hasPlayHomeRun)
            {
                ChangeState(State::BeforeSwing); // ホームランアニメーションに切り替え
                hasPlayHomeRun = true;      // アニメーション再生済みフラグを設定
            }
        }
        else if (current_state == State::BeforeSwing)
        {
            // BeforeSwingアニメーションが終了したらBattingIdleに戻す
            if (animation_time >= animated_model->animations[current_animation_index].duration)
            {
                ChangeState(State::BattingIdle);
                ThrowingStateTime = 0.0f;  // ThrowingStateTimeをリセット
                hasPlayHomeRun = false;    // フラグをリセット
            }
        }
        else
        {
            // Throwingステート以外になったらリセット
            ThrowingStateTime = 0.0f;
            hasPlayHomeRun = false; // フラグをリセット
        }

        // Swingアニメーションの場合、マウス位置に応じて腕の角度を変更
        if (current_state == State::Swinging)
        {
            // マウスカーソル位置を取得
            Mouse& mouse = Input::Instance().GetMouse();
            float mouseY = static_cast<float>(mouse.GetPositionY());
			float mouseX = static_cast<float>(mouse.GetPositionX());

            Graphics& graphics = Graphics::Instance();
            float screenHeight = static_cast<float>(graphics.GetScreenHeight());
			float screenWidth = static_cast<float>(graphics.GetScreenWidth());

            // マウスY座標を正規化（0.0～1.0）
            swingHeight = mouseY / screenHeight;
            swingHeight = std::clamp(swingHeight, 0.0f, 1.0f);

			// マウスX座標を正規化（-1.0～1.0）
            swingWidth = mouseX / screenWidth;
			swingWidth = std::clamp(swingWidth, -1.0f, 1.0f);

            // 腕の角度オフセットを計算（-45°～ +45°の範囲）
            armAngleOffset = DirectX::XMConvertToRadians(-45.0f + swingHeight * 90.0f);

            // 腕のボーンを変更
            ModifyArmBones();

            // スイング開始からの経過時間を更新
            swingStartTime += elapsedTime;

            if (animation_time >= animation_duration)
            {
                ChangeState(State::BattingIdle);
            }
        }
        else
        {
            // ループ処理
            if (animation_time > animation_duration)
            {
                animation_time = fmod(animation_time, animation_duration);
            }
        }

    }
    
}

// ステート切り替え
void Player::ChangeState(State newState)
{
    if (current_state == newState)
        return;

    previous_state = current_state;
    current_state = newState;

    // アニメーションインデックスを変更
    int new_index = animation_indices[static_cast<int>(newState)];

    // インデックスが有効範囲内かチェック
    if (animated_model && new_index >= 0 && new_index < animated_model->animations.size())
    {
        current_animation_index = new_index;
        animation_time = 0.0f;  // アニメーション時間をリセット
    }

    // 構えに戻った際、またはスイングを開始した際に当たり判定を復活させる
    if (newState == State::BattingIdle)
    {
        if (pxBatRigidBody)
        {
            physx::PxShape* shape = nullptr;
            if (pxBatRigidBody->getShapes(&shape, 1))
            {
                // 当たり判定を再度有効にする
                shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);
            }
        }
    }
}


// マウスの位置によってスイングの高さを変える
void Player::ModifyArmBones()
{
    // 左腕のボーンを探す
    int leftArmIndex = animated_model->GetNodeIndex("mixamorig:LeftArm");
    
    if (leftArmIndex < 0) return;

    // 左腕の回転を変更
    DirectX::XMMATRIX additionalRotation = DirectX::XMMatrixRotationX(armAngleOffset);
    UpdateNodeTransform(leftArmIndex, additionalRotation);

    // 子ノード（前腕以降）のグローバル変換を再計算
    UpdateChildrenRecursive(leftArmIndex);

	// 右腕のボーンを探す
    int rightArmIndex = animated_model->GetNodeIndex("mixamorig:RightShoulder");
    
    if (rightArmIndex < 0) return;
    // 右腕の回転を変更
    additionalRotation = DirectX::XMMatrixRotationY(-armAngleOffset);
    UpdateNodeTransform(rightArmIndex, additionalRotation);
    // 子ノード（前腕以降）のグローバル変換を再計算
    UpdateChildrenRecursive(rightArmIndex);
}

void Player::UpdateNodeTransform(int nodeIndex, const DirectX::XMMATRIX& additionalRotation)
{
    if (nodeIndex < 0 || nodeIndex >= animated_nodes.size()) return;

    gltf_model::node& node = animated_nodes[nodeIndex];

    // 現在のローカル変換を取得
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);

    // 追加の回転を適用
    DirectX::XMMATRIX localMatrix = S * additionalRotation * R * T;

    // グローバル変換を計算
    if (node.parent >= 0)
    {
        DirectX::XMMATRIX parentGlobal = DirectX::XMLoadFloat4x4(&animated_nodes[node.parent].global_transform);
        DirectX::XMMATRIX globalMatrix = localMatrix * parentGlobal;
        DirectX::XMStoreFloat4x4(&node.global_transform, globalMatrix);
    }
    else
    {
        DirectX::XMStoreFloat4x4(&node.global_transform, localMatrix);
    }
}

void Player::UpdateChildrenRecursive(int nodeIndex)
{
    if (nodeIndex < 0 || nodeIndex >= animated_nodes.size()) return;

    gltf_model::node& node = animated_nodes[nodeIndex];

    // すべての子ノードを更新
    for (int childIndex : node.children)
    {
        gltf_model::node& child = animated_nodes[childIndex];

        // 子のローカル変換を取得
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(child.scale.x, child.scale.y, child.scale.z);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&child.rotation));
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(child.translation.x, child.translation.y, child.translation.z);
        DirectX::XMMATRIX localMatrix = S * R * T;

        // 親のグローバル変換と結合
        DirectX::XMMATRIX parentGlobal = DirectX::XMLoadFloat4x4(&node.global_transform);
        DirectX::XMMATRIX globalMatrix = localMatrix * parentGlobal;
        DirectX::XMStoreFloat4x4(&child.global_transform, globalMatrix);

        // 再帰的に子の子も更新
        UpdateChildrenRecursive(childIndex);
    }
}
