#pragma once
#include <DirectXMath.h>
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

class EffectManager
{
private:
	EffectManager() {}
	~EffectManager() {}

public:
	static EffectManager& Instance()
	{
		static EffectManager instance;
		return instance;
	}
	

	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);
	//Effekseerのマネージャーを取得するための関数
	Effekseer::ManagerRef GetEffekseerManager() { return effekseerManager; }

private:
	Effekseer::ManagerRef effekseerManager;// Effekseerのマネージャーを取得するための関数
	EffekseerRenderer::RendererRef effekseerRenderer;// Effekseerのレンダラーを取得するための関数
};