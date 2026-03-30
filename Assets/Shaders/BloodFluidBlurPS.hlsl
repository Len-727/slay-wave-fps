// ============================================================
//  BloodFluidBlurPS.hlsl
//  Pass 2: バイラテラルブラー - 深度を滑らかにして粒を融合させる
//
//  【何をしてるか？】
//  普通のブラー(ガウシアン)だと全部ボケるけど、
//  バイラテラルブラーは「深度が近いピクセル同士だけ」ぼかす。
//  → 血の粒が液体のように融合するが、背景との境界はクッキリ残る。
//
//  【修正】
//  - ループを固定回数(MAX_RADIUS=15)にして [unroll] で展開
//  - Sample → SampleLevel(0) でgradient問題を回避
// ============================================================

// --- フルスクリーンクワッドの入力 ---
struct PSInput
{
    float4 position : SV_POSITION; // スクリーン座標
    float2 texcoord : TEXCOORD0; // UV座標 (0,0)?(1,1)
};

// --- 深度テクスチャ(Pass 1の出力) ---
Texture2D<float> DepthTexture : register(t0);
SamplerState PointSampler : register(s0); // 点サンプリング(補間しない)

// --- ブラー設定 ---
cbuffer BlurCB : register(b0)
{
    float2 TexelSize; // 1ピクセルのUV空間サイズ(1/width, 1/height)
    float BlurRadius; // ブラーの半径(ピクセル数) ※最大15
    float DepthThreshold; // この深度差以上は混ぜない(エッジ保護)
    float2 BlurDirection; // ブラー方向 (1,0)=水平, (0,1)=垂直
    float2 Padding; // アラインメント用
};

//  固定の最大半径(コンパイル時に確定する定数)
// BlurRadiusがこれ以下なら早期breakで実質的に動的半径と同じ効果
static const int MAX_RADIUS = 15;

float main(PSInput input) : SV_Target
{
    float centerDepth = DepthTexture.SampleLevel(PointSampler, input.texcoord, 0);

    //Dilation(膨張): 深度0の隙間を近傍パーティクルの深度で埋める 
    // これがないと粒と粒の「隙間」が永遠に0のままで融合しない
    if (centerDepth <= 0.0f)
    {
        float maxDepth = 0.0f;
        [unroll]
        for (int d = -5; d <= 5; d++)
        {
            float2 sampleUV = input.texcoord + BlurDirection * TexelSize * (float) d;
            float s = DepthTexture.SampleLevel(PointSampler, sampleUV, 0);
            maxDepth = max(maxDepth, s);
        }
        if (maxDepth > 0.0f)
            centerDepth = maxDepth * 0.8f; // 少し奥にオフセットして自然に
        else
            return 0.0f; // 周囲にパーティクルなし→完全スキップ
    }

    if (centerDepth >= 1.0f)
        return centerDepth;

    float totalWeight = 0.0f;
    float totalDepth = 0.0f;

    int radius = min((int) BlurRadius, MAX_RADIUS);

    float sigma = max((float) radius * 0.5f, 0.001f);
    float invTwoSigmaSq = -1.0f / (2.0f * sigma * sigma);

    float invTwoThreshSq = -1.0f / (2.0f * DepthThreshold * DepthThreshold);

    //  [unroll] + 固定MAX_RADIUSループ → コンパイラが確実に展開できる
    [unroll]
    for (int i = -MAX_RADIUS; i <= MAX_RADIUS; i++)
    {
        // 実際のBlurRadiusを超えたらスキップ
        // ※ [unroll]展開後、コンパイラがこの条件を最適化してくれる
        if (i < -radius || i > radius)
            continue;

        // サンプル位置(BlurDirectionで水平/垂直を切り替え)
        float2 sampleUV = input.texcoord + BlurDirection * TexelSize * (float) i;

        //  SampleLevel(0)で安全に読む(gradient不要)
        float sampleDepth = DepthTexture.SampleLevel(PointSampler, sampleUV, 0);
        
        // // //: 深度0のサンプルはスキップ（隙間は合算に入れない）
        if (sampleDepth <= 0.0f)
            continue;

        // === ガウシアン重み(距離が遠いほど影響小) ===
        float gaussWeight = exp((float) (i * i) * invTwoSigmaSq);

        // === 深度差による重み(深度が違いすぎたら混ぜない) ===
        float depthDiff = sampleDepth - centerDepth;
        float depthWeight = exp(depthDiff * depthDiff * invTwoThreshSq);

        // === 合計重み(ガウシアン × 深度) ===
        float weight = gaussWeight * depthWeight;

        totalDepth += sampleDepth * weight;
        totalWeight += weight;
    }

    // 重みつき平均を返す
    return (totalWeight > 0.0f) ? (totalDepth / totalWeight) : centerDepth;
}
