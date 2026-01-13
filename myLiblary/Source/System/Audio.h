#pragma once

#include <xaudio2.h>
#include "AudioSource.h"
#include"SubMixSource.h"

// オーディオ
class Audio
{
public:
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
	std::unique_ptr<AudioSource> LoadAudioSource(const char* filename, IXAudio2SubmixVoice* sv = nullptr);
	//サブミックスボイスの作成
	std::unique_ptr<SubMixVoice> MakeSubMix();
public:
	IXAudio2* Get_IXAudio2() { return xaudio; }
private:
	static Audio* instance;

	IXAudio2* xaudio = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
};
