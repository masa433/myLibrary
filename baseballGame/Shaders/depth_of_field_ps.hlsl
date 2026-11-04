#include "depth_of_field.hlsli"

Texture2D color_map : register(t0);
Texture2D bokeh_map : register(t1);
Texture2D depth_map : register(t2);

#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
SamplerState sampler_states[3] : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    //深度値を取得
    float depth = depth_map.Sample(sampler_states[POINT], pin.texcoord);
    
    //深度値から距離を求める
    //  float view_space_z = near * far / (far - depth * (far - near));
    float view_space_z = camera_clip_distance.z / (camera_clip_distance.y - depth * camera_clip_distance.w);
    
    // 焦点距離と被写界深度の範囲を使って、ぼけ具合を計算
    float alpha = abs(view_space_z - focus_distance) / dof_range;
    alpha = saturate(alpha); // 0から1の範囲に制限
    
    //ぼかし色取得
    float3 origin_color = color_map.Sample(sampler_states[LINEAR], pin.texcoord).rgb;
    float3 bokeh_color = bokeh_map.Sample(sampler_states[LINEAR], pin.texcoord).rgb;
    float3 color = bokeh_color * alpha + origin_color * (1.0 - alpha);
    
    return float4(color, 1.0);
}