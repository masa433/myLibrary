#include "../pch.h"
#include "EnemyEgg.h"
#include "MathUtils.h"
#include "Player.h"
#include "ProjectileStraight.h"


//コンストラクタ
EnemyEgg::EnemyEgg()
{
	model = new Model("Data/Model/enemy1/robot.mdl");

	//モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.03f;

	//幅、高さ設定
	radius = 3.5f;
	height = 5.0f;

	//徘徊ステートへ遷移
	SetIdleState();
}

EnemyEgg::~EnemyEgg()
{
	delete model;
}

//更新処理
void EnemyEgg::Update(float elapsedTime)
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

void EnemyEgg::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::ShadowMap);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

//newで実体を生成したら自分で責任をもってdeleteで削除(ヒープ領域を解放)しなけらばならない

//死亡したときに呼ばれる
void EnemyEgg::OnDead()
{
	Destroy();

}

void EnemyEgg::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
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
void EnemyEgg::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;
}

//ターゲット位置をランダム設定
void EnemyEgg::SetRandomTargetPosition()
{
	float theta = MathUtils::RandomRange(-DirectX::XM_PI, DirectX::XM_PI);
	float range = MathUtils::RandomRange(0.0f, territoryRange+200.0f);
	targetPosition.x = territoryOrigin.x + sinf(theta) * range;
	targetPosition.y = territoryOrigin.y;
	targetPosition.z = territoryOrigin.z + cosf(theta) * range;
}

//目標地点へ移動
void EnemyEgg::MoveTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate)
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
void EnemyEgg::SetWanderState()
{
	state = State::Wander;

	//目標地点設定
	SetRandomTargetPosition();
}

//徘徊ステート更新処理
void EnemyEgg::UpdateWanderState(float elapsedTime)
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

void EnemyEgg::SetIdleState()
{
	state = State::Idle;
	stateTimer = MathUtils::RandomRange(3.0f, 5.0f);
}

void EnemyEgg::UpdateIdleState(float elapsedTime)
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

bool EnemyEgg::SearchPlayer()
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

void EnemyEgg::SetChaseState()
{
	state = State::Chase;
	stateTimer = MathUtils::RandomRange(3.0f, 5.0f);
}

void EnemyEgg::UpdateChaseState(float elapsedTime)
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

	// TODO 01_03
	// 攻撃範囲に入ったとき攻撃ステートへ遷移しなさい
	if (dist < attackRange)
	{
		SetAttackState();
	}
}

void EnemyEgg::SetAttackState()
{
	state = State::Attack;
	stateTimer = 0.0f;
}

void EnemyEgg::UpdateAttackState(float elapsedTime)
{
	//目標地点をプレイヤー位置に設定
	targetPosition = Player::Instance().GetPosition();

	//目標地点へ移動
	MoveTarget(elapsedTime, 5.0f, 1.0f);

	//タイマー処理
	stateTimer -= elapsedTime;
	if (stateTimer < 0.0f)
	{
		//前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);
		//発射位置
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;
		//発射
		ProjectileStraight* projectile = new ProjectileStraight(&projectileManager);
		projectile->Launch(dir, pos);
		stateTimer = 2.0f;
	}

	//プレイヤーを見失ったら
	if (!SearchPlayer())
	{
		SetIdleState();
	}
}

