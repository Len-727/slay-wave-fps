// ============================================================
//  BloodFluidCompositePS.hlsl
//  Gothic Swarm - 血液流体シミュレーション Pass 3: 合成パス
//
//  【役割】
//  血がある場所だけ α ブレンドで描画し、シーンに重ねる。
//  血がないピクセルは discard (シーンに触らない)。
//
//  【アプローチ】
//  CopyResource 不要 → パフォーマンス向上。
//  既存の描画結果の上に血を重ねるだけ。
// ============================================================
#include "Common.hlsli"

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// --- テクスチャ ---
Texture2D<float> BlurredDepth : register(t0);  // ブラー済み深度
SamplerState     PointSampler : register(s0);

// --- 定数バッファ ---
cbuffer CompositeCB : register(b0)
{
    float4x4 InvProjection;   // 逆プロジェクション行列
    float2   TexelSize;       // 1/width, 1/height
    float    Time;
    float    Thickness;       // 血の厚み (色の濃さに影響)
    float3   LightDir;
    float    SpecularPower;
    float3   CameraPos;
    float    FluidAlpha;      // 全体の透明度
};

// --- 血の色定数 ---
static const float3 FRESH_BLOOD     = float3(0.60f, 0.02f, 0.02f);
static const float3 DARK_BLOOD      = float3(0.25f, 0.01f, 0.01f);
static const float3 SPEC_COLOR      = float3(1.0f, 0.7f, 0.5f);
static const float3 FRESNEL_COLOR   = float3(0.8f, 0.15f, 0.05f);
static const float  SPEC_EXPONENT   = 32.0f;
static const float  FRESNEL_EXP     = 2.0f;
static const float  FRESNEL_SCALE   = 0.6f;
static const float  TONEMAP_SHOULDER = 0.3f;

// === 深度からビュー空間座標を復元 ===
float3 ReconstructViewPos(float2 uv, float depth)
{
    float4 clipPos;
    clipPos.x = uv.x * 2.0f - 1.0f;
    clipPos.y = (1.0f - uv.y) * 2.0f - 1.0f;
    clipPos.z = depth;
    clipPos.w = 1.0f;

    float4 viewPos = mul(clipPos, InvProjection);
    return viewPos.xyz / viewPos.w;
}

float4 main(PSInput input) : SV_Target
{
    float depth = BlurredDepth.SampleLevel(PointSampler, input.TexCoord, 0);

    // 血がないピクセルはスキップ
    if (depth <= 0.0f || depth >= 1.0f)
        discard;

    // === 周囲 4 ピクセルの深度から法線を復元 ===
    float depthL = BlurredDepth.SampleLevel(PointSampler, input.TexCoord + float2(-TexelSize.x, 0.0f), 0);
    float depthR = BlurredDepth.SampleLevel(PointSampler, input.TexCoord + float2( TexelSize.x, 0.0f), 0);
    float depthU = BlurredDepth.SampleLevel(PointSampler, input.TexCoord + float2(0.0f, -TexelSize.y), 0);
    float depthD = BlurredDepth.SampleLevel(PointSampler, input.TexCoord + float2(0.0f,  TexelSize.y), 0);

    float3 posC = ReconstructViewPos(input.TexCoord, depth);
    float3 posL = ReconstructViewPos(input.TexCoord + float2(-TexelSize.x, 0.0f), depthL);
    float3 posR = ReconstructViewPos(input.TexCoord + float2( TexelSize.x, 0.0f), depthR);
    float3 posU = ReconstructViewPos(input.TexCoord + float2(0.0f, -TexelSize.y), depthU);
    float3 posD = ReconstructViewPos(input.TexCoord + float2(0.0f,  TexelSize.y), depthD);

    float3 normal = normalize(cross(posR - posL, posD - posU));

    // === ライティング ===
    float3 lightDir = normalize(LightDir);
    float3 viewDir  = float3(0.0f, 0.0f, -1.0f);

    // ディフューズ (ハーフランバート)
    float diffuse = saturate(dot(normal, -lightDir)) * 0.5f + 0.5f;

    // スペキュラ (Blinn-Phong)
    float3 halfVec  = normalize(-lightDir + viewDir);
    float  specular = pow(saturate(dot(normal, halfVec)), SPEC_EXPONENT) * 2.0f;

    // フレネル
    float NdotV  = saturate(dot(normal, -viewDir));
    float fresnel = pow(1.0f - NdotV, FRESNEL_EXP) * FRESNEL_SCALE;

    // === 血の色 ===
    float  thickFactor = saturate(Thickness);
    float3 bloodColor  = lerp(FRESH_BLOOD, DARK_BLOOD, thickFactor * 0.5f);

    // === 最終合成 ===
    float3 litColor = bloodColor * diffuse
                    + SPEC_COLOR * specular
                    + FRESNEL_COLOR * fresnel;

    litColor = ReinhardTonemap(litColor, TONEMAP_SHOULDER);

    return float4(litColor, FluidAlpha);
}
