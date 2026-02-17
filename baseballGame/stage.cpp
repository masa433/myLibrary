#include "stage.h"
#include "imgui.h"
#include "Graphics.h"

// 初期化
void stage::initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	model = std::make_unique<Model>(".\\resources\\field\\stadium.mdl");
	// 位置、スケール、回転の初期化
	position = { 0.0f, 0.0f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };
	angle = { 0.0f, 0.0f, 0.0f };

	
}

// 更新
void stage::update(float elapsedTime)
{
#ifdef  USE_IMGUI
	if (ImGui::CollapsingHeader("Stage"))
	{
		ImGui::DragFloat3("Position", &position.x);
		ImGui::DragFloat3("Scale", &scale.x);
		ImGui::DragFloat3("Angle", &angle.x);
	}
#endif //  USE_IMGUI



	UpdateTransform();
}

void stage::render(RenderContext& rc)
{

	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

	modelRenderer->Render(rc, transform, model.get(), ShaderId::Lambert);
}

// 終了
void stage::uninitialize()
{
	model.reset();
}

