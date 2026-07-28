#include "sprite.hlsli"
Texture2D color_map : register(t0);
SamplerState point_sampler_state : register(s0);
SamplerState liner_sampler_state : register(s1);
SamplerState anisotropic_sampler_state : register(s2);
float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = color_map.Sample(anisotropic_sampler_state, pin.texcoord);
    float alpha = color.a;
#if 1
    // Inverse gamma process 
    const float GAMMA = 0.8;
    color.rgb = pow(color.rgb, GAMMA);
#endif 
    return float4(color.rgb, alpha) * pin.color;
    //return color_map.Sample(point_sampler_state, pin.texcoord) * pin.color;
}