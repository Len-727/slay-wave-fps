// ============================================================
//  StunRingVS.hlsl
//  Gothic Swarm - DOOM Eternal 風スタンリング ビルボード頂点シェーダー
//
//  【役割】
//  敵の位置にカメラ向きの四角形 (ビルボード) を展開する。
//  View 行列の右・上ベクトルを使って常にカメラを向く。
// ============================================================

cbuffer RingCB : register(b0)
{
    matrix View;        // ビュー行列
    matrix Projection;  // プロジェクション行列
    float3 EnemyPos;    // 敵のワールド位置
    float  RingSize;    // リングの半径
    float  Time;        // アニメーション用の時間
    float3 Padding;     // 16 バイト境界揃え
};

struct VSInput
{
    float3 Position : POSITION;   // -1〜+1 のローカル座標
    float2 TexCoord : TEXCOORD0;  //  0〜1 の UV 座標
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // === ビルボード: View 行列からカメラの右・上を取得 ===
    float3 right = float3(View[0][0], View[1][0], View[2][0]);
    float3 up    = float3(View[0][1], View[1][1], View[2][1]);

    // 敵の位置を中心にクワッドを展開
    float3 worldPos = EnemyPos
                    + right * input.Position.x * RingSize
                    + up    * input.Position.y * RingSize;

    float4 viewPos  = mul(float4(worldPos, 1.0f), View);
    output.Position = mul(viewPos, Projection);
    output.TexCoord = input.TexCoord;

    return output;
}
