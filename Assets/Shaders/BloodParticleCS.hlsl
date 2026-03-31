// ============================================================
//  BloodParticleCS.hlsl
//  Gothic Swarm - GPU Compute Shader: 血パーティクルの物理演算
//
//  【役割】
//  毎フレーム全パーティクルに対して物理演算を実行:
//    1) 重力で下に引っ張る
//    2) 空気抵抗で徐々に減速
//    3) 床に当たったらバウンス
//    4) 寿命を減らす (0 以下 → 死亡)
//
//  【スレッドグループ】
//  256 スレッド/グループ × ceil(MAX_PARTICLES / 256) グループ
// ============================================================

// --- パーティクル構造体 (C++ 側と完全一致: 48 バイト) ---
struct Particle
{
    float3 Position;  // ワールド座標           12 bytes
    float  Life;      // 残り寿命 (秒)            4 bytes
    float3 Velocity;  // 移動方向 + 速度        12 bytes
    float  MaxLife;   // 初期寿命 (フェード用)    4 bytes
    float  Size;      // ビルボードの大きさ       4 bytes
    float3 Padding;   // 16 バイト境界揃え       12 bytes
};

// --- UAV: 読み書き可能パーティクルバッファ ---
RWStructuredBuffer<Particle> Particles : register(u0);

// --- 定数バッファ (C++ の UpdateCBData と一致) ---
cbuffer UpdateCB : register(b0)
{
    float  DeltaTime;     // 1 フレームの経過時間 (秒)
    float  Gravity;       // 重力加速度 (負の値) 例: -15.0
    float  Drag;          // 空気抵抗係数 例: 0.985
    float  FloorY;        // 床の Y 座標 例: 0.02
    float  BounceFactor;  // バウンス減衰 例: 0.2
    float  Time;          // 累計時間 (秒)
    float2 Padding;       // アラインメント用
};

// --- 物理定数 ---
static const float FLOOR_FRICTION   = 0.8f;   // 床接触時の水平減速率
static const float BOUNCE_THRESHOLD = 0.1f;   // これ以下のバウンス速度は停止

// === ディスパッチ: 256 スレッド / グループ ===
[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.x;
    Particle p = Particles[idx];

    // 死んだパーティクルはスキップ
    if (p.Life <= 0.0f)
        return;

    // 1) 重力
    p.Velocity.y += Gravity * DeltaTime;

    // 2) 空気抵抗
    p.Velocity *= Drag;

    // 3) 位置更新
    p.Position += p.Velocity * DeltaTime;

    // 4) 床衝突
    if (p.Position.y < FloorY)
    {
        p.Position.y = FloorY;
        p.Velocity.y = abs(p.Velocity.y) * BounceFactor;
        p.Velocity.xz *= FLOOR_FRICTION;

        if (abs(p.Velocity.y) < BOUNCE_THRESHOLD)
            p.Velocity.y = 0.0f;
    }

    // 5) 寿命管理
    p.Life -= DeltaTime;
    if (p.Life <= 0.0f)
    {
        p.Life     = 0.0f;
        p.Size     = 0.0f;
        p.Velocity = float3(0.0f, 0.0f, 0.0f);
    }

    Particles[idx] = p;
}
