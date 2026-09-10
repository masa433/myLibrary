#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <memory>
#include <wrl.h>
#include "FontRenderer.h"
#include "sprite.h"
#include "json.hpp"
#include "Player.h"
#include "Pitcher.h"
#include "SpecialAbility.h"
#include <random>
#include <functional>
#include "..\Sources\Audio\AudioSource.h"
#include "..\Sources\Audio\Audio.h"

#define BATTER_COUNT 24
#define SHOP_ITEM_COUNT 22
#define SHOP_ITEM_DISPLAY_COUNT 7
#define SHOP_ITEM_RANDOM 6

using json = nlohmann::json;
class ShopManager
{
public:

	enum class ShopItemID
	{
		PowerUp,
		ContactUp,
		ContactAsist,
		BallIncrease,
		WindDisable,
		PitchPowerDown,
		PitchBreakDown,
		PitchTypeDecrease,
		HomeRunMultiplier,
		BreakingBallMultiplier,
		GravityChange,
		SpecialAbilityActiveRateUp,
		NetDecrease,
		HalfPrice,
		FreePrice,
		Reroll,
		BallZoneRateUp,
		BallZoneBonus,
	};

	static ShopManager& Instance()
	{
		static ShopManager instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);
	void InitializeShopButtonSprites(ID3D11Device* device, ID3D11DeviceContext* context);
	void BuildShopItem();
	void UpdateShopItem();

private:

	struct ShopSprite
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	std::unique_ptr<ShopSprite> shopBackSpriteData;
	std::unique_ptr<sprite> shopBackSprite;

	std::unique_ptr<sprite> soldOutSprite;
	
	DirectX::XMFLOAT2 soldOutSize = { 250.0f, 250.0f };

	std::unique_ptr<sprite> coinSprite;
	DirectX::XMFLOAT2 coinOffset = { 50.0f, 135.0f };
	DirectX::XMFLOAT2 coinSize = { 40.0f, 40.0f };

	std::unique_ptr<ShopSprite> nextRoundButtonData;
	std::unique_ptr<sprite> nextRoundButtonSprite;

	DirectX::XMFLOAT2 nextRoundButtonSize = { 200.0f, 50.0f };
	DirectX::XMFLOAT2 nextRoundButtonPosition = { 960.0f, 900.0f };

	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;


	float easingDuration = 1.0f; // イージングの時間（秒）
	float easingTimer = 0.0f; // イージングのタイマー
	bool isAnimating = false; // アニメーション中かどうかのフラグ
	bool isShopOpen = false; // ショップが開いているかどうかのフラグ
	bool isShopClosed = true; // ショップが閉じているかどうかのフラグ

	FontRenderer moneyFont;

	DirectX::XMFLOAT2 moneyFontPosition = { 1550.0f, 128.0f };	
	float moneyFontScale = 1.0f;
	DirectX::XMFLOAT4 moneyFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	int selectedBatterIndex = -1; // 選択されたバッターのインデックス

	FontRenderer fontRenderer;

	std::unique_ptr<ShopSprite> batterSpriteData[BATTER_COUNT];
	std::unique_ptr<sprite> batterSprites[BATTER_COUNT];

	DirectX::XMFLOAT2 batterParamBackPosition = { 1450.0f, 550.0f };
	DirectX::XMFLOAT2 batterParamBackSize = { 550.0f, 450.0f };

	FontRenderer priceFont;

	DirectX::XMFLOAT2 priceFontOffset = { 20.0f, 170.0f };
	float priceFontScale = 0.8f;
	DirectX::XMFLOAT4 priceFontColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	struct BatterParamFontData
	{
		DirectX::XMFLOAT2 position;
		float scale;
		DirectX::XMFLOAT4 color;
	};

	BatterParamFontData powerFontData;
	BatterParamFontData contactFontData;
	BatterParamFontData powerRankFontData;
	BatterParamFontData contactRankFontData;

	float currentOffsetY = -1080.0f; // 初期位置を画面外の上部に設定
	float startOffsetY = -1080.0f; // 初期位置を画面外の上部に設定
	float targetOffsetY = 0.0f; // 目標位置を画面中央に設定

	const DirectX::XMFLOAT2 baseShopBackPos = { 960.0f, 540.0f };
	const DirectX::XMFLOAT2 baseMoneyFontPos = { 1550.0f, 128.0f };
	const DirectX::XMFLOAT2 baseBatterParamPos = { 1450.0f, 550.0f };

public:
	void OpenShop()
	{
		isAnimating = true;
		isShopOpen = true;
		easingTimer = 0.0f;
		startOffsetY = currentOffsetY; // 現在位置を初期位置に設定
		targetOffsetY = 0.0f; // 目標位置を画面中央に設定

		for(int i = 0; i < SHOP_ITEM_DISPLAY_COUNT; ++i)
		{
			slotPurchased[i] = false; // 購入済みフラグをリセット
			slotHover[i] = false; // ホバーフラグをリセット
		}
		currentShopItemIndices = ShopLayout(); // ショップアイテムのレイアウトを更新
	}

	void CloseShop()
	{
		isAnimating = true;
		isShopClosed = true;
		isShopOpen = false;
		easingTimer = 0.0f;
		startOffsetY = currentOffsetY; // 現在位置を初期位置に設定
		targetOffsetY = -1080.0f; // 目標位置を画面外の上部に設定
		
	}

	bool IsShopOpen() const
	{
		return isShopOpen && !isAnimating;
	}

	bool IsClosing() const
	{
		return isShopClosed && isAnimating;
	}

	//ランダムで7このショップアイテムを選択する関数
	std::vector<int> GetRondomShopItem(int count)
	{
		std::vector<int> allItems;

		for (int i = 0; i < SHOP_ITEM_COUNT; ++i)
		{
			if ((!shopItems[i].isPurchased) &&
				shopItems[i].id != ShopItemID::Reroll && 
				(!shopItems[i].isButtonVisible || shopItems[i].isButtonVisible()) &&
				(!shopItems[i].isButtonEnabled || shopItems[i].isButtonEnabled()))//購入済みでないアイテムかつリロールボタン以外のアイテムで、出現条件を満たすアイテムのインデックスを取得
			{
				allItems.push_back(i);//購入済みでないアイテムのインデックスを追加
			}
		}
		
		std::vector<int> result;
		for (int pick = 0; pick < count && !allItems.empty(); ++pick)
		{
			std::vector<float> weights;
			for (int idx : allItems)
			{
				weights.push_back(shopItems[idx].appearanceRate);
			}

			std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
			size_t chosen = dist(rng);//選ばれたアイテムのインデックス

			result.push_back(allItems[chosen]);

			//ミートアシスト、風無効化、重力変化、半額、無料は1つしか出現しないようにする
			if(shopItems[allItems[chosen]].id == ShopItemID::ContactAsist ||
				shopItems[allItems[chosen]].id == ShopItemID::WindDisable ||
				shopItems[allItems[chosen]].id == ShopItemID::GravityChange ||
				shopItems[allItems[chosen]].id == ShopItemID::HalfPrice ||
				shopItems[allItems[chosen]].id == ShopItemID::FreePrice||
				shopItems[allItems[chosen]].id == ShopItemID::SpecialAbilityActiveRateUp||
				shopItems[allItems[chosen]].id == ShopItemID::PitchTypeDecrease) 
			{
				allItems.erase(allItems.begin() + chosen);//選ばれたアイテムを削除して、次の選択で同じアイテムが出現しないようにする
			}
		}

		return result;
	}

	std::vector<int> ShopLayout()
	{
		std::vector<int> layout = GetRondomShopItem(SHOP_ITEM_RANDOM);

		//リロールボタンは7枠目に固定配置
		int rerollIndex = -1;
		for(int i = 0; i < SHOP_ITEM_COUNT; ++i)
		{
			if(shopItems[i].id == ShopItemID::Reroll)
			{
				rerollIndex = i;//リロールボタンのインデックスを取得
				break;
			}
		}

		if (rerollIndex != -1)
		{
			layout.push_back(rerollIndex);//リロールボタンを7枠目に追加
		}

		return layout;
		
	}

	void RerollShopItems()
	{
		currentShopItemIndices = ShopLayout();

		
		std::fill(std::begin(slotPurchased), std::end(slotPurchased), false);
		std::fill(std::begin(slotHover), std::end(slotHover), false);
		
	}

	//ショップアイテムを半額にする関数
	void ApplyHalfPrice()
	{
		for (int i = 0; i < SHOP_ITEM_DISPLAY_COUNT; ++i)
		{
			int itemIndex = currentShopItemIndices[i];
			if (shopItems[itemIndex].id != ShopItemID::Reroll) // リロールボタンは除外
			{
				shopItems[itemIndex].price /= 2; // 価格を半額にする
				isHalfPriceApplied = true; // 半額適用フラグを設定
			}
		}
	}

	//ショップアイテムを無料にする関数
	void ApplyFreePrice()
	{
		for (int i = 0; i < SHOP_ITEM_DISPLAY_COUNT; ++i)
		{
			int itemIndex = currentShopItemIndices[i];
			if (shopItems[itemIndex].id != ShopItemID::Reroll) // リロールボタンは除外
			{
				shopItems[itemIndex].price = 0; // 価格を無料にする
				isFreePriceApplied = true; // 無料適用フラグを設定
			}
		}
	}

	//無料中もしくは半額中にどれか一つでも購入したら、すべての商品の無料状態を解除する関数
	void RemoveFreeOrHalfPrice()
	{
		for (int i = 0; i < SHOP_ITEM_DISPLAY_COUNT; ++i)
		{
			int itemIndex = currentShopItemIndices[i];
			if (shopItems[itemIndex].id != ShopItemID::Reroll) // リロールボタンは除外
			{
				if (shopItems[itemIndex].price == 0 || shopItems[itemIndex].price == shopItems[itemIndex].originalPrice / 2) // 無料状態または半額状態のアイテムがある場合
				{
					shopItems[itemIndex].price = shopItems[itemIndex].originalPrice; // 元の価格に戻す
					isHalfPriceApplied = false; // 半額適用フラグをリセット
					isFreePriceApplied = false; // 無料適用フラグをリセット
				}
			}
		}
	}

	//マウス座標が次へボタンの範囲内にあるかどうかを判定する関数
	bool IsMouseOverNextRoundButton(float mouseX, float mouseY)
	{
		float buttonLeft = nextRoundButtonPosition.x - nextRoundButtonSize.x / 2.0f;
		float buttonRight = nextRoundButtonPosition.x + nextRoundButtonSize.x / 2.0f;
		float buttonTop = (nextRoundButtonPosition.y + currentOffsetY) - nextRoundButtonSize.y / 2.0f;
		float buttonBottom = (nextRoundButtonPosition.y + currentOffsetY) + nextRoundButtonSize.y / 2.0f;
		return (mouseX >= buttonLeft && mouseX <= buttonRight &&
				mouseY >= buttonTop && mouseY <= buttonBottom);
	}

	//次へボタンが押されたかどうかを判定する関数
	bool IsNextRoundButtonPressed(float mouseX, float mouseY, bool isMouseClicked)
	{
		return IsMouseOverNextRoundButton(mouseX, mouseY) && isMouseClicked;
	}

private:

	std::mt19937 rng{ std::random_device{}() }; // 乱数生成器

private:

	static const char* GetBatterPowerRankString(Player::BatterPowerRank rank)
	{
		switch (rank)
		{
		case Player::BatterPowerRank::F: return "F";
		case Player::BatterPowerRank::E: return "E";
		case Player::BatterPowerRank::D: return "D";
		case Player::BatterPowerRank::C: return "C";
		case Player::BatterPowerRank::B: return "B";
		case Player::BatterPowerRank::A: return "A";
		case Player::BatterPowerRank::S: return "S";
		default: return "";
		}
	}

	static const char* GetBatterContactRankString(Player::BatterContactRank rank)
	{
		switch (rank)
		{
		case Player::BatterContactRank::F: return "F";
		case Player::BatterContactRank::E: return "E";
		case Player::BatterContactRank::D: return "D";
		case Player::BatterContactRank::C: return "C";
		case Player::BatterContactRank::B: return "B";
		case Player::BatterContactRank::A: return "A";
		case Player::BatterContactRank::S: return "S";
		default: return "";
		}
	}

public:

	enum class ShopState
	{
		Normal,
		SelectPitchType,
		SelectSpecialAbility,
	};

	ShopState currentShopState = ShopState::Normal;

private:

	//球種削除用

	std::vector<Pitcher::PitchType> pitchTypeChoices; // 利用可能な球種のリスト
	int hoveredPitchTypeIndex = -1; // ホバー中の球種のインデックス
	
	static constexpr int MAX_PITCH_TYPE_CHOICES = 19;// 最大球種数
	DirectX::XMFLOAT2 pitchTypeIconPositions[MAX_PITCH_TYPE_CHOICES];
	DirectX::XMFLOAT2 pitchTypeIconSize = { 150.0f, 37.5f };
	std::unique_ptr<sprite> pitchTypeSprites[MAX_PITCH_TYPE_CHOICES];

	FontRenderer pitchTypeFont;
	DirectX::XMFLOAT2 pitchTypeFontPosition = { 960.0f, 100.0f };
	float pitchTypeFontScale = 0.5f;

	void UpdateSelectPitchTypeState();
	void RenderSelectPitchTypeState();

	// 球種ごとのアイコン画像パスを返す
	static std::wstring GetPitchTypeIconPath(Pitcher::PitchType type);

private:
	//特殊能力削除用
	std::vector<SpecialAbility::AbilityID> specialAbilityChoices; // 利用可能な特殊能力のリスト
	int hoveredSpecialAbilityIndex = -1; // ホバー中の特殊能力のインデックス
	static constexpr int MAX_SPECIAL_ABILITY_CHOICES = 17; // 最大特殊能力数
	DirectX::XMFLOAT2 specialAbilityIconPositions[MAX_SPECIAL_ABILITY_CHOICES];
	DirectX::XMFLOAT2 specialAbilityIconSize = { 350.0f, 100.0f };
	std::unique_ptr<sprite> specialAbilitySprites[MAX_SPECIAL_ABILITY_CHOICES];
	FontRenderer specialAbilityFont;
	DirectX::XMFLOAT2 specialAbilityFontPosition = { 960.0f, 100.0f };
	float specialAbilityFontScale = 1.0f;
	float ActiveRateUpAmount = 5.0f;
	void UpdateSelectSpecialAbilityState();
	void RenderSelectSpecialAbilityState();
	// 特殊能力ごとのアイコン画像パスを返す
	static std::wstring GetSpecialAbilityIconPath(SpecialAbility::AbilityID id);

private:

	//ショップに必要なデータを保持する構造体
	struct ShopData
	{
		ShopItemID id;//ショップアイテムのID
		std::wstring texturePath;//ショップの背景画像のパス
		std::wstring descriptionPath;//ショップの説明画像のパス
		std::string name;//商品名
		int price;//商品の価格
		int originalPrice;//商品の元の価格
		int increaseBallCount;//ボールの増加量
		float appearanceRate;//その商品の出現確率
		bool isPurchased;//購入済みかどうか
		int level;//商品のレベル
		bool isUnlocked;//その商品のアンロック状態
		bool isHover;//その商品のホバー状態
		int powerUp = 0; // 威力アップの効果量
		int contactUp = 0; // ミートアップの効果量
		int pitcherPowerPenalty = 0.0f; // 威圧感能力の投手へのペナルティ
		int pitcherBreakBallPenalty = 0.0f; // 威圧感能力の変化球へのペナルティ
		float homerunMultiplierUp = 0.0f; // ホームラン倍率アップの効果量
		float breakingBallMultiplierUp = 0.0f; // 変化球倍率アップの効果量
		float gravityChange = 0.0f; // 重力変化の効果量
		float specialAbilityActiveRateUp = 0.0f; // 特殊能力発動率アップの効果量
		int netDecrease = 0; // ネット減少の効果量
		int targetHomerun = 0; // ホームランの目標数
		float ballZoneRateUp = 0.0f; // ボールゾーンの出現率アップの効果量
		int ballZoneBonus = 0; // ボールゾーンのボーナス倍率の効果量
		
		std::function<bool()> isButtonVisible; //ボタンの出現条件
		std::function<bool()> isButtonEnabled; //ボタンの有効条件

		//ボタンを押したときの処理
		std::function<void()> onButtonPressed;
	};

	ShopData shopItems[SHOP_ITEM_COUNT];

	std::unique_ptr<ShopSprite> shopItemSprites[SHOP_ITEM_COUNT];
	std::unique_ptr<sprite> shopItemSpriteObjects[SHOP_ITEM_COUNT];

	DirectX::XMFLOAT2 shopItemPositions[SHOP_ITEM_DISPLAY_COUNT] = {
		{ 250.0f, 400.0f },
		{ 500.0f, 400.0f },
		{ 750.0f, 400.0f },
		{ 1000.0f, 400.0f },
		{ 375.0f, 700.0f },
		{ 625.0f, 700.0f },
		{ 875.0f, 700.0f }
	};

	DirectX::XMFLOAT2 shopItemSize = { 200.0f, 225.0f };

	std::vector<int> currentShopItemIndices; // 現在表示されているショップアイテムのインデックス

private:
	bool AbilityIsOwned() const;//特殊能力が所有されているかどうかを判定する関数
	void ApplyPowerUp(int power);
	void ApplyContactUp(int contact);
	void IncreaseBallCount(int count);
	void IncreaseHomeRunMultiplier(float multiplier);
	void IncreaseBreakingBallMultiplier(float multiplier);
	void PitcherPowerRankDown(int penalty);
	void PitcherBreakBallRankDown(int penalty);
	void EnableMeetAssist();//ミートアシストを有効化する関数
	void DisableWindEffect();//風の影響を無効化する関数	
	void IncreaseBallZoneRate(float rate);
	void IncreaseBallZoneBonus(float bonus);
	void ChangeGravity(float gravity);
	void SelectPitchTypeState();//ピッチャーの持っている球種を選択するステート	
	void SelectSpecialAbilityState();//バッターが持っている特殊能力を選択するステート

	int shopPowerRankDown = 0; //ショップでの威圧感能力の投手へのペナルティ
	int shopBreakRankDown = 0; //ショップでの威圧感能力の変化球へのペナルティ

	bool slotPurchased[SHOP_ITEM_DISPLAY_COUNT] = {  }; // 各スロットの購入状態を保持する配列
	bool slotHover[SHOP_ITEM_DISPLAY_COUNT] = {  }; // 各スロットのホバー状態を保持する配列

	bool isHalfPriceApplied = false; // 半額状態が適用されているかどうかのフラグ
	bool isFreePriceApplied = false; // 無料状態が適用されているかどうかのフラグ

private:

	//オーディオ関連
	AudioSource* purchaseSound = nullptr;
	AudioSource* notEnoughMoneySound = nullptr;
	AudioSource* hoverSound = nullptr;
	AudioSource* selectPitchTypeSound = nullptr;
	AudioSource* selectSpecialAbilitySound = nullptr;

private:
	bool isNextRoundHovered = false; // 次へボタンホバー状態
	bool isNextRoundPressed = false; // 次へボタン押下状態
	float nextRoundButtonScale = 1.0f; // 描画用のスケール倍率
};