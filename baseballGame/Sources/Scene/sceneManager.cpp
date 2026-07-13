#include "sceneManager.h"

void sceneManager::Update(float elapsedTime)
{
	if (nextScene != nullptr)
	{
		//古いシーンを終了
		Clear();

		//新しいシーンを設定
		currentScene = std::move(nextScene);
		nextScene = nullptr;

		//シーン初期化処理
		if(!currentScene->IsReady())
		{
			currentScene->initialize();
		}
	}

	//シーン更新処理
	if(currentScene != nullptr)
	{
		currentScene->update(elapsedTime);
	}
}

void sceneManager::Render()
{
	if(currentScene != nullptr)
	{
		currentScene->render(0.0f);
	}
}

void sceneManager::DrawGUI()
{
	if (currentScene != nullptr)
	{
		currentScene->DrawGUI();
	}
}

//シーンクリア
void sceneManager::Clear()
{
	if(currentScene != nullptr)
	{
		currentScene->uninitialize();
		currentScene = nullptr;
	}
}

//シーン切り替え
void sceneManager::ChangeScene(scene* scene)
{
	nextScene.reset(scene);
}	