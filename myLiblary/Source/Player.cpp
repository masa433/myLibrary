#include "pch.h"
#include "Player.h"
#include "Camera.h"
#include "System/Input.h"
#include "EnemyManager.h"
#include "Collision.h"
#include "ProjectileStraight.h"
#include "ProjectileHoming.h"
#include "Object.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include <System/Graphics.h>
#include <ImGuizmo.h>


//初期化
void Player::Initialize() 
{
	model = new Model("Data/Model/player/Bot.mdl");

	//モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.03f;
	radius = 4.0f;
	height = 9.0f;
	angle.y = DirectX::XMConvertToRadians(180.0f);
	
	bat = std::make_unique<Model>("Data/Model/bat/bat.gltf");
	batScale = { 1.2f,1.2f,1.2f };
	batPosition = { -8.0f, 0.0f, 4.0f };
	batAngle = { 0.0f, 0.0f, 17.2f };

	//AudioManager::Instance().GetSound(SoundList::GameBGM)->Play(false, 1.0f);

	hitEffect = new Effect("Data/Effect/Hit.efk");
	health = 5;
	position =DirectX::XMFLOAT3(3.6f, 0.0f, 66.0f);
	velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	isDead = false;

	SetBattingIdleState();

	//頭ノード取得
	const char* headName = "mixamorig:Head";
	Model::Node* headNode = model->FindNode(headName);

	//ローカル前方ベクトル計算
	{
		DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&headNode->globalTransform);
		DirectX::XMMATRIX InverseWorldTransform = DirectX::XMMatrixInverse(nullptr, WorldTransform);
		DirectX::XMVECTOR HeadWorldForward = DirectX::XMVectorSet(0, 0, 1, 0);
		DirectX::XMVECTOR HeadLocalForward = DirectX::XMVector3TransformNormal(HeadWorldForward, InverseWorldTransform);

		HeadLocalForward = DirectX::XMVector3Normalize(HeadLocalForward);
		DirectX::XMStoreFloat3(&headLocalForward, HeadLocalForward);

	}

	// ターゲット位置
	targetPosition = { 0, 2, 1 };

}

//終了化
void Player::Finalize()
{
	// モデルが存在すれば解放
	if (model)
	{
		delete model;
		model = nullptr;
	}

	// 音声やエフェクトの解放
	/*if (hitSE)
	{
		delete hitSE;
		hitSE = nullptr;
	}*/

	if (hitEffect)
	{
		delete hitEffect;
		hitEffect = nullptr;
	}
}


//更新処理
void Player::Update(float elapsedTime) 
{

	if (isDead) return;  // 死亡している場合は更新を行わない

	
	switch (state)
	{
	case State::BatIdle:
		UpdateBattingIdleState(elapsedTime);
		break;
	case State::BatSwing:
		UpdateSwingState(elapsedTime);
		break;
	case State::BatSwingReverse:
		UpdateSwingReverseState(elapsedTime);
		break;
	default:
		break;
	}


	//移動入力処理
	InputMove(elapsedTime);

	//ジャンプ入力処理
	InputJump();

	//速力処理更新
	UpdateVelocity(elapsedTime);

	//オブジェクト行列を更新
	UpdateTransform();

	//モデル行列更新
	model->UpdateTransform();

	bat->UpdateTransform();

	model->UpdateAnimation(elapsedTime);

	ModifyLeftArmBone();
	ModifyRightArmBone();

	const char* handName = "mixamorig:LeftHandMiddle1";

	// バットのローカル行列を計算（バット専用の変数を使用）
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(batScale.x, batScale.y, batScale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(batAngle.x, batAngle.y, batAngle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(batPosition.x, batPosition.y, batPosition.z);
	DirectX::XMMATRIX batLocalMatrix = S * R * T;

	for (const Model::Node& node : model->GetNodes())
	{
		if (strcmp(node.name, handName) == 0)
		{
			// 左手ノードの行列を取得
			DirectX::XMMATRIX leftHandMatrix = DirectX::XMLoadFloat4x4(&node.globalTransform);

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

	//頭ノード取得
	const char* headName = "mixamorig:Head";
	Model::Node* headNode = model->FindNode(headName);

	// 頭がターゲット位置を正面に捉えるように回転させる
	{
		//ターゲットまでのベクトルをローカル空間に変換
		DirectX::XMMATRIX HeadWorldTransform = DirectX::XMLoadFloat4x4(&headNode->globalTransform);
		DirectX::XMMATRIX InverseHeadWorldTransform = DirectX::XMMatrixInverse(nullptr, HeadWorldTransform);
		DirectX::XMVECTOR TargetWorldPosition = DirectX::XMVectorSet(targetPosition.x, targetPosition.y, targetPosition.z, 1);
		DirectX::XMVECTOR TargetLocalPosition = DirectX::XMVector3TransformCoord(TargetWorldPosition, InverseHeadWorldTransform);
		DirectX::XMVECTOR HeadToTargetLocal = DirectX::XMVectorSubtract(TargetLocalPosition, DirectX::XMVectorSet(0, 0, 0, 1));
		HeadToTargetLocal = DirectX::XMVector3Normalize(HeadToTargetLocal);


		// ローカル空間での回転軸と回転角を求める
		DirectX::XMVECTOR HeadLocalForwardVec = DirectX::XMLoadFloat3(&headLocalForward);
		DirectX::XMVECTOR RotationAxis = DirectX::XMVector3Cross(HeadLocalForwardVec, HeadToTargetLocal);
		RotationAxis = DirectX::XMVector3Normalize(RotationAxis);
		DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(HeadLocalForwardVec, HeadToTargetLocal);
		float RotationAngle = acosf(DirectX::XMVectorGetX(Dot));

		// 回転を表すクォータニオンを作成
		DirectX::XMVECTOR SinHalfAngle = DirectX::XMVectorSet(sinf(RotationAngle * 0.5f), sinf(RotationAngle * 0.5f), sinf(RotationAngle * 0.5f), sinf(RotationAngle * 0.5f));
		DirectX::XMVECTOR CosHalfAngle = DirectX::XMVectorSet(cosf(RotationAngle * 0.5f), cosf(RotationAngle * 0.5f), cosf(RotationAngle * 0.5f), cosf(RotationAngle * 0.5f));
		DirectX::XMVECTOR RotationQuat = DirectX::XMVectorSet(
			DirectX::XMVectorGetX(RotationAxis) * DirectX::XMVectorGetX(SinHalfAngle),
			DirectX::XMVectorGetY(RotationAxis) * DirectX::XMVectorGetY(SinHalfAngle),
			DirectX::XMVectorGetZ(RotationAxis) * DirectX::XMVectorGetZ(SinHalfAngle),
			DirectX::XMVectorGetX(CosHalfAngle)
		);
		// 頭ノードの回転に合成
		DirectX::XMVECTOR HeadRotation = DirectX::XMLoadFloat4(&headNode->rotate);
		DirectX::XMVECTOR NewHeadRotation = DirectX::XMQuaternionMultiply(RotationQuat, HeadRotation);
		DirectX::XMStoreFloat4(&headNode->rotate, NewHeadRotation);

		// ワールド行列更新
		
		UpdateNodeGlobal(*headNode);


	}

}

void Player::ModifyLeftArmBone()
{
	const char* shoulderName = "mixamorig:LeftArm";

	for (Model::Node& node : model->GetNodes())
	{
		if (strcmp(node.name, shoulderName) == 0)
		{
			// ローカル行列を取得
			DirectX::XMMATRIX localMatrix = DirectX::XMLoadFloat4x4(&node.localTransform);

			// X軸回転行列を作成（腕を上下に動かす）
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationX(armAngleOffset);

			// 回転を適用
			DirectX::XMMATRIX newLocalMatrix = rotationMatrix * localMatrix;

			// 保存
			DirectX::XMStoreFloat4x4(&node.localTransform, newLocalMatrix);

			// グローバル行列を再計算
			UpdateNodeGlobal(node);

			// 子ボーンも再帰的に更新
			UpdateChildrenGlobal(node);

			break;
		}
	}
}

void Player::ModifyRightArmBone()
{
	const char* shoulderName = "mixamorig:RightShoulder";

	for (Model::Node& node : model->GetNodes())
	{
		if (strcmp(node.name, shoulderName) == 0)
		{
			// ローカル行列を取得
			DirectX::XMMATRIX localMatrix = DirectX::XMLoadFloat4x4(&node.localTransform);

			// X軸回転行列を作成（腕を上下に動かす）
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationX(armAngleOffset);

			// 回転を適用
			DirectX::XMMATRIX newLocalMatrix = rotationMatrix * localMatrix;

			// 保存
			DirectX::XMStoreFloat4x4(&node.localTransform, newLocalMatrix);

			// グローバル行列を再計算
			UpdateNodeGlobal(node);

			// 子ボーンも再帰的に更新
			UpdateChildrenGlobal(node);

			break;
		}
	}
}

void Player::UpdateNodeGlobal(Model::Node& node)
{
	DirectX::XMMATRIX localMatrix = DirectX::XMLoadFloat4x4(&node.localTransform);

	if (node.parent != nullptr)
	{
		DirectX::XMMATRIX parentGlobal = DirectX::XMLoadFloat4x4(&node.parent->globalTransform);
		DirectX::XMMATRIX globalMatrix = localMatrix * parentGlobal;
		DirectX::XMStoreFloat4x4(&node.globalTransform, globalMatrix);
	}
	else
	{
		DirectX::XMStoreFloat4x4(&node.globalTransform, localMatrix);
	}
}

void Player::UpdateChildrenGlobal(Model::Node& node)
{
	for (Model::Node* child : node.children)
	{
		UpdateNodeGlobal(*child);
		UpdateChildrenGlobal(*child); // 再帰的に子の子も更新
	}
}

void Player::OnLanding() 
{ 
	 jumpCount = 0; 
}


//移動入力処理
void Player::InputMove(float elapsedTime) 
{
	//進行ベクトル取得
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	//移動処理
	Move(elapsedTime, moveVec.x, moveVec.z, moveSpeed);

	//旋回処理
	Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
}


// ジャンプ入力処理
void Player::InputJump()
{
    // ボタン入力でジャンプ (ジャンプ回数制限付き)
    GamePad& gamePad = Input::Instance().GetGamePad();
    if (gamePad.GetButtonDown() & GamePad::BTN_A)
    {
        // ジャンプが可能か判定
        if (jumpCount < jumpLimit)
        {
            //Jump(jumpSpeed);
            jumpCount++; // ジャンプ回数を更新
            
        }
		
    }
}



//描画処理
void Player::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	if (model == nullptr) return;  // モデルがnullptrの場合は描画しない

	// モデル描画
	renderer->Render(rc, transform, model, ShaderId::Lambert);
	renderer->Render(rc, batTransform, bat.get(), ShaderId::Lambert);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

//デバッグプリミティブ描画
void Player::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) 
{
	//基底クラスの関数呼び出し
	Character::RenderDebugPrimitive(rc, renderer);

	//弾丸デバッグプリミティブ描画
	projectileManager.RenderDebugPrimitive(rc, renderer);
}

//デバッグ用GUI描画
void Player::DrawDebugGUI() 
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300),ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Player", nullptr, ImGuiWindowFlags_None)) 
	{

		//トランスフォーム
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) 
		{
			//位置
			ImGui::InputFloat3("Position", &position.x);
			//回転
			DirectX::XMFLOAT3 a;
			a.x = DirectX::XMConvertToDegrees(angle.x);
			a.y = DirectX::XMConvertToDegrees(angle.y);
			a.z = DirectX::XMConvertToDegrees(angle.z);
			ImGui::InputFloat3("Angle", &a.x);
			angle.x = DirectX::XMConvertToRadians(a.x);
			angle.y = DirectX::XMConvertToRadians(a.y);
			angle.z = DirectX::XMConvertToRadians(a.z);
			//スケール
			ImGui::InputFloat3("Scale", &scale.x);

			//バットのトランスフォーム
			ImGui::Separator();
			ImGui::Text("Bat Transform");
			ImGui::DragFloat3("Bat Position", &batPosition.x);
			ImGui::DragFloat3("Bat Angle", &batAngle.x);
			ImGui::DragFloat3("Bat Scale", &batScale.x);

			// スイング高さを表示（追加）
			ImGui::Separator();
			ImGui::Text("Swing Height: %.2f", swingHeight);
		}

		// ルックアット設定（追加）
		if (ImGui::CollapsingHeader("Look At Target", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat3("Target Position", &targetPosition.x, 0.1f, -100.0f, 100.0f);

			// プレイヤーの前方にターゲットを配置するボタン
			if (ImGui::Button("Set Target Front"))
			{
				targetPosition.x = position.x + sinf(angle.y) * 10.0f;
				targetPosition.y = position.y + height * 0.8f;
				targetPosition.z = position.z + cosf(angle.y) * 10.0f;
			}
		}
	}
	ImGui::End();
}

DirectX::XMFLOAT3 Player::GetMoveVec() const
{
	// 入力情報を取得
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisLX();  // 左スティックのX軸
	float ay = gamePad.GetAxisLY();  // 左スティックのY軸

	// カメラ方向とスティックの入力値によって進行方向を計算する
	Camera& camera = Camera::Instance();
	const DirectX::XMFLOAT3& cameraRight = camera.GetRight();
	const DirectX::XMFLOAT3& cameraFront = camera.GetFront();

	// 移動ベクトルはXZ平面に水平なベクトルになるようにする

	// カメラ右方向ベクトルをXZ単位ベクトルに変換
	float cameraRightX = cameraRight.x;
	float cameraRightZ = cameraRight.z;
	float cameraRightLength = sqrtf(cameraRightX * cameraRightX + cameraRightZ * cameraRightZ);
	if (cameraRightLength > 0.0f)
	{
		// 単位ベクトル化
		cameraRightX /= cameraRightLength;
		cameraRightZ /= cameraRightLength;
	}

	// カメラ前方向ベクトルをXZ単位ベクトルに変換
	float cameraFrontX = cameraFront.x;
	float cameraFrontZ = cameraFront.z;
	float cameraFrontLength = sqrtf(cameraFrontX * cameraFrontX + cameraFrontZ * cameraFrontZ);
	if (cameraFrontLength > 0.0f)
	{
		// 単位ベクトル化
		cameraFrontX /= cameraFrontLength;
		cameraFrontZ /= cameraFrontLength;
	}

	// スティックの水平入力値をカメラ右方向に反映し
	// スティックの垂直入力値をカメラ前方向に反映し
	// 進行ベクトルを計算する
	DirectX::XMFLOAT3 vec;
	// スティックの水平入力 (ax) はカメラの右方向の反対に、垂直入力 (ay) はカメラの前方向に反映させる
	vec.x = (cameraRightX * ax) + (cameraFrontX * ay); // Aキーで左に動かすために右方向を反転
	vec.z = (cameraRightZ * ax) + (cameraFrontZ * ay); // 同様にZ方向も反転
	// Y軸方向には移動しない
	vec.y = 0.0f;

	return vec;


}


//アニメーション関連
void Player::SetBattingIdleState() 
{
	state = State::BatIdle;
	model->PlayAnimation(BattingIdle, true);
}

void Player::UpdateBattingIdleState(float elapsedTime)
{
	// Zキーでスイング
	if (GetAsyncKeyState('Z') & 0x8000)
	{
		SetSwingState();
	}
}

void Player::SetSwingState() 
{
	state = State::BatSwing;
	isSwingForward = true;
	swingStartTime = 0.0f;
	model->SetAnimationSpeed(1.3f);
	model->PlayAnimation(Swing, false);
}

void Player::UpdateSwingState(float elapsedTime)
{
	// マウスカーソルの位置を取得
	Mouse& mouse = Input::Instance().GetMouse();
	float mouseY = mouse.GetPositionY();

	Graphics& graphics = Graphics::Instance();
	float screenHeight = static_cast<float>(graphics.GetScreenHeight());

	// マウスY座標を正規化（0.0～1.0）
	swingHeight = mouseY / screenHeight;
	swingHeight = std::clamp(swingHeight, 0.0f, 1.0f);

	// 腕の角度オフセットを計算（-45°〜+45°の範囲）
	armAngleOffset = DirectX::XMConvertToRadians(-45.0f + swingHeight * 90.0f);

	// スイング開始からの経過時間を更新
	swingStartTime += elapsedTime;

	if (swingStartTime >= swingDuration - 0.4f) 
	{
		model->SetAnimationSpeed(0.7f); // 再生速度を少し遅くする
	}

	// 順再生が終わったら逆再生を開始
	if (swingStartTime>=swingDuration)
	{
		state = State::BatSwingReverse;
		model->SetAnimationSpeed(0.7f); // 再生速度を遅くする
		model->PlayAnimation(Swing, false, 0.2f, true); // 逆再生
	}
}

void Player::UpdateSwingReverseState(float elapsedTime)
{
	// 腕の角度オフセットを維持
	// （逆再生中もマウス位置で調整したい場合は UpdateSwingState() と同じ処理）

	// 逆再生が終わったら待機状態へ
	if (!model->IsPlayAnimation())
	{
		SetBattingIdleState();
		model->SetAnimationSpeed(1.0f); // 再生速度をリセット
		armAngleOffset = 0.0f; // 腕の角度オフセットをリセット
	}
}






