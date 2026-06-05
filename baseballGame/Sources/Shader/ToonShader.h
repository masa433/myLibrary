#pragma once

#include "ShaderId.h"
#include "../Model/ModelResource.h"

class ToonShader : public Shader
{
public:
	ToonShader(ID3D11Device* device);
	~ToonShader() override = default;
	// 開始処理
	void Begin(const RenderContext& rc) override;

	// 更新処理
	void Update(const RenderContext& rc, const ModelResource::Material& material) override;

	// 終了処理
	void End(const RenderContext& rc) override;

private:
	struct CbMaterial
	{
		DirectX::XMFLOAT4		materialColor;
	};

	Microsoft::WRL::ComPtr<ID3D11VertexShader>		vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>		pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>		inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			constantBuffer;
};
