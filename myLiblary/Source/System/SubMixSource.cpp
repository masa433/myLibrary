#include "pch.h"
#include "SubMixSource.h"
#include <xapofx.h>  // XAudio2CreateReverb

SubMixVoice::SubMixVoice(IXAudio2* xaudio)
{
    // チャンネル数・サンプリングレートを設定（ステレオ 44100Hz）
    const UINT32 channels = 2;
    const UINT32 sampleRate = 44100;

    HRESULT hr = xaudio->CreateSubmixVoice(&pSubMixVoice, channels, sampleRate);
    if (FAILED(hr)) {
        pSubMixVoice = nullptr;
        // エラーハンドリング必要に応じて追加
    }
}

SubMixVoice::~SubMixVoice()
{
    if (pSubMixVoice) {
        pSubMixVoice->DestroyVoice();
        pSubMixVoice = nullptr;
    }
}

void SubMixVoice::SetVolume(float volume)
{
    if (pSubMixVoice) {
        pSubMixVoice->SetVolume(volume);
    }
}

void SubMixVoice::Reverb()
{
    if (!pSubMixVoice) return;

    IUnknown* pReverb = nullptr;
    HRESULT hr = XAudio2CreateReverb(&pReverb);
    if (FAILED(hr) || !pReverb) return;

    XAUDIO2_EFFECT_DESCRIPTOR descriptor = {};
    descriptor.InitialState = TRUE;
    descriptor.OutputChannels = 2;
    descriptor.pEffect = pReverb;

    XAUDIO2_EFFECT_CHAIN chain = {};
    chain.EffectCount = 1;
    chain.pEffectDescriptors = &descriptor;

    pSubMixVoice->SetEffectChain(&chain);

    pReverb->Release();
}
