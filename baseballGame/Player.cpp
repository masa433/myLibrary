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

    position = { 3.5f, 10.0f, 57.0f };
    scale = { -0.03f,0.03f,0.03f };
    angle = { 0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f};
	radius = 0.5f;
	height = 1.8f;

    // アニメーション用のノードをコピー
    animated_nodes = animated_model->nodes;

	//ステートごとのアニメーションインデックス設定
	animation_indices[static_cast<int>(State::BattingIdle)] = 0;      // Idleアニメーション
	animation_indices[static_cast<int>(State::HomeRun)] = 1; // BattingIdleアニメーション
	animation_indices[static_cast<int>(State::Swinging)] = 2;   // Swingingアニメーション
	animation_indices[static_cast<int>(State::Idle)] = 3;    // HomeRunアニメーション

    // 初期ステート設定
    current_state = State::BattingIdle;
    current_animation_index = animation_indices[static_cast<int>(current_state)];

    //バットモデルの読み込み
    bat = std::make_unique<gltf_model>(device, ".\\resources\\object\\bat.glb");
    batScale = { 1.2f,1.2f,1.2f };
    batPosition = { 8.0f, 0.0f, 4.0f };
    batAngle = { 0.0f, 0.0f, 20.6f };

   
	// バットのコライダー作成
    {
		physx::PxCapsuleControllerDesc capsuleDesc;
		capsuleDesc.position = physx::PxExtendedVec3(position.x, position.y, position.z);
		capsuleDesc.upDirection = physx::PxVec3(0, 1, 0);
		capsuleDesc.radius = radius; // スケールを考慮
		capsuleDesc.height = height; // スケールを考慮
		capsuleDesc.slopeLimit = 0.0f; // スロープ制限なし
		capsuleDesc.stepOffset = 0.0f; // ステップオフセットなし
		capsuleDesc.invisibleWallHeight = 0.0f; // 見えない壁なし
		capsuleDesc.maxJumpHeight = 0.0f; // ジャンプなし
		capsuleDesc.material = Physics::Instance().GetMaterial();
        capsuleDesc.registerDeletionListener = true;
        capsuleDesc.clientID = physx::PX_DEFAULT_CLIENT;
        capsuleDesc.userData = this;

		physx::PxControllerManager* controllerManager = Physics::Instance().GetControllerManager();
		pxCapsuleController = static_cast<physx::PxCapsuleController*>(controllerManager->createController(capsuleDesc));
        _ASSERT_EXPR(pxCapsuleController != nullptr, "Failed pxControllerManagar->createController");
        pxCapsuleController->setFootPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
    }
}

// 解放
void Player::Uninitialize()
{
    PX_RELEASE(pxCapsuleController);
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
    const DirectX::XMFLOAT3& ballPosition = Pitcher::Instance().GetBallPosition();
    UpdateLookAt(ballPosition);

	// バットとボールの当たり判定
    //CheckBatAndBallCollision(elapsedTime);
}

void Player::CheckBatAndBallCollision(float elapsedTime)
{
    // バットのワールド行列から位置を取得
    DirectX::XMMATRIX batWorldMatrix = DirectX::XMLoadFloat4x4(&batTransform);
    DirectX::XMVECTOR batPositionVec = batWorldMatrix.r[3];
    DirectX::XMFLOAT3 batWorldPosition;
    DirectX::XMStoreFloat3(&batWorldPosition, batPositionVec);

    // シリンダーオフセットを適用（バットのローカル空間からワールド空間へ変換）
    DirectX::XMVECTOR offsetVec = DirectX::XMLoadFloat3(&cylinderOffset);
    DirectX::XMVECTOR worldOffsetVec = DirectX::XMVector3TransformCoord(offsetVec, batWorldMatrix);
    DirectX::XMFLOAT3 adjustedBatPosition;
    DirectX::XMStoreFloat3(&adjustedBatPosition, worldOffsetVec);

    // バットのスケールを抽出
    DirectX::XMVECTOR scaleVec = DirectX::XMVector3Length(batWorldMatrix.r[0]);
    float extractedScale;
    DirectX::XMStoreFloat(&extractedScale, scaleVec);

    // ボールの位置と半径を取得
    const DirectX::XMFLOAT3& ballPosition = Pitcher::Instance().GetBallPosition();
    float ballRadius = Pitcher::Instance().GetReducedRadius();

    // バットの半径と高さにスケールを適用
    float scaledBatRadius = batRadius * extractedScale;
    float scaledBatHeight = batHeight * extractedScale * 3.0f;

    // 衝突判定
    DirectX::XMFLOAT3 collisionPoint;
    if (collision::IntersectSphereVsCylinder(
        ballPosition,
        ballRadius,
        adjustedBatPosition, // オフセット適用後の位置
        scaledBatRadius,
        scaledBatHeight,
        collisionPoint))
    {
        DirectX::XMFLOAT3 ballVelocity = Pitcher::Instance().GetBallVelocity();

        // 衝突法線を計算
        DirectX::XMVECTOR ballPosVec = DirectX::XMLoadFloat3(&ballPosition);
        DirectX::XMVECTOR collisionPointVec = DirectX::XMLoadFloat3(&collisionPoint);
        DirectX::XMVECTOR normalVec = DirectX::XMVector3Normalize(
            DirectX::XMVectorSubtract(ballPosVec, collisionPointVec));

        // ボールの速度を反射
        DirectX::XMVECTOR ballVelocityVec = DirectX::XMLoadFloat3(&ballVelocity);
        float restitution = 1.5f;
        DirectX::XMVECTOR reflectedVelocity = DirectX::XMVectorScale(
            DirectX::XMVector3Reflect(ballVelocityVec, normalVec), restitution);

        // バットのスイング速度を計算
        DirectX::XMVECTOR batVelocityVec = DirectX::XMVectorSubtract(
            batPositionVec,
            DirectX::XMLoadFloat3(&previousBatPosition)
        );
        batVelocityVec = DirectX::XMVectorScale(batVelocityVec, 1.0f / elapsedTime);

        // 反射速度にバットの速度を加算
        reflectedVelocity = DirectX::XMVectorAdd(reflectedVelocity, batVelocityVec);

        // 現在の位置を保存
        DirectX::XMStoreFloat3(&previousBatPosition, batPositionVec);

        DirectX::XMFLOAT3 newBallVelocity;
        DirectX::XMStoreFloat3(&newBallVelocity, reflectedVelocity);
        Pitcher::Instance().SetBallVelocity(newBallVelocity);

        OutputDebugStringA("Bat hit the ball!\n");
    }
}

// キー入力処理
void Player::HandleInput(float elapsedTime)
{
    DirectX::XMFLOAT3 move_direction = { 0.0f, 0.0f, 0.0f };
    

    // Wキー: 前進
    if (GetAsyncKeyState('W') & 0x8000)
    {
        move_direction.z += 100.0f;
        
    }
    // Sキー: 後退
    if (GetAsyncKeyState('S') & 0x8000)
    {
        move_direction.z -= 100.0f;
        
    }
    // Aキー: 左移動
    if (GetAsyncKeyState('A') & 0x8000)
    {
        move_direction.x -= 100.0f;
        
    }
    // Dキー: 右移動
    if (GetAsyncKeyState('D') & 0x8000)
    {
        move_direction.x += 100.0f;
        
    }

    // スペースキーでスイング
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        if (current_state != State::Swinging)
        {
            ChangeState(State::Swinging);
        }
    }

    // 移動方向を正規化
    DirectX::XMVECTOR moveVec = DirectX::XMLoadFloat3(&move_direction);
    if (!DirectX::XMVector3Equal(moveVec, DirectX::XMVectorZero()))
    {
        moveVec = DirectX::XMVector3Normalize(moveVec);
        DirectX::XMStoreFloat3(&move_direction, moveVec);
    }

    // 重力を適用
    float gravityEffect = gravity * elapsedTime;

    // PhysXキャラクターコントローラーを使用して移動
    physx::PxVec3 displacement(move_direction.x * move_speed * elapsedTime, gravityEffect, move_direction.z * move_speed * elapsedTime);
    physx::PxControllerCollisionFlags collisionFlags = pxCapsuleController->move(displacement, 0.0f, elapsedTime, physx::PxControllerFilters());

    // 地面に接触しているかを判定
    //isOnGround = (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN) != 0;

    // キャラクターの位置を更新
    physx::PxExtendedVec3 footPosition = pxCapsuleController->getFootPosition();
    position.x = static_cast<float>(footPosition.x);
    position.y = static_cast<float>(footPosition.y);
    position.z = static_cast<float>(footPosition.z);

    // y座標が0.0f以下になった場合に0.0fで止める
    if (position.y < 0.0f)
    {
        position.y = 0.0f;

        // PhysXキャラクターコントローラーの位置も修正
        pxCapsuleController->setFootPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
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
void Player::Render(RenderContext& rc)
{
    animated_model->render(rc.context, transform, animated_nodes);
    bat->render(rc.context, batTransform, {});

    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

    // バットのワールド行列を取得
    DirectX::XMMATRIX batWorldMatrix = DirectX::XMLoadFloat4x4(&batTransform);

    // シリンダーオフセットを適用した行列を作成
    DirectX::XMMATRIX offsetMatrix = DirectX::XMMatrixTranslation(
        cylinderOffset.x, cylinderOffset.y, cylinderOffset.z);
    DirectX::XMMATRIX adjustedBatMatrix = offsetMatrix * batWorldMatrix;

    // バットの当たり判定表示（オフセット適用後）
    //shapeRenderer->DrawCylinder(
    //    adjustedBatMatrix,     // オフセット適用後の行列
    //    batRadius,             // 半径
    //    batHeight,             // 高さ
    //    { 1.0f, 0.0f, 0.0f, 1.0f } // 色
    //);
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
        }
        if (ImGui::CollapsingHeader("Bat"))
        {
            ImGui::DragFloat3("Bat Position", &batPosition.x);
            ImGui::DragFloat3("Bat Scale", &batScale.x);
            ImGui::DragFloat3("Bat Angle", &batAngle.x);
        }
        // バットの当たり判定デバッグ用
        if (ImGui::CollapsingHeader("Bat Debug"))
        {
            ImGui::DragFloat("Bat Radius", &batRadius, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Bat Height", &batHeight, 0.1f, 0.1f, 100.0f);
            // 追加: シリンダーの位置オフセット
            ImGui::DragFloat3("Cylinder Offset", &cylinderOffset.x, 0.1f, -100.0f, 100.0f);
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

//アタッチメント処理
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
        if (node.name == handName)  // std::stringの比較に変更
        {
            // 左手ノードの行列を取得
            DirectX::XMMATRIX leftHandMatrix = DirectX::XMLoadFloat4x4(&node.global_transform);

            // プレイヤーのワールド行列を取得
            DirectX::XMMATRIX playerWorldMatrix = DirectX::XMLoadFloat4x4(&transform);

            // バットのワールド行列を計算
            DirectX::XMMATRIX batWorldMatrix = batLocalMatrix * leftHandMatrix * playerWorldMatrix;

            // バットの行列を保存（batTransform に保存）
            DirectX::XMStoreFloat4x4(&batTransform, batWorldMatrix);

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
