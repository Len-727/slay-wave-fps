// ============================================================
//  BloodParticleVS.hlsl
//  Gothic Swarm - GPU 血パーティクル 頂点シェーダー
//
//  【役割】
//  SV_VertexID から「どのパーティクルの、どの角か」を計算し、
//  カメラに向いたビルボード (四角形) の頂点を生成する。
//
//  【速度ストレッチ】
//  速いパーティクルは進行方向に細長くなり、
//  遅いパーティクルは通常の正方形ビルボードになる。
// ============================================================
#include "Common.hlsli"

// --- パーティクル構造体 (CS / C++ と完全一致: 48 バイト) ---
struct Particle
{
    float3 Position;
    float  Life;
    float3 Velocity;
    float  MaxLife;
    float  Size;
    float3 Padding;
};

// --- SRV: CS が書いたパーティクルデータ ---
StructuredBuffer<Particle> Particles : register(t0);

// --- カメラ定数バッファ ---
cbuffer CameraCB : register(b0)
{
    float4x4 View;          // ビュー行列
    float4x4 Projection;    // プロジェクション行列
    float3   CameraRight;   // カメラの右方向ベクトル
    float    Time;           // 累計時間
    float3   CameraUp;      // カメラの上方向ベクトル
    float    SizeScale;      // パーティクルサイズ係数
};

// --- PS への出力 ---
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
    float  Life     : TEXCOORD1;   // 寿命比率 (0〜1)
    float  Size     : TEXCOORD2;   // パーティクルサイズ
};

// === ビルボードの 4 角テーブル ===
static const float2 CORNER_OFFSETS[4] =
{
    float2(-1.0f, +1.0f),  // 0: 左上
    float2(+1.0f, +1.0f),  // 1: 右上
    float2(-1.0f, -1.0f),  // 2: 左下
    float2(+1.0f, -1.0f),  // 3: 右下
};

static const float2 CORNER_UVS[4] =
{
    float2(0.0f, 0.0f),    // 0: 左上
    float2(1.0f, 0.0f),    // 1: 右上
    float2(0.0f, 1.0f),    // 2: 左下
    float2(1.0f, 1.0f),    // 3: 右下
};

// --- 速度ストレッチのパラメータ ---
static const float STRETCH_SPEED_MIN    = 2.0f;   // この速度以下は正方形
static const float STRETCH_SPEED_RANGE  = 8.0f;   // ストレッチが最大になる速度幅
static const float STRETCH_MAX          = 2.0f;   // 最大ストレッチ倍率
static const float STRETCH_SIDE_SCALE   = 0.5f;   // 横方向の縮小率
static const float STRETCH_THRESHOLD    = 0.05f;  // ストレッチ適用の閾値

// --- 血の色 ---
static const float3 FRESH_BLOOD = float3(0.70f, 0.02f, 0.02f);  // 鮮血
static const float3 DRIED_BLOOD = float3(0.15f, 0.005f, 0.005f); // 乾いた血

// --- フェードアウト ---
static const float FADE_START_LIFE = 0.3f;  // 寿命の 30% 以下でフェード開始

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput)0;

    // === パーティクル番号と角番号 ===
    uint particleIdx = vertexID / 4;
    uint cornerIdx   = vertexID % 4;

    Particle p = Particles[particleIdx];

    // === 死んだパーティクル → クリップ空間の裏側 ===
    if (p.Life <= 0.0f || p.Size <= 0.0f)
    {
        output.Position = float4(0.0f, 0.0f, -1.0f, 1.0f);
        return output;
    }

    // === 速度ストレッチ計算 ===
    float speedLen      = length(p.Velocity);
    float stretchAmount = saturate((speedLen - STRETCH_SPEED_MIN) / STRETCH_SPEED_RANGE) * STRETCH_MAX;

    // 速度方向をビュー空間に変換
    float3 velView  = mul(float4(p.Velocity, 0.0f), View).xyz;
    float2 velDir2D = normalize(velView.xy + float2(EPSILON, EPSILON));

    float2 forward = velDir2D * (1.0f + stretchAmount);
    float2 side    = float2(-velDir2D.y, velDir2D.x) * STRETCH_SIDE_SCALE;

    float2 corner = CORNER_OFFSETS[cornerIdx];
    float2 offset;

    if (stretchAmount > STRETCH_THRESHOLD)
    {
        // 速い → 細長いビルボード
        offset = (forward * corner.y + side * corner.x) * p.Size * SizeScale;
    }
    else
    {
        // 遅い → 通常の正方形ビルボード
        offset = CORNER_OFFSETS[cornerIdx] * p.Size * SizeScale;
    }

    // === ワールド座標 → クリップ空間 ===
    float3 worldPos = p.Position
                    + CameraRight * offset.x
                    + CameraUp    * offset.y;

    float4 viewPos  = mul(float4(worldPos, 1.0f), View);
    output.Position = mul(viewPos, Projection);
    output.TexCoord = CORNER_UVS[cornerIdx];

    // === 寿命に応じた血の色 ===
    float lifeRatio = saturate(p.Life / max(p.MaxLife, EPSILON));
    output.Color    = float4(lerp(DRIED_BLOOD, FRESH_BLOOD, lifeRatio),
                             saturate(lifeRatio / FADE_START_LIFE));
    output.Life     = lifeRatio;
    output.Size     = p.Size;

    return output;
}
