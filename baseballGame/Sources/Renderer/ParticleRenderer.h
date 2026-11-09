#pragma once
#include "particle_system.h"
#include "shader.h"
#include "texture.h"
#include "sprite.h"

using json = nlohmann::json;

class ParticleRenderer
{
public:

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
	void Uninitialize();
	void Update(float elapsed_time,bool isHomeRun);
	void Render(ID3D11DeviceContext* immediate_context);
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

private:
	std::unique_ptr<particle_system> particles;
	std::vector<DirectX::XMFLOAT3> particle_positions;
	std::vector<DirectX::XMFLOAT3> fireworks_positions;
	bool isFireworksActive = false;
	float fireworksTimer = 0.0f;
	int fireworksCount = 0;
	const int maxFireworksCount = 5;
	bool isLaunchingFireworks = false;
	int loopCount = 0;
	int maxLoopCount = 2;
};