#include "TextureManager.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>

#include "Graphics.h"
#include "RenderState.h"
#include "imgui.h"

namespace
{
	bool IsTextureFile(const std::filesystem::path& path)
	{
		std::wstring extension = path.extension().wstring();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
		return extension == L".png" || extension == L".jpg" || extension == L".jpeg" || extension == L".bmp";
	}
}

void TextureManager::Initialize(ID3D11Device* device, const wchar_t* directory)
{
	Clear();

	ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();

	std::filesystem::path root(directory);
	if (!std::filesystem::exists(root))
	{
		return;
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
	{
		if (!entry.is_regular_file() || !IsTextureFile(entry.path()))
		{
			continue;
		}

		auto path = entry.path();
		auto loadedSprite = std::make_unique<sprite>(device, context, path.c_str());

		TextureAsset asset{};
		asset.path = path.wstring();
		asset.name = path.filename().string();
		asset.desc = loadedSprite->texture2d_desc;

		assets.push_back(asset);
		sprites.push_back(std::move(loadedSprite));
	}

	if (!assets.empty())
	{
		selectedAsset = 0;
		AddInstance(0);
	}
}

void TextureManager::Render(ID3D11DeviceContext* context)
{
	if (assets.empty())
	{
		return;
	}

	RenderState* renderState = Graphics::Instance().GetRenderState();
	context->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	context->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	ID3D11SamplerState* sampler = renderState->GetSamplerState(SamplerState::LinearClamp);
	context->PSSetSamplers(0, 1, &sampler);

	for (const TextureInstance& instance : instances)
	{
		if (!instance.visible || instance.assetIndex < 0 || instance.assetIndex >= static_cast<int>(sprites.size()))
		{
			continue;
		}

		const DirectX::XMFLOAT4& tint = instance.tint;
		sprites[instance.assetIndex]->render(
			context,
			instance.position.x,
			instance.position.y,
			instance.size.x,
			instance.size.y,
			tint.x,
			tint.y,
			tint.z,
			tint.w,
			instance.rotation);
	}
}

void TextureManager::DrawGUI()
{
	if (!ImGui::CollapsingHeader("Texture Manager"))
	{
		return;
	}

	if (assets.empty())
	{
		ImGui::TextUnformatted("No images found in resources/texture.");
		return;
	}

	std::vector<const char*> assetNames;
	assetNames.reserve(assets.size());
	for (const TextureAsset& asset : assets)
	{
		assetNames.push_back(asset.name.c_str());
	}

	ImGui::Combo("Texture", &selectedAsset, assetNames.data(), static_cast<int>(assetNames.size()));
	const TextureAsset& selected = assets[selectedAsset];
	ImGui::Text("Size: %u x %u", selected.desc.Width, selected.desc.Height);

	if (ImGui::Button("Add"))
	{
		AddInstance(selectedAsset);
	}
	ImGui::SameLine();
	if (ImGui::Button("Duplicate") && selectedInstance >= 0 && selectedInstance < static_cast<int>(instances.size()))
	{
		instances.push_back(instances[selectedInstance]);
		selectedInstance = static_cast<int>(instances.size()) - 1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		RemoveSelected();
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Placed Textures");

	for (int i = 0; i < static_cast<int>(instances.size()); ++i)
	{
		const TextureInstance& instance = instances[i];
		std::string label = std::to_string(i) + ": " + assets[instance.assetIndex].name;
		if (ImGui::Selectable(label.c_str(), selectedInstance == i))
		{
			selectedInstance = i;
			selectedAsset = instance.assetIndex;
		}
	}

	if (selectedInstance < 0 || selectedInstance >= static_cast<int>(instances.size()))
	{
		return;
	}

	TextureInstance& instance = instances[selectedInstance];
	ImGui::Separator();
	ImGui::Checkbox("Visible", &instance.visible);
	if (ImGui::Combo("Instance Texture", &instance.assetIndex, assetNames.data(), static_cast<int>(assetNames.size())))
	{
		selectedAsset = instance.assetIndex;
	}

	ImGui::DragFloat2("Position", &instance.position.x, 1.0f, -2000.0f, 4000.0f);
	ImGui::DragFloat2("Size", &instance.size.x, 1.0f, 1.0f, 4000.0f);
	ImGui::DragFloat("Rotation", &instance.rotation, 1.0f, -360.0f, 360.0f);
	ImGui::ColorEdit4("Tint", &instance.tint.x);

	if (ImGui::Button("Actual Size"))
	{
		const TextureAsset& asset = assets[instance.assetIndex];
		instance.size = { static_cast<float>(asset.desc.Width), static_cast<float>(asset.desc.Height) };
	}
	ImGui::SameLine();
	if (ImGui::Button("Move Up"))
	{
		MoveSelected(-1);
	}
	ImGui::SameLine();
	if (ImGui::Button("Move Down"))
	{
		MoveSelected(1);
	}
}

void TextureManager::Clear()
{
	assets.clear();
	sprites.clear();
	instances.clear();
	selectedAsset = 0;
	selectedInstance = -1;
}

void TextureManager::AddInstance(int assetIndex)
{
	if (assetIndex < 0 || assetIndex >= static_cast<int>(assets.size()))
	{
		return;
	}

	const TextureAsset& asset = assets[assetIndex];
	TextureInstance instance{};
	instance.assetIndex = assetIndex;
	instance.position = { 560.0f, 330.0f };
	instance.size = { static_cast<float>(asset.desc.Width), static_cast<float>(asset.desc.Height) };

	instances.push_back(instance);
	selectedInstance = static_cast<int>(instances.size()) - 1;
}

void TextureManager::MoveSelected(int direction)
{
	int target = selectedInstance + direction;
	if (selectedInstance < 0 || selectedInstance >= static_cast<int>(instances.size()) ||
		target < 0 || target >= static_cast<int>(instances.size()))
	{
		return;
	}

	std::swap(instances[selectedInstance], instances[target]);
	selectedInstance = target;
}

void TextureManager::RemoveSelected()
{
	if (selectedInstance < 0 || selectedInstance >= static_cast<int>(instances.size()))
	{
		return;
	}

	instances.erase(instances.begin() + selectedInstance);
	if (instances.empty())
	{
		selectedInstance = -1;
	}
	else
	{
		selectedInstance = (std::min)(selectedInstance, static_cast<int>(instances.size()) - 1);
	}
}
