Texture2D diffuse_map : register(t0);
SamplerState linear_sampler : register(s1);

cbuffer SunConstants : register(b10)
{
    float4 sun_color; // RGB = 色, A = 強度
    float3 sun_world_pos; // ワールド座標
    float sun_size; // コアの半径 (0.03 程度)
    float sun_glow_scale; // グローの広がり (1.0 程度)
    float3 pad; // パディング
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    
    // アスペクト比補正は呼び出し側でsun_screen_posに反映済み想定
    float dist = length(input.texcoord); // スプライトの中心からの距離 (0.0～1.414程度)

    // コア（鋭い円）
    float core = exp(-dist * dist * 12.0);

    // 内グロー
    float glow1 = exp(-dist * dist * 3.0) * 0.6;

    // 外グロー（大気散乱風）
    float glow2 = exp(-dist * dist * 0.8f * sun_glow_scale) * 0.01;

    float intensity = (core + glow1 + glow2) * sun_color.a;
    float3 col = sun_color.rgb * intensity;

    // HDR値（Bloomが乗るよう1.0超え）
    return float4(col, saturate(intensity));
}