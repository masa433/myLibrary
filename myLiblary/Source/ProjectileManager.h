#pragma once

#include "Projectile.h"

class ProjectileManager 
{
public:
	ProjectileManager();
	~ProjectileManager();

	void Update(float elapsedTime);

	void Render(const RenderContext& rc, ModelRenderer* renderer);

	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	//’eŠÛ“o˜^
	void Register(Projectile* projectile);

	//’eŠÛ‘Síœ
	void Clear();

	//’eŠÛ”æ“¾
	int GetProjectileCount() const { return static_cast<int>(projectiles.size()); }

	//’eŠÛæ“¾
	Projectile* GetProjectile(int index) { return projectiles.at(index); }

	void Remove(Projectile* projectile);

private:
	std::vector<Projectile*> projectiles;

	std::set<Projectile*> removes;

};