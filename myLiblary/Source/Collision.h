#pragma once

#include "System/Model.h"

// ヒット結果
struct HitResult
{
	DirectX::XMFLOAT3	position = { 0, 0, 0 };// レイとポリゴンの交点
	DirectX::XMFLOAT3	normal = { 0, 0, 0 };	// 衝突したポリゴンの法線ベクトル
	DirectX::XMFLOAT3	rotation = { 0, 0, 0 };	// 回転量
	float				distance = 0.0f; 		// レイの始点から交点までの距離
	int					materialIndex = -1; 	// 衝突したポリゴンのマテリアル番号
};

//コリジョン
class Collision 
{
public:
	//球と球の交差判定
	static bool IntersectSphereVsSphere(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		DirectX::XMFLOAT3& outPositionB
	);//汎用性がある
	//独立性を上げる(playerとenemyがない方がいい)

	//円柱と円柱の交差判定
	static bool IntersectCylinderVsCylinder(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		float heughtA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		float heightB,
		DirectX::XMFLOAT3& outPositionB

	);

	//球と円柱の交差判定
	static bool IntersectSphereVsCylinder(

		const DirectX::XMFLOAT3& spherePosition,
		float sphereRadius,
		const DirectX::XMFLOAT3& cylinderPosition,
		float cylinderRadius,
		float cylinderHeight,
		DirectX::XMFLOAT3& outcylinderPosition
	);

	// 箱と箱の交差判定
	static bool IntersectBoxVsBox(
		const DirectX::XMFLOAT3& minA,
		const DirectX::XMFLOAT3& maxA,
		const DirectX::XMFLOAT3& minB,
		const DirectX::XMFLOAT3& maxB
	);

	// レイとモデルの交差判定
	static bool IntersectRayVsModel(
		const DirectX::XMFLOAT3& start,
		const DirectX::XMFLOAT3& end,
		const Model* model,
		HitResult& result
	);
};