#pragma once

#include <memory>
#include <vector>
#include <xaudio2.h>
#include "AudioResource.h"

// オーディオソース
class AudioSource
{
public:
	AudioSource(IXAudio2* xaudio, std::shared_ptr<AudioResource>& resource);
	~AudioSource();

	// 再生
	void Play(bool loop);

	// 停止
	void Stop();

	// 音量設定
	void SetVolume(float volume);

	//重ねて鳴らす
	void PlayOneShot(float volume = 1.0f);

	void Update();

private:
	IXAudio2SourceVoice* sourceVoice = nullptr;
	std::shared_ptr<AudioResource>	resource;

	IXAudio2* xaudio = nullptr;
	// 重ね鳴らし用のボイス一覧（再生中のものだけ保持）
	std::vector<IXAudio2SourceVoice*>  oneShotVoices;
};
