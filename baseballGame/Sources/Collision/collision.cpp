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
	// 球の中心が円柱の上端より上の場合、当たっていない
	if (spherePosition.y > cylinderPosition.y + cylinderHeight + sphereRadius) {
		return false;
	}

	// 球の中心が円柱の下端より下の場合、当たっていない
	if (spherePosition.y < cylinderPosition.y - sphereRadius) {
		return false;
	}

	// XZ平面での距離チェック
	float vx = cylinderPosition.x - spherePosition.x;
	float vz = cylinderPosition.z - spherePosition.z;
	float combinedRadius = sphereRadius + cylinderRadius;
	float distanceXZ = sqrt(vx * vx + vz * vz);

	// XZ平面上で距離が半径の合計より大きい場合、当たっていない
	if (distanceXZ > combinedRadius) {
		return false;
	}

	// XZ平面で単位ベクトル化
	vx /= distanceXZ;
	vz /= distanceXZ;

	// 球が円柱を押し出す位置を計算
	outCylinderPosition.x = spherePosition.x + (vx * combinedRadius);
	outCylinderPosition.y = cylinderPosition.y; // Yはそのまま
	outCylinderPosition.z = spherePosition.z + (vz * combinedRadius);

	return true;
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

