// ============================================================
//  StunRingPS.hlsl
//  Gothic Swarm - DOOM Eternal 風スタンリング ピクセルシェーダー
//
//  【演出フロー】
//  1. 出現時: 0.5 秒かけてリングが展開 + フラッシュ
//  2. 安定後: 呼吸するような脈動 + ブルーム
//  3. 色:     紫→マゼンタ→白 のグラデーション
// ============================================================
#include "Common.hlsli"

cbuffer RingCB : register(b0)
{
    matrix View;
    matrix Projection;
    float3 EnemyPos;
    float  RingSize;
    float  Time;         // スタガー経過時間 (秒)
    float3 Padding;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// --- リングのパラメータ ---
static const float EXPAND_DURATION  = 0.5f;   // 展開にかかる秒数
static const float RING_RADIUS      = 0.75f;  // 最終リング半径 (UV 空間)
static const float RING_WIDTH       = 0.025f;  // メインリングの太さ
static const float BLOOM_WIDTH      = 0.12f;   // ブルームの広がり
static const float BLOOM_BASE       = 0.35f;   // ブルームの基本強度

// --- 脈動パラメータ ---
static const float PULSE_DELAY      = 0.5f;   // 脈動開始までの遅延 (秒)
static const float PULSE_FADE_TIME  = 0.3f;   // 脈動フェードイン時間
static const float PULSE_SPEED      = 2.5f;   // 脈動の速度
static const float PULSE_AMPLITUDE  = 0.15f;  // 脈動の振幅

// --- フラッシュパラメータ ---
static const float FLASH_DECAY      = 4.0f;   // フラッシュの減衰速度
static const float FLASH_INTENSITY  = 0.3f;

// --- インナーフィル ---
static const float INNER_FILL_ALPHA = 0.12f;

// --- 色 ---
static const float3 COLOR_PURPLE  = float3(0.65f, 0.08f, 1.0f);
static const float3 COLOR_MAGENTA = float3(1.0f,  0.25f, 0.85f);
static const float3 COLOR_WHITE   = float3(1.0f,  0.9f,  1.0f);

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.TexCoord * 2.0f - 1.0f; // -1〜+1 の座標系
    float dist = length(uv); // 中心からの距離

    // === 展開アニメーション    ===
    float expandProgress = saturate(Time / EXPAND_DURATION);
    float eased = smoothstep(0.0f, 1.0f, expandProgress);
    float crossSize = RING_RADIUS * eased; // ✕の大きさ（展開中は小さい→大きく）

    if (crossSize < ALPHA_CUTOFF)
        discard;

    // ==============================================
    //  ✕ 形状の生成
    //  2本の対角線（y=x と y=-x）からの距離を使う
    // ==============================================

    // 各対角線からの距離（1.414 = √2）
    float distLine1 = abs(uv.x - uv.y) / 1.414f; // ＼方向の線
    float distLine2 = abs(uv.x + uv.y) / 1.414f; // ／方向の線

    // 2本のうち近い方 = ✕ 形状
    float lineDist = min(distLine1, distLine2);

    // 腕の長さを制限（crossSizeより外はフェードアウト）
    float lengthMask = smoothstep(crossSize + 0.05f, crossSize - 0.1f, dist);

    // === メイン✕ライン（ガウシアン分布で太さを制御）===
    float xLine = exp(-lineDist * lineDist / (2.0f * RING_WIDTH * RING_WIDTH))
                * lengthMask;

    // === ブルーム（✕の周りのぼんやりした光）===
    float bloomBoost = lerp(2.0f, 1.0f, eased);
    float bloom = exp(-lineDist * lineDist / (2.0f * BLOOM_WIDTH * BLOOM_WIDTH))
                * lengthMask * BLOOM_BASE * bloomBoost;

    // === 中心の光点（✕の交差点を強調）===
    float centerGlow = exp(-dist * dist / 0.03f) * 0.5f * eased;

    // === 脈動   ===
    float pulseStrength = saturate((Time - PULSE_DELAY) / PULSE_FADE_TIME);
    float pulse = 1.0f - pulseStrength * PULSE_AMPLITUDE * (1.0f - sin(Time * PULSE_SPEED));

    // === 出現フラッシュ  ===
    float flashIntensity = exp(-Time * FLASH_DECAY);

    // === 合成 ===
    float total = (xLine + bloom + centerGlow) * pulse + flashIntensity * FLASH_INTENSITY;
    total = saturate(total);

    if (total < 0.005f)
        discard;

    // === 色    ==
    float coreBright = xLine * pulse;
    float3 color = lerp(COLOR_PURPLE, COLOR_WHITE, saturate(coreBright * 1.5f));
    color += COLOR_MAGENTA * bloom * 0.4f;
    color += COLOR_WHITE * (flashIntensity * 0.5f + centerGlow);
    color *= pulse;

    return float4(color, total);
}