// ============================================================
//  SkinnedPS.hlsl
//  Gothic Swarm - スキンメッシュ ピクセルシェーダー
//
//  【役割】
//  テクスチャ × ライティング済み頂点カラーで基本色を決定。
//  スタガー状態の敵にはリムライト (DOOM Eternal 風の紫発光) を適用。
//
//  【スタガーフラグ (Color.a) の仕様】
//  0.0〜1.5: 通常 (リムなし)
//  2.0:      NORMAL / RUNNER  → rimPower = 3.0
//  3.0:      TANK             → rimPower = 4.5
//  4.0:      MIDBOSS          → rimPower = 6.0
//  5.0:      BOSS             → rimPower = 8.0
//  小数部 (frac): 脈動のフェーズ
// ============================================================
#include "Common.hlsli"

Texture2D    DiffuseTexture : register(t0);
SamplerState LinearSampler  : register(s0);

cbuffer LightBuffer : register(b2)
{
    float4 AmbientColor;
    float4 DiffuseColor;
    float3 LightDirection;
    float  Padding;
    float3 CameraPos;
    float  Padding2;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
    float3 WorldPos : TEXCOORD1;
};

// --- スタガー判定閾値 ---
static const float STAGGER_THRESHOLD = 1.5f;

// --- リムライト: 敵タイプ別 rimPower (大きい敵ほど細いリム) ---
static const float RIM_POWER_NORMAL  = 3.0f;
static const float RIM_POWER_TANK    = 4.5f;
static const float RIM_POWER_MIDBOSS = 6.0f;
static const float RIM_POWER_BOSS    = 8.0f;

// --- リムライトの色と強度 ---
static const float3 RIM_COLOR    = float3(0.6f, 0.05f, 1.0f);  // 紫
static const float  RIM_STRENGTH = 1.2f;

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor  = DiffuseTexture.Sample(LinearSampler, input.TexCoord);
    float4 baseColor = texColor * float4(input.Color.rgb, 1.0f);

    // === スタガーリムライト ===
    float rimFlag    = input.Color.a;
    bool  isStaggered = (rimFlag > STAGGER_THRESHOLD);

    if (isStaggered)
    {
        float3 viewDir = normalize(CameraPos - input.WorldPos);
        float3 normal  = normalize(input.Normal);
        float  NdotV   = max(abs(dot(normal, viewDir)), 0.05f);
        float  rim     = 1.0f - NdotV;

        // 敵タイプ別の rimPower
        float rimPower = RIM_POWER_NORMAL;
        if (rimFlag > 4.5f)
            rimPower = RIM_POWER_BOSS;
        else if (rimFlag > 3.5f)
            rimPower = RIM_POWER_MIDBOSS;
        else if (rimFlag > 2.5f)
            rimPower = RIM_POWER_TANK;

        rim = pow(rim, rimPower);

        // 脈動 (小数部をフェーズとして使用)
        float phase = frac(rimFlag);
        float pulse = sin(phase * TWO_PI) * 0.25f + 0.75f;

        baseColor.rgb += RIM_COLOR * rim * RIM_STRENGTH * pulse;
    }

    clip(baseColor.a - 0.1f);
    baseColor.a = texColor.a;

    return baseColor;
}
