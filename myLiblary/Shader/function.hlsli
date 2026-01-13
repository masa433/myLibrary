//--------------------------------------------
//	ランバート拡散反射計算関数
//--------------------------------------------
// N:法線(正規化済み)
// L:入射ベクトル(正規化済み)
// C:入射光(色・強さ)
// K:反射率
float3 CalcLambert(float3 N, float3 L, float3 C, float3 K)
{
    float power = saturate(dot(N, L));
    return C * power * K;
}

//--------------------------------------------
//	フォンの鏡面反射計算関数
//--------------------------------------------
// N:法線(正規化済み)
// L:入射ベクトル(正規化済み)
// E:視線ベクトル(正規化済み)
// C:入射光(色・強さ)
// K:反射率
float3 CalcPhongSpecular(float3 N, float3 L, float3 V, float3 C, float3 K)
{
    float3 R = reflect(L, N);
    float power = max(dot(V, R), 0);
    power = pow(power, 128);
    return C * power * K;
}

//--------------------------------------------
//	ハーフランバート拡散反射計算関数
//--------------------------------------------
// N:法線(正規化済み)
// L:入射ベクトル(正規化済み)
// C:入射光(色・強さ)
// K:反射率
float3 ClacHalfLambert(float3 N, float3 L, float3 C, float3 K)
{
    float D = saturate(dot(N, L) * 0.5f + 0.5f);
    return C * D * K;
}

//--------------------------------------------
// リムライト
//--------------------------------------------
// N:法線(正規化済み)
// E:視点方向ベクトル(正規化済み)
// L:入射ベクトル(正規化済み)
// C :ライト色
// RimPower : リムライトの強さ(初期値はテキトーなので自分で設定するが吉)
float3 CalcRimLight(float3 N, float3 V, float3 L, float3 C, float RimPower = 3.0f)
{
    float rim = 1.0f - saturate(dot(N, V));
    return C * pow(rim, RimPower) * saturate(dot(L, V));
}

//--------------------------------------------
// ランプシェーディング
//--------------------------------------------
// tex:ランプシェーディング用テクスチャ
// samp:ランプシェーディング用サンプラステート
// N:法線(正規化済み)
// L:入射ベクトル(正規化済み)
// C:入射光(色・強さ)
// K:反射率
float3 CalcRampShading(Texture2D tex, SamplerState samp, float3 N, float3 L, float3 C, float3 K)
{
    float D = saturate(dot(N, L) * 0.5f + 0.5f);
    float Ramp = tex.Sample(samp, float2(D, 0.5f)).r;
    return C * Ramp * K.rgb;
}

//--------------------------------------------
// 半球ライティング
//--------------------------------------------
// normal:法線(正規化済み)
// up:上方向（片方）
// sky_color:空(上)色
// ground_color:地面(下)色
// hemisphere_weight:重み
float3 CalcHemiSphereLight(float3 normal, float3 up, float3 sky_color, float3 ground_color, float4 hemisphere_weight)
{
    float factor = dot(normal, up) * 0.5f + 0.5f;
    return lerp(ground_color, sky_color, factor) * hemisphere_weight.x;
}

//--------------------------------------------
//	フォグ
//--------------------------------------------
//color:現在のピクセル色
//fog_color:フォグの色
//fog_range:フォグの範囲情報
//eye_length:視点からの距離
float4 CalcFog(in float4 color, float4 fogColor, float2 fogRange, float eyeLength)
{
    float fogAlpha = saturate((eyeLength - fogRange.x) / (fogRange.y - fogRange.x));
    return lerp(color, fogColor, fogAlpha);
}

//--------------------------------------------
//	パノラマスカイボックス
//--------------------------------------------
// tex:パノラマスカイボックス用テクスチャ
// samp: パノラマスカイボックス用サンプラステート
//direction:方向ベクトル(正規化済み)
float4 SampleSkybox(Texture2D tex, SamplerState samp, float3 direction)
{
    static const float PI = 3.14159265f;

    float latitude = (1.0f / (2.0f * PI)) * atan2(direction.z, direction.x) + 0.5f;
    float longitude = (1.0f / PI) * atan2(direction.y, length(direction.xz)) + 0.5f;
    return tex.Sample(samp, float2(1.0f - saturate(latitude), 1.0f - saturate(longitude)));
}
