#include "gltf_model.hlsli"

VS_OUT main(VS_IN vin)
{
    float sigma = vin.tangent.w;
    
    if (skin > -1)
    {
        row_major float4x4 skin_matrix =
            vin.weights.x * joint_matrices[vin.joints.x] +
            vin.weights.y * joint_matrices[vin.joints.y] +
            vin.weights.z * joint_matrices[vin.joints.z] +
            vin.weights.w * joint_matrices[vin.joints.w];
        vin.position = mul(float4(vin.position.xyz, 1), skin_matrix);
        vin.normal = normalize(mul(float4(vin.normal.xyz, 0), skin_matrix));
        vin.tangent = normalize(mul(float4(vin.tangent.xyz, 0), skin_matrix));
    }
    
    VS_OUT vout;
    
    vin.position.w = 1;
    vout.position = mul(vin.position, mul(world, view_projection));
    vout.w_position = mul(vin.position, world);
    
    vin.normal.w = 0;
    vout.w_normal = normalize(mul(vin.normal, world));
    
    vin.tangent.w = 0;
    vout.w_tangent = normalize(mul(vin.tangent, world));
    vout.w_tangent.w = sigma;
    
    vout.texcoord = vin.texcoord;
    
    // シャドウマップ用のパラメーター計算
	{
		// ライトから見たNDC座標を算出
        float4 wvpPos = mul(vin.position, mul(world, light_view_projection));
		// NDC座標からUV座標を算出する
        wvpPos /= wvpPos.w;
        wvpPos.y = -wvpPos.y;
        wvpPos.xy = 0.5f * wvpPos.xy + 0.5f;
        vout.shadow_texcoord = wvpPos.xyz;
    }
    return vout;
}