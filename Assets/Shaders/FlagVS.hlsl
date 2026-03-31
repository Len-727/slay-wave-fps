// ============================================================
//  FlagVS_Standalone.hlsl
//  Gothic Swarm - 旗 (布シミュレーション) 頂点シェーダー
//
//  【役割】
//  正弦波でメッシュの Y/Z を変形し、旗がはためく動きを作る。
//  根元 (X=-1) は固定、先端 (X=+1) ほど大きく揺れる。
// ============================================================

cbuffer MatrixBuffer : register(b0)
{
    matrix WorldMatrix;
    matrix ViewMatrix;
    matrix ProjectionMatrix;
};

cbuffer TimeBuffer : register(b1)
{
    float Time;
    float WaveSpeed;      // 波の伝播速度
    float WaveAmplitude;  // 波の振幅
    float Padding;
};

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
    float3 WorldPos : TEXCOORD1;
};

// --- 波の周波数 ---
static const float WAVE_FREQ_Y = 2.0f;   // Y 方向の波の密度
static const float WAVE_FREQ_Z = 1.5f;   // Z 方向の波の密度
static const float WAVE_Z_RATIO = 0.7f;  // Z 波速の Y 波速に対する比率
static const float WAVE_Z_SCALE = 0.5f;  // Z 方向の振幅スケール

VSOutput main(VSInput input)
{
    VSOutput output;

    // 根元 (X=-1) = 0, 先端 (X=+1) = 1 に正規化
    float widthFactor = (input.Position.x + 1.0f) * 0.5f;

    // 正弦波で Y/Z を変形 (先端ほど大きく揺れる)
    float waveY = sin(input.Position.x * WAVE_FREQ_Y + Time * WaveSpeed)
                * WaveAmplitude * widthFactor;
    float waveZ = cos(input.Position.x * WAVE_FREQ_Z + Time * WaveSpeed * WAVE_Z_RATIO)
                * WaveAmplitude * WAVE_Z_SCALE * widthFactor;

    float3 newPos = input.Position;
    newPos.y += waveY;
    newPos.z += waveZ;

    // 座標変換
    float4 worldPos = mul(float4(newPos, 1.0f), WorldMatrix);
    output.WorldPos = worldPos.xyz;

    float4 viewPos  = mul(worldPos, ViewMatrix);
    output.Position = mul(viewPos, ProjectionMatrix);

    output.TexCoord = input.TexCoord;
    output.Normal   = normalize(mul(input.Normal, (float3x3)WorldMatrix));

    return output;
}
