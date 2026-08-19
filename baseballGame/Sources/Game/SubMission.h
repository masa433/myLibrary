#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include "FontRenderer.h"
#include "Ball.h"
#include "Pitcher.h"
#include "ballDistance.h"
#include <random>
#include "json.hpp"

using json = nlohmann::json;




class SubMission
{
public:
	static SubMission& Instance()
	{
		static SubMission instance;
		return instance;
	}
	void Initialize(ID3D11Device* device);
	void Uninitialize();
	void Update(float elapsedTime);
	void Render();
	void DrawGUI();
	void SaveToJson(json& j);
	void LoadFromJson(const json& j);
	void ResetSubMission();


	enum class Direction { Pull,Center, Opposite };
	Direction direction = Direction::Center;



	int GetTotalReward() const { return totalReward; }

private:
	
	int totalReward = 0;//累計報酬

	//ミッションデータ
	struct MissionData
	{
		std::string description;//ミッションの説明
		int reward = 0;//ミッションの報酬
		int level = 1;//ミッションの難易度
		int required = 1;//ミッションの達成条件
		int current = 0;//現在の進行状況
		bool cleared = false;//ミッションがクリアされたかどうか
		std::vector<std::function<bool()>> conditions; // 全部trueでクリア(AND)
	};

	//ミッションプールの定義
	using MissionFactory = std::function<MissionData()>;// ミッションファクトリ(ミッションデータを生成する関数)
	using MissionPool = std::vector<MissionFactory>;// ミッションプール(ラウンドごとのミッションファクトリの集合)

	void BuildMissionList();// ミッションリストを構築する関数
	void SelectMissionForRound(int round);    // プールから1件選んで出題する
	void EvaluateCurrentMission();// 現在のミッションの達成状況を評価する
	void CheckMission(MissionData& mission);// ミッションの達成状況をチェックする関数

	//クリア条件を設定する関数
	bool ClearDistance(float distance) const;
	bool ClearDirection(Direction direction) const;
	bool ClearBreakingBall(bool isBreaking) const;
	bool ClearCombo(int combo) const;
	bool ClearHomeRunCount(int count) const;

	//ラウンドごとに出すミッションを変える
	int TargetLevel(int round) const
	{
		if (round == 1) return 0;// ラウンド1はミッションプール0を使用
		else if(round == 2 || round == 3) return 1;
		else if(round == 4 || round == 5) return 2;
		else if(round == 6 || round == 7) return 3;
		else return -1; // ラウンドが範囲外の場合
	}

	std::mt19937 rng{ std::random_device{}() };

	FontRenderer missionFont;

	std::map<int, MissionPool> missionPool; // ラウンドごとのミッションプール
	std::unique_ptr<MissionData> currentMission; // 現在のミッションを保持するポインタ
	int currentMissionIndex = -1;// 現在のミッションのインデックス
	int currentRoundCache = -1;// 現在のラウンドをキャッシュする変数
	bool homeRunEventConsumed = false;//ホームランイベントが消費されたかどうかを示すフラグ

	DirectX::XMFLOAT2 listPosition = { 40.0f, 100.0f };
	float fontSize = 0.3f;

};

