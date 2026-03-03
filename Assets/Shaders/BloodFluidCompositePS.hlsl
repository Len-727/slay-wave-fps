// ============================================================
//  BloodFluidCompositePS.hlsl
//  Pass 3: 合成パス - 血がある場所だけ描画(αブレンド方式)
//
//  【新アプローチ】
//  旧: 全画面を上書き(シーンコピー必要 → CopyResource失敗で真っ黒)
//  新: 血がある場所だけ描画、他はdiscard(シーンコピー不要！)
//
//  メリット:
//  - CopyResource不要 → パフォーマンス向上
//  - シーンが消えない(既存の描画結果の上に血を重ねるだけ)
// ============================================================

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// --- テクスチャ ---
Texture2D<float> BlurredDepth : register(t0); // ブラー済み深度
// t1: 不要になった(シーンコピーなし)
SamplerState PointSampler : register(s0);

// --- 定数バッファ ---
cbuffer CompositeCB : register(b0)
{
    float4x4 InvProjection;
    float2 TexelSize;
    float Time;
    float Thickness;
    float3 LightDir;
    float SpecularPower;
    float3 CameraPos;
    float FluidAlpha;
};

// === 深度からビュー空間座標を復元 ===
float3 ReconstructViewPos(float2 uv, float depth)
{
    float4 clipPos;
    clipPos.x = uv.x * 2.0f - 1.0f;
    clipPos.y = (1.0f - uv.y) * 2.0f - 1.0f;
    clipPos.z = depth;
    clipPos.w = 1.0f;

    float4 viewPos = mul(clipPos, InvProjection);
    return viewPos.xyz / viewPos.w;
}

float4 main(PSInput input) : SV_Target
{
    float depth = BlurredDepth.SampleLevel(PointSampler, input.texcoord, 0);

    // // 血がないピクセルは完全にスキップ(シーンに触らない！)
    if (depth <= 0.0f || depth >= 1.0f)
    {
        discard;
    }

    // === 周囲4ピクセルの深度から法線を復元 ===
    float depthL = BlurredDepth.SampleLevel(PointSampler, input.texcoord + float2(-TexelSize.x, 0), 0);
    float depthR = BlurredDepth.SampleLevel(PointSampler, input.texcoord + float2(TexelSize.x, 0), 0);
    float depthU = BlurredDepth.SampleLevel(PointSampler, input.texcoord + float2(0, -TexelSize.y), 0);
    float depthD = BlurredDepth.SampleLevel(PointSampler, input.texcoord + float2(0, TexelSize.y), 0);

    float3 posC = ReconstructViewPos(input.texcoord, depth);
    float3 posL = ReconstructViewPos(input.texcoord + float2(-TexelSize.x, 0), depthL);
    float3 posR = ReconstructViewPos(input.texcoord + float2(TexelSize.x, 0), depthR);
    float3 posU = ReconstructViewPos(input.texcoord + float2(0, -TexelSize.y), depthU);
    float3 posD = ReconstructViewPos(input.texcoord + float2(0, TexelSize.y), depthD);

    float3 ddx_val = posR - posL;
    float3 ddy_val = posD - posU;
    float3 normal = normalize(cross(ddx_val, ddy_val));

    // === PBR血ライティング ===
    float3 lightDir = normalize(LightDir);
    float3 viewDir = float3(0, 0, -1);

    // ディフューズ(ハーフランバート)
    float NdotL = saturate(dot(normal, -lightDir));
    float diffuse = NdotL * 0.5f + 0.5f;

    // スペキュラ(Blinn-Phong)
    float3 halfVec = normalize(-lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfVec));
    float specular = pow(NdotH, 32.0f) * 2.0f;

    // フレネル
    float NdotV = saturate(dot(normal, -viewDir));
    float fresnel = pow(1.0f - NdotV, 2.0f) * 0.6f;

    // === 血の色 ===
    float3 freshBlood = float3(0.6f, 0.02f, 0.02f);
    float3 darkBlood = float3(0.25f, 0.01f, 0.01f);

    float thicknessFactor = saturate(Thickness);
    float3 bloodColor = lerp(freshBlood, darkBlood, thicknessFactor * 0.5f);

    // === 最終合成 ===
    float3 litColor = bloodColor * diffuse;
    litColor += float3(1.0f, 0.7f, 0.5f) * specular;
    litColor += float3(0.8f, 0.15f, 0.05f) * fresnel;

    // HDRトーンマッピング
    litColor = litColor / (litColor + 0.3f);

    // // αブレンドで出力(FluidAlphaで透明度を制御)
    return float4(litColor, FluidAlpha);
}
