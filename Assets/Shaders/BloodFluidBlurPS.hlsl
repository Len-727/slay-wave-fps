// ============================================================
//  BloodFluidBlurPS.hlsl
//  Pass 2: バイラテラルブラー - 深度を滑らかにして粒を融合させる
//
//  【何をしてるか？】
//  普通のブラー(ガウシアン)だと全部ボケるけど、
//  バイラテラルブラーは「深度が近いピクセル同士だけ」ぼかす。
//  → 血の粒が液体のように融合するが、背景との境界はクッキリ残る。
// ============================================================

// --- フルスクリーンクワッドの入力 ---
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// --- 深度テクスチャ(Pass 1の出力) ---
Texture2D<float> DepthTexture : register(t0);
SamplerState PointSampler : register(s0); // 点サンプリング(補間しない)

// --- ブラー設定 ---
cbuffer BlurCB : register(b0)
{
    float2 TexelSize; // 1ピクセルのUV空間サイズ(1/width, 1/height)
    float BlurRadius; // ブラーの半径(ピクセル数)
    float DepthThreshold; // この深度差以上は混ぜない(エッジ保護)
    float2 BlurDirection; // ブラー方向(1,0)=水平, (0,1)=垂直
    float2 Padding; // アラインメント
};

float main(PSInput input) : SV_Target
{
    // === 中心ピクセルの深度 ===
    float centerDepth = DepthTexture.Sample(PointSampler, input.texcoord);

    // 深度が0(=何もない)ならスキップ
    if (centerDepth <= 0.0f || centerDepth >= 1.0f)
        return centerDepth;

    // === バイラテラルブラー(カーネルサイズ = BlurRadius * 2 + 1) ===
    float totalWeight = 0.0f;
    float totalDepth = 0.0f;

    // ブラー半径(整数に丸め)
    int radius = (int) BlurRadius;

    for (int i = -radius; i <= radius; i++)
    {
        // サンプル位置(BlurDirectionで水平/垂直を切り替え)
        float2 sampleUV = input.texcoord + BlurDirection * TexelSize * (float) i;

        // サンプルの深度
        float sampleDepth = DepthTexture.Sample(PointSampler, sampleUV);

        // === ガウシアン重み(距離が遠いほど影響小) ===
        float sigma = (float) radius * 0.5f;
        float gaussWeight = exp(-(float) (i * i) / (2.0f * sigma * sigma));

        // === 深度差による重み(深度が違いすぎたら混ぜない) ===
        float depthDiff = abs(sampleDepth - centerDepth);
        float depthWeight = exp(-depthDiff * depthDiff / (2.0f * DepthThreshold * DepthThreshold));

        // === 合計重み(ガウシアン × 深度) ===
        float weight = gaussWeight * depthWeight;

        totalDepth += sampleDepth * weight;
        totalWeight += weight;
    }

    // 重み付き平均を返す
    return (totalWeight > 0.0f) ? (totalDepth / totalWeight) : centerDepth;
}
