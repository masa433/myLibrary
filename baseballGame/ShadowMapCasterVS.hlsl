#include "ShadowMapCaster.hlsli"
#include "Skinning.hlsli"
#include "Scene.hlsli"

VS_OUT main(
    float4 position : POSITION,
    float3 normal : NORMAL,
    float2 texcoord : TEXCOORD,
    float4 boneWeights : BONE_WEIGHTS,
	uint4 boneIndices : BONE_INDICES
)
{
    VS_OUT vout = (VS_OUT) 0;
    
    position = SkinningPosition(position, boneWeights, boneIndices);
    vout.vertex = mul(position, viewProjection);
    vout.texcoord = texcoord;
    vout.normal = SkinningVector(normal, boneWeights, boneIndices);

    // シャドウマップ用パラメータ計算
    {
        float4 wvpPos = mul(position, lightViewProjection);
        
        //NDC系からUV座標を算出
        wvpPos /= wvpPos.w;
        wvpPos.y = -wvpPos.y;
        wvpPos.xy = 0.5f * wvpPos.xy + 0.5f;
        vout.shadowTexcoord = wvpPos.xyz;
    }
    
    //return mul(position, viewProjection);
    return vout;
}