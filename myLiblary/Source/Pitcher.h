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
};
