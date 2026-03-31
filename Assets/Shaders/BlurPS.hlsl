// ============================================================
//  BlurPS.hlsl
//  Gothic Swarm - 5×5 ガウシアンブラー ピクセルシェーダー
//
//  【役割】
//  シーンテクスチャに 5×5 ガウシアンカーネルを適用してぼかす。
//  texelSize は cbuffer から受け取る (解像度ハードコード排除)。
// ============================================================
#include "Common.hlsli"

Texture2D    RenderTexture  : register(t0);
SamplerState TextureSampler : register(s0);

// --- 定数バッファ (C++ から解像度情報を受け取る) ---
cbuffer BlurParams : register(b0)
{
    float2 TexelSize;      // (1.0 / screenWidth, 1.0 / screenHeight)
    float  BlurStrength;   // ブラー強度スケール (1.0 がデフォルト)
    float  Padding;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// --- 5×5 ガウシアンカーネル (合計 ≈ 1.0) ---
static const int   KERNEL_HALF = 2;
static const float WEIGHTS[25] =
{
    0.003f, 0.013f, 0.022f, 0.013f, 0.003f,
    0.013f, 0.059f, 0.097f, 0.059f, 0.013f,
    0.022f, 0.097f, 0.159f, 0.097f, 0.022f,
    0.013f, 0.059f, 0.097f, 0.059f, 0.013f,
    0.003f, 0.013f, 0.022f, 0.013f, 0.003f
};

float4 main(PSInput input) : SV_Target
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int    index = 0;

    [unroll]
    for (int y = -KERNEL_HALF; y <= KERNEL_HALF; y++)
    {
        [unroll]
        for (int x = -KERNEL_HALF; x <= KERNEL_HALF; x++)
        {
            float2 offset  = float2((float)x, (float)y) * TexelSize * BlurStrength;
            float4 sampled = RenderTexture.Sample(TextureSampler, input.TexCoord + offset);
            color += sampled * WEIGHTS[index];
            index++;
        }
    }

    return color;
}
