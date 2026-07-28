cbuffer TransformBuffer : register(b0)
{
    float2 center; // 中心座標
    float2 size; // サイズ
    float2 screenSize; // 画面サイズ
    float2 padding; // パディング
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_OUT main(uint vertexID : SV_VertexID)
{
    VS_OUT output;
    
    // 頂点の位置を計算
    float2 corner = float2((vertexID ==1 || vertexID == 3) ? 0.5f : -0.5f,
                           (vertexID == 0 || vertexID == 1) ? 0.5f : -0.5f); // 頂点のコーナー位置を決定
    
    output.texcoord = corner + 0.5f; // テクスチャ座標を計算
    
    //ピクセル座標に変換
    float2 pixelPos = center + corner * size; // 中心座標とサイズを使ってピクセル座標を計算
    float2 ndc = (pixelPos / screenSize) * 2.0f - 1.0f; // NDC座標に変換
    ndc.y = -ndc.y; // Y軸を反転（DirectXの座標系に合わせる）
    
    output.position = float4(ndc, 0.0f, 1.0f); // 最終的な位置を設定
    
    return output;

}