#pragma once
#include "scene.h"
#include <memory>

class sceneManager
{
private:
	sceneManager() {}
	~sceneManager() {}

public:
	//インスタンス
	static sceneManager& Instance()
	{
		static sceneManager instance;
		return instance;
	}

	void Update(float elapsedTime);
	void Render();

	void DrawGUI();

	//シーンクリア
	void Clear();

	//シーン切り替え
	void ChangeScene(scene* scene);

private:
	std::unique_ptr<scene> currentScene;
	std::unique_ptr<scene> nextScene;
};