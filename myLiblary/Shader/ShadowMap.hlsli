struct VS_OUT
{
#if 0
    float4 position       : SV_POSITION;
    float4 worldPosition  : POSITION;
    float3 normal         : NORMAL;
    float2 texcoord       : TEXCOORD;
    float4 world_normal   : NORMAL;
    float4 world_position : POSITION;
    float4 color          : COLOR;
    float3 shadowTexcoord : TEXCOORD1;
#else

    float4 vertex : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 position : POSITION;
    float3 tangent : TANGENT;
    float4 color : COLOR;
    float3 shadowTexcoord : TEXCOORD1;
    
#endif
};