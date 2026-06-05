#pragma once

#include <vector>
#include <DirectXMath.h>
#include "../Model/gltf_model.h"
#include "RenderContext.h"

// ヒット結果
struct HitResult
{
	DirectX::XMFLOAT3	position = { 0, 0, 0 };// レイとポリゴンの交点
	DirectX::XMFLOAT3	normal = { 0, 0, 0 };	// 衝突したポリゴンの法線ベクトル
	DirectX::XMFLOAT3	rotation = { 0, 0, 0 };	// 回転量
	float				distance = 0.0f; 		// レイの始点から交点までの距離
	int					materialIndex = -1; 	// 衝突したポリゴンのマテリアル番号
};

class collision 
{
public:
	// 球対球の当たり判定
	static bool IntersectSphereVsSphere(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		DirectX::XMFLOAT3& outPositionB
	);

	//円柱対円柱の当たり判定
	static bool IntersectCylinderVsCylinder(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		float heightA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		float heightB,
		DirectX::XMFLOAT3& outPositionB
	);

	// 球対円柱の当たり判定
	static bool IntersectSphereVsCylinder(
		const DirectX::XMFLOAT3& spherePosition,
		float sphereRadius,
		const DirectX::XMFLOAT3& cylinderPosition,
		float cylinderRadius,
		float cylinderHeight,
		DirectX::XMFLOAT3& outcylinderPosition
	);

	// AABB対AABBの当たり判定
	static bool IntersectAABBVsAABB(
		const DirectX::XMFLOAT3& minA,
		const DirectX::XMFLOAT3& maxA,
		const DirectX::XMFLOAT3& minB,
		const DirectX::XMFLOAT3& maxB
	);


};