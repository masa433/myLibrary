#include "Scene.hlsli"
#include "ShadowMapCaster.hlsli"

cbuffer CbMesh : register(b1)
{
    float4 materialColor;
};

Texture2D DiffuseMap : register(t0);
SamplerState LinearSampler : register(s0);


Texture2D shadowMap : register(t8);
SamplerState ShadowSamplerState : register(s8);

#define SHADOWMAP_WIDTH 2048
#define SHADOWMAP_HEIGHT 2048

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(LinearSampler, pin.texcoord) * materialColor;
    
    float3 N = normalize(pin.normal);
    float3 L = normalize(-lightDirection.xyz);
    float power = max(0, dot(L, N));

    power = power * 0.5 + 1.0f;

    color.rgb *= power;
    
    //// --- ソフトシャドウ（3x3 PCF） ---
    //float2 texelSize = float2(1.0 / SHADOWMAP_WIDTH, 1.0 / SHADOWMAP_HEIGHT);
    //float shadow = 0.0f;
    //[unroll]
    //for (int y = -1; y <= 1; ++y)
    //{
    //    [unroll]
    //    for (int x = -1; x <= 1; ++x)
    //    {
    //        float2 offset = float2(x, y) * texelSize;
    //        float depth = shadowMap.Sample(ShadowSamplerState, pin.shadowTexcoord.xy + offset).r;
    //        shadow += (pin.shadowTexcoord.z - depth > shadowBias) ? 1.0f : 0.0f;
    //    }
    //}
    //shadow /= 9.0f;

    // シャドウ適用
    //color.rgb = lerp(color.rgb, color.rgb * shadowColor.rgb, shadow);
    // --- ハードシャドウ ---
    float depth = shadowMap.Sample(ShadowSamplerState, pin.shadowTexcoord.xy).r;
    
   
	    // 深度バイアスを考慮して、影の中にいるかどうかを判定
    if (pin.shadowTexcoord.z - depth > shadowBias)
    {
        color.rgb *= shadowColor.rgb;
    }
    
    return color;
}

