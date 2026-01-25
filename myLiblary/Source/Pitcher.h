#pragma once

#include"System/ModelRenderer.h"
#include"Character.h"


class Pitcher :public Character
{
public:
	Pitcher() {};
	~Pitcher() override {};

	static Pitcher& Instance()
	{
		static Pitcher instance;
		return instance;
	}

	void Initialize();

	void Finalize();

	//更新処理
	void Update(float elapsedTime);

	//描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	void DrawImGui();

private:

	enum class State
	{
		Throwing,
	};

	enum Animation
	{
		Pitching,
		EnumCount
	};

	void SetPitchingState();

	void UpdatePitchingState(float elapsedTime);

	State state = State::Throwing;

private:

	std::unique_ptr<Model>	pitcher = nullptr;

	std::unique_ptr<Model> ball = nullptr;

	// ボール専用のトランスフォーム情報
	DirectX::XMFLOAT3 ballPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 ballAngle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 ballScale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4X4 ballTransform = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	// ボール専用のトランスフォーム情報
	DirectX::XMFLOAT3 ballWorldPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 ballWorldAngle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 ballWorldScale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4X4 ballWorldTransform = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};

	DirectX::XMFLOAT3 ballOriginOffset;//原点調整

	DirectX::XMFLOAT3 ballVelocity = { 0.0f, 0.0f, 0.0f }; // ボールの速度
	float throwTiming = 0.7f; // ボールを離すタイミング（アニメーション時間の比率）
	float gravity = -9.8f; // 重力加速度
	bool isBallThrown = false; // ボールが投げられたか
};
