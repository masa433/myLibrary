#include "../pch.h"
#include"EnemyManager.h"
#include"Collision.h"

void EnemyManager::Remove(Enemy* enemy)
{
	//破棄リストに追加
	removes.insert(enemy);
}

//更新処理
void EnemyManager::Update(float elapsedTime) 
{
	for (Enemy* enemy : enemies) 
	{
		enemy->Update(elapsedTime);
		//範囲for文で管理しているエネミーの更新処理を一括実装
	}

	//破棄処理
	for (Enemy* enemy : removes)
	{

		//std::vectorから要素を削除する場合はイテレーターで削除しなければならない
		std::vector<Enemy*>::iterator it = std::find(enemies.begin(), enemies.end(), enemy);
		
		

		if (it != enemies.end())
		{
			enemies.erase(it);
		}

		//弾丸の破棄処理
		delete enemy;
	}
	//破棄リストをクリア
	removes.clear();

	//敵同士の衝突処理
	CollisionEnemyVsEnemies();
}

//描画処理
void EnemyManager::Render(const RenderContext& rc, ModelRenderer* renderer) 
{
	for (Enemy* enemy : enemies) 
	{
		enemy->Render(rc, renderer);
	}
	
}

void EnemyManager::Register(Enemy* enemy)
{
	enemies.emplace_back(enemy);
}

// エネミー全削除
void EnemyManager::Clear()
{
	// 各エネミーオブジェクトを削除
	for (Enemy* enemy : enemies)
	{
		delete enemy;
	}
	// ベクター内のポインタをクリア
	enemies.clear();
}

//デバッグプリミティブ描画
void EnemyManager::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) 
{
	for (Enemy* enemy : enemies)
	{
		enemy->RenderDebugPrimitive(rc,renderer);
	}
}

//エネミー同士の衝突処理
void EnemyManager::CollisionEnemyVsEnemies() 
{
	size_t enemyCount = enemies.size();
	for (int i = 0; i < enemyCount; i++)
	{
		Enemy* enemyA = enemies.at(i);

		for (int j = i+1; j < enemyCount; j++) {
			Enemy* enemyB = enemies.at(j);
		

		// 衝突処理
		DirectX::XMFLOAT3 outPosition;
		//if (Collision::IntersectSphereVsSphere(

		//	enemyA->GetPosition(),            
		//	enemyA->GetRadius(),              
		//	enemyB->GetPosition(),            
		//	enemyB->GetRadius(),              
		//	outPosition))
		//{
		//	// 押し出し後の位置設定
		//	enemyB->SetPosition(outPosition);
		//}

			if (Collision::IntersectCylinderVsCylinder(
				enemyA->GetPosition(),
				enemyA->GetRadius(),
				enemyA->GetHeight(),
				enemyB->GetPosition(),
				enemyB->GetRadius(),
				enemyB->GetHeight(),
				outPosition)) 
			{
				enemyB->SetPosition(outPosition);
			}

		}
	}
		
}
