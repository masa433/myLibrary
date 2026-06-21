#include "gltf_model.hlsli"
#include "bidirectional_reflectance_distribution_function.hlsli"

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

#define BASECOLOR_TEXTURE          0
#define METALLIC_ROUGHNESS_TEXTURE 1
#define NORMAL_TEXTURE             2
#define EMISSIVE_TEXTURE           3
#define OCCLUSION_TEXTURE          4
Texture2D<float4> material_textures[5] : register(t1);

Texture2D shadow_map : register(t10);
SamplerState shadow_sampler_state : register(s10);
Texture2D cascade_shadow_map[ShadowBufferSize] : register(t20);
SamplerState cascade_shadow_sampler_state : register(s5);

// Poisson disk サンプルオフセット（9点）
static const float2 PoissonDisk[9] =
{
    float2(0.000, 0.000),
    float2(1.000, 0.000),
    float2(-1.000, 0.000),
    float2(0.000, 1.000),
    float2(0.000, -1.000),
    float2(0.707, 0.707),
    float2(-0.707, 0.707),
    float2(0.707, -0.707),
    float2(-0.707, -0.707),
};

float GetShadowFactorPCF(Texture2D shadowTex, SamplerState samp,
                         float2 uv, float depth, float bias, int nSamples)
{
    float sum = 0.0f;
    float2 texelOffset = shadow_map_texel_size * soft_shadow_radius;
    for (int i = 0; i < nSamples; ++i)
    {
        float2 offset = PoissonDisk[i] * texelOffset;
        float sd = shadowTex.Sample(samp, uv + offset).r;
        sum += (depth - sd > bias) ? 0.0f : 1.0f;
    }
    return sum / (float) nSamples;
    // 0=完全に影, 1=完全に光
}

//--------------------------------------------
//  シャドウ係数取得ヘルパー（通常 / カスケード共通）
//--------------------------------------------
float GetShadowFactor(float3 w_pos, float3 shadow_texcoord)
{
    float factor = 1.0f;

    if (use_cascade)
    {
        //[loop]
        for (int i = 0; i < ShadowBufferSize; ++i)
        {
            float4 wvp = mul(float4(w_pos, 1.0f), cascade_light_view_projection[i]);
            wvp /= wvp.w;
            wvp.y = -wvp.y;
            wvp.xy = wvp.xy * 0.5f + 0.5f;

            if (wvp.z >= 0 && wvp.z <= 1 &&
                wvp.x >= 0 && wvp.x <= 1 &&
                wvp.y >= 0 && wvp.y <= 1)
            {
                if(soft_shadow_enabled)
                {
                    float lit = GetShadowFactorPCF(cascade_shadow_map[i],
                    shadow_sampler_state, wvp.xy, wvp.z, 
                    cascade_shadow_bias[i], soft_shadow_samples);
                    //lit: 0=完全に影, 1=完全に光
                    factor = lerp(cascade_shadow_attenuation, 1.0f, lit);
                }
                else
                { 
                    float depth = cascade_shadow_map[i].Sample(shadow_sampler_state, wvp.xy).r;
                    if (wvp.z - depth > cascade_shadow_bias[i])
                    factor = cascade_shadow_attenuation;
                }
                break;
            }
        }
    }
    else
    {
        if(soft_shadow_enabled)
        {
            float lit = GetShadowFactorPCF(shadow_map, shadow_sampler_state,
                shadow_texcoord.xy, shadow_texcoord.z, shadow_bias, soft_shadow_samples);
            //lit: 0=完全に影, 1=完全に光
            factor = lerp(shadow_attenuation, 1.0f, lit);
        }
        else
        {
        
            float depth = shadow_map.Sample(shadow_sampler_state, shadow_texcoord.xy).r;
            if (shadow_texcoord.z - depth > shadow_bias)
                factor = shadow_attenuation;
        }
    }

    return factor;
}

float4 main(VS_OUT pin, bool is_front_face : SV_IsFrontFace) : SV_TARGET
{
    const material_constants m = materials[material];

    // ベースカラー
    float4 basecolor = (m.pbr_metallic_roughness.basecolor_texture.index > -1)
        ? material_textures[BASECOLOR_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord)
        : m.pbr_metallic_roughness.basecolor_factor;

    // 自己発光色
    float3 emmisive = (m.emissive_texture.index > -1)
        ? material_textures[EMISSIVE_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord).rgb
        : m.emissive_factor;

    // 法線
    float3 N = normalize(pin.w_normal.xyz);
    float3 T = has_tangent ? normalize(pin.w_tangent.xyz) : float3(1, 0, 0);
    float sigma = has_tangent ? pin.w_tangent.w : 1.0f;
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);

    if (!is_front_face)
    {
        T = -T;
        B = -B;
        N = -N;
    }

    if (m.normal_texture.index > -1)
    {
        float3 nf = material_textures[NORMAL_TEXTURE].Sample(sampler_states[LINEAR], pin.texcoord).xyz;
        nf = (nf * 2.0f) - 1.0f;
        nf = normalize(nf * float3(m.normal_texture.scale, m.normal_texture.scale, 1.0f));
        N = normalize(nf.x * T + nf.y * B + nf.z * N);
    }

    // 視線ベクトル（カメラへ向かう）
    float3 V = normalize(camera_position.xyz - pin.w_position.xyz);

    // 平行光源
    float3 L_dir = normalize(directional_light_direction.xyz);
    float3 LC = directional_light_color.rgb * directional_light_color.a * directional_light_intensity;

    // 環境光
    float3 ambient = ambient_color.rgb * ambient_color.a;
    ambient += CalcHemiSphereLight(N, float3(0, 1, 0), sky_color.rgb, ground_color.rgb, hemisphere_weight);

    // シャドウ係数
    float shadow_factor = GetShadowFactor(pin.w_position.xyz, pin.shadow_texcoord);
    
    float4 color = (float4) 0;
    color.a = basecolor.a;

    //================================================
    //  トゥーンシェーディング
    //================================================
    if (toon_shading_enabled)
    {
        // 点光源は簡易的に ambient へ加算
        [loop]
        for (int i = 0; i < (int) light_count.y; ++i)
        {
            float3 PL = pin.w_position.xyz - pointLights[i].position.xyz;
            float len = length(PL);
            if (len >= pointLights[i].range)
                continue;
            float att = saturate(1.0f - len / pointLights[i].range);
            att *= att;
            ambient += pointLights[i].color.rgb * pointLights[i].intensity * att * 0.25f;
        }

        color.rgb = CalcToonShading(
            basecolor.rgb,
            N,
            L_dir,
            V,
            LC * shadow_factor,
            ambient,
            toon_diffuse_steps,
            toon_specular_threshold,
            toon_specular_smoothness,
            toon_rim_threshold,
            toon_rim_smoothness,
            toon_rim_color
        );
    }
    //================================================
    //  通常シェーディング
    //================================================
    else
    {
        // カスケードエリア可視化（デバッグ）
        if (use_cascade && display_cascade_area)
        {
            [loop]
            for (int i = 0; i < ShadowBufferSize; ++i)
            {
                float4 wvp = mul(float4(pin.w_position.xyz, 1.0f), cascade_light_view_projection[i]);
                wvp /= wvp.w;
                wvp.y = -wvp.y;
                wvp.xy = wvp.xy * 0.5f + 0.5f;
                if (wvp.z >= 0 && wvp.z <= 1 && wvp.x >= 0 && wvp.x <= 1 && wvp.y >= 0 && wvp.y <= 1)
                {
                    float col_ = rcp((float) (i / 3 + 1));
                    color.rgb = float3(i % 3 == 0, i % 3 == 1, i % 3 == 2) * col_;
                    return color;
                }
            }
            color.rgb = 0;
            return color;
        }

        float3 dir_diffuse = CalcLambert(N, L_dir, LC, 1) * shadow_factor;

        // 点光源
        float3 pt_diffuse = (float3) 0;
        [loop]
        for (int i = 0; i < (int) light_count.y; ++i)
        {
            float3 PL = pin.w_position.xyz - pointLights[i].position.xyz;
            float len = length(PL);
            if (len >= pointLights[i].range)
                continue;
            float att = saturate(1.0f - len / pointLights[i].range);
            att *= att;
            PL /= len;
            pt_diffuse += CalcLambert(N, PL, pointLights[i].color.rgb * pointLights[i].intensity, 1) * att;
        }

        // スポットライト
        float3 sp_diffuse = (float3) 0;
        [unroll]
        for (int j = 0; j < 6; ++j)
        {
            if (j >= (int) light_count.z)
                break;
            float3 SL = pin.w_position.xyz - spotLights[j].position.xyz;
            float len = length(SL);
            if (len >= spotLights[j].range)
                continue;
            float att = saturate(1.0f - len / spotLights[j].range);
            att *= att;
            SL /= len;
            float ang = dot(normalize(spotLights[j].direction.xyz), SL);
            float area = spotLights[j].innerCorn - spotLights[j].outerCorn;
            att *= saturate(1.0f - (spotLights[j].innerCorn - ang) / area);
            float3 SLC = spotLights[j].color.rgb * spotLights[j].intensity;

            // スポットシャドウ
            float sp_shadow = 1.0f;
            {
                float4 lpos = mul(float4(pin.w_position.xyz, 1.0f), spot_light_view_projection[j]);
                lpos.xyz /= lpos.w;
                float2 uv = lpos.xy * float2(0.5f, -0.5f) + 0.5f;
                if (lpos.z >= 0 && lpos.z <= 1 && uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1)
                {
                    float sd_depth = 0;
                    switch (j)
                    {
                        case 0:
                            sd_depth = spot_shadow_map[0].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 1:
                            sd_depth = spot_shadow_map[1].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 2:
                            sd_depth = spot_shadow_map[2].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 3:
                            sd_depth = spot_shadow_map[3].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 4:
                            sd_depth = spot_shadow_map[4].Sample(shadow_sampler_state, uv).r;
                            break;
                        case 5:
                            sd_depth = spot_shadow_map[5].Sample(shadow_sampler_state, uv).r;
                            break;
                    }
                    if (lpos.z - sd_depth > spot_shadow_bias)
                        sp_shadow = spot_shadow_attenuation;
                }
            }
            sp_diffuse += CalcLambert(N, SL, SLC, 1) * att * sp_shadow;
        }

        color.rgb = basecolor.rgb * (ambient + dir_diffuse + pt_diffuse + sp_diffuse);
    }

    //	フォグ（両モード共通）
    color = CalcFog(color, fog_color, fog_range.xy,
                    length(pin.w_position.xyz - camera_position.xyz));

    //	トーンマッピング（両モード共通・最後に適用）
    color.rgb = ApplyToneMapping(
        color.rgb,
        tone_mapping_mode,
        tone_mapping_exposure,
        tone_mapping_white_point);

    return color;
}