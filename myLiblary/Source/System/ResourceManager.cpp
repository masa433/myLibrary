#include "../pch.h"
#include "System/ResourceManager.h"
#include "System/Graphics.h"

// モデルリソース読み込み
std::shared_ptr<ModelResource> ResourceManager::LoadModelResource(const char* filename)
{
    //最初の一回だけ読み込み、残りは再利用している
    // すでに読み込まれていた場合は、読み込み済みのリソースを返す
    ModelMap::iterator it = models.find(filename);
    if (it != models.end())
    {
        // weak_ptrからshared_ptrに変換

        if (!it->second.expired())
        {
            // 有効なリソースがあればそれを返す
            return it->second.lock();
        }

    }

    // 新規モデルリソースを作成し、ファイルから読み込む
    /*std::shared_ptr<ModelResource> resource = std::make_shared<ModelResource>();
    resource->Load(Graphics::Instance().GetDevice(), filename);*/
    ID3D11Device* device = Graphics::Instance().GetDevice();
    auto model = std::make_shared<ModelResource>();
    model->Load(device, filename);

    // 読み込み管理用の変数に登録
    models[filename] = model;

    // 作成したリソースを返す
    return model;

    //return nullptr;
}