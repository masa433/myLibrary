#include "Ball.h"

#include "Graphics.h"
#include "imgui.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <random>

namespace
{
	float GenerateRandomFloat(float min, float max)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dis(min, max);
		return dis(gen);
	}
}

void Ball::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	model = std::make_unique<gltf_model>(device, ".\\resources\\ball\\ball.glb");

	position = { 0.0f, 0.0f, 0.05f };
	scale = { 1.0f, 1.0f, 1.0f };
	angle = { 0.0f, DirectX::XMConvertToRadians(90.0f), 0.0f };
	worldPosition = { 0.0f, 0.0f, 0.0f };
	worldAngle = angle;
	worldScale = { 1.0f, 1.0f, 1.0f };
	debugRadius = 0.037f;

	physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
	physx::PxScene* pxScene = Physics::Instance().GetScene();

	material = pxPhysics->createMaterial(0.4f, 0.3f, 0.42f);
	physx::PxSphereGeometry geometry(debugRadius);
	physx::PxTransform transform(physx::PxVec3(worldPosition.x, worldPosition.y, worldPosition.z));

	collider = pxPhysics->createRigidDynamic(transform);
	_ASSERT_EXPR(collider != nullptr, "Failed to create ball collider");

	physx::PxShape* shape = pxPhysics->createShape(geometry, *material);
	collider->attachShape(*shape);
	shape->release();

	physx::PxRigidBodyExt::setMassAndUpdateInertia(*collider, 0.145f);
	pxScene->addActor(*collider);
}

void Ball::Uninitialize()
{
	PX_RELEASE(collider);
	PX_RELEASE(material);
	model.reset();
}

void Ball::Update(float elapsedTime)
{
	UpdateFromPhysics(elapsedTime, { 0.0f, 0.0f, 0.0f });
}

void Ball::Render(const RenderContext& rc, ModelRenderer* renderer, bool isThrown)
{

	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();

	if (!model)
	{
		return;
	}

	model->render(rc.deviceContext, isThrown ? worldTransform : handTransform, {});

	// トレイルの描画
	if (ballTrail.size() > 1)
	{


		// 現在アクティブなカメラインスタンスのView/Projection行列を取得して利用する
		Camera& camera = Camera::Instance(); // シングルトンなどから取得

		// 軌跡の色（赤から白へグラデーションなど）
		DirectX::XMFLOAT4 trailColor = { 1.0f, 0.5f, 0.0f, 1.0f };

		for (size_t i = 0; i < ballTrail.size() - 1; ++i)
		{
			// 古いほど薄くするアルファ値の計算
			float alpha = static_cast<float>(i) / ballTrail.size();
			DirectX::XMFLOAT4 color = { trailColor.x, trailColor.y, trailColor.z, alpha };

			primitiveRenderer->AddVertex(ballTrail[i], color);
			primitiveRenderer->AddVertex(ballTrail[i + 1], color);
		}

		// 線の描画を実行（ラインリスト指定）
		primitiveRenderer->Render(rc.deviceContext, camera.GetView(), camera.GetProjection(), D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}
}

void Ball::DrawGUI()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("ball"))
	{
		ImGui::DragFloat3("ballPosition", &position.x);
		ImGui::DragFloat3("ballScale", &scale.x);
		ImGui::DragFloat3("ballAngle", &angle.x);

		ImGui::Separator();
		ImGui::Text("Ball World Transform");
		ImGui::DragFloat3("BallWorldPosition", &worldPosition.x);
		ImGui::DragFloat3("BallWorldAngle", &worldAngle.x);
		ImGui::DragFloat3("BallWorldScale", &worldScale.x);
	}

	if (ImGui::CollapsingHeader("Ball Debug Settings"))
	{
		ImGui::DragFloat("Ball Debug Radius", &debugRadius, 0.05f, 0.05f, 5.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Ball Trail Settings"))
	{
		ImGui::DragFloat("Trail Width", &trailWidth, 0.01f, 0.01f, 1.0f, "%.2f");
		ImGui::DragFloat("MaxTrailLength", &MaxTrailLength, 0.01f, 0.01f, 1.0f, "%.2f");
	}
#endif
}

void Ball::AttachToHand(const std::vector<gltf_model::node>& animatedNodes, const DirectX::XMFLOAT4X4& ownerTransform, const char* handName)
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::XMMATRIX localMatrix = S * R * T;
	DirectX::XMMATRIX ownerWorldMatrix = DirectX::XMLoadFloat4x4(&ownerTransform);

	bool handFound = false;
	for (const gltf_model::node& node : animatedNodes)
	{
		if (node.name == handName)
		{
			DirectX::XMMATRIX handMatrix = DirectX::XMLoadFloat4x4(&node.global_transform);
			DirectX::XMMATRIX ballWorldMatrix = localMatrix * handMatrix * ownerWorldMatrix;
			DirectX::XMStoreFloat4x4(&handTransform, ballWorldMatrix);
			handFound = true;
			break;
		}
	}

	if (!handFound)
	{
		DirectX::XMMATRIX ballWorldMatrix = localMatrix * ownerWorldMatrix;
		DirectX::XMStoreFloat4x4(&handTransform, ballWorldMatrix);
	}

	worldPosition.x = handTransform._41;
	worldPosition.y = handTransform._42;
	worldPosition.z = handTransform._43;
	worldAngle = angle;
	worldScale = scale;
	startPosition = worldPosition;
	SyncColliderToWorldPosition();
	// トレイルをリセット
	ballTrail.clear();
	trailRecordTimer = 0.0f;
}

void Ball::UpdateFromPhysics(float elapsedTime, const DirectX::XMFLOAT3& rotationSpeed)
{
	if (!collider)
	{
		return;
	}

	physx::PxTransform pxTransform = collider->getGlobalPose();
	worldPosition = { pxTransform.p.x, pxTransform.p.y, pxTransform.p.z };
	worldAngle.x += rotationSpeed.x * elapsedTime;
	worldAngle.y += rotationSpeed.y * elapsedTime;
	worldAngle.z += rotationSpeed.z * elapsedTime;
	UpdateWorldTransform();

	// 物理演算中（飛んでいる時）にトレイルを記録
	trailRecordTimer += elapsedTime;
	if (trailRecordTimer >= TrailRecordInterval)
	{
		trailRecordTimer = 0.0f;
		ballTrail.push_back(worldPosition);
		if (ballTrail.size() > MaxTrailLength)
		{
			ballTrail.pop_front();
		}
	}


}

void Ball::UpdateCollider()
{
	if (!collider)
	{
		return;
	}

	physx::PxShape* shape = nullptr;
	if (collider->getShapes(&shape, 1) > 0 && shape)
	{
		shape->setGeometry(physx::PxSphereGeometry(debugRadius));
	}
}

void Ball::ApplyPitchPhysics(bool isKnuckleball, const physx::PxVec3& windVelocity)
{
	if (!collider)
	{
		return;
	}

	if (isKnuckleball)
	{
		float randomLateralForce = GenerateRandomFloat(-0.00001f, 0.00001f);
		collider->addForce(physx::PxVec3(randomLateralForce, 0.0f, 0.0f), physx::PxForceMode::eFORCE);
	}

	physx::PxVec3 currentVelocity = collider->getLinearVelocity();
	velocity = { currentVelocity.x, currentVelocity.y, currentVelocity.z };

	physx::PxVec3 relativeVelocity = currentVelocity - windVelocity;
	float relativeSpeed = relativeVelocity.magnitude();

	constexpr float airDensity = 1.225f;//空気密度(kg/m^3)
	constexpr float ballRadius = 0.0365f;//野球ボールの半径(m)
	const float ballArea = DirectX::XM_PI * ballRadius * ballRadius;//ボールの断面積(m^2)
	constexpr float dragCoeff = 0.4f;//抗力係数

	if (relativeSpeed > 0.0f)
	{
		float dragMag = 0.5f * airDensity * relativeSpeed * relativeSpeed * dragCoeff * ballArea;
		physx::PxVec3 dragForce = -relativeVelocity.getNormalized() * dragMag;
		collider->addForce(dragForce, physx::PxForceMode::eFORCE);
	}

	physx::PxVec3 angularVelocity = collider->getAngularVelocity();
	float angularSpeed = angularVelocity.magnitude();

	if (relativeSpeed > 0.0f && angularSpeed > 0.0f)
	{
		float spinParameter = (ballRadius * angularSpeed) / relativeSpeed;
		float liftCoeff = 1.5f * spinParameter;
		if (liftCoeff > 0.4f) liftCoeff = 0.4f;
		float magnusMag = 0.5f * airDensity * relativeSpeed * relativeSpeed * liftCoeff * ballArea;

		physx::PxVec3 magnusDir = angularVelocity.cross(relativeVelocity);
		if (magnusDir.magnitudeSquared() > 0.0f)
		{
			magnusDir.normalize();
			collider->addForce(magnusDir * magnusMag, physx::PxForceMode::eFORCE);
		}
	}
}

void Ball::Throw(const physx::PxVec3& initialVelocity, const physx::PxVec3& angularVelocity)
{
	if (!collider)
	{
		return;
	}

	startPosition = worldPosition;
	worldScale = { 1.0f, 1.0f, 1.0f };
	worldAngle = angle;
	collider->setLinearVelocity(initialVelocity);
	collider->setAngularVelocity(angularVelocity);
	velocity = { initialVelocity.x, initialVelocity.y, initialVelocity.z };
	UpdateWorldTransform();
	ballTrail.clear();
	trailRecordTimer = 0.0f;
}

void Ball::ResetMotion()
{
	velocity = { 0.0f, 0.0f, 0.0f };
	if (collider)
	{
		collider->setLinearVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
		collider->setAngularVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
	}
}

void Ball::UpdateWorldTransform()
{
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(worldScale.x, worldScale.y, worldScale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(worldAngle.x, worldAngle.y, worldAngle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(worldPosition.x, worldPosition.y, worldPosition.z);
	DirectX::XMStoreFloat4x4(&worldTransform, S * R * T);
}

void Ball::SyncColliderToWorldPosition()
{
	if (!collider)
	{
		return;
	}

	collider->setGlobalPose(physx::PxTransform(physx::PxVec3(worldPosition.x, worldPosition.y, worldPosition.z)));
	ResetMotion();
}

void Ball::SaveToJson(json& j)
{
	j["position"] = { position.x, position.y, position.z };
	j["scale"] = { scale.x, scale.y, scale.z };
	j["angle"] = { angle.x, angle.y, angle.z };
	j["debug_radius"] = debugRadius;
	j["trail_width"] = trailWidth;
	j["max_trail_length"] = MaxTrailLength;
}

void Ball::LoadFromJson(const json& j)
{
	if (j.contains("position"))         position = { j["position"][0], j["position"][1], j["position"][2] };
	if (j.contains("scale"))            scale = { j["scale"][0], j["scale"][1], j["scale"][2] };
	if (j.contains("angle"))            angle = { j["angle"][0], j["angle"][1], j["angle"][2] };
	if (j.contains("debug_radius"))     debugRadius = j["debug_radius"];
	if (j.contains("trail_width"))      trailWidth = j["trail_width"];
	if (j.contains("max_trail_length")) MaxTrailLength = j["max_trail_length"];
}