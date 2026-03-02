// ============================================================
//  BloodFluidCompositePS.hlsl
//  Pass 3: 合成パス - ぼかした深度から法線復元 → PBR血ライティング
//
//  【何をしてるか？】
//  ブラー済み深度バッファから「表面の向き(法線)」を逆算し、
//  その法線を使ってPBRライティングを適用する。
//  → 個々のパーティクルが消えて、ぬるっとした液体表面になる
// ============================================================

// --- フルスクリーンクワッドの入力 ---
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// --- テクスチャ ---
Texture2D<float> BlurredDepth : register(t0); // ブラー済み深度(Pass 2の出力)
Texture2D<float4> SceneColor : register(t1); // 元のシーンカラー(背景)
SamplerState PointSampler : register(s0); // 点サンプリング

// --- 定数バッファ ---
cbuffer CompositeCB : register(b0)
{
    float4x4 InvProjection; // プロジェクション行列の逆(深度→ビュー空間変換)
    float2 TexelSize; // 1ピクセルのUVサイズ
    float Time; // 経過時間(アニメーション用)
    float Thickness; // 血の厚み(色の濃さに影響)
    float3 LightDir; // 光源の方向
    float SpecularPower; // スペキュラの鋭さ
    float3 CameraPos; // カメラの位置
    float FluidAlpha; // 流体の不透明度
};

// === ヘルパー: 深度からビュー空間座標を復元 ===
float3 ReconstructViewPos(float2 uv, float depth)
{
    // UV(0~1) → クリップ空間(-1~+1)
    float4 clipPos;
    clipPos.x = uv.x * 2.0f - 1.0f;
    clipPos.y = (1.0f - uv.y) * 2.0f - 1.0f; // Y反転(DX座標系)
    clipPos.z = depth;
    clipPos.w = 1.0f;

    // クリップ空間 → ビュー空間
    float4 viewPos = mul(clipPos, InvProjection);
    return viewPos.xyz / viewPos.w; // パースペクティブ除算
}

float4 main(PSInput input) : SV_Target
{
    // === 中心ピクセルの深度を取得 ===
    float depth = BlurredDepth.Sample(PointSampler, input.texcoord);

    // 深度がない(=血がない)ピクセルはそのままシーンカラーを返す
    if (depth <= 0.0f || depth >= 1.0f)
    {
        return SceneColor.Sample(PointSampler, input.texcoord);
    }

    // === 周囲4ピクセルの深度から法線を復元(中心差分) ===
    // 右隣と左隣の深度差 → X方向の傾き
    // 上隣と下隣の深度差 → Y方向の傾き
    // この2つの傾きの外積 → 表面の法線ベクトル
    float depthL = BlurredDepth.Sample(PointSampler, input.texcoord + float2(-TexelSize.x, 0));
    float depthR = BlurredDepth.Sample(PointSampler, input.texcoord + float2(TexelSize.x, 0));
    float depthU = BlurredDepth.Sample(PointSampler, input.texcoord + float2(0, -TexelSize.y));
    float depthD = BlurredDepth.Sample(PointSampler, input.texcoord + float2(0, TexelSize.y));

    // ビュー空間座標を復元
    float3 posC = ReconstructViewPos(input.texcoord, depth);
    float3 posL = ReconstructViewPos(input.texcoord + float2(-TexelSize.x, 0), depthL);
    float3 posR = ReconstructViewPos(input.texcoord + float2(TexelSize.x, 0), depthR);
    float3 posU = ReconstructViewPos(input.texcoord + float2(0, -TexelSize.y), depthU);
    float3 posD = ReconstructViewPos(input.texcoord + float2(0, TexelSize.y), depthD);

    // 差分ベクトル
    float3 ddx = posR - posL; // X方向の傾き
    float3 ddy = posD - posU; // Y方向の傾き

    // 外積 → 法線(正規化)
    float3 normal = normalize(cross(ddx, ddy));

    // === PBR血ライティング ===

    // 光源方向(正規化)
    float3 lightDir = normalize(LightDir);
    float3 viewDir = float3(0, 0, -1); // カメラ方向(ビュー空間では常にZ-)

    // --- ディフューズ(拡散反射) ---
    // 表面が光に向いてるほど明るい
    float NdotL = saturate(dot(normal, -lightDir));
    float diffuse = NdotL * 0.6f + 0.4f; // 0.4は環境光(真っ暗にならないように)

    // --- スペキュラ(鏡面反射) ---
    // Blinn-Phong: ハーフベクトルと法線の角度で反射の強さを計算
    float3 halfVec = normalize(-lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfVec));
    float specular = pow(NdotH, SpecularPower) * 1.5f; // 鋭いハイライト

    // --- フレネル効果(端ほど反射が強い) ---
    // 血の表面を斜めから見ると、より光が反射する(現実の液体と同じ)
    float NdotV = saturate(dot(normal, -viewDir));
    float fresnel = pow(1.0f - NdotV, 3.0f) * 0.5f;

    // --- SSS風の厚み表現 ---
    // 深度が深い(=厚い)ところは暗く、浅い(=薄い)ところは明るい赤
    float thicknessFactor = saturate(Thickness * 2.0f);

    // 血の色
    float3 darkBlood = float3(0.08f, 0.003f, 0.003f); // 厚い部分(ドス黒い赤)
    float3 brightBlood = float3(0.55f, 0.025f, 0.005f); // 薄い部分(鮮やかな赤)

    float3 bloodColor = lerp(brightBlood, darkBlood, thicknessFactor);

    // --- 合成 ---
    float3 litColor = bloodColor * diffuse; // ディフューズ
    litColor += float3(1.0f, 0.85f, 0.7f) * specular; // スペキュラ(暖色)
    litColor += float3(0.8f, 0.2f, 0.1f) * fresnel; // フレネル(赤みがかった反射)

    // HDRトーンマッピング(白飛び防止)
    litColor = litColor / (litColor + 0.5f);

    // === シーンカラーとブレンド ===
    float4 scene = SceneColor.Sample(PointSampler, input.texcoord);
    float alpha = FluidAlpha; // 流体の不透明度

    float3 finalColor = lerp(scene.rgb, litColor, alpha);

    return float4(finalColor, 1.0f);
}
