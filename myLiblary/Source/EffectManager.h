#pragma once

#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

class EffectManager 
{
private:
	EffectManager(){}
	~EffectManager(){}

public:
	static EffectManager& Instance()
	{
		static EffectManager instance;
		return instance;
	}

	void Initialize();

	void Finalize();

	void Update(float elapsedTime);

	void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

	//Effekseerのマネージャーの取得
	Effekseer::ManagerRef GetEffekseerManager() { return effekseerManager; }

private:
	Effekseer::ManagerRef effekseerManager;
	EffekseerRenderer::RendererRef effekseerRenderer;
};
