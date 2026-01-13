#pragma once
#include "System/ModelResource.h"

// リソースマネージャークラス
class ResourceManager
{
private:
	ResourceManager() {};
	~ResourceManager() {};

public:
	static ResourceManager& Instance()
	{
		static ResourceManager instance;
		return instance;
	}

	// モデルリソース取得
	std::shared_ptr<ModelResource>LoadModelResource(const char* filename);


private:
	using ModelMap = std::map<std::string, std::weak_ptr<ModelResource>>;

	ModelMap		models;
};