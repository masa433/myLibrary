cbuffer BurstBuffer : register(b0)
{
    float time; // 時間
    float aspectRatio; // アスペクト比
    float progress; // 進行度
    float padding; // パディング
};

#define SPEED (1.0f / 80.0f) // 速度の定義
#define SMOOTH_DIST 0.6f // スムーズな距離の定義
#define PI 3.14159265359f // 円周率の定義

struct PS_IN
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

//疑似乱数生成関数
float hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

// 2Dノイズ関数
float valueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float a = hash21(i);
    float b = hash21(i + float2(1.0, 0.0));
    float c = hash21(i + float2(0.0, 1.0));
    float d = hash21(i + float2(1.0, 1.0));
    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

//オクターブを重ねて自然な揺らぎにする
float fbm(float2 p)
{
    float value = 0.0; // 初期値を0に設定
    float amplitude = 0.5; // 振幅を0.5に設定
    for(int i = 0; i < 5; i++)
    {
        value += amplitude * valueNoise(p); // ノイズ値を加算
        p *= 2.0; // 周波数を2倍にする
        amplitude *= 0.5; // 振幅を半分にする
    }
    return value;
}

//RGB3チャンネル分、それぞれ異なるノイズを生成する関数
float3 noiseTexLookup(float2 uv)
{
    float r = fbm(uv + float2(0.0, 0.0));
    float g = fbm(uv + float2(31.7, 11.3));
    float b = fbm(uv + float2(71.2, 53.9));
    return float3(r, g, b);
}

float4 main(PS_IN input) : SV_TARGET
{
    float2 uv = input.texcoord - float2(0.5, 0.5); // 中心を原点にする
    uv.x *= aspectRatio; // アスペクト比を考慮する
    
    float dist = length(uv); // 中心からの距離を計算
    float angle = (atan2(uv.y, uv.x) + PI) / (2.0 * PI); // 中心からの角度を計算
    
    //角度方向にスクロールするノイズ
    float3 textureDist = noiseTexLookup(float2(time * SPEED * 20.0f, angle * 6.0f)); // ノイズテクスチャを取得
    
    //見た目の揺らぎを作るために、ノイズテクスチャを使って距離を変化させる
    float normalTex = noiseTexLookup(uv * 0.5f);
    
    textureDist = textureDist * 0.4f + 0.5f; // ノイズの値を調整
    
    float3 color = float3(0.0, 0.0, 0.0); // 初期色を黒に設定
    
    if(dist < textureDist.x)
        color.x += smoothstep(0.0, SMOOTH_DIST, textureDist.x - dist); // 赤チャンネルの色を計算
    if(dist < textureDist.y)
        color.y += smoothstep(0.0, SMOOTH_DIST, textureDist.y - dist); // 緑チャンネルの色を計算
    if(dist < textureDist.z)
        color.z += smoothstep(0.0, SMOOTH_DIST, textureDist.z - dist); // 青チャンネルの色を計算
    
    float3 finalColor = color + normalTex * 0.3f; // ノイズを加えて最終色を計算
    
    ////オレンジから金色へのグラデーションを作る
    //float brightness = (finalColor.x + finalColor.y + finalColor.z) / 3.0f;
    //// 明るさに応じて色を補間する
    //float3 fireColor = lerp(float3(0.35f, 0.05f, 0.0f), float3(1.0f, 0.85f, 0.4f), saturate(brightness * 1.5f));
    //finalColor = fireColor * saturate(brightness * 2.0f); // 明るさに応じて色を補間する
    
    //float alpha = saturate(brightness * 2.0f); // 進行度に応じてアルファ値を計算
    float alpha = saturate(max(finalColor.x, max(finalColor.y, finalColor.z))); // RGBの最大値をアルファ値として使用
    
    float radialFade = 1.0f - smoothstep(0.35f, 0.5f, dist); // 中心からの距離に応じてフェードアウトする
    alpha *= radialFade; // アルファ値にフェードアウトを適用する
    
    finalColor *= progress; // 進行度に応じて色を調整する
    alpha *= progress; // 進行度に応じてアルファ値を調整する
    
    return float4(finalColor, alpha);
};