// ============================================================
//  FullscreenQuadVS.hlsl
//  Gothic Swarm - フルスクリーンクワッド頂点シェーダー
//
//  【役割】
//  SV_VertexID から 3 頂点の大三角形を生成し、画面全体を覆う。
//  ポストプロセス・ブラー・合成パスなど全てのフルスクリーン描画で共用。
//
//  【仕組み】
//  頂点バッファ不要。Draw(3,0) だけで画面全体を描画できる。
//  3 頂点が画面の 2 倍の三角形を作り、ラスタライザーが自動クリップ。
//
//  【注意】
//  旧 FullscreenVS.hlsl と統合済み。
//  C++ 側で旧シェーダーを参照していた箇所はこちらに切り替えること。
// ============================================================
#include "Common.hlsli"

FullscreenVSOutput main(uint vertexID : SV_VertexID)
{
    FullscreenVSOutput output;

    // --- UV 座標を頂点 ID から算出 ---
    //  頂点 0: UV(0, 1) → 左下
    //  頂点 1: UV(0,-1) → 左上の遥か外
    //  頂点 2: UV(2, 1) → 右の遥か外
    //  → 3 頂点で画面 (0〜1) を完全カバー
    output.TexCoord = float2(
        (vertexID == 2) ? 2.0f : 0.0f,
        (vertexID == 1) ? -1.0f : 1.0f
    );

    // --- UV → クリップ空間に変換 ---
    output.Position = float4(
        output.TexCoord.x * 2.0f - 1.0f,   // X: 0〜2 → -1〜+3
        1.0f - output.TexCoord.y * 2.0f,    // Y: 反転（DirectX 座標系）
        0.0f,                                // Z: 最前面
        1.0f                                 // W: 同次座標
    );

    return output;
}
