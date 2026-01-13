#pragma once

#include"System/Model.h"
#include "Enemy.h"
#include "ProjectileManager.h"

//スライム
class EnemySlime : public Enemy //エネミークラスを継承
{
public:
	EnemySlime();
	~EnemySlime() override;
	//継承元の仮想関数をオーバーライドをする場合はoverrideキーワードをつけること

		//更新処理
	void Update(float elapsedTime) override;

	//描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)override;

	//縄張り設定
	void SetTerritory(const DirectX::XMFLOAT3& origin, float range);

private:
	//ターゲット位置をランダム設定
	void SetRandomTargetPosition();

	//目標地点へ移動
	void MoveTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate);

	//徘徊ステートへ遷移
	void SetWanderState();

	//徘徊ステート更新処理
	void UpdateWanderState(float elapsedTime);

	//待機ステートへ遷移
	void SetIdleState();

	//待機ステート更新処理
	void UpdateIdleState(float elapsedTime);

	//プレイヤー索敵
	bool SearchPlayer();

	//攻撃ステートへ遷移
	void SetAttackState();

	//攻撃ステート更新処理
	void UpdateAttackState(float elapsedTime);

	//追跡ステートへ遷移
	void SetChaseState();

	//追跡ステート更新処理
	void UpdateChaseState(float elapsedTime);

private:
	enum class State
	{
		Wander,
		Idle,
		Chase,
		Attack
	};


protected:
	void OnDead() override;

private:
	Model* model = nullptr;

	State state = State::Wander;
	DirectX::XMFLOAT3 targetPosition = { 0,0,0 };
	DirectX::XMFLOAT3 territoryOrigin = { 0,0,0 };
	float territoryRange = 10.0f;
	float moveSpeed = 2.0f;
	float turnSpeed = DirectX::XMConvertToRadians(360);

	float stateTimer = 0.0f;

	float searchRange = 40.0f;
	float attackRange = 40.0f;
	
};