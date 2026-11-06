//頂点情報
struct VS_IN
{
    float3 position : POSITION;
    float2 size : TEXCOORD;
    float4 color : COLOR;
    float4 param : PARAMETER;
};

struct GS_IN
{
    float3 position : POSITION;
    float2 size : TEXCOORD;
    float4 color : COLOR;
    float4 param : PARAMETER;
};

struct PS_IN
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

cbuffer GEOMETRY_PARTICLE_DATA : register(b0)
{
    float2 size; // パーティクルのサイズ
    float2 dummy; // パディング用
};

cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 camera_position; // w = focus_distance
    float4 camera_right; // w = dof_range
    float4 camera_up;
    float4 camera_clip_distance;
    row_major float4x4 view_transform;
    row_major float4x4 projection_transform;
    row_major float4x4 view_projection_transform;
};