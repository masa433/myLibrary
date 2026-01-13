#include "../pch.h"
#include "Boss.h"
#include "MathUtils.h"
#include "Player.h"
#include "ProjectileStraight.h"


//コンストラクタ
Boss::Boss() :health(20)
{
	model = new Model("Data/Model/boss/Boss.mdl");

	//モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.03f;

	//幅、高さ設定
	radius = 8.0f;
	height = 17.0f;

	//徘徊ステートへ遷移
	SetIdleState();
}

Boss::~Boss()
{
	delete model;
}

//更新処理
void Boss::Update(float elapsedTime)
{
	//ステート毎の更新処理
	switch (state)
	{
	case State::Wander:
		UpdateWanderState(elapsedTime);
		break;

	case State::Idle:
		UpdateIdleState(elapsedTime);
		break;

	case State::Chase:
		UpdateChaseState(elapsedTime);
		break;

	case State::Attack:
		UpdateAttackState(elapsedTime);
		break;
	}


	//速力処理更新
	UpdateVelocity(elapsedTime);

	//弾丸更新処理
	projectileManager.Update(elapsedTime);

	//無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	//オブジェクト行列を更新
	UpdateTransform();

	//モデル行列を更新
	model->UpdateTransform();


}

void Boss::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::ShadowMap);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

//newで実体を生成したら自分で責任をもってdeleteで削除(ヒープ領域を解放)しなけらばならない

bool Boss::ApplyDamage(int damage, float knockbackStrength)
{
	// ダメージを適用
	health -= damage;

	// 体力が0以下になったら死亡処理を実行
	if (health <= 0) {
		OnDead(); // 死亡処理
		return true; // ボスが死んだことを示す
	}

	return false; // ボスがまだ生きている
}

//死亡したときに呼ばれる
void Boss::OnDead()
{
	Destroy();

}

void Boss::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	//基底クラスのもの
	Enemy::RenderDebugPrimitive(rc, renderer);

	//縄張り範囲をデバッグ円柱描画
	renderer->RenderCylinder(rc, territoryOrigin, territoryRange, 1.0f, DirectX::XMFLOAT4(0, 1, 0, 1));

	//ターゲット位置をデバッグ球描画
	renderer->RenderSphere(rc, targetPosition, 1.0f, DirectX::XMFLOAT4(1, 1, 0, 1));

	//索敵範囲をデバッグ円柱描画
	renderer->RenderCylinder(rc, position, searchRange, 1.0f, DirectX::XMFLOAT4(1, 0, 0, 1));

	//攻撃範囲
	renderer->RenderCylinder(rc, position, attackRange, 1.0f, DirectX::XMFLOAT4(0, 0, 1, 1));

	projectileManager.RenderDebugPrimitive(rc, renderer);
}

//縄張り設定
void Boss::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;
}

//ターゲット位置をランダム設定
void Boss::SetRandomTargetPosition()
{
	float theta = MathUtils::RandomRange(-DirectX::XM_PI, DirectX::XM_PI);
	float range = MathUtils::RandomRange(0.0f, territoryRange+200.0f); // 広い範囲で位置生成
	targetPosition.x = territoryOrigin.x + sinf(theta) * range;
	targetPosition.y = territoryOrigin.y; // 固定の高さ
	targetPosition.z = territoryOrigin.z + cosf(theta) * range;
}


//目標地点へ移動
void Boss::MoveTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate)
{
	//ターゲット方向への進行ベクトルを算出
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vz * vz);
	vx /= dist;
	vz /= dist;

	//移動処理
	Move(elapsedTime, vx, vz, moveSpeed * moveSpeedRate);
	Turn(elapsedTime, vx, vz, turnSpeed * turnSpeedRate);
}



//徘徊ステートへ遷移
void Boss::SetWanderState()
{
	state = State::Wander;

	//目標地点設定
	SetRandomTargetPosition();
}

//徘徊ステート更新処理
void Boss::UpdateWanderState(float elapsedTime)
{
	//目標地点までXZ平面での距離判定
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz;
	if (distSq < radius * radius)
	{
		////次の目標地点へ移動
		//SetRandomTargetPosition();

		SetWanderState();
	}

	//目標地点へ移動
	MoveTarget(elapsedTime, 5.0f, 1.0f);


	if (SearchPlayer())
	{
		SetChaseState();
	}
}

void Boss::SetIdleState()
{
	state = State::Idle;
	stateTimer = MathUtils::RandomRange(3.0f, 5.0f);
}

void Boss::UpdateIdleState(float elapsedTime)
{
	stateTimer -= elapsedTime;
	if (stateTimer <= 0.0f)
	{
		//徘徊ステートへ遷移
		SetWanderState();
	}
	if (SearchPlayer())
	{
		//追跡ステートへ遷移
		SetChaseState();
	}
}

bool Boss::SearchPlayer()
{
	//プレイヤーとの高低差を考慮して3Dでの距離判定をする
	const DirectX::XMFLOAT3& playerPosition = Player::Instance().GetPosition();
	float vx = playerPosition.x - position.x;
	float vy = playerPosition.y - position.y;
	float vz = playerPosition.z - position.z;
	float dist = sqrtf(vx * vx + vy * vy + vz * vz);
	if (dist < searchRange)
	{
		float distXZ = sqrtf(vx * vx + vz * vz);
		//単位ベクトル化
		vx /= distXZ;
		vz /= distXZ;
		//前方ベクトル
		float frontX = sinf(angle.y);
		float frontZ = cosf(angle.y);

		//2つのベクトルの内積値で前後判定
		float dot = (frontX * vx) + (frontZ * vz);
		if (dot > 0.0f)
		{
			return true;
		}
	}
	return false;
}

void Boss::SetChaseState()
{
	state = State::Chase;
	stateTimer = MathUtils::RandomRange(3.0f, 5.0f);
}

void Boss::UpdateChaseState(float elapsedTime)
{
	// 目標地点をプレイヤー位置に設定
	targetPosition = Player::Instance().GetPosition();

	// 目的地点へ移動
	MoveTarget(elapsedTime, 5.0f, 1.0f);

	// タイマー処理
	stateTimer -= elapsedTime;


	// 追跡時間が経過したとき待機ステートへ遷移しなさい
	if (stateTimer < 0)
	{
		SetIdleState();
	}



	float vx = targetPosition.x - position.x;
	float vy = targetPosition.y - position.y;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vy * vy + vz * vz);

	
	if (dist < attackRange)
	{
		SetAttackState();
	}
}

void Boss::SetAttackState()
{
	state = State::Attack;
	stateTimer = 0.0f;
}

void Boss::UpdateAttackState(float elapsedTime)
{
	// プレイヤーの位置を取得
	targetPosition = Player::Instance().GetPosition();

	// プレイヤーに向かって突進
	DirectX::XMFLOAT3 direction;
	direction.x = targetPosition.x - position.x;
	direction.y = 0.0f; // Y軸の移動はなし
	direction.z = targetPosition.z - position.z;

	// 方向ベクトルを正規化
	DirectX::XMVECTOR dirVec = DirectX::XMLoadFloat3(&direction);
	if (DirectX::XMVector3Length(dirVec).m128_f32[0] > 0.0f)
	{
		dirVec = DirectX::XMVector3Normalize(dirVec);
	}
	else
	{
		dirVec = DirectX::XMVectorZero();
	}
	DirectX::XMFLOAT3 normalizedDirection;
	DirectX::XMStoreFloat3(&normalizedDirection, dirVec);

	// 突進速度で位置を更新
	const float dashSpeed = 5.0f;
	position.x += normalizedDirection.x * dashSpeed * elapsedTime;
	position.z += normalizedDirection.z * dashSpeed * elapsedTime;

	// タイマー更新
	stateTimer -= elapsedTime;

	// タイマーが終了したら次の状態へ
	if (stateTimer <= 0.0f)
	{
		SetIdleState();

		// 弾の方向を下向きに設定
		DirectX::XMFLOAT3 projectileDirection;
		projectileDirection.x = targetPosition.x - position.x;    
		projectileDirection.y = -4.0f;  // 下方向
		projectileDirection.z = targetPosition.z - position.z;

		// 発射位置
		DirectX::XMFLOAT3 launchPosition;
		launchPosition.x = position.x;
		launchPosition.y = position.y + height * 0.8f; // ボスの高さの80%に調整
		launchPosition.z = position.z;

		// 弾を発射
		ProjectileStraight* projectile = new ProjectileStraight(&projectileManager);
		projectile->Launch(projectileDirection, launchPosition);

		// 次の状態までの時間を設定
		stateTimer = 2.0f;
	}

	// プレイヤーを見失ったらアイドル状態に戻る
	if (!SearchPlayer())
	{
		SetIdleState();
	}
}






