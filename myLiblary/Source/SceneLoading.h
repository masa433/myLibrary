#pragma once
 
#include "System/Sprite.h"
#include "Scene.h"
#include <thread>

class SceneLoading : public Scene 
{
public:
	SceneLoading(Scene*nextScene):nextScene(nextScene){}
	~SceneLoading()override{}

	void Initialize()override;

	void Finalize()override;

	void Update(float elapsedTime)override;

	void Render()override;

	void DrawGUI()override;


private:
	//ローディングスレッド
	static void LoadingThread(SceneLoading* scene);

private:
	Sprite* sprite = nullptr;
	Sprite* dot = nullptr;
	float angle = 0.0f;
	float time = 0.0f;
	float bounceTimer = 0.0f;
	int currentDotIndex = 0;
	Scene* nextScene = nullptr;
	std::thread* thread = nullptr;
};