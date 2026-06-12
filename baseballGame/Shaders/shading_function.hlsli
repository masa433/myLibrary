
//unit4
//--------------------------------------------
//	ランバート拡散反射計算関数
//--------------------------------------------
// N:法線(正規化済み)
// L:入射ベクトル(正規化済み)
// C:入射光(色・強さ)
// K:反射率
float3 CalcLambert(float3 N, float3 L, float3 C, float3 K)
{
    float power = saturate(dot(N, -L));
    return C * power * K;
}

//unit4
//--------------------------------------------
//	フォンの鏡面反射計算関数
//--------------------------------------------
// N:法線(正規化済み)
// L:入射ベクトル(正規化済み)
// E:視線ベクトル(正規化済み)
// C:入射光(色・強さ)
// K:反射率
float3 CalcPhongSpecular(float3 N, float3 L, float3 E, float3 C, float3 K)
{
    float3 R = reflect(L, N);
    float power = max(dot(-E, R), 0);
    power = pow(power, 128);
    return C * power * K;
}

//unit4
//--------------------------------------------
//	ハーフランバート拡散反射計算関数
//--------------------------------------------
// N:法線(正規化済み)
// L:入射ベクトル(正規化済み)
// C:入射光(色・強さ)
// K:反射率
float3 CalcHalfLambert(float3 N, float3 L, float3 C, float3 K)
{
    float D = saturate(dot(N, -L) * 0.5f + 0.5f);
    return C * D * K;
}

//unit4
//--------------------------------------------
// リムライト
//--------------------------------------------
// N:法線(正規化済み)
// E:視点方向ベクトル(正規化済み)
// L:入射ベクトル(正規化済み)
// C :ライト色
// RimPower : リムライトの強さ(初期値はテキトーなので自分で設定するが吉)
float3 CalcRimLight(float3 N, float3 E, float3 L, float3 C, float RimPower = 3.0f)
{
    float rim = 1.0f - saturate(dot(N, -E));
    return C * pow(rim, RimPower) * saturate(dot(L, -E));
}

//unit4
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
    float D = saturate(dot(N, -L) * 0.5f + 0.5f);
    float Ramp = tex.Sample(samp, float2(D, 0.5f));
    return C * Ramp * K.rgb;

}

//unit5
//--------------------------------------------
// 球体環境マッピング
//--------------------------------------------
// tex:ランプシェーディング用テクスチャ
// samp:ランプシェーディング用サンプラステート
// color: 現在のピクセル色
// N:法線(正規化済み)
// C:入射光(色・強さ)
// value:適応率
float3 CalcSphereEnvironment(Texture2D tex, SamplerState samp, in float3 color, float3 N, float3 E, float value)
{
    float3 R = reflect(E, N);
    float2 texcoord = R.xy * 0.5f + 0.5f;
    return lerp(color.rgb, tex.Sample(samp, texcoord).rgb, value);
}

//unit7
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

//unit7
//--------------------------------------------
//	フォグ
//--------------------------------------------
//color:現在のピクセル色
//fog_color:フォグの色
//fog_range:フォグの範囲情報
//eye_length:視点からの距離
float4 CalcFog(in float4 color, float4 fog_color, float2 fog_range, float eye_length)
{
    float fogAlpha = saturate((eye_length - fog_range.x) / (fog_range.y - fog_range.x));
    return lerp(color, fog_color, fogAlpha);
}

//unit11
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

//	ガンマ係数
static const float GammaFactor = 2.2f;

//	円周率
static const float PI = 3.141592654f;

//--------------------------------------------
//	フレネル項
//--------------------------------------------
//F0	: 垂直入射時の反射率
//VdotH	: 視線ベクトルとハーフベクトル（光源へのベクトルと視点へのベクトルの中間ベクトル
float3 CalcFresnel(float3 F0, float VdotH)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - VdotH, 0.0f, 1.0f), 5.0f);
}

//--------------------------------------------
//	拡散反射BRDF(正規化ランバートの拡散反射)
//--------------------------------------------
//VdotH		: 視線へのベクトルとハーフベクトルとの内積
//fresnelF0	: 垂直入射時のフレネル反射色
//diffuse_reflectance	: 入射光のうち拡散反射になる割合
float3 DiffuseBRDF(float VdotH, float3 fresnelF0, float3 diffuse_reflectance)
{
    return (1.0f - CalcFresnel(fresnelF0, VdotH)) * (diffuse_reflectance / PI);
}


//--------------------------------------------
//	法線分布関数
//--------------------------------------------
//NdotH		: 法線ベクトルとハーフベクトル（光源へのベクトルと視点へのベクトルの中間ベクトル）の内積
//roughness : 粗さ
float CalcNormalDistributionFunction(float NdotH, float roughness)
{
    float a = roughness * roughness; // 粗さを二乗して分布の形状を調整
    float b = (NdotH * NdotH) * (a  - 1.0f) + 1.0f; // 分母の計算
    return a / (PI * b * b); // 分布関数の値を返す
}

//--------------------------------------------
//	幾何減衰項の算出
//--------------------------------------------
//NdotL		: 法線ベクトルと光源へのベクトルとの内積
//NdotV		: 法線ベクトルと視線へのベクトルとの内積
//roughness : 粗さ
float CalcGeometryFunction(float NdotL, float NdotV, float roughness)
{
    float r = roughness * 0.5f; // 粗さを半分にして幾何減衰の形状を調整
    float shadowing = NdotL / (NdotL * (1.0f - r) + r); // 光源への幾何減衰
    float masking = NdotV / (NdotV * (1.0f - r) + r); // 視線への幾何減衰
    return shadowing * masking; // 総合的な幾何減衰を返す
}

//	鏡面反射BRDF（クック・トランスのマイクロファセットモデル）
//--------------------------------------------
//NdotV		: 法線ベクトルと視線へのベクトルとの内積
//NdotL		: 法線ベクトルと光源へのベクトルとの内積
//NdotH		: 法線ベクトルとハーフベクトルとの内積
//VdotH		: 視線へのベクトルとハーフベクトルとの内積
//fresnelF0	: 垂直入射時のフレネル反射色
//roughness	: 粗さ
float3 SpecularBRDF(float NdotV, float NdotL, float NdotH, float VdotH, float3 fresnelF0, float roughness)
{
   //	D項(法線分布)
    float D = CalcNormalDistributionFunction(NdotH, roughness);
	//	G項(幾何減衰項)
    float G = CalcGeometryFunction(NdotL, NdotV, roughness);
	//	F項(フレネル反射)
    float3 F = CalcFresnel(fresnelF0, VdotH);

    return D * G * F / (NdotL * NdotV * 4.0f);

}

//--------------------------------------------
//	直接光の物理ベースライティング
//--------------------------------------------
//diffuse_reflectance	: 入射光のうち拡散反射になる割合
//F0					: 垂直入射時のフレネル反射色
//normal				: 法線ベクトル(正規化済み)
//eye_vector			: 視点に向かうベクトル(正規化済み)
//light_vector			: 光源に向かうベクトル(正規化済み)
//light_color			: ライトカラー
//roughness				: 粗さ
void DirectBRDF(float3 diffuse_reflectance,
				float3 F0,
				float3 normal,
				float3 eye_vector,
				float3 light_vector,
				float3 light_color,
				float roughness,
				out float3 out_diffuse,
				out float3 out_specular)
{
    float3 N = normal;
    float3 L = light_vector;
    float3 V = -eye_vector; // 視点からのベクトルを視線ベクトルに変換
    float3 H = normalize(L + V); // ハーフベクトルの計算
    
    float NdotV = max(0.0001f, dot(N, V));
    float NdotL = max(0.0001f, dot(N, L));
    float NdotH = max(0.0001f, dot(N, H));
    float VdotH = max(0.0001f, dot(V, H));
    
    float3 irradiance = light_color * NdotL; // 入射光の強さを計算
    
    // 拡散反射の計算
    out_diffuse = DiffuseBRDF(VdotH, F0, diffuse_reflectance) * irradiance;
    
    // 鏡面反射の計算
    out_specular = SpecularBRDF(NdotV, NdotL, NdotH, VdotH, F0, roughness) * irradiance;
};

//--------------------------------------------
//	ルックアップテーブルからGGX項を取得
//--------------------------------------------
//brdf_sample_point	: サンプリングポイント
//lut_ggx_map       : GGXルックアップテーブル
//state             : 参照時のサンプラーステート
float4 SampleLutGGX(float2 brdf_sample_point, Texture2D lut_ggx_map, SamplerState state)
{
    return lut_ggx_map.Sample(state, brdf_sample_point);
}

//--------------------------------------------
//	キューブマップから照度を取得
//--------------------------------------------
//v                     : サンプリング方向
//diffuse_iem_cube_map  : 事前計算拡散反射IBLキューブマップ
//state                 : 参照時のサンプラーステート
float4 SampleDiffuseIEM(float3 v, TextureCube diffuse_iem_cube_map, SamplerState state)
{
    return diffuse_iem_cube_map.Sample(state, v);
}

//--------------------------------------------
//	キューブマップから放射輝度を取得
//--------------------------------------------
//v	                        : サンプリング方向
//roughness                 : 粗さ
//specular_pmrem_cube_map   : 事前計算鏡面反射IBLキューブマップ
//state                     : 参照時のサンプラーステート
float4 SampleSpecularPMREM(float3 v, float roughness, TextureCube specular_pmrem_cube_map, SamplerState state)
{
    //  ミップマップによって粗さを表現するため、段階を算出
    uint width, height, mip_maps;
    specular_pmrem_cube_map.GetDimensions(0, width, height, mip_maps);
    float lod = roughness * float(mip_maps - 1);
    return specular_pmrem_cube_map.SampleLevel(state, v, lod);
}

//--------------------------------------------
//	粗さを考慮したフレネル項の近似式
//--------------------------------------------
//F0	    : 垂直入射時の反射率
//VdotN 	: 視線ベクトルと法線ベクトルとの内積
//roughness	: 粗さ
float3 CalcFresnelRoughness(float3 f0, float NdotV, float roughness)
{
    return f0 + (max((float3) (1.0f - roughness), f0) - f0) * pow(saturate(1.0f - NdotV), 5.0f);
}

//--------------------------------------------
//	拡散反射IBL
//--------------------------------------------
//normal                : 法線(正規化済み)
//eye_vector		    : 視線ベクトル(正規化済み)
//roughness				: 粗さ
//diffuse_reflectance	: 入射光のうち拡散反射になる割合
//F0					: 垂直入射時のフレネル反射色
//diffuse_iem_cube_map  : 事前計算拡散反射IBLキューブマップ
//state                 : 参照時のサンプラーステート
float3 DiffuseIBL(float3 normal, float3 eye_vector, float roughness, float3 diffuse_reflectance, float3 f0, TextureCube diffuse_iem_cube_map, SamplerState state)
{
    float3 N = normal;
    float3 V = -eye_vector;

    //  間接拡散反射光の反射率計算
    float NdotV = max(0.0001f, dot(N, V));
    float3 kD = 1.0f - CalcFresnelRoughness(f0, NdotV, roughness);

    float3 irradiance = diffuse_iem_cube_map.Sample(state, normal) .rgb;
    return diffuse_reflectance * irradiance * kD;
}

//--------------------------------------------
//	鏡面反射IBL
//--------------------------------------------
//normal				    : 法線ベクトル(正規化済み)
//eye_vector			    : 視線ベクトル(正規化済み)
//roughness				    : 粗さ
//F0					    : 垂直入射時のフレネル反射色
//lut_ggx_map               : GGXルックアップテーブル
//specular_pmrem_cube_map   : 事前計算鏡面反射IBLキューブマップ
//state                     : 参照時のサンプラーステート
float3 SpecularIBL(float3 normal, float3 eye_vector, float roughness, float3 f0, Texture2D lut_ggx_map, TextureCube specular_pmrem_cube_map, SamplerState state)
{
    float3 N = normal;
    float3 V = -eye_vector;

    float NdotV = max(0.0001f, dot(N, V));
    float3 R = normalize(reflect(-V, N));

    //  ミップマップによって粗さを表現するため、段階を算出
    uint width, height, mip_maps;
    specular_pmrem_cube_map.GetDimensions(0, width, height, mip_maps);
    float lod = roughness * float(mip_maps - 1);
    float3 specular_light = specular_pmrem_cube_map.SampleLevel(state, R, lod) .rgb;

    float2 brdf_sample_point = saturate(float2(NdotV, roughness));
    float2 env_brdf = lut_ggx_map.Sample(state, brdf_sample_point) .rg;

    return specular_light * (f0 * env_brdf.x + env_brdf.y);
}
