#include "ballSprite.h"
#include "Graphics.h"
#include <shader.h>
#include <imgui.h>
#include "Pitcher.h"
#include "Ball.h"

static DirectX::XMFLOAT2 WorldToZoneScreen(
	float worldX, float worldY,
	const DirectX::XMFLOAT2& zoneScreenPos,  // ゾーンスプライト左上
	const DirectX::XMFLOAT2& zoneScreenSize, // ゾーンスプライトサイズ(px)
	const DirectX::XMFLOAT2& zone3DCenter,   // 3Dゾーン中心(x,y)
	const DirectX::XMFLOAT2& zone3DSize)     // 3Dゾーンサイズ(m)
{
	// 3D座標をゾーン内の正規化座標(0~1)に変換
	float normalX = (worldX - (zone3DCenter.x - zone3DSize.x * 0.5f)) / zone3DSize.x;
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
	const DirectX::XMFLOAT2& zone3DSize)     // 3Dゾーンサイズ(m)
{
	// スクリーン座標を正規化座標(0~1)に変換
	float normalX = (screenX - zoneScreenPos.x) / zoneScreenSize.x;
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
		zone3DSize);
}

void ballSprite::SetAITargetFromWorld(float worldX, float worldY)
{
	aiTargetScreen = WorldToZoneScreen(
		worldX, worldY,
		strikeZoneSpriteData->position,
		strikeZoneSpriteData->size,
		zone3DCenter,
		zone3DSize);
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
		zone3DSize);

	DirectX::XMFLOAT2 screenB = WorldToZoneScreen(
		worldBottomRight.x, worldBottomRight.y,
		strikeZoneSpriteData->position,
		strikeZoneSpriteData->size,
		zone3DCenter,
		zone3DSize);

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

	strikeZoneSpriteData = std::make_unique<Sprite>();
	strikeZoneSpriteData->texturePath = L".\\resources\\textures\\strikeZone.png";
	strikeZoneSpriteData->position = { 1100.0f, 400.0f };
	strikeZoneSpriteData->size = { 200.0f, 300.0f };
	strikeZoneSpriteData->rotation = 0.0f;
	strikeZoneSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	strikeZoneSprite = std::make_unique<sprite>(device, strikeZoneSpriteData->texturePath.c_str());

	ballDebugSpriteData = std::make_unique<Sprite>();
	ballDebugSpriteData->texturePath = L".\\resources\\textures\\ball.png";
	ballDebugSpriteData->position = { 1100.0f, 400.0f };
	ballDebugSpriteData->size = { 20.0f, 20.0f };
	ballDebugSpriteData->rotation = 0.0f;
	ballDebugSpriteData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	ballDebugSprite = std::make_unique<sprite>(device, ballDebugSpriteData->texturePath.c_str());
}

void ballSprite::Uninitialize()
{
	strikeZoneSprite.reset();
	strikeZoneSpriteData.reset();
	ballDebugSprite.reset();
	ballDebugSpriteData.reset();
}

void ballSprite::Update(float elapsedTime)
{
	Pitcher& pitcher = Pitcher::Instance();
	Ball& ball = Ball::Instance();

	// Pitcherの考慮を無視するため、Stateのみで判定するか、ボールが動いているかで判定
	bool nowThrown = (pitcher.GetCurrentState() == Pitcher::State::Throwing);

	if (nowThrown && !prevThrown)
		ballTrail2D.clear();
	prevThrown = nowThrown;

	//投球前に目標地点にスプライトを移動
	
		if (hasAITarget)
		{
			// AIが設定したターゲット位置を使用
			DirectX::XMFLOAT2 targetScreenPos = aiTargetScreen;
			ballDebugSpriteData->position.x = targetScreenPos.x - ballDebugSpriteData->size.x * 0.5f;
			ballDebugSpriteData->position.y = targetScreenPos.y - ballDebugSpriteData->size.y * 0.5f;
		}
		
		return;
	

	const DirectX::XMFLOAT3& wp = ball.GetWorldPosition();

	// 投球中かつ、ピッチャープレートからホームベースの範囲
	if (nowThrown && wp.z >= -0.5f && wp.z <= 18.5f)
	{
		// 1. まず Zの進行度 t を計算 (18.0m から 0.0m への進行度 0.0~1.0)
		float t = 1.0f - (wp.z / 18.0f);
		t = max(0.0f, min(1.0f, t));

		float screenX = 0.0f;
		float screenY = 0.0f;

		if (useBallBreak)
		{
			// === 変化量エディタベースの軌跡計算 ===
			const ballBreak2D& brk = pitchBreaks[currentPitchIndex];
			float ease = t * t * (3.0f - 2.0f * t); // smoothstep

			// ゾーン中心を基準に、エディタの設定値を反映
			float normalX = 0.5f + (brk.breakX / 43.0f) * ease;
			float normalY = 0.5f - (brk.breakY / 60.0f) * ease;

			screenX = strikeZoneSpriteData->position.x + normalX * strikeZoneSpriteData->size.x;
			screenY = strikeZoneSpriteData->position.y + normalY * strikeZoneSpriteData->size.y;
		}
		else
		{
			// === 物理演算(PhysX)ベースの軌跡計算（グリッド内完結版） ===
			physx::PxVec3 velocity = ball.GetLinearVelocity();

			// 最終的にボールが Z=0 に到達したときの予測座標を計算
			float finalX = wp.x;
			float finalY = wp.y;

			if (velocity.z < -0.001f && wp.z > 0.0f)
			{
				float t_remain = -wp.z / velocity.z;
				finalX = wp.x + velocity.x * t_remain;
				finalY = wp.y + velocity.y * t_remain;

				// もし重力による自然落下成分も含める場合はコメント解除
				// finalY += 0.5f * -9.81f * t_remain * t_remain;
			}

			// 最終的な到達位置（Z=0面）をスクリーン上の2D座標に変換
			DirectX::XMFLOAT2 finalScreenPos = WorldToZoneScreen(
				finalX, finalY,
				strikeZoneSpriteData->position,
				strikeZoneSpriteData->size,
				zone3DCenter,
				zone3DSize);

			// 【ここが重要】
			// 2D上の初期位置（ピッチャーがボールを離した瞬間の2D位置。例えばグリッドのど真ん中）
			// から、最終的な2D到達位置に向かって、進行度 t で滑らかに移動させる
			DirectX::XMFLOAT2 startScreenPos = WorldToZoneScreen(
				zone3DCenter.x, zone3DCenter.y, // 開始位置を3Dゾーンの中心（グリッド中央）とする
				strikeZoneSpriteData->position,
				strikeZoneSpriteData->size,
				zone3DCenter,
				zone3DSize);

			// 進行度 t (0.0~1.0) で線形補間（ラープ）する
			screenX = startScreenPos.x + (finalScreenPos.x - startScreenPos.x) * t;
			screenY = startScreenPos.y + (finalScreenPos.y - startScreenPos.y) * t;
		}

		// 2D座標を共通でトレイルとデバッグスプライトに適用
		DirectX::XMFLOAT2 currentScreenPos = { screenX, screenY };

		if (ballTrail2D.empty() ||
			fabsf(ballTrail2D.back().x - currentScreenPos.x) > 0.5f ||
			fabsf(ballTrail2D.back().y - currentScreenPos.y) > 0.5f)
		{
			ballTrail2D.push_back(currentScreenPos);
			if ((int)ballTrail2D.size() > MAX_TRAIL)
				ballTrail2D.pop_front();
		}

		// ボール画像の位置を中心基準で更新
		ballDebugSpriteData->position.x = currentScreenPos.x - ballDebugSpriteData->size.x * 0.5f;
		ballDebugSpriteData->position.y = currentScreenPos.y - ballDebugSpriteData->size.y * 0.5f;
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

	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			const auto& p = strikeZoneGrid[y][x];

			float nx = (p.x + 0.43f * 0.5f) / 0.43f;
			float ny = 1.0f - ((p.y - 0.5f) / 0.6f);

			float screenX =
				strikeZoneSpriteData->position.x +
				nx * strikeZoneSpriteData->size.x;

			float screenY =
				strikeZoneSpriteData->position.y +
				ny * strikeZoneSpriteData->size.y;

			ballDebugSprite->render(
				dc,
				screenX - 5,
				screenY - 5,
				10,
				10,
				1, 0, 0, 1,
				0);
		}
	}

	//// ボールゾーンのグリッド描画
	//for(int y = 0; y < 5; y++)
	//{
	//	for(int x = 0; x < 5; x++)
	//	{

	//		const auto& p = ballZoneGrid[y][x];
	//		float nx = (p.x + 0.43f * 0.5f) / 0.43f;
	//		float ny = 1.0f - ((p.y - 0.5f) / 0.6f);
	//		float screenX =
	//			strikeZoneSpriteData->position.x +
	//			nx * strikeZoneSpriteData->size.x;
	//		float screenY =
	//			strikeZoneSpriteData->position.y +
	//			ny * strikeZoneSpriteData->size.y;

	//		// ストライクゾーン内のグリッドは描画しない
	//		if (x >= 1 && x <= 3 && y >= 1 && y <= 3)
	//			continue;

	//		ballDebugSprite->render(
	//			dc,
	//			screenX - 5,
	//			screenY - 5,
	//			10,
	//			10,
	//			0, 0, 1, 1,
	//			0);
	//	}
	//}

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
	}

	ImGui::Separator();
	ImGui::Text(u8"--- 変化量エディタ ---");

	const char* names[] = {
		u8"ストレート",u8"ツーシーム",u8"カットボール",u8"スライダー",
		u8"カーブ",u8"チェンジアップ",u8"フォーク",u8"シンカー",
		u8"縦スライダー",u8"スプリット",u8"スローカーブ",u8"シュート",
		u8"ナックル",u8"スローボール"
	};

	ImGui::Combo(u8"編集球種", &currentPitchIndex, names, 14);

	ballBreak2D& brk = pitchBreaks[currentPitchIndex];
	ImGui::SliderFloat(u8"横変化 (+ アウト / - イン)", &brk.breakX, -20.0f, 20.0f, "%.1f cm");
	ImGui::SliderFloat(u8"縦変化 (+ 伸び / - 落ち)", &brk.breakY, -25.0f, 10.0f, "%.1f cm");

	ImGui::Checkbox(u8"変化量を反映", &useBallBreak);

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
	// 変化量エディタの有効フラグを保存
	j["useBallBreak"] = useBallBreak;

	// 全14球種の変化量をJSONの配列オブジェクトとしてまとめて保存
	json breaksArray = json::array();
	for (int i = 0; i < 14; ++i)
	{
		breaksArray.push_back({
			{"breakX", pitchBreaks[i].breakX},
			{"breakY", pitchBreaks[i].breakY}
			});
	}
	j["pitchBreaks"] = breaksArray;
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

	// 変化量エディタの有効フラグを読み込み
	if (j.contains("useBallBreak"))
	{
		useBallBreak = j["useBallBreak"].get<bool>();
	}

	// 全14球種の変化量を配列から復元
	if (j.contains("pitchBreaks") && j["pitchBreaks"].is_array())
	{
		const auto& breaksArray = j["pitchBreaks"];

		// クラッシュ防止のため、保存されたデータの数と、配列サイズ(14)の小さい方に合わせてループ
		int size = (std::min)(14, (int)breaksArray.size());
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
		zone3DCenter, zone3DSize);
	DirectX::XMFLOAT2 screenB = WorldToZoneScreen(
		worldBR.x, worldBR.y,
		strikeZoneSpriteData->position, strikeZoneSpriteData->size,
		zone3DCenter, zone3DSize);

	outTopLeft = { (std::min)(screenA.x, screenB.x), (std::min)(screenA.y, screenB.y) };
	outBottomRight = { (std::max)(screenA.x, screenB.x), (std::max)(screenA.y, screenB.y) };
}