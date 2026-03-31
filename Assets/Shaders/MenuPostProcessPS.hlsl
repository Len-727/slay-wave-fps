// ============================================================
//  MenuPostProcessPS.hlsl
//  Gothic Swarm - メニュー画面ポストプロセス ピクセルシェーダー
//
//  【演出一覧】
//  1. 熱歪み (ヒートヘイズ)  → 鍛冶場の熱気で画面がゆらゆら
//  2. 暖色の色収差          → 炎の熱でにじむイメージ
//  3. 暖色ビネット          → 画面端をオレンジ暗く
//  4. ヒートブルーム        → 下部のオレンジグロー
//  5. スキャンライン        → 金属・無骨な質感
//  6. DOOM Eternal 風色調補正
//
//  【修正履歴】
//  - cbuffer の二重定義バグを修正
//  - NoiseLib.hlsli に関数を集約
// ============================================================
#include "Common.hlsli"
#include "NoiseLib.hlsli"

Texture2D    SceneTexture : register(t0);
Texture2D    NoiseTexture : register(t1);
SamplerState SamplerLinear : register(s0);

cbuffer MenuFXParams : register(b0)
{
    float Time;                // 経過時間
    float ShakeIntensity;      // シェイク強度 (0〜1)
    float VignetteIntensity;   // ビネット強度 (0〜1)
    float LightningIntensity;  // FX 全体の強度

    float ChromaticAmount;     // 色収差量
    float ScreenWidth;         // 画面幅
    float ScreenHeight;        // 画面高さ
    float Heartbeat;           // 脈動値 (0〜1)
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// ============================================================
//  熱歪み (ヒートヘイズ)
// ============================================================
static const float HEAT_NOISE_SCALE_X = 8.0f;
static const float HEAT_NOISE_SCALE_Y = 6.0f;
static const float HEAT_SCROLL_SPEED  = 0.8f;
static const float HEAT_BASE_STRENGTH = 0.006f;
static const float HEAT_SHAKE_MULT    = 2.0f;

float2 HeatDistortion(float2 uv, float t)
{
    float distX = ValueNoise(uv * HEAT_NOISE_SCALE_X + float2(t * 0.3f, -t * HEAT_SCROLL_SPEED)) - 0.5f;
    float distY = ValueNoise(uv * HEAT_NOISE_SCALE_Y + float2(-t * 0.2f, -t * 1.0f) + 50.0f) - 0.5f;

    // 画面下部ほど歪みが強い (熱源が下)
    float heatMask = smoothstep(0.0f, 0.8f, uv.y);
    float strength = HEAT_BASE_STRENGTH * (1.0f + ShakeIntensity * HEAT_SHAKE_MULT);

    return float2(distX, distY) * strength * heatMask;
}

// ============================================================
//  スキャンライン
// ============================================================
static const float SCAN_BAND_FREQ    = 40.0f;
static const float SCAN_BAND_SPEED   = 2.0f;
static const float SCAN_GLITCH_SPEED = 3.0f;

float Scanlines(float2 uv, float t)
{
    // 極細の横線
    float line1 = frac(uv.y * ScreenHeight * 0.5f);
    line1 = smoothstep(0.0f, 0.03f, line1) * smoothstep(0.06f, 0.03f, line1);

    // CRT 風バンド
    float band = sin(uv.y * SCAN_BAND_FREQ - t * SCAN_BAND_SPEED) * 0.5f + 0.5f;
    band = smoothstep(0.4f, 0.6f, band);

    // グリッチライン
    float glitchY = HashSin(float2(floor(t * SCAN_GLITCH_SPEED), 7.0f));
    float glitch  = 1.0f - smoothstep(0.0f, 0.005f, abs(uv.y - glitchY));
    glitch *= step(0.85f, HashSin(float2(floor(t * 2.0f), 13.0f)));

    return line1 * 0.03f + band * 0.02f + glitch * 0.15f;
}

// ============================================================
//  ヒートブルーム
// ============================================================
static const float3 BOTTOM_GLOW_COLOR = float3(0.12f, 0.03f, 0.005f);

float3 HeatBloom(float2 uv, float t)
{
    float bottomGlow = smoothstep(1.0f, 0.5f, uv.y);
    float glowNoise  = ValueNoise(float2(uv.x * 5.0f, t * 0.3f));
    bottomGlow *= glowNoise;

    float pulse = Heartbeat * Heartbeat;
    bottomGlow *= (0.6f + pulse * 0.4f);

    return BOTTOM_GLOW_COLOR * bottomGlow;
}

// ============================================================
//  メイン
// ============================================================

// --- 色調補正パラメータ ---
static const float3 COLOR_MULT     = float3(1.1f, 0.92f, 0.8f);
static const float3 CONTRAST_GAMMA = float3(1.05f, 1.05f, 1.08f);
static const float  BLOOM_LUM_THRESHOLD = 0.6f;
static const float  BLOOM_STRENGTH      = 0.5f;

// --- スキャンライン色 ---
static const float3 SCAN_COLOR = float3(0.8f, 0.4f, 0.1f);

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.TexCoord;
    float  t  = Time;

    // 1) 熱歪み
    float2 distortion  = HeatDistortion(uv, t);
    float2 distortedUV = uv + distortion;

    // 2) 暖色の色収差
    float2 center   = float2(0.5f, 0.5f);
    float2 dir      = distortedUV - center;
    float  dist     = length(dir);
    float2 caOffset = dir * dist * ChromaticAmount * 1.5f;

    float r = SceneTexture.Sample(SamplerLinear, distortedUV + caOffset * 1.2f).r;
    float g = SceneTexture.Sample(SamplerLinear, distortedUV + caOffset * 0.3f).g;
    float b = SceneTexture.Sample(SamplerLinear, distortedUV - caOffset * 0.5f).b;
    float a = SceneTexture.Sample(SamplerLinear, distortedUV).a;
    float3 color = float3(r, g, b);

    // 3) 暖色ビネット
    float vignette = 1.0f - dist * 1.3f;
    vignette = pow(saturate(vignette), 1.3f);
    float edgeDark = (1.0f - vignette) * VignetteIntensity;
    color *= vignette;
    color.r += edgeDark * 0.08f;
    color.g += edgeDark * 0.02f;

    // 4) ヒートブルーム
    color += HeatBloom(uv, t);

    // 5) スキャンライン
    float scan = Scanlines(uv, t);
    color += SCAN_COLOR * scan;

    // 6) 色調補正
    color *= COLOR_MULT;
    color = pow(color, CONTRAST_GAMMA);

    float lum = Luminance(color);
    color += color * max(lum - BLOOM_LUM_THRESHOLD, 0.0f) * BLOOM_STRENGTH;

    return float4(saturate(color), a);
}
