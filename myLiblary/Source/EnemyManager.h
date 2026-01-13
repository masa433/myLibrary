#pragma once

#include"Enemy.h"

//エネミーマネージャー(すべての敵を管理する)
class EnemyManager 
{
private:
	EnemyManager() {}
	~EnemyManager() {}

public:
	//唯一のインスタンス取得
	static EnemyManager& Instance()
		//エネミーマネージャーはゲームで唯一のものとして扱いたいので
		// シングルトンにする
	{
		static EnemyManager instance;
		return instance;
	}

	//更新処理
	void Update(float elapsedTime);

	//描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	//エネミー登録
	void Register(Enemy* enemy);

	//エネミー全削除
	void Clear();

	//デバッグプリミティブ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	//エネミー数取得
	int GetEnemyCount() const { return static_cast<int>(enemies.size());}

	//エネミー取得
	Enemy* GetEnemy(int index) { return enemies.at(index); }

	//エネミー削除
	void Remove(Enemy* enemy);

private:
	//エネミー同士の衝突処理
	void CollisionEnemyVsEnemies();

	std::vector<Enemy*>  enemies;
	//複数のエネミーを管理するため、エネミーのポインタをstd::vectorで管理する

	std::set<Enemy*> removes;
};

//std::vectorとは、C++が標準で搭載しているSTLの機能の一つ
//可変長の配列を表現するためのもの