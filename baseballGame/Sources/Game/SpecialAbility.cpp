#include "SpecialAbility.h"
#include "Graphics.h"
#include "imgui.h"
#include "shader.h"
#include "physxManager.h"
#include "Player.h"
#include "Pitcher.h"
#include "ballSprite.h"
#include "ballCount.h"
#include "HomeRunCount.h"
#include "RoundManager.h"
#include "Combo.h"

// 条件判定ヘルパー関数の実装
// ボールの方向を判定する関数
bool SpecialAbility::IsDirection(Direction dir) const
{
	float ballAngle = Physics::Instance().GetBallOriginalDirection();
	bool isRightBatter = Player::Instance().IsRightBatter();

	enum class FieldArea { Left, Center, Right, OutOfBounds };// フィールドのエリアを定義(
	FieldArea hitArea = FieldArea::OutOfBounds;

	if (ballAngle >= -15.0f && ballAngle <= 15.0f)
	{
		hitArea = FieldArea::Center;
	}
	if(ballAngle <-15.0f && ballAngle >= -45.0f)
	{
		hitArea = FieldArea::Left;
	}
	if (ballAngle > 15.0f && ballAngle <= 45.0f)
	{
		hitArea = FieldArea::Right;
	}

	switch (dir)
	{
	case Direction::Pull:
		return (isRightBatter && hitArea == FieldArea::Left) || (!isRightBatter && hitArea == FieldArea::Right);
	case Direction::Center:
		return (hitArea == FieldArea::Center);
	case Direction::Opposite:
		return (isRightBatter && hitArea == FieldArea::Right) || (!isRightBatter && hitArea == FieldArea::Left);
	}

	return false;
}

// 高めのボールかどうかを判定する関数
bool SpecialAbility::IsHighBall() const
{
	// 高めのボールの条件を定義
	return ballSprite::Instance().IsHighBall();
}

// 低めのボールかどうかを判定する関数
bool SpecialAbility::IsLowBall() const
{
	// 低めのボールの条件を定義
	return ballSprite::Instance().IsLowBall();
}

// 速球かどうかを判定する関数
bool SpecialAbility::IsFastBall() const
{
	// 速球の条件を定義
	return Pitcher::Instance().IsFastball();
}

// 変化球かどうかを判定する関数
bool SpecialAbility::IsBreakingBall() const
{
	// 変化球の条件を定義
	return Pitcher::Instance().IsBreakingBallBonus();
}

bool SpecialAbility::IsFirstPitch() const
{
	// 初球の条件を定義
	return ballCount::Instance().GetRemainingBalls() == ballCount::Instance().GetInitialBalls();
}

bool SpecialAbility::IsLastBall() const
{
	// ラストボールの条件を定義
	return ballCount::Instance().GetRemainingBalls() == 1;
}

// 残り球数が3球以下、かつ目標本塁打数まであと3以内のときに発動する
bool SpecialAbility::IsLastStandCondition() const
{
	int remainingBalls = ballCount::Instance().GetRemainingBalls();
	int homeRunTarget = RoundManager::Instance().GetCurrentTarget();
	int difference = HomeRunCount::Instance().GetHomeRunCountDifference(homeRunTarget);

	return remainingBalls <= 3 && difference > 0 && difference <= 3;
}

bool SpecialAbility::ComboCount() const
{
	// 連発の条件を定義
	// コンボが1以上であれば発動する
	return Combo::Instance().GetCurrentCombo() >= 1;
}

//ホームランかどうか
bool SpecialAbility::IsHomeRunCondition() const
{
	return (Ball::Instance().GetHasPassedHomeRunZone() && 
		(Ball::Instance().GetHasCollidedWithFence() || Ball::Instance().GetHasCollidedWithGround()))
		|| Ball::Instance().GetHasCollidedWithPole();
}

void SpecialAbility::Initialize(ID3D11Device* device)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	// シェーダーの作成
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());

	

	InitializeAbilities(device, context);
}

void SpecialAbility::InitializeAbilities(ID3D11Device* device, ID3D11DeviceContext* context)
{
	BuildAbility();


	// 各能力のスプライトを初期化
	for (int i = 0; i < ABILITY_COUNT; ++i)
	{
		abilitySpriteData[i] = std::make_unique<Sprite>();
		abilitySpriteData[i]->texturePath = abilities[i].texturePath;
		abilitySpriteData[i]->position = { 100.0f + i * 60.0f, 100.0f }; // 適切な位置に配置
		abilitySpriteData[i]->size = { 300.0f, 50.0f };
		abilitySpriteData[i]->rotation = 0.0f;
		abilitySpriteData[i]->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		abilitySprite[i] = std::make_unique<sprite>(device, context, abilitySpriteData[i]->texturePath.c_str());
	}
}

void SpecialAbility::BuildAbility()
{
	auto& a = abilities;

	// 広角打法
	a[(int)AbilityID::WideAngleBatting].name = u8"広角打法";
	a[(int)AbilityID::WideAngleBatting].texturePath = L".\\resources\\textures\\specialAbilityList\\wideAngleBatting.png";
	a[(int)AbilityID::WideAngleBatting].ballSpeed = 5.0f;
	a[(int)AbilityID::WideAngleBatting].activationRate = 100.0f;
	a[(int)AbilityID::WideAngleBatting].ballCondition = [this]() { return true; }; // 常に発動可能

	// センター返し
	a[(int)AbilityID::CenterReturn].name = u8"センター返し";
	a[(int)AbilityID::CenterReturn].texturePath = L".\\resources\\textures\\specialAbilityList\\centerReturn.png";
	a[(int)AbilityID::CenterReturn].ballSpeed = 5.0f;
	a[(int)AbilityID::CenterReturn].activationRate = 30.0f;
	a[(int)AbilityID::CenterReturn].ballCondition = [this]() { return IsDirection(Direction::Center); }; // センター方向のボールで発動

	// プルヒッター
	a[(int)AbilityID::PullHitter].name = u8"プルヒッター";
	a[(int)AbilityID::PullHitter].texturePath = L".\\resources\\textures\\specialAbilityList\\pullHitter.png";
	a[(int)AbilityID::PullHitter].ballSpeed = 5.0f;
	a[(int)AbilityID::PullHitter].activationRate = 100.0f;
	a[(int)AbilityID::PullHitter].ballCondition = [this]() { return IsDirection(Direction::Pull); }; // プル方向のボールで発動
	a[(int)AbilityID::PullHitter].ballSpeedPenalty = 10.0f; // ペナルティとして球速を下げる
	a[(int)AbilityID::PullHitter].ballPenaltyCondition = [this]() { return IsDirection(Direction::Opposite); }; // 流し方向のボールでペナルティ

	// 流し打ち
	a[(int)AbilityID::OppositeHitter].name = u8"流し打ち";
	a[(int)AbilityID::OppositeHitter].texturePath = L".\\resources\\textures\\specialAbilityList\\oppositeHitter.png";
	a[(int)AbilityID::OppositeHitter].ballSpeed = 5.0f;
	a[(int)AbilityID::OppositeHitter].activationRate = 30.0f;
	a[(int)AbilityID::OppositeHitter].ballCondition = [this]() { return IsDirection(Direction::Opposite); }; // 流し方向のボールで発動
	a[(int)AbilityID::OppositeHitter].ballSpeedPenalty = 10.0f; // ペナルティとして球速を下げる
	a[(int)AbilityID::OppositeHitter].ballPenaltyCondition = [this]() { return IsDirection(Direction::Pull); }; // プル方向のボールでペナルティ

	// ロマン砲
	a[(int)AbilityID::RomanCannon].name = u8"ロマン砲";
	a[(int)AbilityID::RomanCannon].texturePath = L".\\resources\\textures\\specialAbilityList\\romanCannon.png";
	a[(int)AbilityID::RomanCannon].power = 10.0f;
	a[(int)AbilityID::RomanCannon].contact = -15.0f;
	a[(int)AbilityID::RomanCannon].activationRate = 100.0f;
	a[(int)AbilityID::RomanCannon].condition = [this]() { return true; }; // 常に発動可能

	// ハイボールヒッター
	a[(int)AbilityID::HighBallHitter].name = u8"ハイボールヒッター";
	a[(int)AbilityID::HighBallHitter].texturePath = L".\\resources\\textures\\specialAbilityList\\highBallHitter.png";
	a[(int)AbilityID::HighBallHitter].ballSpeed = 5.0f;
	a[(int)AbilityID::HighBallHitter].ballCondition = [this]() { return IsHighBall(); }; // 高めのボールで発動
	a[(int)AbilityID::HighBallHitter].activationRate = 100.0f;
	a[(int)AbilityID::HighBallHitter].ballSpeedPenalty = 10.0f; // ペナルティとして球速を下げる
	a[(int)AbilityID::HighBallHitter].ballPenaltyCondition = [this]() { return IsLowBall(); }; // 低めのボールでペナルティ

	// ローボールヒッター
	a[(int)AbilityID::LowBallHitter].name = u8"ローボールヒッター";
	a[(int)AbilityID::LowBallHitter].texturePath = L".\\resources\\textures\\specialAbilityList\\lowBallHitter.png";
	a[(int)AbilityID::LowBallHitter].ballSpeed = 5.0f;
	a[(int)AbilityID::LowBallHitter].ballCondition = [this]() { return IsLowBall(); }; // 低めのボールで発動
	a[(int)AbilityID::LowBallHitter].activationRate = 30.0f;
	a[(int)AbilityID::LowBallHitter].ballSpeedPenalty = 10.0f; // ペナルティとして球速を下げる
	a[(int)AbilityID::LowBallHitter].ballPenaltyCondition = [this]() { return IsHighBall(); }; // 高めのボールでペナルティ

	// 背水の陣
	a[(int)AbilityID::LastStand].name = u8"背水の陣";
	a[(int)AbilityID::LastStand].texturePath = L".\\resources\\textures\\specialAbilityList\\lastStand.png";
	a[(int)AbilityID::LastStand].power = 15.0f;
	a[(int)AbilityID::LastStand].contact = 15.0f;
	a[(int)AbilityID::LastStand].activationRate = 100.0f;
	a[(int)AbilityID::LastStand].condition = [this]() { return IsLastStandCondition(); }; // 背水の陣の条件で発動

	// フルスイング
	a[(int)AbilityID::FullSwing].name = u8"フルスイング";
	a[(int)AbilityID::FullSwing].texturePath = L".\\resources\\textures\\specialAbilityList\\fullSwing.png";
	a[(int)AbilityID::FullSwing].power = 10.0f;
	a[(int)AbilityID::FullSwing].contact = -10.0f;
	a[(int)AbilityID::FullSwing].activationRate = 30.0f;
	a[(int)AbilityID::FullSwing].condition = [this]() { return true; }; // 常に発動可能

	//ラストボール
	a[(int)AbilityID::LastBall].name = u8"ラストボール";
	a[(int)AbilityID::LastBall].texturePath = L".\\resources\\textures\\specialAbilityList\\lastBall.png";
	a[(int)AbilityID::LastBall].power = 10.0f;
	a[(int)AbilityID::LastBall].contact = 10.0f;
	a[(int)AbilityID::LastBall].activationRate = 100.0f;
	a[(int)AbilityID::LastBall].condition = [this]() { return IsLastBall(); }; // ラストボールの条件で発動

	// 初球
	a[(int)AbilityID::FirstPitcher].name = u8"初球";
	a[(int)AbilityID::FirstPitcher].texturePath = L".\\resources\\textures\\specialAbilityList\\firstPitcher.png";
	a[(int)AbilityID::FirstPitcher].power = 10.0f;
	a[(int)AbilityID::FirstPitcher].contact = 10.0f;
	a[(int)AbilityID::FirstPitcher].activationRate = 100.0f;
	a[(int)AbilityID::FirstPitcher].condition = [this]() { return IsFirstPitch(); }; // 初球の条件で発動

	// 連発
	a[(int)AbilityID::Combo].name = u8"連発";
	a[(int)AbilityID::Combo].texturePath = L".\\resources\\textures\\specialAbilityList\\combo.png";
	a[(int)AbilityID::Combo].comboPowerPerStack = 2.0f;
	a[(int)AbilityID::Combo].activationRate = 100.0f;
	a[(int)AbilityID::Combo].condition = [this]() { return ComboCount(); }; // 連発の条件で発動

	// マネーメーカー
	a[(int)AbilityID::MoneyMaker].name = u8"マネーメーカー";
	a[(int)AbilityID::MoneyMaker].texturePath = L".\\resources\\textures\\specialAbilityList\\moneyMaker.png";
	a[(int)AbilityID::MoneyMaker].moneyMakerBonus = 0.2f; // 獲得金額の20%ボーナス
	a[(int)AbilityID::MoneyMaker].activationRate = 100.0f;

	// 一攫千金
	a[(int)AbilityID::JackPot].name = u8"一攫千金";
	a[(int)AbilityID::JackPot].texturePath = L".\\resources\\textures\\specialAbilityList\\jackPot.png";
	a[(int)AbilityID::JackPot].jackPotMultiplier = 5.0f; // 一攫千金の倍率
	a[(int)AbilityID::JackPot].jackPotChance = 10.0f;

	// 威圧感
	a[(int)AbilityID::Intimidation].name = u8"威圧感";
	a[(int)AbilityID::Intimidation].texturePath = L".\\resources\\textures\\specialAbilityList\\intimidation.png";
	a[(int)AbilityID::Intimidation].pitcherPowerPenalty = 1.0f; // 投手へのペナルティ(球威をワンランクダウンさせる)
	a[(int)AbilityID::Intimidation].pitcherBreakBallPenalty = 1.0f; // 変化球へのペナルティ(変化球をワンランクダウンさせる)
	a[(int)AbilityID::Intimidation].activationRate = 100.0f;
	a[(int)AbilityID::Intimidation].condition = [this]() { return true; }; // 常に発動可能
	// 対速球
	a[(int)AbilityID::VsFastBall].name = u8"対速球";
	a[(int)AbilityID::VsFastBall].texturePath = L".\\resources\\textures\\specialAbilityList\\vsFastBall.png";
	a[(int)AbilityID::VsFastBall].ballSpeed = 5.0f;
	a[(int)AbilityID::VsFastBall].ballCondition = [this]() { return IsFastBall(); }; // 速球の条件で発動
	a[(int)AbilityID::VsFastBall].activationRate = 30.0f; 
	a[(int)AbilityID::VsFastBall].ballSpeedPenalty = 10.0f; // ペナルティとして球速を下げる
	a[(int)AbilityID::VsFastBall].ballPenaltyCondition = [this]() { return IsBreakingBall(); }; // 変化球の条件でペナルティ

	// 対変化球
	a[(int)AbilityID::VsBreakingBall].name = u8"対変化球";
	a[(int)AbilityID::VsBreakingBall].texturePath = L".\\resources\\textures\\specialAbilityList\\vsBreakingBall.png";
	a[(int)AbilityID::VsBreakingBall].ballSpeed = 5.0f;
	a[(int)AbilityID::VsBreakingBall].ballCondition = [this]() { return IsBreakingBall(); }; // 変化球の条件で発動
	a[(int)AbilityID::VsBreakingBall].activationRate = 30.0f;
	a[(int)AbilityID::VsBreakingBall].ballSpeedPenalty = 10.0f; // ペナルティとして球速を下げる
	a[(int)AbilityID::VsBreakingBall].ballPenaltyCondition = [this]() { return IsFastBall(); }; // 速球の条件でペナルティ
}

void SpecialAbility::Uninitialize()
{
	// シェーダーの解放
	spriteVS.Reset();
	spritePS.Reset();
	spriteInputLayout.Reset();
	// スプライトの解放
	for (int i = 0; i < ABILITY_COUNT; ++i)
	{
		abilitySprite[i].reset();
	}

	consoleLog = nullptr;
}

void SpecialAbility::Update(float elapsedTime)
{
	int powerBonus = 0;
	int contactBonus = 0;
	int pitcherPowerPenalty = 0;
	int pitcherBreakPenalty = 0;
	int comboPowerBonus = 0;
	
	for (auto& ability : abilities)
	{
		if (ability.isOwned && ability.isActiveThisRound && ability.condition && ability.condition())
		{
			powerBonus += ability.power;
			contactBonus += ability.contact;
			comboPowerBonus += static_cast<int>(ability.comboPowerPerStack * Combo::Instance().GetCurrentCombo());

		}

		if(ability.isOwned && ability.isActiveThisRound && ability.condition && ability.condition())
		{
			pitcherPowerPenalty += static_cast<int>(ability.pitcherPowerPenalty);
			pitcherBreakPenalty += static_cast<int>(ability.pitcherBreakBallPenalty);

		}
	}

	Player::Instance().ApplyRoundStatBonus(powerBonus + comboPowerBonus, contactBonus);
	ballSprite::Instance().ApplyPowerRankDown(pitcherPowerPenalty);
	ballSprite::Instance().ApplyBreakRankDown(pitcherBreakPenalty);
}

void SpecialAbility::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	//特殊能力のスプライトを描画
	for(int i = 0; i < ABILITY_COUNT; ++i)
	{
		if (abilities[i].isOwned)
		{
			abilitySprite[i]->render(dc,
				abilitySpriteData[i]->position.x, abilitySpriteData[i]->position.y,
				abilitySpriteData[i]->size.x, abilitySpriteData[i]->size.y,
				abilitySpriteData[i]->color.x, abilitySpriteData[i]->color.y, abilitySpriteData[i]->color.z, abilitySpriteData[i]->color.w,
				abilitySpriteData[i]->rotation);
		}
	}

}

void SpecialAbility::RenderAbilityIcons(AbilityID id, const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& size, bool isHighlighted)
{
	int index = static_cast<int>(id);
	if (index < 0 || index >= ABILITY_COUNT) return;
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	// ハイライトの色を設定
	DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	DirectX::XMFLOAT2 hoverSize = size;

	//ホバー中はサイズを少し大きくする
	if (isHighlighted)
	{	
		hoverSize.x *= 1.1f;
		hoverSize.y *= 1.1f;
	}

	abilitySprite[index]->render(dc,
		position.x - hoverSize.x / 2.0f, position.y - hoverSize.y / 2.0f,
		hoverSize.x, hoverSize.y,
		color.x, color.y, color.z, color.w,
		abilitySpriteData[index]->rotation);

	
}

bool SpecialAbility::RollActivation(float ratePercent)
{
	if (ratePercent >= 100.0f) return true;
	std::uniform_real_distribution<float> dist(0.0f, 100.0f);
	return dist(rng) < ratePercent;
}

void SpecialAbility::DrawGUI()
{
	ImGui::Begin("Special Abilities");

	for (int i = 0; i < ABILITY_COUNT; ++i)
	{
		ImGui::PushID(i);

		// 能力名を表示（16文字幅で整列）
		ImGui::Text("%-16s", abilities[i].name.c_str());
		ImGui::SameLine();

		// ===== 1. 所持 / 未所持ボタン =====
		const char* buttonText = abilities[i].isOwned ? u8"[所持中]" : u8"[未所持]";
		if (ImGui::Button(buttonText))
		{
			abilities[i].isOwned = !abilities[i].isOwned;
		}

		ImGui::SameLine();

		// ===== 2. 発動状態の表示（色分け） =====
		if (abilities[i].isActiveThisRound)
		{
			// 発動中：緑色で強調表示 (R, G, B, A)
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.2f, 1.0f), u8"発動中");
		}
		else
		{
			// 停止中・条件外：グレーアウト表示
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), u8"  --  ");
		}

		ImGui::PopID();
	}

	ImGui::End();
}

void SpecialAbility::RollRoundActivation()
{
	for (auto& ability : abilities)
	{
		if (ability.isOwned)
		{
			ability.isActiveThisRound = RollActivation(ability.activationRate);
		}
		else
		{
			ability.isActiveThisRound = false;
		}
	}
}

float SpecialAbility::GetBallVelocityBonus() const
{
	float ballSpeedBonus = 1.0f;

	for(auto& ability : abilities)
	{
		if (ability.isOwned && ability.isActiveThisRound)
		{
			if (ability.ballCondition && ability.ballCondition())
			{
				//ボーナスをパーセントに変換して加算する
				ballSpeedBonus += (ability.ballSpeed / 100.0f);

				if(consoleLog)
				{
					consoleLog->push_back(u8"球速ボーナス発動: " + ability.name);
				}

			}
			else if (ability.ballPenaltyCondition && ability.ballPenaltyCondition())
			{
				ballSpeedBonus -= (ability.ballSpeedPenalty / 100.0f);

				if(consoleLog)
				{
					consoleLog->push_back(u8"球速ペナルティ発動: " + ability.name);
				}
			}
		}
	}

	return ballSpeedBonus;
}
