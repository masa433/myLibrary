#include "pch.h"
#include "misc.h"
#include "Audio.h"

// 初期化
void Audio::Initialize()
{
	HRESULT hr;

	// COMの初期化
	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	UINT32 createFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	//createFlags |= XAUDIO2_DEBUG_ENGINE;
#endif

	// XAudio初期化
	hr = XAudio2Create(&xaudio, createFlags);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// マスタリングボイス生成
	hr = xaudio->CreateMasteringVoice(&masteringVoice);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	masteringVoice->SetVolume(masterVolume); // マスターボリュームを設定
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