//六角形グリッドと対角ワイプ

Texture2D sceneTex : register(t0);
SamplerState samplerLinear : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer TransitionConstants : register(b0)
{
    float2 screen_size;
    float progress; // 0.0 -> 1.0
    float hex_size; // 六角形の大きさ(ピクセル)
    float2 direction; // ワイプ方向(正規化済み)
    float jitter; // 出現タイミングのランダム幅
    float edge_softness; // 出現境界のぼかし量
    float2 _pad;
};

// ポインティトップ六角形グリッド
float hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21)); // 乱数生成
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

// ポインティトップ六角形グリッド
float2 hexGrid(float2 p, out float2 id)
{
    const float2 s = float2(1.0, 1.7320508); // (1, sqrt(3))
    const float2 h = s * 0.5; // 六角形の半分のサイズ
    float2 a = fmod(p, s) - h; // 六角形の中心からのローカル座標
    float2 b = fmod(p - h, s) - h; // 六角形の中心からのローカル座標（オフセット）
    float2 gv = (dot(a, a) < dot(b, b)) ? a : b; // 六角形の中心からのローカル座標
    id = p - gv; // 六角形の代表座標（乱数シード・出現順の計算に使う）
    return gv;
}

// ポインティトップ六角形の距離場（0.5で境界になるよう正規化）
float hexDist(float2 p)
{
    p = abs(p);
    float c = dot(p, normalize(float2(1.0, 1.7320508))); // 六角形の距離場
    c = max(c, p.x);
    return c;
}

float4 main(PSInput input) : SV_TARGET
{
    float2 pixel = input.uv * screen_size;

    //画面中心基準にすることで上・左の半端な隙間をなくす
    const float2 s = float2(1.0, 1.7320508);
    const float2 halfcell = s * 0.5 * hex_size;
    float2 gp = (pixel + halfcell) / hex_size;

    float2 id;
    float2 gv = hexGrid(gp, id);
    float dist = hexDist(gv);

    float2 cellCenterPixel = id * hex_size - halfcell;
    float2 wipeDir = normalize(direction);
    float2 corners[4] =
    {
        float2(0, 0),
        float2(screen_size.x, 0),
        float2(0, screen_size.y),
        float2(screen_size.x, screen_size.y)
    };
    float wipeMin = dot(corners[0], wipeDir);
    float wipeMax = wipeMin;
    [unroll]
    for (int i = 1; i < 4; i++)
    {
        float d = dot(corners[i], wipeDir);
        wipeMin = min(wipeMin, d);
        wipeMax = max(wipeMax, d);
    }

    float wipeRaw = dot(cellCenterPixel, wipeDir);
    float wipe = saturate((wipeRaw - wipeMin) / (wipeMax - wipeMin)); // 0(起点)?1(終点)

    float rnd = hash21(id);
    float order = saturate(wipe + (rnd - 0.5) * jitter);

    const float startMargin = 0.15;
    float threshold = progress * (1.0 + jitter) - jitter * 0.5 - startMargin;
    float localT = smoothstep(order - edge_softness, order + edge_softness, threshold); // 0→1

    float3 sceneColor = sceneTex.Sample(samplerLinear, input.uv).rgb;

    //六角形の内接円が localT が進むほど縮んでいき、外側は黒になる
    float innerRadius = lerp(0.5 + edge_softness, 0.0, localT);
    float shrinkMask = 1.0 - smoothstep(innerRadius - 0.02, innerRadius, dist);

    
    float3 color = lerp(float3(0, 0, 0), sceneColor, shrinkMask);
    
    return float4(color, 1.0);
}