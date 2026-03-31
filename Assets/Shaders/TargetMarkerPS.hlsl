// ============================================================
//  TargetMarkerPS.hlsl
//  Gothic Swarm - DOOM 風ロックオンマーカー ピクセルシェーダー
//
//  【構成要素】
//  1. 外枠: 4 セグメント分割円弧 (十字の隙間)
//  2. 4 つの三角矢印 (内向き、回転)
//  3. 内側の細いリング
//  4. 中央ダイヤモンド (菱形)
//  5. 4 本のクロスヘア
//  6. 控えめな脈動
// ============================================================
#include "Common.hlsli"

cbuffer RingCB : register(b0)
{
    matrix View;
    matrix Projection;
    float3 EnemyPos;
    float  RingSize;
    float  Time;
    float3 Padding;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

// --- 外枠パラメータ ---
static const float OUTER_RADIUS       = 0.88f;
static const float OUTER_WIDTH        = 0.02f;
static const float GAP_HALF_ANGLE     = 0.2f;    // 隙間の半角幅
static const int   GAP_COUNT          = 4;

// --- 矢印パラメータ ---
static const float ARROW_DIST         = 0.65f;   // 矢印の配置半径
static const float ARROW_BASE_WIDTH   = 0.08f;
static const float ARROW_TAPER        = 0.6f;    // 先端の細り具合
static const float ARROW_MIN_X        = -0.04f;
static const float ARROW_MAX_X        = 0.1f;

// --- 内側リング ---
static const float INNER_RADIUS       = 0.45f;
static const float INNER_WIDTH        = 0.012f;

// --- 中央ダイヤモンド ---
static const float DIAMOND_SIZE       = 0.06f;
static const float DIAMOND_LINE_WIDTH = 0.012f;

// --- クロスヘア ---
static const float CROSS_DIST_MIN     = 0.35f;
static const float CROSS_DIST_MAX     = 0.45f;
static const float CROSS_ANGLE_TOL    = 0.03f;

// --- 脈動 ---
static const float PULSE_SPEED        = 4.0f;
static const float PULSE_AMPLITUDE    = 0.08f;

// --- 色 ---
static const float3 MARKER_RED        = float3(1.0f, 0.08f, 0.03f);
static const float3 MARKER_HIGHLIGHT  = float3(1.0f, 0.3f,  0.2f);
static const float  COLOR_INTENSITY   = 1.5f;
static const float  HIGHLIGHT_THRESHOLD = 0.7f;

// --- 回転速度 ---
static const float ROTATION_SPEED     = 0.6f;

float4 main(PSInput input) : SV_TARGET
{
    float2 uv    = input.TexCoord * 2.0f - 1.0f;
    float  dist  = length(uv);
    float  angle = atan2(uv.y, uv.x);
    float  rot   = Time * ROTATION_SPEED;
    float  total = 0.0f;

    // === 1) 外枠: 4 セグメント円弧 ===
    float outerDist = abs(dist - OUTER_RADIUS);
    if (outerDist < OUTER_WIDTH)
    {
        float gapAngle = angle - rot;
        gapAngle -= floor((gapAngle + PI) / TWO_PI) * TWO_PI;

        bool inGap = false;
        [unroll]
        for (int g = 0; g < GAP_COUNT; g++)
        {
            float gapCenter = (float)g * HALF_PI - PI;
            float diff = gapAngle - gapCenter;
            diff -= floor((diff + PI) / TWO_PI) * TWO_PI;
            if (abs(diff) < GAP_HALF_ANGLE)
                inGap = true;
        }

        if (!inGap)
        {
            float edgeSharp = 1.0f - outerDist / OUTER_WIDTH;
            total += edgeSharp * 0.9f;
        }
    }

    // === 2) 4 つの三角矢印 (回転) ===
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float arrowAngle  = (float)i * HALF_PI + rot;
        float2 arrowCenter = float2(cos(arrowAngle), sin(arrowAngle)) * ARROW_DIST;
        float2 rel = uv - arrowCenter;

        float ca = cos(-arrowAngle);
        float sa = sin(-arrowAngle);
        float2 local;
        local.x = rel.x * ca - rel.y * sa;
        local.y = rel.x * sa + rel.y * ca;

        float triWidth = ARROW_BASE_WIDTH - local.x * ARROW_TAPER;
        if (local.x > ARROW_MIN_X && local.x < ARROW_MAX_X
            && abs(local.y) < triWidth && triWidth > 0.0f)
        {
            total += 0.85f;
        }
    }

    // === 3) 内側リング (ガウシアン) ===
    float innerLine = exp(-pow(dist - INNER_RADIUS, 2.0f)
                    / (2.0f * INNER_WIDTH * INNER_WIDTH)) * 0.5f;
    total += innerLine;

    // === 4) 中央ダイヤモンド (マンハッタン距離) ===
    float diamond     = abs(uv.x) + abs(uv.y);
    float diamondLine = abs(diamond - DIAMOND_SIZE);
    if (diamondLine < DIAMOND_LINE_WIDTH)
    {
        total += 0.8f * (1.0f - diamondLine / DIAMOND_LINE_WIDTH);
    }

    // === 5) クロスヘア (固定、非回転) ===
    [unroll]
    for (int c = 0; c < 4; c++)
    {
        float crossAngle = (float)c * HALF_PI;
        float aDiff = angle - crossAngle;
        aDiff -= floor((aDiff + PI) / TWO_PI) * TWO_PI;

        if (abs(aDiff) < CROSS_ANGLE_TOL
            && dist > CROSS_DIST_MIN && dist < CROSS_DIST_MAX)
        {
            total += 0.6f;
        }
    }

    // === 6) 脈動 ===
    float pulse = sin(Time * PULSE_SPEED) * PULSE_AMPLITUDE + (1.0f - PULSE_AMPLITUDE);
    total = saturate(total) * pulse;

    if (total < ALPHA_CUTOFF)
        discard;

    // === 色: ソリッドな赤 ===
    float3 color = MARKER_RED * total * COLOR_INTENSITY;
    if (total > HIGHLIGHT_THRESHOLD)
    {
        float t = (total - HIGHLIGHT_THRESHOLD) / (1.0f - HIGHLIGHT_THRESHOLD);
        color = lerp(color, MARKER_HIGHLIGHT, t * 0.3f);
    }

    return float4(color, total);
}
