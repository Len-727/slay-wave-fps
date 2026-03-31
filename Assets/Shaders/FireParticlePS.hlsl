// ============================================================
//  FireParticlePS_Standalone.hlsl
//  Gothic Swarm - 炎パーティクル ピクセルシェーダー (Standalone)
//
//  【役割】
//  テクスチャ × パーティクルカラーで炎を描画。
//  中心から端に向かう放射フェードで自然なソフトパーティクル効果。
// ============================================================
#include "Common.hlsli"

Texture2D    ParticleTexture : register(t0);
SamplerState LinearSampler   : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
};

// --- ソフトパーティクルパラメータ ---
static const float SOFT_FADE_START = 0.5f;   // フェード開始距離
static const float SOFT_FADE_END   = 1.0f;   // フェード完了距離
static const float UV_CENTER       = 0.5f;

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor   = ParticleTexture.Sample(LinearSampler, input.TexCoord);
    float4 finalColor = texColor * input.Color;

    // 中心からの距離 (0〜1 に正規化)
    float2 centerDist    = input.TexCoord - UV_CENTER;
    float  distFromCenter = length(centerDist) * 2.0f;

    // 端に行くほど透明に (二乗で急峻にフェード)
    float alpha = 1.0f - smoothstep(SOFT_FADE_START, SOFT_FADE_END, distFromCenter);
    alpha *= alpha;

    finalColor.a *= alpha;
    return finalColor;
}
