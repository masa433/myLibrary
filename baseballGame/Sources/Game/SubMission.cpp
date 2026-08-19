#include "SubMission.h"
#include "Graphics.h"
#include "shader.h"
#include <Player.h>
#include "Combo.h"
#include <RoundManager.h>
#include "HomeRunCount.h"

bool SubMission::ClearDistance(float distance) const
{

	return BallDistance::Instance().GetFinalDistance() >= distance;
}

bool SubMission::ClearDirection(Direction direction) const
{
	if (BallDistance::Instance().GetFinalDistance() <= 0.0f) return false;

	bool isRightHanded = Player::Instance().IsRightBatter();
	float ballAngle = Physics::Instance().GetBallOriginalDirection(); // レフト側: マイナス, ライト側: プラス と想定

	int count = 0;

	enum class FieldArea { Left, Center, Right, OutOfBounds };
	FieldArea hitArea = FieldArea::OutOfBounds;
	
	if (BallDistance::Instance().GetFinalDistance() > 0.0f)
	{
		// ボールの角度に基づいてヒットエリアを判定
		if (ballAngle >= -15.0f && ballAngle <= 15.0f)
		{
			hitArea = FieldArea::Center;
		}
		// 左方向の判定
		else if (ballAngle < -15.0f && ballAngle >= -45.0f)
		{
			hitArea = FieldArea::Left;

		}
		// 右方向の判定
		else if (ballAngle > 15.0f && ballAngle <= 45.0f)
		{
			hitArea = FieldArea::Right;
		}

		
		switch(direction)
		{
			case Direction::Pull:
				return (isRightHanded && hitArea == FieldArea::Left) || (!isRightHanded && hitArea == FieldArea::Right);
			case Direction::Center:
				return (hitArea == FieldArea::Center);
			case Direction::Opposite:
				return (isRightHanded && hitArea == FieldArea::Right) || (!isRightHanded && hitArea == FieldArea::Left);
		}

		
	}

	return false;

}

bool SubMission::ClearBreakingBall(bool isBreaking) const
{
	return Pitcher::Instance().IsBreakingBallBonus() == isBreaking;
}

bool SubMission::ClearCombo(int combo) const
{
	return Combo::Instance().GetCurrentCombo() >= combo;
}

bool SubMission::ClearHomeRunCount(int count) const
{
	return HomeRunCount::Instance().GetHomeRunCount() >= count;
}


void SubMission::Initialize(ID3D11Device* device)
{
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	std::vector<int> codepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789m/[]達成挑戦中変化球引っ張り流しセンター方向以上の打とうホームランを本球連続にしよう!");

	missionFont.Initialize(device,
		L".\\resources\\fonts\\GenEiGothicN-U-KL.otf",
		100.0f, screenWidth, screenHeight, 2048, 2048, &codepoints);

	BuildMissionList();
	currentRoundCache = -1; // ラウンドキャッシュを初期化
}

void SubMission::Uninitialize()
{
	missionFont.Uninitialize();
}

void SubMission::Update(float elapsedTime)
{
	// ラウンドが変わった場合にミッションを選択
	int currentRound = RoundManager::Instance().GetCurrentRound();
	if (currentRound != currentRoundCache)
	{
		int targetLevel = TargetLevel(currentRound);
		SelectMissionForRound(targetLevel);
		currentRoundCache = currentRound;
	}
	// 現在のミッションが存在する場合、進行状況を評価
	bool isHomeRun = Ball::Instance().GetHasPassedHomeRunZone() ||
		Ball::Instance().GetHasCollidedWithPole();
	bool isHitFinished = Ball::Instance().GetHasCollidedWithFence() ||
		Ball::Instance().GetHasCollidedWithGround();

	if (isHomeRun && isHitFinished)
	{
		if (!homeRunEventConsumed)
		{
			homeRunEventConsumed = true; // 同じ打球で多重カウントしない
			EvaluateCurrentMission();
		}
	}
	else
	{
		homeRunEventConsumed = false;
	}
}

void SubMission::Render()
{
	if (!currentMission) return;

	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	DirectX::XMFLOAT4 color = currentMission->cleared
		? DirectX::XMFLOAT4(1.0f, 0.84f, 0.0f, 1.0f)
		: DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	std::string line = (currentMission->cleared ? u8"[達成] " : u8"[挑戦中] ")
		+ currentMission->description
		+ " (" + std::to_string(currentMission->current) + "/" + std::to_string(currentMission->required) + ")";

	missionFont.DrawTextW(dc, line.c_str(), listPosition.x, listPosition.y, fontSize,
		color.x, color.y, color.z, color.w);
}

void SubMission::BuildMissionList()
{
	missionPool.clear();

	auto add = [](MissionPool& pool, const char* desc, int reward, int level, int required,
		std::vector<std::function<bool()>> conditions)
		{
			pool.push_back([=]() -> MissionData
				{
					MissionData m;
					m.description = desc;// ミッションの説明
					m.reward = reward;// ミッションの報酬
					m.level = level;// ミッションの難易度
					m.required = required;// ミッションの達成条件
					m.conditions = conditions;// ミッションの条件
					return m;
				});
		};

	// ラウンド1のミッションを追加
	MissionPool& round1Pool = missionPool[0];
	add(round1Pool, u8"ホームランを1本打とう！", 100, 1, 1, { [this]() { return ClearHomeRunCount(1); } });

	//ラウンド2と3のレベル1ミッションを追加
	MissionPool& level1Pool = missionPool[1];
	add(level1Pool, u8"変化球をホームランにしよう！", 100, 1, 1, { [this]() { return ClearBreakingBall(true); } });
	add(level1Pool, u8"引っ張り方向に130m以上のホームランを打とう！", 100, 1, 1, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(130.0f); } });
	add(level1Pool, u8"流し方向に130m以上のホームランを打とう！", 100, 1, 1, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(130.0f); } });
	add(level1Pool, u8"130m以上のホームランを2本打とう！", 100, 1, 2, { [this]() { return ClearDistance(130.0f); } });
	add(level1Pool, u8"140m以上のホームランを1本打とう！", 100, 1, 1, { [this]() { return ClearDistance(140.0f) && ClearHomeRunCount(1); } });
	add(level1Pool, u8"2球連続でホームランを打とう！", 200, 1, 2, { [this]() { return ClearCombo(1); } });

	//ラウンド4と5のレベル2ミッションを追加
	MissionPool& level2Pool = missionPool[2];
	add(level2Pool, u8"変化球を2球ホームランにしよう！", 200, 2, 2, { [this]() { return ClearBreakingBall(true); } });
	add(level2Pool, u8"引っ張り方向に130m以上のホームランを2本打とう！", 200, 2, 2, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(130.0f); } });
	add(level2Pool, u8"引っ張り方向に140m以上のホームランを1本打とう！", 200, 2, 1, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(140.0f) && ClearHomeRunCount(1); } });
	add(level2Pool, u8"流し方向に130m以上のホームランを2本打とう！", 200, 2, 2, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(130.0f); } });
	add(level2Pool, u8"流し方向に140m以上のホームランを1本打とう！", 200, 2, 1, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(140.0f) && ClearHomeRunCount(1); } });
	add(level2Pool, u8"センター方向にホームランを1本打とう!", 200, 2, 1, { [this]() { return ClearDirection(Direction::Center) && ClearHomeRunCount(1); } });
	add(level2Pool, u8"3球連続でホームランを打とう！", 300, 2, 3, { [this]() { return ClearCombo(1); } });
	add(level2Pool, u8"130m以上のホームランを3本打とう！", 300, 2, 3, { [this]() { return ClearDistance(130.0f); } });
	add(level2Pool, u8"140m以上のホームランを2本打とう！", 300, 2, 2, { [this]() { return ClearDistance(140.0f); } });
	//ラウンド6と7のレベル3ミッションを追加
	MissionPool& level3Pool = missionPool[3];
	add(level3Pool, u8"変化球を3球ホームランにしよう！", 300, 3, 3, { [this]() { return ClearBreakingBall(true); } });
	add(level3Pool, u8"引っ張り方向に130m以上のホームランを3本打とう！", 300, 3, 3, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(130.0f); } });
	add(level3Pool, u8"引っ張り方向に140m以上のホームランを2本打とう！", 300, 3, 2, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(140.0f); } });
	add(level3Pool, u8"引っ張り方向に140m以上のホームランを3本打とう！", 400, 3, 3, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(140.0f); } });
	add(level3Pool, u8"流し方向に130m以上のホームランを3本打とう！", 300, 3, 3, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(130.0f); } });
	add(level3Pool, u8"流し方向に140m以上のホームランを2本打とう！", 300, 3, 2, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(140.0f); } });
	add(level3Pool, u8"流し方向に140m以上のホームランを3本打とう！", 400, 3, 3, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(140.0f); } });
	add(level3Pool, u8"センター方向にホームランを3本打とう！", 400, 3, 3, { [this]() { return ClearDirection(Direction::Center) && ClearHomeRunCount(1); } });
	add(level3Pool, u8"センター方向にホームランを2本打とう！", 300, 3, 2, { [this]() { return ClearDirection(Direction::Center) && ClearHomeRunCount(1); } });
	add(level3Pool, u8"140m以上のホームランを3本打とう！", 400, 3, 3, { [this]() { return ClearDistance(140.0f); } });
	add(level3Pool, u8"4球連続でホームランを打とう！", 400, 3, 4, { [this]() { return ClearCombo(4); } });
	add(level3Pool, u8"150m以上のホームランを1本打とう！", 500, 3, 1, { [this]() { return ClearDistance(150.0f); } });
}

void SubMission::SelectMissionForRound(int round)
{
	auto it = missionPool.find(round);
	if (it != missionPool.end())
	{
		MissionPool& pool = it->second;
		if (!pool.empty())
		{
			std::uniform_int_distribution<int> dist(0, static_cast<int>(pool.size()) - 1);
			int index = dist(rng);
			currentMission = std::make_unique<MissionData>(pool[index]());
			currentMissionIndex = index;
		}
		else
		{
			currentMission.reset();
			currentMissionIndex = -1;
		}
	}
	else
	{
		currentMission.reset();
		currentMissionIndex = -1;
	}
}

void SubMission::EvaluateCurrentMission()
{
	if (currentMission)
	{
		CheckMission(*currentMission);
	}
}

void SubMission::CheckMission(MissionData& mission)
{
	if (mission.cleared) return;
	int previousCurrent = mission.current;
	for (const auto& condition : mission.conditions)
	{
		if (condition())
		{
			mission.current++;
		}
	}
	if (mission.current >= mission.required)
	{
		mission.cleared = true;
		totalReward += mission.reward;
	}
}