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

	void ModifyLeftArmBone();

	void ModifyRightArmBone();

	void UpdateNodeGlobal(Model::Node& node);

	void UpdateChildrenGlobal(Model::Node& node);

protected:
	//着地したときに呼ばれる
	void OnLanding() override;

private:
	enum class State 
	{
		BatIdle,
		BatSwing,
		BatSwingReverse,//逆再生用
	};
	
	enum Animation 
	{
		BattingIdle,
		Homerun,
		Swing,
	};

	State state = State::BatIdle;
	
	void SetSwingState();

	void UpdateSwingState(float elapsedTime);

	void SetBattingIdleState();

	void UpdateBattingIdleState(float elapsedTime);

	void UpdateSwingReverseState(float elapsedTime);

private:

	Model* model = nullptr;

	std::unique_ptr<Model> bat = nullptr;

	// バット専用のトランスフォーム情報
	DirectX::XMFLOAT3 batPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 batAngle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 batScale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4X4 batTransform = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};

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

	float swingHeight = 0.5f; // スイングの高さ（0.0～1.0）
	float armAngleOffset = 0.0f; // 腕の角度オフセット（追加）
	bool isSwingForward = true; // スイングが通常再生か逆再生か
	float swingStartTime = 0.0f; // スイング開始時間
	const float swingDuration = 0.6f; // スイングアニメーションの総時間
};