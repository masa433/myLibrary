#pragma once

#define XAUDIO2_HELPER_FUNCTIONS
#include <xaudio2.h>
#include "AudioResource.h"

// オーディオソース
class AudioSource
{
public:
	AudioSource(IXAudio2* xaudio, std::shared_ptr<AudioResource>& resource, IXAudio2SubmixVoice* sv = nullptr);
	~AudioSource();

	void Finalize();

	// 再生
	void Play(bool loop);

	//呼ばれるとまた最初から再生
	void DC_Play();

	//途中から再生
	void Middle_Play(float Time);

	//指定した所からループ
	void Sustain_Loop(float loop_start, float loop_end);

	// 停止
	void Stop();

	//ボリューム
	void Set_Volume(float volum);

	//デシベルを設定
	float Set_Decibel(float decibls);

	//ピッチ
	void Set_Pitch(float Pitch);

	void Pitch_Shift(float pitch);

	//ピッチのリセット
	void Reset_Pitch();

	//一時停止
	void Pause();

	//リスタート
	void ReStart();

	//フィルター
	void Filter(int type);

	//再生中か
	bool IsPlay();

	//サブミックスボイスへ渡す
	void Set_Submix_voice(IXAudio2SubmixVoice* sv);
private:
	IXAudio2SourceVoice* sourceVoice = nullptr;
	std::shared_ptr<AudioResource>	resource;
	float volum_ = 1.0;
public:
	enum Fliter_Type
	{
		LOW_PASS_FILTER,
		HIGH_PASS_FILTER,
		BAND_PASS_FILTER,
		NOTCH_FILTER,
		LOW_PASS_ONE_POLE_FILTER,
		HIGH_PASS_ONE_POLE_FILTER,
	};

};
