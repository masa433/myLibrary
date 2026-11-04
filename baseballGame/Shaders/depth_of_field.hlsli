#include "sprite.hlsli"
#include "shading_function.hlsli"

cbuffer DEPTH_OF_FIELD_CONSTANT_BUFFER : register(b2)
{
    float focus_distance; // 焦点距離
    float dof_range; // 被写界深度の範囲
    float2 dummy; // パディング用
};



cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 camera_position; // w = focus_distance
    float4 camera_right; // w = dof_range
    float4 camera_up;
    float4 camera_clip_distance;
};
