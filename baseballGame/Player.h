#pragma once
#include "game_object.h"
#include "gltf_model.h"
#include <DirectXMath.h>
#include "RenderContext.h"
#include "physxManager.h"

enum class State 
{
    BattingIdle,
	Swinging,
	HomeRun,
    Idle,
	Count
};

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
    void Update(float elapsedTime) ;
    void Render(RenderContext& rc);
    void DrawGUI();

private:
    // キー入力処理
    void HandleInput(float elapsedTime);
    
    // アタッチメント処理
    void AttachBatToHand();

	void UpdateAnimation(float elapsedTime);

	void ChangeState(State newState);

    void UpdateLookAt(const DirectX::XMFLOAT3& targetPosition); // 頭のルックアット処理

    //バットとボールの当たり判定
	void CheckBatAndBallCollision(float elapsedTime);

    // ボーン操作用メソッド
    void ModifyArmBones();
    void UpdateNodeTransform(int nodeIndex, const DirectX::XMMATRIX& additionalRotation);
    void UpdateChildrenRecursive(int nodeIndex);

private:
    // モデル関連
    std::unique_ptr<gltf_model> bat;

    DirectX::XMFLOAT4X4 batTransform = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
    DirectX::XMFLOAT3   batPosition = { 0,0,0 };
    DirectX::XMFLOAT3   batScale = { 1,1,1 };
    DirectX::XMFLOAT3   batAngle = { 0,0,0 };

    float batRadius = 4.0f;
	float batHeight = 85.4f;

    std::unique_ptr<gltf_model> animated_model;
    std::vector<gltf_model::node> animated_nodes;
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

    // 追加: シリンダーの底面位置オフセット
    DirectX::XMFLOAT3 cylinderOffset = { 0.0f, 0.0f, 0.0f };

	float swingHeight = 0.0f;
	float swingWidth = 5.0f;
    float armAngleOffset = 0.0f; // 腕の角度オフセット（追加）
    float swingStartTime = 0.0f; // スイング開始からの経過時間

    physx::PxCapsuleController* pxCapsuleController = nullptr;
    bool isOnGround = true; // 地面にいるかどうか
};