#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include "FontRenderer.h"
#include "sprite.h"
#include "json.hpp"
#include <random>

#define ABILITY_COUNT 17

using json = nlohmann::json;

class SpecialAbility
{
public:
	static SpecialAbility& Instance()
	{
		static SpecialAbility instance;
		return instance;
	}

	enum class AbilityID
	{
		WideAngleBatting,//広角打法
		CenterReturn,//センター返し
		PullHitter,//プルヒッター
		OppositeHitter,//流し打ち
		RomanCannon,//ロマン砲
		HighBallHitter,//ハイボールヒッター
		LowBallHitter,//ローボールヒッター
		LastStand,//背水の陣
		FullSwing,//フルスイング
		LastBall,//ラストボール
		FirstPitcher,//初球
		Combo,//連発
		MoneyMaker,//マネーメーカー
		JackPot,//一攫千金
		Intimidation,//威圧感
		VsFastBall,//対速球
		VsBreakingBall,//対変化球
		Count
	};
	static_assert(static_cast<int>(AbilityID::Count) == ABILITY_COUNT, "ABILITY_COUNTとenumの数が一致していません");

	struct BattingBonus
	{
		float power = 0.0f; // 打撃力ボーナス
		float contact = 0.0f; // ミート力ボーナス
		float ballSpeed = 0.0f; // 球速ボーナス
	};

	struct MoneyBonus
	{
		float percentBonus = 0.0f; // 獲得金額のボーナス率
		bool jackPotTriggered = false; // 一攫千金が発動したかどうか
		float jackPotMultiplier = 0.0f; // 一攫千金の倍率
	};

	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	void InitializeAbilities(ID3D11Device* device, ID3D11DeviceContext* context);
	void BuildAbility();

	bool IsOwned(AbilityID id) const { return abilities[(int)id].isOwned; }
	void SetOwned(AbilityID id, bool owned) { abilities[(int)id].isOwned = owned; }

	bool RollActivation(float ratePercent);// 能力の発動判定を行う関数
	void RollRoundActivation(); // ラウンドごとの能力発動判定を行う関数

	//ランダムな3つの特殊能力のIDを表示する関数
	std::vector<AbilityID> GetRandomAbilities(int count)
	{
		std::vector<AbilityID> allAbilities;
		for (int i = 0; i < ABILITY_COUNT; ++i)
		{
			// 所有していない能力のみを対象にする
			if (!abilities[i].isOwned)
			{
				allAbilities.push_back(static_cast<AbilityID>(i));
			}

			//方向系、高低系、球種系は同タイプのものをすでに持っていたら除外する
			if (abilities[i].type == AbilityType::Directional)
			{
				bool hasDirectional = false;
				for (const auto& ability : abilities)
				{
					if (ability.type == AbilityType::Directional && ability.isOwned)
					{
						hasDirectional = true;
						break;
					}
				}
				if (hasDirectional)
				{
					allAbilities.erase(std::remove(allAbilities.begin(), allAbilities.end(), static_cast<AbilityID>(i)), allAbilities.end());
				}
			}
			else if (abilities[i].type == AbilityType::Height)
			{
				bool hasHeight = false;
				for (const auto& ability : abilities)
				{
					if (ability.type == AbilityType::Height && ability.isOwned)
					{
						hasHeight = true;
						break;
					}
				}
				if (hasHeight)
				{
					allAbilities.erase(std::remove(allAbilities.begin(), allAbilities.end(), static_cast<AbilityID>(i)), allAbilities.end());
				}
			}
			else if (abilities[i].type == AbilityType::PitchType)
			{
				bool hasPitchType = false;
				for (const auto& ability : abilities)
				{
					if (ability.type == AbilityType::PitchType && ability.isOwned)
					{
						hasPitchType = true;
						break;
					}
				}
				if (hasPitchType)
				{
					allAbilities.erase(std::remove(allAbilities.begin(), allAbilities.end(), static_cast<AbilityID>(i)), allAbilities.end());
				}
			}
		}
		std::shuffle(allAbilities.begin(), allAbilities.end(), rng);// ランダムにシャッフル
		if (static_cast<int>(allAbilities.size()) > count)
		{
			allAbilities.resize(count); // countがallAbilitiesのサイズより大きい場合、サイズを調整
		}
		return allAbilities;
	}

	//選択画面用のアイコンを描画する関数
	void RenderAbilityIcons(AbilityID id, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size, bool isHighlighted);

	const std::string& GetName(AbilityID id) const { return abilities[(int)id].name; }

	//打球速度ボーナスの増減関数
	float GetBallVelocityBonus() const;

	//マネーメーカー能力のボーナスを取得する関数
	float GetMoneyMakerBonus() const;

	//一攫千金ボーナスを発動するかの抽選
	float RollJackPotMultiplier();

	//マネーメーカーがアクティブかどうか
	bool IsMoneyMakerActive() const
	{ 
		const auto& ability = abilities[(int)AbilityID::MoneyMaker];
		return ability.isMoneyMakerActive && ability.isActiveThisRound && ability.isOwned; 
	}

	//一攫千金がアクティブかどうか
	bool IsJackPotActive() const
	{
		const auto& ability = abilities[(int)AbilityID::JackPot];
		return  ability.isJackPotActive && ability.isActiveThisRound && ability.isOwned;
	}

	void TriggerShowAbilities();

private:

	//テクスチャ関連
	struct Sprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};


	struct AbilitySprite
	{
		std::unique_ptr<Sprite> spriteData;
		std::unique_ptr<sprite> sprite;
		
		DirectX::XMFLOAT2 iconPosition = { 0.0f, 900.0f };
		DirectX::XMFLOAT2 iconSize = { 100.0f, 100.0f };

	};

	std::unique_ptr<AbilitySprite> abilitySprites[ABILITY_COUNT];

	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   spriteInputLayout;

	//特殊能力のタイプ
	enum class AbilityType
	{
		None,        // 無し
		Directional, // 方向系能力
		Height,      // 高低系能力
		PitchType,   // 球種系能力
		Money,      // お金系能力
		PowerContact, // 打撃力・ミート力系能力
		Situation,    // 状況系能力
		PitcherPenalty, // 投手へのペナルティ系能力
	};

	struct AbilityData
	{
		
		std::wstring texturePath;//テクスチャのファイルパス
		std::string name; // 能力の名前
		float power = 0.0f; // 打撃力ボーナス
		float contact = 0.0f; // ミート力ボーナス
		std::function<bool()> condition; // 能力の発動条件

		float ballSpeed = 0.0f; // 球速ボーナス
		std::function<bool()> ballCondition; // 球速ボーナスの発動条件
		float ballSpeedPenalty = 0.0f; // 球速ボーナスのペナルティ
		std::function<bool()> ballPenaltyCondition; // 球速ボーナスのペナルティ条件

		float activationRate = 0.0f; // 能力の発動率(0～100)
		bool isOwned = false; // 能力を所有しているかどうか
		bool isHovered = false; // マウスオーバー状態かどうか

		float comboPowerPerStack = 0.0f; // 連発能力のスタックごとの打撃力ボーナス
		float moneyMakerBonus = 0.0f; // マネーメーカー能力のボーナス
		float jackPotMultiplier = 0.0f; // 一攫千金能力の倍率
		float jackPotChance = 0.0f; // 一攫千金能力の発動確率
		float pitcherPowerPenalty = 0.0f; // 威圧感能力の投手へのペナルティ
		float pitcherBreakBallPenalty = 0.0f; // 威圧感能力の変化球へのペナルティ
		float pitcherBallSpeedPenalty = 0.0f; // 威圧感能力の球速へのペナルティ

		bool isActiveThisRound = false; // 今回のラウンドで能力が発動したかどうか
		bool isMoneyMakerActive = false;
		bool isJackPotActive = false; // 一攫千金が発動したかどうか

		AbilityType type = AbilityType::None; // 能力のタイプ

	};

	AbilityData abilities[ABILITY_COUNT];

	std::mt19937 rng{ std::random_device{}() }; // 乱数生成器

private:

	enum class Direction { Pull, Center, Opposite };
	Direction direction = Direction::Center;

	//条件判定ヘルパー
	bool IsDirection(Direction direction) const;//方向判定
	bool IsHighBall() const;//高めのボール判定
	bool IsLowBall() const;//低めのボール判定
	bool IsFastBall() const;//速球判定
	bool IsBreakingBall() const;//変化球判定
	bool IsFirstPitch() const;//初球判定
	bool IsLastBall() const;//ラストボール判定
	bool ComboCount() const;//連発判定
	bool IsLastStandCondition() const;//背水の陣判定
	bool IsHomeRunCondition() const;//ホームラン判定

public:
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;
};