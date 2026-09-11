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
#include <deque>
#include "sprite.h"
#include "Ball.h"
#include "json.hpp"
#include <unordered_map>
#include "Effect.h"
#include "../Sources/Audio/Audio.h"
#include "../Sources/Audio/AudioSource.h"

using json = nlohmann::json;

#define PITCHER_COUNT 21

class Pitcher : public GameObject
{
public:

	//インスタンス
	static Pitcher& Instance()
	{
		static Pitcher instance;
		return instance;
	}
	void Initialize();
	void Uninitialize();
	void Update(float elapsedTime);
	void Render(const RenderContext& rc, ModelRenderer* renderer, bool isShadowPass = false);
	void DrawGUI();

	void AttachBallToHand(float elapsedTime);

	void UpdateAnimation(float elapsedTime);

	void UpdateBallCollider();

	void ApplyPhysicsToBall(float elapsedTime);

	void SelectPitchType();

	void SaveToJson(json& j);
	void LoadFromJson(const json& j);

	void ThrowBallBezier();

	float GetBallSpeedKmh() const { return ballSpeedKmh; }

	void SetBallSpeedKmh(float speed) { ballSpeedKmh = speed; }

	void ResetPitchFlags(); // pitchFlagsをリセットする関数

	bool IsBezierTargetCenter(float threshold = 0.1f) const
	{
		const auto& params = pitchParameters[static_cast<int>(selectedPitchType)];

		// bezierTarget の (x, y) が 0.0f 付近（許容誤差 threshold 内）かを判定
		bool isCenterX = std::fabs(params.bezierTarget.x) <= threshold;
		bool isCenterY = std::fabs(params.bezierTarget.y) <= threshold;

		return isCenterX && isCenterY;
	}

	//変化球ボーナス
	bool IsBreakingBallBonus() const
	{
		switch (selectedPitchType)
		{
		case PitchType::Slider:
		case PitchType::Curveball:
		case PitchType::Changeup:
		case PitchType::Forkball:
		case PitchType::TwoSeam:
		case PitchType::Cutter:
		case PitchType::Sinker:
		case PitchType::VerticalSlider:
		case PitchType::Splitter:
		case PitchType::SlowCurve:
		case PitchType::Shooter:
		case PitchType::Knuckleball:
		case PitchType::SlowBall:
		case PitchType::Sweeper:
		case PitchType::Palm:
			return true;
		default:
			return false;
		}
	}

	bool IsFastball() const
	{
		return selectedPitchType == PitchType::Fastball || selectedPitchType == PitchType::CutFastball || selectedPitchType == PitchType::BlazingFastball || selectedPitchType == PitchType::NaturalShoot;
	}

public:

	enum class PitchType
	{
		Fastball,//ストレート
		Slider,//スライダー
		Curveball,//カーブ
		Changeup,//チェンジアップ
		Forkball,//フォーク
		TwoSeam,//ツーシーム
		Cutter,//カットボール
		Sinker,//シンカー
		VerticalSlider,//縦スライダー	
		Splitter,//スプリット
		SlowCurve,//スローカーブ
		Shooter,//シュート
		Knuckleball,//ナックル
		SlowBall,//スローボール
		Sweeper,//スイーパー
		Palm,//パーム
		NaturalShoot,//ナチュラルシュート
		CutFastball,//真っスラ
		BlazingFastball,//火の玉ストレート
	};

	PitchType GetSelectedPitchType() const { return selectedPitchType; }
	bool GetIsBallThrown() const { return isBallThrown; }

	float GetSpeedVarianceKmh(PitchType pitchType) const;
	const char* GetPitchTypeName(PitchType pitchType) const;

private:
	// モデル関連
	std::unique_ptr<gltf_model> rightPitcher;
	std::unique_ptr<gltf_model> leftPitcher;
	gltf_model* currentPitcher = nullptr;
	std::vector<gltf_model::node> animated_nodes;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
	// アニメーション関連
	float animation_time = 0.0f;
	int current_animation_index = 0;
	bool animation_playing = true;

	DirectX::XMFLOAT3 ballStartPosition = { 0.0f, 0.0f, 0.0f };
	float throwTiming = 0.4f;
	bool isBallThrown = false;

	// ボール投球制御
	float ballSpeedKmh = 150.0f; // 投球速度（km/h） - デバッグ可能
	float launchAngleDegrees = -2.5f; // 発射角度（度）
	DirectX::XMFLOAT3 rotationSpeed = { 0.0f, 0.0f, 0.0f }; // 回転速度（度/秒）
	DirectX::XMFLOAT3 throwDirection = { 0.02f, 0.2f, -1.0f }; // 投球方向

	float ballDebugRadius = 0.15f; // デフォルトのスケール倍率
	float reducedRadius = 0.0f;

	float FairFaulJudgeDelayTime = 0.0f; // ファウル判定の遅延時間（秒）

public:


	// ballSprite::pitchBreaks[14] のインデックス（GUIコンボボックス順）と
	// PitchType enum の値を対応させる変換関数。
	// pitchBreaks の並び順: Fastball,TwoSeam,Cutter,Slider,Curveball,Changeup,
	//                        Forkball,Sinker,VerticalSlider,Splitter,SlowCurve,
	//                        Shooter,Knuckleball,SlowBall
	static int PitchTypeToBreakIndex(PitchType type)
	{
		switch (type)
		{
		case PitchType::Fastball:       return 0;
		case PitchType::TwoSeam:        return 1;
		case PitchType::Cutter:         return 2;
		case PitchType::Slider:         return 3;
		case PitchType::Curveball:      return 4;
		case PitchType::Changeup:       return 5;
		case PitchType::Forkball:       return 6;
		case PitchType::Sinker:         return 7;
		case PitchType::VerticalSlider: return 8;
		case PitchType::Splitter:       return 9;
		case PitchType::SlowCurve:      return 10;
		case PitchType::Shooter:        return 11;
		case PitchType::Knuckleball:    return 12;
		case PitchType::SlowBall:       return 13;
		case PitchType::Sweeper:        return 14;
		case PitchType::Palm:           return 15;
		case PitchType::NaturalShoot:   return 16;
		case PitchType::CutFastball:    return 17;
		case PitchType::BlazingFastball:return 18;
		default:                        return 0;
		}
	}

	public:
	PitchType selectedPitchType = PitchType::Fastball;
private:

	struct PitchParameter
	{
		float ballSpeedKmh; // 投球速度（km/h）
		float launchAngleDegrees; // 発射角度（度）
		DirectX::XMFLOAT3 throwDirection; // 投球方向
		DirectX::XMFLOAT3 spinAxis; // 回転軸
		float rpm; // 回転数（回転/分）
		DirectX::XMFLOAT3 visualRotationSpeed;// 見た目の回転速度（度/秒）
		DirectX::XMFLOAT3 visualAngle; // 見た目の角度（度）

		//ベジェ曲線制御用オフセット
		DirectX::XMFLOAT3 bezierCtrl1 = { 0.0f, 0.0f, 0.0f }; //第1制御点
		DirectX::XMFLOAT3 bezierCtrl2 = { 0.0f, 0.0f, 0.0f }; //第2制御点
		DirectX::XMFLOAT3 bezierTarget = { 0.0f, 0.0f, 0.0f }; //ターゲット点

	};
	std::vector<PitchParameter> pitchParameters;

	static constexpr int PITCH_TYPE_COUNT = 19; // 球種の数

	int editerPitchIndex = 0; // エディタで選択された球種のインデックス

	void InitializePitchSettings(); // 球種のパラメーターを初期化する関数
	void SelectPitchTypeByAI();
	void ApplyAIBezierTarget();
	PitchType ChooseAIPitchType() const;

	bool usePitchAI = true;
	float aiStrikeRate = 1.0f;
	float aiNearBallMargin = 0.06f;

	//ファウルになった後に、球種選択に戻るまでの時間
	float foulWaitTime = 1.0f;
	float currentFoulWaitTime = 0.0f;

	bool foulSpriteTriggered = false; // ファウル判定がトリガーされたかどうか

public:
	// 5x5グリッド内でAIが狙う内側3x3のセルインデックス (0〜8、row-major)
	
	int aiTargetZoneIndex = 4;//ストライクゾーン内のインデックス
	int aiTargetZoneRow = 2; //5x5グリッドの行番号(0〜4)
	int aiTargetZoneCol = 2; //5x5グリッドの列番号(0〜4)

	DirectX::XMFLOAT2 aiTarget2D = { 0.0f, 0.0f }; // AIが狙うターゲット位置（2D平面上のX,Z座標）

	const DirectX::XMFLOAT2& GetAITarget2D() const { return aiTarget2D; }// AIが狙うターゲット位置（2D平面上のX,Z座標）を取得
	int GetAITargetRow() const { return aiTargetZoneRow; } // AIが狙うターゲット位置の行番号を取得
	int GetAITargetCol() const { return aiTargetZoneCol; } // AIが狙うターゲット位置の列番号を取得

private:
	//bool hasBeenJudged = false; // 判定済みフラグ


	float throwCounter = 0.0f; // 投球カウンター
	bool hasReachedZero = false; // z = 0.0f に到達したかどうか
	float resultWaitTimer = 0.0f; // 判定待ちタイマー

	bool isRightPitcher = true; // 右投げかどうか

public:
	bool IsRightPitcher() const { return isRightPitcher; }

private:

	//ストライクゾーンのトリガーボックス
	physx::PxRigidStatic* strikeZoneTrigger = nullptr;
	DirectX::XMFLOAT3 boxSize = { 0.43f, 0.6f, 0.001f }; // トリガーボックスのサイズ
	DirectX::XMFLOAT3 boxPosition = { 0.0f, 0.8f, 0.0f }; // トリガーボックスの位置

public:

	// 状態管理
	enum class State
	{
		SelectingPitch,// 球種選択中
		Throwing,// 投球中
		WaitingForResult,// 判定待ち

	};

	State currentState = State::SelectingPitch;
	float stateTime = 0.0f; // 現在の状態に入ってからの経過時間
	const State GetCurrentState() const { return currentState; }



private:
	// ===== 新規追加 =====
	physx::PxVec3 GetSpinAxisFromPitchType() const;


public:
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;

	DirectX::XMFLOAT2 targetPosition3D = { 0.0f, 0.0f }; // AIが狙うターゲット位置（3D空間上のX,Z座標）
	bool hasTargetSet = false; // AIがターゲット位置の接線を設定したかどうか

public:
	//球速のモード
	enum class BallSpeedMode
	{
		slowSpeed,//遅い
		highSpeed,//早い
		realSpeed,//リアルスピード
	};
	BallSpeedMode ballSpeedMode = BallSpeedMode::slowSpeed;

	BallSpeedMode GetBallSpeedMode() const { return ballSpeedMode; }
	void SetBallSpeedMode(BallSpeedMode mode) { ballSpeedMode = mode;}


private:
	void UpdatePitcherModel();

public:
	enum class RealPitcher
	{
		None,
		Nakagawa, //中川
		Mukai, //向井
		Abe, //阿部
		Morita, //森田
		Ito, //伊藤
		Fukuhara, //福原
		Ishikawa,// 石川
		Takaoka,//高岡
		Oda,//織田
		Kondo,//近藤
		Nishi,//西
		Kikuchi,//菊池
		Okubo,//大久保
		Mizuno,//水野
		Fujikawa,//藤川
		Watanabe,//渡邊
		Ishi,//石井
		Kinoshita,//木下
		Masuda,//増田
		Matsuyama,//松山
		Inoue,//井上
		Count,//カウント
	};

	enum class BreakGrade
	{
		F,
		E,
		D,
		C,//デフォルトのpitchBreaksと同じ大きさ
		B,
		A,
		S
	};

	//球速の強さを表す列挙型
	enum class Power
	{
		F,
		E,
		D,
		C,//デフォルトのpitchBreaksと同じ大きさ
		B,
		A,
		S
	};



	//実在投手が投げる球種のデータ
	struct RealArsenalEntry
	{
		PitchType pitchType;
		float weightPercent; // その球種を投げる確率（0.0～1.0）
		float speedKmh; // 球速（km/h）
		BreakGrade breakGrade = BreakGrade::C;
		Power power = Power::C;
	};

	

	// 実在投手プリセットを選択する。球種別球速をpitchParametersへ反映し、
	// 配球AI（ChooseAIPitchType）が実測の投球割合に基づいて球種を選ぶようになる。
	void SelectRealPitcher(RealPitcher rp);
	RealPitcher GetSelectedRealPitcher() const { return selectedRealPitcher; }
	static const char* GetRealPitcherName(RealPitcher rp);

	//配球の偏りを抑える
	std::deque<PitchType> pitchHistory;
	static constexpr int PITCH_HISTORY_SIZE = 6; // 過去6球分の履歴を保持
	float pitchRepeatPenalty = 0.5f; // 過去に投げた球種を再度選ぶ確率を減らすペナルティ（0.0～1.0）
	float pitchSequenceDecay = 0.55f; // 過去の投球履歴の影響を減らす減衰率（0.0～1.0）
	float pitchSequenceFloor = 0.1f; // 重みが下がりすぎないようにする下限倍率

	float GetSequencingMultiplier(PitchType type) const;

	BreakGrade GetPitchGrade(RealPitcher pitcher, PitchType pitchType) const
	{
		if (pitcher == RealPitcher::None) return BreakGrade::F;

		int rpIndex = static_cast<int>(pitcher);
		int pitchIndex = PitchTypeToBreakIndex(pitchType);

		if (pitchIndex < 0 || rpIndex >= static_cast<int>(realPitcherArsenal.size())) return BreakGrade::F;

		return realPitcherArsenal[pitchIndex].breakGrade;
	}

private:
	// 現在選択中の実在投手プリセット、およびその持ち球リスト（配球AIが参照する）
	RealPitcher selectedRealPitcher = RealPitcher::None;
	std::vector<RealArsenalEntry> realPitcherArsenal;

public:
	// 実在投手プリセットのデータテーブルを取得する（球種構成・球速・投げ手・表示名）
	// 該当データが無い場合はfalseを返す
	//outArsenal: 該当投手の持ち球リストを返す（空の場合あり）
	//outIsRight: 該当投手が右投げかどうかを返す
	//outName: 該当投手の表示名を返す
	static bool GetRealPitcherArsenalData(RealPitcher rp, std::vector<RealArsenalEntry>& outArsenal, bool& outIsRight, const char*& outName,int& pitcherRank);

	inline static const std::unordered_map<RealPitcher, int> pitcherToSpriteIndexTable =
	{
		{ RealPitcher::Nakagawa,  0 },
		{ RealPitcher::Mukai,     1 },
		{ RealPitcher::Abe,       2 },
		{ RealPitcher::Morita,    3 },
		{ RealPitcher::Ito,       4 },
		{ RealPitcher::Fukuhara,  5 },
		{ RealPitcher::Ishikawa,  6 },
		{ RealPitcher::Takaoka,   7 },
		{ RealPitcher::Oda,       8 },
		{ RealPitcher::Kondo,     9 },
		{ RealPitcher::Nishi,     10 },
		{ RealPitcher::Kikuchi,   11 },
		{ RealPitcher::Okubo,     12 },
		{ RealPitcher::Mizuno,    13 },
		{ RealPitcher::Fujikawa,  14 },
		{ RealPitcher::Watanabe,  15 },
		{ RealPitcher::Ishi,      16 },
		{ RealPitcher::Kinoshita, 17 },
		{ RealPitcher::Masuda,    18 },
		{ RealPitcher::Matsuyama, 19 },
		{ RealPitcher::Inoue,     20 },
	};

	int realPitcherRank = 1;
	
private:

	struct InfoData
	{
		std::wstring texturePath;
		DirectX::XMFLOAT2 position;
		DirectX::XMFLOAT2 size;
		float rotation;
		DirectX::XMFLOAT4 color;
	};

	DirectX::XMFLOAT2 cursorPosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 cursorSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT4 cursorColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	std::unique_ptr<InfoData> cursorData;
	std::unique_ptr<sprite> cursorSprite;

	DirectX::XMFLOAT2 swingPosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 swingSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT4 swingColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	std::unique_ptr<InfoData> swingData;
	std::unique_ptr<sprite> swingSprite;

	DirectX::XMFLOAT2 infoBackPosition = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 infoBackSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT4 infoBackColor = { 0.0f, 0.0f, 0.0f, 0.7f };

	std::unique_ptr<InfoData> infoBackData;
	std::unique_ptr<sprite> infoBackSprite;

	std::unique_ptr<InfoData> ballTypeData[PITCHER_COUNT];
	std::unique_ptr<sprite> ballTypeSprite[PITCHER_COUNT];
	DirectX::XMFLOAT2 ballTypePosition;
	DirectX::XMFLOAT2 ballTypeSize;
	DirectX::XMFLOAT4 ballTypeColor;

	//シェーダー関連
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;

	std::unique_ptr<Effect> rosinEffect;//投げる際の滑り止め(ロジン)エフェクト
public:
	
	void SetRealPitcherPower(Power power)
	{
		if (realPitcherArsenal.empty()) return;
		realPitcherArsenal[0].power = power;// 0番目の球種のパワーを設定
	}

	int GetRealPitcherPower() const
	{
		if (realPitcherArsenal.empty()) return 0;
		return static_cast<int>(realPitcherArsenal[0].power);// 0番目の球種のパワーを取得
	}

	// ランクダウンを適用して球速を取得する関数
	static Power GetPowerRankDown(Power power, int rankDown)
	{
		if (power == Power::F || rankDown <= 0) return power;
		int index = static_cast<int>(power);
		int newIndex = (std::max)(static_cast<int>(Power::F), index - rankDown);
		return static_cast<Power>(newIndex);
	}

	// ランクダウンを適用して変化球のグレードを取得する関数
	static BreakGrade GetBreakGradeRankDown(BreakGrade grade, int rankDown)
	{
		if (grade == BreakGrade::F || rankDown <= 0) return grade;
		int index = static_cast<int>(grade);
		int newIndex = (std::max)(static_cast<int>(BreakGrade::F), index - rankDown);
		return static_cast<BreakGrade>(newIndex);
	}

	//現在の球種の球威を取得する関数
	Power GetCurrentPitchPower() const
	{
		if (realPitcherArsenal.empty()) return Power::F;
		for (const auto& entry : realPitcherArsenal)
		{
			if (entry.pitchType == selectedPitchType)
			{
				return entry.power;
			}
		}
		return Power::F; // 見つからなかった場合はFを返す
	}

	//現在の球種の変化球グレードを取得する関数
	BreakGrade GetCurrentPitchBreakGrade() const
	{
		if (realPitcherArsenal.empty()) return BreakGrade::F;
		for (const auto& entry : realPitcherArsenal)
		{
			if (entry.pitchType == selectedPitchType)
			{
				return entry.breakGrade;
			}
		}
		return BreakGrade::F; // 見つからなかった場合はFを返す
	}

	//球種を無効化する関数
	void DisablePitchType(PitchType pitchType)
	{
		// disabledPitchTypes に pitchType を追加する
		if(!IsPitchTypeDisabled(pitchType))
		{
			disabledPitchTypes.push_back(pitchType);
		}
	}

	
	bool IsPitchTypeDisabled(PitchType pitchType) const
	{
		// disabledPitchTypes に pitchType が含まれているかを確認
		return std::find(disabledPitchTypes.begin(), disabledPitchTypes.end(), pitchType) != disabledPitchTypes.end();
	}

	//現在のピッチャーがまだ投げられる球種を取得する関数
	std::vector<PitchType> GetAvailablePitchTypes(RealPitcher pitcher) const
	{
		std::vector<PitchType> availablePitchTypes;// 現在のピッチャーが投げられる球種を格納するベクター
		for (const auto& entry : realPitcherArsenal)
		{
			if (!IsPitchTypeDisabled(entry.pitchType))// 無効化されていない球種のみを追加
			{
				availablePitchTypes.push_back(entry.pitchType);// 無効化されていない球種のみを追加
			}
		}
		return availablePitchTypes;
	}

	//球種をリセットする
	void ResetDisabledPitchTypes()
	{
		disabledPitchTypes.clear();
	}

	static constexpr float MIN_STRIKE_RATE = 0.7f; // 最小ストライク率

	//ストライク率を下げる関数
	void DecreaseStrikeRate(float amount)
	{
		aiStrikeRate -= amount;
		if (aiStrikeRate < MIN_STRIKE_RATE)
		{
			aiStrikeRate = MIN_STRIKE_RATE;
		}
	}

	//現在のストライク率を取得する関数
	float GetCurrentStrikeRate() const
	{
		return aiStrikeRate;
	}

	//ストライク率が最小値に達しているかを確認する関数
	bool IsStrikeRateMin() const
	{
		return aiStrikeRate <= MIN_STRIKE_RATE;
	}

private:
	std::vector<PitchType> disabledPitchTypes; // ショップで減らされた球種のリスト

	//風の影響を受けるかどうか
	bool windEffectEnabled = true;

public:

	bool IsWindEffectEnabled() const { return windEffectEnabled; }
	void SetWindEffectEnabled(bool enabled) { windEffectEnabled = enabled; }

	AudioSource* foulSound = nullptr; // ファウル音のAudioSourceへのポインタ

	bool isPlayedFoulSound = false; // ファウル音が再生されたかどうかのフラグ
};