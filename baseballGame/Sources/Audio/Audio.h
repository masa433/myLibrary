#pragma once

#include <xaudio2.h>
#include "AudioSource.h"

// オーディオ
class Audio
{
private:
	Audio() = default;
	~Audio() = default;

public:
	// インスタンス取得
	static Audio& Instance()
	{
		static Audio instance;
		return instance;
	}

	// 初期化
	void Initialize();

	// 終了化
	void Finalize();

	// オーディオソース読み込み
	AudioSource* LoadAudioSource(const char* filename, SoundType type);

	float GetSEVolume() const { return seVolume; }
	float GetBGMVolume() const { return bgmVolume; }
	float GetMasterVolume() const { return masterVolume; }

	void SetSEVolume(float volume) { seVolume = volume; }
	void SetBGMVolume(float volume) { bgmVolume = volume; }	
	void SetMasterVolume(float volume) 
	{ 
		masterVolume = volume;
		if (masteringVoice) {
			masteringVoice->SetVolume(masterVolume);// マスターボリュームを設定
		}
	}

	// カテゴリごとの音量を取得
	float GetCategoryVolume(SoundType type) const
	{
		switch (type) {
		case SoundType::SE:  return seVolume;
		case SoundType::BGM: return bgmVolume;
		default:             return 1.0f;
		}
	}

	void DrawGUI(); // ImGuiで音量調整GUIを描画する関数

private:
	IXAudio2* xaudio = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
	bool comInitialized = false;

	float seVolume = 1.0f; // SEの音量
	float bgmVolume = 1.0f; // BGMの音量
	float masterVolume = 1.0f; // マスターボリューム

};
