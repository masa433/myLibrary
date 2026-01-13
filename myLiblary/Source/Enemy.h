#pragma once
#include"System/ModelRenderer.h"
#include"Character.h"
#include "ProjectileManager.h"

//エネミー
class Enemy :public Character
{
public:
	Enemy() {}
	~Enemy() override{}

	//更新処理
	virtual void Update(float elapsedTime) = 0;
	//継承先で必ず実装させるように純粋仮想関数にする

	//描画処理
	virtual void Render(const RenderContext& rc, ModelRenderer* renderer) = 0;

	//破棄
	void Destroy();

	ProjectileManager& GetProjectileManager() { return projectileManager; }

protected:
	ProjectileManager projectileManager;
};

//純粋仮想関数・・実装がなく、プロトタイプが宣言されているだけの関数
//プロトタイプ宣言に=0をつけるだけで宣言できる
//virtualは仮想関数を表す
//仮想関数・・派生クラスで再定義されるメンバー関数