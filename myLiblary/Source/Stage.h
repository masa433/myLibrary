#pragma once

#include"System/ModelRenderer.h"
#include"Collision.h"

// Source/Stage.h

class Stage {
public:
    Stage();
    ~Stage();

    static Stage& Instance()
    {
        static Stage instance;
        return instance;
    }

    void Update(float elapsedTime);
    void Render(const RenderContext& rc, ModelRenderer* renderer);

    // モデル（表示用）のパラメータ
    void SetModelPosition(const DirectX::XMFLOAT3& pos) { modelParam.position = pos; }
    DirectX::XMFLOAT3 GetModelPosition() const { return modelParam.position; }
    void SetModelScale(const DirectX::XMFLOAT3& scl) { modelParam.scale = scl; }
    void SetModelRotation(const DirectX::XMFLOAT3& rtn) { modelParam.rotation = rtn; }

    // コリジョンモデル用
    void SetCollisionPosition(const DirectX::XMFLOAT3& pos) { collisionParam.position = pos; }
    DirectX::XMFLOAT3 GetCollisionPosition() const { return collisionParam.position; }
    void SetCollisionScale(const DirectX::XMFLOAT3& scl) { collisionParam.scale = scl; }
    void SetCollisionRotation(const DirectX::XMFLOAT3& rtn) { collisionParam.rotation = rtn; }
    void DrawImGui();
    // レイキャスト
    bool RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit);

private:

    struct ModelTransform
    {
        DirectX::XMFLOAT3 position = { 0, 0, 0 };
        DirectX::XMFLOAT3 scale = { 1,1,1 };
        DirectX::XMFLOAT3 rotation = { 0, 0, 0 };
        DirectX::XMFLOAT4X4 transform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    };

private:

    std::unique_ptr<Model> model = nullptr;
	std::unique_ptr<Model> collisionModel = nullptr;
    ModelTransform modelParam;
    ModelTransform collisionParam;
};