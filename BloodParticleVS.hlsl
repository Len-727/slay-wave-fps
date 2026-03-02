// ============================================================
//  BloodParticleVS.hlsl
//  Vertex Shader - パーティクルをカメラに向くクワッド(四角形)に展開
//
//  頂点バッファ不要！SV_VertexIDだけで4頂点を生成
//  (1パーティクルにつき4頂点 x N個のパーティクル)
// ============================================================

// --- Compute Shaderと同じ構造体 ---
struct Particle
{
    float3 position; // ワールド座標
    float life; // 残り寿命
    float3 velocity; // 速度
    float maxLife; // 初期寿命
    float size; // サイズ
    float3 padding; // アラインメント用
};

// --- パーティクルバッファ(読み取り専用) ---
StructuredBuffer<Particle> Particles : register(t0);

// --- カメラ情報の定数バッファ ---
cbuffer CameraCB : register(b0)
{
    float4x4 View; // ビュー行列
    float4x4 Projection; // プロジェクション行列
    float3 CameraRight; // カメラの右方向ベクトル(ワールド空間)
    float Time; // 経過時間
    float3 CameraUp; // カメラの上方向ベクトル(ワールド空間)
    float Padding; // アラインメント用
};

// --- ピクセルシェーダーへの出力 ---
struct VSOutput
{
    float4 position : SV_POSITION; // クリップ空間の位置
    float2 texcoord : TEXCOORD0; // UV座標
    float4 color : COLOR0; // 頂点カラー
    float life : TEXCOORD1; // 寿命の割合(フェード用)
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;

    // === どのパーティクルの何番目の頂点か？ ===
    // 1パーティクル = 4頂点 (0,1,2,3)
    uint particleIndex = vertexID / 4; // パーティクル番号
    uint cornerIndex = vertexID % 4; // 角の番号(0-3)

    Particle p = Particles[particleIndex];

    // === 死んだパーティクル -> 退化三角形(見えない) ===
    if (p.life <= 0.0f || p.size <= 0.0f)
    {
        output.position = float4(0, 0, 0, 1);
        return output;
    }

    // === ビルボードの4つの角のオフセット ===
    // 角の配置:  0--1
    //            |  |
    //            2--3
    float2 corners[4] =
    {
        float2(-1, -1), // 左上
        float2(1, -1), // 右上
        float2(-1, 1), // 左下
        float2(1, 1) // 右下
    };

    float2 corner = corners[cornerIndex];

    // === UV座標(テクスチャ座標) ===
    output.texcoord = corner * 0.5f + 0.5f;

    // === ワールド空間でクワッドを展開(カメラに向くビルボード) ===
    float halfSize = p.size * 0.5f;
    float3 worldPos = p.position
        + CameraRight * (corner.x * halfSize) // カメラの右方向に展開
        + CameraUp * (corner.y * halfSize); // カメラの上方向に展開

    // === クリップ空間に変換 ===
    float4 viewPos = mul(float4(worldPos, 1.0f), View);
    output.position = mul(viewPos, Projection);

    // === 色: 寿命に応じてフェード ===
    float lifeRatio = saturate(p.life / p.maxLife);

    // 血の色: ドス黒い赤(厚い部分) <-> 鮮血(薄い部分)
    float3 bloodColor = lerp(
        float3(0.15f, 0.005f, 0.005f), // ほぼ黒(厚い血)
        float3(0.5f, 0.02f, 0.0f), // 鮮やかな動脈血
        lifeRatio * 0.5f
    );

    output.color = float4(bloodColor, lifeRatio * lifeRatio);
    output.life = lifeRatio;

    return output;
}
