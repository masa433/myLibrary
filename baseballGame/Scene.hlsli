cbuffer CbScene : register(b0)
{
    row_major float4x4 viewProjection;
    float4 lightDirection;
    float4 lightColor; // 追加
    float4 ambientColor; // 追加
    float4 cameraPosition;
};


cbuffer CbShadowmap : register(b8)
{
    row_major float4x4 lightViewProjection; // ライトの位置から見た射影行列
    float3 shadowColor; // 影色
    float shadowBias; // 深度バイアス
};