#include "pch.h"
#include "EffectManager.h"
#include "Graphics.h"

bool EffectManager::Initialize()
{
	Graphics& graphics = Graphics::Instance();

	//Effekseerのレンダラーを初期化
	if (!graphics.GetDevice() || !graphics.GetDeviceContext()) return false;
	effekseerRenderer = EffekseerRendererDX11::Renderer::Create(graphics.GetDevice(), graphics.GetDeviceContext(),2048);
	if (!effekseerRenderer) return false;

	//Effekseerのマネージャーを初期化
	effekseerManager = Effekseer::Manager::Create(2048);
	if (!effekseerManager) return false;

	//Effekseerレンダラーの各種設定
	auto spriteRenderer = effekseerRenderer->CreateSpriteRenderer();
	if (!spriteRenderer) return false;
	effekseerManager->SetSpriteRenderer(spriteRenderer);//スプライトレンダラーの設定
	auto ribbonRenderer = effekseerRenderer->CreateRibbonRenderer();
	if (!ribbonRenderer) return false;
	effekseerManager->SetRibbonRenderer(ribbonRenderer);//リボンレンダラーの設定
	auto ringRenderer = effekseerRenderer->CreateRingRenderer();
	if (!ringRenderer) return false;
	effekseerManager->SetRingRenderer(ringRenderer);//リングレンダラーの設定
	auto trackRenderer = effekseerRenderer->CreateTrackRenderer();
	if (!trackRenderer) return false;
	effekseerManager->SetTrackRenderer(trackRenderer);//トラックレンダラーの設定
	auto modelRenderer = effekseerRenderer->CreateModelRenderer();
	if (!modelRenderer) return false;
	effekseerManager->SetModelRenderer(modelRenderer);//モデルレンダラーの設定
	//Effekseer内でのローダーの設定
	auto textureLoader = effekseerRenderer->CreateTextureLoader();
	if (!textureLoader) return false;
	effekseerManager->SetTextureLoader(textureLoader);//テクスチャローダーの設定
	auto modelLoader = effekseerRenderer->CreateModelLoader();
	if (!modelLoader) return false;
	effekseerManager->SetModelLoader(modelLoader);//モデルローダーの設定
	auto materialLoader = effekseerRenderer->CreateMaterialLoader();
	if (!materialLoader) return false;
	effekseerManager->SetMaterialLoader(materialLoader);//マテリアルローダーの設定
	//Effekseerの座標系を左手系に設定
	effekseerManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
	return true;//座標系の設定
}

void EffectManager::Uninitialize()
{
	effekseerManager.Reset();
	effekseerRenderer.Reset();
}

void EffectManager::Update(float elapsedTime)
{
	if (!effekseerManager) return;
	//Effekseerのマネージャーを更新
	effekseerManager->Update(elapsedTime * 60.0f);
}

void EffectManager::Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	if (!effekseerManager || !effekseerRenderer) return;
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