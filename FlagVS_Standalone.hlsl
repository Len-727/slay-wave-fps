// ========================================
// FlagVS_Standalone.hlsl - 頂点シェーダー（include不要版）
// ========================================

// 定数バッファ
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer TimeBuffer : register(b1)
{
    float time;
    float waveSpeed;
    float waveAmplitude;
    float padding;
};

// 構造体
struct VS_INPUT
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL;
    float3 worldPos : TEXCOORD1;
};

// 頂点シェーダー
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float widthFactor = (input.position.x + 1.0) / 2.0;
    
    float waveY = sin(input.position.x * 2.0 + time * waveSpeed) * waveAmplitude * widthFactor;
    float waveZ = cos(input.position.x * 1.5 + time * waveSpeed * 0.7) * waveAmplitude * 0.5 * widthFactor;
    
    float3 newPosition = input.position;
    newPosition.y += waveY;
    newPosition.z += waveZ;
    
    float4 worldPosition = mul(float4(newPosition, 1.0), worldMatrix);
    output.worldPos = worldPosition.xyz;
    
    float4 viewPosition = mul(worldPosition, viewMatrix);
    output.position = mul(viewPosition, projectionMatrix);
    
    output.texCoord = input.texCoord;
    output.normal = mul(input.normal, (float3x3)worldMatrix);
    output.normal = normalize(output.normal);
    
    return output;
}
