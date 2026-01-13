#include "skymap.hlsli"
#include "Scene.hlsli"

VS_OUT main(float4 position : POSITION, float4 color : COLOR, float2 texcoord : TEXCOORD)
{
    VS_OUT vout;
    vout.position = position;
    vout.color = color;

    vout.texcoord = texcoord;
    
    //  positionにわたってくる座標はNDC空間の座標なので、、
    //  視線×投影行列の逆行列を使ってワールド空間の座標に変換する
    position = mul(position, inverseViewProjection);
    vout.worldPosition = position / position.w;


    return vout;
}
