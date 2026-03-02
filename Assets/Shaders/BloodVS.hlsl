// ============================================================
//  BloodVS.hlsl
//  血パーティクル／デカール専用 頂点シェーダー
//  
//  【役割】頂点をワールド→スクリーン座標に変換し、
//         ライティングに必要な情報をピクセルシェーダーへ渡す
// ============================================================

// --- 定数バッファ（CPU側から毎フレーム送られるデータ）---
cbuffer BloodCB : register(b0)
{
    float4x4 WorldViewProj; // ワールド × ビュー × プロジェクション合成行列
    float3 CameraPos; // カメラのワールド座標
    float Time; // ゲーム経過時間（秒）
    float3 LightDir; // メインライト方向（正規化済み、太陽の向き）
    float ScreenMode; // 0.0=3Dデカール  1.0=2Dスクリーンブラッド
};

// --- 頂点シェーダーへの入力 ---
struct VS_INPUT
{
    float3 Pos : POSITION; // 頂点位置（ワールド or スクリーン座標）
    float4 Color : COLOR0; // 頂点カラー（RGBAで色＋透明度）
    float2 UV : TEXCOORD0; // テクスチャ座標（0?1）
};

// --- ピクセルシェーダーへの出力 ---
struct PS_INPUT
{
    float4 Pos : SV_POSITION; // スクリーン座標（GPUが使う）
    float4 Color : COLOR0; // 頂点カラー（そのまま渡す）
    float2 UV : TEXCOORD0; // テクスチャ座標（そのまま渡す）
    float3 WorldPos : TEXCOORD1; // ワールド座標（ライティング計算用）
};

// ============================================================
//  メイン関数
// ============================================================
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    
    // 頂点位置をスクリーン座標に変換
    output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
    
    // 残りはそのままピクセルシェーダーへ
    output.Color = input.Color;
    output.UV = input.UV;
    output.WorldPos = input.Pos; // ライティング用にワールド座標も渡す
    
    return output;
}
