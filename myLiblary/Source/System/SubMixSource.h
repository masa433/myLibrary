#pragma once
#include<xaudio2.h>
#include<xaudio2fx.h>
#include<xapofx.h>

class SubMixVoice
{
public:
	SubMixVoice(IXAudio2* xaudio);
	~SubMixVoice();
	void SetVolume(float volume);
	void Reverb();


private:

public:
	IXAudio2SubmixVoice* GetSubMixVoice() { return pSubMixVoice; }
private:
	IXAudio2SubmixVoice* pSubMixVoice = nullptr;
};