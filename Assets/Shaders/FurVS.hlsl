// ============================================================
//  FurVS.hlsl
//  Gothic Swarm - ファーシェーダー (シェル法) 頂点シェーダー
//
//  【仕組み】
//  同じメッシュを何層も描画し、各層を法線方向にずらすことで
//  毛 / 苔の立体感を出す (Shell Texturing)。
//  先端の層ほど風で大きく揺れる。
// ============================================================

cbuffer FurParams : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;

    float  FurLength;      // 毛の長さ (苔なら 0.05〜0.15)
    float  CurrentLayer;   // 現在の層番号
    float  TotalLayers;    // 総層数 (16〜32)
    float  Time;

    float3 WindDirection;
    float  WindStrength;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position    : SV_POSITION;
    float2 TexCoord    : TEXCOORD0;
    float  Layer       : TEXCOORD1;   // 0.0 = 根元, 1.0 = 先端
    float3 WorldPos    : TEXCOORD2;
    float3 WorldNormal : TEXCOORD3;
};

// --- 風の揺れパラメータ ---
static const float WIND_WAVE_SPEED  = 2.0f;
static const float WIND_WAVE_FREQ_X = 0.5f;
static const float WIND_WAVE_FREQ_Z = 0.7f;

VSOutput main(VSInput input)
{
    VSOutput output;

    // 層の進行度 (0.0 = 根元, 1.0 = 先端)
    float layerRatio = CurrentLayer / TotalLayers;

    // 法線方向に押し出し (上の層ほど遠く)
    float3 displacement = input.Normal * FurLength * layerRatio;

    // 風の影響 (先端ほど大きく揺れる, layerRatio^2 で根元固定)
    float windWave = sin(Time * WIND_WAVE_SPEED
                       + input.Position.x * WIND_WAVE_FREQ_X
                       + input.Position.z * WIND_WAVE_FREQ_Z);
    float3 windOffset = WindDirection * WindStrength * windWave
                      * layerRatio * layerRatio;

    float3 furPos = input.Position + displacement + windOffset;

    // 座標変換
    float4 worldPos = mul(float4(furPos, 1.0f), World);
    float4 viewPos  = mul(worldPos, View);
    output.Position = mul(viewPos, Projection);

    output.TexCoord    = input.TexCoord;
    output.Layer       = layerRatio;
    output.WorldPos    = worldPos.xyz;
    output.WorldNormal = normalize(mul(input.Normal, (float3x3)World));

    return output;
}
