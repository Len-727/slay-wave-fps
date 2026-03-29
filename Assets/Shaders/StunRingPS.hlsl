// StunRingPS.hlsl - DOOM Eternal風スタンリング

cbuffer RingCB : register(b0)
{
    matrix View;
    matrix Projection;
    float3 EnemyPos;
    float RingSize;
    float Time; //  スタガー経過時間（秒）を受け取る
    float3 Padding;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.TexCoord * 2.0 - 1.0;
    float dist = length(uv);

    // saturate(Time / 0.5) で 0→1 に 0.5秒かけて変化
    // smoothstep で加速→減速のイージング
    float expandProgress = saturate(Time / 0.5);
    float eased = smoothstep(0.0, 1.0, expandProgress); // なめらかに
    float ringRadius = 0.75 * eased;

    // まだ広がってない（Time?0）なら何も描かない
    if (ringRadius < 0.01)
        discard;

    // ===  メインリング（細い）===
    float ringWidth = 0.025;
    float ring = exp(-pow(dist - ringRadius, 2) / (2.0 * ringWidth * ringWidth));

    // ===  外側ブルーム ===
    // 広がり中はブルームを強めに → 安定後は控えめに
    float bloomBoost = lerp(2.0, 1.0, eased); // 広がり中2倍、安定後1倍
    float bloomWidth = 0.12;
    float bloom = exp(-pow(dist - ringRadius, 2) / (2.0 * bloomWidth * bloomWidth)) * 0.35 * bloomBoost;

    // ===  内側フィル ===
    float innerFill = 0.0;
    if (dist < ringRadius)
    {
        float fillFade = dist / max(ringRadius, 0.01);
        innerFill = fillFade * 0.12;
    }

    // ===  脈動（安定後のみ）===
    // 広がり中は脈動なし → 安定したら呼吸するように
    float pulseStrength = saturate((Time - 0.5) / 0.3); // 0.5秒後から脈動開始
    float pulse = 1.0 - pulseStrength * 0.15 * (1.0 - sin(Time * 2.5));

    // ===  広がり中のフラッシュ（出現の瞬間に明るく光る）===
    float flashIntensity = exp(-Time * 4.0); // 0秒で最大、すぐ減衰

    // === 合成 ===
    float total = (ring + bloom + innerFill) * pulse + flashIntensity * 0.3;
    total = saturate(total);

    if (total < 0.005)
        discard;

    // === 色（DOOM Eternal風の紫?マゼンタ）===
    float coreBright = ring * pulse;

    float3 purple = float3(0.65, 0.08, 1.0);
    float3 magenta = float3(1.0, 0.25, 0.85);
    float3 white = float3(1.0, 0.9, 1.0);

    float3 color = lerp(purple, white, saturate(coreBright * 1.5));
    color += magenta * bloom * 0.4;

    // フラッシュ: 出現時に白く光る
    color += white * flashIntensity * 0.5;

    color *= pulse;

    return float4(color, total);
}

