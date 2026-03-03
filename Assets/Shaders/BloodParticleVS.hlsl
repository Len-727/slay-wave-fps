// ============================================================
//  BloodParticleVS.hlsl
//  GPU Vertex Shader: StructuredBufferからビルボードを生成
//
//  【何をしているか？】
//  SV_VertexIDから「どのパーティクルの、どの角か」を計算し、
//  カメラに向いた四角形(ビルボード)の頂点を生成する。
//
//  【たとえ】
//  看板が常にカメラに向くように回転する。
//  GPUが4096枚の看板を一瞬で配置する。
//
//  【頂点の対応】
//  インデックスバッファ: particle i → vertex i*4+0,1,2,3
//  corner 0: 左上(-1,+1)  UV(0,0)
//  corner 1: 右上(+1,+1)  UV(1,0)
//  corner 2: 左下(-1,-1)  UV(0,1)
//  corner 3: 右下(+1,-1)  UV(1,1)
// ============================================================

// --- パーティクル構造体(C++/CSと完全一致: 48バイト) ---
struct Particle
{
    float3 position; // ワールド座標
    float life; // 残り寿命(秒)
    float3 velocity; // 速度
    float maxLife; // 初期寿命
    float size; // ビルボードの大きさ
    float3 padding; // アラインメント
};

// --- StructuredBuffer(SRV t0: CSが書いたパーティクルデータ) ---
StructuredBuffer<Particle> Particles : register(t0);

// --- カメラ定数バッファ(b0) ---
cbuffer CameraCB : register(b0)
{
    float4x4 View; // ビュー行列(転置済み)
    float4x4 Projection; // プロジェクション行列(転置済み)
    float3 CameraRight; // カメラの右方向ベクトル
    float Time; // 累計時間
    float3 CameraUp; // カメラの上方向ベクトル
    float SizeScale;
};

// --- ピクセルシェーダーへの出力 ---
struct VSOutput
{
    float4 position : SV_POSITION; // スクリーン座標
    float2 texcoord : TEXCOORD0; // UV座標(0~1)
    float4 color : COLOR0; // パーティクルの色
    float life : TEXCOORD1; // 寿命の割合(0~1)
    float size : TEXCOORD2;
};

// === 4つの角のオフセットテーブル ===
// corner 0: 左上, 1: 右上, 2: 左下, 3: 右下
static const float2 cornerOffsets[4] =
{
    float2(-1.0f, +1.0f), // 0: 左上
    float2(+1.0f, +1.0f), // 1: 右上
    float2(-1.0f, -1.0f), // 2: 左下
    float2(+1.0f, -1.0f), // 3: 右下
};

static const float2 cornerUVs[4] =
{
    float2(0.0f, 0.0f), // 0: 左上
    float2(1.0f, 0.0f), // 1: 右上
    float2(0.0f, 1.0f), // 2: 左下
    float2(1.0f, 1.0f), // 3: 右下
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput) 0;

    // === どのパーティクルの、どの角か？ ===
    // インデックスバッファが base=i*4 のオフセットを渡すので
    // vertexID / 4 = パーティクル番号
    // vertexID % 4 = 角番号(0~3)
    uint particleIdx = vertexID / 4;
    uint cornerIdx = vertexID % 4;

    // パーティクルデータを読む
    Particle p = Particles[particleIdx];

    // === 死んだパーティクルはスクリーン外に飛ばす ===
    if (p.life <= 0.0f || p.size <= 0.0f)
    {
        // NaNや0除算を避けつつ、描画されない位置に置く
        output.position = float4(0, 0, -1, 1); // クリップ空間の裏側
        output.texcoord = float2(0, 0);
        output.color = float4(0, 0, 0, 0);
        output.life = 0.0f;
        return output;
    }

    // === ビルボード頂点位置の計算 ===
    // 速度ストレッチ: 速いパーティクルは進行方向に細長くなる
    float3 vel = p.velocity;
    float speedLen = length(vel);
    
    // speedLen > 2.0 なら進行方向に伸ばす（遅いやつは普通の正方形）
    float stretchAmount = saturate((speedLen - 2.0f) / 8.0f) * 2.0f;
    // stretchAmount: 0（遅い=正方形）～ 2.0（速い=3倍に伸びる）

    // 速度方向をビュー空間に変換
    float3 velView = mul(float4(vel, 0.0f), View).xyz;
    float2 velDir2D = normalize(velView.xy + float2(0.001f, 0.001f));
    
    // 進行方向ベクトルと直交ベクトル
    float2 forward = velDir2D * (1.0f + stretchAmount); // 伸ばす
    float2 side = float2(-velDir2D.y, velDir2D.x) * 0.5f; // 横は細く
    
    float2 corner = cornerOffsets[cornerIdx];
    float2 offset;
    
    if (stretchAmount > 0.05f)
    {
        // 速い → 細長いビルボード
        offset = (forward * corner.y + side * corner.x) * p.size * SizeScale;
    }
    else
    {
        // 遅い → 通常の正方形ビルボード（ミスト等）
        offset = cornerOffsets[cornerIdx] * p.size * SizeScale;
    }

    float3 worldPos = p.position
                    + CameraRight * offset.x
                    + CameraUp * offset.y;

    // === ワールド → クリップ空間に変換 ===
    float4 viewPos = mul(float4(worldPos, 1.0f), View);
    output.position = mul(viewPos, Projection);

    // === UV座標 ===
    output.texcoord = cornerUVs[cornerIdx];

    // === 色: 寿命に応じた血の色 ===
    float lifeRatio = saturate(p.life / max(p.maxLife, 0.001f));

    // 鮮やかな赤 → ドス黒い赤に変化
    float3 freshBlood = float3(0.7f, 0.02f, 0.02f); // 鮮血
    float3 darkBlood = float3(0.15f, 0.005f, 0.005f); // 乾いた血
    float3 bloodColor = lerp(darkBlood, freshBlood, lifeRatio);

    // アルファ: 最後の30%でフェードアウト
    float alpha = saturate(lifeRatio * 3.33f); // life 30%以下でフェード

    output.color = float4(bloodColor, alpha);
    output.life = lifeRatio;
    output.size = p.size;

    return output;
}
