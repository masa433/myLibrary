#pragma once
#include "scene.h"
#include <array>
#include <string>
#include <vector>
#include <memory>
#include "imgui.h"
#include "Player.h"


class batterSelectScene : public scene
{
public:
		batterSelectScene() = default;
		 ~batterSelectScene() = default;
		 void initialize() override;
		 void update(float elapsed_time) override;
		 void render(float elapsedTime) override;
		 void uninitialize() override;
		 void DrawGUI() override;

		 //スクロールビュー
		 void DrawPlayerScrollView();

private:

	//	選手のリスト
	std::vector<std::shared_ptr<Player>> playerList;
};