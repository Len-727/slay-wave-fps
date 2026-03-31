// ============================================================
//  PostProcessVS.hlsl
//  Gothic Swarm - ポストプロセス用頂点シェーダー（頂点バッファ方式）
//
//  【役割】
//  頂点バッファから受け取った位置と UV をそのまま PS に渡す。
//  フルスクリーンクワッド（4 頂点 + インデックス）を使う場合用。
// ============================================================
#include "Common.hlsli"

// --- 頂点入力（頂点バッファから読む）---
struct VSInput
{
    float3 Position : POSITION;   // クリップ空間座標（-1〜+1）
    float2 TexCoord : TEXCOORD0;  // UV 座標（0〜1）
};

// --- PS に渡すデータ ---
// FullscreenVSOutput (Common.hlsli) と同じレイアウト
FullscreenVSOutput main(VSInput input)
{
    FullscreenVSOutput output;
    output.Position = float4(input.Position, 1.0f);
    output.TexCoord = input.TexCoord;
    return output;
}
