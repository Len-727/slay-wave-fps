// ============================================================
//  BloodFluidDepthPS.hlsl
//  Pass 1: 深度パス - パーティクルの深度を浮動小数点テクスチャに書く
//
//  【何をしてるか？】
//  各パーティクルのビルボードを「球面状の深度」で描く。
//  出力はR32_FLOATテクスチャ(=深度を色として保存)。
//  後のブラーパスでこの深度を滑らかにすると粒が融合する。
// ============================================================

// --- 頂点シェーダーからの入力(BloodParticleVSと同じ) ---
struct PSInput
{
    float4 position : SV_POSITION; // スクリーン上の位置
    float2 texcoord : TEXCOORD0; // UV座標(0~1)
    float4 color : COLOR0; // 頂点カラー
    float life : TEXCOORD1; // 寿命の割合
};

float4 main(PSInput input) : SV_Target
{
    // === UVを-1~+1に変換(中心が0,0) ===
    float2 centered = input.texcoord * 2.0f - 1.0f;
    float distSq = dot(centered, centered); // 中心からの距離の二乗

    // === 円の外側は捨てる(四角い板の外をカット) ===
    if (distSq > 1.0f)
        discard;

    // === 球面の深度オフセット ===
    // sqrt(1 - r^2): 中心が最も手前(1.0)、端が最も奥(0.0)
    // これにより平面のビルボードが球体の深度になる
    float sphereZ = sqrt(1.0f - distSq);

    // === 深度値を出力(SV_POSITIONのzを球面で調整) ===
    // position.zはNDC深度(0=ニア, 1=ファー)
    // sphereZでオフセットすると中心が少し手前に出る
    float depth = input.position.z - sphereZ * 0.005f;
    depth = saturate(depth); // 0~1にクランプ

    // R32_FLOATに深度値を書き込む(RGBAの全チャンネルに同じ値)
    return float4(depth, depth, depth, 1.0f);
}
