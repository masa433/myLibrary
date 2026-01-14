#include "pch.h"
#include "SceneManager.h"


void SceneManager::Update(float elapsedTime)
{
	if (nextScene != nullptr)
	{
		// 古いシーンを終了
		Clear();

		// 新しいシーンを設定
		currentScene = nextScene;  // 現在のシーンに次のシーンを設定
		nextScene = nullptr;       // 次のシーンポインタをリセット

		// シーン初期化処理
		if (!currentScene->IsReady()) {
			currentScene->Initialize();
		}
		
	}


	if (currentScene != nullptr)
	{
		currentScene->Update(elapsedTime);
	}
}

void SceneManager::Render()
{
	if (currentScene != nullptr) 
	{
		currentScene->Render();
	}
}

void SceneManager::DrawGUI() 
{
	if (currentScene != nullptr) 
	{
		currentScene->DrawGUI();
	}
}

void SceneManager::Clear() 
{
	if (currentScene != nullptr) 
	{
		currentScene->Finalize();
		delete currentScene;
		currentScene = nullptr;
	}
}

void SceneManager::ChangeScene(Scene*scene) 
{
	//新しいシーンの設定
	nextScene = scene;
}