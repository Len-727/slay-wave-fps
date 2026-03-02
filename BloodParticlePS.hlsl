// ============================================================
//  BloodParticlePS.hlsl
//  Pixel Shader - PBR風の血レンダリング(GPUパーティクル用)
//
//  BloodPS.hlslと同じ視覚スタイル:
//  - SSS風の厚み表現(中心が暗く、端が明るい)
//  - 濡れたスペキュラハイライト
//  - ソフトな円形マスク(テクスチャ不要)
// ============================================================

// --- 頂点シェーダーからの入力 ---
struct PSInput
{
    float4 position : SV_POSITION; // スクリーン上の位置
    float2 texcoord : TEXCOORD0; // UV座標
    float4 color : COLOR0; // 頂点カラー
    float life : TEXCOORD1; // 寿命の割合
};

float4 main(PSInput input) : SV_TARGET
{
    // === UVから円形マスクを生成 ===
    float2 centered = input.texcoord * 2.0f - 1.0f; // -1 ~ +1 に変換
    float dist = length(centered); // 中心からの距離

    // 円の外側のピクセルは捨てる
    if (dist > 1.0f)
        discard;

    // 端に向かってなめらかにフェードアウト
    float softEdge = 1.0f - smoothstep(0.5f, 1.0f, dist);

    // === SSS風の厚み表現 ===
    // thickness: 中心=1(厚い=暗い), 端=0(薄い=明るい)
    float thickness = 1.0f - dist;
    thickness = thickness * thickness; // 二乗で非線形にして中心をより暗く

    float3 darkBlood = float3(0.10f, 0.005f, 0.005f); // 厚い部分(ドス黒い赤)
    float3 brightBlood = float3(0.55f, 0.025f, 0.0f); // 薄い部分(鮮血)

    // 厚みに応じて色をブレンド
    float3 bloodColor = lerp(brightBlood, darkBlood, thickness);

    // === 濡れたスペキュラハイライト(テカリ) ===
    // UVから疑似法線を生成(球面っぽい表面)
    float3 fakeNormal = normalize(float3(centered.x * 0.6f, centered.y * 0.6f, 1.0f - dist * 0.3f));
    float3 lightDir = normalize(float3(0.3f, -0.8f, 0.5f)); // 斜め上からの光
    float3 viewDir = float3(0, 0, -1); // カメラ方向(簡易)
    float3 halfVec = normalize(-lightDir + viewDir); // ハーフベクトル

    // Blinn-Phongスペキュラ: 指数128で鋭いハイライト
    float NdotH = saturate(dot(fakeNormal, halfVec));
    float specular = pow(NdotH, 128.0f) * 2.0f;

    // 暖色系のスペキュラ色(血の反射は少しオレンジがかる)
    float3 specColor = float3(1.0f, 0.85f, 0.7f) * specular;

    // === 最終合成 ===
    float3 finalColor = bloodColor + specColor;

    // HDRトーンマッピング(白飛び防止)
    finalColor = finalColor / (finalColor + 0.5f);

    // アルファ: 頂点カラーのアルファ x ソフトエッジ
    float alpha = input.color.a * softEdge;

    return float4(finalColor, alpha);
}
