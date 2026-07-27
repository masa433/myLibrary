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

using json = nlohmann::json;

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
	void RenderPlayer(const RenderContext& rc, ModelRenderer* renderer);
	void RenderBat(const RenderContext& rc, ModelRenderer* renderer);
    void DrawGUI();

    bool IsRightBatter() const { return isRightBatter; } // 右打者かどうかを判定するメソッド

    void SetBezierPitching(bool isPitching) { isBezierPitching = isPitching; } // ベジェ曲線投球中フラグを設定

    void SaveToJson(json& j);
    void LoadFromJson(const json& j);

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

    void UpdateBatterModel();

public:
    physx::PxRigidDynamic* GetBatCollider() const { return pxBatRigidBody; }

    enum class State
    {
        BattingIdle,
        BeforeSwing,
        Swinging,
        Idle,
        Count
    };

    enum Animation
    {
        BattingIdle,
        BeforeSwing,
        Swing,
    };

    void ChangeState(State newState);

private:
    // モデル関連
    std::unique_ptr<Model> bat;
	std::unique_ptr<gltf_model> batModel;

    DirectX::XMFLOAT4X4 batTransform = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
    DirectX::XMFLOAT3   batPosition = { 0,0,0 };
    DirectX::XMFLOAT3   batScale = { 1,1,1 };
    DirectX::XMFLOAT3   batAngle = { 0,0,0 };

    std::unique_ptr<gltf_model> rightBatter;
	std::unique_ptr<gltf_model> leftBatter;
    std::vector<gltf_model::node> animated_nodes;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;

    gltf_model* currentBatter = nullptr;

    // アニメーション関連
    float animation_time = 0.0f;
    int current_animation_index = 0;
    bool animation_playing = true;

    // ベジェ曲線投球用
    bool isBezierPitching = false; // ベジェ曲線投球中かどうか
    float beforeSwingStartTime = 0.0f; // BeforeSwingアニメーション開始時のanimation_time

    // ステートマシン関連
    State current_state = State::BattingIdle;
    State previous_state = State::BattingIdle;

    // ステートごとのアニメーションインデックス（Initialize内で設定）
    int animation_indices[static_cast<int>(State::Count)] = { 0, 0, 0, 0 };

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
	bool hasPlayBeforeSwing = false;
	bool isRightBatter = false; // 右打者かどうかのフラグ

	
public:

	bool isPurpleBat = false; // 紫色のバットに当たったかどうかのフラグ

    bool IsSwinging() const { return current_state == State::Swinging; } // スイング中かどうかを判定するメソッド


public:
    // コンソールログへのポインタをセット
    void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
    std::vector<std::string>* consoleLog = nullptr;


public:

    enum class RealBatter
    {
        None,
        Ishiyama,//石山
		Sato,//佐藤
		Okamura,//岡村
		Sakamoto,//坂本
		Odakura,//小田倉
		Kawano,//川野
		Murakami,//村上
		Yamada,//山田
		Suzuki,//鈴木
		Asano,//浅野
		Kimura,//木村
		Matsuda,//松田
		Murai,//村井
		Sasaki,//佐々木
		Takeda,//武田
		Hasegawa,//長谷川
		Nishimura,//西村
		Kojima,//小島
		Nishino,//西野
		Tamura,//田村
		Shimizu,//清水
		Yamashita,//山下
        Nakamura,//中村
		Ueda,//上田
        Count
    };

    enum class BatterPowerRank
    {
		C, // Cランク
        B,
		A,
        S
    };

    inline BatterPowerRank GetPowerRank(int power)
    {
        //90～99はSランク、80以上はAランク、70以上はBランク、60以上はCランク、それ以下はFランク
        if(power >= 90)
            return BatterPowerRank::S;
        else if (power >= 80)
            return BatterPowerRank::A;
        else if (power >= 70)
            return BatterPowerRank::B;
        else if (power >= 60)
            return BatterPowerRank::C;
        else
			return BatterPowerRank::C; //Cランクに統一
    }

    enum class BatterContactRank
	{
        C, // Cランク
		B,
		A,
		S
	};

    inline BatterContactRank GetContactRank(int contact)
    {
        //90～99はSランク、80以上はAランク、70以上はBランク、60以上はCランク、それ以下はFランク
        if(contact >= 90)
            return BatterContactRank::S;
        else if (contact >= 80)
            return BatterContactRank::A;
        else if (contact >= 70)
            return BatterContactRank::B;
        else if (contact >= 60)
            return BatterContactRank::C;
		else
            return BatterContactRank::C; //Cランクに統一
	}

    struct RealArsenalInfo
    {
		int power = 0; // 威力
        int contact = 0; // ミート
    };

	void SelectRealBatter(RealBatter batter);
	RealBatter GetSelectedRealBatter() const { return selectedRealBatter; }
	static const char* GetRealBatterName(RealBatter batter);

    int GetSelectedRealBatterPower() const
    {
        if (realBatterInfo.empty()) return 70;
		return realBatterInfo[0].power; // 仮に1つ目の情報を返す
    }

    void SetSelectedRealBatterPower(int power)
    {
        if (realBatterInfo.empty()) return;
        realBatterInfo[0].power = power; // 仮に1つ目の情報を設定
    }

    int GetSelectedRealBatterContact() const
    {
        if (realBatterInfo.empty()) return 70;
        return realBatterInfo[0].contact; // 仮に1つ目の情報を返す
    }

    void SetSelectedRealBatterContact(int contact)
    {
        if (realBatterInfo.empty()) return;
        realBatterInfo[0].contact = contact; // 仮に1つ目の情報を設定
    }

private:
    RealBatter selectedRealBatter = RealBatter::None;
	std::vector<RealArsenalInfo> realBatterInfo;

public:
    static bool GetRealBatterArsenalData(RealBatter rb, std::vector<RealArsenalInfo>& outArsenal, bool& outIsRight, const char*& outName);

    inline static const std::unordered_map<RealBatter, int> batterToSpriteIndexTable =
    {
        { RealBatter::Ishiyama, 0 },
        { RealBatter::Sato, 1 },
        { RealBatter::Okamura, 2 },
        { RealBatter::Sakamoto, 3 },
        { RealBatter::Odakura, 4 },
        { RealBatter::Kawano, 5 },
        { RealBatter::Murakami, 6 },
        { RealBatter::Yamada, 7 },
        { RealBatter::Suzuki, 8 },
        { RealBatter::Asano, 9 },
        { RealBatter::Kimura, 10 },
        { RealBatter::Matsuda, 11 },
        { RealBatter::Murai, 12 },
        { RealBatter::Sasaki, 13 },
        { RealBatter::Takeda, 14 },
        { RealBatter::Hasegawa, 15 },
        { RealBatter::Nishimura, 16 },
        { RealBatter::Kojima, 17 },
        { RealBatter::Nishino, 18 },
        { RealBatter::Tamura, 19 },
        { RealBatter::Shimizu, 20 },
        { RealBatter::Yamashita, 21 },
        { RealBatter::Nakamura, 22 },
        { RealBatter::Ueda, 23 },

    };
};