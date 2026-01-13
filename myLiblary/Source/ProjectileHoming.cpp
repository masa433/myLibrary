#include "../pch.h"
#include "ProjectileHoming.h"

ProjectileHoming::ProjectileHoming(ProjectileManager* manager) :Projectile(manager)
{
	model = new Model("Data/Model/Sword/Sword.mdl");

	scale.x = scale.y = scale.z = 10.0f;
}

ProjectileHoming::~ProjectileHoming() 
{
	delete model;
}

void ProjectileHoming::Update(float elapsedTime)
{
	lifeTimer -= elapsedTime;
	if (lifeTimer <= 0.0f)
	{
		Destroy();
		return; // ここで処理を終了
	}

	//移動
	{
		float moveSpeed = this->moveSpeed * elapsedTime;
		position.x += direction.x * moveSpeed;
		position.y += direction.y * moveSpeed;
		position.z += direction.z * moveSpeed;

		// yの値が0以下になったらDestroyを呼び出す
		if (position.y <= 0.0f)
		{
			Destroy();
			return; // 処理を終了
		}
	}

	//旋回
	{
		float turnSpeed = this->turnSpeed * elapsedTime;

		//ターゲットまでのベクトルを算出
		DirectX::XMVECTOR Position = DirectX::XMLoadFloat3(&position);
		DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&target);
		DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Target, Position);//引き算

		//ゼロベクトルでないなら回転処理
		DirectX::XMVECTOR LengthSq = DirectX::XMVector3Length(Vec);//ルートがいらない計算
		float lengthSq;
		DirectX::XMStoreFloat(&lengthSq, LengthSq);
		if (lengthSq > 0.00001f)
		{
			//ターゲットまでのベクトルを単位ベクトル化
			Vec = DirectX::XMVector3Normalize(Vec);

			//向いている方向ベクトルを算出
			DirectX::XMVECTOR Direction = DirectX::XMLoadFloat3(&direction);

			//前方方向ベクトルとターゲットまでのベクトルの内積(角度)を算出
			DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(Direction, Vec);

			float dot;
			DirectX::XMStoreFloat(&dot, Dot);

			//2つの単位ベクトルの角度が小さいほど
			//1.0に近づくという性質を利用して回転速度を調整する
			float rot = 1.0 - dot;
			if (rot > turnSpeed)
			{
				rot = turnSpeed;
			}

			//回転角度があるなら回転処理をする
			if (fabsf(rot) > 0.0001f)
			{
				//回転軸を算出
				DirectX::XMVECTOR Axis = DirectX::XMVector3Cross(Direction, Vec);
				//単位ベクトル化
				Axis = DirectX::XMVector3Normalize(Axis);

				//回転軸と回転量から回転行列を算出
				DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationAxis(Axis, rot);

				//現在の行列を回転させる
				DirectX::XMMATRIX Transform = DirectX::XMLoadFloat4x4(&transform);
				Transform = DirectX::XMMatrixMultiply(Transform, Rotation);

				//回転後の前方方向を取り出し、単位ベクトル化する
				Direction = DirectX::XMVector3Normalize(Transform.r[2]);
				DirectX::XMStoreFloat3(&direction, Direction);
			}


		}
	}

	//オブジェクト行列を更新
	UpdateTransform();

	//モデル行列更新
	model->UpdateTransform();
}


void ProjectileHoming::Render(const RenderContext& rc, ModelRenderer* renderer) 
{
	renderer->Render(rc, transform, model, ShaderId::ShadowMap);
}

void ProjectileHoming::Launch(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target)
{
	this->direction = direction;
	this->position = position;
	this->target = target;

	UpdateTransform();
}