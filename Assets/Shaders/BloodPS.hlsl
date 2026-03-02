// ============================================================
//  BloodShader_PS.hlsl
//  血パーティクル／デカール専用 ピクセルシェーダー
//
//  【3つの核心技術】
//  1. SSS風の厚み表現 ? 中心=黒赤、端=鮮血で「液体の厚み」を演出
//  2. スペキュラ反射   ? 超低ラフネスで「濡れたテカテカ感」
//  3. フレネル効果     ? 端ほど反射が強い「液体の表面張力」表現
// ============================================================

// --- 定数バッファ（頂点シェーダーと同じ構造体を共有）---
cbuffer BloodCB : register(b0)
{
    float4x4 WorldViewProj;
    float3 CameraPos;
    float Time;
    float3 LightDir;
    float ScreenMode; // 0.0=3Dデカール  1.0=2Dスクリーンブラッド
};

// --- テクスチャ ---
Texture2D BloodTex : register(t0); // 血スプラッターテクスチャ
SamplerState BloodSampler : register(s0); // サンプラー

// --- ピクセルシェーダーへの入力（VSからの出力）---
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float2 UV : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

// ============================================================
//  メイン関数
// ============================================================
float4 main(PS_INPUT input) : SV_TARGET
{
    // ==== テクスチャサンプリング ====
    float4 texColor = BloodTex.Sample(BloodSampler, input.UV);
    float alpha = texColor.a * input.Color.a;
    
    // 透明ピクセルは即破棄（パフォーマンス最適化）
    if (alpha < 0.01f)
        discard;

    // ==== UV中心からの距離（液体の「厚み」に使う）====
    // UV(0.5, 0.5)が中心 → centered は -1?+1
    float2 centered = input.UV * 2.0f - 1.0f;
    float distFromCenter = length(centered);
    
    // ========================================================
    //  スクリーンブラッドモード（2D）
    //  → ライティングなし、SSS風の色だけ適用
    // ========================================================
    if (ScreenMode > 0.5f)
    {
        // SSS風ベースカラー
        float thickness = saturate(1.0f - distFromCenter);
        float3 darkBlood = float3(0.12f, 0.005f, 0.005f); // 厚い: 黒赤
        float3 brightBlood = float3(0.55f, 0.02f, 0.0f); // 薄い: 鮮血
        float3 baseColor = lerp(brightBlood, darkBlood, thickness * thickness);
        
        // エッジに微かな明るいリム（表面張力の光）
        float rim = pow(saturate(distFromCenter - 0.3f), 2.0f) * 0.3f;
        baseColor += float3(0.3f, 0.02f, 0.0f) * rim;
        
        // 頂点カラーで色味調整
        baseColor *= input.Color.rgb * 2.5f;
        
        return float4(baseColor, alpha);
    }
    
    // ========================================================
    //  3Dデカールモード ? フルライティング
    // ========================================================
    
    // ==== 疑似法線の生成 ====
    // テクスチャのUVから「液体の丸み」を法線として作る
    // 中心は上向き(0,0,1)、端は外向きに傾く → 球面っぽい反射
    float3 normal = normalize(float3(
        centered.x * 0.7f, // X方向の傾き
        centered.y * 0.7f, // Y方向の傾き  
        1.0f - distFromCenter * 0.4f // 端ほどZ成分が弱い
    ));
    
    // ==== SSS風のベースカラー（厚み表現）====
    // 中心（厚い部分）: ドス黒い赤 ? 光が内部で吸収される
    // 端（薄い部分）:   鮮やかな赤 ? 光が透過して明るく見える
    float thickness = saturate(1.0f - distFromCenter);
    float3 darkBlood = float3(0.10f, 0.005f, 0.005f); // ほぼ黒
    float3 brightBlood = float3(0.60f, 0.03f, 0.0f); // 鮮血
    float3 baseColor = lerp(brightBlood, darkBlood, thickness * thickness);
    
    // 微妙な色バリエーション（Time使って少しだけ揺らぐ）
    float colorShift = sin(Time * 0.5f + input.WorldPos.x * 2.0f) * 0.02f;
    baseColor.r += colorShift;
    
    // ==== ディフューズライティング（ハーフランバート）====
    // 通常のランバート(NdotL)だと裏面が真っ黒になる
    // ハーフランバートは 0?1 を 0.5?1.0 にリマップ → 暗い部分も見える
    float NdotL = dot(normal, -LightDir);
    float diffuse = NdotL * 0.4f + 0.6f; // 最低でも0.2の明るさを保証
    
    // ==== スペキュラ反射====
    // Blinn-Phongモデル: ハーフベクトルと法線の内積で反射の強さを決める
    // specPower が高い → 反射が鋭い → 濡れて見える
    float3 viewDir = normalize(CameraPos - input.WorldPos);
    float3 halfVec = normalize(-LightDir + viewDir);
    float NdotH = saturate(dot(normal, halfVec));
    
    float specPower = 196.0f; // 超高い = 鏡面に近い濡れ感
    float specular = pow(NdotH, specPower) * 3.0f; // 強めのハイライト
    
    // 第2ハイライト（広い柔らかい反射 ? 血の「ぬめり」）
    float specBroad = pow(NdotH, 16.0f) * 0.15f;
    
    // ==== フレネル効果（端ほど反射が強い）====
    // 液体は視線が浅い角度ほど鏡のように反射する物理現象
    // pow(1-NdotV, n) で近似。nが大きいほど効果がエッジに集中
    float NdotV = saturate(dot(normal, viewDir));
    float fresnel = pow(1.0f - NdotV, 4.0f) * 0.6f;
    
    // ==== 環境光（完全な暗闇を防ぐ）====
    float3 ambient = baseColor * 0.15f;
    
    // ==== 最終合成 ====
    float3 finalColor = ambient // 環境光
                       + baseColor * diffuse // SSS色 × ライト
                       + float3(1.0f, 0.85f, 0.7f) * specular // 鋭いハイライト（暖色）
                       + float3(0.8f, 0.6f, 0.5f) * specBroad // 広いぬめりハイライト
                       + brightBlood * fresnel; // 端の赤い輝き
    
    // 頂点カラーの色味を反映（デカールごとに色のばらつき）
    finalColor *= input.Color.rgb * 2.0f;
    
    // HDR風のトーンマッピング（白飛び防止）
    finalColor = finalColor / (finalColor + 0.5f);
    
    return float4(finalColor, alpha);
}
