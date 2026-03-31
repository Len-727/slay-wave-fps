// ============================================================
//  BloodFluidBlurPS.hlsl
//  Gothic Swarm - 血液流体シミュレーション Pass 2: バイラテラルブラー
//
//  【役割】
//  深度が近いピクセル同士だけをぼかし、粒を液体のように融合させる。
//  背景との境界はクッキリ残る (エッジ保護)。
//
//  【実装詳細】
//  - ループは固定回数 (MAX_RADIUS) で [unroll] 展開
//  - SampleLevel(0) で gradient 問題を回避
//  - Dilation (膨張) で深度 0 の隙間を近傍で埋める
// ============================================================
#include "Common.hlsli"

// --- 入力: FullscreenQuadVS からの出力 ---
// Common.hlsli の FullscreenVSOutput と同じレイアウト
struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// --- 深度テクスチャ (Pass 1 の出力) ---
Texture2D<float> DepthTexture : register(t0);
SamplerState     PointSampler : register(s0);

// --- ブラー設定 ---
cbuffer BlurCB : register(b0)
{
    float2 TexelSize;       // 1 ピクセルの UV サイズ (1/width, 1/height)
    float  BlurRadius;      // ブラー半径 (ピクセル数, 最大 MAX_RADIUS)
    float  DepthThreshold;  // エッジ保護の深度差閾値
    float2 BlurDirection;   // (1,0) = 水平 / (0,1) = 垂直
    float2 Padding;
};

// --- 固定ループ定数 ---
static const int   MAX_RADIUS       = 15;    // コンパイル時確定のループ上限
static const int   DILATION_RADIUS  = 5;     // 膨張の探索範囲
static const float DILATION_SCALE   = 0.8f;  // 膨張で埋めた深度のスケール

float main(PSInput input) : SV_Target
{
    float centerDepth = DepthTexture.SampleLevel(PointSampler, input.TexCoord, 0);

    // === Dilation: 深度 0 の隙間を近傍パーティクルの深度で埋める ===
    if (centerDepth <= 0.0f)
    {
        float maxDepth = 0.0f;
        [unroll]
        for (int d = -DILATION_RADIUS; d <= DILATION_RADIUS; d++)
        {
            float2 sampleUV = input.TexCoord + BlurDirection * TexelSize * (float)d;
            float  s        = DepthTexture.SampleLevel(PointSampler, sampleUV, 0);
            maxDepth = max(maxDepth, s);
        }

        if (maxDepth > 0.0f)
            centerDepth = maxDepth * DILATION_SCALE;
        else
            return 0.0f;  // 周囲にパーティクルなし → 完全スキップ
    }

    if (centerDepth >= 1.0f)
        return centerDepth;

    // === バイラテラルブラー本体 ===
    int   radius         = min((int)BlurRadius, MAX_RADIUS);
    float sigma          = max((float)radius * 0.5f, EPSILON);
    float invTwoSigmaSq  = -1.0f / (2.0f * sigma * sigma);
    float invTwoThreshSq = -1.0f / (2.0f * DepthThreshold * DepthThreshold);

    float totalWeight = 0.0f;
    float totalDepth  = 0.0f;

    [unroll]
    for (int i = -MAX_RADIUS; i <= MAX_RADIUS; i++)
    {
        if (i < -radius || i > radius)
            continue;

        float2 sampleUV    = input.TexCoord + BlurDirection * TexelSize * (float)i;
        float  sampleDepth = DepthTexture.SampleLevel(PointSampler, sampleUV, 0);

        // 深度 0 はスキップ (隙間は合算に入れない)
        if (sampleDepth <= 0.0f)
            continue;

        // ガウシアン重み (距離ベース)
        float gaussWeight = exp((float)(i * i) * invTwoSigmaSq);

        // 深度差による重み (エッジ保護)
        float depthDiff   = sampleDepth - centerDepth;
        float depthWeight = exp(depthDiff * depthDiff * invTwoThreshSq);

        float weight = gaussWeight * depthWeight;
        totalDepth  += sampleDepth * weight;
        totalWeight += weight;
    }

    return (totalWeight > 0.0f) ? (totalDepth / totalWeight) : centerDepth;
}
