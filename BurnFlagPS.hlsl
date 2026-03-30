// ========================================
// BurnFlagPS.hlsl
// ========================================

// テクスチャ
Texture2D flagTexture : register(t0); // 旗のテクスチャ
Texture2D noiseTexture : register(t1); // ノイズテクスチャ（燃え広がりパターン）
SamplerState samplerState : register(s0);

// 定数バッファ（C++側と一致）
cbuffer BurnParams : register(b0)
{
    float burnProgress; // 燃焼進行度（0.0 = 燃えてない, 1.0 = 完全燃焼）
    float time; // 時間（揺らぎ用）
    float emberWidth; // 燃え際の幅（0.05～0.15推奨）
    float burnDirection; // 燃える方向（予備）
};

// 入力（PostProcessVSに合わせてNORMALなし）
struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// ========================================
// ハッシュ関数（火花のランダム位置に使う）
// ========================================
float hash(float2 p)
{
    float h = dot(p, float2(127.1, 311.7));
    return frac(sin(h) * 43758.5453);
}

// ========================================
// メイン関数
// ========================================
float4 main(PS_INPUT input) : SV_TARGET
{
    // === 1) テクスチャサンプリング ===
    float4 flagColor = flagTexture.Sample(samplerState, input.texCoord);
    
    // ノイズをサンプリング（時間で少しずらして揺らぎを出す）
    float2 noiseUV = input.texCoord + float2(sin(time * 0.5) * 0.01, cos(time * 0.3) * 0.01);
    float noise = noiseTexture.Sample(samplerState, noiseUV).r;
    
    // === 2) 燃焼方向による補正 ===
    float directionBias = 0.0;
    
    // 下から上へ燃える（Y座標を考慮）
    directionBias = (1.0 - input.texCoord.y) * 0.3;
    
    // 左から右へも少し（X座標を考慮）
    directionBias += input.texCoord.x * 0.2;
    
    // ノイズと方向バイアスを組み合わせ
    float burnValue = noise * 0.7 + directionBias;
    
    // === 3) 燃焼状態の判定 ===
    float burnThreshold = burnProgress * 1.3; // 進行度を少し拡張
    
    // 燃え際の範囲
    float emberStart = burnThreshold - emberWidth;
    float emberEnd = burnThreshold;
    
    // === 4) 色の計算 ===
    float4 finalColor;
    
    if (burnValue < emberStart)
    {
        // ========================================
        // 完全に燃えた部分 → 透明（穴）
        // ========================================
        finalColor = float4(0, 0, 0, 0);
    }
    else if (burnValue < emberEnd)
    {
        // ========================================
        // 燃え際（エンバー）→ DOOM風の激しいオレンジ炎
        // ========================================
        float emberT = (burnValue - emberStart) / emberWidth;
        
        // 揺らぎを//（激しい炎のちらつき）
        float flicker = sin(time * 20.0 + noise * 25.0) * 0.15 + 0.85;
        float flicker2 = sin(time * 35.0 + noise * 15.0) * 0.08;
        flicker += flicker2;
        
        // DOOM風カラーグラデーション（黒→赤→オレンジ→黄→白熱）
        float3 emberColor;
        if (emberT < 0.15)
        {
            // 黒 → 暗い赤（焦げ始め）
            emberColor = lerp(
                float3(0.1, 0.02, 0.0),
                float3(0.5, 0.08, 0.0),
                emberT / 0.15
            );
        }
        else if (emberT < 0.35)
        {
            // 暗い赤 → 鮮やかなオレンジ
            emberColor = lerp(
                float3(0.5, 0.08, 0.0),
                float3(1.0, 0.35, 0.02),
                (emberT - 0.15) / 0.2
            );
        }
        else if (emberT < 0.6)
        {
            // オレンジ → 明るい黄色
            emberColor = lerp(
                float3(1.0, 0.35, 0.02),
                float3(1.0, 0.7, 0.1),
                (emberT - 0.35) / 0.25
            );
        }
        else if (emberT < 0.85)
        {
            // 黄色 → 白熱（ほぼ白）
            emberColor = lerp(
                float3(1.0, 0.7, 0.1),
                float3(1.0, 0.9, 0.5),
                (emberT - 0.6) / 0.25
            );
        }
        else
        {
            // 白熱のコア（最も明るい、燃えてない部分との境界）
            emberColor = lerp(
                float3(1.0, 0.9, 0.5),
                float3(1.0, 0.95, 0.7),
                (emberT - 0.85) / 0.15
            );
        }
        
        // フリッカー適用
        emberColor *= flicker;
        
        // 発光効果（HDR風 = 明るい部分がさらに明るく）
        float glow = pow(emberT, 1.5) * 2.0;
        emberColor += float3(glow * 0.4, glow * 0.15, glow * 0.02);
        
        // 火花エフェクト（燃え際にランダムな明るい点）
        float spark = hash(floor(input.texCoord * 200.0) + floor(time * 8.0));
        spark = step(0.97, spark) * step(0.3, emberT);
        emberColor += float3(1.0, 0.8, 0.3) * spark * 1.5;
        
        finalColor = float4(saturate(emberColor), 1.0);
    }
    else if (burnValue < emberEnd + 0.06)
    {
        // ========================================
        // 焦げ始め → 元の色が暗くなる＋オレンジの縁光
        // ========================================
        float charT = (burnValue - emberEnd) / 0.06;
        
        // 元のテクスチャに焦げ色を混ぜる
        float3 charColor = lerp(
            float3(0.2, 0.08, 0.02), // 焦げた暗い色
            flagColor.rgb * 0.6, // 元の色（少し暗く）
            charT
        );
        
        // 焦げ際にオレンジの縁光（熱が伝わってる感じ）
        float edgeGlow = (1.0 - charT) * 0.3;
        charColor += float3(edgeGlow, edgeGlow * 0.3, 0.0);
        
        finalColor = float4(charColor, flagColor.a);
    }
    else
    {
        // ========================================
        // 燃えてない部分 → 元のテクスチャ
        // ========================================
        // 全体にわずかな熱の影響（近くが燃えてるので少し暖色に）
        float heatProximity = smoothstep(emberEnd + 0.2, emberEnd + 0.06, burnValue);
        float3 heated = flagColor.rgb + float3(0.05, 0.015, 0.0) * heatProximity;
        
        finalColor = float4(heated, flagColor.a);
    }
    
    return finalColor;
}
