// SkinnedPS.hlsl - ピクセルシェーダー
// 【役割】各ピクセルの最終的な色を決める

// テクスチャとサンプラー
Texture2D DiffuseTexture : register(t0); // テクスチャ画像
SamplerState LinearSampler : register(s0); // テクスチャの読み方（補間方法）

// 頂点シェーダーからの入力
struct PSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0; // ライティング済みの色
};

// === メイン関数（画面上の各ピクセルに対して並列実行）===
float4 main(PSInput input) : SV_TARGET
{
    // テクスチャから色を取得
    float4 texColor = DiffuseTexture.Sample(LinearSampler, input.TexCoord);

    // テクスチャの色 × ライティングの色 = 最終色
    float4 finalColor = texColor * input.Color;

    // アルファが0に近いピクセルは描画しない（透明部分を切り抜く）
    clip(finalColor.a - 0.1f);

    return finalColor;
}