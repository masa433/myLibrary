#include "../pch.h"
#include "Object.h"

Object::Object()
{
	net = std::make_unique<Model>("Data/Model/field/net.mdl");
	rotation.y = DirectX::XMConvertToRadians(-90.0f);
	position.y = -5.7f;
	scale = { 2.0f,2.0f,2.0f };
}

Object::~Object()
{
}

void Object::Update(float elapsedTime)
{
	using namespace DirectX;
	// 行列計算
	XMMATRIX matScale = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX matRotate = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX matTrans = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX matWorld = matScale * matRotate * matTrans;
	XMStoreFloat4x4(&transform, matWorld);
}

void Object::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, net.get(), ShaderId::Lambert);
}

void Object::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	DirectX::XMFLOAT3 boxPos = position;
	boxPos.y += 5.0f;
	// ネットのデバッグ用ボックスを描画
	renderer->RenderBox(rc, boxPos, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.7f, 5.7f, 5.0f), DirectX::XMFLOAT4(0, 0, 0, 1));
}

// レイキャスト
bool Object::RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit)
{
	return Collision::IntersectRayVsModel(start, end, net.get(), hit);
}

void Object::DrawImGui()
{
	if (ImGui::Begin("Object Transform"))
	{
		ImGui::DragFloat3("Position", &position.x, 0.1f);
		ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat3("Rotation", &rotation.x, 1.0f, 0.0f, 360.0f);
	}
	ImGui::End();
}