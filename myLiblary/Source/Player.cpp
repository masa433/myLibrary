#include "../pch.h"
#include "Player.h"
#include "Camera.h"
#include "System/Input.h"
#include "EnemyManager.h"
#include "Collision.h"
#include "ProjectileStraight.h"
#include "ProjectileHoming.h"
#include "Object.h"
#include "SceneManager.h"
#include "SceneTitle.h"


//初期化
void Player::Initialize() 
{
	model = new Model("Data/Model/player/Bot.mdl");

	//モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.05f;
	radius = 4.0f;
	height = 9.0f;
	angle.y = DirectX::XMConvertToRadians(90.0f);
	

	//AudioManager::Instance().GetSound(SoundList::GameBGM)->Play(false, 1.0f);

	hitEffect = new Effect("Data/Effect/Hit.efk");
	health = 5;
	position =DirectX::XMFLOAT3(-73.0f, 0.0f, 6.0f);
	velocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	isDead = false;

	SetBattingIdleState();
}

//終了化
void Player::Finalize()
{
	// モデルが存在すれば解放
	if (model)
	{
		delete model;
		model = nullptr;
	}

	// 音声やエフェクトの解放
	/*if (hitSE)
	{
		delete hitSE;
		hitSE = nullptr;
	}*/

	if (hitEffect)
	{
		delete hitEffect;
		hitEffect = nullptr;
	}
}


//更新処理
void Player::Update(float elapsedTime) 
{

	if (isDead) return;  // 死亡している場合は更新を行わない

	
	switch (state)
	{
	case State::BatIdle:
		UpdateBattingIdleState(elapsedTime);
		break;
	case State::BatSwing:
		UpdateSwingState(elapsedTime);
		break;
	default:
		break;
	}


	//移動入力処理
	InputMove(elapsedTime);

	
	
	//ジャンプ入力処理
	InputJump();

	//速力処理更新
	UpdateVelocity(elapsedTime);

	//プレイヤーと敵との衝突処理
	CollisionPlayerVsEnemies();

	//オブジェクト行列を更新
	UpdateTransform();

	//モデル行列更新
	model->UpdateTransform();

	model->UpdateAnimation(elapsedTime);

	//弾丸更新処理
	projectileManager.Update(elapsedTime);

	//弾丸入力処理
	InputProjectile();

	//弾丸と敵の衝突処理
	CollisionProjectilesVsEnemies();

	CollisionEnemiesProjectilesVsPlayer();

	if (isDead)  // 死亡後の処理
	{
		deathTimer -= elapsedTime;
		if (deathTimer <= 0.0f)
		{
			
		}
	}

}

void Player::OnLanding() 
{ 
	 jumpCount = 0; 
}


//移動入力処理
void Player::InputMove(float elapsedTime) 
{
	//進行ベクトル取得
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	//移動処理
	Move(elapsedTime, moveVec.x, moveVec.z, moveSpeed);

	//旋回処理
	Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
}


// ジャンプ入力処理
void Player::InputJump()
{
    // ボタン入力でジャンプ (ジャンプ回数制限付き)
    GamePad& gamePad = Input::Instance().GetGamePad();
    if (gamePad.GetButtonDown() & GamePad::BTN_A)
    {
        // ジャンプが可能か判定
        if (jumpCount < jumpLimit)
        {
            //Jump(jumpSpeed);
            jumpCount++; // ジャンプ回数を更新
            
        }
		
    }
}



//描画処理
void Player::Render(const RenderContext& rc,ModelRenderer*renderer) 
{
	if (model == nullptr) return;  // モデルがnullptrの場合は描画しない

	renderer->Render(rc, transform, model, ShaderId::Lambert);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

//デバッグプリミティブ描画
void Player::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) 
{
	//基底クラスの関数呼び出し
	Character::RenderDebugPrimitive(rc, renderer);

	//弾丸デバッグプリミティブ描画
	projectileManager.RenderDebugPrimitive(rc, renderer);
}

//デバッグ用GUI描画
void Player::DrawDebugGUI() 
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300),ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Player", nullptr, ImGuiWindowFlags_None)) 
	{

		//トランスフォーム
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) 
		{
			//位置
			ImGui::InputFloat3("Position", &position.x);
			//回転
			DirectX::XMFLOAT3 a;
			a.x = DirectX::XMConvertToDegrees(angle.x);
			a.y = DirectX::XMConvertToDegrees(angle.y);
			a.z = DirectX::XMConvertToDegrees(angle.z);
			ImGui::InputFloat3("Angle", &a.x);
			angle.x = DirectX::XMConvertToRadians(a.x);
			angle.y = DirectX::XMConvertToRadians(a.y);
			angle.z = DirectX::XMConvertToRadians(a.z);
			//スケール
			ImGui::InputFloat3("Scale", &scale.x);
		}
	}
	ImGui::End();
}

DirectX::XMFLOAT3 Player::GetMoveVec() const
{
	// 入力情報を取得
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisLX();  // 左スティックのX軸
	float ay = gamePad.GetAxisLY();  // 左スティックのY軸

	// カメラ方向とスティックの入力値によって進行方向を計算する
	Camera& camera = Camera::Instance();
	const DirectX::XMFLOAT3& cameraRight = camera.GetRight();
	const DirectX::XMFLOAT3& cameraFront = camera.GetFront();

	// 移動ベクトルはXZ平面に水平なベクトルになるようにする

	// カメラ右方向ベクトルをXZ単位ベクトルに変換
	float cameraRightX = cameraRight.x;
	float cameraRightZ = cameraRight.z;
	float cameraRightLength = sqrtf(cameraRightX * cameraRightX + cameraRightZ * cameraRightZ);
	if (cameraRightLength > 0.0f)
	{
		// 単位ベクトル化
		cameraRightX /= cameraRightLength;
		cameraRightZ /= cameraRightLength;
	}

	// カメラ前方向ベクトルをXZ単位ベクトルに変換
	float cameraFrontX = cameraFront.x;
	float cameraFrontZ = cameraFront.z;
	float cameraFrontLength = sqrtf(cameraFrontX * cameraFrontX + cameraFrontZ * cameraFrontZ);
	if (cameraFrontLength > 0.0f)
	{
		// 単位ベクトル化
		cameraFrontX /= cameraFrontLength;
		cameraFrontZ /= cameraFrontLength;
	}

	// スティックの水平入力値をカメラ右方向に反映し
	// スティックの垂直入力値をカメラ前方向に反映し
	// 進行ベクトルを計算する
	DirectX::XMFLOAT3 vec;
	// スティックの水平入力 (ax) はカメラの右方向の反対に、垂直入力 (ay) はカメラの前方向に反映させる
	vec.x = (cameraRightX * ax) + (cameraFrontX * ay); // Aキーで左に動かすために右方向を反転
	vec.z = (cameraRightZ * ax) + (cameraFrontZ * ay); // 同様にZ方向も反転
	// Y軸方向には移動しない
	vec.y = 0.0f;

	return vec;


}

void Player::CollisionPlayerVsEnemies()
{
	EnemyManager& enemyManager = EnemyManager::Instance();

	// 全ての敵と総当たりで衝突処理
	int enemyCount = enemyManager.GetEnemyCount();
	for (int i = 0; i < enemyCount; ++i)
	{
		Enemy* enemy = enemyManager.GetEnemy(i);

		// 衝突処理
		DirectX::XMFLOAT3 outPosition;
		

		if (Collision::IntersectCylinderVsCylinder(
			position,
			radius,
			height,
			enemy->GetPosition(),
			enemy->GetRadius(),
			enemy->GetHeight(),
			outPosition)) 
		{
			

			// 敵の真上付近にプレイヤーが当たったかを判定する処理

			// プレイヤーの位置ベクトルを作成
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&position);

			// 敵の位置ベクトルを作成
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&enemy->GetPosition());

			// プレイヤーと敵の間のベクトルを計算（P - E）
			DirectX::XMVECTOR V = DirectX::XMVectorSubtract(P, E);

			// 上記のベクトルを正規化（方向ベクトルを求める）
			DirectX::XMVECTOR N = DirectX::XMVector3Normalize(V);

			// 正規化したベクトルを3次元の構造体に格納
			DirectX::XMFLOAT3 normal;
			DirectX::XMStoreFloat3(&normal, N);

			// 正規化ベクトルのy成分が0.8より大きい場合（敵の真上から当たったとみなす）
			if (normal.y > 0.8f)
			{
				// プレイヤーをジャンプさせる（ジャンプ速度を半分にして反動を抑える）
				Jump(jumpSpeed * 0.5f);
				enemy->ApplyDamage(1,0.5f);
			}
			else
			{
				// 敵を画面外に移動させる
				enemy->SetPosition(outPosition);
			}


		}
	}
}

void Player::InputProjectile() 
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	//直進弾丸発射
	if (gamePad.GetButtonDown() & GamePad::BTN_X) 
	{
		//前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);
	

		//発射位置(プレイヤーの腰あたり)
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;

		//発射
		ProjectileStraight* projectile = new ProjectileStraight(&projectileManager);
		projectile->Launch(dir,pos);
		//projectileManager.Register(projectile);
		//弾丸クラスのコンストラクタで呼びだすから削除
	}

	if (gamePad.GetButtonDown() & GamePad::BTN_Y)
	{
		//前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);

		//発射位置(プレイヤーの腰あたり)
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;

		//ターゲット(デフォルトではプレイヤーの前方)
		DirectX::XMFLOAT3 target;
		target.x = pos.x + dir.x * 1000.0f;
		target.y = pos.y + dir.y * 1000.0f;
		target.z = pos.z + dir.z * 1000.0f;

		//一番近くの敵をターゲットにする
		float dist = FLT_MAX;
		EnemyManager& enemyManager = EnemyManager::Instance();
		int enemyCount = enemyManager.GetEnemyCount();
		for (int i = 0; i < enemyCount; ++i) 
		{
			//敵との距離判定
			Enemy* enemy = EnemyManager::Instance().GetEnemy(i);
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&position);
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&enemy->GetPosition());
			DirectX::XMVECTOR V = DirectX::XMVectorSubtract(E, P);
			DirectX::XMVECTOR D = DirectX::XMVector3LengthSq(V);

			float d;
			DirectX::XMStoreFloat(&d, D);
			if (d < dist) 
			{
				dist = d;
				target = enemy->GetPosition();
				target.y += enemy->GetHeight() * 0.5f;
			}
		}

		//発射
		ProjectileHoming* projectile = new ProjectileHoming(&projectileManager);
		projectile->Launch(dir, pos, target);
	}
}

void Player::CollisionProjectilesVsEnemies()
{
	EnemyManager& enemyManager = EnemyManager::Instance();

	// 全ての弾丸とすべての敵を総当たりで衝突処理
	int projectileCount = projectileManager.GetProjectileCount();
	int enemyCount = enemyManager.GetEnemyCount();

	for (int i = 0; i < projectileCount; ++i)
	{
		Projectile* projectile = projectileManager.GetProjectile(i);

		for (int j = 0; j < enemyCount; ++j)
		{
			Enemy* enemy = enemyManager.GetEnemy(j);

			DirectX::XMFLOAT3 outPosition;
			if (Collision::IntersectSphereVsCylinder(
				projectile->GetPosition(),
				projectile->GetRadius(),
				enemy->GetPosition(),
				enemy->GetRadius(),
				enemy->GetHeight(),
				outPosition))
			{
				// ダメージを与える
				if (enemy->ApplyDamage(1, 0.5f)) {
					// ダメージを与えて敵が死亡した場合の処理
					DirectX::XMFLOAT3 impulse;
					const float power = 10.0f;

					// 敵の位置を取得
					const DirectX::XMFLOAT3& e = enemy->GetPosition();
					// 弾の位置を取得
					const DirectX::XMFLOAT3& p = projectile->GetPosition();

					// x-z平面での距離を計算
					float vx = e.x - p.x;
					float vz = e.z - p.z;
					float lengthXZ = sqrtf(vx * vx + vz * vz);
					vx /= lengthXZ;
					vz /= lengthXZ;

					impulse.x = vx * power;
					impulse.y = power * 0.5f;
					impulse.z = vz * power;

					enemy->AddImpulse(impulse);

					// ヒットエフェクト再生
					DirectX::XMFLOAT3 ePosition = enemy->GetPosition();
					ePosition.y += enemy->GetHeight() * 0.5f;

					// スケールを大きくする設定を追加
					DirectX::XMFLOAT3 scale = { 5.0f, 5.0f, 5.0f }; // エフェクトのスケールを2倍に設定
					Effekseer::Handle effectHandle = hitEffect->Play(ePosition);
					hitEffect->SetScale(effectHandle, scale);


					// ヒット効果音再生
					//hitSE->Play(false,0.5f);

					// 弾丸破棄
					projectile->Destroy();
				}

			}
		}
	}
}

void Player::ApplyDamage(int damage, float invincibleTime)
{
	if (isDead) return;  // すでに死亡している場合はダメージを受けない

	health -= damage;  // 体力を減らす

	if (health <= 0)  // 死亡判定
	{
		Die();  // 死亡処理を呼び出す
	}
}


void Player::Die()
{
	if (health <= 0 && !isDead)  // 体力が0以下になり、まだ死亡していない場合
	{
		isDead = true;  // 死亡フラグを立てる

		// 死亡タイマーを開始
		deathTimer = 1.0f;  // 死亡後1秒のタイマーをセット

		// プレイヤーの表示を停止（モデル解放など）
		if (model)  // modelがnullptrでない場合に解放する
		{
			delete model;  // モデルを解放
			model = nullptr;  // モデルポインタをnullptrに設定
		}

		// 死亡エフェクトやサウンドを再生
		hitEffect->Play(position);  // 死亡エフェクトを表示
		//hitSE->Play(false,0.5f);  // 死亡効果音を再生
	}
	
}



void Player::CollisionEnemiesProjectilesVsPlayer()
{
	EnemyManager& enemyManager = EnemyManager::Instance();

	int enemyCount = enemyManager.GetEnemyCount();

	for (int i = 0; i < enemyCount; ++i)
	{
		Enemy* enemy = enemyManager.GetEnemy(i);

		// 各敵のProjectileManagerを取得
		ProjectileManager& projectileManager = enemy->GetProjectileManager();
		int projectileCount = projectileManager.GetProjectileCount();

		for (int j = 0; j < projectileCount; ++j)
		{
			Projectile* projectile = projectileManager.GetProjectile(j);

			// プレイヤーとの衝突判定
			DirectX::XMFLOAT3 outPosition;
			if (Collision::IntersectSphereVsCylinder(
				projectile->GetPosition(),
				projectile->GetRadius(),
				position,  // プレイヤーの位置
				radius,    // プレイヤーの半径
				height,    // プレイヤーの高さ
				outPosition))
			{
				// プレイヤーにダメージを与える
				ApplyDamage(1, 0.5f);  // 1ダメージ、無敵時間0.5秒

				// 吹き飛ばす処理（インパルス）
				DirectX::XMFLOAT3 impulse;
				const float power = 10.0f;
				const DirectX::XMFLOAT3& playerPos = position;
				const DirectX::XMFLOAT3& p = projectile->GetPosition();

				float vx = playerPos.x - p.x;  // x方向の差分
				float vz = playerPos.z - p.z;  // z方向の差分
				float lengthXZ = sqrtf(vx * vx + vz * vz);  // 距離計算
				vx /= lengthXZ;  // 正規化
				vz /= lengthXZ;  // 正規化

				impulse.x = vx * power;
				impulse.y = power * 0.5f;  // y方向のインパルス
				impulse.z = vz * power;

				AddImpulse(impulse);  // 吹き飛ばす

				// ヒットエフェクトを表示
				{
					DirectX::XMFLOAT3 p = position;
					p.y += height * 0.5f;
					DirectX::XMFLOAT3 scale = { 5.0f, 5.0f, 5.0f }; // エフェクトのスケールを2倍に設定
					Effekseer::Handle effectHandle = hitEffect->Play(p);
					hitEffect->SetScale(effectHandle, scale);
				}

				// ヒット効果音を再生
				{
					//hitSE->Play(false,0.5f);  // 効果音の再生
				}

				// 弾丸を破棄
				projectile->Destroy();
			}
		}
	}
}

//アニメーション関連
void Player::SetBattingIdleState() 
{
	state = State::BatIdle;
	model->PlayAnimation(BattingIdle, true);
}

void Player::UpdateBattingIdleState(float elapsedTime) 
{
	// ボタン入力でジャンプ (ジャンプ回数制限付き)
	GamePad& gamePad = Input::Instance().GetGamePad();
	if (gamePad.GetButtonDown() & GamePad::BTN_A)
	{
		SetSwingState();

	}
}

void Player::SetSwingState() 
{
	state = State::BatSwing;
	model->PlayAnimation(Swing, false);
}

void Player::UpdateSwingState(float elapsedTime) 
{
	//アニメーション終了したら待機状態へ
	if (!model->IsPlayAnimation()) 
	{
		SetBattingIdleState();
	}
}




