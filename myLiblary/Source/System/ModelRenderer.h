#pragma once

#include "Model.h"
#include "Shader.h"

enum class ShaderId
{
	Basic,
	Lambert,
	Phong,
	ShadowMap,
	EnumCount
};

class ModelRenderer
{
public:
	ModelRenderer(ID3D11Device* device);
	~ModelRenderer() {}

	// ï`âÊé¿çs
	void Render(const RenderContext& rc, const DirectX::XMFLOAT4X4& worldTransform, const Model* model, ShaderId shaderId);

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4		viewProjection;
		DirectX::XMFLOAT4		lightDirection;
		DirectX::XMFLOAT4       cameraPosition;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4		boneTransforms[256];
	};

	std::unique_ptr<Shader>					shaders[static_cast<int>(ShaderId::EnumCount)];

	Microsoft::WRL::ComPtr<ID3D11Buffer>	sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	skeletonConstantBuffer;
};
