#include"collision.h"
#include <DirectXCollision.h>

//球と球の交差判定
bool collision::IntersectSphereVsSphere(
	const DirectX::XMFLOAT3& positionA,
	float radiusA,
	const DirectX::XMFLOAT3& positionB,
	float radiusB,
	DirectX::XMFLOAT3& outPositionB
)
{
	// A→Bの単位ベクトルを算出
	DirectX::XMVECTOR PositionA = { positionA.x, positionA.y, positionA.z };
	DirectX::XMVECTOR PositionB = { positionB.x, positionB.y, positionB.z };

	// AからBへのベクトルを計算
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PositionB, PositionA);

	// 長さの二乗を計算
	DirectX::XMVECTOR LengthSq = DirectX::XMVector3LengthSq(Vec);
	float lengthSq;
	DirectX::XMStoreFloat(&lengthSq, LengthSq);

	//距離判定
	float range = radiusA + radiusB;
	if (lengthSq > range) {
		return false;
	}
	//AがBを押し出す
	DirectX::XMVECTOR overlap = DirectX::XMVectorScale(Vec, (range - sqrt(lengthSq)) / sqrt(lengthSq));
	PositionB = DirectX::XMVectorAdd(PositionB, overlap);


	DirectX::XMStoreFloat3(&outPositionB, PositionB);
	return true;
}

bool collision::IntersectCylinderVsCylinder(
	const DirectX::XMFLOAT3& positionA,
	float radiusA,
	float heightA,
	const DirectX::XMFLOAT3& positionB,
	float radiusB,
	float heightB,
	DirectX::XMFLOAT3& outPositionB)
{
	// Aの足元がBの頭より上なら当たっていない
	if (positionA.y > positionB.y + heightB)
	{
		return false;
	}

	// Aの頭がBの足元より下なら当たっていない
	if (positionA.y + heightA < positionB.y)
	{
		return false;
	}

	// XZ平面での範囲チェック
	float vx = positionB.x - positionA.x;
	float vz = positionB.z - positionA.z;
	float range = radiusA + radiusB;
	float disXZ = sqrt(vx * vx + vz * vz);

	if (disXZ > range)
	{
		return false;
	}

	// 単位ベクトル化

	vx /= disXZ;
	vz /= disXZ;


	// AがBを押し出す
	outPositionB.x = positionA.x + (vx * range);
	outPositionB.y = positionB.y;// yはそのまま
	outPositionB.z = positionA.z + (vz * range);

	return true;
}

bool collision::IntersectSphereVsCylinder(
	const DirectX::XMFLOAT3& spherePosition,
	float sphereRadius,
	const DirectX::XMFLOAT3& cylinderPosition,
	float cylinderRadius,
	float cylinderHeight,
	DirectX::XMFLOAT3& outCylinderPosition)
{
	// XZ平面での距離チェック
	float vx = spherePosition.x - cylinderPosition.x;
	float vz = spherePosition.z - cylinderPosition.z;
	float distanceXZ = sqrt(vx * vx + vz * vz);

	// Y軸方向の最も近い点を計算（円柱は底面が cylinderPosition.y）
	float closestY = spherePosition.y;
	if (closestY < cylinderPosition.y) {
		closestY = cylinderPosition.y; // 底面
	}
	else if (closestY > cylinderPosition.y + cylinderHeight) {
		closestY = cylinderPosition.y + cylinderHeight; // 上面
	}

	// Y軸方向の距離
	float dy = spherePosition.y - closestY;

	// 球が円柱の側面と当たる場合の判定
	if (closestY == spherePosition.y) {
		// 球の中心が円柱の高さ範囲内にある場合
		// XZ平面での距離が半径の合計より大きければ当たっていない
		float combinedRadius = sphereRadius + cylinderRadius;
		if (distanceXZ > combinedRadius) {
			return false;
		}

		// ゼロ除算を防ぐ
		if (distanceXZ < 0.0001f) {
			// 球が円柱の中心軸上にある場合
			outCylinderPosition.x = cylinderPosition.x + combinedRadius;
			outCylinderPosition.y = closestY;
			outCylinderPosition.z = cylinderPosition.z;
			return true;
		}

		// XZ平面で単位ベクトル化
		vx /= distanceXZ;
		vz /= distanceXZ;

		// 衝突点を計算（円柱の表面）
		outCylinderPosition.x = cylinderPosition.x + (vx * cylinderRadius);
		outCylinderPosition.y = closestY;
		outCylinderPosition.z = cylinderPosition.z + (vz * cylinderRadius);

		return true;
	}
	else {
		// 球の中心が円柱の上下範囲外にある場合
		// 円柱の端面（円）との判定
		float combinedRadius = sphereRadius + cylinderRadius;

		// XZ平面での距離チェック
		if (distanceXZ > combinedRadius) {
			return false;
		}

		// Y軸方向の距離チェック
		if (fabs(dy) > sphereRadius) {
			return false;
		}

		// ゼロ除算を防ぐ
		if (distanceXZ < 0.0001f) {
			outCylinderPosition.x = cylinderPosition.x;
			outCylinderPosition.y = closestY;
			outCylinderPosition.z = cylinderPosition.z;
			return true;
		}

		// XZ平面で単位ベクトル化
		vx /= distanceXZ;
		vz /= distanceXZ;

		// 衝突点を計算（円柱の端面）
		outCylinderPosition.x = cylinderPosition.x + (vx * cylinderRadius);
		outCylinderPosition.y = closestY;
		outCylinderPosition.z = cylinderPosition.z + (vz * cylinderRadius);

		return true;
	}
}

bool collision::IntersectAABBVsAABB(
	const DirectX::XMFLOAT3& minA,
	const DirectX::XMFLOAT3& maxA,
	const DirectX::XMFLOAT3& minB,
	const DirectX::XMFLOAT3& maxB
)
{
	// X軸方向の判定
	if (maxA.x < minB.x || minA.x > maxB.x) {
		return false;
	}
	// Y軸方向の判定
	if (maxA.y < minB.y || minA.y > maxB.y) {
		return false;
	}
	// Z軸方向の判定
	if (maxA.z < minB.z || minA.z > maxB.z) {
		return false;
	}
	// 全ての軸で重なっているので当たっている
	return true;
}

