#include "gltf_model.hlsli"
#include "bidirectional_reflectance_distribution_function.hlsli"

// UNIT.35
struct texture_info
{
    int index;
    int texcoord;
};
struct normal_texture_info
{
    int index;
    int texcoord;
    float scale;
};
struct occlusion_texture_info
{
    int index;
    int texcoord;
    float strength;
};
struct pbr_metallic_roughness
{
    float4 basecolor_factor;
    texture_info basecolor_texture;
    float metallic_factor;
    float roughness_factor;
    texture_info metallic_roughness_texture;
};
struct material_constants
{
    float3 emissive_factor;
    int alpha_mode;
    float alpha_cutoff;
    bool double_sided;
    
    pbr_metallic_roughness pbr_metallic_roughness;
    
    normal_texture_info normal_texture;
    occlusion_texture_info occlusion_texture;
    texture_info emissive_texture;
};
StructuredBuffer<material_constants> materials : register(t0);

// UNIT.36
#define BASECOLOR_TEXTURE 0
#define METALLIC_ROUGHNESS_TEXTURE 1
#define NORMAL_TEXTURE 2
#define EMISSIVE_TEXTURE 3
#define OCCLUSION_TEXTURE 4
Texture2D<float4> material_textures[5] : register(t1);

//#define POINT 0
//#define LINEAR 1
//#define ANISOTROPIC 2
//SamplerState sampler_states[3] : register(s0);

//	シャドウマップ
Texture2D shadow_map : register(t10);
SamplerState shadow_sampler_state : register(s10);

// カスケードシャドウマップ
Texture2D cascade_shadow_map[ShadowBufferSize] : register(t20);
SamplerState cascade_shadow_sampler_state : register(s5);

float4 main(VS_OUT pin, bool is_front_face : SV_IsFrontFace) : SV_TARGET
{
    // UNIT.35
    const material_constants m = materials[material];

    //	ベースカラーを取得
    float4 basecolor = (float4) 0;
    if (m.pbr_metallic_roughness.basecolor_texture.index > -1)
    {
        basecolor = material_textures[BASECOLOR_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord);
    }
    else
    {
        basecolor = m.pbr_metallic_roughness.basecolor_factor;
    }

    //	自己発光色を取得
    float3 emmisive = (float3) 0;
    if (m.emissive_texture.index > -1)
    {
        emmisive = material_textures[EMISSIVE_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord).rgb;
    }
    else
    {
        emmisive = m.emissive_factor;
    }

    //	法線取得
    float3 N = normalize(pin.w_normal.xyz);
    float3 T = has_tangent ? normalize(pin.w_tangent.xyz) : float3(1, 0, 0);
    float sigma = has_tangent ? pin.w_tangent.w : 1.0;
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);
	//	裏面描画の場合は反転しておく
    if (is_front_face == false)
    {
        T = -T;
        B = -B;
        N = -N;
    }
	
	//	法線マッピング
    if (m.normal_texture.index > -1)
    {
        float4 sampled = material_textures[NORMAL_TEXTURE].Sample(sampler_states[LINEAR], pin.texcoord);
        float3 normal_factor = sampled.xyz;
        normal_factor = (normal_factor * 2.0f) - 1.0f;
        normal_factor = normalize(normal_factor * float3(m.normal_texture.scale, m.normal_texture.scale, 1.0));
        N = normalize((normal_factor.x * T) + (normal_factor.y * B) + (normal_factor.z * N));
    }

    //	視線ベクトル
    float3 V = normalize(pin.w_position.xyz - camera_position.xyz);

	//	シェーディング
    float4 color = (float4) 0;
	{
		//	環境光
        float3 ambient = ambient_color.rgb * ambient_color.a;
        ambient += CalcHemiSphereLight(N, float3(0, 1, 0), sky_color.rgb, ground_color.rgb, hemisphere_weight);

		//	平行光源
        float3 directional_diffuse = 0, directional_specular = 0;
		{
            float3 L = normalize(directional_light_direction.xyz);
            float3 LC = directional_light_color.rgb * directional_light_color.a;
            directional_diffuse = CalcLambert(N, L, LC, 1);
            directional_specular = CalcPhongSpecular(N, L, V, LC, 1);

            if(use_cascade)
            {
                //  カスケードシャドウマップ
                int debug_shadowmap_index = -1;
                for (int index = 0; index < ShadowBufferSize; ++index)
                {
                    // ワールド座標 → ライトNDC座標
                    float4 wvpPos = mul(float4(pin.w_position.xyz, 1.0f), cascade_light_view_projection[index]);
                    wvpPos /= wvpPos.w;
                    wvpPos.y = -wvpPos.y;
                    wvpPos.xy = 0.5f * wvpPos.xy + 0.5f;

                    // このカスケードの範囲内か判定
                    if (wvpPos.z >= 0 && wvpPos.z <= 1 && wvpPos.x >= 0 && wvpPos.x <= 1 && wvpPos.y >= 0 && wvpPos.y <= 1)
                    {
                        float depth = cascade_shadow_map[index].Sample(shadow_sampler_state, wvpPos.xy).r;
                        if (wvpPos.z - depth > cascade_shadow_bias[index])
                        {
                            directional_diffuse *= cascade_shadow_attenuation;
                            directional_specular *= cascade_shadow_attenuation;
                        }
                        debug_shadowmap_index = index;
                        break;
                    }
                }
                
                 // カスケードエリア可視化（デバッグ用）
                if (display_cascade_area)
                {
                    if (debug_shadowmap_index >= 0)
                    {
                        float col = rcp((float) (debug_shadowmap_index / 3 + 1));
                        float r = debug_shadowmap_index % 3 == 0;
                        float g = debug_shadowmap_index % 3 == 1;
                        float b = debug_shadowmap_index % 3 == 2;
                        color.rgb = float3(r, g, b) * col;
                    }
                    else
                    {
                        color.rgb = 0;
                    }
                }
            }
            else
            {
                //	平行光源用シャドウマップ
                float depth = shadow_map.Sample(shadow_sampler_state, pin.shadow_texcoord.xy).r;
			    //	深度値を比較して影かどうかを判定する
                if (pin.shadow_texcoord.z - depth > shadow_bias)
                {
                    directional_diffuse *= shadow_attenuation;
                    directional_specular *= shadow_attenuation;
                }
            }

        }

		//	点光源
        float3 point_diffuse = 0, point_specular = 0;
        for (int i = 0; i < 6; ++i)
        {
            if(i>= light_count.y)
                break;
         
            float3 L = pin.w_position.xyz - pointLights[i].position.xyz;
            float len = length(L);
            if (len >= pointLights[i].range)
                continue;
            float attenuateLength = saturate(1.0f - len / pointLights[i].range);
            float attenuation = attenuateLength * attenuateLength;
            L /= len;
            float3 LC = pointLights[i].color.rgb * pointLights[i].intensity;
            point_diffuse += CalcLambert(N, L, LC, 1) * attenuation;
            point_specular += CalcPhongSpecular(N, L, V, LC, 1) * attenuation;
        }

		//	スポットライト
        float3 spot_diffuse = 0, spot_specular = 0;
        [unroll]
        for (int j = 0; j < 6; ++j)
        {
          
            if(j >= light_count.z)
                break;
            
            float3 L = pin.w_position.xyz - spotLights[j].position.xyz;
            float len = length(L);
            if (len >= spotLights[j].range)
                continue;
            float attenuateLength = saturate(1.0f - len / spotLights[j].range);
            float attenuation = attenuateLength * attenuateLength;
            L /= len;
            float3 spotDirection = normalize(spotLights[j].direction.xyz);
            float angle = dot(spotDirection, L);
            float area = spotLights[j].innerCorn - spotLights[j].outerCorn;
            attenuation *= saturate(1.0f - (spotLights[j].innerCorn - angle) / area);
            float3 LC = spotLights[j].color.rgb * spotLights[j].intensity;
           
            //スポットシャドウマップ判定
            float spot_shadow = 1.0f;
    {
                float4 lpos = mul(float4(pin.w_position.xyz, 1.0f),
                          spot_light_view_projection[j]);
                lpos.xyz /= lpos.w;

                // NDC → UV変換
                float2 uv = lpos.xy * float2(0.5f, -0.5f) + 0.5f;

                // 視錐台内かチェック
                if (lpos.z >= 0.0f && lpos.z <= 1.0f &&
            uv.x >= 0.0f && uv.x <= 1.0f &&
            uv.y >= 0.0f && uv.y <= 1.0f)
                {
                    float depth = 0.0f;

                    switch (j)
                    {
                        case 0:
                            depth = spot_shadow_map[0].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 1:
                            depth = spot_shadow_map[1].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 2:
                            depth = spot_shadow_map[2].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 3:
                            depth = spot_shadow_map[3].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 4:
                            depth = spot_shadow_map[4].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 5:
                            depth = spot_shadow_map[5].Sample(shadow_sampler_state, uv).r;
                            break;
                    }
                    if (lpos.z - depth > spot_shadow_bias)
                    {
                        spot_shadow = spot_shadow_attenuation;
                    }
                }
            }
            
            spot_diffuse += CalcLambert(N, L, LC, 1) * attenuation * spot_shadow;
            spot_specular += CalcPhongSpecular(N, L, V, LC, 1) * attenuation * spot_shadow;
        }
		
		//	合算
        color.a = basecolor.a;
        color.rgb += basecolor.rgb * (ambient + directional_diffuse + point_diffuse + spot_diffuse);
        color.rgb += directional_specular + spot_specular + point_specular;
    }
	
	//	自己発光色加算
    //color.rgb += emmisive;
    color = CalcFog(color, fog_color, fog_range.xy, length(pin.w_position.xyz - camera_position.xyz));
    return color;
}