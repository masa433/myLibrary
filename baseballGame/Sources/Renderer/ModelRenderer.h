#pragma once

#include <memory>
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "../Model/Model.h"
#include "Light.h"
#include "../Model/ModelResource.h"
#include "RenderState.h"
#include "RenderContext.h"
#include "ShaderId.h"


enum class ShaderId
{
	Basic,
	Lambert,
	EnumCount
};

class ModelRenderer
{
public:
	ModelRenderer(ID3D11Device* device);
	~ModelRenderer() {}

	// 描画実行
	void Render(const RenderContext& rc, const DirectX::XMFLOAT4X4& worldTransform, const Model* model, ShaderId shaderId);

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4		viewProjection;
		DirectX::XMFLOAT4		lightDirection;
		DirectX::XMFLOAT4		lightColor;
		DirectX::XMFLOAT4		ambientColor;

		//ポイントライト
		DirectX::XMFLOAT3 	 pointLightPosition;
		float pointLightRange;
		DirectX::XMFLOAT3 	 pointLightColor;
		float pad0;

		//スポットライト
		DirectX::XMFLOAT3 	 spotLightPosition;
		DirectX::XMFLOAT3 	 spotLightDirection;
		DirectX::XMFLOAT3 	 spotLightColor;
		float spotLightRange;
		float spotLightInnerConeAngle;
		float spotLightOuterConeAngle;

		DirectX::XMFLOAT4		cameraPosition;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4		boneTransforms[256];
	};

	std::unique_ptr<Shader>					shaders[static_cast<int>(ShaderId::EnumCount)];

	Microsoft::WRL::ComPtr<ID3D11Buffer>	sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	skeletonConstantBuffer;
};
