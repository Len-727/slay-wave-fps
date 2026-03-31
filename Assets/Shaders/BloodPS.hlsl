// ============================================================
//  BloodPS.hlsl
//  Gothic Swarm - 血パーティクル／デカール ピクセルシェーダー
//
//  【3 つの核心技術】
//  1. SSS 風の厚み表現 → 中心=黒赤、端=鮮血で「液体の厚み」を演出
//  2. スペキュラ反射    → 超低ラフネスで「濡れたテカテカ感」
//  3. フレネル効果      → 端ほど反射が強い「液体の表面張力」表現
// ============================================================
#include "Common.hlsli"

// --- 定数バッファ（BloodVS と共有）---
cbuffer BloodCB : register(b0)
{
    float4x4 WorldViewProj;
    float3   CameraPos;
    float    Time;
    float3   LightDir;
    float    ScreenMode;  // 0.0 = 3D デカール / 1.0 = 2D スクリーンブラッド
};

// --- テクスチャ ---
Texture2D    BloodTex     : register(t0);  // 血スプラッターテクスチャ
SamplerState BloodSampler : register(s0);

// --- PS 入力（BloodVS の出力に対応）---
struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

// ============================================================
//  血の色定数（マジックナンバー排除）
// ============================================================
static const float3 DARK_BLOOD_3D    = float3(0.10f, 0.005f, 0.005f);  // 厚い部分（ほぼ黒）
static const float3 BRIGHT_BLOOD_3D  = float3(0.60f, 0.030f, 0.000f);  // 薄い部分（鮮血）
static const float3 DARK_BLOOD_2D    = float3(0.12f, 0.005f, 0.005f);
static const float3 BRIGHT_BLOOD_2D  = float3(0.55f, 0.020f, 0.000f);

// ライティングパラメータ
static const float SPEC_POWER_SHARP = 196.0f;  // 鋭い鏡面反射（濡れ感）
static const float SPEC_POWER_BROAD =  16.0f;  // 広い反射（ぬめり感）
static const float FRESNEL_EXPONENT =   4.0f;  // フレネルの鋭さ
static const float TONEMAP_SHOULDER  =   0.5f;  // トーンマップ強度

float4 main(PSInput input) : SV_TARGET
{
    // ==== テクスチャサンプリング ====
    float4 texColor = BloodTex.Sample(BloodSampler, input.TexCoord);
    float  alpha    = texColor.a * input.Color.a;

    if (alpha < ALPHA_CUTOFF)
        discard;

    // ==== UV 中心からの距離（液体の「厚み」シミュレーション）====
    float2 centered       = input.TexCoord * 2.0f - 1.0f;
    float  distFromCenter = length(centered);

    // ========================================================
    //  2D スクリーンブラッドモード（ライティングなし）
    // ========================================================
    if (ScreenMode > 0.5f)
    {
        float  thickness = saturate(1.0f - distFromCenter);
        float3 baseColor = lerp(BRIGHT_BLOOD_2D, DARK_BLOOD_2D, thickness * thickness);

        // エッジのリム（表面張力の光）
        float rim = pow(saturate(distFromCenter - 0.3f), 2.0f) * 0.3f;
        baseColor += float3(0.3f, 0.02f, 0.0f) * rim;
        baseColor *= input.Color.rgb * 2.5f;

        return float4(baseColor, alpha);
    }

    // ========================================================
    //  3D デカールモード（フルライティング）
    // ========================================================

    // --- 疑似法線（UV から液体の丸みを生成）---
    float3 normal = normalize(float3(
        centered.x * 0.7f,
        centered.y * 0.7f,
        1.0f - distFromCenter * 0.4f
    ));

    // --- SSS 風ベースカラー ---
    float  thickness = saturate(1.0f - distFromCenter);
    float3 baseColor = lerp(BRIGHT_BLOOD_3D, DARK_BLOOD_3D, thickness * thickness);

    // 微妙な色揺らぎ（Time で少しだけ揺れる）
    float colorShift = sin(Time * 0.5f + input.WorldPos.x * 2.0f) * 0.02f;
    baseColor.r += colorShift;

    // --- ディフューズ（ハーフランバート）---
    float NdotL   = dot(normal, -LightDir);
    float diffuse = NdotL * 0.4f + 0.6f;

    // --- スペキュラ（Blinn-Phong: 2 層）---
    float3 viewDir = normalize(CameraPos - input.WorldPos);
    float3 halfVec = normalize(-LightDir + viewDir);
    float  NdotH   = saturate(dot(normal, halfVec));

    float specSharp = pow(NdotH, SPEC_POWER_SHARP) * 3.0f;   // 鏡面ハイライト
    float specBroad = pow(NdotH, SPEC_POWER_BROAD) * 0.15f;   // ぬめり反射

    // --- フレネル（端ほど反射が強い）---
    float NdotV   = saturate(dot(normal, viewDir));
    float fresnel = pow(1.0f - NdotV, FRESNEL_EXPONENT) * 0.6f;

    // --- 環境光 ---
    float3 ambient = baseColor * 0.15f;

    // --- 最終合成 ---
    float3 finalColor = ambient
                       + baseColor * diffuse
                       + float3(1.0f, 0.85f, 0.7f) * specSharp
                       + float3(0.8f, 0.6f, 0.5f)  * specBroad
                       + BRIGHT_BLOOD_3D * fresnel;

    finalColor *= input.Color.rgb * 2.0f;
    finalColor  = ReinhardTonemap(finalColor, TONEMAP_SHOULDER);

    return float4(finalColor, alpha);
}
