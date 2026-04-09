// ============================================================
//  KeyPromptPS.hlsl
//  キープロンプト用ピクセルシェーダー（テクスチャビルボード）
//
//  【役割】テクスチャをサンプリングして脈動アニメーションを加える
//  【VS】StunRingVS.cso を再利用（同じ定数バッファ構造）
// ============================================================

cbuffer RingCB : register(b0)
{
    matrix View;
    matrix Projection;
    float3 EnemyPos;
    float RingSize;
    float Time; // スタガー経過時間（秒）
    float3 Padding;
};

// テクスチャとサンプラー
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    // テクスチャからサンプリング
    float4 texColor = g_texture.Sample(g_sampler, input.TexCoord);

    // 透明部分は早期リターン
    if (texColor.a < 0.01f)
        discard;

    // === 脈動アニメーション ===
    // sin波でゆっくり明滅（1.0〜1.3の範囲）
    float pulse = 1.0f + 0.3f * sin(Time * 3.0f);
    texColor.rgb *= pulse;

    // === 出現フェードイン（0.3秒かけて出現）===
    float fadeIn = saturate(Time / 0.3f);
    texColor.a *= fadeIn;

    return texColor;
}
