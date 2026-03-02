// ============================================================
//  BloodFluidCompositePS.hlsl
//  Pass 3: 合成パス - ぼかした深度から法線復元 → PBR血ライティング
//
//  【修正点】
//  - 血の色を大幅に明るく(暗すぎて見えなかった)
//  - Thickness計算を修正(×2で常にdarkBloodだった)
//  - スペキュラを柔らかく・見えやすく
//  - デバッグモードのコメントを残す
// ============================================================

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// --- テクスチャ ---
Texture2D<float> BlurredDepth : register(t0); // ブラー済み深度
Texture2D<float4> SceneColor : register(t1); // 元のシーンカラー
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

// === ヘルパー: 深度からビュー空間座標を復元 ===
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

    // 深度がない = 血がないピクセル → シーンカラーをそのまま返す
    if (depth <= 0.0f || depth >= 1.0f)
    {
        return SceneColor.SampleLevel(PointSampler, input.texcoord, 0);
    }

    // ★ デバッグ: 血がある場所を蛍光グリーンで表示(問題切り分け用)
    // 下の1行のコメントを外すと血の位置が一目でわかる
    return float4(0.0f, 1.0f, 0.0f, 1.0f);

    // === 周囲4ピクセルの深度から法線を復元(中心差分) ===
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

    float NdotL = saturate(dot(normal, -lightDir));
    float diffuse = NdotL * 0.5f + 0.5f;

    float3 halfVec = normalize(-lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfVec));
    float specular = pow(NdotH, 32.0f) * 2.0f;

    float NdotV = saturate(dot(normal, -viewDir));
    float fresnel = pow(1.0f - NdotV, 2.0f) * 0.6f;

    // === 血の色 ===
    float3 freshBlood = float3(0.6f, 0.02f, 0.02f);
    float3 darkBlood = float3(0.25f, 0.01f, 0.01f);

    float thicknessFactor = saturate(Thickness);
    float3 bloodColor = lerp(freshBlood, darkBlood, thicknessFactor * 0.5f);

    float3 litColor = bloodColor * diffuse;
    litColor += float3(1.0f, 0.7f, 0.5f) * specular;
    litColor += float3(0.8f, 0.15f, 0.05f) * fresnel;

    litColor = litColor / (litColor + 0.3f);

    float4 scene = SceneColor.SampleLevel(PointSampler, input.texcoord, 0);
    float3 finalColor = lerp(scene.rgb, litColor, FluidAlpha);

    return float4(finalColor, 1.0f);
}
