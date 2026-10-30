#pragma once

// サンプラステート
enum class SamplerState
{
	PointWrap,
	PointClamp,
	LinearWrap,
	LinearClamp,
	AnisotropicWrap,
	AnisotropicClamp,

	EnumCount
};

// デプスステート
enum class DepthState
{
	TestAndWrite,
	TestOnly,
	WriteOnly,
	NoTestNoWrite,

	EnumCount
};

// ブレンドステート
enum class BlendState
{
	Opaque,
	Transparency,
	Additive,
	Subtraction,
	Multiply,

	EnumCount
};

// ラスタライザステート
enum class RasterizerState
{
	SolidCullNone,
	SolidCullBack,
	WireCullNone,
	WireCullBack,

	EnumCount
};

// レンダーステート
class RenderState
{
public:
	RenderState(ID3D11Device* device);
	~RenderState() = default;

	bool IsValid() const
	{
		for (const auto& state : samplerStates) if (!state) return false;
		for (const auto& state : depthStencilStates) if (!state) return false;
		for (const auto& state : blendStates) if (!state) return false;
		for (const auto& state : rasterizerStates) if (!state) return false;
		return true;
	}

	// サンプラステート取得
	ID3D11SamplerState* GetSamplerState(SamplerState state) const
	{
		const int index = static_cast<int>(state);
		return index >= 0 && index < static_cast<int>(SamplerState::EnumCount) ? samplerStates[index].Get() : nullptr;
	}

	// デプスステート取得
	ID3D11DepthStencilState* GetDepthStencilState(DepthState state) const
	{
		const int index = static_cast<int>(state);
		return index >= 0 && index < static_cast<int>(DepthState::EnumCount) ? depthStencilStates[index].Get() : nullptr;
	}

	// ブレンドステート取得
	ID3D11BlendState* GetBlendState(BlendState state) const
	{
		const int index = static_cast<int>(state);
		return index >= 0 && index < static_cast<int>(BlendState::EnumCount) ? blendStates[index].Get() : nullptr;
	}

	// ラスタライザーステート取得
	ID3D11RasterizerState* GetRasterizerState(RasterizerState state) const
	{
		const int index = static_cast<int>(state);
		return index >= 0 && index < static_cast<int>(RasterizerState::EnumCount) ? rasterizerStates[index].Get() : nullptr;
	}

private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState>		samplerStates[static_cast<int>(SamplerState::EnumCount)];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilStates[static_cast<int>(DepthState::EnumCount)];
	Microsoft::WRL::ComPtr<ID3D11BlendState>		blendStates[static_cast<int>(BlendState::EnumCount)];
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>	rasterizerStates[static_cast<int>(RasterizerState::EnumCount)];
};
