#pragma once
#include "game_object.h"
#include "gltf_model.h"
#include <DirectXMath.h>
#include "RenderContext.h"
#include "physxManager.h"
#include "Model.h"
#include "ModelRenderer.h"



class Player : public GameObject
{
	//インスタンス
public:
    static Player& Instance()
    {
        static Player instance;
        return instance;
	}

    void Initialize();
    void Uninitialize() ;
    void Update(float elapsedTime);
    void Render(const RenderContext& rc, ModelRenderer* renderer);
    void DrawGUI();

    bool IsRightBatter() const { return isRightBatter; } // 右打者かどうかを判定するメソッド

private:
    // キー入力処理
    void HandleInput(float elapsedTime);
    
    // アタッチメント処理
    void AttachBatToHand();

	void UpdateAnimation(float elapsedTime);

	

    void UpdateLookAt(const DirectX::XMFLOAT3& targetPosition); // 頭のルックアット処理

    // ボーン操作用メソッド
    void ModifyArmBones();
    void UpdateNodeTransform(int nodeIndex, const DirectX::XMMATRIX& additionalRotation);
    void UpdateChildrenRecursive(int nodeIndex);

    void UpdatePhysXMeshTransform(const DirectX::XMFLOAT3& scale);

	void SetBattingIdleState();

	void UpdateBattingIdleState(float elapsedTime);

	void SetSwingState();

	void UpdateSwingState(float elapsedTime);

public:
    physx::PxRigidDynamic* GetBatCollider() const { return pxBatRigidBody; }

    enum class State
    {
        BattingIdle,
        Swinging,
        HomeRun,
        Idle,
        Count
    };

    enum Animation
    {
        BattingIdle,
        HomeRun,
        Swing,
    };

    void ChangeState(State newState);

private:
    // モデル関連
    std::unique_ptr<Model> bat;

    DirectX::XMFLOAT4X4 batTransform = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
    DirectX::XMFLOAT3   batPosition = { 0,0,0 };
    DirectX::XMFLOAT3   batScale = { 1,1,1 };
    DirectX::XMFLOAT4   batAngle = { 0,0,0,1 };

    float batRadius = 0.0f;
	float batHeight = 0.0f;

    std::unique_ptr<gltf_model> animated_model;
    std::vector<gltf_model::node> animated_nodes;
    std::unique_ptr<Model> batter;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;

    // アニメーション関連
    float animation_time = 0.0f;
    int current_animation_index = 0;
    bool animation_playing = true;

    // ステートマシン関連
    State current_state = State::BattingIdle;
    State previous_state = State::BattingIdle;

    // ステートごとのアニメーションインデックス（Initialize内で設定）
    int animation_indices[static_cast<int>(State::Count)] = { 0, 0, 0, 0 };

    // 移動速度
    float move_speed = 5.0f;

	//ルックアット処理関連
    DirectX::XMFLOAT3			headLocalForward = { 0, 0, 1 };	// 頭のローカル前方向
    int							headNodeIndex = -1;				// 頭ノードのインデックス
    bool						enableLookAt = true;			// ルックアット有効フラグ
    DirectX::XMFLOAT3					targetPosition = { 0, 2, 1 };

	float gravity = -9.8f;
    DirectX::XMFLOAT3 previousBatPosition = { 0.0f,0.0f,0.0f };



	float swingHeight = 0.0f;
	float swingWidth = 5.0f;
    float armAngleOffset = 0.0f; // 腕の角度オフセット（追加）
    float swingStartTime = 0.0f; // スイング開始からの経過時間

    physx::PxCapsuleController* pxPlayerCapsuleController = nullptr;
    physx::PxConvexMesh* pxBatConvexMesh = nullptr;
    physx::PxRigidDynamic* pxBatRigidBody = nullptr;
    bool isOnGround = true; // 地面にいるかどうか

    DirectX::XMFLOAT3 meshScale = { 0.0f,0.0f,0.0f };

    physx::PxMaterial* pxBatMaterial = nullptr;//バット専用のマテリアル

    float ThrowingStateTime = 0.0f;
	bool hasPlayHomeRun = false;
	bool isRightBatter = true; // 右打者かどうかのフラグ
};