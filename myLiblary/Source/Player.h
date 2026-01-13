#pragma once

#include"System/ModelRenderer.h"
#include"Character.h"
#include "ProjectileManager.h"
#include "System/AudioSource.h"
#include "Effect.h"

//プレイヤー
class Player :public Character 
{
private:
	Player() : health(5){};
	~Player() override {};

public:

	static Player& Instance() 
	{
		static Player instance;
		return instance;
	}

	void Initialize();

	void Finalize();

	//更新処理
	void Update(float elapsedTime);

	//描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	void DrawDebugGUI();

	void ShowControlPanel();

	

	//ジャンプ入力処理
	void InputJump();

	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

private:
	//スティック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec() const;

	
	//移動入力処理
	void InputMove(float elapsedTime);

	

	//プレイヤーとエネミーとの衝突処理
	void CollisionPlayerVsEnemies();

	//弾丸入力処理
	void InputProjectile();

	//弾丸と敵の衝突処理
	void CollisionProjectilesVsEnemies();

	void CollisionEnemiesProjectilesVsPlayer();

	void ApplyDamage(int damage, float invincibleTime);

	void Die();

protected:
	//着地したときに呼ばれる
	void OnLanding() override;

private:
	enum class State 
	{
		BatSwing,
		BatIdle,
	};
	
	enum Animation 
	{
		BattingIdle,
		Swing,
	};

	State state = State::BatIdle;
	
	void SetSwingState();

	void UpdateSwingState(float elapsedTime);

	void SetBattingIdleState();

	void UpdateBattingIdleState(float elapsedTime);

private:

	Model* model = nullptr;

	float moveSpeed = 20.0f;

	float turnSpeed = DirectX::XMConvertToRadians(720);

	float jumpSpeed = 20.0f;

	int jumpCount = 0;

	int jumpLimit = 2;

	float deathTimer = 1.0f;

	ProjectileManager projectileManager;

	AudioSource* hitSE = nullptr;

	Effect* hitEffect = nullptr;

	int health;  // プレイヤーの体力
	bool isDead = false;  // プレイヤーが死亡したかどうか
};