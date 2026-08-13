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
#include "batSprite.h"
#include "ballSprite.h"
#include <GameTimer.h>
#include <ballCount.h>
#include <Money.h>
#include "shader.h"


// 初期化
void Player::Initialize()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    // モデルの読み込み
    rightBatter = std::make_unique<gltf_model>(device, ".\\resources\\batter\\rightBatter.glb");
    leftBatter = std::make_unique<gltf_model>(device, ".\\resources\\batter\\leftBatter.glb");


    if (IsRightBatter())
    {
        position = { -1.0f, 0.01f, -0.4f };
        batPosition = { -0.08f, 0.0f, 0.05f };
        batAngle = { 0.0f, 0.0f, -1.6f };
    }
    else
    {
        position = { 1.0f, 0.01f, -0.4f };
        batPosition = { 0.08f, 0.0f, 0.05f };
        batAngle = { 0.0f, 0.0f, 1.6f };
    }
    angle = { 0.0f, 0.0f, 0.0f };

    // アニメーション用のノードをコピー

    animated_nodes = rightBatter->nodes;
    animated_nodes = leftBatter->nodes;

    //ステートごとのアニメーションインデックス設定
    animation_indices[static_cast<int>(State::BattingIdle)] = 0;      // Idleアニメーション
    animation_indices[static_cast<int>(State::BeforeSwing)] = 1; // BattingIdleアニメーション
    animation_indices[static_cast<int>(State::Swinging)] = 2;   // Swingingアニメーション


    // 初期ステート設定
    current_state = State::BattingIdle;
    current_animation_index = animation_indices[static_cast<int>(current_state)];

    //バットモデルの読み込み
    bat = std::make_unique<Model>(".\\resources\\object\\bat.mdl");
    batModel = std::make_unique<gltf_model>(device, ".\\resources\\object\\bat.glb");
    batScale = { 1.15f,1.1f,1.15f };

    meshScale = { 0.03f,0.012f,0.03f };

    rightBatter->build_static_batches(device);
    leftBatter->build_static_batches(device);
    batModel->build_static_batches(device);

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
        // CCDを有効化
        pxBatRigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
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

        physx::PxShape* shape = nullptr;
        if (pxBatRigidBody->getShapes(&shape, 1))
        {
            shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
            shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
        }
    

        //シーンに剛体を追加
        pxScene->addActor(*pxBatRigidBody);
    }

    ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
    D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(),
        input_element_desc, _countof(input_element_desc));
    create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

    swingTimingPosition = isRightBatter ? DirectX::XMFLOAT2(1050.0f, 450.0f) : DirectX::XMFLOAT2(650.0f, 450.0f);

	swingTimingInfo = std::make_unique<SwingTimingInfo>();
	swingTimingInfo->texturePath[static_cast<int>(SwingTiming::Late)] = L".\\resources\\textures\\swingLate.png";
	swingTimingInfo->texturePath[static_cast<int>(SwingTiming::Early)] = L".\\resources\\textures\\swingEarly.png";
    swingTimingInfo->position = {swingTimingPosition.x, swingTimingPosition.y};
    swingTimingInfo->size = {swingTimingSize.x, swingTimingSize.y};
    swingTimingInfo->rotation = 0.0f;
    swingTimingInfo->color = {swingTimingColor.x, swingTimingColor.y, swingTimingColor.z, swingTimingColor.w};
    for (int i = 1; i < static_cast<int>(SwingTiming::Count); ++i)
    {
        swingTimingSprite[i] = std::make_unique<sprite>(
            device,
            context,
            swingTimingInfo->texturePath[i].c_str()
        );
    }


	SelectRealBatter(selectedRealBatter);
	UpdateBatterModel();

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

    consoleLog = nullptr;
    currentBatter = nullptr;

    animated_nodes.clear();
    realBatterInfo.clear();

    // GPUリソース解放
    bat.reset();
    batModel.reset();
    rightBatter.reset();
    leftBatter.reset();
}

void Player::UpdateBatterModel()
{
    // currentBatter を利き手に合わせて切り替え (.get() で生ポインタを取得)
	currentBatter = isRightBatter ? rightBatter.get() : leftBatter.get();

    // currentBatter が有効なら、ノード情報をコピーしアニメーション時間をリセット
    if (currentBatter)
    {
        animated_nodes = currentBatter->nodes;
        animation_time = 0.0f;
        current_animation_index = animation_indices[static_cast<int>(current_state)];
    }

    // 立ち位置やバット位置の調整
    position = isRightBatter ? DirectX::XMFLOAT3(-1.0f, 0.01f, -0.4f) : DirectX::XMFLOAT3(1.0f, 0.01f, -0.4f);
    batPosition = isRightBatter ? DirectX::XMFLOAT3(-0.08f, 0.0f, 0.05f) : DirectX::XMFLOAT3(0.08f, 0.0f, 0.05f);
    batAngle = isRightBatter ? DirectX::XMFLOAT3(0.0f, 0.0f, -1.6f) : DirectX::XMFLOAT3(0.0f, 0.0f, 1.6f);

    swingTimingPosition.x = isRightBatter ? 1050.0f : 650.0f;
}


// プレイヤー固有の更新処理
void Player::Update(float elapsedTime)
{

    // HitJudge2Dの更新（スイング判定）
    {
        ballSprite& bs = ballSprite::Instance();
        DirectX::XMFLOAT2 ballSprPos = bs.GetBallSpritePosition();
        DirectX::XMFLOAT2 ballSprSize = bs.GetBallSpriteSize();
        DirectX::XMFLOAT2 ballCenter =
        {
            ballSprPos.x + ballSprSize.x * 0.5f,
            ballSprPos.y + ballSprSize.y * 0.5f
        };
        float ballRadius = ballSprSize.x * 0.5f;
        HitJudge2D::Instance().SetBallRadiusPx(ballRadius);

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(GetForegroundWindow(), &pt);
        float mouseX = static_cast<float>(pt.x);
        float mouseY = static_cast<float>(pt.y);
        DirectX::XMFLOAT2 zoneTopLeft, zoneBottomRight;
        bs.GetBallZoneScreenBounds(zoneTopLeft, zoneBottomRight);
        mouseX = std::max(zoneTopLeft.x, std::min(zoneBottomRight.x, mouseX));
        mouseY = std::max(zoneTopLeft.y, std::min(zoneBottomRight.y, mouseY));


		float zoneHeight = zoneBottomRight.y - zoneTopLeft.y;
		float normalizedY = (mouseY - zoneTopLeft.y) / zoneHeight;
		normalizedY = std::max(0.0f, std::min(1.0f, normalizedY));

        float highAngle = -5.0f; // 高めの角度
        float lowAngle = 45.0f;  // 低めの角度
        float centerAngle = 25.0f; // 中心の角度

        float targetRotation = highAngle + (lowAngle - highAngle) * normalizedY;// 線形補間で角度を計算

        const DirectX::XMFLOAT2 batSize = BatSprite::Instance().GetBatSpriteSize();
        
        float batRotPhysics; // 当たり判定用の回転
        float pivotRatioX = 0.0f;
        

        if (IsRightBatter())
        {
            
            
            batRotPhysics = targetRotation;
			
        }
        else
        {
            batRotPhysics = -targetRotation;
			
        }

		pivotRatioX = 0.7f; // バットのピボット位置（0.0f:左端、1.0f:右端）

		float localPivotX = (pivotRatioX - 0.5f) * batSize.x; // バットのローカル座標でのピボット位置
		float localPivotY = 0.0f; // バットのローカル座標でのピボット位置（Yは中央）


		float rad = DirectX::XMConvertToRadians(batRotPhysics);
		float rotatedPivotX = localPivotX * cosf(rad) - localPivotY * sinf(rad);
		float rotatedPivotY = localPivotX * sinf(rad) + localPivotY * cosf(rad);

        DirectX::XMFLOAT2 batCenter = {
            mouseX - rotatedPivotX,
            mouseY - rotatedPivotY
        };


        // 紫バットOBBを別途計算してボールと重なり判定
        // 紫バットは白バットの先端寄り1/3程度（芯～先端）と仮定
        // batSprite.png上の紫部分のサイズ・オフセットに合わせて調整してください
        {
            const DirectX::XMFLOAT2 purpleSize = BatSprite::Instance().GetBatSpriteSize();
            // 紫バットはバットOBBのローカル座標で先端側にオフセット
            // 右打ち：バットOBB中心からローカルX+方向（先端）にずらす
            float offsetAlongBat = (batSize.x * 0.5f) - (purpleSize.x * 0.5f); // 先端寄りのオフセット量

            float radBat = DirectX::XMConvertToRadians(batRotPhysics);
            float cosB = cosf(radBat), sinB = sinf(radBat);

            OBB2D purpleOBB;
            purpleOBB.center = {
                batCenter.x + cosB * offsetAlongBat,
                batCenter.y + sinB * offsetAlongBat
            };
            purpleOBB.halfSize = { purpleSize.x * 0.5f, purpleSize.y * 0.5f };
            purpleOBB.rotationDeg = batRotPhysics;

            isPurpleBat = HitJudge2D::OBBvsCircle(purpleOBB, ballCenter, ballRadius);
        }

        // 白丸（cursorCenter）とボールの重なり判定は HitJudge2D 内で行う
        // cursorRadius は HitJudge2D のメンバで調整可能
        DirectX::XMFLOAT2 cursorCenter = { mouseX, mouseY };

        float estTime = 99.0f;
        if (Ball::Instance().IsBezierFlying())
        {
            estTime = Ball::Instance().GetBezierRemainingTime();
        }
        else
        {
            // ベジェ終了後（コンタクト直前のaddForceフェーズ）は従来通り物理速度で外挿
            physx::PxVec3 vel = Ball::Instance().GetLinearVelocity();
            float ballZ = Ball::Instance().GetWorldPosition().z;
            if (vel.z < -0.001f && ballZ > 0.0f)
                estTime = ballZ / (-vel.z);
            else if (ballZ <= 0.0f)
                estTime = -(std::abs)(ballZ) / 0.1f;
        }

        HitJudge2D::Instance().isPurpleBat = isPurpleBat;

        // ストライクゾーンのスクリーン境界を取得
        DirectX::XMFLOAT2 szTopLeft, szBottomRight;
        bs.GetStrikeZoneScreenBounds(szTopLeft, szBottomRight); // 後述のgetter

        // ボールスプライトがストライクゾーン外 = ボールゾーン
        bool isBallZone = (ballCenter.x < szTopLeft.x || ballCenter.x > szBottomRight.x ||
            ballCenter.y < szTopLeft.y || ballCenter.y > szBottomRight.y);

		float zoneCenterX = (szTopLeft.x + szBottomRight.x) * 0.5f;
		float zoneWidth = szBottomRight.x - szTopLeft.x;

		float insideCourseMargin = zoneWidth * 0.2f; // コース内のマージン（px）

		bool isInsideCourse = IsRightBatter() ? (ballCenter.x < zoneCenterX - insideCourseMargin) : (ballCenter.x > zoneCenterX + insideCourseMargin);

        HitJudge2D::Instance().isBallZone = isBallZone; // 後述のメンバ
        HitJudge2D::Instance().isInsideCourse = isInsideCourse; // 後述のメンバ

        HitJudge2D::Instance().Update(
            ballCenter, batCenter, batSize, batRotPhysics, cursorCenter, estTime);


    }

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

    float ballZ = Ball::Instance().GetWorldPosition().z;
    if (ballZ <= -5.0f && currentSwingTiming != SwingTiming::None && !Ball::Instance().GetHasCollidedWithBat())
    {
        showSwingTimingSprite = true;
    }
}

// キー入力処理
void Player::HandleInput(float elapsedTime)
{
    //if(GameTimer::Instance().GetRemainingTime() <= 0.0f && !Ball::Instance().GetHasCollidedWithBat() && !(Pitcher::Instance().GetCurrentState() == Pitcher::State::Throwing)) return;
	if (ballCount::Instance().GetRemainingBalls() <= 0) return;

    float ballZ = Ball::Instance().GetWorldPosition().z;

    
  
    // スペースキーでスイング
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000 && ballZ >= -3.0f)
    {
        //スイングカウントを増やす
		IncreaseSwingCount();

        

        if (current_state != State::Swinging)
        {      
            if(Ball::Instance().IsBezierFlying())
            {
                remainingTime = Ball::Instance().GetBezierRemainingTime();
                if (remainingTime > 0.2f)
                {
                    currentSwingTiming = SwingTiming::Early;
                }
				else
                {
                    currentSwingTiming = SwingTiming::Late;
                }
            }
            
        
             ChangeState(State::Swinging); 
             if (Pitcher::Instance().GetIsBallThrown())
             {
                 ballCount::Instance().DecreaseRemainingBalls(1);
				 hasSwungThisPitch = true; // スイングしたことを記録
             }
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
    int neck_joint_index = currentBatter->GetNodeIndex("mixamorig:Head");
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
    //animated_model->render_batched(rc.deviceContext, transform, animated_nodes);
    //

    ////renderer->Render(rc, batTransform, bat.get(), ShaderId::Phong);
    //batModel->render_batched(rc.deviceContext, batTransform, {});

    RenderPlayer(rc, renderer);
    RenderBat(rc, renderer);


}

void Player::RenderPlayer(const RenderContext& rc, ModelRenderer* renderer)
{
    currentBatter->render_batched(rc.deviceContext, transform, animated_nodes);

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();

    dc->VSSetShader(spriteVS.Get(), nullptr, 0);
    dc->PSSetShader(spritePS.Get(), nullptr, 0);
    dc->IASetInputLayout(spriteInputLayout.Get());
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestOnly), 0);
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF); // 半透明のガラス調テクスチャなので有効化推奨


    if (showSwingTimingSprite && swingTimingInfo && currentSwingTiming != SwingTiming::None)
    {
        int timingIndex = static_cast<int>(currentSwingTiming);
        if (timingIndex >= 0 && timingIndex < static_cast<int>(SwingTiming::Count))
        {
            swingTimingSprite[timingIndex]->render(
                dc,
                swingTimingPosition.x,
                swingTimingPosition.y,
				swingTimingSize.x, swingTimingSize.y,
				swingTimingColor.x, swingTimingColor.y, swingTimingColor.z, swingTimingColor.w,
                swingTimingInfo->rotation
            );
		}
    }



    // 描画後の状態をリセット
    dc->VSSetShader(nullptr, nullptr, 0);
    dc->PSSetShader(nullptr, nullptr, 0);
    dc->IASetInputLayout(nullptr);

    dc->OMSetDepthStencilState(
        renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

void Player::RenderBat(const RenderContext& rc, ModelRenderer* renderer)
{
    batModel->render_batched(rc.deviceContext, batTransform, {});
}

void Player::DrawGUI()
{
#ifdef USE_IMGUI

	ImGui::DragInt("Swing Count", &swingCount, 1, 0, 10);

    if (ImGui::CollapsingHeader("Player Info"))
    {
        ImGui::DragFloat3("Position", &position.x);
        ImGui::DragFloat3("Scale", &scale.x);
        ImGui::DragFloat3("Angle", &angle.x);

        // 変更後
        bool prev = isRightBatter;
        if (ImGui::Checkbox("Right Handed", &isRightBatter))
        {
			UpdateBatterModel();
        }
    }
    if (ImGui::CollapsingHeader("Bat"))
    {
        ImGui::DragFloat3("Bat Position", &batPosition.x);
        ImGui::DragFloat3("Bat Scale", &batScale.x);
        ImGui::DragFloat4("Bat Angle", &batAngle.x);
        ImGui::Checkbox("Purple Bat", &isPurpleBat);

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

    }

    if(ImGui::CollapsingHeader("Swing Timing Sprite"))
    {
        ImGui::DragFloat2("Position", &swingTimingPosition.x);
		
        ImGui::DragFloat2("Size", &swingTimingSize.x);
        ImGui::ColorEdit4("Color", &swingTimingColor.x);

		ImGui::Text("Current Swing Timing: %s", (currentSwingTiming == SwingTiming::Early) ? "Early" : (currentSwingTiming == SwingTiming::Late) ? "Late" : "None");
        ImGui::DragFloat("remainingTime", &remainingTime, 0.01f, 0.0f, 1.0f);
	}
    
	HitJudge2D::Instance().DrawGUI();

    if (ImGui::CollapsingHeader(u8"実在打者プリセット"))
    {
        const char* realBatterNames[] = {
                u8"なし",
                u8"石山", u8"佐藤", u8"岡村", u8"坂本", u8"小田倉",
                u8"川野", u8"村上", u8"山田",
                u8"鈴木", u8"浅野", u8"木村", u8"松田", u8"村井",
                u8"佐々木",u8"武田",u8"長谷川",u8"西村",u8"小島",
                u8"西野",u8"田村",u8"清水",u8"山下",u8"中村",u8"上田"
        };
        int realBatterIndex = static_cast<int>(selectedRealBatter);
        if (ImGui::Combo(u8"実在打者", &realBatterIndex, realBatterNames, IM_ARRAYSIZE(realBatterNames)))
        {
            SelectRealBatter(static_cast<RealBatter>(realBatterIndex));
        }

		//各選手のパワー・ミートを表示・編集する
        if (selectedRealBatter != RealBatter::None)
        {
			int power = GetSelectedRealBatterPower();
            if (ImGui::DragInt(u8"パワー", &power, 1, 0, 99))
            {
                SetSelectedRealBatterPower(power);
			}

			int contact = GetSelectedRealBatterContact();
            if(ImGui::DragInt(u8"ミート", &contact, 1, 0, 99))
            {
                SetSelectedRealBatterContact(contact);
                BatSprite::Instance().UpdateCursorSizeByContact(static_cast<int>(contact));
			}
		}
    }

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
    const char* handName = IsRightBatter() ? "mixamorig:RightHandMiddle1" : "mixamorig:LeftHandMiddle1";

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

         
            // ボーンが見つかったらループを抜ける
            break;
        }
    }
}
// ステートマシン更新
void Player::UpdateAnimation(float elapsedTime)
{
    if (animation_playing && currentBatter&& !currentBatter->animations.empty())
    {
        //アニメーションの開始位置をどれくらい進めるか（秒単位で指定）
        // 例：最初の0.1秒をカットして、0.1秒の時点から再生を始める場合
        const float START_OFFSET = 0.05f;

        // スイングを始めたときに、アニメーションを早くする
        if (current_state == State::Swinging)
        {
            // 【追加】スイングが始まったまさにその瞬間（最初のフレーム）であれば
            if (swingStartTime == 0.0f && animation_time < START_OFFSET)
            {
                animation_time = START_OFFSET; // 開始位置を少し進める
            }


            swingStartTime += elapsedTime;
        }

        // BeforeSwingステート以外は通常の速度でアニメーションを再生
        if (current_state != State::BeforeSwing)
        {
            animation_time += elapsedTime;
        }
        else
        {
            // BeforeSwingステートでベジェ曲線投球中の場合、ベジェ曲線の進行度に合わせてアニメーション時間を調整
            if (isBezierPitching && Ball::Instance().IsBezierFlying())
            {
                float bezierT = Ball::Instance().GetBezierT();
                float animation_duration = currentBatter->animations[current_animation_index].duration;

                // ベジェ曲線の進行度に基づいてアニメーション時間を計算
                // bezierT = 0.0 の時は animation_time = beforeSwingStartTime
                // bezierT = 1.0 の時は animation_time = beforeSwingStartTime + animation_duration
                animation_time = beforeSwingStartTime + animation_duration * bezierT;
            }
            else
            {
                // ベジェ曲線が終了したら通常の速度でアニメーションを再生
                animation_time += elapsedTime;
            }
        }

        // 現在のアニメーションを再生
        currentBatter->animate(current_animation_index, animation_time, animated_nodes);

        // アニメーションの長さを取得
        float animation_duration = currentBatter->animations[current_animation_index].duration;

        
        if (current_state == State::BeforeSwing)
        {
            // BeforeSwingアニメーションが終了したらBattingIdleに戻す
            if (animation_time >= currentBatter->animations[current_animation_index].duration + beforeSwingStartTime)
            {
                ChangeState(State::BattingIdle);
                ThrowingStateTime = 0.0f;  // ThrowingStateTimeをリセット
                beforeSwingStartTime = 0.0f; // beforeSwingStartTimeをリセット
            }
        }
        else if (Pitcher::Instance().GetCurrentState() == Pitcher::State::Throwing)
        {
            ThrowingStateTime += elapsedTime;

            // ThrowingStateTimeが0.8以上で、まだアニメーションを再生していない場合
            if (ThrowingStateTime >= 0.75f && !hasPlayBeforeSwing)
            {
                ChangeState(State::BeforeSwing); // ホームランアニメーションに切り替え
                hasPlayBeforeSwing = true;      // アニメーション再生済みフラグを設定
                beforeSwingStartTime = animation_time; // BeforeSwing開始時のanimation_timeを記録
            }
        }
        else
        {
            // Throwingステート以外になったらリセット
            ThrowingStateTime = 0.0f;
            hasPlayBeforeSwing = false; // フラグをリセット
            beforeSwingStartTime = 0.0f; // beforeSwingStartTimeをリセット
        }

        // Swingアニメーションの場合、マウス位置に応じて腕の角度を変更
        if (current_state == State::Swinging)
        {
            // マウスカーソル位置を取得
            Mouse& mouse = Input::Instance().GetMouse();
            float mouseY = static_cast<float>(mouse.GetPositionY());
            float mouseX = static_cast<float>(mouse.GetPositionX());

            // ストライクゾーンのスクリーン境界を取得（コース判定に使用）
            DirectX::XMFLOAT2 zoneTopLeft, zoneBottomRight;
            ballSprite::Instance().GetBallZoneScreenBounds(zoneTopLeft, zoneBottomRight);
            float zoneWidth = zoneBottomRight.x - zoneTopLeft.x;
            float zoneHeight = zoneBottomRight.y - zoneTopLeft.y;

            Graphics& graphics = Graphics::Instance();
            float screenHeight = static_cast<float>(graphics.GetScreenHeight());

            // マウスY座標を正規化（0.0～1.0）：高さ用（画面基準のまま）
            swingHeight = mouseY / screenHeight;
            swingHeight = std::clamp(swingHeight, 0.0f, 1.0f);

            // マウスX座標をストライクゾーン基準で正規化（-1.0=内角側 ～ +1.0=外角側）
            swingWidth = 0.0f;
            if (zoneWidth > 0.0f)
            {
                swingWidth = ((mouseX - zoneTopLeft.x) / zoneWidth) * 2.0f - 1.0f;
                swingWidth = std::clamp(swingWidth, -1.0f, 1.0f);
            }

            // 高さ方向の腕の角度オフセット（-45°～ +45°の範囲）
            armAngleOffset = DirectX::XMConvertToRadians(-45.0f + swingHeight * 90.0f);

            // ゾーン中央(50%)からの高さの距離を「高め成分」「低め成分」に分ける（0.0～1.0）
            // swingHeightは画面基準で上が0.0/下が1.0なので、ゾーン内の高さ比率に変換して判定する
            float zoneSwingHeight = swingHeight; // フォールバック（ゾーン高さが取得できない場合）
            if (zoneHeight > 0.0f)
            {
                zoneSwingHeight = (mouseY - zoneTopLeft.y) / zoneHeight;
                zoneSwingHeight = std::clamp(zoneSwingHeight, 0.0f, 1.0f);
            }
            // zoneSwingHeightは0.0=高め ～ 1.0=低めの想定
            float lowFactor = std::clamp((zoneSwingHeight - 0.5f) / 0.5f, 0.0f, 1.0f);  // 中央以下は0、低めほど1.0
            float highFactor = std::clamp((0.5f - zoneSwingHeight) / 0.5f, 0.0f, 1.0f); // 中央以上は0、高めほど1.0

            // 打者の左右で内角/外角の向きが反転するため符号を補正
            // 右打者：画面右(swingWidth+)が外角、画面左(swingWidth-)が内角
            // 左打者：画面右(swingWidth+)が内角、画面左(swingWidth-)が外角
            float courseSign = IsRightBatter() ? -1.0f : 1.0f;
            float courseValue = courseSign * swingWidth; // +1.0=内角側 ～ -1.0=外角側

            // 内角成分・外角成分（それぞれ0.0～1.0）
            float inAmount = std::clamp(courseValue, 0.0f, 1.0f);   // 内角に振っている度合い
            float outAmount = std::clamp(-courseValue, 0.0f, 1.0f); // 外角に振っている度合い

            // 横方向の角度オフセット（最大振れ幅30度）
            // ・インロー(内角×低め)   : 下向き(プラス)を強める
            // ・インハイ(内角×高め)   : 上向き(マイナス)を強める
            // ・アウトロー/アウトハイ(外角は高さ問わず): 常に上向き(マイナス)を強める
            static constexpr float maxWidthAngle = DirectX::XMConvertToRadians(10.0f);
			static constexpr float maxOutLowAngle = DirectX::XMConvertToRadians(2.0f);
            float widthAngleOffset =
                (inAmount * lowFactor) * maxWidthAngle        // インロー → 下向き
                - (inAmount * highFactor) * maxWidthAngle        // インハイ → 上向き
                - (outAmount * highFactor) * maxWidthAngle        // アウトハイ → 上向き
                - (outAmount * lowFactor) * maxOutLowAngle;       // アウトロー → やや下向き

            armAngleOffset += widthAngleOffset;

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
    if (currentBatter && new_index >= 0 && new_index < currentBatter->animations.size())
    {
        current_animation_index = new_index;
        animation_time = 0.0f;  // アニメーション時間をリセット
    }

    // 構えに戻った際、またはスイングを開始した際に当たり判定を復活させる
    if (newState == State::BattingIdle)
    {
        if (Pitcher::Instance().GetCurrentState() != Pitcher::State::Throwing)
        {
           
            currentSwingTiming = SwingTiming::None;
        }

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
    if (IsRightBatter())
    {
        // 右打者：右腕がメイン
        int rightArmIndex = currentBatter->GetNodeIndex("mixamorig:RightArm");
        if (rightArmIndex < 0) return;
        DirectX::XMMATRIX additionalRotation = DirectX::XMMatrixRotationX(armAngleOffset);
        UpdateNodeTransform(rightArmIndex, additionalRotation);
        UpdateChildrenRecursive(rightArmIndex);

        int leftShoulderIndex = currentBatter->GetNodeIndex("mixamorig:LeftShoulder");
        if (leftShoulderIndex < 0) return;
        additionalRotation = DirectX::XMMatrixRotationY(armAngleOffset);
        UpdateNodeTransform(leftShoulderIndex, additionalRotation);
        UpdateChildrenRecursive(leftShoulderIndex);
    }
    else
    {
        // 左打者：左腕がメイン
        int leftArmIndex = currentBatter->GetNodeIndex("mixamorig:LeftArm");
        if (leftArmIndex < 0) return;
        DirectX::XMMATRIX additionalRotation = DirectX::XMMatrixRotationX(armAngleOffset);
        UpdateNodeTransform(leftArmIndex, additionalRotation);
        UpdateChildrenRecursive(leftArmIndex);

        int rightShoulderIndex = currentBatter->GetNodeIndex("mixamorig:RightShoulder");
        if (rightShoulderIndex < 0) return;
        additionalRotation = DirectX::XMMatrixRotationY(-armAngleOffset);
        UpdateNodeTransform(rightShoulderIndex, additionalRotation);
        UpdateChildrenRecursive(rightShoulderIndex);
    }
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

bool Player::GetRealBatterArsenalData(RealBatter rb, std::vector<RealArsenalInfo>& outArsenal, bool& outIsRight, const char*& outName)
{
    outArsenal.clear();
    outIsRight = true;
    outName = "";

    switch (rb)
    {
    case RealBatter::Ishiyama:
        outName = u8"石山";
		outIsRight = true;
        outArsenal =
        {
            { 52 , 44 } // power（威力）, contact（ミート）

        };
        return true;

    case RealBatter::Sato:
		outName = u8"佐藤";
        outIsRight = false;
        outArsenal =
        {
            { 57 , 52 } // power（威力）, contact（ミート）
		};
		return true;

    case RealBatter::Okamura:
		outName = u8"岡村";
		outIsRight = true;
        outArsenal =
        {
            { 57 , 51 } // power（威力）, contact（ミート）
		};
        return true;

    case RealBatter::Sakamoto:
		outName = u8"坂本";
		outIsRight = true;
        outArsenal =
        {
            { 49 , 48 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Odakura:
		outName = u8"小田倉";
        outIsRight = true;
        outArsenal =
        {
            { 53 , 45 } // power（威力）, contact（ミート）
		};
		return true;

    case RealBatter::Kawano:
		outName = u8"川野";
		outIsRight = false;
        outArsenal =
        {
            { 52 , 36 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Murakami:
		outName = u8"村上";
        outIsRight = false;
        outArsenal =
        {
            { 60 , 48 } // power（威力）, contact（ミート）
		};
		return true;

    case RealBatter::Yamada:
		outName = u8"山田";
        outIsRight = true;
        outArsenal =
        {
            { 51 , 43 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Suzuki:
        outName =  u8"鈴木";
        outIsRight = true;
        outArsenal =
        {
			{ 58 , 55 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Asano:
        outName = u8"浅野";
        outIsRight = false;
        outArsenal =
        {
            { 46 , 49 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Kimura:
        outName = u8"木村";
        outIsRight = true;
        outArsenal =
        {
            { 57 , 42 } // power（威力）, contact（ミート）
		};
		return true;

    case RealBatter::Matsuda:
        outName = u8"松田";
        outIsRight = false;
        outArsenal =
        {
            { 46 , 47 } // power（威力）, contact（ミート）
		};
        return true;

    case RealBatter::Murai:
        outName = u8"村井";
        outIsRight = false;
        outArsenal =
        {
            { 53 , 48 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Sasaki:
        outName = u8"佐々木";
        outIsRight = true;
        outArsenal =
        {
            { 56 , 34 } // power（威力）, contact（ミート）
		};
		return true;

    case RealBatter::Takeda:
        outName = u8"武田";
        outIsRight = false;
        outArsenal =
        {
            { 63 , 55 } // power（威力）, contact（ミート）
		};
        return true;

    case RealBatter::Hasegawa:
        outName = u8"長谷川";
        outIsRight = true;
        outArsenal =
        {
           { 55 , 41 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Nishimura:
        outName = u8"西村";
        outIsRight = false;
        outArsenal =
        {
            { 50 , 50 } // power（威力）, contact（ミート）
		};
        return true;

    case RealBatter::Kojima:
        outName = u8"小島";
        outIsRight = true;
        outArsenal =
        {
            { 55 , 51 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Nishino:
        outName = u8"西野";
        outIsRight = true;
        outArsenal =
        {
            { 51 , 40 } // power（威力）, contact（ミート）
		};
        return true;

    case RealBatter::Tamura:
        outName = u8"田村";
        outIsRight = false;
        outArsenal =
        {
			{ 51 , 39 } // power（威力）, contact（ミート）
		};
		return true;

    case RealBatter::Shimizu:
        outName = u8"清水";
        outIsRight = true;
        outArsenal =
        {
            { 52 , 37 } // power（威力）, contact（ミート）
		};
        return true;

    case RealBatter::Yamashita:
        outName = u8"山下";
        outIsRight = false;
        outArsenal =
        {
			{ 53 , 47 } // power（威力）, contact（ミート）
        };
		return true;

    case RealBatter::Nakamura:
        outName = u8"中村";
		outIsRight = true;
        outArsenal =
        {
            { 46 , 37 } // power（威力）, contact（ミート）
		};
        return true;

    case RealBatter::Ueda:
        outName = u8"上田";
        outIsRight = true;
        outArsenal =
        {
			{ 50 , 50 } // power（威力）, contact（ミート）
		};
        return true;

    default:
	    return false; // 未知の打者
    }

}


const char* Player::GetRealBatterName(RealBatter rb)
{
    std::vector<RealArsenalInfo> arsenal;
    bool isRight;
    const char* name;
    if (GetRealBatterArsenalData(rb, arsenal, isRight, name))
    {
        return name;
    }
    return "Unknown";
}

void Player::SelectRealBatter(RealBatter rb)
{
	selectedRealBatter = rb;
    if(rb == RealBatter::None)
    {
        realBatterInfo.clear();
        if (consoleLog)
        {
            consoleLog->push_back(u8"[Info] 実在打者プリセットを解除しました\n");
        }
        return;
	}

	std::vector<RealArsenalInfo> arsenal;
    bool isRight = true;
    const char* name = "";
    //実在打者のアーセナルデータを取得
    if (!GetRealBatterArsenalData(rb, arsenal, isRight, name))
    {
        selectedRealBatter = RealBatter::None;
        realBatterInfo.clear();
        return;
    }

	//利き手を設定
	realBatterInfo = arsenal;

    BatSprite::Instance().UpdateCursorSizeByContact(GetSelectedRealBatterContact());

	//利き手を設定
    if (isRightBatter != isRight)
    {
		isRightBatter = isRight;
		UpdateBatterModel();
    }

	//コンソールログに出力
    if (consoleLog)
    {
        consoleLog->push_back(u8"[Info] 実在打者プリセットを選択: ");
        consoleLog->push_back(name);
        consoleLog->push_back(u8"\n");
	}
}

void Player::SaveToJson(json& j)
{
    // 基本的なプロパティを保存
    j["position"] = { position.x, position.y, position.z };
    j["scale"] = { scale.x, scale.y, scale.z };
    j["angle"] = { angle.x, angle.y, angle.z };
    j["isRightBatter"] = isRightBatter;
    j["batPosition"] = { batPosition.x, batPosition.y, batPosition.z };
    j["batScale"] = { batScale.x, batScale.y, batScale.z };
    j["batAngle"] = { batAngle.x, batAngle.y, batAngle.z };
    j["meshScale"] = { meshScale.x, meshScale.y, meshScale.z };

	HitJudge2D::Instance().SaveToJson(j["HitJudge2D"]); // HitJudge2Dの状態も保存

	j["selectedRealBatter"] = static_cast<int>(selectedRealBatter); // 選択された実在打者の情報を保存
}

void Player::LoadFromJson(const json& j)
{
    // 基本的なプロパティを読み込む
    if (j.contains("position"))   position = { j["position"][0], j["position"][1], j["position"][2] };
    if (j.contains("scale"))      scale = { j["scale"][0], j["scale"][1], j["scale"][2] };
    if (j.contains("angle"))      angle = { j["angle"][0], j["angle"][1], j["angle"][2] };
    if (j.contains("batPosition")) batPosition = { j["batPosition"][0], j["batPosition"][1], j["batPosition"][2] };
    if (j.contains("batScale"))    batScale = { j["batScale"][0], j["batScale"][1], j["batScale"][2] };
    if (j.contains("batAngle"))    batAngle = { j["batAngle"][0], j["batAngle"][1], j["batAngle"][2] };
    if (j.contains("meshScale")) { meshScale = { j["meshScale"][0], j["meshScale"][1], j["meshScale"][2] }; UpdatePhysXMeshTransform(meshScale); }
  
    //利き手が変わっていれば再初期化
    if (j.contains("isRightBatter") && (bool)j["isRightBatter"] != isRightBatter)
    {
		UpdateBatterModel();
    }

	HitJudge2D::Instance().LoadFromJson(j["HitJudge2D"]); // HitJudge2Dの状態も読み込む

    // 選択された実在打者の情報を読み込む
    /*if (j.contains("selectedRealBatter"))
    {
        int rbIndex = j["selectedRealBatter"];
        if (rbIndex >= 0 && rbIndex < static_cast<int>(RealBatter::Count))
        {
            SelectRealBatter(static_cast<RealBatter>(rbIndex));
        }
        else
        {
            SelectRealBatter(RealBatter::None);
        }
    }*/

	SelectRealBatter(selectedRealBatter); // 選択された実在打者の情報を反映
}