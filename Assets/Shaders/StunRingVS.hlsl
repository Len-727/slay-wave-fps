// StunRingVS.hlsl - ビルボード頂点シェーダー
// 敵の位置にカメラ向きの四角形を作る

// === 定数バッファ ===
cbuffer RingCB : register(b0)
{
    matrix View; // ビュー行列
    matrix Projection; // プロジェクション行列
    float3 EnemyPos; // 敵のワールド位置
    float RingSize; // リングの大きさ（半径）
    float Time; // アニメーション用の時間
    float3 Padding; // 16バイト境界に揃える
};

// === 頂点入力 ===
// 4頂点のクワッド（-1?+1の正方形）
struct VSInput
{
    float3 Position : POSITION; // -1?+1 のローカル座標
    float2 TexCoord : TEXCOORD0; //  0?1 のUV座標
};

// === 頂点出力 ===
struct VSOutput
{
    float4 Position : SV_POSITION; // スクリーン座標
    float2 TexCoord : TEXCOORD0; // UV座標（PSでリング計算に使う）
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // === ビルボード計算 ===
    // View行列の上3x3を転置 = カメラの回転の逆 = 常にカメラを向く
    //
    // View行列の構造:
    //   [Right.x  Right.y  Right.z  0]
    //   [Up.x     Up.y     Up.z     0]
    //   [Fwd.x    Fwd.y    Fwd.z    0]
    //   [...      ...      ...      1]
    //
    // 右方向（カメラのX軸）と上方向（カメラのY軸）を取得
    float3 right = float3(View[0][0], View[1][0], View[2][0]); // カメラの右方向
    float3 up = float3(View[0][1], View[1][1], View[2][1]); // カメラの上方向

    // 敵の位置を中心に、カメラ向きのクワッドを展開
    float3 worldPos = EnemyPos
                    + right * input.Position.x * RingSize // 左右に広げる
                    + up * input.Position.y * RingSize; // 上下に広げる

    // ワールド → ビュー → プロジェクション
    float4 viewPos = mul(float4(worldPos, 1.0), View);
    output.Position = mul(viewPos, Projection);

    // UV座標をそのまま渡す
    output.TexCoord = input.TexCoord;

    return output;
}
