#include "ButtonManager.h"
#include "Graphics.h"
#include "shader.h"
#include "imgui.h"
#include <Windows.h>
#include <commdlg.h>
#include <WICTextureLoader.h>
#include "input.h"
#include "scene_game.h"
#include "scene_loading.h"

#pragma comment(lib, "Comdlg32.lib")

void ButtonManager::Initialize()
{
	HRESULT hr = S_OK;
	ID3D11Device* device = Graphics::Instance().GetDevice();
	

	buttonSpriteData = std::make_unique<std::vector<ButtonSprite>>();
	ButtonSprite defaultButton;
	defaultButton.texturePath = L"";
	defaultButton.textureSRV = nullptr;
	defaultButton.spriteObj = nullptr;
	defaultButton.position = DirectX::XMFLOAT2(100.0f, 100.0f);
	defaultButton.size = DirectX::XMFLOAT2(200.0f, 100.0f);
	defaultButton.rotation = 0.0f;
	defaultButton.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	defaultButton.label = "Button";
	defaultButton.buttonType = ButtonType::None;
	strcpy_s(defaultButton.labelBuffer, "Button");// ラベルの初期値を設定
	buttonSpriteData->push_back(std::move(defaultButton));

	// 入力レイアウトの作成
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// シェーダーの読み込み
	create_vs_from_cso(device, ".\\resources\\shader\\sprite_vs.cso", spriteVS.ReleaseAndGetAddressOf(), spriteInputLayout.ReleaseAndGetAddressOf(), inputElementDesc, _countof(inputElementDesc));
	create_ps_from_cso(device, ".\\resources\\shader\\sprite_ps.cso", spritePS.ReleaseAndGetAddressOf());


	const int screenWidth = static_cast<int>(Graphics::Instance().GetScreenWidth());
	const int screenHeight = static_cast<int>(Graphics::Instance().GetScreenHeight());

	
	std::vector<int> codepoints = FontRenderer::Utf8ToCodepoints(
		u8" !\"#$%&'()*+,-./0123456789:;<=>?@"
		u8"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
		u8"abcdefghijklmnopqrstuvwxyz{|}~"
		u8"あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをんゃゅょっー"
		u8"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンャュョッー"	
	);

	// 日本語グリフを持つフォントを用意して配置する
	fontRenderer = std::make_unique<FontRenderer>();
	fontRenderer->Initialize(device,
		L".\\resources\\fonts\\GenJyuuGothic-P-Bold.ttf",
		28.0f,
		screenWidth, screenHeight,
		1024, 1024,
		&codepoints);
}

void ButtonManager::Update(float elapsedTime)
{
	
	


	//ボタンの更新処理
	//マウスの位置を取得
	Input& input = Input::Instance();

	bool isMouseDown = input.GetMouse().GetButton();
	bool mouseDownEdge = isMouseDown && !prevMouseDown;
	bool mouseUpEdge = !isMouseDown && prevMouseDown;

	//ボタンのクリック判定
	//マウスの位置がボタンの範囲内かどうかの判定
	if(buttonSpriteData && !buttonSpriteData->empty())
	{
		for (auto& button : *buttonSpriteData)
		{
			button.size = button.originalSize;
			
			//グレーになったボタンは反応しないようにする
			if(button.color.x <= 0.5f && button.color.y <= 0.5f && button.color.z <= 0.5f)
			{
				continue;
			}

			//アルファ値が0に近い値なら反応しない
			if(button.currentAlpha <=0.001f)
			{
				continue;
			}

			bool isHovered = IsMouseOverButton({ static_cast<float>(input.GetMouse().GetPositionX()), static_cast<float>(input.GetMouse().GetPositionY()) },
				button.position, button.originalSize);

			const float scaleFactor = 1.15f; // 拡大率（例: 1.15倍）
			DirectX::XMFLOAT2 targetSize = { button.originalSize.x * scaleFactor, button.originalSize.y * scaleFactor };

			float offsetX = (targetSize.x - button.originalSize.x) * 0.5f;
			float offsetY = (targetSize.y - button.originalSize.y) * 0.5f;

			bool isPressed = isHovered && input.GetMouse().GetButton();//押しっぱなし判定

			button.size = isPressed ? button.originalSize : (isHovered ? targetSize : button.originalSize);
		}

		if (mouseDownEdge)
		{
			for (auto& button : *buttonSpriteData)
			{
				if (button.currentAlpha <= 0.001f) continue;

				if (button.color.x <= 0.5f && button.color.y <= 0.5f && button.color.z <= 0.5f) continue;

				bool isHovered = IsMouseOverButton(
					{ static_cast<float>(input.GetMouse().GetPositionX()), static_cast<float>(input.GetMouse().GetPositionY()) },
					button.position, button.originalSize);

				if (isHovered)
				{
					pressedButton = &button; // このフレームでホバーしていたボタンだけをロック
					break; // 1つ見つかったら終了(重なりがなければ十分)
				}

				else
				{
					//ボタンを元のサイズに戻す
					button.size = button.originalSize;
				}
			}
		}

		if(mouseUpEdge)
		{

			
			if (pressedButton != nullptr)
			{
				bool stillHovered = IsMouseOverButton(
					{ static_cast<float>(input.GetMouse().GetPositionX()), static_cast<float>(input.GetMouse().GetPositionY()) },
					pressedButton->position, pressedButton->originalSize);


				if(pressedButton->currentAlpha <= 0.001f)
				{
					stillHovered = false; // アルファ値が0に近い場合はクリック判定を無効化
				}

				if(pressedButton->color.x <= 0.5f && pressedButton->color.y <= 0.5f && pressedButton->color.z <= 0.5f)
				{
					stillHovered = false; // グレーアウトされている場合はクリック判定を無効化
				}

				if (stillHovered)
				{
					switch (pressedButton->buttonType)
					{
					case ButtonType::Start:
						isStartRequested = true;
						break;
					case ButtonType::Settings:
						OutputDebugStringA("Settings button clicked!\n");
						break;
					case ButtonType::Quit:
						OutputDebugStringA("Quit button clicked!\n");
						break;
					case ButtonType::Pose:
						OutputDebugStringA("Pose button clicked!\n");
						break;
					case ButtonType::Return:
						isReturnRequested = true;
						OutputDebugStringA("Return button clicked!\n");
						break;
					case ButtonType::OK:
						isOKRequested = true;
						OutputDebugStringA("OK button clicked!\n");
						break;
					case ButtonType::Close:
						isCloseRequested = true;
						OutputDebugStringA("Close button clicked!\n");
						break;
					case ButtonType::Reroll:
						isRerollRequested = true;
						OutputDebugStringA("Reroll button clicked!\n");
						break;
					case ButtonType::Title:
						isTitleRequested = true;
						OutputDebugStringA("Title button clicked!\n");
						break;
					case ButtonType::BatterSelect:
						isBatterSelectRequested = true;
						OutputDebugStringA("BatterSelect button clicked!\n");
						break;
					case ButtonType::Retry:
						isRetryRequested = true;
						OutputDebugStringA("Retry button clicked!\n");
						break;

					default:
						break;
					}
				}
			}

			pressedButton = nullptr; // 離したら必ずリセット
		}
	}

	prevMouseDown = isMouseDown; // 次フレーム比較用に保存
}

bool ButtonManager::IsMouseOverButton(const DirectX::XMFLOAT2& mousePos, const DirectX::XMFLOAT2& buttonPos, const DirectX::XMFLOAT2& buttonSize)
{
	// マウスの位置がボタンの範囲内にあるかどうかを判定
	//あれば、ボタンのサイズを少し大きくする

	if(mousePos.x >= buttonPos.x && mousePos.x <= buttonPos.x + buttonSize.x &&
		mousePos.y >= buttonPos.y && mousePos.y <= buttonPos.y + buttonSize.y)
	{
		
		return true;
	}
	return false;
}

void ButtonManager::Render(float alpha, ButtonType buttonType)
{
	
	//ボタンの描画処理
	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	
	for (auto& button : *buttonSpriteData)
	{

		
		if (buttonType != ButtonType::None && button.buttonType != buttonType)
		{
			continue;
		}

		//alphaを保存
		button.currentAlpha = alpha;

		//シェーダーの設定
		context->IASetInputLayout(spriteInputLayout.Get());
		context->VSSetShader(spriteVS.Get(), nullptr, 0);
		context->PSSetShader(spritePS.Get(), nullptr, 0);

		context->OMSetDepthStencilState(
			renderState->GetDepthStencilState(DepthState::TestOnly), 0);

		

		if (button.textureSRV)
		{
			context->PSSetShaderResources(0, 1, button.textureSRV.GetAddressOf());
			//ボタンの描画コードをここに追加
			//例: スプライト描画関数を呼び出すなど

			// ここでは、ボタンの位置、サイズ、回転、色などを使用して描画する処理を実装する必要があります
			if (button.spriteObj && buttonSpriteData)
			{
				//positionを中心にずらす

				float offsetX = button.position.x - (button.size.x - button.originalSize.x) * 0.5f;
				float offsetY = button.position.y - (button.size.y - button.originalSize.y) * 0.5f;


				button.spriteObj->render(context,
					offsetX,offsetY,
					button.size.x, button.size.y,
					button.color.x, button.color.y, button.color.z, button.color.w * alpha,
					button.rotation);
			}

			
		}

		if (fontRenderer && !button.label.empty())
		{
			//中央ぞろえにするヘルパー関数
			auto centerTextPosition = [&](const std::string& text, float fontSize, float x, float y) -> DirectX::XMFLOAT2
				{
					float textWidth = 0.0f;
					float textHeight = 0.0f;
					fontRenderer->MeasureText(text.c_str(), fontSize, textWidth, textHeight);
					return { x - textWidth / 2.0f, y - textHeight / 2.0f };
				};


			// フォントの描画
			DirectX::XMFLOAT2 fontPos = centerTextPosition(button.labelBuffer, fontSize,
				fontPosition.x, fontPosition.y
			);

			fontRenderer->DrawText(context, button.labelBuffer,
				fontPos.x, fontPos.y, fontSize, fontColor.x, fontColor.y, fontColor.z, fontColor.w * alpha);
		}
	}

	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	context->IASetInputLayout(nullptr);

	context->OMSetDepthStencilState(
		renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
}

//テクスチャダイアログを開く
std::wstring ButtonManager::OpenTextureFileDialog()
{
	wchar_t fileName[MAX_PATH] = L"";

	OPENFILENAMEW ofn{};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = Graphics::Instance().GetHwnd(); // Graphics側で保持しているHWNDがあれば渡すとダイアログが親ウィンドウ内で扱われます
	ofn.lpstrFilter = L"Texture Files\0*.png;*.jpg;*.jpeg;*.dds;*.tga\0All Files\0*.*\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"テクスチャを選択";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameW(&ofn))
	{
		return fileName;
	}
	return L"";
}	

//ボタンのテクスチャをロードする
void ButtonManager::LoadButtonTexture(ButtonSprite& button, const std::wstring& path)
{
	HRESULT hr = S_OK;
	ID3D11Device* device = Graphics::Instance().GetDevice();
	// 既存のSRVを解放
	button.textureSRV.Reset();
	button.spriteObj.reset();

	// WICTextureLoaderを使用してテクスチャをロード
	hr = DirectX::CreateWICTextureFromFile(device, path.c_str(), nullptr, button.textureSRV.GetAddressOf());
	if (FAILED(hr))
	{
		// ロード失敗時の処理（例: デフォルトテクスチャに置き換えるなど）
		OutputDebugStringA("Failed to load texture: ");
		OutputDebugStringW(path.c_str());
		OutputDebugStringA("\n");
	}
	else
	{
		button.texturePath = path;
		button.spriteObj = std::make_unique<sprite>(device, button.textureSRV);
	}
}

std::string ButtonManager::WideToUtf8(const std::wstring& wide)
{
	if (wide.empty()) return {};
	int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), size, nullptr, nullptr);
	if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();
	return utf8;
}

//ボタンの色を変える関数
void ButtonManager::ChangeColor(DirectX::XMFLOAT4 color,ButtonType buttonType)
{
	//指定されたボタンタイプのボタンの色を変更する
	if(buttonType == ButtonType::None)
	{
		for(auto& button : *buttonSpriteData)
		{
			button.color = color;
		}
	}
	else
	{
		for(auto& button : *buttonSpriteData)
		{
			if(button.buttonType == buttonType)
			{
				button.color = color;
			}
		}
	}
}

void ButtonManager::DrawGUI()
{
#ifdef _DEBUG
#ifdef USE_IMGUI
	if (!buttonSpriteData) return;

	ImGui::Begin(u8"Button Manager");

	if (ImGui::Button(u8"+ ボタン追加"))
	{
		buttonSpriteData->push_back(ButtonSprite{});
	}

	for (size_t i = 0; i < buttonSpriteData->size(); ++i)
	{
		ButtonSprite& btn = (*buttonSpriteData)[i];
		std::string label = "Button " + std::to_string(i);

		if (ImGui::TreeNode(label.c_str()))
		{
			std::string pathUtf8 = WideToUtf8(btn.texturePath);
			ImGui::Text(u8"Texture: %s", pathUtf8.empty() ? "(none)" : pathUtf8.c_str());

			if (btn.textureSRV)
			{
				ImGui::Image((ImTextureID)btn.textureSRV.Get(), ImVec2(64, 64));
			}

			if (ImGui::Button((u8"テクスチャを選択##" + std::to_string(i)).c_str()))
			{
				std::wstring selected = OpenTextureFileDialog();
				if (!selected.empty())
				{
					LoadButtonTexture(btn, selected);
				}
			}

			// ボタンに表示するテキストを手打ち編集
			if (ImGui::InputText((u8"表示テキスト##" + std::to_string(i)).c_str(),
				btn.labelBuffer, sizeof(btn.labelBuffer)))
			{
				btn.label = btn.labelBuffer; // 入力が変わるたびにlabelへ反映
			}
			//フォントの位置、サイズ、色を編集するUI
			ImGui::DragFloat2(u8"フォント位置", &fontPosition.x, 1.0f);
			ImGui::DragFloat(u8"フォントサイズ", &fontSize, 0.01f, 0.1f, 10.0f);
			ImGui::ColorEdit4(u8"フォント色", &fontColor.x);

			//ボタンタイプを選択
			const char* buttonTypeItems[] = { "None", "Start", "Settings", "Quit", "Pose", "Return", "OK", "Close" , "Reroll", "Title", "BatterSelect", "Retry"};
			int currentTypeIndex = static_cast<int>(btn.buttonType);
			ImGui::Combo(u8"ボタンタイプ", &currentTypeIndex, buttonTypeItems, IM_ARRAYSIZE(buttonTypeItems));
			btn.buttonType = static_cast<ButtonManager::ButtonType>(currentTypeIndex);

			ImGui::DragFloat2((u8"位置##" + std::to_string(i)).c_str(), &btn.position.x, 1.0f);
			if(ImGui::DragFloat2((u8"サイズ##" + std::to_string(i)).c_str(), &btn.size.x, 1.0f)) 
			{
				btn.originalSize = btn.size;
			}
			ImGui::DragFloat((u8"回転##" + std::to_string(i)).c_str(), &btn.rotation, 0.01f);
			ImGui::ColorEdit4((u8"色##" + std::to_string(i)).c_str(), &btn.color.x);


			if (ImGui::Button((u8"削除##" + std::to_string(i)).c_str()))
			{
				buttonSpriteData->erase(buttonSpriteData->begin() + i);
				ImGui::TreePop();
				break; // erase後にiが無効になるためループを抜ける
			}

			ImGui::TreePop();
		}
	}

	ImGui::End();
#endif
#endif
}

void ButtonManager::SaveToJson(nlohmann::json& j)
{
	if (!buttonSpriteData) return;
	j["buttons"] = nlohmann::json::array();
	for (const auto& button : *buttonSpriteData)
	{
		nlohmann::json buttonJson;
		buttonJson["texturePath"] = WideToUtf8(button.texturePath);
		buttonJson["position"] = { button.position.x, button.position.y };
		buttonJson["size"] = { button.size.x, button.size.y };
		buttonJson["rotation"] = button.rotation;
		buttonJson["color"] = { button.color.x, button.color.y, button.color.z, button.color.w };
		buttonJson["label"] = button.label;
		buttonJson["buttonType"] = static_cast<int>(button.buttonType);
		j["buttons"].push_back(buttonJson);
	}
}

void ButtonManager::LoadFromJson(const nlohmann::json& j)
{
	if (!buttonSpriteData) return;
	buttonSpriteData->clear();
	for (const auto& buttonJson : j["buttons"])
	{
		ButtonSprite button;
		std::string texturePathUtf8 = buttonJson.value("texturePath", "");
		button.texturePath = std::wstring(texturePathUtf8.begin(), texturePathUtf8.end());
		LoadButtonTexture(button, button.texturePath);
		button.position = { buttonJson["position"][0], buttonJson["position"][1] };
		button.size = { buttonJson["size"][0], buttonJson["size"][1] };
		button.originalSize = button.size;
		button.rotation = buttonJson.value("rotation", 0.0f);
		button.color = { buttonJson["color"][0], buttonJson["color"][1], buttonJson["color"][2], buttonJson["color"][3] };
		button.label = buttonJson.value("label", "");
		strcpy_s(button.labelBuffer, button.label.c_str());
		button.buttonType = static_cast<ButtonType>(buttonJson.value("buttonType", 0));
		buttonSpriteData->push_back(std::move(button));
	}
}