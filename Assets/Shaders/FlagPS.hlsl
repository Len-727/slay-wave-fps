// ============================================================
//  FlagPS_Standalone.hlsl
//  Gothic Swarm - 旗 ピクセルシェーダー
//
//  【役割】
//  テクスチャ × ハーフランバートライティングで旗を描画。
// ============================================================

Texture2D    FlagTexture  : register(t0);
SamplerState SamplerLinear : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
    float3 WorldPos : TEXCOORD1;
};

// --- ライティング定数 ---
static const float3 LIGHT_DIR     = float3(0.5f, -0.5f, -1.0f);
static const float  HALF_LAMBERT_SCALE = 0.5f;
static const float  HALF_LAMBERT_BIAS  = 0.5f;

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = FlagTexture.Sample(SamplerLinear, input.TexCoord);

    float3 lightDir = normalize(LIGHT_DIR);
    float  NdotL    = dot(input.Normal, -lightDir);
    float  diffuse  = NdotL * HALF_LAMBERT_SCALE + HALF_LAMBERT_BIAS;

    return texColor * diffuse;
}
