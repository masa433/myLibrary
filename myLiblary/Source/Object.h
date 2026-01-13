#pragma once

#include"System/ModelRenderer.h"
#include "System/ShapeRenderer.h"
#include"Collision.h"

class Object 
{
	public:
		Object();
		~Object();

		static Object& Instance()
		{
			static Object instance;
			return instance;
		}

		void Update(float elapsedTime);
		void Render(const RenderContext& rc, ModelRenderer* renderer);

		bool RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit);

		void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

		void DrawImGui();



private:
	std::unique_ptr<Model> net = nullptr;

	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 scale = { 1,1,1 };
	DirectX::XMFLOAT3 rotation = { 0, 0, 0 };
	DirectX::XMFLOAT4X4 transform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
};