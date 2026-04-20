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

float4 main(VS_OUT pin) : SV_TARGET
{
    // UNIT.35
    const material_constants m = materials[material];

    // ベースカラー取得
    float4 basecolor_factor = m.pbr_metallic_roughness.basecolor_factor;
    const int basecolor_texture = m.pbr_metallic_roughness.basecolor_texture.index;
    if (basecolor_texture > -1)
    {
        float4 sampled = material_textures[BASECOLOR_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord);
        basecolor_factor *= sampled;
    }

    // エミッシブ取得
    float3 emissive_factor = m.emissive_factor;
    const int emissive_texture = m.emissive_texture.index;
    if (emissive_texture > -1)
    {
        float4 sampled = material_textures[EMISSIVE_TEXTURE].Sample(sampler_states[ANISOTROPIC], pin.texcoord);
        emissive_factor *= sampled.rgb;
    }

    // 法線計算
    float3 N = normalize(pin.w_normal.xyz);
    float3 T = has_tangent ? normalize(pin.w_tangent.xyz) : float3(1, 0, 0);
    float sigma = has_tangent ? pin.w_tangent.w : 1.0;
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);

    // 法線マップ適用
    const int normal_texture = m.normal_texture.index;
    if (normal_texture > -1)
    {
        float4 sampled = material_textures[NORMAL_TEXTURE].Sample(sampler_states[LINEAR], pin.texcoord);
        float3 normal_factor = sampled.xyz;
        normal_factor = (normal_factor * 2.0) - 1.0;
        normal_factor = normalize(normal_factor * float3(m.normal_texture.scale, m.normal_texture.scale, 1.0));
        N = normalize((normal_factor.x * T) + (normal_factor.y * B) + (normal_factor.z * N));
    }

    // Phong ライティング
    float3 E = normalize(camera_position.xyz - pin.w_position.xyz);
    float3 L = normalize(-directional_light_direction.xyz);
    
    // アンビエント
    float3 ambient = ambient_color.rgb * basecolor_factor.rgb;

    // ディフューズ
    float diffuse_power = saturate(dot(N, -L));
    float3 diffuse_color = directional_light_color.rgb * diffuse_power * basecolor_factor.rgb;

    // スペキュラー（メタリック・ラフネスベース）
    float3 specular_color = 0;
    {
        float3 R = reflect(L, N);
        float spec_power = max(dot(-E, R), 0.0f);
        
        // メタリック・ラフネス情報を取得
        float metallic_factor = m.pbr_metallic_roughness.metallic_factor;
        float roughness_factor = m.pbr_metallic_roughness.roughness_factor;
        const int metallic_roughness_texture = m.pbr_metallic_roughness.metallic_roughness_texture.index;
        if (metallic_roughness_texture > -1)
        {
            float4 sampled = material_textures[METALLIC_ROUGHNESS_TEXTURE].Sample(sampler_states[LINEAR], pin.texcoord);
            roughness_factor *= sampled.g;
            metallic_factor *= sampled.b;
        }

        // ラフネスによってスペキュラーの鋭さを調整
        float shininess = lerp(128.0f, 4.0f, roughness_factor);
        spec_power = pow(spec_power, shininess);
        
        // メタリックが高いほどスペキュラーが強くなる
        float3 spec_reflect = lerp(float3(0.01f, 0.01f, 0.01f), basecolor_factor.rgb, metallic_factor);
        specular_color = directional_light_color.rgb * spec_power * spec_reflect;
    }

    // 最終色
    float3 color = ambient + diffuse_color + specular_color + emissive_factor;
    
    return float4(color, basecolor_factor.a);
}