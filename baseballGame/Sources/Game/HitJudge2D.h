#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include "imgui.h"

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
    bool  purpleBat = false; // 紫バットだったか
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
	float hitWindowAfterSec = 0.10f;  //遅すぎ判定

    // バット矩形のうち「当たり」と見なす上端オフセット（px）
   // バット画像の上端からこの範囲をヒット帯とする
    float batHitBandHeight = 10.0f;

    // カーソル円の半径（px）
    float cursorRadius = 15.0f;

    // 紫バット判定フラグ（外部から set する）
    bool  isPurpleBat = false;

    // カーソル円と重なった時の速度ボーナス
    float cursorOverlapBonus = 0.08f;   // +8%
    // 紫バットのペナルティ
    float purpleBatPenalty = 0.20f;   // -20%

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
        outResult.purpleBat = isPurpleBat;

        float scale = 1.0f;
        if (overlapResult_.cursorOverlap)
            scale += cursorOverlapBonus;
        if (isPurpleBat)
            scale -= purpleBatPenalty;

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

private:
    HitJudge2D() = default;

    struct OverlapInfo
    {
        bool  anyOverlap = false;
        float ratio = 0.0f;
        bool  cursorOverlap = false;
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
        }

        // 丸カーソルとの判定（変更なし）
        float dx = ballCenter_.x - cursorCenter_.x;
        float dy = ballCenter_.y - cursorCenter_.y;
        float distSq = dx * dx + dy * dy;
        float sumR = br + cursorRadius;
        info.cursorOverlap = (distSq < sumR * sumR);

        return info;
    }

    // ---- 内部状態 ----
    DirectX::XMFLOAT2 ballCenter_ = {};
    DirectX::XMFLOAT2 batTL_ = {};
    DirectX::XMFLOAT2 batSize_ = {};
    float              batRot_ = 0.0f;
    DirectX::XMFLOAT2 cursorCenter_ = {};
    float              timeToZone_ = 99.0f;

    float              ballRadius_px_ = 10.0f;  // 2Dボール画像の半径(px)

    OverlapInfo        overlapResult_ = {};
    HitJudge2DResult   lastResult_ = {};
    bool               swingConsumed_ = false;

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
            ImGui::Text("Overlapping: %s", hj.IsOverlapping() ? "YES" : "no");
            ImGui::Text("Overlap ratio: %.2f", hj.GetOverlapRatio());
            ImGui::Text("TimeToZone: %.3f sec", hj.GetTimeToZone());
            ImGui::DragFloat("Hit Window Before(sec)", &hj.hitWindowBeforeSec, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Hit Window After(sec)", &hj.hitWindowAfterSec, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Bat Hit Band Height(px)", &hj.batHitBandHeight, 1.0f, 1.0f, 100.0f);
            ImGui::DragFloat("Cursor Radius(px)", &hj.cursorRadius, 1.0f, 1.0f, 80.0f);
            ImGui::DragFloat("Cursor Overlap Bonus", &hj.cursorOverlapBonus, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Purple Bat Penalty", &hj.purpleBatPenalty, 0.01f, 0.0f, 1.0f);
            ImGui::Checkbox("Is Purple Bat", &hj.isPurpleBat);
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
