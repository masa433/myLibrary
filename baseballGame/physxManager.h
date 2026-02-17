#pragma once

#include <vector>
#include <DirectXMath.h>
#include <PxPhysicsAPI.h>

// フィジクス
class Physics
{
private:
	Physics() = default;
	~Physics() = default;

public:
	// インスタンス取得
	static Physics& Instance()
	{
		static Physics instance;
		return instance;
	}

	// 初期化
	void Initialize();

	// 終了化
	void Finalize();

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection, const DirectX::XMFLOAT3& lightDirection);

	// フィジクス取得
	physx::PxPhysics* GetPhysics() { return pxPhysics; }

	// シーン取得
	physx::PxScene* GetScene() { return pxScene; }

	// コントローラーマネージャー取得
	physx::PxControllerManager* GetControllerManager() { return pxControllerManager; }

	// マテリアル取得
	physx::PxMaterial* GetMaterial() { return pxMaterial; }

private:

	physx::PxDefaultAllocator			pxAllocator;
	physx::PxDefaultErrorCallback		pxErrorCallback;
	physx::PxFoundation* pxFoundation = nullptr;
	physx::PxPhysics* pxPhysics = nullptr;
	physx::PxDefaultCpuDispatcher* pxDispatcher = nullptr;
	physx::PxScene* pxScene = nullptr;
	physx::PxControllerManager* pxControllerManager = nullptr;

	physx::PxMaterial* pxMaterial = nullptr;

	physx::PxPvd* pxPvd = nullptr;

	struct Line
	{
		DirectX::XMFLOAT3	start;
		DirectX::XMFLOAT3	end;
		DirectX::XMFLOAT4	color;
	};

	struct Capsule
	{
		DirectX::XMFLOAT4X4	transform;
		float				radius;
		float				height;
		DirectX::XMFLOAT4	color;
	};
	std::vector<Line>		lines;
	std::vector<Capsule>	capsules;
};
