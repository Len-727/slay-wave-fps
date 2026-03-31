// ============================================================
//  SkinnedVS.hlsl
//  Gothic Swarm - インスタンシング対応スキニング頂点シェーダー
//
//  【役割】
//  ボーンアニメーション (最大 4 ボーン) を適用し、
//  インスタンスごとのワールド行列で配置する。
//  ハーフランバートでライティングを計算し、PS に渡す。
// ============================================================

// === 定数バッファ ===
cbuffer FrameBuffer : register(b0)
{
    matrix View;
    matrix Projection;
};

cbuffer LightBuffer : register(b2)
{
    float4 AmbientColor;
    float4 DiffuseColor;
    float3 LightDirection;
    float  Padding;
    float3 CameraPos;
    float  Padding2;
};

// === 全インスタンスのボーン行列 (t1: PS の t0 はテクスチャ用) ===
StructuredBuffer<matrix> BoneBuffer : register(t1);

// === 頂点入力 ===
struct VSInput
{
    // --- 頂点データ ---
    float3 Position   : POSITION;
    float3 Normal     : NORMAL;
    float2 TexCoord   : TEXCOORD0;
    uint4  BoneIndex  : BLENDINDICES;
    float4 BoneWeight : BLENDWEIGHT;

    // --- インスタンスデータ (敵 1 体ごとに異なる) ---
    matrix World      : INST_WORLD;
    float4 Color      : INST_COLOR;       // .a にスタガーフラグを格納
    uint   BoneOffset : INST_BONEOFFSET;  // StructuredBuffer 内のオフセット
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;       // ライティング済み色 (.a = スタガーフラグ)
    float3 WorldPos : TEXCOORD1;    // リムライト計算用
};

// --- ライティング定数 ---
static const float WRAP_SCALE = 0.5f;   // ハーフランバートの折り返し量
static const float WRAP_BIAS  = 0.5f;

VSOutput main(VSInput input)
{
    VSOutput output;

    // === スキニング: ボーン行列の加重ブレンド ===
    float4 pos  = float4(input.Position, 1.0f);
    float4 norm = float4(input.Normal,   0.0f);

    float4 skinnedPos  = (float4)0;
    float4 skinnedNorm = (float4)0;

    matrix bone0 = BoneBuffer[input.BoneOffset + input.BoneIndex.x];
    skinnedPos  += mul(pos,  bone0) * input.BoneWeight.x;
    skinnedNorm += mul(norm, bone0) * input.BoneWeight.x;

    matrix bone1 = BoneBuffer[input.BoneOffset + input.BoneIndex.y];
    skinnedPos  += mul(pos,  bone1) * input.BoneWeight.y;
    skinnedNorm += mul(norm, bone1) * input.BoneWeight.y;

    matrix bone2 = BoneBuffer[input.BoneOffset + input.BoneIndex.z];
    skinnedPos  += mul(pos,  bone2) * input.BoneWeight.z;
    skinnedNorm += mul(norm, bone2) * input.BoneWeight.z;

    matrix bone3 = BoneBuffer[input.BoneOffset + input.BoneIndex.w];
    skinnedPos  += mul(pos,  bone3) * input.BoneWeight.w;
    skinnedNorm += mul(norm, bone3) * input.BoneWeight.w;

    // === 座標変換 ===
    float4 worldPos = mul(skinnedPos, input.World);
    float4 viewPos  = mul(worldPos, View);
    output.Position = mul(viewPos, Projection);
    output.WorldPos = worldPos.xyz;

    // === ライティング (ハーフランバート) ===
    float3 worldNormal = normalize(mul(skinnedNorm.xyz, (float3x3)input.World));
    output.Normal = worldNormal;

    float NdotL = dot(worldNormal, -LightDirection);
    float wrap  = NdotL * WRAP_SCALE + WRAP_BIAS;
    float3 lit  = AmbientColor.rgb + wrap * WRAP_BIAS;

    // .a はスタガーフラグとしてそのまま PS に渡す
    output.Color   = float4(lit * input.Color.rgb, input.Color.a);
    output.TexCoord = input.TexCoord;

    return output;
}
