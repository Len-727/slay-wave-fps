// ============================================================
//  FireParticleVS_Standalone.hlsl
//  Gothic Swarm - 炎パーティクル 頂点シェーダー (Standalone)
//
//  【役割】
//  パーティクルの中心位置にカメラ向きのビルボードを生成。
//  View 行列から右・上ベクトルを抽出してビルボード計算を行う。
// ============================================================

cbuffer MatrixBuffer : register(b0)
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
};

struct VSInput
{
    float3 Position : POSITION;   // パーティクル中心位置
    float2 TexCoord : TEXCOORD0;  // UV (0〜1)
    float4 Color    : COLOR0;     // パーティクル色
    float  Size     : PSIZE;      // パーティクルサイズ
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
};

// --- ビルボードの UV → オフセット変換 ---
static const float UV_CENTER = 0.5f;

VSOutput main(VSInput input)
{
    VSOutput output;

    // View 行列からカメラの右・上ベクトルを取得
    float3 right = float3(ViewMatrix._11, ViewMatrix._21, ViewMatrix._31);
    float3 up    = float3(ViewMatrix._12, ViewMatrix._22, ViewMatrix._32);

    // UV からオフセットを計算 (中心 = 0.5 → -0.5〜+0.5)
    float2 offset = (input.TexCoord - UV_CENTER) * input.Size;

    // ビルボード頂点位置
    float3 worldPos = input.Position + right * offset.x + up * offset.y;

    // 座標変換
    float4 viewPos  = mul(float4(worldPos, 1.0f), ViewMatrix);
    output.Position = mul(viewPos, ProjectionMatrix);
    output.TexCoord = input.TexCoord;
    output.Color    = input.Color;

    return output;
}
