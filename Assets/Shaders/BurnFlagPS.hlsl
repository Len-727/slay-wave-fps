// ============================================================
//  BurnFlagPS.hlsl
//  Gothic Swarm - 旗燃焼エフェクト ピクセルシェーダー
//
//  【演出フロー】
//  1. burnProgress (0→1) に応じて旗が下から上へ燃え広がる
//  2. 燃え際 (エンバー) は DOOM 風の黒→赤→オレンジ→黄→白グラデーション
//  3. 焦げ始めゾーンでオレンジの縁光
//  4. 未燃焼部分にも微かな熱の影響
// ============================================================
#include "Common.hlsli"
#include "NoiseLib.hlsli"

Texture2D    FlagTexture  : register(t0);
Texture2D    NoiseTexture : register(t1);
SamplerState SamplerLinear : register(s0);

cbuffer BurnParams : register(b0)
{
    float BurnProgress;   // 0.0 = 未燃焼, 1.0 = 完全燃焼
    float Time;
    float EmberWidth;     // 燃え際の幅 (0.05〜0.15 推奨)
    float BurnDirection;  // 予備
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// --- 燃焼方向バイアス ---
static const float DIR_BIAS_Y     = 0.3f;   // 下→上方向の重み
static const float DIR_BIAS_X     = 0.2f;   // 左→右方向の重み
static const float NOISE_WEIGHT   = 0.7f;   // ノイズの寄与率

// --- 進行度の拡張 ---
static const float BURN_EXTEND    = 1.3f;

// --- エンバーカラー (5段階グラデーション) ---
static const float3 EMBER_COLOR_0 = float3(0.1f,  0.02f, 0.0f);   // 焦げ始め
static const float3 EMBER_COLOR_1 = float3(0.5f,  0.08f, 0.0f);   // 暗い赤
static const float3 EMBER_COLOR_2 = float3(1.0f,  0.35f, 0.02f);  // オレンジ
static const float3 EMBER_COLOR_3 = float3(1.0f,  0.7f,  0.1f);   // 黄色
static const float3 EMBER_COLOR_4 = float3(1.0f,  0.9f,  0.5f);   // 白熱
static const float3 EMBER_COLOR_5 = float3(1.0f,  0.95f, 0.7f);   // コア

// --- エンバーのグラデーション閾値 ---
static const float EMBER_T0 = 0.15f;
static const float EMBER_T1 = 0.35f;
static const float EMBER_T2 = 0.60f;
static const float EMBER_T3 = 0.85f;

// --- 焦げゾーン ---
static const float CHAR_WIDTH  = 0.06f;
static const float3 CHAR_COLOR = float3(0.2f, 0.08f, 0.02f);

// --- 火花 ---
static const float3 SPARK_COLOR     = float3(1.0f, 0.8f, 0.3f);
static const float  SPARK_THRESHOLD = 0.97f;
static const float  SPARK_GRID      = 200.0f;
static const float  SPARK_SPEED     = 8.0f;
static const float  SPARK_INTENSITY = 1.5f;

// --- フリッカー ---
static const float FLICKER_SPEED_1  = 20.0f;
static const float FLICKER_SPEED_2  = 35.0f;

float4 main(PSInput input) : SV_TARGET
{
    // === テクスチャサンプリング ===
    float4 flagColor = FlagTexture.Sample(SamplerLinear, input.TexCoord);

    float2 noiseUV = input.TexCoord + float2(sin(Time * 0.5f) * 0.01f,
                                              cos(Time * 0.3f) * 0.01f);
    float  noise   = NoiseTexture.Sample(SamplerLinear, noiseUV).r;

    // === 燃焼方向バイアス ===
    float dirBias = (1.0f - input.TexCoord.y) * DIR_BIAS_Y
                  + input.TexCoord.x * DIR_BIAS_X;
    float burnValue = noise * NOISE_WEIGHT + dirBias;

    // === 燃焼閾値 ===
    float burnThreshold = BurnProgress * BURN_EXTEND;
    float emberStart    = burnThreshold - EmberWidth;
    float emberEnd      = burnThreshold;

    // === 領域判定と色計算 ===
    float4 finalColor;

    if (burnValue < emberStart)
    {
        // --- 完全燃焼 → 透明 ---
        finalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else if (burnValue < emberEnd)
    {
        // --- エンバー (燃え際): DOOM 風グラデーション ---
        float emberT = (burnValue - emberStart) / EmberWidth;

        // フリッカー
        float flicker = sin(Time * FLICKER_SPEED_1 + noise * 25.0f) * 0.15f + 0.85f;
        flicker += sin(Time * FLICKER_SPEED_2 + noise * 15.0f) * 0.08f;

        // 5 段階カラーグラデーション
        float3 emberColor;
        if (emberT < EMBER_T0)
            emberColor = lerp(EMBER_COLOR_0, EMBER_COLOR_1, emberT / EMBER_T0);
        else if (emberT < EMBER_T1)
            emberColor = lerp(EMBER_COLOR_1, EMBER_COLOR_2, (emberT - EMBER_T0) / (EMBER_T1 - EMBER_T0));
        else if (emberT < EMBER_T2)
            emberColor = lerp(EMBER_COLOR_2, EMBER_COLOR_3, (emberT - EMBER_T1) / (EMBER_T2 - EMBER_T1));
        else if (emberT < EMBER_T3)
            emberColor = lerp(EMBER_COLOR_3, EMBER_COLOR_4, (emberT - EMBER_T2) / (EMBER_T3 - EMBER_T2));
        else
            emberColor = lerp(EMBER_COLOR_4, EMBER_COLOR_5, (emberT - EMBER_T3) / (1.0f - EMBER_T3));

        emberColor *= flicker;

        // 発光 (HDR 風)
        float glow = pow(emberT, 1.5f) * 2.0f;
        emberColor += float3(glow * 0.4f, glow * 0.15f, glow * 0.02f);

        // 火花
        float spark = HashSin(floor(input.TexCoord * SPARK_GRID) + floor(Time * SPARK_SPEED));
        spark = step(SPARK_THRESHOLD, spark) * step(0.3f, emberT);
        emberColor += SPARK_COLOR * spark * SPARK_INTENSITY;

        finalColor = float4(saturate(emberColor), 1.0f);
    }
    else if (burnValue < emberEnd + CHAR_WIDTH)
    {
        // --- 焦げ始め ---
        float charT = (burnValue - emberEnd) / CHAR_WIDTH;
        float3 charColor = lerp(CHAR_COLOR, flagColor.rgb * 0.6f, charT);

        float edgeGlow = (1.0f - charT) * 0.3f;
        charColor += float3(edgeGlow, edgeGlow * 0.3f, 0.0f);

        finalColor = float4(charColor, flagColor.a);
    }
    else
    {
        // --- 未燃焼 (微かな熱の影響) ---
        float heatProximity = smoothstep(emberEnd + 0.2f, emberEnd + CHAR_WIDTH, burnValue);
        float3 heated = flagColor.rgb + float3(0.05f, 0.015f, 0.0f) * heatProximity;

        finalColor = float4(heated, flagColor.a);
    }

    return finalColor;
}
