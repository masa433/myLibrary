#include "phong_shader.hlsli"

Texture2D color_map : register(t0);
Texture2D normal_map : register(t1);
Texture2D metallic_roughness_map : register(t2);
SamplerState sampler_states : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    // ベースカラーテクスチャをサンプリング
    float4 diffuse_color = color_map.Sample(sampler_states,pin.texcoord);
    diffuse_color *= basecolor_factor;

    // メタリック・ラフネステクスチャをサンプリング
    float4 mr_sample = metallic_roughness_map.Sample(sampler_states,
    pin.texcoord);
    float metallic = mr_sample.b * metallic_factor; // B チャンネル = メタリック
    float roughness = mr_sample.g * roughness_factor; // G チャンネル = ラフネス

    // ライティング計算
    float3 E = normalize(pin.world_position.xyz - camera_position.xyz);
    float3 L = normalize(directional_light_direction.xyz);
    float3 N = normalize(pin.world_normal.xyz);

    // アンビエント
    float3 ambient = ambient_color.rgb * diffuse_color.rgb;

    // ディフューズ
    float diffuse_power = saturate(dot(N, -L));
    float3 directional_diffuse = directional_light_color.rgb * diffuse_power * diffuse_color.rgb;

    // スペキュラー（メタリック・ラフネスベース）
    float3 directional_specular = 0;
    {
        float3 H = normalize(-E - L); // ハーフベクトル
        float spec_power = max(dot(N, H), 0.0f);
        
        // ラフネスによってスペキュラーの鋭さを調整（mix → lerp に変更）
        float shininess = lerp(128.0f, 4.0f, roughness);
        spec_power = pow(spec_power, shininess);
        
        // メタリックが高いほど、ベースカラーをスペキュラーに使用
        float3 spec_color = lerp(float3(0.04f, 0.04f, 0.04f), diffuse_color.rgb, metallic);
        directional_specular = directional_light_color.rgb * spec_power * spec_color;
    }

    float4 color = float4(ambient + directional_diffuse + directional_specular, diffuse_color.a);
    return color;
}