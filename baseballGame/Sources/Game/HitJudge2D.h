#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include "imgui.h"
#include "ballSprite.h"
#include "batSprite.h"

// ============================================================
//  HitJudge2D  ―  2Dスプライト重なり & タイミング判定
//
//  使い方：
//    1. Player::Update() の中で Update() を毎フレーム呼ぶ
//    2. スイング入力を検知したら TrySwing() を呼ぶ
//    3. 衝突コールバック(onContact)で GetHitResult() を見て
//       PhysX 側の速度に補正を掛ける
// ============================================================
struct HitJudge2DResult
{
    bool  validHit = false;  // 有効な当たり（空振りでない）
    float velocityScale = 1.0f;  // 打球速度への倍率
    float overlapRatio = 0.0f;  // 重なり度 0~1（デバッグ用）
    bool  cursorOverlap = false; // 丸カーソルが重なっていたか
	float cursorOverlapRatio = 0.0f; // 丸カーソルの重なり度 0~1
    bool  purpleBat = false; // 紫バットだったか
	bool isGroundBall = false; // 地面に落ちる打球か（onContact で判定）
	float launchAngle2DDeg = 15.0f; // 2D判定での打球角度（onContact で計算）
	float hitNormalizedY = 0.0f; // バット上端からのヒット位置（0=上端, 1=下端）
	bool isBallZone = false; // ボールがストライクゾーン内に入っていたか（デバッグ用）
};

struct OBB2D
{
    DirectX::XMFLOAT2 center; // 中心座標(px)
    DirectX::XMFLOAT2 halfSize; // 半サイズ(px)
	float rotationDeg; // 回転角度(度)
};

class HitJudge2D
{
public:
    static HitJudge2D& Instance()
    {
        static HitJudge2D inst;
        return inst;
    }

    //設定
    //ヒット有効窓：ボールがストライクゾーン到達の何秒前から何秒後まで有効か
	float hitWindowBeforeSec = 0.5f; //早すぎ判定
	float hitWindowAfterSec = 0.5f;  //遅すぎ判定

    // バット矩形のうち「当たり」と見なす上端オフセット（px）
   // バット画像の上端からこの範囲をヒット帯とする
    float batHitBandHeight = 10.0f;

    // カーソル円の半径（px）
    float cursorRadius = BatSprite::Instance().GetBatCursorSpriteSize().x * 0.5f;

    // 紫バット判定フラグ（外部から set する）
    bool  isPurpleBat = false;
	bool  isBallZone = false; // ボールがストライクゾーン内に入っていたか（外部から set する）

    // カーソル円と重なった時の速度ボーナス
    float cursorOverlapBonus = 0.4f;   // +40%
    // 紫バットのペナルティ
    float purpleBatPenalty = 0.20f;   // -20%

    //ボールゾーンのペナルティ
	float ballZonePenalty = 0.20f; // -20%

    // ballScreenCenter  : 2Dボールスプライトの中心(px)
    // batTopLeft        : バット矩形の左上(px)
    // batSize           : バット矩形のサイズ(px)
    // batRotationDeg    : バットの回転（現在は矩形 AABB で近似）
    // cursorCenter      : 丸カーソルの中心(px)
    // estTimeToZone     : ボールがストライクゾーン到達までの残り秒数
    //                     (0 = 到達、正 = まだ来ていない、負 = 通過済み)
    void Update(const DirectX::XMFLOAT2& ballScreenCenter,
        const DirectX::XMFLOAT2& batCenter,
        const DirectX::XMFLOAT2& batSize,
        float batRotationDeg,
        const DirectX::XMFLOAT2& cursorCenter,
        float estTimeToZone)
    {
        ballCenter_ = ballScreenCenter;
        batTL_ = batCenter;
        batSize_ = batSize;
        batRot_ = batRotationDeg;
        cursorCenter_ = cursorCenter;
        timeToZone_ = estTimeToZone;

        // 重なり計算（AABB + ボール半径で簡易判定）
        overlapResult_ = CalcOverlap();

        swingConsumed_ = false;   // 毎フレームリセット（TrySwing で消費）
    }

    // ---- スイング入力時に呼ぶ ----
    // 戻り値が true なら有効ヒット、false なら空振り
    bool TrySwing(HitJudge2DResult& outResult)
    {
        outResult = {};

        // タイミング判定
        bool timingOK = (timeToZone_ >= -hitWindowAfterSec &&
            timeToZone_ <= hitWindowBeforeSec);

        // 重なり判定
        bool overlapOK = overlapResult_.anyOverlap;

        if (!timingOK || !overlapOK)
        {
            // 空振り
            outResult.validHit = false;
            return false;
        }

        // ---- 有効ヒット ----
        outResult.validHit = true;
        outResult.overlapRatio = overlapResult_.ratio;
        outResult.cursorOverlap = overlapResult_.cursorOverlap;
		outResult.cursorOverlapRatio = overlapResult_.cursorOverlapRatio;
        outResult.purpleBat = isPurpleBat;
		outResult.isGroundBall = overlapResult_.isGroundBall;
		outResult.launchAngle2DDeg = overlapResult_.launchAngle2DDeg;
		outResult.hitNormalizedY = overlapResult_.hitNormalizedY;
		outResult.isBallZone = isBallZone;

        float scale = 1.0f;

        //ボールゾーンなら減速
        if(outResult.isBallZone)
        {
            scale -= ballZonePenalty;
		}
      
        // 優先順位：カーソル重なり > 紫バット
        if (overlapResult_.cursorOverlap)
        {
            
			//どれくらい重なっているかでボーナスを増減する場合は、ここで ratio を使って調整可能
			scale += cursorOverlapBonus * overlapResult_.cursorOverlapRatio;

        }
        else if (isPurpleBat)
        {
            // 白丸が重なっていない かつ 紫バット：ペナルティ
            scale -= purpleBatPenalty;
        }

        outResult.velocityScale = scale;
        lastResult_ = outResult;
        swingConsumed_ = true;
        return true;
    }

    // ---- 最後の有効結果を取得（onContact から参照する用）----
    const HitJudge2DResult& GetLastResult() const { return lastResult_; }

    // デバッグ用：現在フレームの重なり状態
    bool  IsOverlapping()   const { return overlapResult_.anyOverlap; }
    float GetOverlapRatio() const { return overlapResult_.ratio; }
    float GetTimeToZone()   const { return timeToZone_; }

	bool IsCursorOverlapping() const { return overlapResult_.cursorOverlap; }

private:
    HitJudge2D() = default;

    struct OverlapInfo
    {
        bool  anyOverlap = false;
        float ratio = 0.0f;
        bool  cursorOverlap = false;
		float cursorOverlapRatio = 0.0f;
        bool  isGroundBall = false;
        float launchAngle2DDeg = 15.0f;  
        float hitNormalizedY = 0.0f;     
    };

    // ---- AABB + ボール半径による重なり判定 ----
    // 「ボールの下半分とバットの上端付近が重なる」を実装
    OverlapInfo CalcOverlap() const
{
    OverlapInfo info;
    const float br = ballRadius_px_;

    // batTL_ を「中心」として OBB を構築
    OBB2D batOBB;
    batOBB.center = batTL_;   // Update() で中心を渡すようにしたのでそのまま使う
    batOBB.halfSize = { batSize_.x * 0.5f, batSize_.y * 0.5f };
    batOBB.rotationDeg = batRot_;

    // OBBvsCircle で判定
    info.anyOverlap = OBBvsCircle(batOBB, ballCenter_, br);

    if (info.anyOverlap)
    {
        // 重なり度：ボール中心からOBB表面までの距離で簡易計算
        float rad = DirectX::XMConvertToRadians(batOBB.rotationDeg);
        float cosA = cosf(-rad), sinA = sinf(-rad);
        float dx = ballCenter_.x - batOBB.center.x;
        float dy = ballCenter_.y - batOBB.center.y;
        float localX = cosA * dx - sinA * dy;
        float localY = sinA * dx + cosA * dy;
        float clampX = (std::max)(-batOBB.halfSize.x, (std::min)(batOBB.halfSize.x, localX));
        float clampY = (std::max)(-batOBB.halfSize.y, (std::min)(batOBB.halfSize.y, localY));
        float dist = sqrtf((localX - clampX) * (localX - clampX) + (localY - clampY) * (localY - clampY));
        info.ratio = 1.0f - (std::min)(1.0f, dist / br);

        // ボールのローカルY（スクリーン座標系）を用いて上/下を判定
        // localY < 0 : ボール中心がバットの上側（フライ寄り）
        // localY > 0 : ボール中心がバットの下側（ゴロ寄り）
        float maxOffset = batOBB.halfSize.y + br;
        float normalizedY = 0.0f;
        if (maxOffset > 0.0f)
        {
            // normalizedY_internal: -1.0 (上端) ... 0.0 (中心) ... +1.0 (下端)
            normalizedY = (std::max)(-1.0f, (std::min)(1.0f, localY / maxOffset));
        }

        // 外部 API の仕様（HitJudge2DResult::hitNormalizedY）は 0 = 上端, 1 = 下端 としているため、
        // -1..+1 を 0..1 にマッピングして保存する
        info.hitNormalizedY = (normalizedY + 1.0f) * 0.5f; // 0..1

        // 打球仰角：内部計算は normalizedY_internal を用いる（上側ほど大きなフライ角度）
        float internalY = normalizedY; // -1..+1
        float angle;
        if (internalY <= 0.0f)
        {
            // 上側（中心 -> top）
            float t = -internalY; // 0..1
            angle = launchAngleCenter + t * (launchAngleTop - launchAngleCenter);
        }
        else
        {
            // 下側（center -> bottom）
            float t = internalY; // 0..1
            angle = launchAngleCenter + t * (launchAngleBottom - launchAngleCenter);
        }
        info.launchAngle2DDeg = angle;
        info.isGroundBall = (info.hitNormalizedY >= groundBallThreshold);
    }

    // 丸カーソルとの判定（変更なし）
    float dx = ballCenter_.x - cursorCenter_.x;
    float dy = ballCenter_.y - cursorCenter_.y;
    float dist = sqrtf(dx * dx + dy * dy);
    float sumR = br + cursorRadius;
    info.cursorOverlap = (dist < sumR);

    if (info.cursorOverlap)
    {
        // カーソル重なり度：ボール中心からカーソル中心までの距離で簡易計算
        info.cursorOverlapRatio = 1.0f - (dist / sumR);
        info.cursorOverlapRatio = (std::max)(0.0f, info.cursorOverlapRatio);
    }

    return info;
}

    // ---- 内部状態 ----
    DirectX::XMFLOAT2 ballCenter_ = {};
    DirectX::XMFLOAT2 batTL_ = {};
    DirectX::XMFLOAT2 batSize_ = {};
    float              batRot_ = 0.0f;
    DirectX::XMFLOAT2 cursorCenter_ = {};
    float              timeToZone_ = 99.0f;

    float              ballRadius_px_ = ballSprite::Instance().GetBallSpriteSize().x * 0.5f;  // 2Dボール画像の半径(px)

    OverlapInfo        overlapResult_ = {};
    HitJudge2DResult   lastResult_ = {};
    bool               swingConsumed_ = false;

    // 打球角度マッピング（ボール上端に当たった時 → 最大フライ、下端 → ゴロ）
    float launchAngleTop = 150.0f;   // ボール上端に当たった時の仰角(度)
    float launchAngleCenter = 0.0f;   // ボール中心に当たった時
    float launchAngleBottom = -5.0f;  // ボール下端に当たった時(ゴロ)
    float groundBallThreshold = 0.5f;  // hitNormalizedY がこれ以上でゴロ判定

public:
    // ballRadius_px を外から設定できるようにする
    void SetBallRadiusPx(float r) { ballRadius_px_ = r; }

    void SetPendingResult(const HitJudge2DResult& r)
    {
        pendingResult_ = r;
        hasPendingResult_ = true;
    }

    void ClearPendingResult()
    {
        hasPendingResult_ = false;
    }

    bool ConsumePendingResult(HitJudge2DResult& out)
    {
        if (!hasPendingResult_) return false;
        out = pendingResult_;
        hasPendingResult_ = false;
        return true;
    }

    void DrawGUI()
    {
        if (ImGui::CollapsingHeader("HitJudge2D Debug"))
        {
            HitJudge2D& hj = HitJudge2D::Instance();
            ImGui::Text(u8"かぶっているか : %s", hj.IsOverlapping() ? "YES" : "no");
            ImGui::Text(u8"重なり率: %.2f", hj.GetOverlapRatio());
            ImGui::Text(u8"時間: %.3f 秒", hj.GetTimeToZone());
            ImGui::Text(u8"ゴロ判定の割合: %.3f  (- = フライ, + = ゴロ)",
                hj.overlapResult_.hitNormalizedY);
            ImGui::Text(u8"打球角度 2D: %.1f 度", hj.overlapResult_.launchAngle2DDeg);
            ImGui::Text(u8"ゴロ判定: %s", hj.overlapResult_.isGroundBall ? "YES" : "no");
            // DrawGUI
            ImGui::Text(u8"カーソル重なり: %s", hj.IsCursorOverlapping() ? "YES" : "no");

            ImGui::Text(u8"カーソル重なり率: %.2f", hj.overlapResult_.cursorOverlapRatio); 
            ImGui::Separator();

            ImGui::DragFloat(u8"ヒットの有効時間 (前)", &hj.hitWindowBeforeSec, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(u8"ヒットの有効時間 (後)", &hj.hitWindowAfterSec, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(u8"バットヒットバンドの高さ (px)", &hj.batHitBandHeight, 1.0f, 1.0f, 100.0f);
            ImGui::DragFloat(u8"カーソルの半径 (px)", &hj.cursorRadius, 1.0f, 1.0f, 80.0f);
            ImGui::DragFloat(u8"カーソル重なりボーナス", &hj.cursorOverlapBonus, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(u8"パープルバットペナルティ", &hj.purpleBatPenalty, 0.01f, 0.0f, 1.0f);
            ImGui::Checkbox(u8"パープルバットか", &hj.isPurpleBat);
            ImGui::Text(u8"-- 打球角度マッピング --");
            ImGui::DragFloat(u8"角度 上端 (フライ)", &hj.launchAngleTop, 0.5f, 0.0f, 60.0f);
            ImGui::DragFloat(u8"角度 中心", &hj.launchAngleCenter, 0.5f, -30.0f, 60.0f);
            ImGui::DragFloat(u8"角度 下端 (ゴロ)", &hj.launchAngleBottom, 0.5f, -30.0f, 10.0f);
            ImGui::DragFloat(u8"地面の閾値", &hj.groundBallThreshold, 0.01f, 0.0f, 1.0f);
            ImGui::Text(u8"ボールゾーン: %s", hj.isBallZone ? "YES" : "no");
            ImGui::DragFloat(u8"ボールゾーンペナルティ", &hj.ballZonePenalty, 0.01f, 0.0f, 1.0f);
        }
    }

	/// 2D OBB と円の衝突判定（回転矩形と円）
    static bool OBBvsCircle(const OBB2D& obb, DirectX::XMFLOAT2 circleCenter, float radius)
    {
		float rad = DirectX::XMConvertToRadians(obb.rotationDeg);// 角度をラジアンに変換
		float cosA = cosf(-rad), sinA = sinf(-rad);

		//ボールの中心をOBBのローカル座標系に変換
		float dx = circleCenter.x - obb.center.x;
		float dy = circleCenter.y - obb.center.y;
		float localX = cosA * dx - sinA * dy;// 逆回転
		float localY = sinA * dx + cosA * dy;// 逆回転

        // 最近傍点クランプ
        float clampX = (std::max)(-obb.halfSize.x, (std::min)(obb.halfSize.x, localX));
        float clampY = (std::max)(-obb.halfSize.y, (std::min)(obb.halfSize.y, localY));

        float distSq = (localX - clampX) * (localX - clampX)
            + (localY - clampY) * (localY - clampY);
        return distSq <= radius * radius;
    }

private:
    HitJudge2DResult pendingResult_ = {};
    bool             hasPendingResult_ = false;
};
