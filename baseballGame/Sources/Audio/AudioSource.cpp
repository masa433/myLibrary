#include "Misc.h"
#include "AudioSource.h"

// コンストラクタ
AudioSource::AudioSource(IXAudio2* xaudio, std::shared_ptr<AudioResource>& resource)
	: resource(resource), xaudio(xaudio)
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

	HRESULT hr = sourceVoice->Stop(0);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	hr = sourceVoice->FlushSourceBuffers();
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

// 音量設定
void AudioSource::SetVolume(float volume)
{
	sourceVoice->SetVolume(volume);
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

	oneShotVoice->SetVolume(volume);
	hr = oneShotVoice->Start();
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// 再生中のボイスを保持
	oneShotVoices.push_back(oneShotVoice);
}

// 再生し終わったワンショットボイスを片付ける（毎フレーム呼ぶ）
void AudioSource::Update()
{
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
			++it;
		}
	}
}