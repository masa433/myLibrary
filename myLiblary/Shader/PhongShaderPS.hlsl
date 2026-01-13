#include "PhongShader.hlsli"

// テクスチャとマテリアルカラーを使う場合
Texture2D tex0 : register(t0);
SamplerState sam0 : register(s0);

// 定数バッファ例
cbuffer cbMaterial : register(b1)
{
    float3 materialColor;
    float pad0;
};

float4 main(VS_OUT input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(lightDirection);
    float3 V = normalize(-input.worldPos);
    float3 H = normalize(L + V);

    float3 texColor = tex0.Sample(sam0, input.texcoord).rgb;
    float3 baseColor = texColor * materialColor; // テクスチャとマテリアルカラーを合成

    float3 ambient = float3(0.5f, 0.5f, 0.5f) * baseColor;
    float3 diffuse = max(dot(N, L), 0.0f) * baseColor;
    float3 specular = pow(max(dot(N, H), 0.0f), 32) * float3(1.0f, 1.0f, 1.0f);

    return float4(ambient + diffuse + specular, 1.0f);
}