#include "geometry_particle.hlsli"

//1頂点から4頂点を生成する
[maxvertexcount(4)]
void main(point GS_IN gin[1], inout TriangleStream<PS_IN> output)
{
    //ビルボード化するために頂点座標をワールド空間からビュー空間に変換する
    float4 pos = mul(float4(gin[0].position, 1.0f), view_transform);
    
    //点を面にするため4頂点の座標を生成する
    float rot = gin[0].param.x; //回転角度
    float s = sin(rot); //回転角度のsin
    float c = cos(rot); //回転角度のcos
    float4 right = float4(c, -s, 0, 0) * (gin[0].size.x * 0.5f); //回転後の右方向ベクトル
    float4 up = float4(s, c, 0, 0) * (gin[0].size.y * 0.5f); //回転後の上方向ベクトル
    
    //4角形ポリゴンを生成
    float4 pos_left_top = pos - right + up; //左上
    float4 pos_left_bottom = pos - right - up; //左下
    float4 pos_right_top = pos + right + up; //右上
    float4 pos_right_bottom = pos + right - up; //右下
    
    //UV座標の切り取り位置をtypeから算出
    uint type = (uint) gin[0].param.y; //パーティクルの種類
    uint komax = (uint) gin[0].param.z; //パーティクルの種類の最大値
    uint komay = (uint) gin[0].param.w; //パーティクルの種類の最大値
    float w = 1.0 / komax; //UV座標の切り取り幅
    float h = 1.0 / komay; //UV座標の切り取り高さ
    float2 uv = float2((type % komax) * w, (type / komax) * h); //UV座標の切り取り位置
    
    //左上の点の位置(射影座標系)・UV・色を計算して出力
    PS_IN pout = (PS_IN) 0;
    pout.color = gin[0].color; //色をそのまま送る
    pout.position = mul(pos_left_top, projection_transform);
    pout.texcoord = uv + float2(0, 0);//テクスチャ左上
    output.Append(pout); //左上の点を出力

    //右上の点の位置(射影座標系)・UV・色を計算して出力
    pout.color = gin[0].color; //色をそのまま送る
    pout.position = mul(pos_right_top, projection_transform);
    pout.texcoord = uv + float2(w, 0); //テクスチャ右上
    output.Append(pout); //右上の点を出力
    
    //左下の点の位置(射影座標系)・UV・色を計算して出力
    pout.color = gin[0].color; //色をそのまま送る
    pout.position = mul(pos_left_bottom, projection_transform);
    pout.texcoord = uv + float2(0, h); //テクスチャ左下
    output.Append(pout); //左下の点を出力

    //右下の点の位置(射影座標系)・UV・色を計算して出力
    pout.color = gin[0].color; //色をそのまま送る
    pout.position = mul(pos_right_bottom, projection_transform);
    pout.texcoord = uv + float2(w, h); //テクスチャ右下
    output.Append(pout); //右下の点を出力
    
    output.RestartStrip(); //ストリップを再開する
}