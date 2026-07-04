#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "../Model/gltf_model.h"
#include "game_object.h"
#include "RenderContext.h"
#include "physxManager.h"
#include "ModelRenderer.h"

class Flag :  public GameObject
{
public:
	enum class FlagColor
	{
		Blue,
		Green,
		Japan,
		Red,
		Yellow,
	};

	// index      : 5本の中で何本目か(0～4)。等間隔配置の位置計算に使う
	// color      : 読み込む旗の色
	// spacing    : 旗同士の間隔(m)
	// basePosition, baseYAngleDeg : 5本全体を置きたい場所・向き
	void Initialize(int index, FlagColor color, float spacing,
		const DirectX::XMFLOAT3& basePosition = { 0.0f, 0.0f, 140.0f },
		float baseYAngleDeg = 180.0f);
	void UnInitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI(int index);

private:
	static const char* GetModelPath(FlagColor color);


	std::unique_ptr<gltf_model> flagModel;
	FlagColor flagColor = FlagColor::Red;

	float windTime = 0.0f;
	float windPhaseOffset = 0.0f; // 旗ごとに位相をずらして全部同じ動きにならないようにする

	Microsoft::WRL::ComPtr<ID3D11Buffer> windConstantBuffer;

	bool usePhysicsSimulation = true; // 物理シミュレーションを使用するか
	float springStiffness = 50.0f;    // バネ剛性
	float springDamping = 0.5f;       // バネ減衰
	float particleMass = 0.1f;        // パーティクル質量
};