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
    float BlurRadius; // ブラーの強さ(sigmaに使う)
    float DepthThreshold; // この深度差以上は混ぜない(エッジ保護)
    float2 BlurDirection; // ブラー方向(1,0)=水平, (0,1)=垂直
    float2 Padding; // アラインメント
};

//  ループ回数は固定！(コンパイル時定数)
// GPUシェーダーでは動的ループ回数が使えない場合がある
static const int BLUR_RADIUS = 7; // 7ピクセル半径 = 15タップ

float main(PSInput input) : SV_Target
{
    // === 中心ピクセルの深度 ===
    float centerDepth = DepthTexture.Sample(PointSampler, input.texcoord);

    // 深度が0(=何もない)か1(=最遠)ならスキップ
    if (centerDepth <= 0.0f || centerDepth >= 1.0f)
        return centerDepth;

    // === バイラテラルブラー(15タップ: -7 ~ +7) ===
    float totalWeight = 0.0f;
    float totalDepth = 0.0f;

    // sigma(ガウシアンの広がり)
    float sigma = BlurRadius * 0.5f;
    float sigmaSq2 = 2.0f * sigma * sigma; // 2 sigma^2(距離用)
    float depthSq2 = 2.0f * DepthThreshold * DepthThreshold; // 2 sigma^2(深度用)

    //  固定回数ループ(-BLUR_RADIUS ~ +BLUR_RADIUS)
    [unroll]  // コンパイラに「展開してOK」と明示
    for (int i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++)
    {
        // サンプル位置(BlurDirectionで水平/垂直を切り替え)
        float2 sampleUV = input.texcoord + BlurDirection * TexelSize * (float) i;

        // サンプルの深度
        float sampleDepth = DepthTexture.Sample(PointSampler, sampleUV);

        // === ガウシアン重み(距離が遠いほど影響小) ===
        float gaussWeight = exp(-(float) (i * i) / sigmaSq2);

        // === 深度差による重み(深度が違いすぎたら混ぜない) ===
        float depthDiff = abs(sampleDepth - centerDepth);
        float depthWeight = exp(-depthDiff * depthDiff / depthSq2);

        // === 合計重み(ガウシアン x 深度) ===
        float weight = gaussWeight * depthWeight;

        totalDepth += sampleDepth * weight;
        totalWeight += weight;
    }

    // 重み付き平均を返す
    return (totalWeight > 0.0f) ? (totalDepth / totalWeight) : centerDepth;
}
