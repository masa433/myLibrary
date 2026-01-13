#include "../pch.h"
#include "Pitcher.h"

void Pitcher::Initialize()
{
	pitcher = std::make_unique<Model>("Data/Model/pitcher/pitcher.mdl");

	scale.x = scale.y = scale.z = 0.05f;
	position = { 20.5f, 0.0f, 0.0f };
	angle.y = DirectX::XMConvertToRadians(-90.0f);

	SetPitchingState();
}

void Pitcher::Finalize()
{
	
}

void Pitcher::Update(float elapsedTime)
{
	// 更新処理
	switch (state)
	{
	case Pitcher::State::Throwing:
		UpdatePitchingState(elapsedTime);
		break;
	default:
		break;
	}

	UpdateVelocity(elapsedTime);

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	pitcher->UpdateTransform();

	// ピッチング状態の更新処理
	pitcher->UpdateAnimation(elapsedTime);
}

void Pitcher::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	// 描画処理
	renderer->Render(rc, transform, pitcher.get(), ShaderId::Lambert);
}

void Pitcher::DrawImGui()
{
	// ImGuiによる描画処理
}

void Pitcher::SetPitchingState()
{
	state = State::Throwing;
	pitcher->PlayAnimation(Animation::Pitching, true);
}

void Pitcher::UpdatePitchingState(float elapsedTime)
{
	
}
