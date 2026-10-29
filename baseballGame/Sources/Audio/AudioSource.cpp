#include "pch.h"
#include "Misc.h"
#include "AudioSource.h"
#include "Audio.h"

// コンストラクタ
AudioSource::AudioSource(IXAudio2* xaudio, std::shared_ptr<AudioResource>& resource, SoundType type)
	: resource(resource), xaudio(xaudio), soundType(type)
{
	HRESULT hr;

	// ソースボイスを生成
	hr = xaudio->CreateSourceVoice(&sourceVoice, &resource->GetWaveFormat());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	
}

// デストラクタ
AudioSource::~AudioSource()
{
	if (sourceVoice != nullptr)
	{
		sourceVoice->DestroyVoice();
		sourceVoice = nullptr;
	}

	//再生中の重ね鳴らし用のボイスを破棄
	for(auto* voice : oneShotVoices)
	{
		if(voice != nullptr)
		{
			voice->Stop(0);
			voice->DestroyVoice();
		}
	}
	oneShotVoices.clear();	
}

// 再生
void AudioSource::Play(bool loop)
{
	if (!sourceVoice || !resource) return;

	Stop();

	// ソースボイスにデータを送信
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = resource->GetAudioBytes();
	buffer.pAudioData = resource->GetAudioData();
	buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	HRESULT hr = sourceVoice->SubmitSourceBuffer(&buffer);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	sourceVoice->SetVolume(1.0f);
	hr = sourceVoice->Start();
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

// 停止
void AudioSource::Stop()
{
	if (!sourceVoice) return;

	fadeDuration = 0.0f; // フェードアウト中止
	
	HRESULT hr = sourceVoice->Stop(0);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	hr = sourceVoice->FlushSourceBuffers();
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

// 音量設定
void AudioSource::SetVolume(float volume)
{
	baseVolume = volume;
	sourceVoice->SetVolume(volume * Audio::Instance().GetCategoryVolume(soundType));
}
	
void AudioSource::PlayOneShot(float volume)
{
	if (!sourceVoice || !resource) return;

	// 新しいソースボイスを作成
	IXAudio2SourceVoice* oneShotVoice = nullptr;
	HRESULT hr = xaudio->CreateSourceVoice(&oneShotVoice, &resource->GetWaveFormat());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	if (FAILED(hr) || !oneShotVoice) return;

	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = resource->GetAudioBytes();
	buffer.pAudioData = resource->GetAudioData();
	buffer.LoopCount = 0;// 1回だけ再生
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	hr = oneShotVoice->SubmitSourceBuffer(&buffer);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	oneShotVoice->SetVolume(volume * Audio::Instance().GetCategoryVolume(soundType));
	hr = oneShotVoice->Start();
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// 再生中のボイスを保持
	oneShotVoices.push_back(oneShotVoice);
}

void AudioSource::PlayOneShotReverb(float volume, float wetDryMix)
{
	if (!sourceVoice || !resource) return;
	// 新しいソースボイスを作成
	IXAudio2SourceVoice* oneShotVoice = nullptr;
	HRESULT hr = xaudio->CreateSourceVoice(&oneShotVoice, &resource->GetWaveFormat());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	if (FAILED(hr) || !oneShotVoice) return;
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = resource->GetAudioBytes();
	buffer.pAudioData = resource->GetAudioData();
	buffer.LoopCount = 0;// 1回だけ再生
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	hr = oneShotVoice->SubmitSourceBuffer(&buffer);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	oneShotVoice->SetVolume(volume * Audio::Instance().GetCategoryVolume(soundType));

	// リバーブエフェクトを追加
	IUnknown* reverbEffect = nullptr;
	XAudio2CreateReverb(&reverbEffect);// リバーブエフェクトを作成

	XAUDIO2_EFFECT_DESCRIPTOR effectDesc = {};
	effectDesc.pEffect = reverbEffect;// リバーブエフェクトを設定
	effectDesc.InitialState = TRUE; // エフェクトを有効にする
	effectDesc.OutputChannels = 2; // ステレオ出力

	XAUDIO2_EFFECT_CHAIN effectChain = {};
	effectChain.EffectCount = 1;// エフェクトの数を設定
	effectChain.pEffectDescriptors = &effectDesc;// エフェクトチェーンを設定
	oneShotVoice->SetEffectChain(&effectChain);// エフェクトチェーンをソースボイスに設定
	reverbEffect->Release(); // リバーブエフェクトの参照を解放

	// リバーブのパラメータを設定
	XAUDIO2FX_REVERB_I3DL2_PARAMETERS preset = XAUDIO2FX_I3DL2_PRESET_ARENA; // I3DL2プリセットを使用
	XAUDIO2FX_REVERB_PARAMETERS reverbParams;
	ReverbConvertI3DL2ToNative(&preset, &reverbParams); // I3DL2プリセットをネイティブパラメータに変換
	reverbParams.WetDryMix = wetDryMix; // ウェット/ドライ比を設定

	oneShotVoice->SetEffectParameters(0, &reverbParams, sizeof(reverbParams));// リバーブパラメータを設定
	oneShotVoice->EnableEffect(0); // エフェクトを有効化

	hr = oneShotVoice->Start();
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


	// 再生中のボイスを保持
	oneShotVoices.push_back(oneShotVoice);
}

// 再生し終わったワンショットボイスを片付ける（毎フレーム呼ぶ）
void AudioSource::Update()
{
	//現在の音の音量を毎フレーム更新する
	if(sourceVoice)
	{
		sourceVoice->SetVolume(baseVolume * Audio::Instance().GetCategoryVolume(soundType));
	}

	for (auto it = oneShotVoices.begin(); it != oneShotVoices.end(); )
	{
		XAUDIO2_VOICE_STATE state;
		(*it)->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

		if (state.BuffersQueued == 0) // 再生が終わった
		{
			(*it)->Stop(0);
			(*it)->DestroyVoice();
			it = oneShotVoices.erase(it);
		}
		else
		{
			//oneShotの音量を毎フレーム更新する
			(*it)->SetVolume(baseVolume * Audio::Instance().GetCategoryVolume(soundType));
			++it;

		}
	}
}

// フェードアウト開始
void AudioSource::StartFadeOut(float duration)
{
	if (!sourceVoice) return;
	if (duration <= 0.0f) { Stop(); return; }
	if (fadeDuration > 0.0f) return;          // すでにフェード中なら何もしない
	sourceVoice->GetVolume(&fadeStartVol);   // 今の音量から下げる
	fadeOutTime = 0.0f;
	fadeDuration = duration;
}

// フェードアウト更新
void AudioSource::UpdateFadeOut(float elapsedTime)
{
	if (!sourceVoice || fadeDuration <= 0.0f) return;
	fadeOutTime += elapsedTime;
	float t = (std::min)(fadeOutTime / fadeDuration, 1.0f);

	float newVolume = fadeStartVol * (1.0f - t);
	SetVolume(newVolume * Audio::Instance().GetCategoryVolume(soundType));

	if (t >= 1.0f)
	{
		t = 1.0f;
		Stop(); // フェードアウト完了後に停止
	}
	
}

// フェードイン開始
void AudioSource::StartFadeIn(float duration, bool loop)
{
	if (!sourceVoice) return;
	if (duration <= 0.0f) { SetVolume(0.0f); return; }
	if (fadeInDuration > 0.0f) return;          // すでにフェード中なら何もしない
	fadeInStartVol = 0.0f;                     // フェードインは0から始める
	SetVolume(fadeInStartVol * Audio::Instance().GetCategoryVolume(soundType));
	fadeInTime = 0.0f;
	fadeInDuration = duration;

	Stop();

	// ソースボイスにデータを送信
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = resource->GetAudioBytes();
	buffer.pAudioData = resource->GetAudioData();
	buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	HRESULT hr = sourceVoice->SubmitSourceBuffer(&buffer);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	sourceVoice->SetVolume(baseVolume * Audio::Instance().GetCategoryVolume(soundType));
	hr = sourceVoice->Start();
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

// フェードイン更新
void AudioSource::UpdateFadeIn(float elapsedTime)
{
	if (!sourceVoice || fadeInDuration <= 0.0f) return;
	fadeInTime += elapsedTime;
	float t = (std::min)(fadeInTime / fadeInDuration, 1.0f);

	float newVolume = fadeInStartVol + (1.0f - fadeInStartVol) * t;
	SetVolume(newVolume * Audio::Instance().GetCategoryVolume(soundType));

	if (t >= 1.0f)
	{
		t = 1.0f;
		fadeInDuration = 0.0f; // フェードイン完了
	}
}

void AudioSource::Reverb(float wetDryMix)
{
	if (!sourceVoice) return;

	IUnknown* reverbEffect = nullptr;
	XAudio2CreateReverb(&reverbEffect);// リバーブエフェクトを作成

	XAUDIO2_EFFECT_DESCRIPTOR effectDesc = {};
	effectDesc.pEffect = reverbEffect;// リバーブエフェクトを設定
	effectDesc.InitialState = TRUE; // エフェクトを有効にする
	effectDesc.OutputChannels = 2; // ステレオ出力

	XAUDIO2_EFFECT_CHAIN effectChain = {};
	effectChain.EffectCount = 1;// エフェクトの数を設定
	effectChain.pEffectDescriptors = &effectDesc;// エフェクトチェーンを設定

	sourceVoice->SetEffectChain(&effectChain);// エフェクトチェーンをソースボイスに設定
	reverbEffect->Release(); // リバーブエフェクトの参照を解放

	// リバーブのパラメータを設定
	XAUDIO2FX_REVERB_I3DL2_PARAMETERS preset = XAUDIO2FX_I3DL2_PRESET_ARENA; // I3DL2プリセットを使用
	XAUDIO2FX_REVERB_PARAMETERS reverbParams;
	ReverbConvertI3DL2ToNative(&preset, &reverbParams); // I3DL2プリセットをネイティブパラメータに変換
	reverbParams.WetDryMix = wetDryMix; // ウェット/ドライ比を設定
	sourceVoice->SetEffectParameters(0, &reverbParams, sizeof(reverbParams));// リバーブパラメータを設定
	sourceVoice->EnableEffect(0); // エフェクトを有効化
}

void AudioSource::StopOneShotSounds()
{
	oneShotFadeDuration = 0.0f; // フェードアウト中止
	oneShotFadeOutTime = 0.0f;  // フェードアウト時間リセット

	for (auto* voice : oneShotVoices)
	{
		if (voice != nullptr)
		{
			voice->Stop(0);
			voice->DestroyVoice();
		}
	}
	oneShotVoices.clear();
	oneShotFadeDuration = 0.0f; // フェードアウト中止
	oneShotFadeOutTime = 0.0f;  // フェードアウト時間リセット
}

void AudioSource::StartFadeOutOneShot(float duration, float volume)
{
	if (duration <= 0.0f) { StopOneShotSounds(); return; }
	for (auto* voice : oneShotVoices)
	{
		if (voice != nullptr)
		{
			// フェードアウトのために音量を設定
			voice->SetVolume(volume * Audio::Instance().GetCategoryVolume(soundType));
		}
	}
	oneShotFadeDuration = duration;
	oneShotFadeOutTime = 0.0f;
}

void AudioSource::UpdateFadeOutOneShot(float elapsedTime)
{
	if (oneShotFadeDuration <= 0.0f) return;
	oneShotFadeOutTime += elapsedTime;
	float t = (std::min)(oneShotFadeOutTime / oneShotFadeDuration, 1.0f);

	float newVolume = oneShotFadeStartVol * (1.0f - t);
	for (auto* voice : oneShotVoices)
	{
		if (voice != nullptr)
		{
			voice->SetVolume(newVolume * Audio::Instance().GetCategoryVolume(soundType));
		}
	}

	if (t >= 1.0f)
	{
		StopOneShotSounds(); // フェードアウト完了後に停止
		oneShotFadeDuration = 0.0f; // フェードアウト完了
	}
}

