#include "../pch.h"
#include"Collision.h"

//球と球の交差判定
bool Collision::IntersectSphereVsSphere(
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
	float range =radiusA+radiusB;
	if (lengthSq >range ) {
		return false;
	}
	//AがBを押し出す
	DirectX::XMVECTOR overlap = DirectX::XMVectorScale(Vec, (range - sqrt(lengthSq)) / sqrt(lengthSq));
	PositionB = DirectX::XMVectorAdd(PositionB, overlap);


	DirectX::XMStoreFloat3(&outPositionB, PositionB);
	return true;
}

bool Collision::IntersectCylinderVsCylinder(
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

bool Collision::IntersectSphereVsCylinder(
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

bool Collision::IntersectBoxVsBox(
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

// レイとモデルの交差判定
bool Collision::IntersectRayVsModel(
	const DirectX::XMFLOAT3& start,
	const DirectX::XMFLOAT3& end,
	const Model* model,
	HitResult& result)
{
	DirectX::XMVECTOR WorldStart = DirectX::XMLoadFloat3(&start);
	DirectX::XMVECTOR WorldEnd = DirectX::XMLoadFloat3(&end);
	DirectX::XMVECTOR WorldRayVec = DirectX::XMVectorSubtract(WorldEnd, WorldStart);
	DirectX::XMVECTOR WorldRayLength = DirectX::XMVector3Length(WorldRayVec);

	// ワールド空間のレイの長さ
	DirectX::XMStoreFloat(&result.distance, WorldRayLength);

	bool hit = false;
	const ModelResource* resource = model->GetResource();
	for (const ModelResource::Mesh& mesh : resource->GetMeshes())
	{
		// メッシュノード取得
		const Model::Node& node = model->GetNodes().at(mesh.nodeIndex);

		// レイをワールド空間からローカル空間へ変換
		DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&node.globalTransform);
		DirectX::XMMATRIX InverseWorldTransform = DirectX::XMMatrixInverse(nullptr, WorldTransform);

		DirectX::XMVECTOR S = DirectX::XMVector3TransformCoord(WorldStart, InverseWorldTransform);
		DirectX::XMVECTOR E = DirectX::XMVector3TransformCoord(WorldEnd, InverseWorldTransform);
		DirectX::XMVECTOR SE = DirectX::XMVectorSubtract(E, S);
		DirectX::XMVECTOR V = DirectX::XMVector3Normalize(SE);
		DirectX::XMVECTOR Length = DirectX::XMVector3Length(SE);

		// レイの長さ
		float neart;
		DirectX::XMStoreFloat(&neart, Length);

		// 三角形（面）との交差判定
		const std::vector<ModelResource::Vertex>& vertices = mesh.vertices;
		const std::vector<UINT> indices = mesh.indices;

		int materialIndex = -1;
		DirectX::XMVECTOR HitPosition;
		DirectX::XMVECTOR HitNormal;
		for (const ModelResource::Subset& subset : mesh.subsets)
		{
			for (UINT i = 0; i < subset.indexCount; i += 3)
			{
				UINT index = subset.startIndex + i;

				// 三角形の頂点を抽出
				const ModelResource::Vertex& a = vertices.at(indices.at(index));
				const ModelResource::Vertex& b = vertices.at(indices.at(index + 1));
				const ModelResource::Vertex& c = vertices.at(indices.at(index + 2));

				DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&a.position);
				DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&b.position);
				DirectX::XMVECTOR C = DirectX::XMLoadFloat3(&c.position);

				// 三角形の三辺ベクトルを算出
				DirectX::XMVECTOR AB = DirectX::XMVectorSubtract(B, A);
				DirectX::XMVECTOR BC = DirectX::XMVectorSubtract(C, B);
				DirectX::XMVECTOR CA = DirectX::XMVectorSubtract(A, C);

				// 三角形の法線ベクトルを算出		
				DirectX::XMVECTOR N = DirectX::XMVector3Cross(AB, BC);

				// 内積の結果がプラスならば裏向き
				DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(V, N);
				float d;
				DirectX::XMStoreFloat(&d, Dot);
				if (d >= 0) continue;

				// レイと平面の交点を算出
				DirectX::XMVECTOR SA = DirectX::XMVectorSubtract(A, S);
				DirectX::XMVECTOR X = DirectX::XMVectorDivide(DirectX::XMVector3Dot(N, SA), Dot);
				float x;
				DirectX::XMStoreFloat(&x, X);
				if (x < .0f || x > neart) continue;	// 交点までの距離が今までに計算した最近距離より
				// 大きい時はスキップ
				DirectX::XMVECTOR P = DirectX::XMVectorAdd(DirectX::XMVectorMultiply(V, X), S);

				// 交点が三角形の内側にあるか判定
				// １つめ
				DirectX::XMVECTOR PA = DirectX::XMVectorSubtract(A, P);
				DirectX::XMVECTOR Cross1 = DirectX::XMVector3Cross(PA, AB);
				DirectX::XMVECTOR Dot1 = DirectX::XMVector3Dot(Cross1, N);
				float dot1;
				DirectX::XMStoreFloat(&dot1, Dot1);
				if (dot1 < 0.0f) continue;
				// ２つめ
				DirectX::XMVECTOR PB = DirectX::XMVectorSubtract(B, P);
				DirectX::XMVECTOR Cross2 = DirectX::XMVector3Cross(PB, BC);
				DirectX::XMVECTOR Dot2 = DirectX::XMVector3Dot(Cross2, N);
				float dot2;
				DirectX::XMStoreFloat(&dot2, Dot2);
				if (dot2 < 0.0f) continue;
				// ３つめ
				DirectX::XMVECTOR PC = DirectX::XMVectorSubtract(C, P);
				DirectX::XMVECTOR Cross3 = DirectX::XMVector3Cross(PC, CA);
				DirectX::XMVECTOR Dot3 = DirectX::XMVector3Dot(Cross3, N);
				float dot3;
				DirectX::XMStoreFloat(&dot3, Dot3);
				if (dot3 < 0.0f) continue;

				// 最近距離を更新
				neart = x;

				// 交点と法線を更新
				HitPosition = P;
				HitNormal = N;
				materialIndex = subset.materialIndex;
			}
		}
		if (materialIndex >= 0)
		{
			// ローカル空間からワールド空間へ変換
			DirectX::XMVECTOR WorldPosition = DirectX::XMVector3TransformCoord(HitPosition, WorldTransform);
			DirectX::XMVECTOR WorldCrossVec = DirectX::XMVectorSubtract(WorldPosition, WorldStart);
			DirectX::XMVECTOR WorldCrossLength = DirectX::XMVector3Length(WorldCrossVec);
			float distance;
			DirectX::XMStoreFloat(&distance, WorldCrossLength);

			// ヒット情報保存
			if (result.distance > distance)
			{
				DirectX::XMVECTOR WorldNormal = DirectX::XMVector3TransformNormal(HitNormal, WorldTransform);

				result.distance = distance;
				result.materialIndex = materialIndex;
				DirectX::XMStoreFloat3(&result.position, WorldPosition);
				DirectX::XMStoreFloat3(&result.normal, DirectX::XMVector3Normalize(WorldNormal));
				hit = true;
			}
		}
	}

	return hit;

}