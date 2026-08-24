#include "Effect.h"
#include "EffectManager.h"
#include "Graphics.h"

Effect::Effect(const char* fileName)
{
	//エフェクトを読み込む前にロックする
	std::lock_guard<std::mutex> lock(Graphics::Instance().GetMutex());

	//Effekseerのリソースを読み込む
	char16_t utf16FileName[256];
	Effekseer::ConvertUtf8ToUtf16(utf16FileName, 256, fileName);

	//Effekseerのマネージャーを取得
	Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

	//Effekseerのエフェクトを読み込む
	effekseerEffect = Effekseer::Effect::Create(effekseerManager, (EFK_CHAR*)utf16FileName);
}

// エフェクトの再生
Effekseer::Handle Effect::Play(const DirectX::XMFLOAT3& position, float scale)
{
	//Effekseerのマネージャーを取得
	Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();
	//エフェクトの再生
	Effekseer::Handle handle = effekseerManager->Play(effekseerEffect, position.x, position.y, position.z);
	//スケールの設定
	effekseerManager->SetScale(handle, scale, scale, scale);
	return handle;
}

// エフェクトの停止
void Effect::Stop(Effekseer::Handle handle)
{
	//Effekseerのマネージャーを取得
	Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();
	//エフェクトの停止
	effekseerManager->StopEffect(handle);
}

//座標設定
void Effect::SetPosition(Effekseer::Handle handle, const DirectX::XMFLOAT3& position)
{
	//Effekseerのマネージャーを取得
	Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();
	//座標の設定
	effekseerManager->SetLocation(handle, position.x, position.y, position.z);
}

//スケール設定
void Effect::SetScale(Effekseer::Handle handle, const DirectX::XMFLOAT3& scale)
{
	//Effekseerのマネージャーを取得
	Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();
	//スケールの設定
	effekseerManager->SetScale(handle, scale.x, scale.y, scale.z);
}