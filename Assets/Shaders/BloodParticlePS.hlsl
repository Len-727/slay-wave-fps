// ============================================================
//  BloodParticlePS.hlsl v3 - デュアルテクスチャ版
//
//  【2つのテクスチャを同時使用】
//  t1 = 血の霧（mist）   → 小さいパーティクル用
//  t2 = 血の液体（splash）→ 大きいパーティクル用
// ============================================================

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    float life : TEXCOORD1;
    float size : TEXCOORD2; 
};

// --- 2つのフリップブックテクスチャ ---
Texture2D MistTexture : register(t1); // 霧（小粒用）
Texture2D SplashTexture : register(t2); // 液体（大粒用）

SamplerState LinearSampler : register(s0);

// --- フリップブック設定 ---
static const float GRID_X = 8.0f;
static const float GRID_Y = 8.0f;
static const float TOTAL_FRAMES = 64.0f;



float2 CalcFlipbookUV(float2 localUV, float lifeRatio)
{
    // 寿命からフレーム番号を計算
    float progress = saturate(1.0f - lifeRatio);
    float frameFloat = progress * (TOTAL_FRAMES - 1.0f);
    uint frameIndex = (uint) frameFloat;
    
    uint col = frameIndex % (uint) GRID_X;
    uint row = frameIndex / (uint) GRID_X;
    
    float2 uv;
    uv.x = ((float) col + localUV.x) / GRID_X;
    uv.y = ((float) row + localUV.y) / GRID_Y;
    return uv;
}

float4 main(PSInput input) : SV_Target
{
    // ============================================================
    //  フリップブックUV計算（両テクスチャ共通）
    // ============================================================
    float2 flipUV = CalcFlipbookUV(input.texcoord, input.life);
    
    // ============================================================
    //  両テクスチャからサンプリング
    // ============================================================
    float4 mistColor = MistTexture.Sample(LinearSampler, flipUV);
    float4 splashColor = SplashTexture.Sample(LinearSampler, flipUV);
    
    // ============================================================
    //  サイズに基づくブレンド
    // ============================================================
    float blendFactor = smoothstep(0.15f, 0.4f, input.size);
    float4 texColor = lerp(splashColor, mistColor, blendFactor);
    
    // ============================================================
    //  テクスチャなしフォールバック
    // ============================================================
    if (texColor.a < 0.01f)
    {
        // 従来の円形パーティクル
        float2 centered = input.texcoord * 2.0f - 1.0f;
        float distSq = dot(centered, centered);
        if (distSq > 1.0f)
            discard;
        float coreFade = smoothstep(1.0f, 0.2f, sqrt(distSq));
        float alpha = input.color.a * coreFade;
        float3 fallbackColor = input.color.rgb * (0.7f + 0.3f * coreFade);
        return float4(fallbackColor, alpha);
    }
    
    // ============================================================
    //  最終出力
    // ============================================================
    float3 finalColor = texColor.rgb * input.color.rgb * 4.0f;
    float finalAlpha = texColor.a * input.color.a;
    
    return float4(finalColor, finalAlpha);
}
