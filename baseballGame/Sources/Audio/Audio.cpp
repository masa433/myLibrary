#include "pch.h"
#include "misc.h"
#include "Audio.h"

// 初期化
void Audio::Initialize()
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	hr = XAudio2Create(&xaudio, 0);
	if (FAILED(hr) || !xaudio)
	{
		xaudio = nullptr;
		return;   // 音なしで続行（または MessageBox で通知）
	}

	hr = xaudio->CreateMasteringVoice(&masteringVoice);
	if (FAILED(hr) || !masteringVoice)
	{
		masteringVoice = nullptr;
		return;
	}
	masteringVoice->SetVolume(masterVolume);
}

// 終了化
void Audio::Finalize()
{
	// マスタリングボイス破棄
	if (masteringVoice != nullptr)
	{
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}

	// XAudio終了化
	if (xaudio != nullptr)
	{
		xaudio->Release();
		xaudio = nullptr;
	}

	// COM終了化
	CoUninitialize();
}

// オーディオソース読み込み
AudioSource* Audio::LoadAudioSource(const char* filename, SoundType type)
{
	std::shared_ptr<AudioResource> resource = std::make_shared<AudioResource>(filename);
	return new AudioSource(xaudio, resource, type);
}


void Audio::DrawGUI()
{
	ImGui::Begin("Audio Settings");
	ImGui::SliderFloat("SE Volume", &seVolume, 0.0f, 1.0f);
	ImGui::SliderFloat("BGM Volume", &bgmVolume, 0.0f, 1.0f);
	float master = masterVolume;
	if (ImGui::SliderFloat("Master Volume", &master, 0.0f, 1.0f))
	{
		SetMasterVolume(master);   // マスタリングボイスにも反映
	}
	ImGui::End();
}