#pragma once
#include "scene.h"
#include "sceneManager.h"
#include "scene_game.h"
#include "scene_title.h"
#include "batterSelectScene.h"
#include "scene_loading.h"

static void ChangeSceneTitleToBatterSelect()
{
	sceneManager::Instance().ChangeScene(new scene_loading(new batterSelectScene()));
}

static void ChangeSceneBatterSelectToGame()
{
	sceneManager::Instance().ChangeScene(new scene_loading(new scene_game()));
}

static void ChangeSceneGameToTitle()
{
	sceneManager::Instance().ChangeScene(new scene_loading(new scene_title()));
}

static void ChangeSceneGameToBatterSelect()
{
	sceneManager::Instance().ChangeScene(new scene_loading(new batterSelectScene()));
}

static void ChangeSceneBatterSelectToTitle()
{
	sceneManager::Instance().ChangeScene(new scene_loading(new scene_title()));
}

static void ChangeSceneGameToGame()
{
	sceneManager::Instance().ChangeScene(new scene_loading(new scene_game()));
}