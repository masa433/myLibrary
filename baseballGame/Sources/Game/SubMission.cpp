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

	enum class FieldArea { Left, Center, Right, OutOfBounds };// フィールドのエリアを定義(
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
		u8"0123456789m/[]"
		u8"達成挑戦中変化球引っ張り流しセンター方向"
		u8"以上ので打とうホームランを本球連続にしよう！");

	missionFont.Initialize(device,
		L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf",
		200.0f, screenWidth, screenHeight, 4096, 4096, &codepoints);

	progressFont.Initialize(device,
		L".\\resources\\fonts\\Futur12.ttf",
		150.0f, screenWidth, screenHeight, 4096, 4096, &codepoints);

	BuildMissionList();
	currentRoundCache = -1; // ラウンドキャッシュを初期化
}

void SubMission::Uninitialize()
{
	missionFont.Uninitialize();
	progressFont.Uninitialize();
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
	
	if (currentMission && !currentMission->cleared)
	{
		if (currentMission->description.find(u8"連続") != std::string::npos)
		{
			currentMission->current = Combo::Instance().GetCurrentCombo();
		}
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

	std::string line = currentMission->description;
	//進捗状況の文字列
	std::string progress = std::to_string(currentMission->current) + " / " + std::to_string(currentMission->required);

	//文字数を計算する
	size_t charCount = 0;
	for (size_t i = 0; i < line.size();)
	{
		unsigned char c = static_cast<unsigned char>(line[i]);// UTF-8の先頭バイトを取得
		if (c < 0x80)
		{
			i += 1; // ASCII文字
		}
		else if ((c & 0xE0) == 0xC0)
		{
			i += 2; // 2バイト文字
		}
		else if ((c & 0xF0) == 0xE0)
		{
			i += 3; // 3バイト文字
		}
		else if ((c & 0xF8) == 0xF0)
		{
			i += 4; // 4バイト文字
		}
		else
		{
			i += 1; // 不正なUTF-8シーケンスとして扱う
		}
		charCount++;
	}

	//基準の文字数を超えたらスケールを徐々に小さくする
	dynamicFontSize = fontSize;
	const size_t baseCharCount = 16; // 基準の文字数
	if (charCount > baseCharCount)
	{
		float shrinkRatio = 1.0f - static_cast<float>(charCount - baseCharCount) * 0.1f;// 文字数が1増えるごとに0.001ずつ縮小
		if (shrinkRatio < 0.6f) shrinkRatio = 0.6f; // 最小スケールを0.6に制限
		dynamicFontSize *= shrinkRatio;
	}

	//Textを中央ぞろえで描画する
	float drawWidth = 0.0f, drawHeight = 0.0f;

	missionFont.MeasureText(line.c_str(), dynamicFontSize, drawWidth, drawHeight);// 描画するテキストの幅と高さを取得

	float drawX = listPosition.x - drawWidth / 2.0f; // 中央ぞろえのためにX座標を調整

	missionFont.DrawTextW(dc, line.c_str(), drawX, listPosition.y, dynamicFontSize,
		color.x, color.y, color.z, color.w);

	float progressWidth = 0.0f, progressHeight = 0.0f;
	progressFont.MeasureText(progress.c_str(), progressFontSize, progressWidth, progressHeight);
	float progressX = progressPosition.x - progressWidth / 2.0f; // 中央ぞろえのためにX座標を調整
	progressFont.DrawTextW(dc, progress.c_str(), progressX, progressPosition.y, progressFontSize,
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
	add(level1Pool, u8"変化球を1球ホームランにしよう！", 200, 1, 1, { [this]() { return ClearBreakingBall(true); } });
	add(level1Pool, u8"引っ張り方向に130m以上のホームランを1本打とう！", 300, 1, 1, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(130.0f); } });
	add(level1Pool, u8"流し方向に130m以上のホームランを1本打とう！", 300, 1, 1, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(130.0f); } });
	add(level1Pool, u8"130m以上のホームランを2本打とう！", 300, 1, 2, { [this]() { return ClearDistance(130.0f); } });
	add(level1Pool, u8"140m以上のホームランを1本打とう！", 300, 1, 1, { [this]() { return ClearDistance(140.0f); } });
	add(level1Pool, u8"2球連続でホームランを打とう！", 400, 1, 2, { [this]() { return ClearCombo(1); } });

	//ラウンド4と5のレベル2ミッションを追加
	MissionPool& level2Pool = missionPool[2];
	add(level2Pool, u8"変化球を2球ホームランにしよう！", 200, 2, 2, { [this]() { return ClearBreakingBall(true); } });
	add(level2Pool, u8"引っ張り方向に130m以上のホームランを2本打とう！", 300, 2, 2, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(130.0f); } });
	add(level2Pool, u8"引っ張り方向に140m以上のホームランを1本打とう！", 400, 2, 1, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(140.0f); } });
	add(level2Pool, u8"流し方向に130m以上のホームランを2本打とう！", 300, 2, 2, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(130.0f); } });
	add(level2Pool, u8"流し方向に140m以上のホームランを1本打とう！", 400, 2, 1, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(140.0f); } });
	add(level2Pool, u8"センター方向にホームランを1本打とう!", 400, 2, 1, { [this]() { return ClearDirection(Direction::Center); } });
	add(level2Pool, u8"3球連続でホームランを打とう！", 700, 2, 3, { [this]() { return ClearCombo(1); } });
	add(level2Pool, u8"130m以上のホームランを3本打とう！", 500, 2, 3, { [this]() { return ClearDistance(130.0f); } });
	add(level2Pool, u8"140m以上のホームランを2本打とう！", 600, 2, 2, { [this]() { return ClearDistance(140.0f); } });
	//ラウンド6と7のレベル3ミッションを追加
	MissionPool& level3Pool = missionPool[3];
	add(level3Pool, u8"変化球を3球ホームランにしよう！", 500, 3, 3, { [this]() { return ClearBreakingBall(true); } });
	add(level3Pool, u8"引っ張り方向に130m以上のホームランを3本打とう！", 500, 3, 3, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(130.0f); } });
	add(level3Pool, u8"引っ張り方向に140m以上のホームランを2本打とう！", 600, 3, 2, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(140.0f); } });
	add(level3Pool, u8"引っ張り方向に140m以上のホームランを3本打とう！", 700, 3, 3, { [this]() { return ClearDirection(Direction::Pull) && ClearDistance(140.0f); } });
	add(level3Pool, u8"流し方向に130m以上のホームランを3本打とう！", 500, 3, 3, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(130.0f); } });
	add(level3Pool, u8"流し方向に140m以上のホームランを2本打とう！", 600, 3, 2, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(140.0f); } });
	add(level3Pool, u8"流し方向に140m以上のホームランを3本打とう！", 700, 3, 3, { [this]() { return ClearDirection(Direction::Opposite) && ClearDistance(140.0f); } });
	add(level3Pool, u8"センター方向にホームランを3本打とう！", 800, 3, 3, { [this]() { return ClearDirection(Direction::Center); } });
	add(level3Pool, u8"センター方向にホームランを2本打とう！", 600, 3, 2, { [this]() { return ClearDirection(Direction::Center); } });
	add(level3Pool, u8"140m以上のホームランを3本打とう！", 700, 3, 3, { [this]() { return ClearDistance(140.0f); } });
	add(level3Pool, u8"4球連続でホームランを打とう！", 1000, 3, 4, { [this]() { return ClearCombo(4); } });
	add(level3Pool, u8"150m以上のホームランを1本打とう！", 1000, 3, 1, { [this]() { return ClearDistance(150.0f); } });
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
	//コンボ系のサブミッションは現在のコンボ数をそのままcurrentに加算する
	if(mission.description.find(u8"連続") != std::string::npos)
	{
		mission.current = Combo::Instance().GetCurrentCombo();

		if(mission.current >= mission.required)
		{
			mission.cleared = true;
		}
		return;
	}

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
	}
}

void SubMission::ResetSubMission()
{
	currentMission.reset();
	currentMissionIndex = -1;
	totalReward = 0;
	currentRoundCache = -1; // ラウンドキャッシュをリセット
}

void SubMission::DrawGUI()
{
	if (ImGui::CollapsingHeader(u8"フォント設定"))
	{
		ImGui::DragFloat2(u8"位置", &listPosition.x, 1.0f, 0.0f, Graphics::Instance().GetScreenWidth());
		ImGui::SliderFloat(u8"フォントサイズ", &fontSize, 0.1f, 1.0f);
		ImGui::Separator();

		ImGui::DragFloat2(u8"進捗位置", &progressPosition.x, 1.0f, 0.0f, Graphics::Instance().GetScreenWidth());
		ImGui::SliderFloat(u8"進捗フォントサイズ", &progressFontSize, 0.1f, 1.0f);
	}

	// 現在アクティブなミッションの編集・クリアテスト
	if (currentMission && ImGui::CollapsingHeader(u8"現在進行中のミッション"))
	{
		ImGui::Text(u8"説明: %s", currentMission->description.c_str());
		ImGui::Checkbox(u8"クリア済みフラグ", &currentMission->cleared);
		ImGui::DragInt(u8"現在のカウント (current)", &currentMission->current, 1, 0, currentMission->required);
		ImGui::DragInt(u8"必要カウント (required)", &currentMission->required, 1, 1, 100);
		ImGui::DragInt(u8"報酬額 (reward)", &currentMission->reward, 10, 0, 10000);

		if (ImGui::Button(u8"即時クリア適用"))
		{
			currentMission->current = currentMission->required;
			currentMission->cleared = true;
			totalReward += currentMission->reward;
		}
	}

	// 全ミッションのリスト表示・選択機能
	if (ImGui::CollapsingHeader(u8"サブミッション一覧 & 変更"))
	{
		for (auto& pair : missionPool)
		{
			int round = pair.first;
			MissionPool& pool = pair.second; // 参照で取得

			if (ImGui::TreeNode((u8"レベル " + std::to_string(round)).c_str()))
			{
				for (size_t i = 0; i < pool.size(); ++i)
				{
					// ラムダ式生成前のミッション情報を一時確認用として生成
					MissionData tempMission = pool[i]();

					ImGui::PushID(static_cast<int>(i));

					// ツリーノードで各ミッションを展開
					if (ImGui::TreeNode(tempMission.description.c_str()))
					{
						ImGui::Text(u8"報酬: %d | 達成条件: %d", tempMission.reward, tempMission.required);

						// ボタンを押したらそのミッションを強制的に現在アクティブなミッションにセット
						if (ImGui::Button(u8"このミッションに切り替える"))
						{
							currentMission = std::make_unique<MissionData>(tempMission);
							currentMissionIndex = static_cast<int>(i);
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
		}
	}
}

void SubMission::SaveToJson(json& j)
{
	
	j["listPosition"] = { listPosition.x, listPosition.y };
	//j["fontSize"] = fontSize;
	j["progressPosition"] = { progressPosition.x, progressPosition.y };
	j["progressFontSize"] = progressFontSize;
}

void SubMission::LoadFromJson(const json& j)
{

	if (j.contains("listPosition") && j["listPosition"].is_array() && j["listPosition"].size() == 2)
	{
		listPosition.x = j["listPosition"][0].get<float>();
		listPosition.y = j["listPosition"][1].get<float>();
	}
	//fontSize = j.value("fontSize", 0.3f);
	if(j.contains("progressPosition") && j["progressPosition"].is_array() && j["progressPosition"].size() == 2)
	{
		progressPosition.x = j["progressPosition"][0].get<float>();
		progressPosition.y = j["progressPosition"][1].get<float>();
	}
	progressFontSize = j.value("progressFontSize", 0.25f);
}