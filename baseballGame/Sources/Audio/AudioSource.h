#pragma once

#include <memory>
#include <vector>
#include <xaudio2.h>
#include <xaudio2fx.h>// リバーブ
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

	void PlayOneShotReverb(float volume = 1.0f, float wetDryMix = 50.0f);

	void Update();

	void StartFadeOut(float duration);

	void UpdateFadeOut(float elapsedTime);

	void StartFadeIn(float duration,bool loop);

	void UpdateFadeIn(float elapsedTime);
	
	//リバーブ関数
	void Reverb(float wetDryMix);

	//今なっているすべてのワンショットの音を止める関数
	void StopOneShotSounds();

	void StartFadeOutOneShot(float duration, float volume = 1.0f);

	void UpdateFadeOutOneShot(float elapsedTime);
	
private:
	IXAudio2SourceVoice* sourceVoice = nullptr;
	std::shared_ptr<AudioResource>	resource;

	IXAudio2* xaudio = nullptr;
	// 重ね鳴らし用のボイス一覧（再生中のものだけ保持）
	std::vector<IXAudio2SourceVoice*>  oneShotVoices;

	float fadeOutTime = 0.0f;
	float fadeDuration = 0.0f;
	float fadeStartVol = 1.0f;

	float fadeInTime = 0.0f;
	float fadeInDuration = 0.0f;
	float fadeInStartVol = 0.0f;

	float oneShotFadeOutTime = 0.0f;
	float oneShotFadeDuration = 0.0f;
	float oneShotFadeStartVol = 1.0f;
};
