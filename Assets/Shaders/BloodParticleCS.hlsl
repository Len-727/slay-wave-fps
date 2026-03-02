// ============================================================
//  BloodParticleCS.hlsl
//  GPU Compute Shader: 血パーティクルの物理演算
//
//  【何をしているか？】
//  毎フレーム、全パーティクルに対して：
//    1) 重力で下に引っ張る
//    2) 空気抵抗で徐々に減速
//    3) 床に当たったらバウンス
//    4) 寿命を減らす(0以下→死亡)
//
//  【たとえ】
//  ボールを空中に投げると、重力で落ち、地面で跳ね返り、
//  やがて止まる。それを4096個同時にGPUで計算する。
// ============================================================

// --- パーティクル構造体(C++側と完全一致: 48バイト) ---
struct Particle
{
    float3 position; // ワールド座標         12バイト
    float life; // 残り寿命(秒)          4バイト
    float3 velocity; // 移動方向+速度        12バイト
    float maxLife; // 初期寿命(フェード用)   4バイト
    float size; // ビルボードの大きさ     4バイト
    float3 padding; // アラインメント用      12バイト
};

// --- 読み書き可能バッファ(UAV: u0) ---
RWStructuredBuffer<Particle> Particles : register(u0);

// --- 定数バッファ(C++のUpdateCBDataと一致) ---
cbuffer UpdateCB : register(b0)
{
    float DeltaTime; // 1フレームの経過時間(秒)
    float Gravity; // 重力加速度(負の値) 例: -15.0
    float Drag; // 空気抵抗係数 例: 0.985
    float FloorY; // 床のY座標 例: 0.02
    float BounceFactor; // バウンス係数 例: 0.2
    float Time; // 累計時間(秒)
    float2 Padding; // アラインメント用
};

// 256スレッド/グループ
[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.x;
    Particle p = Particles[idx];

    // 死んでいるパーティクルはスキップ
    if (p.life <= 0.0f)
        return;

    // 1) 重力
    p.velocity.y += Gravity * DeltaTime;

    // 2) 空気抵抗
    p.velocity *= Drag;

    // 3) 位置更新
    p.position += p.velocity * DeltaTime;

    // 4) 床衝突
    if (p.position.y < FloorY)
    {
        p.position.y = FloorY;
        p.velocity.y = abs(p.velocity.y) * BounceFactor;
        p.velocity.x *= 0.8f;
        p.velocity.z *= 0.8f;
        if (abs(p.velocity.y) < 0.1f)
            p.velocity.y = 0.0f;
    }

    // 5) 寿命
    p.life -= DeltaTime;
    if (p.life <= 0.0f)
    {
        p.life = 0.0f;
        p.size = 0.0f;
        p.velocity = float3(0, 0, 0);
    }

    Particles[idx] = p;
}
