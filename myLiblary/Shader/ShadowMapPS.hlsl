#include "Scene.hlsli"
#include "ShadowMap.hlsli"

cbuffer CbMesh : register(b1)
{
    float4 materialColor;
};

Texture2D DiffuseMap : register(t0);
SamplerState LinearSampler : register(s0);


Texture2D shadowMap : register(t8);
SamplerState ShadowSamplerState : register(s8);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(LinearSampler, pin.texcoord) * materialColor;
    
    float3 N = normalize(pin.normal);
    float3 L = normalize(-lightDirection.xyz);
    float power = max(0, dot(L, N));

    power = power * 0.5 + 0.5f;

    color.rgb *= power;

    float depth = shadowMap.Sample(ShadowSamplerState, pin.shadowTexcoord.xy).r;

    if (pin.shadowTexcoord.z - depth > shadowBias)
    {
        color.rgb *= shadowColor.rgb;
    }
    
    return color;
}