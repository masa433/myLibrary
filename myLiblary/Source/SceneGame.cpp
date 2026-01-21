#include "pch.h"
#include "System/Misc.h"
#include "System/Graphics.h"
#include "SceneGame.h"
#include"Camera.h"
#include"EnemyManager.h"
#include"EnemySlime.h"
#include "EnemyEgg.h"
#include "Player.h"
#include "Boss.h"
#include "EffectManager.h"
#include "CameraController.h"
#include "System/Input.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "System/GpuResourceUtils.h"
#include "Pitcher.h"



//シャドウマップ用定数
CONST LONG SHADOWMAP_WIDTH{ 1280 };
CONST LONG SHADOWMAP_HEIGHT{ 720 };
CONST float SHADOWMAP_DRAWRECT{ 100 };

void SceneGame::ShadowMapInit()
{
	HRESULT hr = S_OK;

	device = Graphics::Instance().GetDevice();

	// バッファ生成
	GpuResourceUtils::CreateConstantBuffer(device.Get(), sizeof(shadowmap_constants), shadowMap_Constant_Buffer.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer{};
	D3D11_TEXTURE2D_DESC texture2dDesc{};
	texture2dDesc.Width = SHADOWMAP_WIDTH;
	texture2dDesc.Height = SHADOWMAP_HEIGHT;
	texture2dDesc.MipLevels = 1;
	texture2dDesc.ArraySize = 1;
	texture2dDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texture2dDesc.SampleDesc.Count = 1;
	texture2dDesc.SampleDesc.Quality = 0;
	texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
	texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texture2dDesc.CPUAccessFlags = 0;
	texture2dDesc.MiscFlags = 0;
	hr = device->CreateTexture2D(&texture2dDesc, NULL, depthBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	//	深度ステンシルビュー生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView(depthBuffer.Get(),
		&depthStencilViewDesc,
		shadowMap_Depth_Stencil_View.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	//	シェーダーリソースビュー生成
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(depthBuffer.Get(),
		&shaderResourceViewDesc,
		shadowMap_Shader_Resource_View.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// サンプラーステートの生成
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = FLT_MAX;
	samplerDesc.BorderColor[1] = FLT_MAX;
	samplerDesc.BorderColor[2] = FLT_MAX;
	samplerDesc.BorderColor[3] = FLT_MAX;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState(&samplerDesc, shadowMap_Sampler_State.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

}



void SceneGame::Initialize()
{
	// ステージ初期化
	stage = std::make_unique<Stage>();

	net = std::make_unique<Object>();

	// プレイヤー初期化
	Player::Instance().Initialize();
	
	Pitcher::Instance().Initialize();


	// カメラ初期設定
	Graphics& graphics = Graphics::Instance();
	Camera& camera = Camera::Instance();
	camera.SetLookAt(
		DirectX::XMFLOAT3(0, 10, -10), // 視点
		DirectX::XMFLOAT3(0, 0, 0), // 注視点
		DirectX::XMFLOAT3(0, 1, 0) // 上方向
	);
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45), // 視野角
		graphics.GetScreenWidth() / graphics.GetScreenHeight(), // 画面アスペクト比
		0.1f, // クリップ距離(短)
		1000.0f // クリップ距離(長)
	);

	// カメラコントローラー初期化
	cameraController = new CameraController();

	freeCameraController->SyncCameraToController(camera);

	ShadowMapInit();

	// skymap
	{
		skymapSprite = std::make_unique<Sprite>("Data\\Sprite\\skyMap_bluesky.png");
	}
	//AudioManager::Instance().GetSound(SoundList::TitleBGM)->Play(false,1.0f);
}


void SceneGame::Finalize()
{

	// プレイヤー終了化
	Player::Instance().Finalize();

	Pitcher::Instance().Finalize();

	// カメラコントローラー終了化
	if (cameraController != nullptr)
	{
		delete cameraController;
		cameraController = nullptr;
	}

	// エネミー管理クラス終了化
	//EnemyManager::Instance().Clear();

	/*if (gameBGM)
	{
		delete gameBGM;
		gameBGM = nullptr;
	}*/

	skymapSprite.reset();
	stage.reset();
	net.reset();
}


void SceneGame::Update(float elapsedTime)
{
	Camera& camera = Camera::Instance(); // 取得を追加


	// カメラコントローラー更新処理
	DirectX::XMFLOAT3 target = Player::Instance().GetPosition();
	target.y += 0.5f;
	cameraController->SetTarget(target);
	cameraController->Update(elapsedTime);

#ifdef _DEBUG
	// カメラ更新処理
	freeCameraController->Update();
	freeCameraController->SyncControllerToCamera(camera);

#endif
	// ステージ更新処理
	stage->Update(elapsedTime);

	net->Update(elapsedTime);

	// プレイヤー更新処理
	Player::Instance().Update(elapsedTime);

	Pitcher::Instance().Update(elapsedTime);

	// エネミー更新処理
	//EnemyManager::Instance().Update(elapsedTime); // エネミーを全て更新

	//エフェクト更新処理
	EffectManager::Instance().Update(elapsedTime);

	GamePad& gamePad = Input::Instance().GetGamePad();
	// Rキーでゲーム初期化
	if (GetAsyncKeyState('R') & 0x8000) {
		Finalize();    // 現在のリソースを解放
		Initialize();  // 再初期化
		return;        // 2重初期化防止のため
	}
	
	/*if (gamePad.GetButtonDown() & GamePad::BTN_B)
	{
		
		SceneManager::Instance().ChangeScene(new SceneTitle);

	}*/
	/*gameBGM->Play(true,0.5f);
	gameBGM->SetVolume(0.5f);*/

}

void SceneGame::RenderShadowMap()
{
	//シャドウマップ描画処理
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	ShapeRenderer* shapeRenderer = graphics.GetShapeRenderer();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	// 描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.lightDirection = { 0.0f,-1.0f,0.0f };	// ライト方向（下方向）
	rc.renderState = graphics.GetRenderState();

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	cacheRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   cacheDepthStencilView;
	dc->OMGetRenderTargets(1, cacheRenderTargetView.GetAddressOf(), cacheDepthStencilView.GetAddressOf());

	// シャドウマップ
	dc->ClearDepthStencilView(shadowMap_Depth_Stencil_View.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	dc->OMSetRenderTargets(0, nullptr, shadowMap_Depth_Stencil_View.Get());

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(SHADOWMAP_WIDTH);
	viewport.Height = static_cast<float>(SHADOWMAP_HEIGHT);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &viewport);

	//	モデルクラスでのラスタライザーステート設定をきったからここで設定する
	/*rc.deviceContext->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));*/

	{
		Camera& camera = Camera::Instance();


		// ライトの位置から見た視線行列を生成
		DirectX::XMVECTOR LightPosition = DirectX::XMLoadFloat3(&lightDirection);
		LightPosition = DirectX::XMVectorScale(LightPosition, -50);
		DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(LightPosition,
			DirectX::XMVectorSet(camera.GetFocus().x, camera.GetFocus().y, camera.GetFocus().z, 1.0f),
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		// シャドウマップに描画したい範囲の射影行列を生成
		DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicLH(SHADOWMAP_DRAWRECT, SHADOWMAP_DRAWRECT,
			0.1f, 300.0f);

		
		DirectX::XMStoreFloat4x4(&rc.view, V);
		DirectX::XMStoreFloat4x4(&rc.projection, P);


		// 定数バッファの更新
		{
			
			shadowmap_constants shadowMapConstant;
			DirectX::XMStoreFloat4x4(&shadowMapConstant.light_view_projection, V * P);
			shadowMapConstant.shadow_color = shadow_color;
			shadowMapConstant.shadow_bias = shadow_bias;
			dc->UpdateSubresource(shadowMap_Constant_Buffer.Get(), 0, 0, &shadowMapConstant, 0, 0);
			dc->VSSetConstantBuffers(8, 1, shadowMap_Constant_Buffer.GetAddressOf());
			dc->PSSetConstantBuffers(8, 1, shadowMap_Constant_Buffer.GetAddressOf());
		}
		// ステージ描画
		//stage->Render(rc, modelRenderer);

		// プレイヤー描画
		Player::Instance().Render(rc, modelRenderer);
		Pitcher::Instance().Render(rc, modelRenderer);
		net->Render(rc, modelRenderer);
	}

	dc->OMSetRenderTargets(1, cacheRenderTargetView.GetAddressOf(), cacheDepthStencilView.Get());
}

// 描画処理
void SceneGame::Render()
{
	RenderShadowMap();

	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	ShapeRenderer* shapeRenderer = graphics.GetShapeRenderer();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	/*Camera::Instance().SetPerspectiveFov(45,
		graphics.GetScreenWidth() / graphics.GetScreenHeight(),
		0.1f,
		1000.0f);*/

	// 描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.lightDirection = lightDirection;
	rc.renderState = graphics.GetRenderState();


	//カメラパラメータ設定
	Camera& camera = Camera::Instance();
	rc.view = camera.GetView();
	rc.projection = camera.GetProjection();

	dc->PSSetShaderResources(8, 1, shadowMap_Shader_Resource_View.GetAddressOf());
	dc->PSSetSamplers(8, 1, shadowMap_Sampler_State.GetAddressOf());

	//	モデルクラスでのラスタライザーステート設定をきったからここで設定する
	rc.deviceContext->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// skymap
	{
		skymapSprite->SkyRender(rc, 0, 0, 0, 1920, 1080, 0, 0, 1280, 720, 0, 1, 1, 1, 1);
		dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	}

	// 3Dモデル描画
	{
		stage->Render(rc, modelRenderer);
		Player::Instance().Render(rc, modelRenderer);
		//EnemyManager::Instance().Render(rc, modelRenderer);
		EffectManager::Instance().Render(rc.view, rc.projection);
		net->Render(rc, modelRenderer);
		Pitcher::Instance().Render(rc, modelRenderer);
	}

	// 3Dデバッグ描画
	{
		Player::Instance().RenderDebugPrimitive(rc, shapeRenderer);
		//EnemyManager::Instance().RenderDebugPrimitive(rc, shapeRenderer);
		Object::Instance().RenderDebugPrimitive(rc, shapeRenderer);
	}

	// 2Dスプライト描画
	{
		// ...
	}

	ID3D11ShaderResourceView* clearShaderResourceView[] = { nullptr };
	dc->PSSetShaderResources(8, 1, clearShaderResourceView);
}



// GUI描画
void SceneGame::DrawGUI()
{
	// プレイヤーデバッグ描画
	Player::Instance().DrawDebugGUI();
	//Player::Instance().ShowControlPanel();
	//CameraController::Instance().DrawGUI();

	if (stage) {
		stage->DrawImGui();
	}

	net->DrawImGui();
	Pitcher::Instance().DrawImGui();

	//// ウィンドウの位置とサイズを設定
	//ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Once);
	//ImGui::SetNextWindowSize(ImVec2(10, 10), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Title", nullptr, ImGuiWindowFlags_None))
	{
		if (ImGui::Button(u8"タイトルへ"))
		{
			SceneManager::Instance().ChangeScene(new SceneTitle());
		}
	}
	ImGui::End();
	ImGui::ColorEdit3("shadow_color", &shadow_color.x);
	ImGui::SliderFloat("shadow_bias", &shadow_bias, 0.0f, +0.01f);
	if (ImGui::TreeNode("texture"))
	{
		ImGui::Text("shadow_map");
		ImGui::Image(shadowMap_Shader_Resource_View.Get(), { 256, 256 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });

		ImGui::TreePop();
	}

	if (ImGui::Begin("Light", nullptr, ImGuiWindowFlags_None)) {
		ImGui::DragFloat3("Light Direction", &lightDirection.x, 0.05f, -1.0f, 1.0f);
	}
	ImGui::End();
}
