// ========================================
// FlagPS_Standalone.hlsl - ピクセルシェーダー（include不要版）
// ========================================

// 構造体
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL;
    float3 worldPos : TEXCOORD1;
};

// テクスチャ
Texture2D flagTexture : register(t0);
SamplerState samplerState : register(s0);

// ピクセルシェーダー
float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 textureColor = flagTexture.Sample(samplerState, input.texCoord);
    
    float3 lightDir = normalize(float3(0.5, -0.5, -1.0));
    float NdotL = dot(input.normal, -lightDir);
    float diffuse = max(NdotL, 0.0);
    diffuse = diffuse * 0.5 + 0.5;
    
    float4 finalColor = textureColor * diffuse;
    
    return finalColor;
}
