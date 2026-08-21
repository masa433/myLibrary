#include "EffectManager.h"
#include "Graphics.h"

void EffectManager::Initialize()
{
	Graphics& graphics = Graphics::Instance();

	//Effekseerのレンダラーを初期化
	effekseerRenderer = EffekseerRendererDX11::Renderer::Create(graphics.GetDevice(), graphics.GetDeviceContext(),2048);

	//Effekseerのマネージャーを初期化
	effekseerManager = Effekseer::Manager::Create(2048);

	//Effekseerレンダラーの各種設定
	effekseerManager->SetSpriteRenderer(effekseerRenderer->CreateSpriteRenderer());//スプライトレンダラーの設定
	effekseerManager->SetRibbonRenderer(effekseerRenderer->CreateRibbonRenderer());//リボンレンダラーの設定
	effekseerManager->SetRingRenderer(effekseerRenderer->CreateRingRenderer());//リングレンダラーの設定
	effekseerManager->SetTrackRenderer(effekseerRenderer->CreateTrackRenderer());//トラックレンダラーの設定
	effekseerManager->SetModelRenderer(effekseerRenderer->CreateModelRenderer());//モデルレンダラーの設定
	//Effekseer内でのローダーの設定
	effekseerManager->SetTextureLoader(effekseerRenderer->CreateTextureLoader());//テクスチャローダーの設定
	effekseerManager->SetModelLoader(effekseerRenderer->CreateModelLoader());//モデルローダーの設定
	effekseerManager->SetMaterialLoader(effekseerRenderer->CreateMaterialLoader());//マテリアルローダーの設定
	//Effekseerの座標系を左手系に設定
	effekseerManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);//座標系の設定
}

void EffectManager::Uninitialize()
{
	
}

void EffectManager::Update(float elapsedTime)
{
	//Effekseerのマネージャーを更新
	effekseerManager->Update(elapsedTime * 60.0f);
}

void EffectManager::Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	//ビュープロジェクション行列をEffekseerレンダラーに設定
	effekseerRenderer->SetCameraMatrix(*reinterpret_cast<const Effekseer::Matrix44*>(&view));//ビュー行列をEffekseerレンダラーに設定
	effekseerRenderer->SetProjectionMatrix(*reinterpret_cast<const Effekseer::Matrix44*>(&projection));//プロジェクション行列をEffekseerレンダラーに設定

	//Effekseerの描画を開始
	effekseerRenderer->BeginRendering();

	//Effekseerの描画を実行
	effekseerManager->Draw();

	//Effekseerの描画を終了
	effekseerRenderer->EndRendering();
}