Texture2D renderTexture : register(t0);

// サンプラー
SamplerState textureSampler : register(s0);

// 入力構造体
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// メイン関数
float4 main(PSInput input) : SV_Target
{
    // === ガウシアンブラーの実装 ===
    float2 texelSize = float2(1.0f / 1920.0f, 1.0f / 1080.0f);
    
    // ブラー強度（C++から渡すこともできる）
    float blurStrength = 1.0f;
    
    
    float weights[25] =
    {
        0.003, 0.013, 0.022, 0.013, 0.003,
        0.013, 0.059, 0.097, 0.059, 0.013,
        0.022, 0.097, 0.159, 0.097, 0.022,
        0.013, 0.059, 0.097, 0.059, 0.013,
        0.003, 0.013, 0.022, 0.013, 0.003
    };
    
    // 周辺ピクセルをサンプリング
    float4 color = float4(0, 0, 0, 0);
    int index = 0;
    
    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            // オフセット座標
            float2 offset = float2(x, y) * texelSize * blurStrength;
            
            // サンプリング
            float4 sample = renderTexture.Sample(textureSampler, input.texcoord + offset);
            
            // 加重平均
            color += sample * weights[index];
            index++;
        }
    }
    
    return color;
}