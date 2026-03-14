cbuffer CbScene : register(b0)
{
    row_major float4x4 viewProjection;
    float4 lightDirection;
    float4 lightColor; // ’Ç‰Á
    float4 ambientColor; // ’Ç‰Á
    float4 cameraPosition;
};
