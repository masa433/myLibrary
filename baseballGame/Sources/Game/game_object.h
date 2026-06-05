#pragma once
#include "DirectXMath.h"
#include "../Model/gltf_model.h"
#include <vector>
#include <memory>
class GameObject
{
protected:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 angle;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 front;
	DirectX::XMFLOAT4X4 transform;
	std::shared_ptr<gltf_model> model;
	float radius = 0.0f;
	float height = 0.0f;
public:
	GameObject()
		:position(0, 0, 0), angle(0, 0, 0), scale(1, 1, 1) {
	}

	virtual ~GameObject() {}

	//位置を設定
	DirectX::XMFLOAT3 GetPosition() const { return position; }

	//位置を取得
	void SetPosition(DirectX::XMFLOAT3& position) { this->position = position; }

	//向きを取得
	DirectX::XMFLOAT3 GetAngle() const { return angle; }

	//向きを設定
	void SetAngle(DirectX::XMFLOAT3& angle) { this->angle = angle; }

	//スケールを取得
	DirectX::XMFLOAT3 GetScale() const { return scale; }

	//スケールを設定
	void SetScale(DirectX::XMFLOAT3& scale) { this->scale = scale; }

	//横方向ベクトル取得
	DirectX::XMFLOAT3 GetRight() const { return right; }

	//上方向ベクトル取得
	DirectX::XMFLOAT3 GetUp() const { return up; }

	//前方向ベクトル取得
	DirectX::XMFLOAT3 GetFront() const { return front; }

	//位置行列取得
	DirectX::XMFLOAT4X4 GetTransform() const { return transform; }

	//位置更新
	void UpdateTransform();

	//モデル設定
	void SetModel(std::shared_ptr<gltf_model> model) { this->model = model; }

	//モデル取得
	std::shared_ptr<gltf_model> GetModel() const { return model; }

	//当たり判定用の半径設定
	void SetRadius(float radius) { this->radius = radius; }

	//当たり判定用の高さ設定
	void SetHeight(float height) { this->height = height; }

	// 半径取得
	float GetRadius() const { return radius; }

	// 高さ取得
	float GetHeight() const { return height; }

};