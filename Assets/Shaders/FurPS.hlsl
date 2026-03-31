// ============================================================
//  FurPS.hlsl
//  Gothic Swarm - ファーシェーダー (苔・草用) ピクセルシェーダー
//
//  【方式】
//  ノイズベースのストランド判定 + シャープな境界で自然な苔を表現。
//  FBM を 2 スケール合成し、先端ほどまばらに discard する。
// ============================================================
#include "Common.hlsli"
#include "NoiseLib.hlsli"

cbuffer FurParams : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;

    float  FurLength;
    float  CurrentLayer;
    float  TotalLayers;
    float  Time;

    float3 WindDirection;
    float  WindStrength;

    float3 BaseColor;
    float  FurDensity;
    float3 TipColor;
    float  Padding2;

    float3 LightDir;
    float  AmbientStrength;
};

struct PSInput
{
    float4 Position    : SV_POSITION;
    float2 TexCoord    : TEXCOORD0;
    float  Layer       : TEXCOORD1;
    float3 WorldPos    : TEXCOORD2;
    float3 WorldNormal : TEXCOORD3;
};

// --- ストランド判定パラメータ ---
static const float FINE_SCALE      = 60.0f;    // 細かいノイズのスケール
static const float COARSE_SCALE    = 8.0f;     // 粗いノイズのスケール
static const float2 FINE_OFFSET    = float2(3.7f, 1.2f);
static const float2 COARSE_OFFSET  = float2(9.1f, 4.3f);
static const float FINE_WEIGHT     = 0.7f;
static const float COARSE_WEIGHT   = 0.3f;
static const float EDGE_SHARPNESS  = 0.05f;    // smoothstep の幅

// --- 色バリエーション ---
static const float COLOR_VAR_SCALE = 45.0f;
static const float COLOR_VAR_MIN   = 0.65f;
static const float COLOR_VAR_RANGE = 0.7f;
static const float PATCH_MIN       = 0.55f;
static const float PATCH_RANGE     = 0.65f;

// --- セルフシャドウ ---
static const float SELF_SHADOW_MIN = 0.15f;

// --- アルファ ---
static const float ALPHA_LAYER_DECAY = 0.5f;

float4 main(PSInput input) : SV_TARGET
{
    float layer = input.Layer;

    // === ストランド判定 ===
    float fine   = FBM3(input.TexCoord * FINE_SCALE   + FINE_OFFSET);
    float coarse = FBM3(input.TexCoord * COARSE_SCALE + COARSE_OFFSET);
    float furNoise = fine * FINE_WEIGHT + coarse * COARSE_WEIGHT;

    // 先端ほどまばら
    float threshold = FurDensity * (1.0f - layer * layer);
    float strandMask = 1.0f - smoothstep(threshold - EDGE_SHARPNESS,
                                          threshold + EDGE_SHARPNESS, furNoise);

    if (strandMask < EDGE_SHARPNESS)
        discard;

    // === 色計算 ===
    float3 furColor = lerp(BaseColor, TipColor, layer);

    // 色バリエーション (ValueNoise で軽量)
    float colorVar = ValueNoise(input.TexCoord * COLOR_VAR_SCALE + 7.1f);
    furColor *= (COLOR_VAR_MIN + colorVar * COLOR_VAR_RANGE);

    // パッチ (coarse を再利用)
    float patch = smoothstep(0.3f, 0.7f, coarse);
    furColor *= (PATCH_MIN + patch * PATCH_RANGE);

    // === ライティング ===
    float3 normal  = normalize(input.WorldNormal);
    float3 ld      = normalize(-LightDir);
    float  diffuse = max(dot(normal, ld), 0.0f);

    float selfShadow = lerp(SELF_SHADOW_MIN, 1.0f, layer);
    float lighting   = AmbientStrength + diffuse * selfShadow;
    furColor *= lighting;

    // === アルファ ===
    float alpha = strandMask * (1.0f - layer * ALPHA_LAYER_DECAY);
    return float4(furColor, saturate(alpha));
}
