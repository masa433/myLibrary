#include "FrustumCulling.h"

void FrustumCulling::Construct(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	using namespace DirectX;

	// ビュー行列とプロジェクション行列を読み込む
	XMMATRIX V = XMLoadFloat4x4(&view);
	XMMATRIX P = XMLoadFloat4x4(&projection);
	XMMATRIX VP = V * P;

	// ビュー行列とプロジェクション行列の積を保存する
	XMFLOAT4X4 vpMatrix;
	XMStoreFloat4x4(&vpMatrix, VP);

	// フラスタムの6つの平面を抽出する
	// 左平面 : 行列の第4列 + 第1列
	planes[0].normal.x = vpMatrix._14 + vpMatrix._11;
	planes[0].normal.y = vpMatrix._24 + vpMatrix._21;
	planes[0].normal.z = vpMatrix._34 + vpMatrix._31;
	planes[0].distance = vpMatrix._44 + vpMatrix._41;

	// 右平面 : 行列の第4列 - 第1列
	planes[1].normal.x = vpMatrix._14 - vpMatrix._11;
	planes[1].normal.y = vpMatrix._24 - vpMatrix._21;
	planes[1].normal.z = vpMatrix._34 - vpMatrix._31;
	planes[1].distance = vpMatrix._44 - vpMatrix._41;

	// 上平面 : 行列の第4列 - 第2列
	planes[2].normal.x = vpMatrix._14 - vpMatrix._12;
	planes[2].normal.y = vpMatrix._24 - vpMatrix._22;
	planes[2].normal.z = vpMatrix._34 - vpMatrix._32;
	planes[2].distance = vpMatrix._44 - vpMatrix._42;

	// 下平面 : 行列の第4列 + 第2列
	planes[3].normal.x = vpMatrix._14 + vpMatrix._12;
	planes[3].normal.y = vpMatrix._24 + vpMatrix._22;
	planes[3].normal.z = vpMatrix._34 + vpMatrix._32;
	planes[3].distance = vpMatrix._44 + vpMatrix._42;

	// 近平面 : 行列の第4列 + 第3列
	planes[4].normal.x = vpMatrix._14 + vpMatrix._13;
	planes[4].normal.y = vpMatrix._24 + vpMatrix._23;
	planes[4].normal.z = vpMatrix._34 + vpMatrix._33;
	planes[4].distance = vpMatrix._44 + vpMatrix._43;

	// 遠平面 : 行列の第4列 - 第3列
	planes[5].normal.x = vpMatrix._14 - vpMatrix._13;
	planes[5].normal.y = vpMatrix._24 - vpMatrix._23;
	planes[5].normal.z = vpMatrix._34 - vpMatrix._33;
	planes[5].distance = vpMatrix._44 - vpMatrix._43;

	//各平面を正規化
	for(int i = 0; i < 6; ++i)
	{
		XMVECTOR normal = XMLoadFloat3(&planes[i].normal);
		float length = XMVectorGetX(XMVector3Length(normal));

		if (length > 0.0001f)
		{
			normal = XMVector3Normalize(normal);
			XMStoreFloat3(&planes[i].normal, normal);
			planes[i].distance /= length;
		}
	}
}

bool FrustumCulling::IssphereVisible(const DirectX::XMFLOAT3& center, float radius) const
{
	for(int i = 0; i < 6; ++i)
	{
		float distance = DistanceToPlane(planes[i], center);
		if (distance < -radius)
		{
			return false; // 球がフラスタムの外にある
		}
	}
	return true; // 球がフラスタムの内側または交差している
}

bool FrustumCulling::IsAABBVisible(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max) const
{
	// AABBの8つの頂点を計算
	DirectX::XMFLOAT3 corners[8] = {
		{ min.x, min.y, min.z },
		{ max.x, min.y, min.z },
		{ min.x, max.y, min.z },
		{ max.x, max.y, min.z },
		{ min.x, min.y, max.z },
		{ max.x, min.y, max.z },
		{ min.x, max.y, max.z },
		{ max.x, max.y, max.z }
	};

	for (int plane = 0; plane < 6; ++plane)
	{
		bool allOutSide = true;

		for(int corner = 0; corner < 8; ++corner)
		{
			float distance = DistanceToPlane(planes[plane], corners[corner]);
			if (distance >= 0)
			{
				allOutSide = false;
				break; // この平面に対して少なくとも1つの頂点が内側にある
			}
		}

		if (allOutSide)
		{
			return false; // AABBがフラスタムの外にある
		}
	}

	return true; // AABBがフラスタムの内側または交差している
}

float FrustumCulling::DistanceToPlane(const Plane& plane, const DirectX::XMFLOAT3& point) const
{
	using namespace DirectX;

	XMVECTOR normal = XMLoadFloat3(&plane.normal);
	XMVECTOR pointVec = XMLoadFloat3(&point);

	// 平面の法線と点の内積を計算し、距離を求める
	float dot = XMVectorGetX(XMVector3Dot(normal, pointVec));
	return dot + plane.distance;// 平面の距離を加算
}

// ワールド変換行列を考慮したバウンディング球体の可視性チェック
bool FrustumCulling::IsTransformedSphereVisible(const DirectX::XMFLOAT3& center, float radius, const DirectX::XMFLOAT4X4& worldTransform) const
{
	using namespace DirectX;
	
	// ワールド変換行列で中心点を変換
	XMVECTOR centerVec = XMLoadFloat3(&center);
	XMMATRIX world = XMLoadFloat4x4(&worldTransform);
	XMVECTOR transformedCenter = XMVector3Transform(centerVec, world);

	//スケールを考慮して半径を調整
	//ワールド行列のスケール成分を取得
	XMVECTOR scaleX = XMVector3Length(world.r[0]);
	XMVECTOR scaleY = XMVector3Length(world.r[1]);
	XMVECTOR scaleZ = XMVector3Length(world.r[2]);

	//最大スケールを取得
	float maxScale = XMVectorGetX(XMVectorMax(XMVectorMax(scaleX, scaleY), scaleZ));
	float scaledRadius = radius * maxScale;//スケールを考慮した半径

	//変換後の中心点とスケールを考慮した半径で可視性を判定
	XMFLOAT3 transformedCenterFloat;
	XMStoreFloat3(&transformedCenterFloat, transformedCenter);

	return IssphereVisible(transformedCenterFloat, scaledRadius);
}