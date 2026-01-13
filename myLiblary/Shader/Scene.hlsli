cbuffer CbScene : register(b0)
{
	row_major float4x4	viewProjection;
	float4				lightDirection;
    float3				cameraPosition;
};

cbuffer CbShadowmap : register(b8)
{
    row_major float4x4 lightViewProjection;
    float3 shadowColor;
    float shadowBias; 
};

cbuffer CbSkymap : register(b5)
{
    row_major float4x4 inverseViewProjection;
};