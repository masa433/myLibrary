#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>
#include "gltf_model.h"
#include "game_object.h"
#include "RenderContext.h"

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
		void Render(RenderContext& rc);
		void DrawGUI();

		void AttachBallToHand(float elapsedTime);

		void UpdateAnimation(float elapsedTime);

public:
		const DirectX::XMFLOAT3& GetBallPosition() const { return ballWorldPosition; }
		const DirectX::XMFLOAT3& GetBallScale() const { return ballWorldScale; }

		const DirectX::XMFLOAT3& GetBallVelocity() const { return ballVelocity; }
		void SetBallVelocity(const DirectX::XMFLOAT3& velocity) { ballVelocity = velocity; }
		const float GetBallDebugRadius() const { return ballDebugRadius; }

private:
	// モデル関連
		std::unique_ptr<gltf_model> pitcher;
		std::vector<gltf_model::node> animated_nodes;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;
		// アニメーション関連
		float animation_time = 0.0f;
		int current_animation_index = 0;
		bool animation_playing = true;

		//ボール関連
		std::unique_ptr<gltf_model> ball;
		DirectX::XMFLOAT4X4 ballTransform = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
		DirectX::XMFLOAT3 ballPosition = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT3 ballScale = { 1.0f,1.0f,1.0f };
		DirectX::XMFLOAT3 ballAngle = { 0.0f,0.0f,0.0f };

		// ボール専用のトランスフォーム情報
		DirectX::XMFLOAT3 ballWorldPosition = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 ballWorldAngle = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 ballWorldScale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4X4 ballWorldTransform = {
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			0,0,0,1
		};

		DirectX::XMFLOAT3 ballVelocity = { 0.0f, 0.0f, 0.0f }; // ボールの速度
		DirectX::XMFLOAT3 ballStartPosition = { 0.0f, 0.0f, 0.0f }; // 投球開始位置（追加）
		float throwTiming = 0.4f; // ボールを離すタイミング（アニメーション時間の比率）
		float gravity = -9.8f; // 重力加速度
		bool isBallThrown = false; // ボールが投げられたか
		// ボール投球制御
		float ballSpeedKmh = 150.0f; // 投球速度（km/h） - デバッグ可能
		float launchAngleDegrees = -2.0f; // 発射角度（度）
		DirectX::XMFLOAT3 rotationSpeed = { 0.0f, 0.0f, 0.0f }; // 回転速度（度/秒）
		DirectX::XMFLOAT3 throwDirection = { -0.02f, -0.2f, 1.0f }; // 投球方向

		// 変化球パラメータ
		float horizontalBreak = 0.0f; // 横方向の変化量（正:右、負:左）
		float verticalBreak = 0.0f;   // 縦方向の変化量（正:上、負:下）
		float breakStartDistance = 10.0f; // 変化が始まる距離

		float ballDebugRadius = 0.15f; // デフォルトのスケール倍率
		float reducedRadius = 0.0f;
};
