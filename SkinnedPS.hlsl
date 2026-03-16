// SkinnedPS.hlsl - ピクセルシェーダー

// テクスチャとサンプラー
Texture2D DiffuseTexture : register(t0); // テクスチャ画像
SamplerState LinearSampler : register(s0); // テクスチャの読み方（補間方法）

// ライト定数バッファ（VSと同じ構造）
cbuffer LightBuffer : register(b2)
{
    float4 AmbientColor; // 環境光の色
    float4 DiffuseColor; // 拡散光の色
    float3 LightDirection; // ライトの方向
    float Padding; // パディング
    float3 CameraPos; // カメラのワールド位置（リムライト計算用）
    float Padding2; // パディング
};

// 頂点シェーダーからの入力
struct PSInput
{
    float4 Position : SV_POSITION; // スクリーン座標
    float3 Normal : NORMAL; // ワールド法線
    float2 TexCoord : TEXCOORD0; // UV座標
    float4 Color : COLOR0; // ライティング済みの色
    float3 WorldPos : TEXCOORD1; // ワールド座標
};

// === メイン関数（画面上の各ピクセルに対して並列実行）===
float4 main(PSInput input) : SV_TARGET
{
    // テクスチャから色を取得
    float4 texColor = DiffuseTexture.Sample(LinearSampler, input.TexCoord);

    // ベースカラー = テクスチャ × ライティング色
    float4 baseColor = texColor * float4(input.Color.rgb, 1.0);

    // ====================================================
    //  リムライト（スタガー中の敵だけ）
    // ====================================================
    // Color.a の使い方:
    //   1.0        = 通常状態（リムライトなし）
    //   2.0 以上   = スタガー中（リムライトあり）
    //   小数部分   = パルスアニメーション用の位相

    float rimFlag = input.Color.a; // スタガーフラグ
    bool isStaggered = (rimFlag > 1.5); // 1.5以上ならスタガー中

    if (isStaggered)
    {
        // ---  視線方向を計算 ---
        float3 viewDir = normalize(CameraPos - input.WorldPos);

        // ---  法線を正規化 ---
        // 「この面がどっちを向いているか」
        float3 normal = normalize(input.Normal);

        // ---  リムライト計算 ---
        float NdotV = saturate(dot(normal, viewDir));

        // 1.0 - NdotV で「縁ほど大きい値」にする
        float rim = 1.0 - NdotV;
        
        rim = pow(rim, 2.5);

        // ---  パルスアニメーション ---
        // rimFlag の小数部分を使って脈動させる
        float phase = frac(rimFlag); // 0.0～1.0 の周期
        float pulse = sin(phase * 6.28318) * 0.25 + 0.75; // 0.5～1.0 で脈動

        // ---  紫のリムカラーを加算 ---
        float3 rimColor = float3(0.6, 0.05, 1.0);
        float rimStrength = 2.5; // 光の強さ（調整可能）

        baseColor.rgb += rimColor * rim * rimStrength * pulse;
    }

    // アルファが0に近いピクセルは描画しない（透明部分を切り抜く）
    clip(baseColor.a - 0.1);

    // アルファを1.0に戻す（スタガーフラグの値が最終出力に影響しないように）
    baseColor.a = texColor.a;

    return baseColor;
}

