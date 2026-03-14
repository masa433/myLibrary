#include "ToonShader.hlsli"

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

#if 0
	// phong shading
	float3 diffuse = color.rgb * max(0, dot(L, N) * 0.5 + 0.5);
	
	float3 specular = pow(max(0, dot(N, normalize(V + L))), 128);
	
	return float4(diffuse + specular, color.a);
#else
	// toon shading
    float NoL = max(0, dot(N, L));
    float NoH = max(0, dot(N, H));

    float irradiance = smoothstep(0.0, 0.01, NoL);
	
    float3 ambient_color = ambientColor.rgb * color.rgb;
    float3 ambient = lerp(0.4 * ambient_color, 0, irradiance);
	
    float3 diffuse_color = lightColor.rgb * color.rgb;
    float3 diffuse = irradiance * diffuse_color;
	
    float specular_intensity = pow(NoH, 512);
    float specular_intensity_smooth = smoothstep(0.005, 0.01, specular_intensity);
    float3 specular_color = color.rgb;
    float3 specular = specular_intensity_smooth * specular_color;
	
    return float4(ambient + diffuse + specular, color.a);

#endif
}
