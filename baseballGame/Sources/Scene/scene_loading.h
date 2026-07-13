#pragma once
#include "scene.h"
#include <thread>

class scene_loading : public scene
{
public:
	scene_loading(scene* nextScene) : nextScene(nextScene) {}
	~scene_loading() override {}

	void initialize() override;
	void uninitialize() override;
	void update(float elapsed_time) override;
	void render(float elapsed_time) override;
	void DrawGUI() override;

private:
	//ローディングスレッド
	static void LoadingThread(scene_loading* scene);

private:

	std::unique_ptr<scene> nextScene;
	std::unique_ptr<std::thread> thread;
};