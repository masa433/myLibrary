#include "PhongShader.hlsli"
#include "shading_function.hlsli"

cbuffer CbMesh : register(b1)
{
    float4 materialColor;
};

Texture2D DiffuseMap : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = DiffuseMap.Sample(LinearSampler, pin.texcoord) * materialColor;

    
    float3 N = normalize(pin.normal);
    float3 L = normalize(-lightDirection.xyz);
    float3 V = normalize(cameraPosition.xyz - pin.position.xyz);
    float3 H = normalize(L + V);

    float4 ka = { 0.2f, 0.2f, 0.2f, 1.0f }; // 環境光に対する反射係数
    float4 kd = { 0.8f, 0.8f, 0.8f, 1.0f }; // 拡散光に対する反射係数
    float4 ks = { 1.0f, 1.0f, 1.0f, 1.0f }; // 鏡面光に対する反射係数
    
#if 1
    // phong shading
    float3 ambient = ambientColor.rgb * color.rgb;
    float3 diffuse = color.rgb * lightColor.rgb * max(0, dot(L, N) * 0.5 + 0.5);
    float3 specular = lightColor.rgb * pow(max(0, dot(N, normalize(V + L))), 128);

        // ポイントライト
    float3 pointDiffuse = float3(0, 0, 0);
    float3 pointSpecular = float3(0, 0, 0);
    {
        float3 pointLightDir = pin.position.xyz - pointLightPosition.xyz;
        float pointLightDist = length(pointLightDir);
        
        if (pointLightDist < pointLightRange)
        {
            float attenuateLength = saturate(1.0f - (pointLightDist / pointLightRange));
            float attenuation = attenuateLength * attenuateLength; // 二次関数で減衰させる
            pointLightDir /= pointLightDist; // 正規化
            pointDiffuse = CalcLambert(N, pointLightDir, pointLightColor.rgb, kd.rgb) * attenuation;
            pointSpecular = CalcPhongSpecular(N, pointLightDir, V, pointLightColor.rgb, ks.rgb) * attenuation;
        }
    }

    // スポットライト
    float3 spotDiffuse = float3(0, 0, 0);
    float3 spotSpecular = float3(0, 0, 0);
    {
        float3 spotLightDir = pin.position.xyz - spotLightPosition.xyz;
        float spotLightDist = length(spotLightDir);
        
        if (spotLightDist < spotLightRange)
        {
            float attenuateLength = saturate(1.0f - spotLightDist / spotLightRange);
            float attenuation = attenuateLength * attenuateLength;
            spotLightDir /= spotLightDist;
            float3 spotDirection = normalize(spotLightDirection.xyz);
            float angle = dot(spotDirection, spotLightDir);
            float area = spotLightInnerAngle - spotLightOuterAngle;
            attenuation *= saturate(1.0 - (spotLightInnerAngle - angle) / area);
            spotDiffuse += CalcLambert(N, spotLightDir, spotLightColor.rgb, kd.rgb) * attenuation;
            spotSpecular += CalcPhongSpecular(N, spotLightDir, V, spotLightColor.rgb, ks.rgb) * attenuation;


        }
    }

    float3 finalColor = ambient + diffuse + specular + pointDiffuse + pointSpecular + spotDiffuse + spotSpecular;
    return float4(finalColor, color.a);
#else
    // toon shading
    float NoL = max(0, dot(N, L));
    float NoH = max(0, dot(N, H));

    float irradiance = smoothstep(0.0, 0.01, NoL);

    float3 ambient = ambientColor.rgb * color.rgb;
    float3 diffuse = irradiance * lightColor.rgb * color.rgb;

    float specular_intensity = pow(NoH, 512);
    float specular_intensity_smooth = smoothstep(0.005, 0.01, specular_intensity);
    float3 specular = specular_intensity_smooth * lightColor.rgb * color.rgb;

    return float4(ambient + diffuse + specular, color.a);

#endif
}