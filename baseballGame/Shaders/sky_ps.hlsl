// sky_ps.hlsl
// 太陽のディスク表現と大気散乱の近似を伴う、昼夜サイクル対応のスカイシェーダー

// --- 定数バッファ ---

// b1: シーン定数（カメラ位置、ビュー・プロジェクション行列など）
cbuffer scene_constants : register(b1)
{
    float4x4 view_projection;
    float4   camera_position;
    float4   camera_right;
    float4   camera_up;
};

// b9: スカイ定数
cbuffer sky_constants : register(b9)
{
    float4   sun_direction;      // 太陽に向かうワールド空間の方向ベクトル（正規化済み）、wは未使用
    float4   sun_color;          // 太陽ディスクの色
    float4   sky_zenith_color;   // 天頂（空の最上部）の色
    float4   sky_horizon_color;  // 地平線の色
    float4   sky_ground_color;   // 地平線より下の色
    float4   cloud_color; // 雲の色（未使用）
    float4   cloud_params; // 雲のパラメータ x=被覆率、y=スケール、z=速度、w=柔らかさ   
    float    time_of_day;        // 時刻（0.0 = 深夜、0.25 = 日の出、0.5 = 正午、0.75 = 日没、1.0 = 深夜）
    float    sun_size;           // 太陽ディスクの視野角（ラジアン単位、目安：0.025）
    float    sun_bloom_size;     // 太陽の柔らかな光（ブルーム）の半径（目安：0.12）
    float cloud_time; // 雲の時間（アニメーション用）
};

// --- 頂点シェーダーからの入力構造体 ---
struct PS_INPUT
{
    float4 position  : SV_POSITION;
    float2 texcoord  : TEXCOORD0;
};

// --- ユーティリティ: クリップ空間からワールド空間のレイ（視線）方向を復元 ---
float3 ReconstructRayDir(float2 uv)
{
    // uv [0, 1] を NDC（正規化デバイス座標） [-1, 1] に変換
    float2 ndc = uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    // 実際には最もクリーンなアプローチ: camera_right, camera_up を使い、前方向を計算する
    float3 right   = normalize(camera_right.xyz);
    float3 up      = normalize(camera_up.xyz);
    float3 forward = cross(right, up);  // 左手系: 右 x 上 = 後ろ方向、そのため反転させる
    
     // 手動でVP行列を反転
    float4x4 VP = view_projection;

    
    // プロジェクション行列（列優先レイアウト）から、半幅と半高を復元する
    // VP[0][0] = (1/tan(fovX/2)) * (H/W), VP[1][1] = 1/tan(fovY/2)
    float3 col0 = float3(VP[0][0], VP[0][1], VP[0][2]); // X軸のスケール
    float3 col1 = float3(VP[1][0], VP[1][1], VP[1][2]); // Y軸のスケール
    
    float inv_tan_fov_x = dot(col0, right);//回転の影響を受けない
    float inv_tan_fov_y = dot(col1, up); //回転の影響を受けない

    float3 ray = forward
               + right   * (ndc.x / inv_tan_fov_x)
               + up      * (ndc.y / inv_tan_fov_y);

    return normalize(ray);
}

// --- 空のグラデーション ---
float3 SkyGradient(float3 ray_dir, float3 sun_dir)
{
    // 高さ要素: -1（地面）から +1（天頂）
    float height = ray_dir.y; // [-1, 1]

    // 地平線の下か上か
    float horizon_blend = smoothstep(-0.05f, 0.05f, height);
    float zenith_blend  = saturate(height * 10.0f);

    //float3 horizon_color = sky_horizon_color.rgb;
    float3 zenith_color  = sky_zenith_color.rgb;
    float3 ground_color  = sky_ground_color.rgb;

    float3 sky = lerp(ground_color, zenith_color, zenith_blend);
    sky = lerp(ground_color, sky, horizon_blend);

    return sky;
}

// --- 地平線付近の大気の靄（ひずみ・ヘイズ） ---
float3 HorizonHaze(float3 ray_dir, float3 sun_dir)
{
    float horizon_factor = 1.0f - abs(ray_dir.y);
    horizon_factor = pow(saturate(horizon_factor), 4.0f);

    // 朝焼け/夕焼けの靄は、太陽の近くほど温かみのある色になる
    float sun_align = dot(ray_dir, sun_dir) * 0.5f + 0.5f;
    float3 warm_haze = lerp(float3(0.3f, 0.15f, 0.05f), float3(1.0f, 0.5f, 0.2f), sun_align);
    float3 cold_haze = float3(0.6f, 0.7f, 0.9f);

    // 時間帯に基づいてブレンド（太陽の色や輝度が昼・夜を示している）
    float day = saturate(sun_dir.y * 3.0f);
    float3 haze_color = lerp(warm_haze, cold_haze, day);

    return haze_color * horizon_factor * 0.1f;
}

// --- 太陽のディスクとコロナ（光輪） ---
float3 SunDisk(float3 ray_dir, float3 sun_dir)
{
    float cos_angle = dot(ray_dir, sun_dir);
    float cos_size  = cos(sun_size);           // はっきりしたエッジ
    float cos_bloom = cos(sun_bloom_size);     // 柔らかな輝き

    // 太陽の明確な円盤
    float disk = step(cos_size, cos_angle);

    // 柔らかな輝き / コロナ
    float bloom_t = saturate((cos_angle - cos_bloom) / (1.0f - cos_bloom));
    float bloom   = pow(bloom_t, 6.0f);

    // 太陽の輪郭の周辺減光（太陽の端にいくほど暗くする表現）
    float limb = saturate((cos_angle - cos_size) / (1.0f - cos_size));
    float limb_darken = lerp(0.7f, 1.0f, sqrt(limb));

    // 太陽が地平線の上にある場合のみ表示
    float above_horizon = smoothstep(0.00f, 0.03f, sun_dir.y);
    
    float bloom_fade = saturate(sun_dir.y * 10.0f);
    float filtered_bloom = bloom * bloom_fade;

    float3 sun_contrib = sun_color.rgb * (disk * limb_darken + filtered_bloom * 0.5f);
    return sun_contrib * above_horizon;
}

// --- 月 ---
float3 Moon(float3 ray_dir, float3 sun_dir, float night_factor)
{
    if (night_factor < 0.01f) return float3(0, 0, 0);

    // 月は太陽の反対側に位置する
    float3 moon_dir = -sun_dir;
    moon_dir.y = abs(moon_dir.y); // 夜間は常に月を地平線の上に維持（演出上の選択）

    float cos_angle = dot(ray_dir, moon_dir);
    float cos_size  = cos(0.018f); // 太陽よりも一回り小さく
    float cos_bloom = cos(0.06f);

    float disk  = step(cos_size, cos_angle);
    float bloom_t = saturate((cos_angle - cos_bloom) / (1.0f - cos_bloom));
    float bloom   = pow(bloom_t, 4.0f);

    float above = smoothstep(-0.02f, 0.05f, moon_dir.y);

    float3 moon_color = float3(0.9f, 0.92f, 1.0f) * (disk * 0.9f + bloom * 0.15f) * above;
    return moon_color * night_factor;
}


float CloudHash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f); // 乱数生成のためのハッシュ関数
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float CloudNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    
   
    // 4つの隣接するグリッドポイントのハッシュ値を取得 
    float a = CloudHash(i); // 左下
    float b = CloudHash(i + float2(1.0f, 0.0f)); // 右下
    float c = CloudHash(i + float2(0.0f, 1.0f)); // 左上
    float d = CloudHash(i + float2(1.0f, 1.0f)); // 右上
    
    float2 u = f * f * (3.0f - 2.0f * f); // スムーズステップ補間
    
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float CloudFbm(float2 p)
{
    float value = 0.0f, amplitude = 0.5f; // フラクタルブラウン運動（FBM）による雲の生成
    [unroll]
    for (int i = 0; i < 5; i++)
    {
        value += amplitude * CloudNoise(p); // 雲のノイズを加算
        p *= 2.02f; // 周波数を倍に
        amplitude *= 0.5f; // 振幅を半分に
    }

    return value;
}

// --- 雲の描画 ---
float3 Clouds(float3 ray_dir,float3 sun_dir,float3 base_sky)
{
    float horizon_fade = smoothstep(0.02f, 0.20f, ray_dir.y); // 地平線付近で雲をフェードアウト
    if (horizon_fade <= 0.0f)
        return base_sky; // 地平線の下では雲を描画しない
    
    float coverage = cloud_params.x; // 雲の被覆率
    if(coverage <= 0.0f)
        return base_sky; // 雲の被覆率が0なら雲は描画しない
    
    float scale = max(cloud_params.y, 0.001f); // 雲のスケール（小さすぎる場合は最小値に制限）
    float speed = cloud_params.z; // 雲の移動速度
    float softness = max(cloud_params.w, 0.01f); // 雲の柔らかさ（小さすぎる場合は最小値に制限）
    
    float2 cloud_uv = ray_dir.xz / ray_dir.y; // 平面投影のUV座標
    float2 wind = float2(1.0f, 0.35f) * cloud_time * speed; // 風による雲の移動
    float2 uv = cloud_uv * scale * 0.15f + wind; // 雲のUV座標をスケーリングして風の影響を加える
    
    float n = CloudFbm(uv); // 雲のフラクタルノイズを取得
    float density = smoothstep(1.0f - coverage - softness, 1.0f - coverage + softness, n); // 雲の密度を計算（被覆率と柔らかさに基づく）
    
    float bulge = smoothstep(0.3f, 0.9f, n); // 雲の膨らみを計算（雲の形状に変化を与える）
    float3 lit_color = cloud_color.rgb * 0.75f; // 雲の基本色を設定（太陽光の影響を受ける）
    float3 shadow_color = cloud_color.rgb * 0.25f; // 雲の影の色を設定（太陽光の影響を受ける）
    float3 cloud_col = lerp(shadow_color, lit_color, bulge); // 雲の色を膨らみに基づいて補間
    
    float3 sun_tint = saturate(sun_color.rgb / max(max(sun_color.r, sun_color.g), max(sun_color.b, 0.001f))); // 太陽の色を正規化して雲に反映
    //cloud_col *= lerp(float3(1.0f, 1.0f, 1.0f), sun_tint, 0.50f); // 太陽の色を雲に少し反映させる

    float sun_up = saturate(sun_dir.y * 2.0f); // 太陽が地平線の上にあるかどうかを判定
    cloud_col *= lerp(0.7f, 1.0f, sun_up); // 太陽が上にあるときは雲を明るくする

    cloud_col = saturate(cloud_col);
    
    float alpha = density * horizon_fade;
    return lerp(base_sky, cloud_col, alpha);

}

// --- メイン関数 ---
float4 main(PS_INPUT input) : SV_TARGET
{
    float3 ray_dir = ReconstructRayDir(input.texcoord);

    float3 sun_dir = normalize(sun_direction.xyz);

    // 夜間係数: 太陽が地平線より下にあるとき
    float night_factor = saturate(1.0f - smoothstep(-0.2f, 0.2f, sun_dir.y));
    
    // 薄明（トワイライト：太陽が地平線付近にあるとき）のセカンドブレンド
    float twilight = saturate(1.0f - abs(sun_dir.y) * 5.0f);

    // 空のベースグラデーション
    float3 sky = SkyGradient(ray_dir, sun_dir);
    
    sky += HorizonHaze(ray_dir, sun_dir);

    sky += Clouds(ray_dir, sun_dir, sky);
    
    // 月
    sky += Moon(ray_dir, sun_dir, night_factor);

    // 太陽ディスク（地平線上にある場合のみ表示、薄明時は輝きへとフェードする）
    sky += SunDisk(ray_dir, sun_dir);

    return float4(sky, 1.0f);
}