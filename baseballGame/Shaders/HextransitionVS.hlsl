struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2); // UV座標を生成
    output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0); // クリップ座標に変換
    return output;
}