// SkinnedVS.hlsl - インスタンシング対応スキニング頂点シェーダー

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
    float Padding;
    float3 CameraPos; //カメラのワールド位置（リムライト計算用）
    float Padding2; // 16バイト境界に揃えるパディング
};

// === StructuredBuffer: 全インスタンスのボーン行列 ===
// t0 はピクセルシェーダーのテクスチャに割るので t1 を使う
StructuredBuffer<matrix> BoneBuffer : register(t1);

// === 頂点入力（モデルの頂点データ）===
struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    uint4 BoneIndex : BLENDINDICES;
    float4 BoneWeight : BLENDWEIGHT;

    // --- インスタンスデータ（敵1体ごとに違う）---
    matrix World : INST_WORLD; // ワールド行列
    float4 Color : INST_COLOR; // 色（.a にスタガーフラグを仕込む）
    uint BoneOffset : INST_BONEOFFSET; // StructuredBuffer内のオフセット
};

// ★変更: WorldPos を//（PSでリムライト計算に使う）
struct VSOutput
{
    float4 Position : SV_POSITION; // スクリーン座標
    float3 Normal : NORMAL; // ワールド法線
    float2 TexCoord : TEXCOORD0; // UV座標
    float4 Color : COLOR0; // ライティング済みの色
    float3 WorldPos : TEXCOORD1; // ワールド座標（リムライト用）
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // --- スキニング: BoneOffset を使って自分のボーン行列を読む ---
    float4 pos = float4(input.Position, 1.0f);
    float4 norm = float4(input.Normal, 0.0f);

    float4 skinnedPos = (float4) 0;
    float4 skinnedNorm = (float4) 0;

    // ボーン0
    matrix bone0 = BoneBuffer[input.BoneOffset + input.BoneIndex.x];
    skinnedPos += mul(pos, bone0) * input.BoneWeight.x;
    skinnedNorm += mul(norm, bone0) * input.BoneWeight.x;

    // ボーン1
    matrix bone1 = BoneBuffer[input.BoneOffset + input.BoneIndex.y];
    skinnedPos += mul(pos, bone1) * input.BoneWeight.y;
    skinnedNorm += mul(norm, bone1) * input.BoneWeight.y;

    // ボーン2
    matrix bone2 = BoneBuffer[input.BoneOffset + input.BoneIndex.z];
    skinnedPos += mul(pos, bone2) * input.BoneWeight.z;
    skinnedNorm += mul(norm, bone2) * input.BoneWeight.z;

    // ボーン3
    matrix bone3 = BoneBuffer[input.BoneOffset + input.BoneIndex.w];
    skinnedPos += mul(pos, bone3) * input.BoneWeight.w;
    skinnedNorm += mul(norm, bone3) * input.BoneWeight.w;

    // --- 座標変換 ---
    float4 worldPos = mul(skinnedPos, input.World);
    float4 viewPos = mul(worldPos, View);
    output.Position = mul(viewPos, Projection);

    //  ワールド座標をPSに渡す
    output.WorldPos = worldPos.xyz;

    // --- ライティング ---
    float3 worldNormal = normalize(mul(skinnedNorm.xyz, (float3x3) input.World));
    output.Normal = worldNormal;

    float NdotL = dot(worldNormal, -LightDirection);
    float wrap = NdotL * 0.5f + 0.5f;
    float3 lit = AmbientColor.rgb + wrap * 0.5f;

    // Color.a はそのまま渡す（スタガーフラグとして使う）
    output.Color = float4(lit * input.Color.rgb, input.Color.a);

    output.TexCoord = input.TexCoord;
    return output;
}
