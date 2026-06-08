#include "Lights.hlsli"
#define SPOTLIGHT_COUNT 36

// 頂点シェーダーへの入力構造体
struct VS_IN
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    uint4 joints : JOINTS;
    float4 weights : WEIGHTS;
};

// 頂点シェーダーからピクセルシェーダーへの出力構造体
struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 w_position : POSITION;
    float4 w_normal : NORMAL;
    float4 w_tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 shadow_texcoord : TEXCOORD1;
};

// プリミティブごとの定数バッファ（b0）
cbuffer PRIMITIVE_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world;
    int material;
    bool has_tangent;
    int skin;
    int pad;
};

// シーン全体の定数バッファ（b1）
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 camera_position;
};

// ライト定数バッファ（b3）
cbuffer LIGHT_CONSTANT_BUFFER : register(b3)
{
    float4 ambient_color;
    float4 directional_light_direction;
    float4 directional_light_color;
    point_lights pointLights[6];
    spot_lights spotLights[SPOTLIGHT_COUNT];
};

// 半球ライト定数バッファ（b4）
cbuffer HEMISPHERE_LIGHT_CONSTANT_BUFFER : register(b4)
{
    float4 sky_color;
    float4 ground_color;
    float4 hemisphere_weight;
};

//フォグ定数バッファ（b5）
cbuffer FOG_CONSTANT_BUFFER : register(b5)
{
    float4 fog_color;
    float4 fog_range;
};

cbuffer SHADOWMAP_CONSTANT_BUFFER : register(b6)
{
    row_major float4x4 light_view_projection;
    float shadow_attenuation;
    float shadow_bias;
    float2 shadow_dummy;
};

// UNIT.37
static const uint PRIMITIVE_MAX_JOINTS = 512;
cbuffer PRIMITIVE_JOINT_CONSTANTS : register(b2)
{
    row_major float4x4 joint_matrices[PRIMITIVE_MAX_JOINTS];
};

#include "shading_function.hlsli"