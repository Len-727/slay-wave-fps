// ============================================================
//  GaussianBlur.hlsl
//  Gothic Swarm - 深度ベース DOF (被写界深度) ピクセルシェーダー
//
//  【役割】
//  深度テクスチャから焦点距離との差を計算し、
//  ピンボケ量に応じてガウシアンブラーを適用する。
//  焦点付近はシャープ、遠方/近景はボケる。
// ============================================================
#include "Common.hlsli"

Texture2D    RenderTexture  : register(t0);  // カラーテクスチャ
Texture2D    DepthTexture   : register(t1);  // 深度テクスチャ
SamplerState TextureSampler : register(s0);

cbuffer BlurParams : register(b0)
{
    float2 TexelSize;      // (1/width, 1/height)
    float  BlurStrength;   // ブラー強度 (0.0〜1.0)
    float  FocalDepth;     // 焦点距離 (線形深度)
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// --- 深度線形化パラメータ ---
static const float NEAR_PLANE     = 0.1f;
static const float FAR_PLANE      = 1000.0f;
static const float DEPTH_RANGE    = FAR_PLANE - NEAR_PLANE;

// --- DOF パラメータ ---
static const float DOF_RANGE      = 10.0f;   // この距離差で最大ブラー
static const float BLUR_MIN       = 0.01f;   // これ以下はブラーなし
static const float BLUR_RADIUS    = 3.0f;    // ブラーのピクセル半径

// --- 9 サンプルガウシアンカーネル (合計 ≈ 1.0) ---
static const int   KERNEL_SIZE    = 9;
static const int   KERNEL_HALF    = 4;
static const float WEIGHTS[9] =
{
    0.05f, 0.09f, 0.12f, 0.15f, 0.18f, 0.15f, 0.12f, 0.09f, 0.05f
};

// --- 縦方向クロスブラーの減衰 ---
static const float CROSS_BLUR_WEIGHT = 0.5f;
static const float TOTAL_NORMALIZE   = 1.5f;  // 1.0 + CROSS_BLUR_WEIGHT

float4 main(PSInput input) : SV_TARGET
{
    float4 originalColor = RenderTexture.Sample(TextureSampler, input.TexCoord);

    // 深度を線形に変換
    float rawDepth   = DepthTexture.Sample(TextureSampler, input.TexCoord).r;
    float linearDepth = NEAR_PLANE * FAR_PLANE
                      / (FAR_PLANE - rawDepth * DEPTH_RANGE);

    // 焦点距離との差でブラー量を決定
    float depthDiff  = abs(linearDepth - FocalDepth);
    float blurAmount = saturate(depthDiff / DOF_RANGE) * BlurStrength;

    if (blurAmount < BLUR_MIN)
        return originalColor;

    // === 水平ガウシアンブラー ===
    float4 blurred  = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float2 blurDir  = TexelSize * blurAmount * BLUR_RADIUS;

    [unroll]
    for (int i = 0; i < KERNEL_SIZE; i++)
    {
        float2 offset = blurDir * (float)(i - KERNEL_HALF);
        blurred += RenderTexture.Sample(TextureSampler, input.TexCoord + offset) * WEIGHTS[i];
    }

    // === 縦方向クロスブラー (加算) ===
    [unroll]
    for (int j = 0; j < KERNEL_SIZE; j++)
    {
        float2 offset = float2(0.0f, blurDir.y) * (float)(j - KERNEL_HALF);
        blurred += RenderTexture.Sample(TextureSampler, input.TexCoord + offset)
                 * WEIGHTS[j] * CROSS_BLUR_WEIGHT;
    }

    blurred /= TOTAL_NORMALIZE;
    return blurred;
}
