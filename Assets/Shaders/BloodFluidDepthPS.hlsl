// ============================================================
//  BloodFluidDepthPS.hlsl
//  Gothic Swarm - 血液流体シミュレーション Pass 1: 深度パス
//
//  【役割】
//  各パーティクルのビルボードを「球面状の深度」で描く。
//  出力は R32_FLOAT テクスチャ (深度を色として保存)。
//  後段のブラーパスでこの深度を滑らかにすると粒が融合する。
// ============================================================
#include "Common.hlsli"

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
    float  Life     : TEXCOORD1;
};

// --- 球面深度パラメータ ---
static const float SPHERE_DEPTH_OFFSET = 0.005f;  // 球面の深度オフセット量

float4 main(PSInput input) : SV_Target
{
    // UV を -1〜+1 に変換 (中心 = 0,0)
    float2 centered = input.TexCoord * 2.0f - 1.0f;
    float  distSq   = dot(centered, centered);

    // 円の外側はカット
    if (distSq > 1.0f)
        discard;

    // 球面の深度オフセット: sqrt(1 - r^2)
    // 中心が最も手前 (1.0)、端が最も奥 (0.0)
    float sphereZ = sqrt(1.0f - distSq);

    // NDC 深度を球面で調整
    float depth = saturate(input.Position.z - sphereZ * SPHERE_DEPTH_OFFSET);

    // R32_FLOAT に深度値を書き込む
    return float4(depth, depth, depth, 1.0f);
}
