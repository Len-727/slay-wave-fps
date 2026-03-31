// ============================================================
//  TitleBackgroundPS.hlsl
//  Gothic Swarm - タイトル画面背景 ピクセルシェーダー
//
//  【演出一覧】
//  0. 熱歪み (UV ワープ)
//  1. 溶岩地面 (ダークベース + メタルノイズ)
//  2. 溶岩亀裂 (Voronoi + 脈動)
//  3. 炎柱 (下から立ち上がる)
//  4. 下部ヒートグロー + 溶岩プール
//  5. 煙・灰 (上部)
//  6. エンバー (火の粉: ノイズベース 8 レイヤー)
//  7. ランダムフラッシュ
//  8. ビネット
//  9. DOOM Eternal 風色調補正
//
//  【リファクタリング】
//  - NoiseLib.hlsli に hash/noise/fbm/warpedFbm を集約
//  - マジックナンバーを static const に整理
// ============================================================
#include "Common.hlsli"
#include "NoiseLib.hlsli"

SamplerState SamplerLinear : register(s0);

cbuffer TitleBGParams : register(b0)
{
    float Time;
    float ScreenWidth;
    float ScreenHeight;
    float Intensity;    // フェードイン用 (0→1)
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// ============================================================
//  溶岩亀裂 (Voronoi ベース)
// ============================================================
static const float CRACK_LINE_WIDTH = 0.12f;
static const float CRACK_CORE_WIDTH = 0.03f;
static const float CRACK_CORE_BOOST = 0.5f;
static const float CRACK_CELL_DRIFT = 0.12f;

float LavaCracks(float2 uv, float t, float scale)
{
    float2 p = uv * scale;

    float minDist    = 1.0f;
    float secondDist = 1.0f;

    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 cell   = float2((float)x, (float)y);
            float2 cellId = floor(p) + cell;

            float2 cellCenter = cell + float2(
                Hash21(cellId) * 0.8f + sin(t * 0.2f + Hash21(cellId) * TWO_PI) * CRACK_CELL_DRIFT,
                Hash21(cellId + 100.0f) * 0.8f + cos(t * 0.18f + Hash21(cellId + 100.0f) * TWO_PI) * CRACK_CELL_DRIFT
            );

            float d = length(frac(p) - cellCenter);
            if (d < minDist)
            {
                secondDist = minDist;
                minDist = d;
            }
            else if (d < secondDist)
            {
                secondDist = d;
            }
        }
    }

    float crack = 1.0f - smoothstep(0.0f, CRACK_LINE_WIDTH, secondDist - minDist);
    float core  = 1.0f - smoothstep(0.0f, CRACK_CORE_WIDTH, secondDist - minDist);
    return saturate(crack + core * CRACK_CORE_BOOST);
}

// ============================================================
//  マグマ脈動
// ============================================================
float MagmaPulse(float2 uv, float t)
{
    float warp   = WarpedFBM(uv * 2.5f, t);
    float pulse1 = sin(t * 1.2f + warp * 8.0f) * 0.5f + 0.5f;
    float pulse2 = sin(t * 0.7f + warp * 5.0f + 2.0f) * 0.5f + 0.5f;
    float pulse3 = sin(t * 2.0f + warp * 3.0f + 4.0f) * 0.5f + 0.5f;
    float pulse  = pulse1 * 0.5f + pulse2 * 0.3f + pulse3 * 0.2f;
    return pow(pulse, 1.5f) * warp;
}

// ============================================================
//  エンバー (火の粉) - 単一レイヤー
// ============================================================
float3 EmberLayer(float2 uv, float t, float scrollSpeed, float noiseScale,
                  float threshold, float brightness, float3 tintA, float3 tintB)
{
    float2 scrollUV = uv * noiseScale;
    scrollUV.y -= t * scrollSpeed;
    scrollUV.x += sin(t * 0.4f + uv.y * 3.0f) * 0.3f;

    float n     = FBM7(scrollUV);
    float spark = saturate((n - threshold) / (1.0f - threshold));
    spark = pow(spark, 3.0f);

    float  colorMix  = frac(n * 7.31f + scrollUV.x * 0.1f);
    float3 sparkColor = lerp(tintA, tintB, colorMix);

    // 白熱コア
    float coreHeat = saturate((n - (threshold + 0.03f)) / 0.02f);
    sparkColor = lerp(sparkColor, float3(1.0f, 0.95f, 0.8f), pow(coreHeat, 2.0f) * 0.7f);

    float heightFade = smoothstep(0.0f, 0.8f, uv.y);
    float flicker    = 0.7f + 0.3f * sin(t * 8.0f + n * 50.0f);

    return sparkColor * spark * brightness * heightFade * flicker;
}

// ============================================================
//  エンバー合成 (8 レイヤー)
// ============================================================
static const float3 PAL_RED_ORANGE   = float3(1.0f, 0.1f,  0.02f);
static const float3 PAL_YELLOW_ORANGE = float3(1.0f, 0.2f,  0.05f);
static const float3 PAL_BRIGHT_YELLOW = float3(1.0f, 0.35f, 0.1f);
static const float3 PAL_DEEP_RED     = float3(0.7f, 0.02f, 0.0f);

float3 Embers(float2 uv, float t)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    result += EmberLayer(uv,                     t,        0.15f, 8.0f,  0.72f, 0.6f,  PAL_RED_ORANGE,    PAL_YELLOW_ORANGE);
    result += EmberLayer(uv,                     t + 10.0f, 0.25f, 14.0f, 0.75f, 0.5f,  PAL_YELLOW_ORANGE, PAL_BRIGHT_YELLOW);
    result += EmberLayer(uv + float2(3.7f,1.2f), t + 20.0f, 0.20f, 16.0f, 0.76f, 0.45f, PAL_DEEP_RED,     PAL_RED_ORANGE);
    result += EmberLayer(uv + float2(7.1f,5.3f), t + 30.0f, 0.40f, 24.0f, 0.78f, 0.4f,  PAL_YELLOW_ORANGE, PAL_BRIGHT_YELLOW);
    result += EmberLayer(uv + float2(13.5f,8.9f),t + 40.0f, 0.35f, 28.0f, 0.79f, 0.35f, PAL_RED_ORANGE,    PAL_YELLOW_ORANGE);
    result += EmberLayer(uv + float2(21.0f,3.4f),t + 50.0f, 0.55f, 40.0f, 0.80f, 0.3f,  PAL_BRIGHT_YELLOW, float3(1.0f,0.95f,0.7f));
    result += EmberLayer(uv + float2(31.7f,11.2f),t+ 60.0f, 0.50f, 45.0f, 0.81f, 0.25f, PAL_YELLOW_ORANGE, PAL_BRIGHT_YELLOW);
    result += EmberLayer(uv + float2(47.3f,19.8f),t+ 70.0f, 0.70f, 60.0f, 0.83f, 0.2f,  PAL_BRIGHT_YELLOW, float3(1.0f,1.0f,0.9f));
    return result;
}

// ============================================================
//  炎柱
// ============================================================
static const int   FIRE_COLUMN_COUNT = 4;
static const float FIRE_COLUMN_BASE_WIDTH = 0.03f;

float FireColumns(float2 uv, float t)
{
    float result = 0.0f;
    [unroll]
    for (int i = 0; i < FIRE_COLUMN_COUNT; i++)
    {
        float fi     = (float)i;
        float offset = fi * 0.25f + 0.1f;
        float colX   = offset + sin(t * 0.3f + fi * 2.0f) * 0.08f;
        float distX  = abs(uv.x - colX);
        float width  = FIRE_COLUMN_BASE_WIDTH + sin(t * 1.5f + fi * 3.0f + uv.y * 8.0f) * 0.015f;
        float column = 1.0f - smoothstep(0.0f, width, distX);

        float yFade  = smoothstep(1.0f, 0.3f, uv.y);
        float fNoise = ValueNoise(float2(uv.x * 10.0f, uv.y * 5.0f - t * 3.0f + fi * 10.0f));
        result += column * yFade * fNoise;
    }
    return saturate(result);
}

// ============================================================
//  メイン
// ============================================================

// --- 溶岩の色 ---
static const float3 LAVA_DARK   = float3(0.5f,  0.02f, 0.02f);
static const float3 LAVA_MID    = float3(0.85f, 0.08f, 0.03f);
static const float3 LAVA_BRIGHT = float3(1.0f,  0.2f,  0.1f);

// --- 色調補正 ---
static const float3 TITLE_COLOR_MULT     = float3(1.3f, 0.55f, 0.45f);
static const float3 TITLE_CONTRAST_GAMMA = float3(1.1f, 1.15f, 1.2f);
static const float  TITLE_BLOOM_THRESH   = 0.5f;
static const float  TITLE_BLOOM_STRENGTH = 0.8f;

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.TexCoord;
    float  t  = Time;

    // 0) 熱歪み
    float2 distortion;
    distortion.x = ValueNoise(uv * 5.0f + float2(t * 0.5f, 0.0f)) - 0.5f;
    distortion.y = ValueNoise(uv * 5.0f + float2(0.0f, t * 0.4f)) - 0.5f;
    float2 warpedUV = uv + distortion * 0.008f;

    // 1) ベース
    float metalNoise  = FBM7(warpedUV * 8.0f + 5.0f);
    float darkPattern = WarpedFBM(warpedUV * 1.5f, t * 0.5f);
    float3 base = float3(0.05f, 0.01f, 0.01f);
    base += float3(0.03f, 0.008f, 0.005f) * metalNoise;
    base += float3(0.025f, 0.005f, 0.003f) * darkPattern;

    // 2) 溶岩亀裂
    float crack1     = LavaCracks(warpedUV, t, 3.5f);
    float crack2     = LavaCracks(warpedUV, t * 1.3f + 100.0f, 6.0f);
    float totalCrack = saturate(crack1 + crack2 * 0.5f);
    float pulse      = MagmaPulse(warpedUV, t);

    float  lavaInt   = 0.7f + pulse * 0.5f;
    float3 lavaColor = lerp(LAVA_DARK, LAVA_MID, pulse);
    lavaColor = lerp(lavaColor, LAVA_BRIGHT, totalCrack * pulse * 0.5f);
    base += lavaColor * totalCrack * lavaInt;

    float bloom = LavaCracks(warpedUV * 0.95f + 0.025f, t, 3.0f);
    base += float3(0.25f, 0.02f, 0.02f) * bloom * pulse * 0.4f;

    // 3) 炎柱
    float columns = FireColumns(warpedUV, t);
    float3 fireColor = lerp(float3(0.6f, 0.03f, 0.0f),
                             float3(1.0f, 0.15f, 0.05f), 1.0f - warpedUV.y);
    base += fireColor * columns * 0.8f;

    // 4) 下部ヒートグロー
    float bottomHeat = smoothstep(1.0f, 0.35f, uv.y);
    float heatNoise  = FBM7(float2(uv.x * 8.0f, t * 0.2f));
    float heatPulse  = sin(t * 1.0f) * 0.15f + 0.85f;
    bottomHeat *= (0.6f + heatNoise * 0.4f) * heatPulse;
    base += float3(0.4f, 0.03f, 0.02f) * bottomHeat;

    // 溶岩プール
    float lavaPool  = smoothstep(1.0f, 0.85f, uv.y);
    float poolNoise = FBM7(float2(uv.x * 4.0f + t * 0.3f, t * 0.15f));
    base += float3(0.5f, 0.05f, 0.03f) * lavaPool * poolNoise;

    // 5) 煙 + 灰
    float smoke1 = FBM7(uv * 3.0f + float2(t * 0.04f, -t * 0.03f));
    float smoke2 = FBM7(uv * 2.0f + float2(-t * 0.03f, t * 0.05f) + 20.0f);
    float combined = (smoke1 + smoke2) * 0.5f;
    float smokeMask = smoothstep(0.5f, 0.0f, uv.y);
    float3 smokeColor = float3(0.06f, 0.015f, 0.015f) + float3(0.05f, 0.01f, 0.008f) * combined;
    base += smokeColor * combined * smokeMask * 0.6f;

    // 6) エンバー
    base += Embers(uv, t);

    // 7) フラッシュ
    float flash = Hash21(float2(floor(t * 0.8f), 42.0f));
    flash = pow(max(flash - 0.92f, 0.0f) * 12.5f, 2.0f);
    base += float3(0.2f, 0.02f, 0.02f) * flash;

    // 8) ビネット
    float2 center  = uv - float2(0.5f, 0.45f);
    float  vig     = 1.0f - dot(center, center) * 1.6f;
    vig = lerp(saturate(vig), 1.0f, smoothstep(0.7f, 1.0f, uv.y) * 0.5f);
    base *= vig;

    // 9) 色調補正
    base *= TITLE_COLOR_MULT;
    base = pow(base, TITLE_CONTRAST_GAMMA);
    float lum = Luminance(base);
    base += base * max(lum - TITLE_BLOOM_THRESH, 0.0f) * TITLE_BLOOM_STRENGTH;

    base *= Intensity;
    return float4(saturate(base), 1.0f);
}
