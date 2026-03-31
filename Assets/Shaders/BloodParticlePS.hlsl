// ============================================================
//  BloodParticlePS.hlsl
//  Gothic Swarm - 血パーティクル ピクセルシェーダー (デュアルテクスチャ)
//
//  【2 つのテクスチャを同時使用】
//  t1 = 血の霧 (mist)    → 小さいパーティクル用
//  t2 = 血の液体 (splash) → 大きいパーティクル用
//  サイズに応じて smoothstep でブレンド。
// ============================================================
#include "Common.hlsli"

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
    float  Life     : TEXCOORD1;
    float  Size     : TEXCOORD2;
};

// --- テクスチャ (t0 は CS の StructuredBuffer が使用中) ---
Texture2D    MistTexture   : register(t1);  // 霧 (小粒用)
Texture2D    SplashTexture : register(t2);  // 液体 (大粒用)
SamplerState LinearSampler : register(s0);

// --- フリップブック設定 ---
static const float FLIPBOOK_GRID_X    = 8.0f;
static const float FLIPBOOK_GRID_Y    = 8.0f;
static const float FLIPBOOK_TOTAL     = 64.0f;  // GRID_X * GRID_Y

// --- サイズブレンド閾値 ---
static const float BLEND_SIZE_MIN     = 0.15f;  // これ以下 → splash 100%
static const float BLEND_SIZE_MAX     = 0.40f;  // これ以上 → mist 100%

// --- フォールバック用 ---
static const float FALLBACK_CORE_MIN  = 0.2f;
static const float FALLBACK_CORE_MAX  = 1.0f;

// --- 色の強度 ---
static const float COLOR_INTENSITY    = 4.0f;

// ============================================================
//  フリップブック UV 計算
//  寿命比率からアニメーションのフレーム番号を導出し、
//  グリッド上の UV 座標を返す。
// ============================================================
float2 CalcFlipbookUV(float2 localUV, float lifeRatio)
{
    float progress   = saturate(1.0f - lifeRatio);
    float frameFloat = progress * (FLIPBOOK_TOTAL - 1.0f);
    uint  frameIndex = (uint)frameFloat;

    uint col = frameIndex % (uint)FLIPBOOK_GRID_X;
    uint row = frameIndex / (uint)FLIPBOOK_GRID_X;

    return float2(
        ((float)col + localUV.x) / FLIPBOOK_GRID_X,
        ((float)row + localUV.y) / FLIPBOOK_GRID_Y
    );
}

float4 main(PSInput input) : SV_Target
{
    // --- フリップブック UV ---
    float2 flipUV = CalcFlipbookUV(input.TexCoord, input.Life);

    // --- 両テクスチャからサンプリング ---
    float4 mistColor   = MistTexture.Sample(LinearSampler, flipUV);
    float4 splashColor = SplashTexture.Sample(LinearSampler, flipUV);

    // --- サイズに基づくブレンド ---
    float  blendFactor = smoothstep(BLEND_SIZE_MIN, BLEND_SIZE_MAX, input.Size);
    float4 texColor    = lerp(splashColor, mistColor, blendFactor);

    // --- テクスチャなしフォールバック (円形パーティクル) ---
    if (texColor.a < ALPHA_CUTOFF)
    {
        float2 centered = input.TexCoord * 2.0f - 1.0f;
        float  distSq   = dot(centered, centered);

        if (distSq > 1.0f)
            discard;

        float coreFade    = smoothstep(FALLBACK_CORE_MAX, FALLBACK_CORE_MIN, sqrt(distSq));
        float alpha       = input.Color.a * coreFade;
        float3 fallback   = input.Color.rgb * (0.7f + 0.3f * coreFade);
        return float4(fallback, alpha);
    }

    // --- 最終出力 ---
    float3 finalColor = texColor.rgb * input.Color.rgb * COLOR_INTENSITY;
    float  finalAlpha = texColor.a * input.Color.a;

    return float4(finalColor, finalAlpha);
}
