
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

//トーンマッピング関数群
//  ApplyToneMapping() の mode 引数で実行時切り替え
//    0 = なし(パススルー)  1 = Reinhard
//    2 = Reinhard Extended  3 = Uncharted2
//    4 = ACES               5 = Lottes


float3 LinearToSRGB(float3 color)
{
    // ガンマ補正を適用して線形空間からsRGB空間に変換
    return pow(max(color, 0.0f), 1.0f / GammaFactor);
}

float3 ApplyExposure(float3 color, float exposure)
{
    // 露出補正を適用
    return color * exposure;
}

float3 ToneMapReinhard(float3 color)
{
    // Reinhardトーンマッピング
    return color / (color + 1.0f);
}

// white_point : 推奨 4～8
float3 ToneMapReinhardExtended(float3 c, float white_point)
{
    // Reinhard Extendedトーンマッピング
    return (c * (1.0f + c / (white_point * white_point))) / (1.0f + c);
}

float3 UC2(float3 x)
{
    // Uncharted2トーンマッピング
    const float A = 0.15f, B = 0.50f, C = 0.10f;
    const float D = 0.20f, E = 0.02f, F = 0.30f;
    // Uncharted2トーンマッピングの計算式
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 ToneMapUncharted2(float3 c)
{
    const float W = 11.2f; // 白色点の輝度
    return UC2(c) / UC2(float3(W, W, W)); // 白色点で正規化
}

float3 ToneMapACES(float3 c)
{
    // ACESトーンマッピング
    return saturate((c * (2.43f * c + 0.03f)) / (c * (2.43f * c + 0.59f) + 0.14f));
}

// Lottesトーンマッピング
float3 ToneMapLottes(float3 c)
{
    const float a = 1.6f, d_ = 0.977f, hdrMax = 8.0f;
    const float midIn = 0.18f, midOut = 0.267f;
    float3 b_ = (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
                ((pow(hdrMax, a * d_) - pow(midIn, a * d_)) * midOut);
    float3 c_ = (pow(hdrMax, a * d_) * pow(midIn, a) -
                 pow(hdrMax, a) * pow(midIn, a * d_) * midOut) /
                ((pow(hdrMax, a * d_) - pow(midIn, a * d_)) * midOut);
    return pow(c, a) / (pow(c, a * d_) * b_ + c_);
}

float3 ApplyToneMapping(float3 color, int mode, float exposure, float white_point)
{
    color = ApplyExposure(color, exposure);
    [flatten]
    switch (mode)
    {
        case 1:
            return LinearToSRGB(ToneMapReinhard(color));
        case 2:
            return LinearToSRGB(ToneMapReinhardExtended(color, white_point));
        case 3:
            return LinearToSRGB(ToneMapUncharted2(color));
        case 4:
            return LinearToSRGB(ToneMapACES(color));
        case 5:
            return LinearToSRGB(ToneMapLottes(color));
        default:
            return color;
    }
}

//--------------------------------------------
//トゥーンシェーディング関数群
//--------------------------------------------

// 拡散輝度を steps 段に量子化
float ToonQuantize(float NdotL, int steps)
{
    float s = 1.0f / (float) steps;
    return floor(NdotL / s) * s + s * 0.5f;
}

// ハードエッジ鏡面反射
float ToonSpecular(float spec_raw, float threshold, float smoothness)
{
    return smoothstep(threshold - smoothness, threshold + smoothness, spec_raw);
}

// ハードエッジリムライト
float ToonRim(float rim_raw, float threshold, float smoothness)
{
    return smoothstep(threshold - smoothness, threshold + smoothness, rim_raw);
}

// N, L, V はすべて正規化済みを渡すこと
// L  : 光源へ向かうベクトル
// V  : カメラへ向かうベクトル（-eye_vector）
// rim_color.w に強度を格納
float3 CalcToonShading(
    float3 base_color,
    float3 N, float3 L, float3 V,
    float3 light_color,
    float3 ambient,
    int diffuse_steps,
    float spec_threshold,
    float spec_smoothness,
    float rim_threshold,
    float rim_smoothness,
    float4 rim_color)
{
    // 拡散
    float NdotL = saturate(dot(N, -L));
    float3 diffuse = base_color * light_color * ToonQuantize(NdotL, diffuse_steps);

    // 鏡面
    float3 H = normalize(-L + V);
    float NdotH = saturate(dot(N, H));
    float3 specular = light_color * ToonSpecular(pow(NdotH, 64.0f), spec_threshold, spec_smoothness);

    // リム
    float rim_raw = 1.0f - saturate(dot(N, V));
    float3 rim = rim_color.rgb * rim_color.w
                    * ToonRim(rim_raw, rim_threshold, rim_smoothness)
                    * saturate(dot(-L, V));

    return ambient * base_color + diffuse + specular + rim;
}