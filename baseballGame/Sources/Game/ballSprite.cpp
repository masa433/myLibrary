#include "ballSprite.h"
#include "Graphics.h"
#include <shader.h>
#include <imgui.h>
#include "Pitcher.h"
#include "Ball.h"
#include <algorithm>
#include <cmath>

namespace
{
	float Clamp01(float value)
	{
		return (std::max)(0.0f, (std::min)(1.0f, value));
	}

	float SmoothStep(float value)
	{
		value = Clamp01(value);
		return value * value * (3.0f - 2.0f * value);
	}

	DirectX::XMFLOAT2 EvalCubicBezier2D(
		const DirectX::XMFLOAT2& p0,
		const DirectX::XMFLOAT2& p1,
		const DirectX::XMFLOAT2& p2,
		const DirectX::XMFLOAT2& p3,
		float t)
	{
		const float u = 1.0f - t;
		const float u2 = u * u;
		const float u3 = u2 * u;
		const float t2 = t * t;
		const float t3 = t2 * t;

		return {
			u3 * p0.x + 3.0f * u2 * t * p1.x + 3.0f * u * t2 * p2.x + t3 * p3.x,
			u3 * p0.y + 3.0f * u2 * t * p1.y + 3.0f * u * t2 * p2.y + t3 * p3.y
		};
	}

	DirectX::XMFLOAT2 EvalPitchBreakScreenPath(
		const DirectX::XMFLOAT2& targetScreenPos,
		const ballSprite::ballBreak2D& breakData,
		const DirectX::XMFLOAT2& zoneScreenSize,
		int pitchBreakIndex,
		float t,
		bool isRightPitcher)
	{
		//左投手の時はX軸を反転する
		float effectiveBreakX = isRightPitcher ? breakData.breakX : -breakData.breakX;

		const float breakOffsetX = (effectiveBreakX / 43.0f) * zoneScreenSize.x;
		const float breakOffsetY = (breakData.breakY / 60.0f) * zoneScreenSize.y;

		DirectX::XMFLOAT2 p0 = {
			targetScreenPos.x - breakOffsetX,
			targetScreenPos.y + breakOffsetY
		};
		DirectX::XMFLOAT2 travel = {
			targetScreenPos.x - p0.x,
			targetScreenPos.y - p0.y
		};

		DirectX::XMFLOAT2 p1 = {
			p0.x + travel.x * 0.08f,
			p0.y + travel.y * 0.08f
		};
		DirectX::XMFLOAT2 p2 = {
			targetScreenPos.x - travel.x * 0.30f,
			targetScreenPos.y - travel.y * 0.10f
		};

		// 球種ごとの変化量を調整
		switch (pitchBreakIndex)
		{
		case 3:  // スライダー
		case 2:  // カットボール
		case 11: // シュート
		case 1:  // ツーシーム
		case 14: // スイーパー
			p1.y = p0.y + travel.y * 0.03f;
			p2.y = targetScreenPos.y + travel.y * 0.04f;
			break;
		case 4:  // カーブ
		case 10: // スローカーブ
			p1.x = p0.x + travel.x * 0.04f;
			p1.y = p0.y + travel.y * 0.02f;
			p2.x = targetScreenPos.x - travel.x * 0.42f;
			p2.y = targetScreenPos.y - travel.y * 0.22f;
			break;
		case 8:  // 縦スライダー
		case 6:  // フォークボール
		case 9:  // スプリット
			p1.x = p0.x + travel.x * 0.02f;
			p2.x = targetScreenPos.x - travel.x * 0.08f;
			break;
		default:
			break;
		}

		//const float lateBreakT = SmoothStep((t - 0.12f) / 0.88f);
		return EvalCubicBezier2D(p0, p1, p2, targetScreenPos, Clamp01(t));
	}
}
static DirectX::XMFLOAT2 WorldToZoneScreen(
	float worldX, float worldY,
	const DirectX::XMFLOAT2& zoneScreenPos,  // ゾーンスプライト左上
	const DirectX::XMFLOAT2& zoneScreenSize, // ゾーンスプライトサイズ(px)
	const DirectX::XMFLOAT2& zone3DCenter,   // 3Dゾーン中心(x,y)
	const DirectX::XMFLOAT2& zone3DSize,     // 3Dゾーンサイズ(m)
	bool isRightPitcher)     
{
	// 3D座標をゾーン内の正規化座標(0~1)に変換
	float normalX = (worldX - (zone3DCenter.x - zone3DSize.x * 0.5f)) / zone3DSize.x;

	// 左投手なら正規化座標を反転させる
	if (!isRightPitcher) {
		normalX = 1.0f - normalX;
	}

	float normalY = (worldY - (zone3DCenter.y - zone3DSize.y * 0.5f)) / zone3DSize.y;

	// Y軸反転（3DはY上が正、2DはY下が正）
	normalY = 1.0f - normalY;

	// スクリーン座標に変換（ボール画像の中心を基準）
	float screenX = zoneScreenPos.x + normalX * zoneScreenSize.x;
	float screenY = zoneScreenPos.y + normalY * zoneScreenSize.y;

	return { screenX, screenY };
}

// 3D座標から2Dスクリーン座標への変換
static DirectX::XMFLOAT2 ZoneScreenToWorld(
	float screenX, float screenY,
	const DirectX::XMFLOAT2& zoneScreenPos,  // ゾーンスプライト左上
	const DirectX::XMFLOAT2& zoneScreenSize, // ゾーンスプライトサイズ(px)
	const DirectX::XMFLOAT2& zone3DCenter,   // 3Dゾーン中心(x,y)
	const DirectX::XMFLOAT2& zone3DSize,     // 3Dゾーンサイズ(m)
	bool isRightPitcher)     
{
	// スクリーン座標を正規化座標(0~1)に変換
	float normalX = (screenX - zoneScreenPos.x) / zoneScreenSize.x;

	// 左投手なら正規化座標を反転させる
	if (!isRightPitcher) {
		normalX = 1.0f - normalX;
	}

	float normalY = (screenY - zoneScreenPos.y) / zoneScreenSize.y;
	// Y軸反転（3DはY上が正、2DはY下が正）
	normalY = 1.0f - normalY;
	// 正規化座標を3D座標に変換
	float worldX = (zone3DCenter.x - zone3DSize.x * 0.5f) + normalX * zone3DSize.x;
	float worldY = (zone3DCenter.y - zone3DSize.y * 0.5f) + normalY * zone3DSize.y;
	return { worldX, worldY };
}

DirectX::XMFLOAT2 ballSprite::GetAITarget3D() const
{
	return ZoneScreenToWorld(
		aiTargetScreen.x, aiTargetScreen.y,
		strikeZoneSpriteData->position,
		strikeZoneSpriteData->size,
		zone3DCenter,
		zone3DSize,
		Pitcher::Instance().IsRightPitcher());
}

void ballSprite::SetAITargetFromWorld(float worldX, float worldY)
{
	aiTargetScreen = WorldToZoneScreen(
		worldX, worldY,
		strikeZoneSpriteData->position,
		strikeZoneSpriteData->size,
		zone3DCenter,
		zone3DSize,
		Pitcher::Instance().IsRightPitcher());
	hasAITarget = true;
}

void ballSprite::GetBallZoneScreenBounds(DirectX::XMFLOAT2& outTopLeft, DirectX::XMFLOAT2& outBottomRight) const
{
	if (!strikeZoneSpriteData)
	{
		//初期化
		outTopLeft = { 0.0f,0.0f };
		outBottomRight = { 0.0f,0.0f };
		return;
	}

	
	const DirectX::XMFLOAT2& worldTopLeft = ballZoneGrid[0][0];
	const DirectX::XMFLOAT2& worldBottomRight = ballZoneGrid[4][4];

	DirectX::XMFLOAT2 screenA = WorldToZoneScreen(
		worldTopLeft.x, worldTopLeft.y,
		strikeZoneSpriteData->position,
		strikeZoneSpriteData->size,
		zone3DCenter,
		zone3DSize,
		Pitcher::Instance().IsRightPitcher());

	DirectX::XMFLOAT2 screenB = WorldToZoneScreen(
		worldBottomRight.x, worldBottomRight.y,
		strikeZoneSpriteData->position,
		strikeZoneSpriteData->size,
		zone3DCenter,
		zone3DSize,
		Pitcher::Instance().IsRightPitcher());

	outTopLeft.x = (std::min)(screenA.x, screenB.x);
	outTopLeft.y = (std::min)(screenA.y, screenB.y);
	outBottomRight.x = (std::max)(screenA.x, screenB.x);
	outBottomRight.y = (std::max)(screenA.y, screenB.y);
}

void ballSprite::Initialize(ID3D11Device* device)
{
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, "sprite_vs.cso", spriteVS.GetAddressOf(), spriteInputLayout.GetAddressOf(),
		input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, "sprite_ps.cso", spritePS.GetAddressOf());

	//ストライクゾーンの初期化
	strikeZoneSpriteData = std::make_unique<Sprite>();
	strikeZoneSpriteData->texturePath = L".\\resources\\textures\\strikeZone.png";
	strikeZoneSpriteData->position = { 1100.0f, 400.0f };
	strikeZoneSpriteData->size = { 200.0f, 300.0f };
	strikeZoneSpriteData->rotation = 0.0f;
	strikeZoneSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	strikeZoneSprite = std::make_unique<sprite>(device, strikeZoneSpriteData->texturePath.c_str());

	//ボールの初期化
	ballDebugSpriteData = std::make_unique<Sprite>();
	ballDebugSpriteData->texturePath = L".\\resources\\textures\\ball.png";
	ballDebugSpriteData->position = { 1100.0f, 400.0f };
	ballDebugSpriteData->size = { 30.0f, 30.0f };
	ballDebugSpriteData->rotation = 0.0f;
	ballDebugSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	ballDebugSprite = std::make_unique<sprite>(device, ballDebugSpriteData->texturePath.c_str());

	//ボールボードの初期化
	ballBoardSpriteData = std::make_unique<Sprite>();
	ballBoardSpriteData->texturePath = L".\\resources\\textures\\ballBoard.png";
	ballBoardSpriteData->position = { 1100.0f, 400.0f };
	ballBoardSpriteData->size = { 200.0f, 300.0f };
	ballBoardSpriteData->rotation = 0.0f;
	ballBoardSpriteData->color = { 1.0f, 1.0f, 1.0f, 0.7f };
	ballBoardSprite = std::make_unique<sprite>(device, ballBoardSpriteData->texturePath.c_str());

	// added: pitch info font init (日本語対応版)
	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	// 球種名と球速表示に必要な文字だけをベイクする
	std::vector<int> pitchInfoCodepoints = FontRenderer::Utf8ToCodepoints(
		u8"0123456789.km/h"
		u8"失投ストレートスライダー"
		u8"カーブチェンジアップフォーク"
		u8"ツーシームカットボールシンカー"
		u8"スクリュー縦スプリットスローカーブ"
		u8"シュートナックルボールスイーパーパーム"
	);

	// 日本語グリフを持つフォントを用意して配置する
	pitchInfoFont.Initialize(device,
		L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf",
		28.0f,
		screenWidth, screenHeight,
		512, 512,
		&pitchInfoCodepoints);
}

void ballSprite::Uninitialize()
{
	strikeZoneSprite.reset();
	strikeZoneSpriteData.reset();
	ballDebugSprite.reset();
	ballDebugSpriteData.reset();
	ballBoardSprite.reset();
	ballBoardSpriteData.reset();
	pitchInfoFont.Uninitialize();
}

void ballSprite::Update(float elapsedTime)
{
	//実在投手の切り替えを検知し、その投手の球種に応じた変化量を設定する
	SyncRealPitcherBreaks();

	Pitcher& pitcher = Pitcher::Instance();
	Ball& ball = Ball::Instance();

	currentPitchIndex = Pitcher::PitchTypeToBreakIndex(pitcher.GetSelectedPitchType());

	const bool pitchingState = (pitcher.GetCurrentState() == Pitcher::State::Throwing);
	const bool nowThrown = pitcher.GetIsBallThrown();

	if (nowThrown && !prevThrown)
	{
		ballTrail2D.clear();
		strikeJudgeDone = false;
	}
	prevThrown = nowThrown;

	const DirectX::XMFLOAT3& wp = ball.GetWorldPosition();

	auto AddTrailPoint = [&](const DirectX::XMFLOAT2& currentScreenPos)
	{
		if (ballTrail2D.empty() ||
			fabsf(ballTrail2D.back().x - currentScreenPos.x) > 0.5f ||
			fabsf(ballTrail2D.back().y - currentScreenPos.y) > 0.5f)
		{
			ballTrail2D.push_back(currentScreenPos);
			if ((int)ballTrail2D.size() > MAX_TRAIL)
			{
				ballTrail2D.pop_front();
			}
		}
	};

	auto ApplyBallSpritePosition = [&](const DirectX::XMFLOAT2& currentScreenPos)
	{
		ballDebugSpriteData->position.x = currentScreenPos.x - ballDebugSpriteData->size.x * 0.5f;
		ballDebugSpriteData->position.y = currentScreenPos.y - ballDebugSpriteData->size.y * 0.5f;
	};

	auto GetPitchProgress = [&]()
	{
		if (nowThrown)
		{
			float t = Clamp01(ball.GetBezierT());
			//P1とP3の間での進行度を返す
			static constexpr float P1T = 0.15f;
			static constexpr float P3T = 1.0f;
			float linear;
			if (t < P1T)
			{
				linear = 0.0f;
			}
			else if (t > P3T)
			{	
				linear =  1.0f;
			}
			else
			{
				linear = (t - P1T) / (P3T - P1T);
			}
			return SmoothStep(linear);
		}
		if (pitchingState)
		{
			return 0.0f;
		}
		return 0.0f;
	};

	if (hasAITarget)
	{
		const ballBreak2D& brk = pitchBreaks[currentPitchIndex];
		const DirectX::XMFLOAT2 targetScreenPos = aiTargetScreen;
		DirectX::XMFLOAT2 currentScreenPos;

		// ベジェ曲線が終着点(t>=1.0)に到達済みの場合は、
		// 補間計算を経由せず必ず終着点(targetScreenPos)にスプライトを一致させる
		if (nowThrown && !ball.IsBezierFlying() && ball.GetBezierT() >= 1.0f)
		{
			currentScreenPos = targetScreenPos;
		}
		else
		{
			currentScreenPos = EvalPitchBreakScreenPath(
				targetScreenPos,
				brk,
				strikeZoneSpriteData->size,
				currentPitchIndex,
				GetPitchProgress(),
				pitcher.IsRightPitcher());
		}

		ApplyBallSpritePosition(currentScreenPos);
		if (nowThrown)
		{
			AddTrailPoint(currentScreenPos);
		}

		//3Dボールのポジションzが0.0fの時またはボールとバットが当たった時に、BallBoardを表示する
		if(Ball::Instance().GetWorldPosition().z <= 0.0f || Ball::Instance().GetHasCollided())
		{
			showBallBoard = true;
		}
		else
		{
			showBallBoard = false;
		}
	}
	else if (nowThrown && wp.z >= -0.5f && wp.z <= 18.5f)
	{
		float t = Clamp01(ball.GetBezierT());
		if (t <= 0.0f)
		{
			t = Clamp01(1.0f - (wp.z / 18.0f));
		}

		DirectX::XMFLOAT2 currentScreenPos = {};

		if (useBallBreak)
		{
			const ballBreak2D& brk = pitchBreaks[currentPitchIndex];
			DirectX::XMFLOAT2 targetScreenPos = WorldToZoneScreen(
				zone3DCenter.x,
				zone3DCenter.y,
				strikeZoneSpriteData->position,
				strikeZoneSpriteData->size,
				zone3DCenter,
				zone3DSize,
				pitcher.IsRightPitcher());

			currentScreenPos = EvalPitchBreakScreenPath(
				targetScreenPos,
				brk,
				strikeZoneSpriteData->size,
				currentPitchIndex,
				t,
				pitcher.IsRightPitcher());
		}
		else
		{
			physx::PxVec3 velocity = ball.GetLinearVelocity();

			float finalX = wp.x;
			float finalY = wp.y;

			if (velocity.z < -0.001f && wp.z > 0.0f)
			{
				float t_remain = -wp.z / velocity.z;
				finalX = wp.x + velocity.x * t_remain;
				finalY = wp.y + velocity.y * t_remain;
			}

			DirectX::XMFLOAT2 finalScreenPos = WorldToZoneScreen(
				finalX, finalY,
				strikeZoneSpriteData->position,
				strikeZoneSpriteData->size,
				zone3DCenter,
				zone3DSize,
				pitcher.IsRightPitcher());

			DirectX::XMFLOAT2 startScreenPos = WorldToZoneScreen(
				zone3DCenter.x, zone3DCenter.y,
				strikeZoneSpriteData->position,
				strikeZoneSpriteData->size,
				zone3DCenter,
				zone3DSize,
				pitcher.IsRightPitcher());

			currentScreenPos = {
				startScreenPos.x + (finalScreenPos.x - startScreenPos.x) * t,
				startScreenPos.y + (finalScreenPos.y - startScreenPos.y) * t
			};
		}

		AddTrailPoint(currentScreenPos);
		ApplyBallSpritePosition(currentScreenPos);
	}

	if (pitchingState && wp.z < -0.5f && wp.z > -0.7f && !strikeJudgeDone)
	{
		strikeJudgeDone = true;

		DirectX::XMFLOAT2 ballCenter = {
			ballDebugSpriteData->position.x + ballDebugSpriteData->size.x * 0.5f,
			ballDebugSpriteData->position.y + ballDebugSpriteData->size.y * 0.5f
		};
		DirectX::XMFLOAT2 szTopLeft, szBottomRight;
		GetStrikeZoneScreenBounds(szTopLeft, szBottomRight);

		bool isStrike = (ballCenter.x >= szTopLeft.x && ballCenter.x <= szBottomRight.x &&
			ballCenter.y >= szTopLeft.y && ballCenter.y <= szBottomRight.y);

		if (consoleLog)
		{
			char buf[256];
			if (isStrike)
				snprintf(buf, sizeof(buf), u8"[Info] ストライク！");
			else
				snprintf(buf, sizeof(buf), u8"[Info] ボール！");
			consoleLog->push_back(buf);
		}
	}
}

void ballSprite::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();

	// Wind と同じようにシェーダーをセット
	dc->VSSetShader(spriteVS.Get(), nullptr, 0);
	dc->PSSetShader(spritePS.Get(), nullptr, 0);
	dc->IASetInputLayout(spriteInputLayout.Get());

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	if (strikeZoneSprite && strikeZoneSpriteData)
	{
		strikeZoneSprite->render(dc,
			strikeZoneSpriteData->position.x, strikeZoneSpriteData->position.y,
			strikeZoneSpriteData->size.x, strikeZoneSpriteData->size.y,
			strikeZoneSpriteData->color.x, strikeZoneSpriteData->color.y,
			strikeZoneSpriteData->color.z, strikeZoneSpriteData->color.w,
			strikeZoneSpriteData->rotation);
	}

	
	if (ballDebugSprite && ballDebugSpriteData)
	{
		ballDebugSprite->render(dc,
			ballDebugSpriteData->position.x, ballDebugSpriteData->position.y,
			ballDebugSpriteData->size.x, ballDebugSpriteData->size.y,
			ballDebugSpriteData->color.x, ballDebugSpriteData->color.y,
			ballDebugSpriteData->color.z, ballDebugSpriteData->color.w,
			ballDebugSpriteData->rotation);
	}

	if(ballBoardSprite && ballBoardSpriteData && showBallBoard)
	{
		ballBoardSprite->render(dc,
			ballBoardSpriteData->position.x, ballBoardSpriteData->position.y,
			ballBoardSpriteData->size.x, ballBoardSpriteData->size.y,
			ballBoardSpriteData->color.x, ballBoardSpriteData->color.y,
			ballBoardSpriteData->color.z, ballBoardSpriteData->color.w,
			ballBoardSpriteData->rotation);

		if (pitchInfoFont.IsValid())
		{
			Pitcher& pitcher = Pitcher::Instance();
			const char* pitchTypeName = pitcher.GetPitchTypeName(pitcher.GetSelectedPitchType());
			const float ballSpeedKmh = pitcher.GetBallSpeedKmh();

			char speedText[32];
			snprintf(speedText, sizeof(speedText), "%.0fkm/h", ballSpeedKmh);

			const int pitchIndex = Pitcher::PitchTypeToBreakIndex(pitcher.GetSelectedPitchType());
			const DirectX::XMFLOAT2& nameOffset = pitchNameOffsets[pitchIndex];
			const DirectX::XMFLOAT2& speedOffset = pitchSpeedOffsets[pitchIndex];

			const float boardTop = ballBoardSpriteData->position.y;
			const float boardBottom = ballBoardSpriteData->position.y + ballBoardSpriteData->size.y;
			const float boardCenterY = boardTop + (boardBottom - boardTop) * 0.5f;

			float speedTextWidth = 0.0f, speedTextHeight = 0.0f;
			pitchInfoFont.MeasureText(speedText, 1.0f, speedTextWidth, speedTextHeight);

			// 左側: 球種（白固定）
			const float pitchTextX = ballBoardSpriteData->position.x + nameOffset.x;
			const float pitchTextY = boardCenterY + nameOffset.y;
			pitchInfoFont.DrawTextW(dc, pitchTypeName, pitchTextX, pitchTextY, pitchInfoFontScale, 1.0f, 1.0f, 1.0f, 1.0f);

			// 右側: 球速（150km/h超で黄色）
			const DirectX::XMFLOAT4& speedColor = (ballSpeedKmh > pitchSpeedFastThresholdKmh) ? pitchSpeedFastColor : pitchSpeedNormalColor;
			const float speedTextX = ballBoardSpriteData->position.x + ballBoardSpriteData->size.x - speedOffset.x - speedTextWidth;
			const float speedTextY = boardCenterY + speedOffset.y;
			pitchInfoFont.DrawTextW(dc, speedText, speedTextX, speedTextY, pitchInfoFontScale, speedColor.x, speedColor.y, speedColor.z, speedColor.w);
		}
	}

	
	// 後始末（Wind と同じ）
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	dc->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

	
}

void ballSprite::DrawGUI()
{
	// GUI drawing logic for ball sprite if needed
	if (ImGui::CollapsingHeader(u8"2D スプライト"))
	{
		ImGui::DragFloat2(u8"ゾーン 位置(px)", &strikeZoneSpriteData->position.x, 1.0f);
		ImGui::DragFloat2(u8"ゾーン サイズ(px)", &strikeZoneSpriteData->size.x, 1.0f, 1.0f, 2000.0f);
		ImGui::ColorEdit4(u8"ゾーン 透明度", &strikeZoneSpriteData->color.x);

		ImGui::Separator();

		ImGui::DragFloat2(u8"ボール 位置(px)", &ballDebugSpriteData->position.x, 1.0f);
		ImGui::DragFloat2(u8"ボール サイズ(px)", &ballDebugSpriteData->size.x, 1.0f, 1.0f, 2000.0f);
		ImGui::ColorEdit4(u8"ボール 透明度", &ballDebugSpriteData->color.x);

		ImGui::Separator();

		ImGui::DragFloat2(u8"ボールボード 位置(px)", &ballBoardSpriteData->position.x, 1.0f);
		ImGui::DragFloat2(u8"ボールボード サイズ(px)", &ballBoardSpriteData->size.x, 1.0f, 1.0f, 2000.0f);
		ImGui::ColorEdit4(u8"ボールボード 透明度", &ballBoardSpriteData->color.x);
		ImGui::Checkbox(u8"ボールボード表示", &showBallBoard);
	}

	if (ImGui::CollapsingHeader(u8"球種情報フォント"))
	{
		ImGui::DragFloat(u8"フォントサイズ", &pitchInfoFontScale, 0.1f, 1.0f, 100.0f);

		ImGui::Separator();
		ImGui::Text(u8"球速 色設定");
		ImGui::ColorEdit4(u8"通常色", &pitchSpeedNormalColor.x);
		ImGui::ColorEdit4(u8"速球色", &pitchSpeedFastColor.x);
		ImGui::DragFloat(u8"速球判定 (km/h)", &pitchSpeedFastThresholdKmh, 1.0f, 0.0f, 300.0f);
	}

	const Pitcher::RealPitcher selectedRP = Pitcher::Instance().GetSelectedRealPitcher();
	if (selectedRP != Pitcher::RealPitcher::None)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), u8"※「%s」専用の変化量データを編集中（他の投手には影響しません）",
			Pitcher::GetRealPitcherName(selectedRP));
	}
	else
	{
		ImGui::Text(u8"※現在は共通エディター値を編集中（実在投手プリセット未選択）");
	}

	const char* names[] = {
		u8"ストレート", u8"スライダー", u8"カーブ", u8"チェンジアップ", u8"フォーク",
		u8"ツーシーム", u8"カットボール", Pitcher::Instance().IsRightPitcher() ? u8"シンカー" : u8"スクリュー", u8"縦スライダー", u8"スプリット",
		u8"スローカーブ", u8"シュート", u8"ナックルボール", u8"スローボール", u8"スイーパー", u8"パーム"
	};

	ImGui::Combo(u8"編集する球種", reinterpret_cast<int*>(&Pitcher::Instance().selectedPitchType), names, 16);

	const int editIndex = Pitcher::PitchTypeToBreakIndex(Pitcher::Instance().GetSelectedPitchType());

	// 実在投手選択中なら、その投手専用の変化量セットを直接編集する
	PitchBreakSet* activeSet = nullptr;
	if (selectedRP != Pitcher::RealPitcher::None)
	{
		activeSet = &realPitcherBreaks[static_cast<int>(selectedRP)];
	}

	ballBreak2D& brk = pitchBreaks[editIndex];

	// ----- グレード（段階）で一括指定 -----
	if (activeSet != nullptr)
	{
		int gradeIndex = static_cast<int>(activeSet->grades[editIndex]); // E=0 ... S=5
		static const char* gradeNames[] = { u8"E", u8"D", u8"C", u8"B", u8"A", u8"S" };
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), u8"曲がりグレード : %s", GetBreakGradeLabel(activeSet->grades[editIndex]));
		if (ImGui::Combo(u8"グレード変更", &gradeIndex, gradeNames, IM_ARRAYSIZE(gradeNames)))
		{
			activeSet->grades[editIndex] = static_cast<Pitcher::BreakGrade>(gradeIndex);
			// グレード基準の形状（=C相当の向き）に対して倍率をかけ直す
			const float baseScale = GetBreakGradeScale(Pitcher::BreakGrade::C);
			const float newScale = GetBreakGradeScale(activeSet->grades[editIndex]);
			const float ratio = newScale / baseScale;
			activeSet->breaks[editIndex].breakX = brk.breakX * ratio;
			activeSet->breaks[editIndex].breakY = brk.breakY * ratio;
			brk = activeSet->breaks[editIndex];
		}
		ImGui::Spacing();
	}

	// ----- 数値での微調整（従来通り） -----
	float displayBreakX = Pitcher::Instance().IsRightPitcher() ? brk.breakX : -brk.breakX;
	if (ImGui::SliderFloat(u8"横変化 (+ アウト / - イン)", &displayBreakX, -20.0f, 20.0f, "%.1f cm"))
	{
		brk.breakX = Pitcher::Instance().IsRightPitcher() ? displayBreakX : -displayBreakX;
		if (activeSet != nullptr)
		{
			activeSet->breaks[editIndex] = brk; // 投手専用データにも反映
		}
	}
	if (ImGui::SliderFloat(u8"縦変化 (+ 落ち / - 上げ)", &brk.breakY, -25.0f, 10.0f, "%.1f cm"))
	{
		if (activeSet != nullptr)
		{
			activeSet->breaks[editIndex] = brk; // 投手専用データにも反映
		}
	}

	ImGui::Checkbox(u8"変化量を反映", &useBallBreak);

	ImGui::Separator();
	ImGui::Text(u8"球種名/球速 表示位置 (%s)", names[editIndex]);
	ImGui::DragFloat2(u8"球種名 オフセット(左右, Y)##pitchName", &pitchNameOffsets[editIndex].x, 0.5f, -200.0f, 500.0f);
	ImGui::DragFloat2(u8"球速 オフセット(右寄せ, Y)##pitchSpeed", &pitchSpeedOffsets[editIndex].x, 0.5f, -200.0f, 500.0f);
}

void ballSprite::BuildRealPitcherBreakSet(Pitcher::RealPitcher rp)
{
	int index = static_cast<int>(rp);
	if(index < 0 || index >= static_cast<int>(Pitcher::RealPitcher::Count))
	{
		return;// 無効な投手
	}

	PitchBreakSet& breakSet = realPitcherBreaks[index];
	if(breakSet.initialized)
	{
		return; // すでに初期化済み
	}

	// この時点のpitchBreaks[16]を「グレードCの基準形状」として使用する
	ballBreak2D baseShape[16];
	for(int i = 0; i < 16; ++i)
	{
		baseShape[i] = pitchBreaks[i];
		breakSet.breaks[i] = pitchBreaks[i];
		breakSet.grades[i] = Pitcher::BreakGrade::C; // デフォルトはグレードC
	}

	std::vector<Pitcher::RealArsenalEntry> arsenal;
	bool isRight = true;
	const char* name = "";
	if (!Pitcher::GetRealPitcherArsenalData(rp, arsenal, isRight, name))
	{
		breakSet.initialized = true;
		return;
	}

	// arsenalの各球種について、グレードに応じてpitchBreaksを調整する
	for(const Pitcher::RealArsenalEntry& entry : arsenal)
	{
		int breakIndex = Pitcher::PitchTypeToBreakIndex(entry.pitchType);
		if(breakIndex < 0 || breakIndex >= 16)
		{
			continue; // 無効な球種
		}

		// グレードに応じて変化量を調整する
		const float scale = GetBreakGradeScale(entry.breakGrade);
		breakSet.breaks[breakIndex].breakX = baseShape[breakIndex].breakX * scale;
		breakSet.breaks[breakIndex].breakY = baseShape[breakIndex].breakY * scale;
		breakSet.grades[breakIndex] = entry.breakGrade;
	}

	breakSet.initialized = true;
}

void ballSprite::SyncRealPitcherBreaks()
{
	const Pitcher::RealPitcher currentRealPitcher = Pitcher::Instance().GetSelectedRealPitcher();
	if (currentRealPitcher == lastAppliedPitcher)
	{
		return; // 変更なし
	}
	lastAppliedPitcher = currentRealPitcher;

	if(currentRealPitcher == Pitcher::RealPitcher::None)
	{
		return; // 実在投手なし
	}

	BuildRealPitcherBreakSet(currentRealPitcher);

	const int index = static_cast<int>(currentRealPitcher);
	const PitchBreakSet& breakSet = realPitcherBreaks[index];
	for(int i = 0; i < 16; ++i)
	{
		pitchBreaks[i] = breakSet.breaks[i];
	}
}

void ballSprite::SaveToJson(json& j)
{
	j["strikeZoneSprite"] = {
		{"texturePath", strikeZoneSpriteData->texturePath},
		{"position", {strikeZoneSpriteData->position.x, strikeZoneSpriteData->position.y}},
		{"size", {strikeZoneSpriteData->size.x, strikeZoneSpriteData->size.y}},
		{"rotation", strikeZoneSpriteData->rotation},
		{"color", {strikeZoneSpriteData->color.x, strikeZoneSpriteData->color.y, strikeZoneSpriteData->color.z, strikeZoneSpriteData->color.w}}
	};
	j["ballDebugSprite"] = {
		{"texturePath", ballDebugSpriteData->texturePath},
		{"position", {ballDebugSpriteData->position.x, ballDebugSpriteData->position.y}},
		{"size", {ballDebugSpriteData->size.x, ballDebugSpriteData->size.y}},
		{"rotation", ballDebugSpriteData->rotation},
		{"color", {ballDebugSpriteData->color.x, ballDebugSpriteData->color.y, ballDebugSpriteData->color.z, ballDebugSpriteData->color.w}}
	};
	j["ballBoardSprite"] = {
		{"texturePath", ballBoardSpriteData->texturePath},
		{"position", {ballBoardSpriteData->position.x, ballBoardSpriteData->position.y}},
		{"size", {ballBoardSpriteData->size.x, ballBoardSpriteData->size.y}},
		{"rotation", ballBoardSpriteData->rotation},
		{"color", {ballBoardSpriteData->color.x, ballBoardSpriteData->color.y, ballBoardSpriteData->color.z, ballBoardSpriteData->color.w}},
		{"showBallBoard", showBallBoard }
	};
	j["pitchInfoFont"] = {
		{"fontScale", pitchInfoFontScale},
		{"pitchSpeedNormalColor", {pitchSpeedNormalColor.x, pitchSpeedNormalColor.y, pitchSpeedNormalColor.z, pitchSpeedNormalColor.w}},
		{"pitchSpeedFastColor", {pitchSpeedFastColor.x, pitchSpeedFastColor.y, pitchSpeedFastColor.z, pitchSpeedFastColor.w}},
		{"pitchSpeedFastThresholdKmh", pitchSpeedFastThresholdKmh}
	};
	// 変化量エディタの有効フラグを保存
	j["useBallBreak"] = useBallBreak;

	json realPitcherBreaksJson = json::array();
	for (size_t rp = 1; rp < realPitcherBreaks.size(); ++rp) // 0=Noneはスキップ
	{
		if (!realPitcherBreaks[rp].initialized) continue;

		json breaksArr = json::array();
		for (int i = 0; i < 16; ++i)
		{
			breaksArr.push_back({
				{"breakX", realPitcherBreaks[rp].breaks[i].breakX},
				{"breakY", realPitcherBreaks[rp].breaks[i].breakY},
				{"grade", static_cast<int>(realPitcherBreaks[rp].grades[i])}
				});
		}
		realPitcherBreaksJson.push_back({
			{"pitcher", static_cast<int>(rp)},
			{"breaks", breaksArr}
			});
	}
	j["realPitcherBreaks"] = realPitcherBreaksJson;

	// 全14球種の変化量をJSONの配列オブジェクトとしてまとめて保存
	json breaksArray = json::array();
	for (int i = 0; i < 16; ++i)
	{
		breaksArray.push_back({
			{"breakX", pitchBreaks[i].breakX},
			{"breakY", pitchBreaks[i].breakY}
			});
	}
	j["pitchBreaks"] = breaksArray;

	j["pitchNameOffsets"] = json::array();
	for (int i = 0; i < 16; ++i)
	{
		j["pitchNameOffsets"].push_back({ pitchNameOffsets[i].x, pitchNameOffsets[i].y });
	}

	j["pitchSpeedOffsets"] = json::array();
	for (int i = 0; i < 16; ++i)
	{
		j["pitchSpeedOffsets"].push_back({ pitchSpeedOffsets[i].x, pitchSpeedOffsets[i].y });
	}
}

void ballSprite::LoadFromJson(const json& j)
{
	if (j.contains("strikeZoneSprite"))
	{
		const auto& sz = j["strikeZoneSprite"];
		//strikeZoneSpriteData->texturePath = sz.value("texturePath", L".\\resources\\textures\\strikeZone.png");
		strikeZoneSpriteData->position.x = sz["position"][0].get<float>();
		strikeZoneSpriteData->position.y = sz["position"][1].get<float>();
		strikeZoneSpriteData->size.x = sz["size"][0].get<float>();
		strikeZoneSpriteData->size.y = sz["size"][1].get<float>();
		strikeZoneSpriteData->rotation = sz.value("rotation", 0.0f);
		strikeZoneSpriteData->color.x = sz["color"][0].get<float>();
		strikeZoneSpriteData->color.y = sz["color"][1].get<float>();
		strikeZoneSpriteData->color.z = sz["color"][2].get<float>();
		strikeZoneSpriteData->color.w = sz["color"][3].get<float>();
	}
	if (j.contains("ballDebugSprite"))
	{
		const auto& bd = j["ballDebugSprite"];
		//ballDebugSpriteData->texturePath = bd.value("texturePath", L".\\resources\\textures\\ball.png");
		ballDebugSpriteData->position.x = bd["position"][0].get<float>();
		ballDebugSpriteData->position.y = bd["position"][1].get<float>();
		ballDebugSpriteData->size.x = bd["size"][0].get<float>();
		ballDebugSpriteData->size.y = bd["size"][1].get<float>();
		ballDebugSpriteData->rotation = bd.value("rotation", 0.0f);
		ballDebugSpriteData->color.x = bd["color"][0].get<float>();
		ballDebugSpriteData->color.y = bd["color"][1].get<float>();
		ballDebugSpriteData->color.z = bd["color"][2].get<float>();
		ballDebugSpriteData->color.w = bd["color"][3].get<float>();
	}

	if(j.contains("ballBoardSprite"))
	{
		const auto& bb = j["ballBoardSprite"];
		//ballBoardSpriteData->texturePath = bb.value("texturePath", L".\\resources\\textures\\ballBoard.png");
		ballBoardSpriteData->position.x = bb["position"][0].get<float>();
		ballBoardSpriteData->position.y = bb["position"][1].get<float>();
		ballBoardSpriteData->size.x = bb["size"][0].get<float>();
		ballBoardSpriteData->size.y = bb["size"][1].get<float>();
		ballBoardSpriteData->rotation = bb.value("rotation", 0.0f);
		ballBoardSpriteData->color.x = bb["color"][0].get<float>();
		ballBoardSpriteData->color.y = bb["color"][1].get<float>();
		ballBoardSpriteData->color.z = bb["color"][2].get<float>();
		ballBoardSpriteData->color.w = bb["color"][3].get<float>();
		showBallBoard = bb.value("showBallBoard", true);
	}

	if(j.contains("pitchInfoFont"))
	{
		const auto& pf = j["pitchInfoFont"];
		pitchInfoFontScale = pf.value("fontScale", 28.0f);
		pitchSpeedNormalColor.x = pf["pitchSpeedNormalColor"][0].get<float>();
		pitchSpeedNormalColor.y = pf["pitchSpeedNormalColor"][1].get<float>();
		pitchSpeedNormalColor.z = pf["pitchSpeedNormalColor"][2].get<float>();
		pitchSpeedNormalColor.w = pf["pitchSpeedNormalColor"][3].get<float>();
		pitchSpeedFastColor.x = pf["pitchSpeedFastColor"][0].get<float>();
		pitchSpeedFastColor.y = pf["pitchSpeedFastColor"][1].get<float>();
		pitchSpeedFastColor.z = pf["pitchSpeedFastColor"][2].get<float>();
		pitchSpeedFastColor.w = pf["pitchSpeedFastColor"][3].get<float>();
		pitchSpeedFastThresholdKmh = pf.value("pitchSpeedFastThresholdKmh", 150.0f);
	}

	// 変化量エディタの有効フラグを読み込み
	if (j.contains("useBallBreak"))
	{
		useBallBreak = j["useBallBreak"].get<bool>();
	}

	// LoadFromJson 内の末尾あたりに追加
	if (j.contains("realPitcherBreaks") && j["realPitcherBreaks"].is_array())
	{
		for (const auto& entry : j["realPitcherBreaks"])
		{
			int rp = entry.value("pitcher", -1);
			if (rp <= 0 || rp >= static_cast<int>(realPitcherBreaks.size())) continue;
			if (!entry.contains("breaks") || !entry["breaks"].is_array()) continue;

			const auto& breaksArr = entry["breaks"];
			for (size_t i = 0; i < breaksArr.size() && i < 16; ++i)
			{
				if (breaksArr[i].contains("breakX"))
					realPitcherBreaks[rp].breaks[i].breakX = breaksArr[i]["breakX"].get<float>();
				if (breaksArr[i].contains("breakY"))
					realPitcherBreaks[rp].breaks[i].breakY = breaksArr[i]["breakY"].get<float>();
				if (breaksArr[i].contains("grade"))
					realPitcherBreaks[rp].grades[i] = static_cast<Pitcher::BreakGrade>(breaksArr[i]["grade"].get<int>());
			}
			realPitcherBreaks[rp].initialized = true;
		}
	}

	// 現在選択中の実在投手のデータを強制再反映
	lastAppliedPitcher = Pitcher::RealPitcher::None;
	SyncRealPitcherBreaks();

	// 全14球種の変化量を配列から復元
	if (j.contains("pitchBreaks") && j["pitchBreaks"].is_array())
	{
		const auto& breaksArray = j["pitchBreaks"];

		// クラッシュ防止のため、保存されたデータの数と、配列サイズ(16)の小さい方に合わせてループ
		int size = (std::min)(16, (int)breaksArray.size());
		for (int i = 0; i < size; ++i)
		{
			if (breaksArray[i].contains("breakX")) {
				pitchBreaks[i].breakX = breaksArray[i]["breakX"].get<float>();
			}
			if (breaksArray[i].contains("breakY")) {
				pitchBreaks[i].breakY = breaksArray[i]["breakY"].get<float>();
			}
		}
	}

	
	// 球種名/球速の表示位置オフセットを復元
	if (j.contains("pitchNameOffsets") && j["pitchNameOffsets"].is_array())
	{
		const auto& nameOffsetsArray = j["pitchNameOffsets"];
		int size = (std::min)(16, (int)nameOffsetsArray.size());
		for (int i = 0; i < size; ++i)
		{
			pitchNameOffsets[i].x = nameOffsetsArray[i][0].get<float>();
			pitchNameOffsets[i].y = nameOffsetsArray[i][1].get<float>();
		}
	}

	if(j.contains("pitchSpeedOffsets") && j["pitchSpeedOffsets"].is_array())
	{
		const auto& speedOffsetsArray = j["pitchSpeedOffsets"];
		int size = (std::min)(16, (int)speedOffsetsArray.size());
		for (int i = 0; i < size; ++i)
		{
			pitchSpeedOffsets[i].x = speedOffsetsArray[i][0].get<float>();
			pitchSpeedOffsets[i].y = speedOffsetsArray[i][1].get<float>();
		}
	}
}

//ストライクゾーン境界のゲッター
void ballSprite::GetStrikeZoneScreenBounds(DirectX::XMFLOAT2& outTopLeft, DirectX::XMFLOAT2& outBottomRight) const
{
	if (!strikeZoneSpriteData)
	{
		outTopLeft = { 0.0f, 0.0f };
		outBottomRight = { 0.0f, 0.0f };
		return;
	}
	// strikeZoneGrid[0][0]?[2][2] の3×3グリッドの外接矩形
	const DirectX::XMFLOAT2& worldTL = strikeZoneGrid[0][0];
	const DirectX::XMFLOAT2& worldBR = strikeZoneGrid[2][2];

	DirectX::XMFLOAT2 screenA = WorldToZoneScreen(
		worldTL.x, worldTL.y,
		strikeZoneSpriteData->position, strikeZoneSpriteData->size,
		zone3DCenter, zone3DSize,
		Pitcher::Instance().IsRightPitcher());
	DirectX::XMFLOAT2 screenB = WorldToZoneScreen(
		worldBR.x, worldBR.y,
		strikeZoneSpriteData->position, strikeZoneSpriteData->size,
		zone3DCenter, zone3DSize,
		Pitcher::Instance().IsRightPitcher());

	outTopLeft = { (std::min)(screenA.x, screenB.x), (std::min)(screenA.y, screenB.y) };
	outBottomRight = { (std::max)(screenA.x, screenB.x), (std::max)(screenA.y, screenB.y) };
}