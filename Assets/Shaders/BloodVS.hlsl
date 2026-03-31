// ============================================================
//  BloodVS.hlsl
//  Gothic Swarm - 血パーティクル／デカール 頂点シェーダー
//
//  【役割】
//  頂点をワールド → スクリーン座標に変換し、
//  ライティングに必要な情報を PS に中継する。
// ============================================================

// --- 定数バッファ（毎フレーム CPU から送信）---
cbuffer BloodCB : register(b0)
{
    float4x4 WorldViewProj;  // ワールド × ビュー × プロジェクション合成行列
    float3   CameraPos;      // カメラのワールド座標
    float    Time;            // 経過時間（秒）
    float3   LightDir;       // メインライト方向（正規化済み）
    float    ScreenMode;      // 0.0 = 3D デカール / 1.0 = 2D スクリーンブラッド
};

// --- 頂点入力 ---
struct VSInput
{
    float3 Position : POSITION;   // 頂点位置（ワールド or スクリーン座標）
    float4 Color    : COLOR0;     // 頂点カラー（RGBA）
    float2 TexCoord : TEXCOORD0;  // UV 座標（0〜1）
};

// --- PS に渡すデータ ---
struct VSOutput
{
    float4 Position : SV_POSITION;  // スクリーン座標（ラスタライザ用）
    float4 Color    : COLOR0;       // 頂点カラー（そのまま）
    float2 TexCoord : TEXCOORD0;    // UV（そのまま）
    float3 WorldPos : TEXCOORD1;    // ワールド座標（ライティング計算用）
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.Position = mul(float4(input.Position, 1.0f), WorldViewProj);
    output.Color    = input.Color;
    output.TexCoord = input.TexCoord;
    output.WorldPos = input.Position;  // ライティング用にワールド座標も渡す

    return output;
}
