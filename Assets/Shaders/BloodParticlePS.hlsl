// ============================================================
//  BloodParticlePS.hlsl
//  GPU Pixel Shader: 血パーティクルの見た目
//
//  【何をしているか？】
//  四角いビルボードを「丸い血の粒」にする。
//  中心は濃く、端は薄い → 自然な血しぶき。
//
//  【たとえ】
//  紙に赤インクを落とすと、中心が濃くて端がにじむ。
//  それをGPUでリアルタイムに再現する。
// ============================================================

// --- 頂点シェーダーからの入力 ---
struct PSInput
{
    float4 position : SV_POSITION; // スクリーン座標
    float2 texcoord : TEXCOORD0; // UV (0~1)
    float4 color : COLOR0; // 血の色 + アルファ
    float life : TEXCOORD1; // 寿命の割合 (0~1)
};

float4 main(PSInput input) : SV_Target
{
    // === UV を中心座標(-1~+1)に変換 ===
    float2 centered = input.texcoord * 2.0f - 1.0f;
    float distSq = dot(centered, centered); // 中心からの距離の二乗

    // === 円の外側を捨てる(四角→丸) ===
    if (distSq > 1.0f)
        discard;

    // === 血の濃さ: 中心が濃く、端が薄い ===
    // smoothstep(1, 0, dist) = 端(1)で0、中心(0)で1
    float dist = sqrt(distSq);
    float coreFade = smoothstep(1.0f, 0.2f, dist); // 端から80%までフェード

    // === 最終アルファ ===
    float alpha = input.color.a * coreFade;

    // === 色: 中心は明るめ、端は暗め ===
    float3 finalColor = input.color.rgb * (0.7f + 0.3f * coreFade);

    return float4(finalColor, alpha);
}
