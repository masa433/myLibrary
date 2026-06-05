#include "sprite.hlsli"

cbuffer LUMINANCE_EXTRACT : register(b2)
{
    float threshold; // 高輝度抽出のための閾値
    float intensity; // ブルームの強度
    float2 dummy;
}

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

//RGB色空間の数値から輝度値への変換関数
//rgb:RGB色空間の数値
float RGB2Luminance(float3 rgb)
{
    static const float3 luminance_value = float3(0.299f, 0.587f, 0.114f);
    return dot(luminance_value, rgb);
}

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = texture0.Sample(sampler0, pin.texcoord) * pin.color;

	// RGB > 輝度値に変換
    float luminance = RGB2Luminance(color.rgb);

	// 閾値との差を算出
    float contribution = max(0, luminance - threshold);

	// 出力する色を補正する
    color.rgb *= contribution / luminance;
    color.rgb *= intensity;
    return color;
}
