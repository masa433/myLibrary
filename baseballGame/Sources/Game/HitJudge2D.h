#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include "imgui.h"
#include "ballSprite.h"
#include "batSprite.h"
#include "json.hpp"

using json = nlohmann::json;


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
	float exitVelocityMps = 0.0f; // 打球初速（m/s）
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

	float timingJustWindowSec = 0.05f; //ジャスト判定窓（秒）
	float timingSlightWindowSec = 0.01f; //少し早い/遅い判定窓（秒）

    // バット矩形のうち「当たり」と見なす上端オフセット（px）
   // バット画像の上端からこの範囲をヒット帯とする
    float batHitBandHeight = 10.0f;

    // カーソル円の半径（px）
    float cursorRadius = BatSprite::Instance().GetBatCursorSpriteSize().x * 0.5f;

    // 紫バット判定フラグ（外部から set する）
    bool  isPurpleBat = false;
	bool  isBallZone = false; // ボールがストライクゾーン内に入っていたか（外部から set する）
    bool  isInsideCourse = false; // ボールがインコースだったか

	float insideCourseJustWindowScale = 0.5f; // インコースジャスト判定窓の倍率（0.5なら半分の窓でジャスト判定）

    // カーソル円と重なった時の速度ボーナス
    float cursorOverlapBonus = 0.4f;   // +40%
    // 紫バットのペナルティ
    float purpleBatPenalty = 0.20f;   // -20%

    //ボールゾーンのペナルティ
	float ballZonePenalty = 0.20f; // -20%

	float cursorOverlapPenalty = 0.3f; // -20%
	float cursorNeutralPoint = 0.5f; // カーソル重なり度の中立点（0.5以上でボーナス、0.5未満でペナルティ）

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
            
            float t = overlapResult_.cursorOverlapRatio; // 0..1
			
            if (t >= cursorNeutralPoint)
            {
				float bonusT = (t - cursorNeutralPoint) / (1.0f - cursorNeutralPoint); // 0..1
                scale -= cursorOverlapBonus * bonusT;
            }
            else
            {
                float penaltyT = (cursorNeutralPoint - t) / cursorNeutralPoint; // 0..1
				scale -= cursorOverlapPenalty * penaltyT;
            }

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

    bool EvaluateContact(HitJudge2DResult& outResult)
    {
        bool overlapOK = overlapResult_.anyOverlap;

        if (!overlapOK)
        {
            // 空振り
            outResult.validHit = false;
            return false;
        }


        outResult = {};
		outResult.overlapRatio = overlapResult_.ratio;
		outResult.cursorOverlap = overlapResult_.cursorOverlap;
		outResult.cursorOverlapRatio = overlapResult_.cursorOverlapRatio;
		outResult.purpleBat = isPurpleBat;
		outResult.isGroundBall = overlapResult_.isGroundBall;
		outResult.launchAngle2DDeg = overlapResult_.launchAngle2DDeg;
		outResult.hitNormalizedY = overlapResult_.hitNormalizedY;
		outResult.isBallZone = isBallZone;
        outResult.validHit = true;
 
		float scale = CalcTimingScale();

        if(outResult.isBallZone)
        {
            scale -= ballZonePenalty;
		}

        if(overlapResult_.cursorOverlap)
        {
            float t = overlapResult_.cursorOverlapRatio; // 0..1
            if(t >= cursorNeutralPoint)
            {
                float bonusT = (t - cursorNeutralPoint) / (1.0f - cursorNeutralPoint); // 0..1
                scale -= cursorOverlapBonus * bonusT;
            }
            else
            {
                float penaltyT = (cursorNeutralPoint - t) / cursorNeutralPoint; // 0..1
                scale -= cursorOverlapPenalty * penaltyT;
            }
        }
        else if(isPurpleBat)
        {
            scale -= purpleBatPenalty;
		}

        outResult.velocityScale = scale;

        //打球初速を設定する
		float meetQuality = overlapResult_.ratio;//重なり度を打球初速に反映
        if (overlapResult_.cursorOverlap)
        {
			meetQuality = meetQuality * 0.5f + overlapResult_.cursorOverlapRatio * 0.5f; //重なり度とカーソル重なり度の平均を取る
        }
		meetQuality = std::clamp(meetQuality, 0.0f, 1.0f);//0..1にクランプ

        //芯を外したときの最低速度と芯でとらえたときの最高速度
		constexpr float kMinExitVelocityKmh = 80.0f; // 80km/h
		constexpr float kMaxExitVelocityKmh = 160.0f; // 160km/h

		// 重なり度に応じて打球初速を線形補間
		float baseKmh = kMinExitVelocityKmh + (kMaxExitVelocityKmh - kMinExitVelocityKmh) * meetQuality;

		//タイミング・ボールゾーンなどの倍率を反映
        baseKmh *= (std::max)(0.0f, scale);

		outResult.exitVelocityMps = baseKmh / 3.6f; // m/s に変換

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
            info.cursorOverlapRatio = 1.0f - (dist / sumR);
            info.cursorOverlapRatio = (std::max)(0.0f, info.cursorOverlapRatio);
        }
    
        return info;
    }

    float CalcTimingScale() const
    {
        char buffer[64];

        float effectiveJustWindow = isInsideCourse
            ? timingJustWindowSec * insideCourseJustWindowScale
            : timingJustWindowSec;

        if (isInsideCourse)
        {
            // インコースの時のジャスト判定窓（下限を0.0fに縛って、早打ち側も少し厳しくする）
            if (timeToZone_ >= 0.0f && timeToZone_ <= timingJustWindowSec)
            {
                snprintf(buffer, sizeof(buffer), "Hit timing: %.3f sec (Just/Inside)\n", timeToZone_);
                OutputDebugStringA(buffer);
                return 1.1f;
            }
        }
        else
        {
            // アウトコースの時のジャスト判定窓（今まで通り）
            if (timeToZone_ >= -10.0f && timeToZone_ <= effectiveJustWindow)
            {
                snprintf(buffer, sizeof(buffer), "Hit timing: %.3f sec (Just/Outside)\n", timeToZone_);
                OutputDebugStringA(buffer);
                return 1.1f;
            }
        }

		return 1.0f; // デフォルト倍率
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
	bool               swingConsumed_ = false;// 1回のスイングで1回だけ有効ヒットを返すためのフラグ

    // 打球角度マッピング（ボール上端に当たった時 → 最大フライ、下端 → ゴロ）
    float launchAngleTop = 150.0f;   // ボール上端に当たった時の仰角(度)
    float launchAngleCenter = 0.0f;   // ボール中心に当たった時
    float launchAngleBottom = -150.0f;  // ボール下端に当たった時(ゴロ)
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

            //インサイドかどうか
			ImGui::Text(u8"インコース: %s", hj.isInsideCourse ? "YES" : "no");
			//インサイドジャスト判定窓の倍率
			ImGui::DragFloat(u8"インサイドジャスト判定窓の倍率", &hj.insideCourseJustWindowScale, 0.01f, 0.0f, 1.0f);
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
			ImGui::DragFloat(u8"カーソル重なりペナルティ", &hj.cursorOverlapPenalty, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat(u8"カーソル重なり中立点", &hj.cursorNeutralPoint, 0.01f, 0.0f, 1.0f);
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

    void SaveToJson(json& j) const
    {
        j["hitWindowBeforeSec"] = hitWindowBeforeSec;
        j["hitWindowAfterSec"] = hitWindowAfterSec;
        j["batHitBandHeight"] = batHitBandHeight;
        j["cursorRadius"] = cursorRadius;
        j["cursorOverlapBonus"] = cursorOverlapBonus;
        j["purpleBatPenalty"] = purpleBatPenalty;
        j["ballZonePenalty"] = ballZonePenalty;
        j["launchAngleTop"] = launchAngleTop;
        j["launchAngleCenter"] = launchAngleCenter;
        j["launchAngleBottom"] = launchAngleBottom;
        j["groundBallThreshold"] = groundBallThreshold;
		j["cursorOverlapPenalty"] = cursorOverlapPenalty;
		j["cursorNeutralPoint"] = cursorNeutralPoint;
	}


    void LoadFromJson(const json& j)
    {
        hitWindowBeforeSec = j.value("hitWindowBeforeSec", hitWindowBeforeSec);
        hitWindowAfterSec = j.value("hitWindowAfterSec", hitWindowAfterSec);
        batHitBandHeight = j.value("batHitBandHeight", batHitBandHeight);
        cursorRadius = j.value("cursorRadius", cursorRadius);
        cursorOverlapBonus = j.value("cursorOverlapBonus", cursorOverlapBonus);
        purpleBatPenalty = j.value("purpleBatPenalty", purpleBatPenalty);
        ballZonePenalty = j.value("ballZonePenalty", ballZonePenalty);
        launchAngleTop = j.value("launchAngleTop", launchAngleTop);
        launchAngleCenter = j.value("launchAngleCenter", launchAngleCenter);
        launchAngleBottom = j.value("launchAngleBottom", launchAngleBottom);
        groundBallThreshold = j.value("groundBallThreshold", groundBallThreshold);
		cursorNeutralPoint = j.value("cursorNeutralPoint", cursorNeutralPoint);
		cursorOverlapPenalty = j.value("cursorOverlapPenalty", cursorOverlapPenalty);
	}

private:
    HitJudge2DResult pendingResult_ = {};
    bool             hasPendingResult_ = false;
};
