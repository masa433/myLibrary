#pragma once
#include "game_object.h"
#include "../Model/Model.h"
#include <DirectXMath.h>
#include "RenderContext.h"
#include "physxManager.h"
#include "Model.h"
#include "ModelRenderer.h"
#include "../Model/gltf_model.h"
#include "json.hpp"
#include "HitJudge2D.h"

class Catcher : public GameObject
{
public:
	//インスタンス
	static Catcher& Instance()
	{
		static Catcher instance;
		return instance;
	}
	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	void AttachMittToHand();

	DirectX::XMFLOAT3 GetMittWorldPosition() const
	{
		return { mittTransform._41, mittTransform._42, mittTransform._43 };//ミットのワールド座標を返す
	}

private:
	//キャッチャーのモデル
	std::shared_ptr<gltf_model> catcherModel;
	//キャッチャーの位置と角度
	DirectX::XMFLOAT3 catcherPosition;
	DirectX::XMFLOAT3 catcherAngle;
	//キャッチャーのスケール
	DirectX::XMFLOAT3 catcherScale;
	DirectX::XMFLOAT4X4 catcherTransform = DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	//キャッチャーのアニメーション関連
	std::vector<gltf_model::node> animatedNodes;


	std::unique_ptr<gltf_model> catcherMitt; //キャッチャーのミットモデル
	DirectX::XMFLOAT3 mittPosition; //ミットの位置
	DirectX::XMFLOAT3 mittAngle; //ミットの角度
	DirectX::XMFLOAT3 mittScale; //ミットのスケール
	DirectX::XMFLOAT4X4 mittTransform = DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	
};