// TargetMarkerPS.hlsl - DOOM風ロックオンマーカー
cbuffer RingCB : register(b0)
{
    matrix View;
    matrix Projection;
    float3 EnemyPos;
    float RingSize;
    float Time;
    float3 Padding;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

#define PI 3.14159265

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.TexCoord * 2.0 - 1.0;
    float dist = length(uv);
    float angle = atan2(uv.y, uv.x);

    float rot = Time * 0.6;
    float total = 0.0;

    // ===  外枠: 4セグメントに分割された円弧 ===
    // 4方向に隙間がある円（十字の隙間）
    float outerRadius = 0.88;
    float outerWidth = 0.02;
    float outerDist = abs(dist - outerRadius);

    if (outerDist < outerWidth)
    {
        // 4方向（上下左右）に隙間を作る
        float gapAngle = angle - rot;
        gapAngle = gapAngle - floor((gapAngle + PI) / (2.0 * PI)) * 2.0 * PI;

        // 0, 90, 180, 270度付近に隙間
        bool inGap = false;
        for (int g = 0; g < 4; g++)
        {
            float gapCenter = g * PI * 0.5 - PI;
            float diff = gapAngle - gapCenter;
            diff = diff - floor((diff + PI) / (2.0 * PI)) * 2.0 * PI;
            if (abs(diff) < 0.2)
                inGap = true; // 隙間の幅
        }

        if (!inGap)
        {
            float edgeSharp = 1.0 - outerDist / outerWidth;
            total += edgeSharp * 0.9;
        }
    }

    // ===  4つの三角矢印（内向き、回転する）===
    for (int i = 0; i < 4; i++)
    {
        float arrowAngle = i * PI * 0.5 + rot;

        // 矢印の位置（外円の内側）
        float arrowDist = 0.65;
        float2 arrowCenter = float2(cos(arrowAngle), sin(arrowAngle)) * arrowDist;

        // ピクセルから矢印中心への相対座標
        float2 rel = uv - arrowCenter;

        // 矢印の向き（中心に向かう）= -arrowAngle方向
        float ca = cos(-arrowAngle);
        float sa = sin(-arrowAngle);
        float2 local;
        local.x = rel.x * ca - rel.y * sa; // 矢印ローカルX（進行方向）
        local.y = rel.x * sa + rel.y * ca; // 矢印ローカルY（横方向）

        // 三角形: x > 0 で |y| < (0.08 - x * 0.8)
        float triWidth = 0.08 - local.x * 0.6;
        if (local.x > -0.04 && local.x < 0.1 && abs(local.y) < triWidth && triWidth > 0)
        {
            total += 0.85;
        }
    }

    // ===  内側の細いリング ===
    float innerRadius = 0.45;
    float innerWidth = 0.012;
    float innerLine = exp(-pow(dist - innerRadius, 2) / (2.0 * innerWidth * innerWidth)) * 0.5;
    total += innerLine;

    // ===  中央ダイヤモンド（菱形）===
    float diamond = abs(uv.x) + abs(uv.y); // マンハッタン距離 = 菱形
    float diamondSize = 0.06;
    float diamondLine = abs(diamond - diamondSize);
    if (diamondLine < 0.012)
    {
        total += 0.8 * (1.0 - diamondLine / 0.012);
    }

    // === 4本の短いクロスヘア===
    for (int c = 0; c < 4; c++)
    {
        float crossAngle = c * PI * 0.5; // 固定（回転しない）
        float aDiff = angle - crossAngle;
        aDiff = aDiff - floor((aDiff + PI) / (2.0 * PI)) * 2.0 * PI;

        if (abs(aDiff) < 0.03 && dist > 0.35 && dist < 0.45)
        {
            total += 0.6;
        }
    }

    // ===  脈動（控えめ）===
    float pulse = sin(Time * 4.0) * 0.08 + 0.92;

    total = saturate(total) * pulse;

    if (total < 0.01)
        discard;

    // === 色: ソリッドな赤）===
    float3 color = float3(1.0, 0.08, 0.03) * total * 1.5;

    if (total > 0.7)
        color = lerp(color, float3(1.0, 0.3, 0.2), (total - 0.7) / 0.3 * 0.3);

    return float4(color, total);
}
