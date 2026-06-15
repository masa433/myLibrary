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
    float    time_of_day;        // 時刻（0.0 = 深夜、0.25 = 日の出、0.5 = 正午、0.75 = 日没、1.0 = 深夜）
    float    sun_size;           // 太陽ディスクの視野角（ラジアン単位、目安：0.025）
    float    sun_bloom_size;     // 太陽の柔らかな光（ブルーム）の半径（目安：0.12）
    float    sky_dummy;
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

    // ビュー・プロジェクション行列の逆行列を使用してワールド空間の方向を求める
    //float4x4 inv_vp = view_projection;

    // view_projectionの逆行列が必要。
    // 近平面と遠平面のワールド位置を構築して、その差分を計算する。
    float4 near_h = float4(ndc.x, ndc.y, 0.0f, 1.0f);
    float4 far_h  = float4(ndc.x, ndc.y, 1.0f, 1.0f);

    // 手動でVP行列を反転
    float4x4 VP = view_projection;

    // ヘルパー: scene_constantsに格納されているカメラの基底ベクトルを使用する
    // よりシンプルなアプローチ: カメラの各ベクトルから復元する
    // 転置のテクニックを用いてスクリーン空間からワールド空間への変換を導出する
    // 実際には、逆VP行列を使ってゼロからレイを復元する方が綺麗。

    // 標準的な余因子行列による逆行列計算はHLSLでは複雑なため、代わりに
    // カメラ位置 ＋ スクリーン方向 からレイを再構築する。
    // カメラのベクトル（右、上）は利用可能なので、前方向 = cross(右, 上) を計算する。
    // ただしFOVも必要となる。直接保持していないため、VP行列から復元する。

    // VP行列からカメラの前方向を抽出:
    // プロジェクションによって平行移動が除去され、ビューによってカメラがエンコードされる。
    // VPの第2行（行優先のHLSL）は、プロジェクション後のZ行となる。
    // 代わりに、2つのクリップ座標点を逆投影する最も単純な方法を使用する。

    // VPはHLSL内で行優先（行ベクトル）
    // 逆投影するためには: VP * w_pos = h_pos  =>  w_pos = h_pos * VP^-1 を解く
    // ここでは 4x4 の小行列展開による逆行列を使用する。

    // 実際には最もクリーンなアプローチ: camera_right, camera_up を使い、前方向を計算する
    float3 right   = normalize(camera_right.xyz);
    float3 up      = normalize(camera_up.xyz);
    float3 forward = cross(right, up);  // 左手系: 右 x 上 = 後ろ方向、そのため反転させる
    

    // プロジェクション行列（列優先レイアウト）から、半幅と半高を復元する
    // VP[0][0] = (1/tan(fovX/2)) * (H/W), VP[1][1] = 1/tan(fovY/2)
    float inv_tan_fov_x = VP[0][0]; // X軸のスケール
    float inv_tan_fov_y = VP[1][1]; // Y軸のスケール

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
    float zenith_blend  = saturate(height * 1.5f);

    float3 horizon_color = sky_horizon_color.rgb;
    float3 zenith_color  = sky_zenith_color.rgb;
    float3 ground_color  = sky_ground_color.rgb;

    float3 sky = lerp(horizon_color, zenith_color, zenith_blend);
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

    return haze_color * horizon_factor * 0.6f;
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
    float above_horizon = smoothstep(-0.02f, 0.05f, sun_dir.y);

    float3 sun_contrib = sun_color.rgb * (disk * limb_darken + bloom * 0.5f);
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

    // 大気の靄（ヘイズ）
    sky += HorizonHaze(ray_dir, sun_dir);

    // 月
    sky += Moon(ray_dir, sun_dir, night_factor);

    // 太陽ディスク（地平線上にある場合のみ表示、薄明時は輝きへとフェードする）
    sky += SunDisk(ray_dir, sun_dir);

    return float4(sky, 1.0f);
}