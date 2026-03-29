// SkinnedPS.hlsl - ピクセルシェーダー

Texture2D DiffuseTexture : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer LightBuffer : register(b2)
{
    float4 AmbientColor;
    float4 DiffuseColor;
    float3 LightDirection;
    float Padding;
    float3 CameraPos;
    float Padding2;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
    float3 WorldPos : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = DiffuseTexture.Sample(LinearSampler, input.TexCoord);
    float4 baseColor = texColor * float4(input.Color.rgb, 1.0);
    

    float rimFlag = input.Color.a;
    bool isStaggered = (rimFlag > 1.5);

    if (isStaggered)
    {
        float3 viewDir = normalize(CameraPos - input.WorldPos);
        float3 normal = normalize(input.Normal);
        float NdotV = max(abs(dot(normal, viewDir)), 0.05);
        float rim = 1.0 - NdotV;

        //  タイプ別にリムの太さを変える（デカい敵ほど細く）
        float rimPower = 3.0; // NORMAL/RUNNER
        if (rimFlag > 4.5)
            rimPower = 8.0; // BOSS
        else if (rimFlag > 3.5)
            rimPower = 6.0; // MIDBOSS
        else if (rimFlag > 2.5)
            rimPower = 4.5; // TANK

        rim = pow(rim, rimPower);

        float phase = frac(rimFlag);
        float pulse = sin(phase * 6.28318) * 0.25 + 0.75;

        float rimStrength = 1.2; // 強さは全敵統一
        float3 rimColor = float3(0.6, 0.05, 1.0);
        baseColor.rgb += rimColor * rim * rimStrength * pulse;
    }

    clip(baseColor.a - 0.1);
    baseColor.a = texColor.a;

    return baseColor;
}
