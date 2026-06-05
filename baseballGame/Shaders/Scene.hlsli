cbuffer CbScene : register(b0)
{
    row_major float4x4 viewProjection;
    float4 lightDirection;
    float4 lightColor; // 追加
    float4 ambientColor; // 追加
    
    // ポイントライト
    float3 pointLightPosition;
    float pointLightRange;
    float3 pointLightColor;
    float pad0;
    
    // スポットライト
    float3 spotLightPosition;
    float spotLightRange;
    float3 spotLightDirection;
    float spotLightInnerAngle;
    float3 spotLightColor;
    float spotLightOuterAngle;
    
    float4 cameraPosition;
};


cbuffer CbShadowmap : register(b8)
{
    row_major float4x4 lightViewProjection; // ライトの位置から見た射影行列
    float3 shadowColor; // 影色
    float shadowBias; // 深度バイアス
};