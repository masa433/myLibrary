#include "../pch.h"
#include"Stage.h"


//コンストラクタ
Stage::Stage() {
	collisionModel = std::make_unique<Model>("Data/Model/field/stadium.mdl");
	//model = new Model("Data/Model/baseball/base.gltf");
    //rotation.x = -DirectX::XM_PIDIV2; // -90度
}

Stage::~Stage() {

	//ステージモデルを破棄
	//delete model;
	
}

//更新処理
void Stage::Update(float elapsedTime) {
    using namespace DirectX;
    // model用
    XMMATRIX matScale = XMMatrixScaling(modelParam.scale.x, modelParam.scale.y, modelParam.scale.z);
    XMMATRIX matRotate = XMMatrixRotationRollPitchYaw(modelParam.rotation.x, modelParam.rotation.y, modelParam.rotation.z);
    XMMATRIX matTrans = XMMatrixTranslation(modelParam.position.x, modelParam.position.y, modelParam.position.z);
    XMMATRIX matWorld = matScale * matRotate * matTrans;
    XMStoreFloat4x4(&modelParam.transform, matWorld);

    // collisionModel用
    matScale = XMMatrixScaling(collisionParam.scale.x, collisionParam.scale.y, collisionParam.scale.z);
    matRotate = XMMatrixRotationRollPitchYaw(collisionParam.rotation.x, collisionParam.rotation.y, collisionParam.rotation.z);
    matTrans = XMMatrixTranslation(collisionParam.position.x, collisionParam.position.y, collisionParam.position.z);
    matWorld = matScale * matRotate * matTrans;
    XMStoreFloat4x4(&collisionParam.transform, matWorld);
}

void Stage::Render(const RenderContext& rc, ModelRenderer* renderer) {
    //renderer->Render(rc, modelParam.transform, model, ShaderId::ShadowMap);
    renderer->Render(rc, collisionParam.transform, collisionModel.get(), ShaderId::ShadowMap);
}

// レイキャスト
bool Stage::RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit)
{
    return Collision::IntersectRayVsModel(start, end, collisionModel.get(), hit);
}

void Stage::DrawImGui() {
    if (ImGui::Begin("Model Transform")) {
        ImGui::DragFloat3("Position", &modelParam.position.x, 0.1f);
        ImGui::DragFloat3("Scale", &modelParam.scale.x, 1.0f, 1.0f, 100.0f);
        ImGui::DragFloat3("Rotation", &modelParam.rotation.x, 1.0f, 0.0f, 360.0f);
    }
    ImGui::End();

    if (ImGui::Begin("Collision Transform")) {
        ImGui::DragFloat3("Position", &collisionParam.position.x, 0.1f);
        ImGui::DragFloat3("Scale", &collisionParam.scale.x, 1.0f, 1.0f, 100.0f);
        ImGui::DragFloat3("Rotation", &collisionParam.rotation.x, 1.0f, 0.0f, 360.0f);
    }
    ImGui::End();
}