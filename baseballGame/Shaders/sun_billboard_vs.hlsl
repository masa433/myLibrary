cbuffer SceneConstants : register(b1)
{
    row_major float4x4 view_projection;
    float4 camera_position;
    float4 camera_right;
    float4 camera_up;
};

cbuffer SunBillBoardConstants : register(b10)
{
    float4 sun_color; // RGB = 色, A = 強度
    float3 sun_world_pos; // ワールド座標
    float sun_size; // コアの半径 (0.03 程度)
    float sun_glow_scale; // グローの広がり (1.0 程度)
    float3 pad; // パディング
};

struct VS_INPUT
{
    float2 corner : TEXCOORD0; // スプライトの四隅を(-1, -1), (1, -1), (-1, 1), (1, 1)で指定
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    
    VS_OUTPUT output;
    
    //カメラの右ベクトルと上ベクトルを計算
    float3 right = camera_right.xyz;
    float3 up = camera_up.xyz;
    right = normalize(right);
    up = normalize(up);
    
    // スプライトの四隅のワールド座標を計算
    float3 worldPos = sun_world_pos + right * input.corner.x * sun_size + up * input.corner.y * sun_size;
    
    output.position = mul(float4(worldPos, 1), view_projection); // ワールド座標をビュー射影行列で変換してスクリーン座標に
    output.texcoord = input.corner; // UV座標はスプライトの四隅を(-1, -1)～(1, 1)で指定
    
    return output;
    
}