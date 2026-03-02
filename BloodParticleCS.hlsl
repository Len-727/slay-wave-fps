// ============================================================
//  BloodParticleCS.hlsl
//  Compute Shader - GPUで血パーティクルの物理演算を並列処理
//
//  1スレッド = 1パーティクル
//  更新内容: 位置、速度(重力+空気抵抗)、寿命
//  死んだパーティクル: life <= 0 -> size = 0 (見えなくなる)
// ============================================================

// --- パーティクル構造体 (C++側と完全一致させる) ---
struct Particle
{
    float3 position; // ワールド座標
    float life; // 残り寿命(秒)
    float3 velocity; // 移動方向 + 速度
    float maxLife; // 初期寿命(フェード計算用)
    float size; // ビルボードの大きさ
    float3 padding; // 48バイトにアラインメント
};

// --- 読み書き可能バッファ: GPUが毎フレーム読み書きする ---
RWStructuredBuffer<Particle> Particles : register(u0);

// --- CPUから毎フレーム送られる定数 ---
cbuffer UpdateCB : register(b0)
{
    float DeltaTime; // フレーム時間(秒)
    float Gravity; // 重力の強さ(負の値 = 下向き)
    float Drag; // 空気抵抗(0.98 = 軽い抵抗)
    float FloorY; // 床の高さ(これ以下でバウンス/停止)
    float BounceFactor; // バウンス後に残る速度の割合(0.3 = 30%)
    float Time; // 経過時間の合計
    float2 Padding; // 32バイトにアラインメント
};

// --- 1グループ256スレッド(パーティクル処理の標準) ---
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    Particle p = Particles[id.x];

    // === 死んだパーティクルはスキップ ===
    if (p.life <= 0.0f)
    {
        p.size = 0.0f; // 見えなくする
        Particles[id.x] = p;
        return;
    }

    // === 寿命を減らす ===
    p.life -= DeltaTime;

    // === 重力を適用(Y軸下向き) ===
    p.velocity.y += Gravity * DeltaTime;

    // === 空気抵抗を適用(徐々に減速) ===
    p.velocity *= Drag;

    // === 位置を更新 ===
    p.position += p.velocity * DeltaTime;

    // === 床との衝突判定(簡易バウンス) ===
    if (p.position.y < FloorY)
    {
        p.position.y = FloorY;
        p.velocity.y = -p.velocity.y * BounceFactor;

        // 床に着いたら水平速度を大幅に減衰(血は床に張り付く)
        p.velocity.x *= 0.3f;
        p.velocity.z *= 0.3f;

        // ほぼ動いてなかったら完全停止
        if (abs(p.velocity.y) < 0.5f)
        {
            p.velocity = float3(0, 0, 0);
            p.life = min(p.life, 2.0f); // 床に落ちたらすぐ消える
        }
    }

    // === 死ぬ直前にサイズを縮小(フェードアウト) ===
    float lifeRatio = saturate(p.life / p.maxLife);
    // 寿命の80%までフルサイズ、残り20%で縮小
    float sizeFade = saturate(lifeRatio * 5.0f);
    p.size *= sizeFade > 0.99f ? 1.0f : sizeFade;

    // === 結果をバッファに書き戻す ===
    Particles[id.x] = p;
}
