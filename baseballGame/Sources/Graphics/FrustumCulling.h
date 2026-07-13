#pragma once
#include <DirectXMath.h>

// フラスタムカリングを行うためのクラス
class FrustumCulling
{
public:
	FrustumCulling() = default;
	~FrustumCulling() = default;

	//ビュー行列とプロジェクション行列からフラスタムを抽出
	void Construct(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

	//球がフラスタム内にあるかどうかを判定する
	bool IssphereVisible(const DirectX::XMFLOAT3& center, float radius) const;

	//AABBがフラスタム内にあるかどうかを判定する
	bool IsAABBVisible(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max) const;

	//デバッグ用のフラスタムの平面を表す構造体
	struct Plane
	{
		DirectX::XMFLOAT3 normal;
		float distance;
	};

	//ワールド変換行列を考慮したバウンディング球体の可視性チェック
	bool IsTransformedSphereVisible(const DirectX::XMFLOAT3& center, float radius, const DirectX::XMFLOAT4X4& worldTransform) const;

private:
	Plane planes[6]; //フラスタムの6つの平面(左、右、上、下、前、後)

	//平面と点の距離を計算
	float DistanceToPlane(const Plane& plane, const DirectX::XMFLOAT3& point) const;
};