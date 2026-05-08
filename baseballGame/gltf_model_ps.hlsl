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

        }

		//	点光源
        float3 point_diffuse = 0, point_specular = 0;
        for (int i = 0; i < 6; ++i)
        {
            
         
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
        for (int j = 0; j < 6; ++j)
        {
          
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
            spot_diffuse += CalcLambert(N, L, LC, 1) * attenuation;
            spot_specular += CalcPhongSpecular(N, L, V, LC, 1) * attenuation;
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