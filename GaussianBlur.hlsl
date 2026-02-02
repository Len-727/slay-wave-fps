// GaussianBlur.hlsl - 深度ベースDOFシェーダー

// テクスチャとサンプラー
Texture2D renderTexture : register(t0); // カラーテクスチャ
Texture2D depthTexture : register(t1); // 深度テクスチャ
SamplerState textureSampler : register(s0);

// 定数バッファ
cbuffer BlurParams : register(b0)
{
    float2 texelSize; // 1ピクセルのサイズ (1/width, 1/height)
    float blurStrength; // ブラー強度 (0.0?1.0)
    float focalDepth; // 焦点距離（正規化深度）
};

// 入力構造体
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// ガウシアンウェイト（9サンプル）
static const float weights[9] =
{
    0.05, 0.09, 0.12, 0.15, 0.18, 0.15, 0.12, 0.09, 0.05
};

// メイン関数
float4 main(PSInput input) : SV_TARGET
{
    // 元のカラーを取得
    float4 originalColor = renderTexture.Sample(textureSampler, input.texcoord);
    
    // 深度を取得（0=近い、1=遠い）
    float depth = depthTexture.Sample(textureSampler, input.texcoord).r;
    
    // 深度を線形に変換（near=0.1, far=1000）
    float nearPlane = 0.1;
    float farPlane = 1000.0;
    float linearDepth = nearPlane * farPlane / (farPlane - depth * (farPlane - nearPlane));
    
    // 焦点距離との差でブラー量を計算
    float depthDiff = abs(linearDepth - focalDepth);
    float blurAmount = saturate(depthDiff / 10.0) * blurStrength;
    
    // ブラーが不要ならそのまま返す
    if (blurAmount < 0.01)
    {
        return originalColor;
    }
    
    // ガウシアンブラー適用
    float4 blurredColor = float4(0, 0, 0, 0);
    float2 blurDir = texelSize * blurAmount * 3.0; // ブラー半径
    
    for (int i = 0; i < 9; i++)
    {
        float2 offset = blurDir * (i - 4); // -4?+4のオフセット
        blurredColor += renderTexture.Sample(textureSampler, input.texcoord + offset) * weights[i];
    }
    
    // 縦方向も加算（クロスブラー）
    for (int j = 0; j < 9; j++)
    {
        float2 offset = float2(0, blurDir.y) * (j - 4);
        blurredColor += renderTexture.Sample(textureSampler, input.texcoord + offset) * weights[j] * 0.5;
    }
    
    // 正規化
    blurredColor /= 1.5;
    
    return blurredColor;
}