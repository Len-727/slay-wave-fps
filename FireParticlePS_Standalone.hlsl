// ========================================
// FireParticlePS_Standalone.hlsl
// 炎パーティクル用 ピクセルシェーダー（Standalone版）
// ========================================

// ========================================
// テクスチャとサンプラー
// ========================================

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

// ========================================
// 構造体
// ========================================

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
};

// ========================================
// ピクセルシェーダー
// ========================================

float4 main(PS_INPUT input) : SV_TARGET
{
    // === テクスチャサンプリング ===
    float4 textureColor = shaderTexture.Sample(SampleType, input.texCoord);
    
    // === パーティクル色と合成 ===
    float4 finalColor = textureColor * input.color;
    
    // === ソフトパーティクル効果 ===
    // 中心から端に向かって透明にする
    
    // 中心からの距離を計算
    float2 centerDist = input.texCoord - 0.5;
    float distFromCenter = length(centerDist) * 2.0;
    
    // 端に行くほど透明に
    float alpha = 1.0 - smoothstep(0.5, 1.0, distFromCenter);
    alpha = alpha * alpha;
    
    // 最終的なアルファ値
    finalColor.w *= alpha;
    
    return finalColor;
}