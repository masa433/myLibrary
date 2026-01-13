#include "skymap.hlsli"
#include "Scene.hlsli"
#include "function.hlsli"

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    //return texture0.Sample(sampler0, pin.texcoord) * pin.color;
    
    //  pin.world_positionにはピクセル毎のワールド座標が入ってくるので、
    //  それを用いてピクセル毎の視線ベクトルを求める。
    float3 E = normalize(pin.worldPosition.xyz - cameraPosition.xyz);

    //  スカイボックスから色を取得する
    return SampleSkybox(texture0, sampler0, E);
}
