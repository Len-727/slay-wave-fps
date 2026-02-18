// =============================================================
// FurPS.hlsl - ファーシェーダー（苔・草用）
// ノイズ方式 + シャープな境界で自然な苔を表現
// =============================================================

cbuffer FurParams : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    
    float FurLength;
    float CurrentLayer;
    float TotalLayers;
    float Time;
    
    float3 WindDirection;
    float WindStrength;
    
    float3 BaseColor;
    float FurDensity;
    float3 TipColor;
    float Padding2;
    
    float3 LightDir;
    float AmbientStrength;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float Layer : TEXCOORD1;
    float3 WorldPos : TEXCOORD2;
    float3 WorldNormal : TEXCOORD3;
};

// --- ノイズ関数 ---
float hash21(float2 p)
{
    p = frac(p * float2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return frac(p.x * p.y);
}

float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + float2(1, 0));
    float c = hash21(i + float2(0, 1));
    float d = hash21(i + float2(1, 1));
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float fbm(float2 p)
{
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 3; i++)
    {
        v += a * noise(p);
        p *= 2.17;
        a *= 0.48;
    }
    return v;
}

// =============================================================
float4 main(PS_INPUT input) : SV_TARGET
{
    float layer = input.Layer;
    
    // ==============================================
    // ストランド判定
    // ==============================================
    
    // 2つの異なるスケールのFBMを組み合わせる
    // 細かいノイズ（毛の粒感）+ 大きいノイズ（苔の塊感）
    float fine = fbm(input.TexCoord * 60.0 + float2(3.7, 1.2));
    float coarse = fbm(input.TexCoord * 8.0 + float2(9.1, 4.3));
    
    // 合成（細かいのをメインに、粗いので変調）
    float furNoise = fine * 0.7 + coarse * 0.3;
    
    // 閾値（先端ほどまばら）
    float threshold = FurDensity * (1.0 - layer * layer);
    
    // smoothstepでシャープな境界（のっぺり防止）
    float strandMask = 1.0 - smoothstep(threshold - 0.05, threshold + 0.05, furNoise);
    
    // ほぼ透明ならdiscard
    if (strandMask < 0.05)
    {
        discard;
    }
    
    // ==============================================
    // 色計算
    // ==============================================
    
    // ★coarseはストランド判定で既に計算済みだから使い回す
    float3 furColor = lerp(BaseColor, TipColor, layer);
    
    // ★colorVarもfbmじゃなくnoiseで十分
    float colorVar = noise(input.TexCoord * 45.0 + 7.1);
    furColor *= (0.65 + colorVar * 0.7);
    
    // patchはcoarseを再利用
    float patch = smoothstep(0.3, 0.7, coarse);
    furColor *= (0.55 + patch * 0.65);
    
    // ==============================================
    // ライティング
    // ==============================================
    float3 normal = normalize(input.WorldNormal);
    float3 ld = normalize(-LightDir);
    float diffuse = max(dot(normal, ld), 0.0);
    
    float selfShadow = lerp(0.15, 1.0, layer);
    float lighting = AmbientStrength + diffuse * selfShadow;
    furColor *= lighting;
    
    // ==============================================
    // アルファ
    // ==============================================
    float alpha = strandMask * (1.0 - layer * 0.5);
    alpha = saturate(alpha);
    
    return float4(furColor, alpha);
}
