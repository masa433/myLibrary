#include "pch.h"
#include "ProjectileManager.h"

ProjectileManager::ProjectileManager() 
{

}

ProjectileManager::~ProjectileManager() 
{
	Clear();
}

//’eŠÛíœ
void ProjectileManager::Remove(Projectile* projectile)
{
	//”jŠüƒŠƒXƒg‚É’Ç‰Á
	removes.insert(projectile);
}

void ProjectileManager::Update(float elapsedTime) 
{
	for (Projectile* projectile : projectiles) 
	{
		projectile->Update(elapsedTime);
	}

	//”jŠüˆ—
	for (Projectile* projectile : removes) 
	{

		//std::vector‚©‚ç—v‘f‚ğíœ‚·‚éê‡‚ÍƒCƒeƒŒ[ƒ^[‚Åíœ‚µ‚È‚¯‚ê‚Î‚È‚ç‚È‚¢
		std::vector<Projectile*>::iterator it = std::find(projectiles.begin(), projectiles.end(), projectile);

		if (it != projectiles.end())
		{
			projectiles.erase(it);
		}

		//’eŠÛ‚Ì”jŠüˆ—
		delete projectile;
	}
	//”jŠüƒŠƒXƒg‚ğƒNƒŠƒA
	removes.clear();
}

void ProjectileManager::Render(const RenderContext& rc, ModelRenderer* renderer) 
{
	for (Projectile* projectile : projectiles) 
	{
		projectile->Render(rc, renderer);
	}
}

void ProjectileManager::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) 
{
	for (Projectile* projectile : projectiles) 
	{
		projectile->RenderDebugPrimitive(rc, renderer);
	}
}

//’eŠÛ“o˜^
void ProjectileManager::Register(Projectile* projectile)
{
	projectiles.emplace_back(projectile);
}

//’eŠÛ‘Síœ
void ProjectileManager::Clear() 
{
	for (Projectile* projectile : projectiles) 
	{
		delete projectile;
	}
	projectiles.clear();
}

