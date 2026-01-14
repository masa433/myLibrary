#include "pch.h"
#include "ProjectileStraight.h"

ProjectileStraight::ProjectileStraight(ProjectileManager* manager) :
	Projectile(manager)//基底クラスのコンストラクタ
{
	//model = new Model("Data/Model/SpikeBall/SpikeBall.mdl");
	model = new Model("Data/Model/Sword/Sword.mdl");

	//scale.x = scale.y = scale.z = 0.5f;
	scale.x = scale.y = scale.z = 10.0f;
}

ProjectileStraight::~ProjectileStraight()
{
	delete model;
}

void ProjectileStraight::Update(float elapsedTime) 
{

	//寿命処理
	lifeTimer -= elapsedTime;
	if (lifeTimer <= 0.0f)
	{
		Destroy();
	}

	//移動
	float speed = this->speed * elapsedTime;
	position.x += direction.x * speed;
	position.y += direction.y * speed;
	position.z += direction.z * speed;

	//オブジェクト行列を更新
	UpdateTransform();

	//モデル行列更新
	model->UpdateTransform();

}

void ProjectileStraight::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::ShadowMap);
}

void ProjectileStraight::Launch(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& position)
{
	this->direction = direction;
	this->position = position;
}