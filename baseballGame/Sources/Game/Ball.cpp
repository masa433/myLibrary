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

void Ball::SetBezierTargetPosition(const DirectX::XMFLOAT3& targetPosition)
{
	bezierData.p3 = targetPosition;
}

// ---- ベジェ曲線投球開始 ----
void Ball::ThrowBezier(const BezierPitchData& data,
	const DirectX::XMFLOAT3& visualRotationSpeed,
	const DirectX::XMFLOAT3& visualAngle)
{
	if(!collider)
	{
		return;
	}

	bezierData = data;
	bezierT = 0.0f;
	bezierFlying = true;

	//キネマティックモードに切り替え
	//collider->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);

	//// 重力・空気抵抗で動かないよう速度だけゼロにしておく
	//collider->setLinearVelocity(physx::PxVec3(0, 0, 0));
	//collider->setAngularVelocity(physx::PxVec3(0, 0, 0));

	//見た目の回転をリセット
	modelRotationSpeed = visualRotationSpeed;
	modelAngle =
	{
		DirectX::XMConvertToRadians(visualAngle.x),
		DirectX::XMConvertToRadians(visualAngle.y),
		DirectX::XMConvertToRadians(visualAngle.z)
	};

	startPosition = worldPosition;
	ballTrail.clear();
	trailRecordTimer = 0.0f;
}

void Ball::UpdateBezierFlight(float elapsedTime)
{
	if (!bezierFlying || !collider)
	{
		return;
	}

	bezierT += elapsedTime / bezierData.durationSec; // bezierT を 0..1 にする

	if (bezierT > 1.0f) bezierT = 1.0f;

	DirectX::XMFLOAT3 pos = EvalCubicBezier(bezierT);

	// 終了直前（または十分近い）になったら「接線方向」を物理へ渡して動的に移行する
	const float endThresholdZ = 0.01f;
	const float endThresholdT = 0.995f; // t 判定も併用して安定させる
	bool nearEnd = (std::fabs(bezierData.p3.z - pos.z) < endThresholdZ) || (bezierT >= endThresholdT);

	if (nearEnd)
	{
		// パラメータ t 用の安全な値（1.0 直前の接線を使う）
		float t_for_deriv = (std::min)(bezierT, 0.999f);

		// ベジェのパラメータ t に対する導関数 dB/dt を計算
		DirectX::XMFLOAT3 deriv = EvalCubicBezierDerivative(t_for_deriv);

		// 実際のワールド速度 = dB/dt * (1 / durationSec)  （t は 0..1 を durationSec 秒で走る）
		DirectX::XMFLOAT3 worldVel = {
			deriv.x / bezierData.durationSec,
			deriv.y / bezierData.durationSec,
			deriv.z / bezierData.durationSec
		};

		// 必要なら Y 軸補正を少し入れる（地面落下抑制等）
		// worldVel.y += 0.0f; // 必要なら調整

		// kinematic を解除して物理へ戻す（速度を設定）
		collider->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
		collider->setLinearVelocity(physx::PxVec3(worldVel.x, worldVel.y, worldVel.z));

		// ベジェ移動は終了
		bezierFlying = false;

		// 現在位置を記録して戻る（以後は物理が位置を更新する）
		worldPosition = pos;
		return; // 以降で kinematic に対して setGlobalPose しないようにする
	}

	// 通常のキネマティック移動
	collider->setGlobalPose(physx::PxTransform(physx::PxVec3(pos.x, pos.y, pos.z)));
	worldPosition = pos;
}

// ---- バット衝突時に onContact() から呼ぶ ----
void Ball::CancelBezier()
{
	if (!bezierFlying) return;
	bezierFlying = false;
	// DynamicへはonContact()側でSetLinearVelocity前に切り替える
}

// 3次ベジェ曲線の導関数（dB/dt）
DirectX::XMFLOAT3 Ball::EvalCubicBezierDerivative(float t) const
{
	float u = 1.0f - t;
	// B'(t) = 3(1-t)^2 (P1 - P0) + 6(1-t)t (P2 - P1) + 3 t^2 (P3 - P2)
	float c0 = 3.0f * u * u;
	float c1 = 6.0f * u * t;
	float c2 = 3.0f * t * t;

	return {
		c0 * (bezierData.p1.x - bezierData.p0.x) + c1 * (bezierData.p2.x - bezierData.p1.x) + c2 * (bezierData.p3.x - bezierData.p2.x),
		c0 * (bezierData.p1.y - bezierData.p0.y) + c1 * (bezierData.p2.y - bezierData.p1.y) + c2 * (bezierData.p3.y - bezierData.p2.y),
		c0 * (bezierData.p1.z - bezierData.p0.z) + c1 * (bezierData.p2.z - bezierData.p1.z) + c2 * (bezierData.p3.z - bezierData.p2.z)
	};
}

//3次ベジェ曲線の評価
DirectX::XMFLOAT3 Ball::EvalCubicBezier(float t) const
{
	float u = 1.0f - t;
	float u2 = u * u;
	float u3 = u2 * u;
	float t2 = t * t;
	float t3 = t2 * t;

	// B(t) = u³P0 + 3u²tP1 + 3ut²P2 + t³P3
	return {
		u3 * bezierData.p0.x + 3 * u2 * t * bezierData.p1.x
			+ 3 * u * t2 * bezierData.p2.x + t3 * bezierData.p3.x,
		u3 * bezierData.p0.y + 3 * u2 * t * bezierData.p1.y
			+ 3 * u * t2 * bezierData.p2.y + t3 * bezierData.p3.y,
		u3 * bezierData.p0.z + 3 * u2 * t * bezierData.p1.z
			+ 3 * u * t2 * bezierData.p2.z + t3 * bezierData.p3.z
	};
}

void Ball::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	model = std::make_unique<gltf_model>(device, ".\\resources\\ball\\ball.glb");

	position = { 0.0f, 0.0f, 0.05f };
	scale = { 1.2f, 1.2f, 1.2f };
	angle = { 0.0f, 0.0f, 0.0f };
	worldPosition = { 0.0f, 0.0f, 0.0f };
	worldAngle = angle;
	worldScale = { 1.2f, 1.2f, 1.2f };
	debugRadius = 0.037f;

	physx::PxPhysics* pxPhysics = Physics::Instance().GetPhysics();
	physx::PxScene* pxScene = Physics::Instance().GetScene();

	material = pxPhysics->createMaterial(0.4f, 0.3f, 0.42f);
	physx::PxSphereGeometry geometry(debugRadius);
	physx::PxTransform transform(physx::PxVec3(worldPosition.x, worldPosition.y, worldPosition.z));

	collider = pxPhysics->createRigidDynamic(transform);
	collider->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
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
	
}



void Ball::Render(const RenderContext& rc, ModelRenderer* renderer, bool isThrown)
{

	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();

	if (!model)
	{
		return;
	}

	model->render(rc.deviceContext, worldTransform, {});

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

	if (ImGui::CollapsingHeader("Ball Visual Rotation"))
	{
		ImGui::Text("Model Rotation Speed (deg/s) - 独立設定");
		ImGui::DragFloat("Rot X (バックスピン/トップスピン)", &modelRotationSpeed.x, 10.0f, -3600.0f, 3600.0f, "%.0f");
		ImGui::DragFloat("Rot Y (サイドスピン)", &modelRotationSpeed.y, 10.0f, -3600.0f, 3600.0f, "%.0f");
		ImGui::DragFloat("Rot Z (ジャイロ)", &modelRotationSpeed.z, 10.0f, -3600.0f, 3600.0f, "%.0f");

		ImGui::Separator();
		ImGui::DragFloat3("Model Angle (rad)", &modelAngle.x, 0.01f);
		if (ImGui::Button("Reset Model Angle"))
			modelAngle = { 0.0f, 0.0f, 0.0f };
		if (ImGui::Button("Reset Rotation Speed"))
			modelRotationSpeed = { 0.0f, 0.0f, 0.0f };
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
	modelAngle = { 0.0f, 0.0f, 0.0f };
	modelRotationSpeed = { 0.0f, 0.0f, 0.0f };

	UpdateWorldTransform();
}

void Ball::UpdateFromPhysics(float elapsedTime)
{
	if (!collider)
	{
		return;
	}

	// 1. PhysXコライダーからは「位置」だけを取得する
	if (!bezierFlying)
	{
		physx::PxTransform pose = collider->getGlobalPose();
		worldPosition = DirectX::XMFLOAT3(pose.p.x, pose.p.y, pose.p.z);
	}

	// 2. 「回転」はコライダーを完全に無視し、純粋なパラメーター(rotationSpeed)のみで自前計算する
	// rotationSpeed は「度/秒(deg/s)」で計算されているため、経過時間を掛けて今フレームの回転量を求める
	modelAngle.x += DirectX::XMConvertToRadians(modelRotationSpeed.x) * elapsedTime;
	modelAngle.y += DirectX::XMConvertToRadians(modelRotationSpeed.y) * elapsedTime;
	modelAngle.z += DirectX::XMConvertToRadians(modelRotationSpeed.z) * elapsedTime;

	// 3. 角度が無限に増え続けないように 0 ～ 2π の範囲に丸める
	auto WrapAngle = [](float& angle) {
		const float twoPi = 2.0f * 3.14159265f;
		if (angle > twoPi) angle -= twoPi;
		if (angle < 0.0f) angle += twoPi;
		};
	WrapAngle(modelAngle.x);
	WrapAngle(modelAngle.y);
	WrapAngle(modelAngle.z);
	UpdateWorldTransform();

	// 物理演算中（飛んでいる時）にトレイルを記録

	if (hasCollidedWithBat)
	{
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
	constexpr float dragCoeff = 0.38f;//抗力係数

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

void Ball::Throw(const physx::PxVec3& initialVelocity, const physx::PxVec3& angularVelocity, const DirectX::XMFLOAT3& visualRotationSpeed, const DirectX::XMFLOAT3& visualAngle)
{
	if (!collider)
	{
		return;
	}

	startPosition = worldPosition;
	worldScale = scale;
	worldAngle = angle;
	collider->setLinearVelocity(initialVelocity);
	collider->setAngularVelocity(angularVelocity);
	velocity = { initialVelocity.x, initialVelocity.y, initialVelocity.z };
	UpdateWorldTransform();
	ballTrail.clear();
	trailRecordTimer = 0.0f;
	modelRotationSpeed = visualRotationSpeed;
	modelAngle = {                             // ← 追加
		DirectX::XMConvertToRadians(visualAngle.x),
		DirectX::XMConvertToRadians(visualAngle.y),
		DirectX::XMConvertToRadians(visualAngle.z)
	};
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
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(modelAngle.x, modelAngle.y, modelAngle.z);
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